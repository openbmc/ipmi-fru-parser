#ifndef __IPMI_FRU_AREA_H__
#define __IPMI_FRU_AREA_H__

#include "frup.hpp"
#include "writefrudata.hpp"

#include <stddef.h>
#include <stdint.h>
#include <systemd/sd-bus.h>

#include <memory>
#include <string>
#include <vector>

class ipmi_fru;
typedef std::vector<std::unique_ptr<ipmi_fru>> fru_area_vec_t;

class ipmi_fru
{
  private:
    // Unique way of identifying a FRU
    uint8_t iv_fruid = 0;

    // Type of the fru matching offsets in common header
    ipmi_fru_area_type iv_type = IPMI_FRU_AREA_INTERNAL_USE;

    // Name of the fru area. ( BOARD/CHASSIS/PRODUCT )
    std::string iv_name;

    // Length of a specific fru area.
    size_t iv_len = 0;

    // Special bit for BMC readable eeprom only.
    bool iv_bmc_fru = false;

    // If a FRU is physically present.
    bool iv_present = false;

    // Whether a particular area is valid ?
    bool iv_valid = false;

    // Actual area data.
    uint8_t* iv_data = nullptr;

    // fru inventory dbus name
    std::string iv_bus_name;

    // fru inventory dbus object path
    std::string iv_obj_path;

    // fru inventory dbus interface name
    std::string iv_intf_name;

    // Default constructor disabled.
    ipmi_fru();

  public:
    // constructor
    ipmi_fru(const uint8_t fruid, const ipmi_fru_area_type type,
             bool bmc_fru = false);

    // Destructor
    virtual ~ipmi_fru();

    // If a particular area has been marked valid / invalid
    inline bool is_valid() const
    {
        return iv_valid;
    }

    // Sets the present bit
    inline void set_present(const bool present)
    {
        iv_present = present;
    }

    // Sets the valid bit for a corresponding area.
    inline void set_valid(const bool valid)
    {
        iv_valid = valid;
    }

    // If a particular area accessible only by BMC
    inline bool is_bmc_fru() const
    {
        return iv_bmc_fru;
    }

    // returns fru id;
    uint8_t get_fruid() const
    {
        return iv_fruid;
    }

    // Returns the length.
    size_t get_len() const
    {
        return iv_len;
    }

    // Returns the type of the current fru area
    ipmi_fru_area_type get_type() const
    {
        return iv_type;
    }

    // Returns the name
    const char* get_name() const
    {
        return iv_name.c_str();
    }

    // Returns SD bus name
    const char* get_bus_name() const
    {
        return iv_bus_name.c_str();
    }

    // Retrns SD bus object path
    const char* get_obj_path() const
    {
        return iv_obj_path.c_str();
    }

    // Returns SD bus interface name
    const char* get_intf_name() const
    {
        return iv_intf_name.c_str();
    }

    // Returns the data portion
    inline uint8_t* get_data() const
    {
        return iv_data;
    }

    // Accepts a pointer to data and sets it in the object.
    void set_data(const uint8_t*, const size_t);

    // Sets the dbus parameters
    void update_dbus_paths(const char*, const char*, const char*);
};

#endif
