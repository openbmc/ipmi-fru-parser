#!/usr/bin/env python
r"""
This script will parse the input inventory yaml file and generate
a header file and source file which will be used by the ipmiFruParser
to collect and update the inventory manager.

"""
import yaml
import types
import sys
import os
from optparse import OptionParser

fruid_dict = {}


r"""
Filling the c++ structures from the python
structures.
"""

def createCmap(ofile):
    ofile.write('fruObjectDict fruList = ')
    writeValue(ofile, fruid_dict)
    ofile.write(';\n\n')
 
r"""
Write the Python list data in c++
List initializer format
"""


def writeList(ofile, valueList):
    ofile.write('{')
    for value in valueList:
        ofile.write('\"' + value + '\",')     
    ofile.write('}')


r"""
Write the Python Dict data in c++
Map initializer format
"""


def writeDict(ofile, valueDict):
    ofile.write('{\n')

    for key, value in valueDict.iteritems():
        ofile.write('{\n')
        if (type(key).__name__ == 'int'):
            ofile.write('\"' + str(key) + '\",')
        else:
            ofile.write('\"' + key + '\",')
        writeValue(ofile, value)
        ofile.write('},')

    ofile.write('}\n')


r"""
Write the value in the c++ format
by looking at the type of the value.
"""


def writeValue(ofile, value):
    if (type(value).__name__ == 'str'):
        ofile.write('\"' + value + '\"')
    if (type(value).__name__ == 'bool'):
        ofile.write('\"' + str(value) + '\"')
    elif (type(value).__name__ == 'list'):
        writeList(ofile, value)
    elif (type(value).__name__ == 'dict'):
        writeDict(ofile, value)


def gen_inventory_files(i_inventory_yaml):
    with open(i_inventory_yaml, 'r') as f:
        ifile = yaml.safe_load(f)

    with open('inventory-gen.hpp', 'w') as ofile:
        ofile.write('/* !!! WARNING: This is a GENERATED Code..')
        ofile.write('Please do NOT Edit !!! */\n\n')

        ofile.write('#include <iostream>\n')
        ofile.write('#include <map>\n')
        ofile.write('#include <list>\n\n')

        ofile.write('using dict = std::map<std::string,std::string>;\n')
        ofile.write('using dictOfDict = std::map<std::string,dict>;\n\n')

        ofile.write('/* Dictionary of Properties */\n')
        ofile.write('using propertiesDict = std::map<std::string,dictOfDict>;\n\n')

        ofile.write('/* Dictionary of Interface and the Properties */\n')
        ofile.write('using interfacePropertiesDict = std::map<std::string,dictOfDict>;\n\n')

        ofile.write('/* Dictionary of object and the interfaces */\n')
        ofile.write('using objectInterfaceDict = std::map<std::string,interfacePropertiesDict>;\n\n')
   
        ofile.write('/* Dictionary of fru and the objects */\n')
        ofile.write('using fruObjectDict = std::map<std::string,objectInterfaceDict>;\n\n')        

        r"""
        Parse the Yaml and creates the python structure
        eg:- Each FRU is having list of objects which needs to be updated.
             Each object have list of interfaces.
             Each interface having list of Dbus properties.
             Each Dbus Property having the info of IPMI(eg section,ipmiPropName)
        """

        for fru in ifile: #Iterating over all the fru
            fru_id = fru["FRUID"]
            objects = fru["Instances"]
            instance_dict = {}

            for instance in objects: #Iteraring over all the objects of the fru
                object_name  = instance["Instance"]
                interface_list = instance["Interfaces"]

                interface_dict = {}

                for interfaces in interface_list: #Iterating over all the interfaces of the object.
                    interface_name = interfaces["Interface"]
                    properties_list = interfaces["Properties"]
                    properties_dict = {}

                    for prop in properties_list: #Iterating over all the Dbus Properties of the interface.
                        property_name = prop["Property"]
                        prop_dict = {}

                        for key,value in prop.iteritems(): #Iterating over all property info.
                            if(key == "Property"):
                                continue
                            else:
                                prop_dict[key] = value;

                            properties_dict[property_name] = prop_dict
                        interface_dict[interface_name] = properties_dict
                    instance_dict[object_name] = interface_dict
                fruid_dict[fru_id] = instance_dict


    r"""
    Once Association has been built create the  the c++
    structure.
    """ 

    with open('inventory-gen.cpp', 'w') as ofile2:
        ofile2.write('/* !!! WARNING: This is a GENERATED Code..')
        ofile2.write('Please do NOT Edit !!! */\n\n')
        ofile2.write('#include "inventory-gen.hpp"\n\n')
        createCmap(ofile2)


def main(i_args):

    parser = OptionParser()

    parser.add_option("-i", "--inventory", dest="inventory_yaml", default="inventory.yaml",
                      help="input inventory yaml file to parse")

    (options, args) = parser.parse_args(i_args)

    if (not (os.path.isfile(options.inventory_yaml))):
        print "Can not find input yaml file " + options.inventory_yaml
        exit(1)

    gen_inventory_files(options.inventory_yaml)


# Only run if it's a script
if __name__ == '__main__':
    main(sys.argv[1:])       
