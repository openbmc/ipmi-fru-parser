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

fru_device_dict = {}
fru_info_dict = {}
fru_properties_dict = {}
instance_info_dict = {}
instance_properties_dict = {}

r"""
Creates the association
between deviceInstance and its info.
eg: bmc0: {
            instancePath: xxxx
             type:- BMC
          }
"""

def createInstanceInfoAssociation(instance, device):
    infodict = {}
    for key,value in device.iteritems():
        if (key == "Properties"):
           createInstancePropertiesAssociation(value,instance)
           continue
        infodict[key] = value
    instance_info_dict[instance] = infodict

r"""
Creates the association
between deviceInstance and its properties.
eg: bmc0:
         - flash1:
              Interface:
              ObjectPath:
              FruSection:
              VpdPropertyName:

         - flash2:
"""


def createInstancePropertiesAssociation(instanceProperties, instance):
    propertiesdict = {}
    for propDict in instanceProperties:
        prop_dict = {}
        for key, value in propDict.iteritems():
            if(key == "Property"):
                continue
            prop_dict[key] = value
        propertiesdict[propDict["Property"]] = prop_dict
    instance_properties_dict[instance] = propertiesdict

r"""
Create the association
between fruid and the aasociated devices
eg: fruid:[bmc0,bmc1,eth0,eth1]

"""


def createFruDeviceAssociation(fru_id, devices):
    instance_list = []
    for device in devices:
        instance = device["Instance"]
        instance_list.append(instance)
        createInstanceInfoAssociation(instance, device)
    fru_device_dict[fru_id] = instance_list


r"""
Create the association
between fruid and its info.
eg: fruid: {
            instancePath: xxxx
             type:- FruType
          }
"""


def createFruinfoAssociation(fru_id, fruinfo):
    infodict = {}
    for name, value in fruinfo.iteritems():
        infodict[name] = value
    fru_info_dict[fru_id] = infodict


r"""
Create the association
between fruid and its properties.
eg: fruid:
         - flash1 :
              Interface:
              ObjectPath:
              FruSection:
              VpdPropertyName:

         - flash2:
"""


def createFruPropertiesAssociation(fru_id, fruProperties):
    propertiesdict = {}
    for propDict in fruProperties:
        prop_dict = {}
        for key, value in propDict.iteritems():
            prop_dict[key] = value
        propertiesdict[propDict["Property"]] = prop_dict
    fru_properties_dict[fru_id] = propertiesdict


r"""
Filling the c++ structures from the python
structures.
"""


def createFruDeviceMap(ofile):
    ofile.write('fruDict fruDevice = ')
    writeValue(ofile, fru_device_dict)
    ofile.write(';\n\n')


def createFRUPropertiesMap(ofile):
    ofile.write('fruPropertiesDict fruProperties = ')
    writeValue(ofile, fru_properties_dict)
    ofile.write(';\n\n')


def createFRUInfoMap(ofile):
    ofile.write('fruInfoDict fruInfo = ')
    writeValue(ofile, fru_info_dict)
    ofile.write(';\n\n')


def createInstancePropertiesMap(ofile):
    ofile.write('instancePropertiesDict instanceProperties = ')
    writeValue(ofile, instance_properties_dict)
    ofile.write(';\n\n')


def createInstanceInfoMap(ofile):
    ofile.write('instanceInfoDict instanceInfo = ')
    writeValue(ofile, instance_info_dict)
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


def gen_inventory_files(i_inventory_yaml, i_output_hpp, i_output_cpp):
    with open(i_inventory_yaml, 'r') as f:
        ifile = yaml.safe_load(f)

    with open(i_output_hpp, 'w') as ofile:
        ofile.write('/* !!! WARNING: This is a GENERATED Code..')
        ofile.write('Please do NOT Edit !!! */\n\n')

        ofile.write('#include <iostream>\n')
        ofile.write('#include <map>\n')
        ofile.write('#include <list>\n\n')

        ofile.write('using dict = std::map<std::string,std::string>;\n')
        ofile.write('using dictOfDict = std::map<std::string,dict>;\n\n')

        ofile.write('/* Association between the fruid and the associated device instances */\n')
        ofile.write('using fruDict = std::map<std::string,std::list<std::string>>;\n\n')

        ofile.write('/* Association between the instance names and the instance info.*/\n')
        ofile.write('using instanceInfoDict = dictOfDict;\n\n')

        ofile.write('/* Association between the instance names and the instance properties */\n')
        ofile.write('using instancePropertiesDict = std::map<std::string,dictOfDict>;\n\n')

        ofile.write('/* Association between the fruid and the fruinfo */\n')
        ofile.write('using fruInfoDict = std::map<std::string,dict>;\n\n')

        ofile.write('/* Association between the fruid and the properties of the fru */\n')
        ofile.write('using fruPropertiesDict = std::map<std::string,dictOfDict>;\n\n')

        r"""
        Create the association
        1) fruid ---list of DeviceInstances
        2) DeviceInstance -----DeviceInstanceInfo
        3) DeviceInstance------DeviceInstanceProperties
        4) fruid ----FruInfo
        5) fruid ----FruProperties
        """

        for fru in ifile:
            fru_id = fru["FRUID"]
            devices = fru["Deviceinstances"]
            fruinfo = fru["FRUInfo"]
            fruProperties = fru["Properties"]
            if devices is not None:
                createFruDeviceAssociation(fru_id, devices)
            if fruinfo is not None:
                createFruinfoAssociation(fru_id, fruinfo)
            if fruProperties is not None:
                createFruPropertiesAssociation(fru_id, fruProperties)

    r"""
    Once Association has been built create the  the c++
    structure.
    """

    with open(i_output_cpp, 'w') as ofile2:
        ofile2.write('/* !!! WARNING: This is a GENERATED Code..')
        ofile2.write('Please do NOT Edit !!! */\n\n')
        ofile2.write('#include '+ i_output_hpp +'\n\n')
        createFruDeviceMap(ofile2)
        createFRUPropertiesMap(ofile2)
        createFRUInfoMap(ofile2)
        createInstancePropertiesMap(ofile2)
        createInstanceInfoMap(ofile2)


def main(i_args):

    parser = OptionParser()

    parser.add_option("-i", "--inventory", dest="inventory_yaml", default="inventory.yaml",
                      help="input inventory yaml file to parse")

    parser.add_option("-o", "--output_hpp", dest="output_hpp",
                      default="inventory-gen.hpp",
                      help="output to generate, inventory-gen.hpp is defualt")

    parser.add_option("-p", "--output_cpp", dest="output_cpp",
                      default="inventory-gen.cpp",
                      help="output to generate, inventory-gen.cpp is defualt")

    (options, args) = parser.parse_args(i_args)

    if (not (os.path.isfile(options.inventory_yaml))):
        print "Can not find input yaml file " + options.inventory_yaml
        exit(1)

    gen_inventory_files(options.inventory_yaml,
                 options.output_hpp,
                 options.output_cpp)


# Only run if it's a script
if __name__ == '__main__':
    main(sys.argv[1:])
