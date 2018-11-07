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

#include <cstdint>
#include <vector>

enum IpmiMultirecordType
{
    POWER_SUPPLY_INFO = 0x00,
    DC_OUTPUT = 0x01,
    DC_LOAD = 0x02,
    // MANAGEMENT_ACCESS = 0x03, // Not supported due to variable length format
    BASE_COMPATIBILITY = 0x04,
    EXTEND_COMPATIBILITY = 0x05,
    ASF_FIXED_SMBUS_DEVICE = 0x06,
    ASF_LEGACY_DEVICE_ALERTS = 0x07,
    ASF_REMOTE_CONTROL = 0x08,
    EXTENDED_DC_OUTPUT = 0x09,
    EXTENDED_DC_LOAD = 0x0A,
    TOTAL_MR_TYPES
};

// maximum type that is currently supported by the parser
#define MAX_PARSE_TYPE DC_OUTPUT

// clang-format off
// A helper macro to access bitfields using mask and shift
#define MR_BITS(r, p) (((r) & MR_##p##_MASK) >> MR_##p##_SHIFT)
// clang-format on

// These are per IPMI Platform Management FRU Information Storage
// Definition v1.0, section 16.1:
#define MR_PARAM_EOLST_MASK 0x80
#define MR_PARAM_EOLST_SHIFT 7
#define MR_PARAM_FORMAT_MASK 0x0F
#define MR_PARAM_FORMAT_SHIFT 0

struct MultirecordHeader
{
    uint8_t recordTypeId;
    uint8_t recordParams;

    inline bool endOfList(void) const
    {
        return MR_BITS(recordParams, PARAM_EOLST);
    }
    inline uint8_t recordFormat(void) const
    {
        return MR_BITS(recordParams, PARAM_FORMAT);
    }

    uint8_t recordLength;
    uint8_t recordChecksum;
    uint8_t headerChecksum;

} __attribute__((packed));

// These are per IPMI Platform Management FRU Information Storage
// Definition v1.0, section 18.1:
#define MR_PSU_POLARITY_MASK 0x10
#define MR_PSU_POLARITY_SHIFT 4
#define MR_PSU_HOTSWAP_MASK 0x08
#define MR_PSU_HOTSWAP_SHIFT 3
#define MR_PSU_AUTOSW_MASK 0x04
#define MR_PSU_AUTOSW_SHIFT 2
#define MR_PSU_CORRECTION_MASK 0x02
#define MR_PSU_CORRECTION_SHIFT 1
#define MR_PSU_PREDFAIL_MASK 0x01
#define MR_PSU_PREDFAIL_SHIFT 0
#define MR_PSU_HOLDUP_MASK 0xF000
#define MR_PSU_HOLDUP_SHIFT 11
#define MR_PSU_CAPACITY_MASK 0x0FFF
#define MR_PSU_CAPACITY_SHIFT 0
#define MR_PSU_VOLTAGE1_MASK 0xF0
#define MR_PSU_VOLTAGE1_SHIFT 4
#define MR_PSU_VOLTAGE2_MASK 0x0F
#define MR_PSU_VOLTAGE2_SHIFT 0

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
        return MR_BITS(binaryFlags, PSU_POLARITY);
    }
    inline bool hotSwap(void) const
    {
        return MR_BITS(binaryFlags, PSU_HOTSWAP);
    }
    inline bool autoswitch(void) const
    {
        return MR_BITS(binaryFlags, PSU_AUTOSW);
    }
    inline bool powerCorrection(void) const
    {
        return MR_BITS(binaryFlags, PSU_CORRECTION);
    }
    inline bool predictiveFail(void) const
    {
        return MR_BITS(binaryFlags, PSU_PREDFAIL);
    }

    uint16_t peakWattage;

    inline uint16_t holdUpTime(void) const
    {
        return MR_BITS(peakWattage, PSU_HOLDUP);
    }
    inline uint16_t peakCapacity(void) const
    {
        return MR_BITS(peakWattage, PSU_CAPACITY);
    }

    uint8_t combinedWattage;

    inline uint8_t voltage1(void) const
    {
        return MR_BITS(combinedWattage, PSU_VOLTAGE1);
    }
    inline uint8_t voltage2(void) const
    {
        return MR_BITS(combinedWattage, PSU_VOLTAGE2);
    }

    uint16_t totalWattage;
    uint8_t tachometerLow;

} __attribute__((packed));

// These are per IPMI Platform Management FRU Information Storage
// Definition v1.0, section 18.2:
#define MR_DCOUT_STANDBY_MASK 0x80
#define MR_DCOUT_STANDBY_SHIFT 7
#define MR_DCOUT_OUTNUMBER_MASK 0x0F
#define MR_DCOUT_OUTNUMBER_SHIFT 0

struct DcOutputInfo
{
    uint8_t outputInformation;

    inline bool standby(void) const
    {
        return MR_BITS(outputInformation, DCOUT_STANDBY);
    }
    inline uint8_t outNumber(void) const
    {
        return MR_BITS(outputInformation, DCOUT_OUTNUMBER);
    }

    int16_t nominalVoltage;     /* 10 mV */
    int16_t maxNegativeVoltage; /* 10 mV */
    int16_t maxPositiveVoltage; /* 10 mV */
    uint16_t rippleAndNoise;    /* mV */
    uint16_t minCurrentDraw;    /* mA */
    uint16_t maxCurrentDraw;    /* mA */
} __attribute__((packed));

union Record
{
    PowerSupplyInfo powerSupplyInfo;
    DcOutputInfo dcOutputInfo;
};

struct TypedRecord
{
    Record data;
    IpmiMultirecordType type;
};

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
                      IPMIMultiInfo* info);
