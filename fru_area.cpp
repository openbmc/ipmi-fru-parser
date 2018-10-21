#include "fru_area.hpp"

#include "frup.hpp"

#include <cstdint>
#include <cstring>
#include <phosphor-logging/log.hpp>

using namespace phosphor::logging;

//----------------------------------------------------------------
// Constructor
//----------------------------------------------------------------
IPMIFruArea::IPMIFruArea(const uint8_t fruid, const ipmi_fru_area_type type,
                         bool bmc_fru)
{
    fruid = fruid;
    type = type;
    bmc_fru = bmc_fru;
    isValid = false;
    data = NULL;
    isPresent = false;

    if (type == IPMI_FRU_AREA_INTERNAL_USE)
    {
        name = "INTERNAL_";
    }
    else if (type == IPMI_FRU_AREA_CHASSIS_INFO)
    {
        name = "CHASSIS_";
    }
    else if (type == IPMI_FRU_AREA_BOARD_INFO)
    {
        name = "BOARD_";
    }
    else if (type == IPMI_FRU_AREA_PRODUCT_INFO)
    {
        name = "PRODUCT_";
    }
    else if (type == IPMI_FRU_AREA_MULTI_RECORD)
    {
        name = "MULTI_";
    }
    else
    {
        name = IPMI_FRU_AREA_TYPE_MAX;
        log<level::ERR>("Invalid Area", entry("TYPE=%d", type));
    }
}

//-----------------------------------------------------
// For a FRU area type, accepts the data and updates
// area specific data.
//-----------------------------------------------------
void IPMIFruArea::set_data(const uint8_t* data, const size_t len)
{
    len = len;
    data = new uint8_t[len];
    std::memcpy(data, data, len);
}

//-----------------------------------------------------
// Sets the dbus parameters
//-----------------------------------------------------
void IPMIFruArea::update_dbus_paths(const char* bus_name, const char* obj_path,
                                    const char* intf_name)
{
    bus_name = bus_name;
    obj_path = obj_path;
    intf_name = intf_name;
}

//-------------------
// Destructor
//-------------------
IPMIFruArea::~IPMIFruArea()
{
    if (data != NULL)
    {
        delete[] data;
        data = NULL;
    }
}
