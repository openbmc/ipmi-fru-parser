#ifndef OPENBMC_IPMI_FRU_PARSER_H
#define OPENBMC_IPMI_FRU_PARSER_H

/* Parse an IPMI write fru data message into a dictionary containing name value pair of VPD entries.*/
int parse_fru (const void* msgbuf, sd_bus_message* vpdtbl);

#endif
