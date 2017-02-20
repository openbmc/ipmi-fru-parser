## This file is a template.  The comment below is emitted
## into the rendered file; feel free to edit this file.
// WARNING: Generated source. Do not edit!

#include "types.hpp"

using namespace ipmi::vpd;

extern const std::map<Path, InterfaceMap> extras = {
% for path in dict.iterkeys():
<%
    interfaces = dict[path]
%>\
    {"${path}",{
    % for interface,properties in interfaces.iteritems():
        {"${interface}",{
        % for property,value in properties.iteritems():
            {"${property}", ${value}},
        % endfor
        }},
    % endfor
    }},
% endfor
};
