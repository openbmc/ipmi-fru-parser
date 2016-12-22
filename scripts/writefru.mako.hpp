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
