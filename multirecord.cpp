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

#include <cstring>
#include <multirecord.hpp>
#include <phosphor-logging/log.hpp>
#include <sstream>
#include <writefrudata.hpp>

using namespace phosphor::logging;

static uint8_t record_sizes[] =
{
    sizeof(PowerSupplyInfo),
    sizeof(DcOutputInfo)
};

std::string joinHex(const std::string& prefix, const uint8_t* data, const size_t size)
{
    std::stringstream stream;
    stream << prefix;
    for (size_t i = 0; i < size; ++i)
        stream << " " << std::hex << static_cast<int>(data[i]);
    return stream.str();
}

const uint8_t* parseRecord(const uint8_t* recordData, TypedRecord* record)
{
    uint8_t headerSize = sizeof(MultirecordHeader);
#ifdef IPMI_FRU_DEBUG
    log<level::DEBUG>(joinHex("Record header:", recordData, headerSize).c_str());
#endif
    // verify header checksum
    uint8_t checksum = calculate_crc(recordData, headerSize);

    if (checksum)
    {
        log<level::ERR>("Invalid header checksum");
        return NULL;
    }

    // parse header
    MultirecordHeader header;
    memcpy(&header, recordData, headerSize);
    // set type
    record->type = static_cast<IpmiMultirecordType>(header.recordTypeId);

    if (record->type > MAX_PARSE_TYPE)
    {
        log<level::ERR>("Unsupported record type");
        return NULL;
    }

    recordData += headerSize;

    // verify record len
    if (header.recordLength != record_sizes[header.recordTypeId])
    {
        log<level::ERR>("Invalid record length");
        return NULL;
    }
#ifdef IPMI_FRU_DEBUG
    log<level::DEBUG>(joinHex("Record data:", recordData, header.recordLength).c_str());
#endif
    // verify checksum
    checksum = calculate_crc(recordData,
                             header.recordLength) - header.recordChecksum;

    if (checksum)
    {
        log<level::ERR>("Invalid record checksum");
        return NULL;
    }

    // copy data
    memcpy(&(record->data), recordData, header.recordLength);

    if (header.recordParams.endOfList)
    {
        recordData = NULL;
    }
    else
    {
        recordData += header.recordLength;
    }
    return recordData;
}

void parseMutirecord(const uint8_t* fruData, const size_t dataLen,
                      IPMIMultiInfo* info)
{
    const uint8_t* recordData = fruData;
    TypedRecord record;
    while (recordData && static_cast<size_t>(recordData - fruData) <= dataLen)
    {
        recordData = parseRecord(recordData, &record);
        info->push_back(record);
#ifdef IPMI_FRU_DEBUG
        switch (record.type)
        {
            case POWER_SUPPLY_INFO:
                log<level::DEBUG>("Record PowerSupplyInfo",
                    entry("CAPACITY=%d", 
                        static_cast<int>(record.data.powerSupplyInfo.overalCapacity),
                    "PEAK_VA=%d", 
                        static_cast<int>(record.data.powerSupplyInfo.peakVa),
                    "INRUSH_CURRENT=%d", 
                        static_cast<int>(record.data.powerSupplyInfo.inrushCurrent),
                    "INRUSH_INTERVAL=%d", 
                        static_cast<int>(record.data.powerSupplyInfo.inrushInterval),
                    "LOW_IN_VOLTAGE1=%d", 
                        static_cast<int>(record.data.powerSupplyInfo.lowInVoltage1),
                    "HIGH_IN_VOLTAGE1=%d", 
                        static_cast<int>(record.data.powerSupplyInfo.highInVoltage1),
                    "LOW_IN_VOLTAGE2=%d", 
                        static_cast<int>(record.data.powerSupplyInfo.lowInVoltage2),
                    "HIGH_IN_VOLTAGE2=%d", 
                        static_cast<int>(record.data.powerSupplyInfo.highInVoltage2),
                    "LOW_IN_FREQUENCY=%d", 
                        static_cast<int>(record.data.powerSupplyInfo.lowInFrequency),
                    "HIGH_IN_FREQUENCY=%d", 
                        static_cast<int>(record.data.powerSupplyInfo.highInFrequency),
                    "IN_DROPOUT_TOLERANCE=%d", 
                        static_cast<int>(record.data.powerSupplyInfo.inDropoutTolerance),
                    "PIN_POLARITY=%d", 
                        static_cast<int>(record.data.powerSupplyInfo.binaryFlags.pinPolarity),
                    "HOT_SWAP=%d", 
                        static_cast<int>(record.data.powerSupplyInfo.binaryFlags.hotSwap),
                    "AUTOSWITCH=%d", 
                        static_cast<int>(record.data.powerSupplyInfo.binaryFlags.autoswitch),
                    "POWER_CORRECTION=%d", 
                        static_cast<int>(record.data.powerSupplyInfo.binaryFlags.powerCorrection),
                    "PREDICTIVE_FAIL=%d", 
                        static_cast<int>(record.data.powerSupplyInfo.binaryFlags.predictiveFail),
                    "HOLD_UP_TIME=%d", 
                        static_cast<int>(record.data.powerSupplyInfo.peakWattage.holdUpTime),
                    "VOLTAGE1=%d", 
                        static_cast<int>(record.data.powerSupplyInfo.combinedWattage.voltage1),
                    "VOLTAGE2=%d", 
                        static_cast<int>(record.data.powerSupplyInfo.combinedWattage.voltage2),
                    "TOTAL_WATTAGE=%d", 
                        static_cast<int>(record.data.powerSupplyInfo.totalWattage),
                    "TAHOMETER_LOW=%d", 
                        static_cast<int>(record.data.powerSupplyInfo.tahometerLow)));
                break;
            case DC_OUTPUT:
                log<level::DEBUG>("Record PowerSupplyInfo",
                    entry(
                    "NOMINAL_VOLTAGE=%d", 
                        static_cast<int>(record.data.dcOutputInfo.nominalVoltage),
                    "MAX_NEGATIVE_VOLTAGE=%d",
                        static_cast<int>(record.data.dcOutputInfo.maxNegativeVoltage),
                    "MAX_POSITIVE_VOLTAGE=%d",
                        static_cast<int>(record.data.dcOutputInfo.maxPositiveVoltage),
                    "RIPPLE_AND_NOISE=%d", 
                        static_cast<int>(record.data.dcOutputInfo.rippleAndNoise),
                    "MIN_CURRENT_DRAW=%d", 
                        static_cast<int>(record.data.dcOutputInfo.minCurrentDraw),
                    "MAX_CURRENT_DRAW=%d", 
                        static_cast<int>(record.data.dcOutputInfo.maxCurrentDraw),
                    "OUT_NUMBER=%d",
                        static_cast<int>(record.data.dcOutputInfo.outputInformation.outNumber),
                    "STANDBY=%d", 
                        static_cast<int>(record.data.dcOutputInfo.outputInformation.standby)));
                break;
            default:
                log<level::DEBUG>("Unsupported record");
                break;
        }
#endif
    }
}