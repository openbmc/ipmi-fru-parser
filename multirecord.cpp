#include <multirecord.hpp>
#include <phosphor-logging/log.hpp>
#include <sstream>
#include <writefrudata.hpp>

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
    uint8_t checksum = calculateCRC(recordData, headerSize);

    if (checksum)
    {
        log<level::ERR>("Invalid header checksum");
        return NULL;
    }

    // parse header
    MultirecordHeader header;
    memcpy(&header, recordData, headerSize);

    // set type
    record->type = static_cast<ImpiMultirecordType>(header.recordTypeId);

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
    checksum =
        calculateCRC(recordData, header.recordLength) - header.recordChecksum;

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
                log<level::DEBUG>(
                    "Record PowerSupplyInfo",
                    entry("CAPACITY=%d",
                          static_cast<int>(
                              record.data.powerSupplyInfo.overallCapacity)),
                    entry("PEAK_VA=%d",
                          static_cast<int>(record.data.powerSupplyInfo.peakVa)),
                    entry("INRUSH_CURRENT=%d",
                          static_cast<int>(
                              record.data.powerSupplyInfo.inrushCurrent)),
                    entry("INRUSH_INTERVAL=%d",
                          static_cast<int>(
                              record.data.powerSupplyInfo.inrushInterval)),
                    entry("LOW_IN_VOLTAGE1=%d",
                          static_cast<int>(
                              record.data.powerSupplyInfo.lowInVoltage1)),
                    entry("HIGH_IN_VOLTAGE1=%d",
                          static_cast<int>(
                              record.data.powerSupplyInfo.highInVoltage1)),
                    entry("LOW_IN_VOLTAGE2=%d",
                          static_cast<int>(
                              record.data.powerSupplyInfo.lowInVoltage2)),
                    entry("HIGH_IN_VOLTAGE2=%d",
                          static_cast<int>(
                              record.data.powerSupplyInfo.highInVoltage2)),
                    entry("LOW_IN_FREQUENCY=%d",
                          static_cast<int>(
                              record.data.powerSupplyInfo.lowInFrequency)),
                    entry("HIGH_IN_FREQUENCY=%d",
                          static_cast<int>(
                              record.data.powerSupplyInfo.highInFrequency)),
                    entry("IN_DROPOUT_TOLERANCE=%d",
                          static_cast<int>(
                              record.data.powerSupplyInfo.inDropoutTolerance)),
                    entry("PIN_POLARITY=%d",
                          static_cast<int>(
                              record.data.powerSupplyInfo.pinPolarity())),
                    entry("HOT_SWAP=%d",
                          static_cast<int>(
                              record.data.powerSupplyInfo.hotSwap())),
                    entry("AUTOSWITCH=%d",
                          static_cast<int>(
                              record.data.powerSupplyInfo.autoswitch())),
                    entry("POWER_CORRECTION=%d",
                          static_cast<int>(
                              record.data.powerSupplyInfo.powerCorrection())),
                    entry("PREDICTIVE_FAIL=%d",
                          static_cast<int>(
                              record.data.powerSupplyInfo.predictiveFail())),
                    entry("HOLD_UP_TIME=%d",
                          static_cast<int>(
                              record.data.powerSupplyInfo.holdUpTime())),
                    entry("VOLTAGE1=%d",
                          static_cast<int>(
                              record.data.powerSupplyInfo.voltage1())),
                    entry("VOLTAGE2=%d",
                          static_cast<int>(
                              record.data.powerSupplyInfo.voltage2())),
                    entry("TOTAL_WATTAGE=%d",
                          static_cast<int>(
                              record.data.powerSupplyInfo.totalWattage)),
                    entry("TACHOMETER_LOW=%d",
                          static_cast<int>(
                              record.data.powerSupplyInfo.tachometerLow)));
                break;
            case DC_OUTPUT:
                log<level::DEBUG>(
                    "Record DcOutputInfo",
                    entry("NOMINAL_VOLTAGE=%d",
                          static_cast<int>(
                              record.data.dcOutputInfo.nominalVoltage)),
                    entry("MAX_NEGATIVE_VOLTAGE=%d",
                          static_cast<int>(
                              record.data.dcOutputInfo.maxNegativeVoltage)),
                    entry("MAX_POSITIVE_VOLTAGE=%d",
                          static_cast<int>(
                              record.data.dcOutputInfo.maxPositiveVoltage)),
                    entry("RIPPLE_AND_NOISE=%d",
                          static_cast<int>(
                              record.data.dcOutputInfo.rippleAndNoise)),
                    entry("MIN_CURRENT_DRAW=%d",
                          static_cast<int>(
                              record.data.dcOutputInfo.minCurrentDraw)),
                    entry("MAX_CURRENT_DRAW=%d",
                          static_cast<int>(
                              record.data.dcOutputInfo.maxCurrentDraw)),
                    entry(
                        "OUT_NUMBER=%d",
                        static_cast<int>(record.data.dcOutputInfo.outNumber())),
                    entry(
                        "STANDBY=%d",
                        static_cast<int>(record.data.dcOutputInfo.standby())));
                break;
            default:
                log<level::DEBUG>("unsupported record");
                break;
        }
#endif
    }
}
