#include <iostream>
#include <iterator>
#include <algorithm>
#include <cassert>
#include "argument.hpp"

ArgumentParser::ArgumentParser(int argc, char** argv)
{
    int option = 0;
    while(-1 != (option = getopt_long(argc, argv, optionstr, options, NULL)))
    {
        if ((option == '?') || (option == 'h'))
        {
            usage(argv);
            exit(-1);
        }

        auto i = &options[0];
        while ((i->val != option) && (i->val != 0)) ++i;

        if (i->val)
            arguments[i->name] = (i->has_arg ? optarg : true_string);
    }
}

const std::string& ArgumentParser::operator[](const std::string& opt)
{
    auto i = arguments.find(opt);
    if (i == arguments.end())
    {
        return empty_string;
    }
    else
    {
        return i->second;
    }
}

void ArgumentParser::usage(char** argv)
{
    std::cerr << "Usage: " << argv[0] << " [options]" << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << " --eeprom=<eeprom file path> Absolute file name of eeprom"
        << std::endl;
    std::cerr << " --fruid=<FRU ID>            valid fru id in integer"
        << std::endl;
    std::cerr << " --help                      display help"
        << std::endl;
}

const option ArgumentParser::options[] =
{
    { "eeprom",   required_argument,  NULL, 'e' },
    { "fruid",    required_argument,  NULL, 'f' },
    { "help",     no_argument,        NULL, 'h' },
    { 0, 0, 0, 0},
};

const char* ArgumentParser::optionstr = "e:f:?h";

const std::string ArgumentParser::true_string = "true";
const std::string ArgumentParser::empty_string = "";
