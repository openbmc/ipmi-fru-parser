#include "writefrudata.hpp"

#include <CLI/CLI.hpp>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <phosphor-logging/log.hpp>

using namespace phosphor::logging;

static void exit_with_error(const char* err, char** argv)
{
    ArgumentParser::usage(argv);
    std::cerr << std::endl;
    std::cerr << "ERROR: " << err << std::endl;
    exit(-1);
}

static uint8_t parse_fruid_or_exit(const char* fruid_str, char** argv)
{
    unsigned long fruid;
    char* endptr = NULL;

    // The FRUID string must not be empty.
    if (fruid_str == nullptr || *fruid_str == '\0')
    {
        exit_with_error("Empty fruid.", argv);
    }

    errno = 0;
    fruid = std::strtoul(fruid_str, &endptr, 16);

    // Handle error cases
    if (errno == ERANGE)
    {
        exit_with_error("fruid is out of range.", argv);
    }
    if (errno != 0)
    {
        exit_with_error("Could not parse fruid.", argv);
    }
    if (*endptr != '\0')
    {
        // The string was not fully parsed, e.g. contains invalid characters
        exit_with_error("Invalid fruid.", argv);
    }
    if (fruid > MAX_FRU_ID)
    {
        // The string was parsed, but the set FRUID is too large.
        exit_with_error("fruid is out of range.", argv);
    }

    return fruid;
}

//--------------------------------------------------------------------------
// This gets called by udev monitor soon after seeing hog plugs for EEPROMS.
//--------------------------------------------------------------------------
int main(int argc, char** argv)
{
    int rc = 0;
    uint8_t fruid;
    std::string eeprom_file;
    const uint8_t MAX_FRU_ID = 0xfe;

    CLI::App app{"OpenBMC IPMI-FRU-Parser"};
    app.add_option("-e,--eeprom", eeprom_file, "Absolute file name of eeprom")
        ->check(CLI::ExistingFile);
    app.add_option("-f,--fruid", fruid, "valid fru id in integer")
        ->check(CLI::Range(0, MAX_FRU_ID));

    // Read the arguments.
    CLI11_PARSE(app, argc, argv);

    // Now that we have the file that contains the eeprom data, go read it
    // and update the Inventory DB.
    auto bus = sdbusplus::bus::new_default();
    bool bmc_fru = true;
    rc = validateFRUArea(fruid, eeprom_file.c_str(), bus, bmc_fru);

    return (rc < 0 ? EXIT_FAILURE : EXIT_SUCCESS);
}
