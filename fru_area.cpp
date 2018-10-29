#include "fru_area.hpp"

#include "frup.hpp"

#include <cstdint>
#include <cstring>
#include <phosphor-logging/log.hpp>
#include <string>
#include <unordered_map>

using namespace phosphor::logging;

std::string IPMIFruArea::getNameFromType(ipmi_fru_area_type type)
{
    static const std::unordered_map<ipmi_fru_area_type, std::string> areaName =
        {
            {IPMI_FRU_AREA_INTERNAL_USE, "INTERNAL_"},
            {IPMI_FRU_AREA_CHASSIS_INFO, "CHASSIS_"},
            {IPMI_FRU_AREA_BOARD_INFO, "BOARD_"},
            {IPMI_FRU_AREA_PRODUCT_INFO, "PRODUCT_"},
            {IPMI_FRU_AREA_MULTI_RECORD, "MULTI_"},
        };

    auto search = areaName.find(type);
    if (search == areaName.end())
    {
        log<level::ERR>("Invalid Area", entry("TYPE=%d", type));
        // IPMI_FRU_AREA_TYPE_MAX is not a string...
        return "";
    }

    return search->second;
}

IPMIFruArea::IPMIFruArea(const uint8_t fruID, const ipmi_fru_area_type type,
                         bool bmcOnlyFru) :
    fruID(fruID),
    type(type), name(getNameFromType(type)), bmcOnlyFru(bmcOnlyFru)
{
}

void IPMIFruArea::setData(const uint8_t* value, const size_t length)
{
    data.reserve(length); // pre-allocate the space.
    data.insert(data.begin(), value, value + length);
}
