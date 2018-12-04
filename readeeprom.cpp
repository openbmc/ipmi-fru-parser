#include "argument.hpp"
#include "writefrudata.hpp"

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

//--------------------------------------------------------------------------
// This gets called by udev monitor soon after seeing hog plugs for EEPROMS.
//--------------------------------------------------------------------------
int main(int argc, char** argv)
{
    const uint8_t INVALID_FRU_ID = 0xff;
    int rc = 0;
    uint8_t fruid = INVALID_FRU_ID;
    char* endptr = NULL;

    // Read the arguments.
    auto cli_options = std::make_unique<ArgumentParser>(argc, argv);

    // Parse out each argument.
    auto eeprom_file = (*cli_options)["eeprom"];
    if (eeprom_file == ArgumentParser::empty_string)
    {
        // User has not passed in the appropriate argument value
        exit_with_error("eeprom data not found.", argv);
    }

    auto fruid_str = (*cli_options)["fruid"];
    if (fruid_str == ArgumentParser::empty_string)
    {
        // User has not passed in the appropriate argument value
        exit_with_error("fruid data not found.", argv);
    }

    // Extract the fruid
    fruid = std::strtol(fruid_str.c_str(), &endptr, 16);

    // On parse error 0 is returned and endptr points to the input string.
    if (fruid == INVALID_FRU_ID ||
        (fruid == 0 && endptr == fruid_str.c_str()))
    {
        // User has not passed in the appropriate argument value
        exit_with_error("Invalid fruid.", argv);
    }

    // Finished getting options out, so release the parser.
    cli_options.release();

    // Now that we have the file that contains the eeprom data, go read it
    // and update the Inventory DB.
    auto bus = sdbusplus::bus::new_default();
    bool bmc_fru = true;
    rc = validateFRUArea(fruid, eeprom_file.c_str(), bus, bmc_fru);

    return (rc < 0 ? EXIT_FAILURE : EXIT_SUCCESS);
}
