// !!! WARNING: This is a GENERATED Code..Please do NOT Edit !!!
#include <iostream>
#include "frup.hpp"

extern const FruMap frus = {
% for key in fruDict.keys():
   {${key},{
<%
    fru = fruDict[key]
%>
    % for object,interfaces in fru.items():
         {"${object}",{
         % for interface,properties in interfaces.items():
             {"${interface}",{
            % for dbus_property,property_value in properties.items():
                 {"${dbus_property}",{
                % for name,value in property_value.items():
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
