/**
 * IPMI FRU MultiRecord Area parser header file
 *
 * This file is a part of ipmi-fru-parser project
 *
 * Copyright (c) 2018 YADRO
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "types.hpp"

#include <cstdint>
#include <cstring>
#include <memory>
#include <variant>
#include <vector>

/**
 * MultiRecord FRU Area record types.
 *
 * These are per IPMI Platform Management FRU Information Storage
 * Definition v1.0 revision 1.3, Table 16-2
 */
enum class IpmiMultirecordType : uint8_t
{
    powerSupplyInfo = 0x00,
    dcOutput = 0x01,
    dcLoad = 0x02,
    managementAccess = 0x03, // Not supported due to variable length format
    baseCompatibility = 0x04,
    extendCompatibility = 0x05,
    asfFixedSmbusDevice = 0x06,
    asdLegacyDeviceAlerts = 0x07,
    asfRemoteControl = 0x08,
    extendedDcOutput = 0x09,
    extendedDcLoad = 0x0A,
};

class MultirecordInfo
{
  public:
    virtual ipmi::vpd::PropertyMap updatePropertyMap() const = 0;
    virtual IpmiMultirecordType type() const = 0;
    virtual ~MultirecordInfo() = default;
};

using TypedRecord = std::unique_ptr<MultirecordInfo>;
using IPMIMultiInfo = std::vector<TypedRecord>;

/**
 * Parse Multirecord Area of fru_data. Results are stored in info.
 *
 * @param[in] fruData - a pointer to FRU storage buffer
 * @param[in] dataLen - length of the FRU storage buffer
 * @param[out] info - the output vector of TypedRecord entries containing
 *                    the parsed fields
 */
void parseMultirecord(const uint8_t* fruData, const size_t dataLen,
                      IPMIMultiInfo& info);
