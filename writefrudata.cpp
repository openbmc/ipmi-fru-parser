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

//------------------------------------------------------------
// Cleanup routine
// Must always be called as last reference to fruFilePointer.
//------------------------------------------------------------
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

//------------------------------------------------------------------------
// Gets the value of the key from the fru dictionary of the given section.
// FRU dictionary is parsed fru data for all the sections.
//------------------------------------------------------------------------
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

// Get the inventory service from the mapper.
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

// Takes FRU data, invokes Parser for each fru record area and updates
// Inventory
//------------------------------------------------------------------------
int updateInventory(FruAreaVector& areaVector, sdbusplus::bus::bus& bus)
{
    // Generic error reporter
    int rc = 0;
    uint8_t fruid = 0;
    IPMIFruInfo fruData;

    // For each FRU area, extract the needed data , get it parsed and update
    // the Inventory.
    for (const auto& fruArea : areaVector)
    {
        fruid = fruArea->getFruID();
        // Fill the container with information
        rc = parse_fru_area(fruArea->getType(),
                            static_cast<const void*>(fruArea->getData()),
                            fruArea->getLength(), fruData);
        if (rc < 0)
        {
            log<level::ERR>("Error parsing FRU records");
            return rc;
        }
    } // END walking the vector of areas and updating

    // For each Fru we have the list of instances which needs to be updated.
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
        log<level::ERR>("Unable to get the fru info",
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

        for (const auto& interfaceList : instance.interfaces)
        {
            PropertyMap props; // store all the properties
            for (const auto& properties : interfaceList.second)
            {
                std::string value;
                decltype(auto) pdata = properties.second;

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
        objects.emplace(objectPath, interfaces);
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

//------------------------------------------------
// Takes the pointer to stream of bytes and length
// and returns the 8 bit checksum
// This algo is per IPMI V2.0 spec
//-------------------------------------------------
unsigned char calculateCRC(const unsigned char* data, size_t len)
{
    char crc = 0;
    size_t byte = 0;

    for (byte = 0; byte < len; byte++)
    {
        crc += *data++;
    }

    return (-crc);
}

//---------------------------------------------------------------------
// Accepts a fru area offset in commom hdr and tells which area it is.
//---------------------------------------------------------------------
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

///-----------------------------------------------
// Validates the data for crc and mandatory fields
///-----------------------------------------------
int verifyFruData(const uint8_t* data, const size_t len)
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

    // See if the calculated CRC matches with the embedded one.
    // CRC to be calculated on all except the last one that is CRC itself.
    checksum = calculateCRC(data, len - 1);
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

///----------------------------------------------------
// Checks if a particular fru area is populated or not
///----------------------------------------------------
bool removeInvalidArea(const std::unique_ptr<IPMIFruArea>& fruArea)
{
    // Filter the ones that are empty
    if (!(fruArea->getLength()))
    {
        return true;
    }
    return false;
}

///----------------------------------------------------------------------------------
// Populates various FRU areas
// @prereq : This must be called only after validating common header.
///----------------------------------------------------------------------------------
int ipmiPopulateFruAreas(uint8_t* fruData, const size_t dataLen,
                         FruAreaVector& fruAreaVec)
{
    int rc = -1;

    // Now walk the common header and see if the file size has atleast the last
    // offset mentioned by the struct common_header. If the file size is less
    // than the offset of any if the fru areas mentioned in the common header,
    // then we do not have a complete file.
    for (uint8_t fruEntry = IPMI_FRU_INTERNAL_OFFSET;
         fruEntry < (sizeof(struct common_header) - 2); fruEntry++)
    {
        rc = -1;
        // Actual offset in the payload is the offset mentioned in common header
        // multiplied by 8. Common header is always the first 8 bytes.
        size_t areaOffset = fruData[fruEntry] * IPMI_EIGHT_BYTES;
        if (areaOffset && (dataLen < (areaOffset + 2)))
        {
            // Our file size is less than what it needs to be. +2 because we are
            // using area len that is at 2 byte off areaOffset
            log<level::ERR>("fru file is incomplete",
                            entry("SIZE=%d", dataLen));
            return rc;
        }
        else if (areaOffset)
        {
            // Read 2 bytes to know the actual size of area.
            uint8_t areaHeader[2] = {0};
            std::memcpy(areaHeader, &((uint8_t*)fruData)[areaOffset],
                        sizeof(areaHeader));

            // Size of this area will be the 2nd byte in the fru area header.
            size_t areaLen = areaHeader[1] * IPMI_EIGHT_BYTES;
            uint8_t areaData[areaLen] = {0};

            log<level::DEBUG>("Fru Data", entry("SIZE=%d", dataLen),
                              entry("AREA OFFSET=%d", areaOffset),
                              entry("AREA_SIZE=%d", areaLen));

            // See if we really have that much buffer. We have area offset amd
            // from there, the actual len.
            if (dataLen < (areaLen + areaOffset))
            {
                log<level::ERR>("Incomplete Fru file",
                                entry("SIZE=%d", dataLen));
                return rc;
            }

            // Save off the data.
            std::memcpy(areaData, &((uint8_t*)fruData)[areaOffset], areaLen);

            // Validate the crc
            rc = verifyFruData(areaData, areaLen);
            if (rc < 0)
            {
                log<level::ERR>("Err validating fru area",
                                entry("OFFSET=%d", areaOffset));
                return rc;
            }
            else
            {
                log<level::DEBUG>("Successfully verified area checksum.",
                                  entry("OFFSET=%d", areaOffset));
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
        } // If we have fru data present
    }     // Walk struct common_header

    // Not all the fields will be populated in a fru data. Mostly all cases will
    // not have more than 2 or 3.
    fruAreaVec.erase(
        std::remove_if(fruAreaVec.begin(), fruAreaVec.end(), removeInvalidArea),
        fruAreaVec.end());

    return EXIT_SUCCESS;
}

///---------------------------------------------------------
// Validates the fru data per ipmi common header constructs.
// Returns with updated struct common_header and also file_size
//----------------------------------------------------------
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
        log<level::ERR>("Incomplete fru data file", entry("SIZE=%d", dataLen));
        return rc;
    }

    // Verify the crc and size
    rc = verifyFruData(commonHdr, sizeof(commonHdr));
    if (rc < 0)
    {
        log<level::ERR>("Failed to validate common header");
        return rc;
    }

    return EXIT_SUCCESS;
}

///-----------------------------------------------------
// Accepts the filename and validates per IPMI FRU spec
//----------------------------------------------------
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
        log<level::ERR>("Unable to open fru file",
                        entry("FILE=%s", fruFilename),
                        entry("ERRNO=%s", std::strerror(errno)));
        return cleanupError(fruFilePointer, fruAreaVec);
    }

    // Get the size of the file to see if it meets minimum requirement
    if (std::fseek(fruFilePointer, 0, SEEK_END))
    {
        log<level::ERR>("Unable to seek fru file",
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
        log<level::ERR>("Failed reading fru data.",
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

    // Now that we validated the common header, populate various fru sections if
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
