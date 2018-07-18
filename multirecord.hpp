/**
 * @brief IPMI FRU MultiRecord Area parser
 *
 * This file is part of phosphor-ipmi-fru project.
 *
 * Copyright (c) 2018 YADRO
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author: Alexander Soldatov <a.soldatov@yadro.com>
 */

#pragma once

#include <stdio.h>
#include <stdint.h>
#include <vector>

enum IpmiMultirecordType
{
    POWER_SUPPLY_INFO           = 0x00,
    DC_OUTPUT                   = 0x01,
    DC_LOAD                     = 0x02,
    //MANAGEMENT_ACCESS           = 0x03,   // Not supported due to variable length format
    BASE_COMPATIBILITY          = 0x04,
    EXTEND_COMPATIBILITY        = 0x05,
    ASF_FIXED_SMBUS_DEVICE      = 0x06,
    ASF_LEGACY_DEVICE_ALERTS    = 0x07,
    ASF_REMOTE_CONTROL          = 0x08,
    EXTENDED_DC_OUTPUT          = 0x09,
    EXTENDED_DC_LOAD            = 0x0A
};

// maximum type that is currently supported by the parser
#define MAX_PARSE_TYPE          DC_OUTPUT

struct MultirecordHeader
{
    uint8_t recordTypeId;
    struct
    {
        uint8_t recordFormat    : 4;
        uint8_t reserved        : 3;
        uint8_t endOfList       : 1;
    } recordParams;
    uint8_t recordLength;
    uint8_t recordChecksum;
    uint8_t headerChecksum;
}  __attribute__((packed));

struct PowerSupplyInfo
{
    uint16_t overalCapacity;
    uint16_t peakVa;
    uint8_t inrushCurrent;
    uint8_t inrushInterval;               /* ms */
    int16_t lowInVoltage1;                /* 10 mV */
    int16_t highInVoltage1;               /* 10 mV */
    int16_t lowInVoltage2;                /* 10 mV */
    int16_t highInVoltage2;               /* 10 mV */
    uint8_t lowInFrequency;
    uint8_t highInFrequency;
    uint8_t inDropoutTolerance;           /* ms */
    struct
    {
        uint8_t predictiveFail      : 1;
        uint8_t powerCorrection     : 1;
        uint8_t autoswitch          : 1;
        uint8_t hotSwap             : 1;
        uint8_t pinPolarity         : 1;
        uint8_t reserved            : 3;
    } binaryFlags;
    struct
    {
        uint16_t peakCapacity       : 12;
        uint16_t holdUpTime         : 4;    /* s */
    } peakWattage;
    struct
    {
        uint8_t voltage1            : 4;
        uint8_t voltage2            : 4;
    } combinedWattage;
    uint16_t totalWattage;
    uint8_t tahometerLow;
} __attribute__((packed));

struct DcOutputInfo
{
    struct
    {
        uint8_t outNumber           : 4;
        uint8_t reserved            : 3;
        uint8_t standby             : 1;
    } outputInformation;
    int16_t nominalVoltage;                 /* 10 mV */
    int16_t maxNegativeVoltage;             /* 10 mV */
    int16_t maxPositiveVoltage;             /* 10 mV */
    uint16_t rippleAndNoise;                /* mV */
    uint16_t minCurrentDraw;                /* mA */
    uint16_t maxCurrentDraw;                /* mA */
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

/* Parses data from Multirecord Area from fru_data. Results are stored in info */
void parseMutirecord(const uint8_t* fruData, const size_t dataLen,
                      IPMIMultiInfo* info);