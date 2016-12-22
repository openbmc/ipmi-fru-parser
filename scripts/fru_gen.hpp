// !!! WARNING: This is a GENERATED Code..Please do NOT Edit !!!
#pragma once

#include <iostream>

#include <string>
#include <list>
#include <map>

using IPMIFruMetadata = std::string;
using IPMIFruMetadataValue = std::string;
using IPMIFruMap = std::map<IPMIFruMetadata,IPMIFruMetadataValue>;

using DbusProperty = std::string;
using DbusPropertyMap = std::map<DbusProperty,IPMIFruMap>;

using DbusInterface = std::string;
using DbusInterfaceMap = std::map<DbusInterface,DbusPropertyMap>;

using FruInstancePath = std::string;
using FruInstanceMap = std::map<FruInstancePath,DbusInterfaceMap>;

using FruId = uint32_t;
using FruMap = std::map<FruId,FruInstanceMap>;


const FruMap frus = {
   {0,{

         {"/system",{
             {"xyz.openbmc_project.Inventory.Revision",{
                 {"Version",{
                     {"IPMIFruSection","Product"},
                     {"IPMIFruProperty","Version"},
                 }},
             }},
             {"xyz.openbmc_project.Inventory.Decorator.Asset",{
                 {"PartNumber",{
                     {"IPMIFruSection","Product"},
                     {"IPMIFruProperty","Part Number"},
                 }},
                 {"SerialNumber",{
                     {"IPMIFruSection","Product"},
                     {"IPMIFruProperty","Serial Number"},
                 }},
                 {"BuildDate",{
                     {"IPMIFruSection","Product"},
                     {"IPMIFruProperty","Mfg Date"},
                 }},
                 {"Manufacturer",{
                     {"IPMIFruSection","Product"},
                     {"IPMIFruProperty","Manufacturer"},
                 }},
             }},
             {"xyz.openbmc_project.Inventory.Item",{
                 {"PrettyName",{
                     {"IPMIFruSection","Product"},
                     {"IPMIFruProperty","Product Name"},
                 }},
             }},
        }},
   }},
   {1,{

         {"/system/chassis/motherboard/dimm0",{
             {"xyz.openbmc_project.Inventory.Revision",{
                 {"Version",{
                     {"IPMIFruSection","Product"},
                     {"IPMIFruProperty","Version"},
                 }},
             }},
             {"xyz.openbmc_project.Inventory.Decorator.Asset",{
                 {"PartNumber",{
                     {"IPMIFruSection","Product"},
                     {"IPMIFruProperty","Part Number"},
                 }},
                 {"SerialNumber",{
                     {"IPMIFruSection","Product"},
                     {"IPMIFruProperty","Serial Number"},
                 }},
                 {"BuildDate",{
                     {"IPMIFruSection","Product"},
                     {"IPMIFruProperty","Mfg Date"},
                 }},
                 {"Manufacturer",{
                     {"IPMIFruSection","Product"},
                     {"IPMIFruProperty","Manufacturer"},
                 }},
             }},
             {"xyz.openbmc_project.Inventory.Item",{
                 {"PrettyName",{
                     {"IPMIFruSection","Product"},
                     {"IPMIFruProperty","Product Name"},
                 }},
             }},
        }},
   }},
   {2,{

         {"/system/chassis/motherboard/dimm1",{
             {"xyz.openbmc_project.Inventory.Revision",{
                 {"Version",{
                     {"IPMIFruSection","Product"},
                     {"IPMIFruProperty","Version"},
                 }},
             }},
             {"xyz.openbmc_project.Inventory.Decorator.Asset",{
                 {"PartNumber",{
                     {"IPMIFruSection","Product"},
                     {"IPMIFruProperty","Part Number"},
                 }},
                 {"SerialNumber",{
                     {"IPMIFruSection","Product"},
                     {"IPMIFruProperty","Serial Number"},
                 }},
                 {"BuildDate",{
                     {"IPMIFruSection","Product"},
                     {"IPMIFruProperty","Mfg Date"},
                 }},
                 {"Manufacturer",{
                     {"IPMIFruSection","Product"},
                     {"IPMIFruProperty","Manufacturer"},
                 }},
             }},
             {"xyz.openbmc_project.Inventory.Item",{
                 {"PrettyName",{
                     {"IPMIFruSection","Product"},
                     {"IPMIFruProperty","Product Name"},
                 }},
             }},
        }},
   }},
   {3,{

         {"/system/chassis/motherboard/cpu0",{
             {"xyz.openbmc_project.Inventory.Decorator.Asset",{
                 {"PartNumber",{
                     {"IPMIFruSection","Board"},
                     {"IPMIFruProperty","Part Number"},
                 }},
                 {"SerialNumber",{
                     {"IPMIFruSection","Board"},
                     {"IPMIFruProperty","Serial Number"},
                 }},
                 {"BuildDate",{
                     {"IPMIFruSection","Board"},
                     {"IPMIFruProperty","Mfg Date"},
                 }},
                 {"Manufacturer",{
                     {"IPMIFruSection","Board"},
                     {"IPMIFruProperty","Manufacturer"},
                 }},
             }},
             {"xyz.openbmc_project.Inventory.Item",{
                 {"PrettyName",{
                     {"IPMIFruSection","Board"},
                     {"IPMIFruProperty","Product Name"},
                 }},
             }},
        }},
   }},
   {4,{

         {"/system/chassis/motherboard/cpu1",{
             {"xyz.openbmc_project.Inventory.Decorator.Asset",{
                 {"PartNumber",{
                     {"IPMIFruSection","Board"},
                     {"IPMIFruProperty","Part Number"},
                 }},
                 {"SerialNumber",{
                     {"IPMIFruSection","Board"},
                     {"IPMIFruProperty","Serial Number"},
                 }},
                 {"BuildDate",{
                     {"IPMIFruSection","Board"},
                     {"IPMIFruProperty","Mfg Date"},
                 }},
                 {"Manufacturer",{
                     {"IPMIFruSection","Board"},
                     {"IPMIFruProperty","Manufacturer"},
                 }},
             }},
             {"xyz.openbmc_project.Inventory.Item",{
                 {"PrettyName",{
                     {"IPMIFruSection","Board"},
                     {"IPMIFruProperty","Product Name"},
                 }},
             }},
        }},
   }},
};
