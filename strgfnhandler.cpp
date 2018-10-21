#include "writefrudata.hpp"

#include <host-ipmid/ipmid-api.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>

void register_netfn_storage_write_fru() __attribute__((constructor));

sd_bus* ipmid_get_sd_bus_connection(void);

using namespace phosphor::logging;

///-------------------------------------------------------
// Called by IPMI netfn router for write fru data command
//--------------------------------------------------------
ipmi_ret_t ipmi_storage_write_fru_data(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                                       ipmi_request_t request,
                                       ipmi_response_t response,
                                       ipmi_data_len_t data_len,
                                       ipmi_context_t context)
{
    FILE* fp = NULL;
    char fru_file_name[16] = {0};
    uint8_t offset = 0;
    uint16_t len = 0;
    ipmi_ret_t rc = IPMI_CC_INVALID;
    const char* mode = NULL;

    // From the payload, extract the header that has fruid and the offsets
    auto reqptr = static_cast<write_fru_data_t*>(request);

    // Maintaining a temporary file to pump the data
    std::sprintf(fru_file_name, "%s%02x", "/tmp/ipmifru", reqptr->frunum);

    offset = ((uint16_t)reqptr->offsetms) << 8 | reqptr->offsetls;

    // Length is the number of request bytes minus the header itself.
    // The header contains an extra byte to indicate the start of
    // the data (so didn't need to worry about word/byte boundaries)
    // hence the -1...
    len = ((uint16_t)*data_len) - (sizeof(write_fru_data_t) - 1);

    // On error there is no response data for this command.
    *data_len = 0;

#ifdef __IPMI__DEBUG__
    log<level::DEBUG>("IPMI WRITE-FRU-DATA", entry("FILE=%s", fru_file_name),
                      entry("OFFSET=%d", offset), entry("LENGTH=%d", len));
#endif

    if (access(fru_file_name, F_OK) == -1)
    {
        mode = "wb";
    }
    else
    {
        mode = "rb+";
    }

    if ((fp = std::fopen(fru_file_name, mode)) != NULL)
    {
        if (std::fseek(fp, offset, SEEK_SET))
        {
            log<level::ERR>("Seek into fru file failed",
                            entry("FILE=%s", fru_file_name),
                            entry("ERRNO=%s", std::strerror(errno)));
            std::fclose(fp);
            return rc;
        }

        if (std::fwrite(&reqptr->data, len, 1, fp) != 1)
        {
            log<level::ERR>("Write into fru file failed",
                            entry("FILE=%s", fru_file_name),
                            entry("ERRNO=%s", std::strerror(errno)));
            std::fclose(fp);
            return rc;
        }

        std::fclose(fp);
    }
    else
    {
        log<level::ERR>("Error trying to write to fru file",
                        entry("FILE=%s", fru_file_name));
        return rc;
    }

    // If we got here then set the resonse byte
    // to the number of bytes written
    std::memcpy(response, &len, 1);
    *data_len = 1;
    rc = IPMI_CC_OK;

    // Get the reference to global sd_bus object
    sd_bus* bus_type = ipmid_get_sd_bus_connection();

    // We received some bytes. It may be full or partial. Send a valid
    // FRU file to the inventory controller on DBus for the correct number
    sdbusplus::bus::bus bus{bus_type};
    bool bmc_fru = false;
    validateFRUArea(reqptr->frunum, fru_file_name, bus, bmc_fru);

    return rc;
}

//-------------------------------------------------------
// Registering WRITE FRU DATA command handler with daemon
//-------------------------------------------------------
void register_netfn_storage_write_fru()
{
    std::printf("Registering NetFn:[0x%X], Cmd:[0x%X]\n", NETFUN_STORAGE,
                IPMI_CMD_WRITE_FRU_DATA);

    ipmi_register_callback(NETFUN_STORAGE, IPMI_CMD_WRITE_FRU_DATA, NULL,
                           ipmi_storage_write_fru_data, SYSTEM_INTERFACE);
}
