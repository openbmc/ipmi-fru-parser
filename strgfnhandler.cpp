#include "writefrudata.hpp"

#include <ipmid/api.h>
#include <unistd.h>

#include <ipmid/api-types.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>

#include <cstdio>
#include <cstring>

void registerNetFnStorageWriteFru() __attribute__((constructor));

sd_bus* ipmid_get_sd_bus_connection(void);

///-------------------------------------------------------
// Called by IPMI netfn router for write fru data command
//--------------------------------------------------------
ipmi_ret_t ipmiStorageWriteFruData(
    ipmi_netfn_t /*netfn*/, ipmi_cmd_t /*cmd*/, ipmi_request_t request,
    ipmi_response_t response, ipmi_data_len_t dataLen,
    ipmi_context_t /*context*/)
{
    FILE* fp = nullptr;
    char fruFilename[16] = {0};
    size_t offset = 0;
    size_t len = 0;
    ipmi_ret_t rc = ipmi::ccInvalidCommand;
    const char* mode = nullptr;

    // From the payload, extract the header that has fruid and the offsets
    auto reqptr = static_cast<write_fru_data_t*>(request);

    // Maintaining a temporary file to pump the data
    std::sprintf(fruFilename, "%s%02x", "/tmp/ipmifru", reqptr->frunum);

    offset = ((size_t)reqptr->offsetms) << 8 | reqptr->offsetls;

    // Length is the number of request bytes minus the header itself.
    // The header contains an extra byte to indicate the start of
    // the data (so didn't need to worry about word/byte boundaries)
    // hence the -1...
    len = ((size_t)*dataLen) - (sizeof(write_fru_data_t) - 1);

    // On error there is no response data for this command.
    *dataLen = 0;

    lg2::debug(
        "IPMI WRITE-FRU-DATA, file name: {FILE}, offset: {OFFSET}, length: {LENGTH}",
        "FILE", fruFilename, "OFFSET", offset, "LENGTH", len);

    if (access(fruFilename, F_OK) == -1)
    {
        mode = "wb";
    }
    else
    {
        mode = "rb+";
    }

    if ((fp = std::fopen(fruFilename, mode)) != nullptr)
    {
        if (std::fseek(fp, offset, SEEK_SET))
        {
            lg2::error(
                "Seek into fru file failed, file name: {FILE}, errno: {ERRNO}",
                "FILE", fruFilename, "ERRNO", std::strerror(errno));
            std::fclose(fp);
            return rc;
        }

        if (std::fwrite(&reqptr->data, len, 1, fp) != 1)
        {
            lg2::error(
                "Write into fru file failed, file name: {FILE}, errno: {ERRNO}",
                "FILE", fruFilename, "ERRNO", std::strerror(errno));
            std::fclose(fp);
            return rc;
        }

        std::fclose(fp);
    }
    else
    {
        lg2::error("Error trying to write to {FILE}", "FILE", fruFilename);
        return rc;
    }

    // If we got here then set the response byte
    // to the number of bytes written
    std::memcpy(response, &len, 1);
    *dataLen = 1;

    // Get the reference to global sd_bus object
    sd_bus* bus_type = ipmid_get_sd_bus_connection();

    // We received some bytes. It may be full or partial. Send a valid
    // FRU file to the inventory controller on DBus for the correct number
    sdbusplus::bus_t bus{bus_type};
    return validateFRUArea(reqptr->frunum, fruFilename, bus);
}

//-------------------------------------------------------
// Registering WRITE FRU DATA command handler with daemon
//-------------------------------------------------------
void registerNetFnStorageWriteFru()
{
    lg2::info(
        "Registering WRITE FRU DATA command handler, netfn:{NETFN}, cmd:{CMD}",
        "NETFN", lg2::hex, ipmi::netFnStorage, "CMD", lg2::hex,
        ipmi::storage::cmdWriteFruData);

    ipmi_register_callback(ipmi::netFnStorage, ipmi::storage::cmdWriteFruData,
                           nullptr, ipmiStorageWriteFruData, SYSTEM_INTERFACE);
}
