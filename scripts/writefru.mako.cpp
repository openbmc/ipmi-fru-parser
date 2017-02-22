// !!! WARNING: This is a GENERATED Code..Please do NOT Edit !!!
#include <iostream>
#include "frup.hpp"

extern const FruMap frus = {
% for key in fruDict.iterkeys():
   % if key:
       {${key},{
<%
       fru = fruDict[key]
%>
       % for object,interfaces in fru.iteritems():
           % if object and interfaces:
               {"${object}",{
               % for interface,properties in interfaces.iteritems():
                   % if interface and properties:
                       {"${interface}",{
                       % for dbus_property,property_value in properties.iteritems():
                           %if dbus_property and property_value:
                               {"${dbus_property}",{
                               % for name,value in property_value.iteritems():
                                   %if name and value:
                                       {"${name}","${value}"},
                                   %endif
                               % endfor
                               }},
                           %endif
                       % endfor
                       }},
                   %endif
               % endfor
               }},
           %endif
       % endfor
       }},
   %endif
% endfor
};
