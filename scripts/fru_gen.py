#!/usr/bin/env python
import os
import yaml
import sys
from optparse import OptionParser
from mako.template import Template


def gen_inventory_files(i_inventory_yaml, i_output_hpp, i_output_cpp):
    with open(i_inventory_yaml, 'r') as f:
        ifile = yaml.safe_load(f)

    with open(i_output_hpp, 'w') as ofile:
        ofile.write('/* !!! WARNING: This is a GENERATED Code..')
        ofile.write('Please do NOT Edit !!! */\n\n')

        ofile.write('#include <iostream>\n')
        ofile.write('#include <map>\n')
        ofile.write('#include <list>\n\n')

        ofile.write('using MetaDataDict = std::map<std::string,std::string>;\n')

        ofile.write('/* Dictionary of Dbus property and the associated metaData.*/\n')
        ofile.write('using PropertiesDict = std::map<std::string,MetaDataDict>;\n\n')

        ofile.write('/* Dictionary of Interface and the Properties. */\n')
        ofile.write('using InterfacePropertiesDict = std::map<std::string,PropertiesDict>;\n\n')

        ofile.write('/* Dictionary of object and the interfaces. */\n')
        ofile.write('using ObjectInterfaceDict = std::map<std::string,InterfacePropertiesDict>;\n\n')

        ofile.write('/* Dictionary of fru and the objects. */\n')
        ofile.write('using FruObjectDict = std::map<std::string,ObjectInterfaceDict>;\n\n')

        # Render the mako template
        template = os.path.join('./writefru.mako.cpp')
        t = Template(filename=template)
        with open(i_output_cpp, 'w') as fd:
            fd.write(t.render
                    (fruDict = ifile, hppfile = i_output_hpp))


def main():

    sys_args = sys.argv[1:]
    parser = OptionParser()

    parser.add_option("-i", "--inventory",
                      dest="inventory_yaml", default="inventory.yaml",
                      help="input inventory yaml file to parse")

    parser.add_option("-o", "--output_hpp", dest="output_hpp",
                      default="inventory-gen.hpp",
                      help="output to generate, inventory-gen.hpp is default")

    parser.add_option("-p", "--output_cpp", dest="output_cpp",
                      default="inventory-gen.cpp",
                      help="output to generate, inventory-gen.cpp is default")

    (options, args) = parser.parse_args(sys_args)

    if (not (os.path.isfile(options.inventory_yaml))):
        print "Can not find input yaml file " + options.inventory_yaml
        exit(1)

    gen_inventory_files(options.inventory_yaml,
                        options.output_hpp,
                        options.output_cpp)


# Only run if it's a script
if __name__ == '__main__':
    main()
