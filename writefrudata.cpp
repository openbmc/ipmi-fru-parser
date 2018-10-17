#include <exception>
#include <vector>
#include <stdlib.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <systemd/sd-bus.h>
#include <unistd.h>
#include <host-ipmid/ipmid-api.h>
#include <iostream>
#include <memory>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <mapper.h>
#include "types.hpp"
#include "frup.hpp"
#include "fru-area.hpp"
#include <sdbusplus/server.hpp>

using namespace ipmi::vpd;

extern const FruMap frus;
extern const std::map<Path, InterfaceMap> extras;

//----------------------------------------------------------------
// Constructor
//----------------------------------------------------------------
ipmi_fru::ipmi_fru(const uint8_t fruid, const ipmi_fru_area_type type,
                   sd_bus *bus_type, bool bmc_fru)
{
    iv_fruid = fruid;
    iv_type = type;
    iv_bmc_fru = bmc_fru;
    iv_bus_type = bus_type;
    iv_valid = false;
    iv_data = NULL;
    iv_present = false;

    if(iv_type == IPMI_FRU_AREA_INTERNAL_USE)
    {
        iv_name = "INTERNAL_";
    }
    else if(iv_type == IPMI_FRU_AREA_CHASSIS_INFO)
    {
        iv_name = "CHASSIS_";
    }
    else if(iv_type == IPMI_FRU_AREA_BOARD_INFO)
    {
        iv_name = "BOARD_";
    }
    else if(iv_type == IPMI_FRU_AREA_PRODUCT_INFO)
    {
        iv_name = "PRODUCT_";
    }
    else if(iv_type == IPMI_FRU_AREA_MULTI_RECORD)
    {
        iv_name = "MULTI_";
    }
    else
    {
        iv_name = IPMI_FRU_AREA_TYPE_MAX;
        fprintf(stderr, "ERROR: Invalid Area type :[%d]\n",iv_type);
    }
}

//-----------------------------------------------------
// For a FRU area type, accepts the data and updates
// area specific data.
//-----------------------------------------------------
void ipmi_fru::set_data(const uint8_t *data, const size_t len)
{
    iv_len = len;
    iv_data = new uint8_t[len];
    memcpy(iv_data, data, len);
}

//-----------------------------------------------------
// Sets the dbus parameters
//-----------------------------------------------------
void ipmi_fru::update_dbus_paths(const char *bus_name,
                      const char *obj_path, const char *intf_name)
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
    if(iv_data != NULL)
    {
        delete [] iv_data;
        iv_data = NULL;
    }
}

//------------------------------------------------
// Takes the pointer to stream of bytes and length
// and returns the 8 bit checksum
// This algo is per IPMI V2.0 spec
//-------------------------------------------------
unsigned char calculate_crc(const unsigned char *data, size_t len)
{
    char crc = 0;
    size_t byte = 0;

    for(byte = 0; byte < len; byte++)
    {
        crc += *data++;
    }

    return(-crc);
}

//---------------------------------------------------------------------
// Accepts a fru area offset in commom hdr and tells which area it is.
//---------------------------------------------------------------------
ipmi_fru_area_type get_fru_area_type(uint8_t area_offset)
{
    ipmi_fru_area_type type = IPMI_FRU_AREA_TYPE_MAX;

    switch(area_offset)
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
int verify_fru_data(const uint8_t *data, const size_t len)
{
    uint8_t checksum = 0;
    int rc = -1;

    // Validate for first byte to always have a value of [1]
    if(data[0] != IPMI_FRU_HDR_BYTE_ZERO)
    {
        fprintf(stderr, "Invalid entry:[%d] in byte-0\n",data[0]);
        return rc;
    }
#ifdef __IPMI_DEBUG__
    else
    {
        printf("SUCCESS: Validated [0x%X] in entry_1 of fru_data\n",data[0]);
    }
#endif

    // See if the calculated CRC matches with the embedded one.
    // CRC to be calculated on all except the last one that is CRC itself.
    checksum = calculate_crc(data, len - 1);
    if(checksum != data[len-1])
    {
#ifdef __IPMI_DEBUG__
        fprintf(stderr, "Checksum mismatch."
                " Calculated:[0x%X], Embedded:[0x%X]\n",
                checksum, data[len]);
#endif
        return rc;
    }
#ifdef __IPMI_DEBUG__
    else
    {
        printf("SUCCESS: Checksum matches:[0x%X]\n",checksum);
    }
#endif

    return EXIT_SUCCESS;
}

//------------------------------------------------------------------------
// Gets the value of the key from the fru dictionary of the given section.
// FRU dictionary is parsed fru data for all the sections.
//------------------------------------------------------------------------

std::string getFRUValue(const std::string& section,
                        const std::string& key,
                        const std::string& delimiter,
                        IPMIFruInfo& fruData)
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
            [&key](auto& e){ return key == e.first; });

    if (itr != last)
    {
        fruValue = itr->second;
    }

    //if the key is custom property then the value could be in two formats.
    //1) custom field 2 = "value".
    //2) custom field 2 =  "key:value".
    //if delimiter length = 0 i.e custom field 2 = "value"

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
//Get the inventory service from the mapper.
auto getService(sdbusplus::bus::bus& bus,
                         const std::string& intf,
                         const std::string& path)
{
    auto mapperCall = bus.new_method_call(
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper",
            "GetObject");

    mapperCall.append(path);
    mapperCall.append(std::vector<std::string>({intf}));

    auto mapperResponseMsg = bus.call(mapperCall);
    if (mapperResponseMsg.is_method_error())
    {
        throw std::runtime_error("ERROR in mapper call");
    }

    std::map<std::string, std::vector<std::string>> mapperResponse;
    mapperResponseMsg.read(mapperResponse);

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
            std::cerr << "ERROR parsing FRU records\n";
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
        service = getService(bus,intf,path);
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << e.what() << "\n";
        return -1;
    }

    auto iter = frus.find(fruid);
    if (iter == frus.end())
    {
        std::cerr << "ERROR Unable to get the fru info for FRU="
                  << static_cast<int>(fruid) << "\n";
        return -1;
    }

    auto& instanceList = iter->second;
    if (instanceList.size() <= 0)
    {
        std::cout << "Object List empty for this FRU="
                  << static_cast<int>(fruid) << "\n";
    }

    ObjectMap objects;
    for (auto& instance : instanceList)
    {
        InterfaceMap interfaces;
        const auto& extrasIter = extras.find(instance.path);

        for (auto& interfaceList : instance.interfaces)
        {
            PropertyMap props;//store all the properties
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
            if(extras.end() != extrasIter)
            {
                const auto& propsIter =
                    (extrasIter->second).find(interfaceList.first);
                if((extrasIter->second).end() != propsIter)
                {
                    for(const auto& map : propsIter->second)
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
        if(extras.end() != extrasIter)
        {
            for(const auto& entry : extrasIter->second)
            {
                if(interfaces.end() == interfaces.find(entry.first))
                {
                    interfaces.emplace(entry.first, entry.second);
                }
            }
        }
        objects.emplace(path,interfaces);
    }

    auto pimMsg = bus.new_method_call(
                          service.c_str(),
                          path.c_str(),
                          intf.c_str(),
                          "Notify");
    pimMsg.append(std::move(objects));
    auto inventoryMgrResponseMsg = bus.call(pimMsg);
    if (inventoryMgrResponseMsg.is_method_error())
    {
        std::cerr << "Error in notify call\n";
        return -1;
    }
    return rc;
}

///----------------------------------------------------
// Checks if a particular fru area is populated or not
///----------------------------------------------------
bool remove_invalid_area(const std::unique_ptr<ipmi_fru> &fru_area)
{
    // Filter the ones that are empty
    if(!(fru_area->get_len()))
    {
        return true;
    }
    return false;
}

///----------------------------------------------------------------------------------
// Populates various FRU areas
// @prereq : This must be called only after validating common header.
///----------------------------------------------------------------------------------
int ipmi_populate_fru_areas(uint8_t *fru_data, const size_t data_len,
                            fru_area_vec_t & fru_area_vec)
{
    size_t area_offset = 0;
    int rc = -1;

    // Now walk the common header and see if the file size has atleast the last
    // offset mentioned by the common_hdr. If the file size is less than the
    // offset of any if the fru areas mentioned in the common header, then we do
    // not have a complete file.
    for(uint8_t fru_entry = IPMI_FRU_INTERNAL_OFFSET;
            fru_entry < (sizeof(struct common_header) -2); fru_entry++)
    {
        rc = -1;
        // Actual offset in the payload is the offset mentioned in common header
        // multiplied by 8. Common header is always the first 8 bytes.
        area_offset = fru_data[fru_entry] * IPMI_EIGHT_BYTES;
        if(area_offset && (data_len < (area_offset + 2)))
        {
            // Our file size is less than what it needs to be. +2 because we are
            // using area len that is at 2 byte off area_offset
            fprintf(stderr, "fru file is incomplete. Size:[%zd]\n",data_len);
            return rc;
        }
        else if(area_offset)
        {
            // Read 2 bytes to know the actual size of area.
            uint8_t area_hdr[2] = {0};
            memcpy(area_hdr, &((uint8_t *)fru_data)[area_offset], sizeof(area_hdr));

            // Size of this area will be the 2nd byte in the fru area header.
            size_t  area_len = area_hdr[1] * IPMI_EIGHT_BYTES;
            uint8_t area_data[area_len] = {0};

            printf("fru data size:[%zd], area offset:[%zd], area_size:[%zd]\n",
                    data_len, area_offset, area_len);

            // See if we really have that much buffer. We have area offset amd
            // from there, the actual len.
            if(data_len < (area_len + area_offset))
            {
                fprintf(stderr, "Incomplete Fru file.. Size:[%zd]\n",data_len);
                return rc;
            }

            // Save off the data.
            memcpy(area_data, &((uint8_t *)fru_data)[area_offset], area_len);

            // Validate the crc
            rc = verify_fru_data(area_data, area_len);
            if(rc < 0)
            {
                fprintf(stderr, "Error validating fru area. offset:[%zd]\n",area_offset);
                return rc;
            }
            else
            {
                printf("Successfully verified area checksum. offset:[%zd]\n",area_offset);
            }

            // We already have a vector that is passed to us containing all
            // of the fields populated. Update the data portion now.
            for(auto& iter : fru_area_vec)
            {
                if((iter)->get_type() == get_fru_area_type(fru_entry))
                {
                    (iter)->set_data(area_data, area_len);
                }
            }
        } // If we have fru data present
    } // Walk common_hdr

    // Not all the fields will be populated in a fru data. Mostly all cases will
    // not have more than 2 or 3.
    fru_area_vec.erase(std::remove_if(fru_area_vec.begin(), fru_area_vec.end(),
                       remove_invalid_area), fru_area_vec.end());

    return EXIT_SUCCESS;
}

///---------------------------------------------------------
// Validates the fru data per ipmi common header constructs.
// Returns with updated common_hdr and also file_size
//----------------------------------------------------------
int ipmi_validate_common_hdr(const uint8_t *fru_data, const size_t data_len)
{
    int rc = -1;

    uint8_t common_hdr[sizeof(struct common_header)] = {0};
    if(data_len >= sizeof(common_hdr))
    {
        memcpy(common_hdr, fru_data, sizeof(common_hdr));
    }
    else
    {
        fprintf(stderr, "Incomplete fru data file. Size:[%zd]\n", data_len);
        return rc;
    }

    // Verify the crc and size
    rc = verify_fru_data(common_hdr, sizeof(common_hdr));
    if(rc < 0)
    {
        fprintf(stderr, "Failed to validate common header\n");
        return rc;
    }

    return EXIT_SUCCESS;
}

//------------------------------------------------------------
// Cleanup routine
//------------------------------------------------------------
int cleanup_error(FILE *fru_fp, fru_area_vec_t & fru_area_vec)
{
    if(fru_fp != NULL)
    {
        fclose(fru_fp);
        fru_fp = NULL;
    }

    if(!(fru_area_vec.empty()))
    {
        fru_area_vec.clear();
    }

    return  -1;
}

///-----------------------------------------------------
// Accepts the filename and validates per IPMI FRU spec
//----------------------------------------------------
int ipmi_validate_fru_area(const uint8_t fruid, const char *fru_file_name,
                           sd_bus *bus_type, const bool bmc_fru)
{
    size_t data_len = 0;
    size_t bytes_read = 0;
    int rc = -1;

    // Vector that holds individual IPMI FRU AREAs. Although MULTI and INTERNAL
    // are not used, keeping it here for completeness.
    fru_area_vec_t fru_area_vec;

    for(uint8_t fru_entry = IPMI_FRU_INTERNAL_OFFSET;
        fru_entry < (sizeof(struct common_header) -2); fru_entry++)
    {
        // Create an object and push onto a vector.
        std::unique_ptr<ipmi_fru> fru_area = std::make_unique<ipmi_fru>
                         (fruid, get_fru_area_type(fru_entry), bus_type, bmc_fru);

        // Physically being present
        bool present = access(fru_file_name, F_OK) == 0;
        fru_area->set_present(present);

        fru_area_vec.emplace_back(std::move(fru_area));
    }

    FILE *fru_fp = fopen(fru_file_name,"rb");
    if(fru_fp == NULL)
    {
        fprintf(stderr, "ERROR: opening:[%s]\n",fru_file_name);
        perror("Error:");
        return cleanup_error(fru_fp, fru_area_vec);
    }

    // Get the size of the file to see if it meets minimum requirement
    if(fseek(fru_fp, 0, SEEK_END))
    {
        perror("Error:");
        return cleanup_error(fru_fp, fru_area_vec);
    }

    // Allocate a buffer to hold entire file content
    data_len = ftell(fru_fp);
    uint8_t fru_data[data_len] = {0};

    rewind(fru_fp);
    bytes_read = fread(fru_data, data_len, 1, fru_fp);
    if(bytes_read != 1)
    {
        fprintf(stderr, "Failed reading fru data. Bytes_read=[%zd]\n",bytes_read);
        perror("Error:");
        return cleanup_error(fru_fp, fru_area_vec);
    }

    // We are done reading.
    fclose(fru_fp);
    fru_fp = NULL;

    rc = ipmi_validate_common_hdr(fru_data, data_len);
    if(rc < 0)
    {
        return cleanup_error(fru_fp, fru_area_vec);
    }

    // Now that we validated the common header, populate various fru sections if we have them here.
    rc = ipmi_populate_fru_areas(fru_data, data_len, fru_area_vec);
    if(rc < 0)
    {
        fprintf(stderr,"Populating FRU areas failed for:[%d]\n",fruid);
        return cleanup_error(fru_fp, fru_area_vec);
    }
    else
    {
        printf("SUCCESS: Populated FRU areas for:[%s]\n",fru_file_name);
    }

#ifdef __IPMI_DEBUG__
    for(auto& iter : fru_area_vec)
    {
        printf("FRU ID : [%d]\n",(iter)->get_fruid());
        printf("AREA NAME : [%s]\n",(iter)->get_name());
        printf("TYPE : [%d]\n",(iter)->get_type());
        printf("LEN : [%d]\n",(iter)->get_len());
        printf("BUS NAME : [%s]\n", (iter)->get_bus_name());
        printf("OBJ PATH : [%s]\n", (iter)->get_obj_path());
        printf("INTF NAME :[%s]\n", (iter)->get_intf_name());
    }
#endif

    // If the vector is populated with everything, then go ahead and update the
    // inventory.
    if(!(fru_area_vec.empty()))
    {

#ifdef __IPMI_DEBUG__
        printf("\n SIZE of vector is : [%d] \n",fru_area_vec.size());
#endif
        rc = ipmi_update_inventory(fru_area_vec, bus_type);
        if(rc <0)
        {
            fprintf(stderr, "Error updating inventory\n");
        }
    }

    // we are done with all that we wanted to do. This will do the job of
    // calling any destructors too.
    fru_area_vec.clear();

    return rc;
}
