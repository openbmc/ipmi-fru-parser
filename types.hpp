#pragma once

#include <string>
#include <map>
#include <sdbusplus/server.hpp>

namespace ipmi
{
namespace vpd
{

    using Path = std::string;

    using Property = std::string;
    using Value = sdbusplus::message::variant<bool, int64_t, std::string>;
    using PropertyMap = std::map<Property, Value>;

    using Interface = std::string;
    using InterfaceMap = std::map<Interface, PropertyMap>;

    using Object = sdbusplus::message::object_path;
    using ObjectMap = std::map<Object, InterfaceMap>;

} //namespace vpd
} //namespace ipmi
