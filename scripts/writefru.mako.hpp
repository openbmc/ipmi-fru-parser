// !!! WARNING: This is a GENERATED Code..Please do NOT Edit !!!
#pragma once

#include <iostream>

#include <list>
#include <map>

// Dictionary of meta data eg IPMI Propertyname,FRU Section
// for a specific DBUS property.
using MetaDataMap = std::map<std::string,std::string>;

// Dictionary of Dbus property and the associated metaData.
using PropertiesMap = std::map<std::string,MetaDataMap>;

// Dictionary of Interface and the Properties.
using InterfacePropertiesMap = std::map<std::string,PropertiesMap>;

// Dictionary of object and the interfaces.
using ObjectInterfaceMap = std::map<std::string,InterfacePropertiesMap>;

// Dictionary of fru and the objects.
using FruObjectMap = std::map<int,ObjectInterfaceMap>;


const FruObjectMap  frus = {
% for key in fruDict.iterkeys():
   {${key},{
<%
    fru = fruDict[key]
%>
    % for object,interfaces in fru.iteritems():
         {"${object}",{
         % for interface,properties in interfaces.iteritems():
             {"${interface}",{
            % for dbus_property,property_value in properties.iteritems():
                 {"${dbus_property}",{
                % for name,value in property_value.iteritems():
                     {"${name}","${value}"},
                % endfor
                 }},
            % endfor
             }},
         % endfor
        }},
    % endfor
   }},
% endfor
};
