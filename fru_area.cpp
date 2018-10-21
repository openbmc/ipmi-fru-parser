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
                         bool bmc_fru) :
    iv_fruid(fruid),
    iv_type(type), iv_bmc_fru(bmc_fru)
{
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
void IPMIFruArea::set_data(const uint8_t* data, const size_t len)
{
    iv_len = len;
    iv_data = new uint8_t[len];
    std::memcpy(iv_data, data, len);
}

//-----------------------------------------------------
// Sets the dbus parameters
//-----------------------------------------------------
void IPMIFruArea::update_dbus_paths(const char* bus_name, const char* obj_path,
                                    const char* intf_name)
{
    iv_bus_name = bus_name;
    iv_obj_path = obj_path;
    iv_intf_name = intf_name;
}

//-------------------
// Destructor
//-------------------
IPMIFruArea::~IPMIFruArea()
{
    if (iv_data != NULL)
    {
        delete[] iv_data;
        iv_data = NULL;
    }
}
