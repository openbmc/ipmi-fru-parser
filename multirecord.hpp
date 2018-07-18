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

#ifdef IPMI_FRU_PARSER_DEBUG
#define TRACE_LOCAL(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define TRACE_LOCAL(fmt, ...)
#endif

enum impi_multirecord_type_t
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

typedef struct multirecord_header
{
    uint8_t record_type_id;
    struct
    {
        uint8_t record_format   : 4;
        uint8_t reserved        : 3;
        uint8_t end_of_list     : 1;
    } record_params;
    uint8_t record_length;
    uint8_t record_checksum;
    uint8_t header_checksum;
}  __attribute__((packed)) multirecord_header_t;

typedef struct power_supply_info
{
    uint16_t overal_capacity;
    uint16_t peak_va;
    uint8_t inrush_current;
    uint8_t inrush_interval;                /* ms */
    int16_t low_in_voltage1;                /* 10 mV */
    int16_t high_in_voltage1;               /* 10 mV */
    int16_t low_in_voltage2;                /* 10 mV */
    int16_t high_in_voltage2;               /* 10 mV */
    uint8_t low_in_frequency;
    uint8_t high_in_frequency;
    uint8_t in_dropout_tolerance;           /* ms */
    struct
    {
        uint8_t predictive_fail     : 1;
        uint8_t power_correction    : 1;
        uint8_t autoswitch          : 1;
        uint8_t hot_swap            : 1;
        uint8_t pin_polarity        : 1;
        uint8_t reserved            : 3;
    } binary_flags;
    struct
    {
        uint16_t peak_capacity      : 12;
        uint16_t hold_up_time       : 4;    /* s */
    } peak_wattage;
    struct
    {
        uint8_t voltage1            : 4;
        uint8_t voltage2            : 4;
    } combined_wattage;
    uint16_t total_wattage;
    uint8_t tahometer_low;
} __attribute__((packed)) power_supply_info_t;

typedef struct dc_output_info
{
    struct
    {
        uint8_t out_number          : 4;
        uint8_t reserved            : 3;
        uint8_t standby             : 1;
    } output_information;
    int16_t nominal_voltage;                /* 10 mV */
    int16_t max_negative_voltage;           /* 10 mV */
    int16_t max_positive_voltage;           /* 10 mV */
    uint16_t ripple_and_noise;              /* mV */
    uint16_t min_current_draw;              /* mA */
    uint16_t max_current_draw;              /* mA */
} __attribute__((packed)) dc_output_info_t;

typedef union
{
    power_supply_info_t power_supply_info;
    dc_output_info_t dc_output_info;
} record_t;

typedef struct
{
    record_t data;
    impi_multirecord_type_t type;
} typed_record_t;

using IPMIMultiInfo = std::vector<typed_record_t>;

/* Parses data from Multirecord Area from fru_data. Results are stored in info */
void parse_mutirecord(uint8_t* fru_data, const size_t data_len,
                      IPMIMultiInfo* info);