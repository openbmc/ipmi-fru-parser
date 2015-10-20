#ifndef OPENBMC_IPMI_FRU_PARSER_H
#define OPENBMC_IPMI_FRU_PARSER_H

/* Parse an IPMI write fru data message into a dictionary containing name value pair of VPD entries.*/
int parse_fru (const void* msgbuf, sd_bus_message* vpdtbl);
int parse_fru_area (const uint8_t area, const void* msgbuf, const uint8_t len, sd_bus_message* vpdtbl);

enum ipmi_fru_area_type
{
    IPMI_FRU_AREA_INTERNAL_USE = 0x00,
    IPMI_FRU_AREA_CHASSIS_INFO,
    IPMI_FRU_AREA_BOARD_INFO,
    IPMI_FRU_AREA_PRODUCT_INFO,
    IPMI_FRU_AREA_MULTI_RECORD,
    IPMI_FRU_AREA_TYPE_MAX
};

#endif
