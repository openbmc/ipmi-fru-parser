#include "fru_area.hpp"

#include "frup.hpp"

#include <cstdint>
#include <cstring>
#include <phosphor-logging/log.hpp>
#include <string>

using namespace phosphor::logging;

std::string IPMIFruArea::getTypeFromName(ipmi_fru_area_type type)
{
    std::string name;

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

    return name;
}

IPMIFruArea::IPMIFruArea(const uint8_t fruID, const ipmi_fru_area_type type,
                         bool bmcOnlyFru) :
    fruID(fruID),
    type(type), bmcOnlyFru(bmcOnlyFru), name(getTypeFromName(type))
{
}

void IPMIFruArea::setData(const uint8_t* value, const size_t length)
{
    data.reserve(length);  // pre-allocate the space.
    data.insert(data.begin(), value, value+length);
}
