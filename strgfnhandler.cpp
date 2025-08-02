#include "writefrudata.hpp"

#include <unistd.h>

#include <ipmid/api-types.hpp>
#include <ipmid/handler.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>

#include <cstdio>
#include <cstring>

void registerNetFnStorageWriteFru() __attribute__((constructor));

sd_bus* ipmid_get_sd_bus_connection(void);

///-------------------------------------------------------
// Called by IPMI netfn router for write fru data command
//--------------------------------------------------------
ipmi::RspType<uint8_t> ipmiStorageWriteFruData(uint8_t fruId, uint16_t offset,
                                               std::vector<uint8_t>& buffer)
{
    FILE* fp = nullptr;
    char fruFilename[16] = {0};
    std::string mode = "rb+";

    // Maintaining a temporary file to pump the data
    std::sprintf(fruFilename, "%s%02x", "/tmp/ipmifru", fruId);

    if (access(fruFilename, F_OK) == -1)
    {
        mode = "wb";
    }

    if ((fp = std::fopen(fruFilename, mode.c_str())) != nullptr)
    {
        if (std::fseek(fp, offset, SEEK_SET))
        {
            lg2::error(
                "Seek into fru file failed, file name: {FILE}, errno: {ERRNO}",
                "FILE", fruFilename, "ERRNO", std::strerror(errno));
            std::fclose(fp);

            return ipmi::responseInvalidFieldRequest();
        }

        if (std::fwrite(buffer.data(), 1, buffer.size(), fp) != buffer.size())
        {
            lg2::error(
                "Write into fru file failed, file name: {FILE}, errno: {ERRNO}",
                "FILE", fruFilename, "ERRNO", std::strerror(errno));
            std::fclose(fp);

            return ipmi::responseInvalidFieldRequest();
        }

        std::fclose(fp);
    }
    else
    {
        lg2::error("Error trying to write to {FILE}", "FILE", fruFilename);

        return ipmi::responseInvalidFieldRequest();
    }

    // Get the reference to global sd_bus object
    sd_bus* bus_type = ipmid_get_sd_bus_connection();

    // We received some bytes. It may be full or partial. Send a valid
    // FRU file to the inventory controller on DBus for the correct number
    sdbusplus::bus_t bus{bus_type};
    validateFRUArea(fruId, fruFilename, bus);

    return ipmi::responseSuccess(buffer.size());
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

    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnStorage,
                          ipmi::storage::cmdWriteFruData,
                          ipmi::Privilege::Admin, ipmiStorageWriteFruData);
}
