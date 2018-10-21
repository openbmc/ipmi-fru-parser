#ifndef __IPMI_FRU_AREA_H__
#define __IPMI_FRU_AREA_H__

#include "frup.hpp"
#include "writefrudata.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

using std::uint8_t;

class IPMIFruArea
{
  private:
    // Unique way of identifying a FRU
    uint8_t fruid = 0;

    // Type of the fru matching offsets in common header
    ipmi_fru_area_type type = IPMI_FRU_AREA_INTERNAL_USE;

    // Name of the fru area. ( BOARD/CHASSIS/PRODUCT )
    std::string name;

    // Length of a specific fru area.
    size_t len = 0;

    // Special bit for BMC readable eeprom only.
    bool bmc_fru = false;

    // If a FRU is physically present.
    bool isPresent = false;

    // Whether a particular area is valid ?
    bool isValid = false;

    // Actual area data.
    uint8_t* data = nullptr;

    // fru inventory dbus name
    std::string bus_name;

    // fru inventory dbus object path
    std::string obj_path;

    // fru inventory dbus interface name
    std::string intf_name;

    // Default constructor disabled.
    IPMIFruArea();

  public:
    // constructor
    IPMIFruArea(const uint8_t fruid, const ipmi_fru_area_type type,
                bool bmc_fru = false);

    // Destructor
    virtual ~IPMIFruArea();

    // Sets the present bit
    inline void set_present(const bool present)
    {
        isPresent = present;
    }

    // returns fru id;
    uint8_t get_fruid() const
    {
        return fruid;
    }

    // Returns the length.
    size_t get_len() const
    {
        return len;
    }

    // Returns the type of the current fru area
    ipmi_fru_area_type get_type() const
    {
        return type;
    }

    // Returns the name
    const char* get_name() const
    {
        return name.c_str();
    }

    // Returns SD bus name
    const char* get_bus_name() const
    {
        return bus_name.c_str();
    }

    // Retrns SD bus object path
    const char* get_obj_path() const
    {
        return obj_path.c_str();
    }

    // Returns SD bus interface name
    const char* get_intf_name() const
    {
        return intf_name.c_str();
    }

    // Returns the data portion
    inline uint8_t* get_data() const
    {
        return data;
    }

    // Accepts a pointer to data and sets it in the object.
    void set_data(const uint8_t*, const size_t);

    // Sets the dbus parameters
    void update_dbus_paths(const char*, const char*, const char*);
};

#endif
