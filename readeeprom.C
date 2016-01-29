#include <iostream>
#include <memory>
#include "argument.H"
#include "writefrudata.H"

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
int main(int argc, char **argv)
{
    int rc = 0;
    uint8_t fruid = 0;

    // Handle to per process system bus
    sd_bus *bus_type = NULL;

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
    if (eeprom_file == ArgumentParser::empty_string)
    {
        // User has not passed in the appropriate argument value
        exit_with_error("fruid data not found.", argv);
    }

    // Extract the fruid
    fruid = strtol(fruid_str.c_str(), NULL, 16);
    if(fruid == 0)
    {
        // User has not passed in the appropriate argument value
        exit_with_error("Invalid fruid.", argv);
    }

    // Finished getting options out, so release the parser.
    cli_options.release();

    // Get a handle to System Bus
    rc = sd_bus_open_system(&bus_type);
    if (rc < 0)
    {
        fprintf(stderr, "Failed to connect to system bus: %s\n",strerror(-rc));
    }
    else
    {
        // Now that we have the file that contains the eeprom data, go read it and
        // update the Inventory DB.
        bool bmc_fru = true;
        rc = ipmi_validate_fru_area(fruid, eeprom_file.c_str(), bus_type, bmc_fru);
    }

    // Cleanup
    sd_bus_unref(bus_type);

    return (rc < 0 ? EXIT_FAILURE : EXIT_SUCCESS);
}
