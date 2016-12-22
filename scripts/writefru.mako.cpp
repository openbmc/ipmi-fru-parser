#include "${hppfile}"
FruObjectDict  fruList = {
% for key in fruDict.iterkeys():
   {"${key}",{
<%
    fru = fruDict[key]
%>
    % for object,interfaces in fru.iteritems():
         {"${object}",{
         % for interface,properties in interfaces.iteritems():
             {"${interface}",{
            % for dbus_property,property_value in properties.iteritems():
                 {"${dbus_property}",{
                % if property_value:
                   %for name,value in property_value.iteritems():
                     {"${name}","${value}"},
                   %endfor
                % endif
                 }},
            % endfor
             }},
         % endfor
        }},
    % endfor
   }},
% endfor
};

