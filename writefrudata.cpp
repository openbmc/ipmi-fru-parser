#include "fru-area.hpp"
#include "frup.hpp"
#include "types.hpp"

#include <dlfcn.h>
#include <host-ipmid/ipmid-api.h>
#include <mapper.h>
#include <systemd/sd-bus.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/server.hpp>
#include <sstream>
#include <vector>

using namespace ipmi::vpd;
using namespace phosphor::logging;

extern const FruMap frus;
extern const std::map<Path, InterfaceMap> extras;

namespace
{

//------------------------------------------------------------
// Cleanup routine
// Must always be called as last reference to fru_fp.
//------------------------------------------------------------
int cleanupError(FILE* fru_fp, fru_area_vec_t& fru_area_vec)
{
    if (fru_fp != NULL)
    {
        std::fclose(fru_fp);
    }

    if (!(fru_area_vec.empty()))
    {
        fru_area_vec.clear();
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

    auto itr =
        std::find_if(first, last, [&key](auto& e) { return key == e.first; });

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
                fruValue = fruValue.substr(delimiterpos + 1);
        }
    }
    return fruValue;
}

} // namespace

//----------------------------------------------------------------
// Constructor
//----------------------------------------------------------------
ipmi_fru::ipmi_fru(const uint8_t fruid, const ipmi_fru_area_type type,
                   sd_bus* bus_type, bool bmc_fru)
{
    iv_fruid = fruid;
    iv_type = type;
    iv_bmc_fru = bmc_fru;
    iv_bus_type = bus_type;
    iv_valid = false;
    iv_data = NULL;
    iv_present = false;

    if (iv_type == IPMI_FRU_AREA_INTERNAL_USE)
    {
        iv_name = "INTERNAL_";
    }
    else if (iv_type == IPMI_FRU_AREA_CHASSIS_INFO)
    {
        iv_name = "CHASSIS_";
    }
    else if (iv_type == IPMI_FRU_AREA_BOARD_INFO)
    {
        iv_name = "BOARD_";
    }
    else if (iv_type == IPMI_FRU_AREA_PRODUCT_INFO)
    {
        iv_name = "PRODUCT_";
    }
    else if (iv_type == IPMI_FRU_AREA_MULTI_RECORD)
    {
        iv_name = "MULTI_";
    }
    else
    {
        iv_name = IPMI_FRU_AREA_TYPE_MAX;
        log<level::ERR>("Invalid Area", entry("TYPE=%d", iv_type));
    }
}

//-----------------------------------------------------
// For a FRU area type, accepts the data and updates
// area specific data.
//-----------------------------------------------------
void ipmi_fru::set_data(const uint8_t* data, const size_t len)
{
    iv_len = len;
    iv_data = new uint8_t[len];
    std::memcpy(iv_data, data, len);
}

//-----------------------------------------------------
// Sets the dbus parameters
//-----------------------------------------------------
void ipmi_fru::update_dbus_paths(const char* bus_name, const char* obj_path,
                                 const char* intf_name)
{
    iv_bus_name = bus_name;
    iv_obj_path = obj_path;
    iv_intf_name = intf_name;
}

//-------------------
// Destructor
//-------------------
ipmi_fru::~ipmi_fru()
{
    if (iv_data != NULL)
    {
        delete[] iv_data;
        iv_data = NULL;
    }
}

//------------------------------------------------
// Takes the pointer to stream of bytes and length
// and returns the 8 bit checksum
// This algo is per IPMI V2.0 spec
//-------------------------------------------------
unsigned char calculate_crc(const unsigned char* data, size_t len)
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
ipmi_fru_area_type get_fru_area_type(uint8_t area_offset)
{
    ipmi_fru_area_type type = IPMI_FRU_AREA_TYPE_MAX;

    switch (area_offset)
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
int verify_fru_data(const uint8_t* data, const size_t len)
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
        log<level::DEBUG>("Validated in entry_1 of fru_data",
                          entry("ENTRY=0x%X", static_cast<uint32_t>(data[0])));
    }
#endif

    // See if the calculated CRC matches with the embedded one.
    // CRC to be calculated on all except the last one that is CRC itself.
    checksum = calculate_crc(data, len - 1);
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

//------------------------------------------------------------------------
// Takes FRU data, invokes Parser for each fru record area and updates
// Inventory
//------------------------------------------------------------------------
int ipmi_update_inventory(fru_area_vec_t& area_vec, sd_bus* bus_sd)
{
    // Generic error reporter
    int rc = 0;
    uint8_t fruid = 0;
    IPMIFruInfo fruData;

    // For each FRU area, extract the needed data , get it parsed and update
    // the Inventory.
    for (const auto& fruArea : area_vec)
    {
        fruid = fruArea->get_fruid();
        // Fill the container with information
        rc = parse_fru_area((fruArea)->get_type(), (void*)(fruArea)->get_data(),
                            (fruArea)->get_len(), fruData);
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
    sdbusplus::bus::bus bus{bus_sd};
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
    for (auto& instance : instanceList)
    {
        InterfaceMap interfaces;
        const auto& extrasIter = extras.find(instance.path);

        for (auto& interfaceList : instance.interfaces)
        {
            PropertyMap props; // store all the properties
            for (auto& properties : interfaceList.second)
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
        sdbusplus::message::object_path path = instance.path;
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
        objects.emplace(path, interfaces);
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

///----------------------------------------------------
// Checks if a particular fru area is populated or not
///----------------------------------------------------
bool remove_invalid_area(const std::unique_ptr<ipmi_fru>& fru_area)
{
    // Filter the ones that are empty
    if (!(fru_area->get_len()))
    {
        return true;
    }
    return false;
}

///----------------------------------------------------------------------------------
// Populates various FRU areas
// @prereq : This must be called only after validating common header.
///----------------------------------------------------------------------------------
int ipmi_populate_fru_areas(uint8_t* fru_data, const size_t data_len,
                            fru_area_vec_t& fru_area_vec)
{
    int rc = -1;

    // Now walk the common header and see if the file size has atleast the last
    // offset mentioned by the common_hdr. If the file size is less than the
    // offset of any if the fru areas mentioned in the common header, then we do
    // not have a complete file.
    for (uint8_t fru_entry = IPMI_FRU_INTERNAL_OFFSET;
         fru_entry < (sizeof(struct common_header) - 2); fru_entry++)
    {
        rc = -1;
        // Actual offset in the payload is the offset mentioned in common header
        // multiplied by 8. Common header is always the first 8 bytes.
        size_t area_offset = fru_data[fru_entry] * IPMI_EIGHT_BYTES;
        if (area_offset && (data_len < (area_offset + 2)))
        {
            // Our file size is less than what it needs to be. +2 because we are
            // using area len that is at 2 byte off area_offset
            log<level::ERR>("fru file is incomplete",
                            entry("SIZE=%d", data_len));
            return rc;
        }
        else if (area_offset)
        {
            // Read 2 bytes to know the actual size of area.
            uint8_t area_hdr[2] = {0};
            std::memcpy(area_hdr, &((uint8_t*)fru_data)[area_offset],
                        sizeof(area_hdr));

            // Size of this area will be the 2nd byte in the fru area header.
            size_t area_len = area_hdr[1] * IPMI_EIGHT_BYTES;
            uint8_t area_data[area_len] = {0};

            log<level::DEBUG>("Fru Data", entry("SIZE=%d", data_len),
                              entry("AREA OFFSET=%d", area_offset),
                              entry("AREA_SIZE=%d", area_len));

            // See if we really have that much buffer. We have area offset amd
            // from there, the actual len.
            if (data_len < (area_len + area_offset))
            {
                log<level::ERR>("Incomplete Fru file",
                                entry("SIZE=%d", data_len));
                return rc;
            }

            // Save off the data.
            std::memcpy(area_data, &((uint8_t*)fru_data)[area_offset],
                        area_len);

            // Validate the crc
            rc = verify_fru_data(area_data, area_len);
            if (rc < 0)
            {
                log<level::ERR>("Err validating fru area",
                                entry("OFFSET=%d", area_offset));
                return rc;
            }
            else
            {
                log<level::DEBUG>("Successfully verified area checksum.",
                                  entry("OFFSET=%d", area_offset));
            }

            // We already have a vector that is passed to us containing all
            // of the fields populated. Update the data portion now.
            for (auto& iter : fru_area_vec)
            {
                if ((iter)->get_type() == get_fru_area_type(fru_entry))
                {
                    (iter)->set_data(area_data, area_len);
                }
            }
        } // If we have fru data present
    }     // Walk common_hdr

    // Not all the fields will be populated in a fru data. Mostly all cases will
    // not have more than 2 or 3.
    fru_area_vec.erase(std::remove_if(fru_area_vec.begin(), fru_area_vec.end(),
                                      remove_invalid_area),
                       fru_area_vec.end());

    return EXIT_SUCCESS;
}

///---------------------------------------------------------
// Validates the fru data per ipmi common header constructs.
// Returns with updated common_hdr and also file_size
//----------------------------------------------------------
int ipmi_validate_common_hdr(const uint8_t* fru_data, const size_t data_len)
{
    int rc = -1;

    uint8_t common_hdr[sizeof(struct common_header)] = {0};
    if (data_len >= sizeof(common_hdr))
    {
        std::memcpy(common_hdr, fru_data, sizeof(common_hdr));
    }
    else
    {
        log<level::ERR>("Incomplete fru data file", entry("SIZE=%d", data_len));
        return rc;
    }

    // Verify the crc and size
    rc = verify_fru_data(common_hdr, sizeof(common_hdr));
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
int validateFRUArea(const uint8_t fruid, const char* fru_file_name,
                    sd_bus* bus_type, const bool bmc_fru)
{
    size_t data_len = 0;
    size_t bytes_read = 0;
    int rc = -1;

    // Vector that holds individual IPMI FRU AREAs. Although MULTI and INTERNAL
    // are not used, keeping it here for completeness.
    fru_area_vec_t fru_area_vec;

    for (uint8_t fru_entry = IPMI_FRU_INTERNAL_OFFSET;
         fru_entry < (sizeof(struct common_header) - 2); fru_entry++)
    {
        // Create an object and push onto a vector.
        std::unique_ptr<ipmi_fru> fru_area = std::make_unique<ipmi_fru>(
            fruid, get_fru_area_type(fru_entry), bus_type, bmc_fru);

        // Physically being present
        bool present = access(fru_file_name, F_OK) == 0;
        fru_area->set_present(present);

        fru_area_vec.emplace_back(std::move(fru_area));
    }

    FILE* fru_fp = std::fopen(fru_file_name, "rb");
    if (fru_fp == NULL)
    {
        log<level::ERR>("Unable to open fru file",
                        entry("FILE=%s", fru_file_name),
                        entry("ERRNO=%s", std::strerror(errno)));
        return cleanupError(fru_fp, fru_area_vec);
    }

    // Get the size of the file to see if it meets minimum requirement
    if (std::fseek(fru_fp, 0, SEEK_END))
    {
        log<level::ERR>("Unable to seek fru file",
                        entry("FILE=%s", fru_file_name),
                        entry("ERRNO=%s", std::strerror(errno)));
        return cleanupError(fru_fp, fru_area_vec);
    }

    // Allocate a buffer to hold entire file content
    data_len = std::ftell(fru_fp);
    uint8_t fru_data[data_len] = {0};

    std::rewind(fru_fp);
    bytes_read = std::fread(fru_data, data_len, 1, fru_fp);
    if (bytes_read != 1)
    {
        log<level::ERR>("Failed reading fru data.",
                        entry("BYTESREAD=%d", bytes_read),
                        entry("ERRNO=%s", std::strerror(errno)));
        return cleanupError(fru_fp, fru_area_vec);
    }

    // We are done reading.
    std::fclose(fru_fp);
    fru_fp = NULL;

    rc = ipmi_validate_common_hdr(fru_data, data_len);
    if (rc < 0)
    {
        return cleanupError(fru_fp, fru_area_vec);
    }

    // Now that we validated the common header, populate various fru sections if
    // we have them here.
    rc = ipmi_populate_fru_areas(fru_data, data_len, fru_area_vec);
    if (rc < 0)
    {
        log<level::ERR>("Populating FRU areas failed", entry("FRU=%d", fruid));
        return cleanupError(fru_fp, fru_area_vec);
    }
    else
    {
        log<level::DEBUG>("Populated FRU areas",
                          entry("FILE=%s", fru_file_name));
    }

#ifdef __IPMI_DEBUG__
    for (auto& iter : fru_area_vec)
    {
        std::printf("FRU ID : [%d]\n", (iter)->get_fruid());
        std::printf("AREA NAME : [%s]\n", (iter)->get_name());
        std::printf("TYPE : [%d]\n", (iter)->get_type());
        std::printf("LEN : [%d]\n", (iter)->get_len());
        std::printf("BUS NAME : [%s]\n", (iter)->get_bus_name());
        std::printf("OBJ PATH : [%s]\n", (iter)->get_obj_path());
        std::printf("INTF NAME :[%s]\n", (iter)->get_intf_name());
    }
#endif

    // If the vector is populated with everything, then go ahead and update the
    // inventory.
    if (!(fru_area_vec.empty()))
    {

#ifdef __IPMI_DEBUG__
        std::printf("\n SIZE of vector is : [%d] \n", fru_area_vec.size());
#endif
        rc = ipmi_update_inventory(fru_area_vec, bus_type);
        if (rc < 0)
        {
            log<level::ERR>("Error updating inventory.");
        }
    }

    // we are done with all that we wanted to do. This will do the job of
    // calling any destructors too.
    fru_area_vec.clear();

    return rc;
}
