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
  public:
    IPMIFruArea() = delete;

    // constructor
    IPMIFruArea(const uint8_t fruID, const ipmi_fru_area_type type,
                bool bmcOnlyFru = false);

    // Destructor
    virtual ~IPMIFruArea();

    // Sets the present bit
    inline void setPresent(const bool present)
    {
        isPresent = present;
    }

    // returns fru id;
    uint8_t getFruID() const
    {
        return fruID;
    }

    // Returns the length.
    size_t getLength() const
    {
        return data.size();
    }

    // Returns the type of the current fru area
    ipmi_fru_area_type getType() const
    {
        return type;
    }

    // Returns the name
    const char* getName() const
    {
        return name.c_str();
    }

    // Returns SD bus name
    const char* getBusName() const
    {
        return busName.c_str();
    }

    // Retrns SD bus object path
    const char* getObjectPath() const
    {
        return objectPath.c_str();
    }

    // Returns SD bus interface name
    const char* getInterfaceName() const
    {
        return interfaceName.c_str();
    }

    // Returns the data portion
    inline const uint8_t* getData() const
    {
        return data.data();
    }

    // Accepts a pointer to data and sets it in the object.
    void setData(const uint8_t*, const size_t);

    // Sets the dbus parameters
    void updateDbusPaths(const char*, const char*, const char*);

  private:
    // Unique way of identifying a FRU
    uint8_t fruID = 0;

    // Type of the fru matching offsets in common header
    ipmi_fru_area_type type = IPMI_FRU_AREA_INTERNAL_USE;

    // Name of the fru area. ( BOARD/CHASSIS/PRODUCT )
    std::string name;

    // Special bit for BMC readable eeprom only.
    bool bmcOnlyFru = false;

    // If a FRU is physically present.
    bool isPresent = false;

    // Whether a particular area is valid ?
    bool isValid = false;

    // Actual area data.
    std::vector<uint8_t> data;

    // fru inventory dbus name
    std::string busName;

    // fru inventory dbus object path
    std::string objectPath;

    // fru inventory dbus interface name
    std::string interfaceName;
};

#endif
