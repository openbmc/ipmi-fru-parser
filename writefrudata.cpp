#include "writefrudata.hpp"

#include "fru_area.hpp"
#include "frup.hpp"
#include "types.hpp"

#include <host-ipmid/ipmid-api.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sstream>
#include <vector>

using namespace ipmi::vpd;
using namespace phosphor::logging;

extern const FruMap frus;
extern const std::map<Path, InterfaceMap> extras;

using FruAreaVector = std::vector<std::unique_ptr<IPMIFruArea>>;

namespace
{

/**
 * Cleanup routine
 * Must always be called as last reference to fruFilePointer.
 *
 * @param[in] fruFilePointer - FRU file pointer to close
 * @param[in] fruAreaVec - vector of FRU areas
 * @return -1
 */
int cleanupError(FILE* fruFilePointer, FruAreaVector& fruAreaVec)
{
    if (fruFilePointer != NULL)
    {
        std::fclose(fruFilePointer);
    }

    if (!(fruAreaVec.empty()))
    {
        fruAreaVec.clear();
    }

    return -1;
}

/**
 * Gets the value of the key from the FRU dictionary of the given section.
 * FRU dictionary is parsed FRU data for all the sections.
 *
 * @param[in] section - FRU section name
 * @param[in] key - key for secion
 * @param[in] delimiter - delimiter for parsing custom fields
 * @param[in] fruData - the FRU data to search for the section
 * @return FRU value
 */
std::string getFRUValue(const std::string& section, const std::string& key,
                        const std::string& delimiter, IPMIFruInfo& fruData)
{

    auto minIndexValue = 0;
    auto maxIndexValue = 0;
    std::string fruValue = "";

    if (section == "Board")
    {
        minIndexValue = OPENBMC_VPD_KEY_BOARD_MFG_DATE;
        maxIndexValue = OPENBMC_VPD_KEY_BOARD_MAX;
    }
    else if (section == "Product")
    {
        minIndexValue = OPENBMC_VPD_KEY_PRODUCT_MFR;
        maxIndexValue = OPENBMC_VPD_KEY_PRODUCT_MAX;
    }
    else if (section == "Chassis")
    {
        minIndexValue = OPENBMC_VPD_KEY_CHASSIS_TYPE;
        maxIndexValue = OPENBMC_VPD_KEY_CHASSIS_MAX;
    }

    auto first = fruData.cbegin() + minIndexValue;
    auto last = first + (maxIndexValue - minIndexValue) + 1;

    auto itr = std::find_if(first, last,
                            [&key](const auto& e) { return key == e.first; });

    if (itr != last)
    {
        fruValue = itr->second;
    }

    // if the key is custom property then the value could be in two formats.
    // 1) custom field 2 = "value".
    // 2) custom field 2 =  "key:value".
    // if delimiter length = 0 i.e custom field 2 = "value"

    constexpr auto customProp = "Custom Field";
    if (key.find(customProp) != std::string::npos)
    {
        if (delimiter.length() > 0)
        {
            size_t delimiterpos = fruValue.find(delimiter);
            if (delimiterpos != std::string::npos)
            {
                fruValue = fruValue.substr(delimiterpos + 1);
            }
        }
    }
    return fruValue;
}

/**
 * Get the inventory service from the mapper.
 *
 * @param[in] bus - sdbusplus handle to use for dbus call
 * @param[in] intf - interface
 * @param[in] path - the object path
 * @return the dbus service that owns the interface for that path
 */
auto getService(sdbusplus::bus::bus& bus, const std::string& intf,
                const std::string& path)
{
    auto mapperCall =
        bus.new_method_call("xyz.openbmc_project.ObjectMapper",
                            "/xyz/openbmc_project/object_mapper",
                            "xyz.openbmc_project.ObjectMapper", "GetObject");

    mapperCall.append(path);
    mapperCall.append(std::vector<std::string>({intf}));
    std::map<std::string, std::vector<std::string>> mapperResponse;

    try
    {
        auto mapperResponseMsg = bus.call(mapperCall);
        mapperResponseMsg.read(mapperResponse);
    }
    catch (const sdbusplus::exception::SdBusError& ex)
    {
        log<level::ERR>("Exception from sdbus call",
                        entry("WHAT=%s", ex.what()));
        throw;
    }

    if (mapperResponse.begin() == mapperResponse.end())
    {
        throw std::runtime_error("ERROR in reading the mapper response");
    }

    return mapperResponse.begin()->first;
}

// MultiRecord area record name and num matching.
// It is used by ipmiUpdateMultirecord function.
std::map<std::string, IpmiMultirecordType> multirecordMap = {
    {"PowerSupply", POWER_SUPPLY_INFO}, {"DcOutput", DC_OUTPUT}};

/**
 * Adding property values for Power Supply Information record
 *
 * @param[in] data - readed data
 * @param[in,out] props - DBus properties map
 */
void ipmiUpdatePsuInfo(const PowerSupplyInfo& data, PropertyMap& props)
{
    props.emplace("OverallCapacity",
                  static_cast<int64_t>(data.overallCapacity));
    props.emplace("PeakVA", static_cast<int64_t>(data.peakVa));
    props.emplace("InrushCurrent", static_cast<int64_t>(data.inrushCurrent));
    props.emplace("InrushInterval", static_cast<int64_t>(data.inrushInterval));
    props.emplace("LowInputVoltage1", static_cast<int64_t>(data.lowInVoltage1));
    props.emplace("HighInputVoltage1",
                  static_cast<int64_t>(data.highInVoltage1));
    props.emplace("LowInputVoltage2", static_cast<int64_t>(data.lowInVoltage2));
    props.emplace("HighInputVoltage2",
                  static_cast<int64_t>(data.highInVoltage2));
    props.emplace("LowInputFrequency",
                  static_cast<int64_t>(data.lowInFrequency));
    props.emplace("HighInputFrequency",
                  static_cast<int64_t>(data.highInFrequency));
    props.emplace("InDropoutTolerance",
                  static_cast<int64_t>(data.inDropoutTolerance));
    props.emplace("HoldUpTime", static_cast<int64_t>(data.holdUpTime()));
    props.emplace("PeakCapacity", static_cast<int64_t>(data.peakCapacity()));
    props.emplace("Voltage1", static_cast<int64_t>(data.voltage1()));
    props.emplace("Voltage2", static_cast<int64_t>(data.voltage2()));
    props.emplace("TotalWattage", static_cast<int64_t>(data.totalWattage));
    props.emplace("TachometerLow", static_cast<int64_t>(data.tachometerLow));
    props.emplace("HotSwap", data.hotSwap());
    props.emplace("PinPolarity", data.pinPolarity());
    props.emplace("Autoswitch", data.autoswitch());
    props.emplace("PowerFactor", data.powerCorrection());
    props.emplace("PredictiveFail", data.predictiveFail());
}

/**
 * Adding property values for DC Output record
 *
 * @param data[in] - readed data
 * @param props[in,out] - DBus properties map
 */
void ipmiUpdateDcLoad(const DcOutputInfo& data, PropertyMap& props)
{
    props.emplace("OutputNumber", static_cast<int64_t>(data.outNumber()));
    props.emplace("NominalVoltage", static_cast<int64_t>(data.nominalVoltage));
    props.emplace("MaxNegativeVoltage",
                  static_cast<int64_t>(data.maxNegativeVoltage));
    props.emplace("MaxPositiveVoltage",
                  static_cast<int64_t>(data.maxPositiveVoltage));
    props.emplace("RippleAndNoise", static_cast<int64_t>(data.rippleAndNoise));
    props.emplace("MinCurrentDraw", static_cast<int64_t>(data.minCurrentDraw));
    props.emplace("MaxCurrentDraw", static_cast<int64_t>(data.maxCurrentDraw));
    props.emplace("Standby", data.standby());
}

/**
 * Takes FRU data, invokes Parser for each FRU record area and updates
 * inventory.
 *
 * @param[in] areaVector - vector of FRU areas
 * @param[in] bus - handle to sdbus for calling methods, etc
 * @return return non-zero of failure
 */
int updateInventory(FruAreaVector& areaVector, sdbusplus::bus::bus& bus)
{
    // Generic error reporter
    int rc = 0;
    uint8_t fruid = 0;
    IPMIFruInfo fruData;
    IPMIMultiInfo multiData;

    // For each FRU area, extract the needed data , get it parsed and update
    // the Inventory.
    for (const auto& fruArea : areaVector)
    {
        fruid = fruArea->getFruID();
        // Fill the container with information
        rc = parse_fru_area(fruArea->getType(),
                            static_cast<const void*>(fruArea->getData()),
                            fruArea->getLength(), fruData, multiData);
        if (rc < 0)
        {
            log<level::ERR>("Error parsing FRU records");
            return rc;
        }
    } // END walking the vector of areas and updating

    // For each FRU we have the list of instances which needs to be updated.
    // Each instance object implements certain interfaces.
    // Each Interface is having Dbus properties.
    // Each Dbus Property would be having metaData(eg section,VpdPropertyName).

    // Here we are just printing the object,interface and the properties.
    // which needs to be called with the new inventory manager implementation.
    using namespace std::string_literals;
    static const auto intf = "xyz.openbmc_project.Inventory.Manager"s;
    static const auto path = "/xyz/openbmc_project/inventory"s;
    std::string service;
    try
    {
        service = getService(bus, intf, path);
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << "\n";
        return -1;
    }

    auto iter = frus.find(fruid);
    if (iter == frus.end())
    {
        log<level::ERR>("Unable to get the FRU info",
                        entry("FRU=%d", static_cast<int>(fruid)));
        return -1;
    }

    auto& instanceList = iter->second;
    if (instanceList.size() <= 0)
    {
        log<level::DEBUG>("Object list empty for this FRU",
                          entry("FRU=%d", static_cast<int>(fruid)));
    }

    ObjectMap objects;
    for (const auto& instance : instanceList)
    {
        InterfaceMap interfaces;
        const auto& extrasIter = extras.find(instance.path);
        bool skipObject = false;

        for (const auto& interfaceList : instance.interfaces)
        {
            PropertyMap props; // store all the properties
            for (const auto& properties : interfaceList.second)
            {
                std::string value;
                decltype(auto) pdata = properties.second;

                // if object refers to multirecord data, skip it
                if (pdata.section == "MultiRecord" &&
                    properties.first == "Data")
                {
                    auto mapIt = multirecordMap.find(pdata.property);
                    if (mapIt == multirecordMap.end())
                        continue;

                    int i = 0;
                    for (auto& mr : multiData)
                    {
                        if (mr.type == mapIt->second)
                        {
                            PropertyMap props;
                            InterfaceMap interfaces;
                            switch (mapIt->second)
                            {
                                case POWER_SUPPLY_INFO:
                                    ipmiUpdatePsuInfo(mr.data.powerSupplyInfo,
                                                      props);
                                    break;
                                case DC_OUTPUT:
                                    ipmiUpdateDcLoad(mr.data.dcOutputInfo,
                                                     props);
                                    break;
                                default:
                                    continue;
                                    break;
                            }
                            interfaces.emplace(std::move(interfaceList.first),
                                               std::move(props));
                            sdbusplus::message::object_path path =
                                instance.path + std::to_string(i);
                            objects.emplace(path, interfaces);
                            ++i;
                        }
                    }

                    skipObject = true;
                    continue;
                }

                if (!pdata.section.empty() && !pdata.property.empty())
                {
                    value = getFRUValue(pdata.section, pdata.property,
                                        pdata.delimiter, fruData);
                }
                props.emplace(std::move(properties.first), std::move(value));
            }
            // Check and update extra properties
            if (extras.end() != extrasIter)
            {
                const auto& propsIter =
                    (extrasIter->second).find(interfaceList.first);
                if ((extrasIter->second).end() != propsIter)
                {
                    for (const auto& map : propsIter->second)
                    {
                        props.emplace(map.first, map.second);
                    }
                }
            }
            interfaces.emplace(std::move(interfaceList.first),
                               std::move(props));
        }

        // Call the inventory manager
        sdbusplus::message::object_path objectPath = instance.path;
        // Check and update extra properties
        if (extras.end() != extrasIter)
        {
            for (const auto& entry : extrasIter->second)
            {
                if (interfaces.end() == interfaces.find(entry.first))
                {
                    interfaces.emplace(entry.first, entry.second);
                }
            }
        }

        if (!skipObject)
        {
            objects.emplace(objectPath, interfaces);
        }
    }

    auto pimMsg = bus.new_method_call(service.c_str(), path.c_str(),
                                      intf.c_str(), "Notify");
    pimMsg.append(std::move(objects));

    try
    {
        auto inventoryMgrResponseMsg = bus.call(pimMsg);
    }
    catch (const sdbusplus::exception::SdBusError& ex)
    {
        log<level::ERR>("Error in notify call", entry("WHAT=%s", ex.what()));
        return -1;
    }

    return rc;
}

} // namespace

/**
 * Take a pointer to an array of bytes, and a length,
 * and return the 8-bit checksum as per IPMI Platform Management Information
 * Storage Definition v1.0 document revision 1.2.
 *
 * @param[in] data - The pointer to the array of bytes
 * @param[in] len - The length of the array
 * @return the checksum value
 */
uint8_t calculateChecksum(const unsigned char* data, size_t len)
{
    char crc = 0;
    size_t byte = 0;

    for (byte = 0; byte < len; byte++)
    {
        crc += *data++;
    }

    return (-crc);
}

/**
 * Accepts a FRU area offset into a commom header and tells which area it is.
 *
 * @param[in] areaOffset - offset to lookup the area type
 * @return the ipmi_fru_area_type
 */
ipmi_fru_area_type getFruAreaType(uint8_t areaOffset)
{
    ipmi_fru_area_type type = IPMI_FRU_AREA_TYPE_MAX;

    switch (areaOffset)
    {
        case IPMI_FRU_INTERNAL_OFFSET:
            type = IPMI_FRU_AREA_INTERNAL_USE;
            break;

        case IPMI_FRU_CHASSIS_OFFSET:
            type = IPMI_FRU_AREA_CHASSIS_INFO;
            break;

        case IPMI_FRU_BOARD_OFFSET:
            type = IPMI_FRU_AREA_BOARD_INFO;
            break;

        case IPMI_FRU_PRODUCT_OFFSET:
            type = IPMI_FRU_AREA_PRODUCT_INFO;
            break;

        case IPMI_FRU_MULTI_OFFSET:
            type = IPMI_FRU_AREA_MULTI_RECORD;
            break;

        default:
            type = IPMI_FRU_AREA_TYPE_MAX;
    }

    return type;
}

/**
 * Validates the data for checksum and mandatory fields.
 *
 * @param[in] data - the data to verify
 * @param[in] len - the length of the region to verify
 * @param[in] useChecksum - whether or not to verify the checksum for the area
 * @return non-zero on failure
 */
static int verifyFruData(const uint8_t* data, const size_t len,
                         bool useChecksum)
{
    uint8_t checksum = 0;
    int rc = -1;

    // Validate for first byte to always have a value of [1]
    if (data[0] != IPMI_FRU_HDR_BYTE_ZERO)
    {
        log<level::ERR>("Invalid entry in byte-0",
                        entry("ENTRY=0x%X", static_cast<uint32_t>(data[0])));
        return rc;
    }
#ifdef __IPMI_DEBUG__
    else
    {
        log<level::DEBUG>("Validated in entry_1 of fruData",
                          entry("ENTRY=0x%X", static_cast<uint32_t>(data[0])));
    }
#endif

    // If the area (e.g. Internal Area) doesn't need checksum checking,
    // then we have nothing to do and report a success once we get here.
    if (!useChecksum)
    {
        return EXIT_SUCCESS;
    }

    // See if the calculated CRC matches with the embedded one.
    // CRC to be calculated on all except the last one that is CRC itself.
    checksum = calculateChecksum(data, len - 1);
    if (checksum != data[len - 1])
    {
#ifdef __IPMI_DEBUG__
        log<level::ERR>(
            "Checksum mismatch",
            entry("Calculated=0x%X", static_cast<uint32_t>(checksum)),
            entry("Embedded=0x%X", static_cast<uint32_t>(data[len])));
#endif
        return rc;
    }
#ifdef __IPMI_DEBUG__
    else
    {
        log<level::DEBUG>("Checksum matches");
    }
#endif

    return EXIT_SUCCESS;
}

/**
 * Checks if a particular FRU area is populated or not.
 *
 * @param[in] reference to FRU area pointer
 * @return true if the area is empty
 */
bool removeInvalidArea(const std::unique_ptr<IPMIFruArea>& fruArea)
{
    // Filter the ones that are empty
    if (!(fruArea->getLength()))
    {
        return true;
    }
    return false;
}

/**
 * Populates various FRU areas.
 *
 * @prereq : This must be called only after validating common header
 * @param[in] fruData - pointer to the FRU bytes
 * @param[in] dataLen - the length of the FRU data
 * @param[in] fruAreaVec - the FRU area vector to update
 */
int ipmiPopulateFruAreas(uint8_t* fruData, const size_t dataLen,
                         FruAreaVector& fruAreaVec)
{
    int rc = -1;

    // Now walk the common header and see if the file size has atleast the last
    // offset mentioned by the struct common_header. If the file size is less
    // than the offset of any if the FRU areas mentioned in the common header,
    // then we do not have a complete file.
    for (uint8_t fruEntry = IPMI_FRU_INTERNAL_OFFSET;
         fruEntry <= IPMI_FRU_MULTI_OFFSET; ++fruEntry)
    {
        rc = -1;
        // Actual offset in the payload is the offset mentioned in common header
        // multiplied by 8. Common header is always the first 8 bytes.
        size_t areaOffset = fruData[fruEntry] * IPMI_BLOCK_SIZE;
        if (areaOffset && (dataLen < (areaOffset + 2)))
        {
            // Our file size is less than what it needs to be. +2 because we are
            // using area len that is at 2 byte off areaOffset
            log<level::ERR>("FRU file is incomplete",
                            entry("SIZE=%d", dataLen));
            return rc;
        }
        else if (areaOffset)
        {
            // Read 2 bytes to know the actual size of area.
            uint8_t areaHeader[2] = {0};
            std::memcpy(areaHeader, &((uint8_t*)fruData)[areaOffset],
                        sizeof(areaHeader));

            // Size of this area will be the 2nd byte in the FRU area header.
            size_t areaLen = areaHeader[1] * IPMI_BLOCK_SIZE;

            // The area length is usually taken from the area header.
            // Some areas however lack that information.
            bool hasSizeHdr = (fruEntry != IPMI_FRU_INTERNAL_OFFSET &&
                               fruEntry != IPMI_FRU_MULTI_OFFSET);

            // For areas without a header, take areaLen from common header
            if (!hasSizeHdr)
            {
                // First assume that this area spans till the end of FRU data
                // and there are no areas behind it.
                areaLen = dataLen - areaOffset;

                // Now check if there is any other area listed in the common
                // header that starts after this one. If so, then this
                // area can't span past that point.
                //
                // Areas' data aren't obliged to go in the same order as
                // they are listed in the common header, so start with the
                // very first one.
                for (uint8_t i = IPMI_FRU_INTERNAL_OFFSET;
                     i < IPMI_FRU_MULTI_OFFSET; ++i)
                {
                    if (i == fruEntry)
                    {
                        // Don't check self
                        continue;
                    }

                    if (fruData[i] > fruData[fruEntry])
                    {
                        areaLen =
                            (fruData[i] - fruData[fruEntry]) * IPMI_BLOCK_SIZE;
                        break;
                    }
                }
            }

            uint8_t areaData[areaLen] = {0};

            log<level::DEBUG>("FRU Data", entry("SIZE=%d", dataLen),
                              entry("AREA OFFSET=%d", areaOffset),
                              entry("AREA_SIZE=%d", areaLen));

            // See if we really have that much buffer. We have area offset amd
            // from there, the actual len.
            if (dataLen < (areaLen + areaOffset))
            {
                log<level::ERR>("Incomplete FRU file",
                                entry("SIZE=%d", dataLen));
                return rc;
            }

            // Save off the data.
            std::memcpy(areaData, &((uint8_t*)fruData)[areaOffset], areaLen);

            // Validate the area if possible.
            //  * Internal Use Area doesn't have a specified format
            //    and we can't validate checksum, but it still has
            //    a standard version header that needs to be checked,
            //    but doesn't have a checksum;
            //  * MultiRecord Area has a different format and
            //    is checked elsewhere.
            bool isCheckable = (fruEntry != IPMI_FRU_MULTI_OFFSET);
            bool useChecksum = hasSizeHdr; // No header means no checksum

            if (isCheckable)
            {
                rc = verifyFruData(areaData, areaLen, useChecksum);
                if (rc < 0)
                {
                    log<level::ERR>("Err validating FRU area",
                                    entry("OFFSET=%d", areaOffset));
                    continue;
                }
                else
                {
                    log<level::DEBUG>("FRU area looks valid.",
                                      entry("OFFSET=%d", areaOffset));
                }
            }

            // We already have a vector that is passed to us containing all
            // of the fields populated. Update the data portion now.
            for (auto& iter : fruAreaVec)
            {
                if (iter->getType() == getFruAreaType(fruEntry))
                {
                    iter->setData(areaData, areaLen);
                }
            }
        } // If we have FRU data present
    }     // Walk struct common_header

    // Not all the fields will be populated in a FRU data. Mostly all cases will
    // not have more than 2 or 3.
    fruAreaVec.erase(
        std::remove_if(fruAreaVec.begin(), fruAreaVec.end(), removeInvalidArea),
        fruAreaVec.end());

    return EXIT_SUCCESS;
}

/**
 * Validates the FRU data per ipmi common header constructs.
 * Returns with updated struct common_header and also file_size
 *
 * @param[in] fruData - the FRU data
 * @param[in] dataLen - the length of the data
 * @return non-zero on failure
 */
int ipmiValidateCommonHeader(const uint8_t* fruData, const size_t dataLen)
{
    int rc = -1;

    uint8_t commonHdr[sizeof(struct common_header)] = {0};
    if (dataLen >= sizeof(commonHdr))
    {
        std::memcpy(commonHdr, fruData, sizeof(commonHdr));
    }
    else
    {
        log<level::ERR>("Incomplete FRU data file", entry("SIZE=%d", dataLen));
        return rc;
    }

    // Verify the CRC and size
    rc = verifyFruData(commonHdr, sizeof(commonHdr), true);
    if (rc < 0)
    {
        log<level::ERR>("Failed to validate common header");
        return rc;
    }

    return EXIT_SUCCESS;
}

int validateFRUArea(const uint8_t fruid, const char* fruFilename,
                    sdbusplus::bus::bus& bus, const bool bmcOnlyFru)
{
    size_t dataLen = 0;
    size_t bytesRead = 0;
    int rc = -1;

    // Vector that holds individual IPMI FRU AREAs. Although MULTI and INTERNAL
    // are not used, keeping it here for completeness.
    FruAreaVector fruAreaVec;

    for (uint8_t fruEntry = IPMI_FRU_INTERNAL_OFFSET;
         fruEntry < (sizeof(struct common_header) - 2); fruEntry++)
    {
        // Create an object and push onto a vector.
        std::unique_ptr<IPMIFruArea> fruArea = std::make_unique<IPMIFruArea>(
            fruid, getFruAreaType(fruEntry), bmcOnlyFru);

        // Physically being present
        bool present = access(fruFilename, F_OK) == 0;
        fruArea->setPresent(present);

        fruAreaVec.emplace_back(std::move(fruArea));
    }

    FILE* fruFilePointer = std::fopen(fruFilename, "rb");
    if (fruFilePointer == NULL)
    {
        log<level::ERR>("Unable to open FRU file",
                        entry("FILE=%s", fruFilename),
                        entry("ERRNO=%s", std::strerror(errno)));
        return cleanupError(fruFilePointer, fruAreaVec);
    }

    // Get the size of the file to see if it meets minimum requirement
    if (std::fseek(fruFilePointer, 0, SEEK_END))
    {
        log<level::ERR>("Unable to seek FRU file",
                        entry("FILE=%s", fruFilename),
                        entry("ERRNO=%s", std::strerror(errno)));
        return cleanupError(fruFilePointer, fruAreaVec);
    }

    // Allocate a buffer to hold entire file content
    dataLen = std::ftell(fruFilePointer);
    uint8_t fruData[dataLen] = {0};

    std::rewind(fruFilePointer);
    bytesRead = std::fread(fruData, dataLen, 1, fruFilePointer);
    if (bytesRead != 1)
    {
        log<level::ERR>("Failed reading FRU data.",
                        entry("BYTESREAD=%d", bytesRead),
                        entry("ERRNO=%s", std::strerror(errno)));
        return cleanupError(fruFilePointer, fruAreaVec);
    }

    // We are done reading.
    std::fclose(fruFilePointer);
    fruFilePointer = NULL;

    rc = ipmiValidateCommonHeader(fruData, dataLen);
    if (rc < 0)
    {
        return cleanupError(fruFilePointer, fruAreaVec);
    }

    // Now that we validated the common header, populate various FRU sections if
    // we have them here.
    rc = ipmiPopulateFruAreas(fruData, dataLen, fruAreaVec);
    if (rc < 0)
    {
        log<level::ERR>("Populating FRU areas failed", entry("FRU=%d", fruid));
        return cleanupError(fruFilePointer, fruAreaVec);
    }
    else
    {
        log<level::DEBUG>("Populated FRU areas", entry("FILE=%s", fruFilename));
    }

#ifdef __IPMI_DEBUG__
    for (const auto& iter : fruAreaVec)
    {
        std::printf("FRU ID : [%d]\n", iter->getFruID());
        std::printf("AREA NAME : [%s]\n", iter->getName());
        std::printf("TYPE : [%d]\n", iter->getType());
        std::printf("LEN : [%d]\n", iter->getLength());
    }
#endif

    // If the vector is populated with everything, then go ahead and update the
    // inventory.
    if (!(fruAreaVec.empty()))
    {

#ifdef __IPMI_DEBUG__
        std::printf("\n SIZE of vector is : [%d] \n", fruAreaVec.size());
#endif
        rc = updateInventory(fruAreaVec, bus);
        if (rc < 0)
        {
            log<level::ERR>("Error updating inventory.");
        }
    }

    // we are done with all that we wanted to do. This will do the job of
    // calling any destructors too.
    fruAreaVec.clear();

    return rc;
}
