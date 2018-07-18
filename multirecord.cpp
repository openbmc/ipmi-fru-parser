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

#include "multirecord.hpp"
#include "writefrudata.hpp"
#include <string.h>

static uint8_t record_sizes[] =
{
    sizeof(power_supply_info_t),
    sizeof(dc_output_info_t)
};

uint8_t* parse_record(uint8_t* record_data, typed_record_t* record)
{
    uint8_t header_size = sizeof(multirecord_header_t);
    TRACE_LOCAL("Header: ");
    for (uint16_t i = 0; i < header_size; ++i)
    {
        TRACE_LOCAL("%02x ", record_data[i]);
    }
    TRACE_LOCAL("\n");

    // verify header checksum
    uint8_t checksum = calculate_crc(record_data, header_size);

    if (checksum)
    {
        TRACE_LOCAL("Invalid header checksum\n");
        return NULL;
    }

    // parse header
    multirecord_header_t header;
    memcpy(&header, record_data, header_size);
    TRACE_LOCAL("end of list: %d\n", header.record_params.end_of_list);
    // set type
    record->type = (impi_multirecord_type_t)header.record_type_id;

    if (record->type > MAX_PARSE_TYPE)
    {
        TRACE_LOCAL("Unsupported record type\n");
        return NULL;
    }

    record_data += header_size;

    // verify record len
    if (header.record_length != record_sizes[header.record_type_id])
    {
        TRACE_LOCAL("Invalid record lenght\n");
        return NULL;
    }

    TRACE_LOCAL("Record: ");
    for (uint16_t i = 0; i < header.record_length; ++i)
    {
        TRACE_LOCAL("%02x ", record_data[i]);
    }
    TRACE_LOCAL("\n");

    // verify checksum
    checksum = calculate_crc(record_data,
                             header.record_length) - header.record_checksum;

    if (checksum)
    {
        TRACE_LOCAL("Invalid record checksum\n");
        return NULL;
    }

    // copy data
    memcpy(&(record->data), record_data, header.record_length);

    if (header.record_params.end_of_list)
    {
        TRACE_LOCAL("Last record\n");
        record_data = NULL;
    }
    else
    {
        record_data += header.record_length;
    }
    return record_data;
}

void parse_mutirecord(uint8_t* fru_data, const size_t data_len,
                      IPMIMultiInfo* info)
{
    uint8_t* record_data = fru_data;
    typed_record_t record;
    while (record_data && (size_t)(record_data - fru_data) <= data_len)
    {
        record_data = parse_record(record_data, &record);
        info->push_back(record);
#ifdef IPMI_FRU_PARSER_DEBUG
        printf("record type: %d\n", record.type);
        switch (record.type)
        {
            case POWER_SUPPLY_INFO:
                printf("power supply info\n");
                printf("capacity: %d\n", record.data.power_supply_info.overal_capacity);
                printf("peak va: %d\n", record.data.power_supply_info.peak_va);
                printf("inrush current: %d\n", record.data.power_supply_info.inrush_current);
                printf("inrush interval: %d\n", record.data.power_supply_info.inrush_interval);
                printf("low in voltage1: %d\n", record.data.power_supply_info.low_in_voltage1);
                printf("high in voltage1: %d\n",
                       record.data.power_supply_info.high_in_voltage1);
                printf("low in voltage2: %d\n", record.data.power_supply_info.low_in_voltage2);
                printf("high in voltage2: %d\n",
                       record.data.power_supply_info.high_in_voltage2);
                printf("low in frequency: %d\n",
                       record.data.power_supply_info.low_in_frequency);
                printf("high in frequency: %d\n",
                       record.data.power_supply_info.high_in_frequency);
                printf("in dropout tolerance: %d\n",
                       record.data.power_supply_info.in_dropout_tolerance);
                printf("pin polarity: %d\n",
                       record.data.power_supply_info.binary_flags.pin_polarity);
                printf("hot swap: %d\n", record.data.power_supply_info.binary_flags.hot_swap);
                printf("autoswitch: %d\n",
                       record.data.power_supply_info.binary_flags.autoswitch);
                printf("power correction: %d\n",
                       record.data.power_supply_info.binary_flags.power_correction);
                printf("predictive fail: %d\n",
                       record.data.power_supply_info.binary_flags.predictive_fail);
                printf("hold up time: %d\n",
                       record.data.power_supply_info.peak_wattage.hold_up_time);
                printf("voltage1: %d\n",
                       record.data.power_supply_info.combined_wattage.voltage1);
                printf("voltage2: %d\n",
                       record.data.power_supply_info.combined_wattage.voltage2);
                printf("total wattage: %d\n", record.data.power_supply_info.total_wattage);
                printf("tahometer low: %d\n", record.data.power_supply_info.tahometer_low);
                printf("\n");
                break;
            case DC_OUTPUT:
                printf("dc output info\n");
                printf("nominal voltage: %d\n", record.data.dc_output_info.nominal_voltage);
                printf("max negative voltage: %d\n",
                       record.data.dc_output_info.max_negative_voltage);
                printf("max positive voltage: %d\n",
                       record.data.dc_output_info.max_positive_voltage);
                printf("ripple and noise: %d\n", record.data.dc_output_info.ripple_and_noise);
                printf("min current draw: %d\n", record.data.dc_output_info.min_current_draw);
                printf("max current draw: %d\n", record.data.dc_output_info.max_current_draw);
                printf("out number: %d\n",
                       record.data.dc_output_info.output_information.out_number);
                printf("standby: %d\n", record.data.dc_output_info.output_information.standby);
                printf("\n");
                break;
            default:
                printf("unsupported record\n");
                break;
        }
#endif
    }
}