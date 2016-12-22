#include "inventory-gen.hpp"
FruObjectDict  fruList = {
   {"1",{

         {"/system",{
             {"xyz.openbmc_project.Inventory.Decorator.Replaceable",{
                 {"FieldReplaceable",{
                     {"DefaultValue","True"},
                 }},
             }},
             {"xyz.openbmc_project.Inventory.Decorator.Asset",{
                 {"PartNumber",{
                     {"FRUSection","PRODUCT"},
                     {"IPMI_PropertyName","Board Part Number"},
                 }},
                 {"SerialNumber",{
                     {"FRUSection","PRODUCT"},
                     {"IPMI_PropertyName","Board Serial Number"},
                 }},
                 {"Manufacturer",{
                     {"FRUSection","PRODUCT"},
                     {"IPMI_PropertyName","Board Manufacturer"},
                 }},
             }},
             {"xyz.openbmc_project.Inventory.Decorator.Revision",{
                 {"Version",{
                     {"FRUSection","PRODUCT"},
                     {"IPMI_PropertyName","Product Version"},
                 }},
             }},
             {"xyz.openbmc_project.Inventory.Item",{
                 {"Present",{
                     {"DefaultValue","True"},
                 }},
             }},
        }},
   }},
   {"3",{

         {"/system/chassis/motherboard/cpu0",{
             {"xyz.openbmc_project.Inventory.Decorator.Replaceable",{
                 {"FieldReplaceable",{
                     {"DefaultValue","True"},
                 }},
             }},
             {"xyz.openbmc_project.Inventory.Decorator.Asset",{
                 {"PartNumber",{
                     {"FRUSection","BOARD"},
                     {"IPMI_PropertyName","Board Part Number"},
                 }},
                 {"SerialNumber",{
                     {"FRUSection","BOARD"},
                     {"IPMI_PropertyName","Board Serial Number"},
                 }},
                 {"Manufacturer",{
                     {"FRUSection","BOARD"},
                     {"IPMI_PropertyName","Board Manufacturer"},
                 }},
             }},
             {"xyz.openbmc_project.Inventory.Item",{
                 {"Present",{
                     {"DefaultValue","True"},
                 }},
             }},
        }},
         {"/system/chassis/motherboard/cpu0/core0",{
             {"xyz.openbmc_project.Inventory.Decorator.Replaceable",{
                 {"FieldReplaceable",{
                     {"DefaultValue","True"},
                 }},
             }},
             {"xyz.openbmc_project.Inventory.Item",{
                 {"Present",{
                     {"DefaultValue","True"},
                 }},
             }},
        }},
         {"/system/chassis/motherboard/cpu0/core2",{
             {"xyz.openbmc_project.Inventory.Decorator.Replaceable",{
                 {"FieldReplaceable",{
                     {"DefaultValue","True"},
                 }},
             }},
             {"xyz.openbmc_project.Inventory.Item",{
                 {"Present",{
                     {"DefaultValue","True"},
                 }},
             }},
        }},
         {"/system/chassis/motherboard/cpu0/core3",{
             {"xyz.openbmc_project.Inventory.Decorator.Replaceable",{
                 {"FieldReplaceable",{
                     {"DefaultValue","True"},
                 }},
             }},
             {"xyz.openbmc_project.Inventory.Item",{
                 {"Present",{
                     {"DefaultValue","True"},
                 }},
             }},
        }},
         {"/system/chassis/motherboard/cpu0/core1",{
             {"xyz.openbmc_project.Inventory.Decorator.Replaceable",{
                 {"FieldReplaceable",{
                     {"DefaultValue","True"},
                 }},
             }},
             {"xyz.openbmc_project.Inventory.Item",{
                 {"Present",{
                     {"DefaultValue","True"},
                 }},
             }},
        }},
   }},
};

