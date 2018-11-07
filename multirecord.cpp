/**
 * IPMI FRU MultiRecord Area parser
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

#include <multirecord.hpp>
#include <phosphor-logging/log.hpp>
#include <sstream>
#include <writefrudata.hpp>

#ifdef IPMI_FRU_PARSER_DEBUG
#include <cinttypes>
#endif

using namespace phosphor::logging;

static uint8_t recordSizes[TOTAL_MR_TYPES] = {
    [POWER_SUPPLY_INFO] = sizeof(PowerSupplyInfo),
    [DC_OUTPUT] = sizeof(DcOutputInfo)};

#ifdef IPMI_FRU_DEBUG
std::string joinHex(const std::string& prefix, const uint8_t* data,
                    const size_t size)
{
    std::stringstream stream;
    stream << prefix;
    for (size_t i = 0; i < size; ++i)
        stream << " " << std::hex << static_cast<int>(data[i]);
    return stream.str();
}
#endif

const uint8_t* parseRecord(const uint8_t* recordData, TypedRecord* record)
{
    uint8_t headerSize = sizeof(MultirecordHeader);
#ifdef IPMI_FRU_DEBUG
    log<level::DEBUG>(
        joinHex("Record header:", recordData, headerSize).c_str());
#endif
    // verify header checksum
    uint8_t checksum = calculateChecksum(recordData, headerSize);

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

    // verify record length
    if (header.recordLength != recordSizes[header.recordTypeId])
    {
        log<level::ERR>("Invalid record length");
        return NULL;
    }
#ifdef IPMI_FRU_DEBUG
    log<level::DEBUG>(
        joinHex("Record data:", recordData, header.recordLength).c_str());
#endif

    // verify checksum
    checksum = calculateChecksum(recordData, header.recordLength) -
               header.recordChecksum;

    if (checksum)
    {
        log<level::ERR>("Invalid record checksum");
        return NULL;
    }

    // copy data
    memcpy(&(record->data), recordData, header.recordLength);

    if (header.endOfList())
    {
        recordData = NULL;
    }
    else
    {
        recordData += header.recordLength;
    }
    return recordData;
}

void parseMultirecord(const uint8_t* fruData, const size_t dataLen,
                      IPMIMultiInfo* info)
{
    const uint8_t* recordData = fruData;
    TypedRecord record;
    while (recordData && static_cast<size_t>(recordData - fruData) <= dataLen)
    {
        recordData = parseRecord(recordData, &record);
        info->push_back(record);
#ifdef IPMI_FRU_PARSER_DEBUG
        switch (record.type)
        {
            case POWER_SUPPLY_INFO:
                // clang-format off
                log<level::DEBUG>(
                    "Record PowerSupplyInfo",
                    entry("CAPACITY=" PRIu16,
                          record.data.powerSupplyInfo.overallCapacity),
                    entry("PEAK_VA=" PRIu16,
                          record.data.powerSupplyInfo.peakVa),
                    entry("INRUSH_CURRENT=" PRIu8,
                          record.data.powerSupplyInfo.inrushCurrent),
                    entry("INRUSH_INTERVAL=" PRIu8,
                          record.data.powerSupplyInfo.inrushInterval),
                    entry("LOW_IN_VOLTAGE1=" PRIi16,
                          record.data.powerSupplyInfo.lowInVoltage1),
                    entry("HIGH_IN_VOLTAGE1=" PRIi16,
                          record.data.powerSupplyInfo.highInVoltage1),
                    entry("LOW_IN_VOLTAGE2=" PRIi16,
                          record.data.powerSupplyInfo.lowInVoltage2),
                    entry("HIGH_IN_VOLTAGE2=" PRIi16,
                          record.data.powerSupplyInfo.highInVoltage2),
                    entry("LOW_IN_FREQUENCY=" PRIu8,
                          record.data.powerSupplyInfo.lowInFrequency),
                    entry("HIGH_IN_FREQUENCY=" PRIu8,
                          record.data.powerSupplyInfo.highInFrequency),
                    entry("IN_DROPOUT_TOLERANCE=" PRIu8,
                          record.data.powerSupplyInfo.inDropoutTolerance),
                    entry("PIN_POLARITY=%d",
                          record.data.powerSupplyInfo.pinPolarity()),
                    entry("HOT_SWAP=%d",
                          record.data.powerSupplyInfo.hotSwap()),
                    entry("AUTOSWITCH=%d",
                          record.data.powerSupplyInfo.autoswitch()),
                    entry("POWER_CORRECTION=%d",
                          record.data.powerSupplyInfo.powerCorrection()),
                    entry("PREDICTIVE_FAIL=%d",
                          record.data.powerSupplyInfo.predictiveFail()),
                    entry("HOLD_UP_TIME=" PRIu16,
                          record.data.powerSupplyInfo.holdUpTime()),
                    entry("VOLTAGE1=" PRIu8,
                          record.data.powerSupplyInfo.voltage1())),
                    entry("VOLTAGE2=" PRIu8,
                          record.data.powerSupplyInfo.voltage2()),
                    entry("TOTAL_WATTAGE=" PRIu16,
                          record.data.powerSupplyInfo.totalWattage),
                    entry("TACHOMETER_LOW=" PRIu8,
                          record.data.powerSupplyInfo.tachometerLow);
                // clang-format on
                break;
            case DC_OUTPUT:
                log<level::DEBUG>(
                    "Record DcOutputInfo",
                    entry("NOMINAL_VOLTAGE=" PRIi16,
                          record.data.dcOutputInfo.nominalVoltage),
                    entry("MAX_NEGATIVE_VOLTAGE=" PRIi16,
                          record.data.dcOutputInfo.maxNegativeVoltage),
                    entry("MAX_POSITIVE_VOLTAGE=" PRIi16,
                          record.data.dcOutputInfo.maxPositiveVoltage),
                    entry("RIPPLE_AND_NOISE=" PRIu16,
                          record.data.dcOutputInfo.rippleAndNoise),
                    entry("MIN_CURRENT_DRAW=" PRIu16,
                          record.data.dcOutputInfo.minCurrentDraw),
                    entry("MAX_CURRENT_DRAW=" PRIu16,
                          record.data.dcOutputInfo.maxCurrentDraw),
                    entry("OUT_NUMBER=" PRIu8,
                          record.data.dcOutputInfo.outNumber()),
                    entry("STANDBY=%d",
                          record.data.dcOutputInfo.standby());
                break;
            default:
                log<level::DEBUG>("unsupported record");
                break;
        }
#endif
    }
}
