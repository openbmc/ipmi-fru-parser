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

enum class IpmiMultirecordType
{
    powerSupplyInfo = 0x00,
    dcOutput = 0x01,
    dcLoad = 0x02,
    // managementAccess = 0x03, // Not supported due to variable length format
    baseCompatibility = 0x04,
    extendCompatibility = 0x05,
    asfFixedSmbusDevice = 0x06,
    asdLegacyDeviceAlerts = 0x07,
    asfRemoteControl = 0x08,
    extendedDcOutput = 0x09,
    extendedDcLoad = 0x0A,
    totalMrTypes
};

// maximum type that is currently supported by the parser
constexpr auto MAX_PARSE_TYPE = IpmiMultirecordType::dcOutput;
// clang-format off
// These are per IPMI Platform Management FRU Information Storage
enum class IpmiMultirecordParam
{
// Definition v1.0, section 16.1:
    paramEolst,
    paramFormat,
// Definition v1.0, section 18.1:
    psuPolarity,
    psuHotswap,
    psuAutosw,
    psuCorrection,
    psuPredfail,
    psuHoldup,
    psuCapacity,
    psuVoltage1,
    psuVoltage2,
// Definition v1.0, section 18.2:
    dcOutStandby,
    dcOutOutnumber,
};

// These are per IPMI Platform Management FRU Information Storage
static const uint16_t mask[] =
{
// Definition v1.0, section 16.1:
    0x80,                   // paramEolst
    0x0F,                   // paramFormat
// Definition v1.0, section 18.1:
    0x10,                   // psuPolarity
    0x08,                   // psuHotswap
    0x04,                   // psuAutosw
    0x02,                   // psuCorrection
    0x01,                   // psuPredfail
    0xF000,                 // psuHoldup
    0x0FFF,                 // psuCapacity
    0xF0,                   // psuVoltage1
    0x0F,                   // psuVoltage2
// Definition v1.0, section 18.2:
    0x80,                   // dcOutStandby
    0x0F,                   // dcOutOutnumber
};

// These are per IPMI Platform Management FRU Information Storage
static const uint8_t shift[] =
{
// Definition v1.0, section 16.1:
    7,                      // paramEolst
    0,                      // paramFormat
// Definition v1.0, section 18.1:
    4,                      // psuPolarity
    3,                      // psuHotswap
    2,                      // psuAutosw
    1,                      // psuCorrection
    0,                      // psuPredfail
    11,                     // psuHoldup
    0,                      // psuCapacity
    4,                      // psuVoltage1
    0,                      // psuVoltage2
// Definition v1.0, section 18.2:
    7,                      // dcOutStandby
    0,                      // dcOutOutnumber
};
// clang-format on
// A helper function to access bitfields using mask and shift
inline uint16_t mrBits(const uint16_t r, const IpmiMultirecordParam& p)
{
    uint8_t index = static_cast<int>(p);
    return (r & mask[index]) >> shift[index];
}

// These are per IPMI Platform Management FRU Information Storage
// Definition v1.0, section 16.1:
struct MultirecordHeader
{
    uint8_t recordTypeId;
    uint8_t recordParams;

    inline bool endOfList(void) const
    {
        return mrBits(recordParams, IpmiMultirecordParam::paramEolst);
    }
    inline uint8_t recordFormat(void) const
    {
        return mrBits(recordParams, IpmiMultirecordParam::paramFormat);
    }

    uint8_t recordLength;
    uint8_t recordChecksum;
    uint8_t headerChecksum;

} __attribute__((packed));

// These are per IPMI Platform Management FRU Information Storage
// Definition v1.0, section 18.1:
struct PowerSupplyInfo
{
    uint16_t overallCapacity;
    uint16_t peakVa;
    uint8_t inrushCurrent;
    uint8_t inrushInterval; /* ms */
    int16_t lowInVoltage1;  /* 10 mV */
    int16_t highInVoltage1; /* 10 mV */
    int16_t lowInVoltage2;  /* 10 mV */
    int16_t highInVoltage2; /* 10 mV */
    uint8_t lowInFrequency;
    uint8_t highInFrequency;
    uint8_t inDropoutTolerance; /* ms */
    uint8_t binaryFlags;

    inline bool pinPolarity(void) const
    {
        return mrBits(binaryFlags, IpmiMultirecordParam::psuPolarity);
    }
    inline bool hotSwap(void) const
    {
        return mrBits(binaryFlags, IpmiMultirecordParam::psuHotswap);
    }
    inline bool autoswitch(void) const
    {
        return mrBits(binaryFlags, IpmiMultirecordParam::psuAutosw);
    }
    inline bool powerCorrection(void) const
    {
        return mrBits(binaryFlags, IpmiMultirecordParam::psuCorrection);
    }
    inline bool predictiveFail(void) const
    {
        return mrBits(binaryFlags, IpmiMultirecordParam::psuPredfail);
    }

    uint16_t peakWattage;

    inline uint16_t holdUpTime(void) const
    {
        return mrBits(peakWattage, IpmiMultirecordParam::psuHoldup);
    }
    inline uint16_t peakCapacity(void) const
    {
        return mrBits(peakWattage, IpmiMultirecordParam::psuCapacity);
    }

    uint8_t combinedWattage;

    inline uint8_t voltage1(void) const
    {
        return mrBits(combinedWattage, IpmiMultirecordParam::psuVoltage1);
    }
    inline uint8_t voltage2(void) const
    {
        return mrBits(combinedWattage, IpmiMultirecordParam::psuVoltage2);
    }

    uint16_t totalWattage;
    uint8_t tachometerLow;

} __attribute__((packed));

// These are per IPMI Platform Management FRU Information Storage
// Definition v1.0, section 18.2:
struct DcOutputInfo
{
    uint8_t outputInformation;

    inline bool standby(void) const
    {
        return mrBits(outputInformation, IpmiMultirecordParam::dcOutStandby);
    }
    inline uint8_t outNumber(void) const
    {
        return mrBits(outputInformation, IpmiMultirecordParam::dcOutOutnumber);
    }

    int16_t nominalVoltage;     /* 10 mV */
    int16_t maxNegativeVoltage; /* 10 mV */
    int16_t maxPositiveVoltage; /* 10 mV */
    uint16_t rippleAndNoise;    /* mV */
    uint16_t minCurrentDraw;    /* mA */
    uint16_t maxCurrentDraw;    /* mA */
} __attribute__((packed));

class MultirecordInfo
{
  public:
    virtual void updatePropertyMap(ipmi::vpd::PropertyMap& props) const = 0;
    virtual IpmiMultirecordType type() const = 0;
    virtual ~MultirecordInfo() = default;
};

class PowerSupplyClass : public MultirecordInfo
{
  public:
    PowerSupplyClass(const uint8_t* data)
    {
        std::memcpy(&(data_), data, sizeof(data_));
    }
    void updatePropertyMap(ipmi::vpd::PropertyMap& props) const
    {
        props.emplace("OverallCapacity",
                      static_cast<int64_t>(data_.overallCapacity));
        props.emplace("PeakVA", static_cast<int64_t>(data_.peakVa));
        props.emplace("InrushCurrent",
                      static_cast<int64_t>(data_.inrushCurrent));
        props.emplace("InrushInterval",
                      static_cast<int64_t>(data_.inrushInterval));
        props.emplace("LowInputVoltage1",
                      static_cast<int64_t>(data_.lowInVoltage1));
        props.emplace("HighInputVoltage1",
                      static_cast<int64_t>(data_.highInVoltage1));
        props.emplace("LowInputVoltage2",
                      static_cast<int64_t>(data_.lowInVoltage2));
        props.emplace("HighInputVoltage2",
                      static_cast<int64_t>(data_.highInVoltage2));
        props.emplace("LowInputFrequency",
                      static_cast<int64_t>(data_.lowInFrequency));
        props.emplace("HighInputFrequency",
                      static_cast<int64_t>(data_.highInFrequency));
        props.emplace("InDropoutTolerance",
                      static_cast<int64_t>(data_.inDropoutTolerance));
        props.emplace("HoldUpTime", static_cast<int64_t>(data_.holdUpTime()));
        props.emplace("PeakCapacity",
                      static_cast<int64_t>(data_.peakCapacity()));
        props.emplace("Voltage1", static_cast<int64_t>(data_.voltage1()));
        props.emplace("Voltage2", static_cast<int64_t>(data_.voltage2()));
        props.emplace("TotalWattage", static_cast<int64_t>(data_.totalWattage));
        props.emplace("TachometerLow",
                      static_cast<int64_t>(data_.tachometerLow));
        props.emplace("HotSwap", data_.hotSwap());
        props.emplace("PinPolarity", data_.pinPolarity());
        props.emplace("Autoswitch", data_.autoswitch());
        props.emplace("PowerFactor", data_.powerCorrection());
        props.emplace("PredictiveFail", data_.predictiveFail());
    }
    virtual IpmiMultirecordType type() const
    {
        return IpmiMultirecordType::powerSupplyInfo;
    }

  private:
    PowerSupplyInfo data_;
};

class DcOutputClass : public MultirecordInfo
{
  public:
    DcOutputClass(const uint8_t* data)
    {
        std::memcpy(&(data_), data, sizeof(data_));
    }
    virtual void updatePropertyMap(ipmi::vpd::PropertyMap& props) const
    {
        props.emplace("OutputNumber", static_cast<int64_t>(data_.outNumber()));
        props.emplace("NominalVoltage",
                      static_cast<int64_t>(data_.nominalVoltage));
        props.emplace("MaxNegativeVoltage",
                      static_cast<int64_t>(data_.maxNegativeVoltage));
        props.emplace("MaxPositiveVoltage",
                      static_cast<int64_t>(data_.maxPositiveVoltage));
        props.emplace("RippleAndNoise",
                      static_cast<int64_t>(data_.rippleAndNoise));
        props.emplace("MinCurrentDraw",
                      static_cast<int64_t>(data_.minCurrentDraw));
        props.emplace("MaxCurrentDraw",
                      static_cast<int64_t>(data_.maxCurrentDraw));
        props.emplace("Standby", data_.standby());
    }
    virtual IpmiMultirecordType type() const
    {
        return IpmiMultirecordType::dcOutput;
    }

  private:
    DcOutputInfo data_;
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
