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

#include <cstddef>
#include <ios>
#include <iostream>
#include <multirecord.hpp>
#include <phosphor-logging/log.hpp>
#include <sstream>
#include <writefrudata.hpp>

#ifdef IPMI_FRU_PARSER_DEBUG
#include <cinttypes>
#endif

namespace multirecord
{

using namespace phosphor::logging;

// Maximum type that is currently supported by the parser
constexpr auto maxParsedType = IpmiMultirecordType::dcOutput;

// Enumeration of supported bit-field MultiRecord FRU Area parameters.
//
// These are defined in various tables within IPMI FRU Information Storage
// Definition v1.0 revision 1.3.
//
enum class IpmiMultirecordParam
{
    // clang-format off
    // Table 16-1:
    paramEolst,  // End-of-list marker
    paramFormat, // Record format version
    // Table 18-1:
    psuTachoPPR,                      // Tacho Pulses per Rotation
    psuFailPinPolarity = psuTachoPPR, // Predictive fail pin polarity
    psuHotswap,                       // Hot swap support
    psuAutoswitch,                    // Autoswitch
    psuCorrection,                    // Power Factor Correction
    psuPredFailSupport,               // Predictive Fail Support
    psuHoldup,                        // Peak wattage hold up time
    psuCapacity,                      // Peak capacity
    psuVoltage1,                      // Voltage 1 in combined wattage
    psuVoltage2,                      // Voltage 2 in combined wattage
    // Table 18-2:
    dcOutStandby,   // Output standby flag, Table 18-2
    dcOutOutnumber, // Output number, Table 18-2
                    // clang-format on
};

// Volts to millivolts
constexpr auto V2mV(const double V)
{
    return static_cast<int>(V * 1000);
}

// Indices and values are per IPMI FRU Definition 1.0 rev 1.3, section 18.1.14:
constexpr int voltages[] = {
    V2mV(12.0),  // [0b0000]
    V2mV(-12.0), // [0b0001]
    V2mV(5.0),   // [0b0010]
    V2mV(3.3)    // [0b0011]
};

// A helper function to extract bitfield values using mask and shift
static inline uint16_t mrBits(const uint16_t r, const IpmiMultirecordParam& p)
{
    struct MaskShift
    {
        uint16_t nbits; // Number of bits for parameter p, LSB at pos
        uint8_t pos;    // Position of LSB in a word or byte r
    };

    // Bit sizes and positions for bit-field parameters indexed by p.
    // The masks are taken from corresponding tables in
    // IPMI FRU Information Storage Definition v1.0 revision 1.3
    static const MaskShift ms[] = {
        // Table 16-1:
        {1, 7}, // paramEolst
        {4, 0}, // paramFormat
        // Table 18-1:
        {1, 4},  // psuTachoPPR
        {1, 3},  // psuHotswap
        {1, 2},  // psuAutoswitch
        {1, 1},  // psuCorrection
        {1, 0},  // psuPredFailSupport
        {4, 11}, // psuHoldup
        {12, 0}, // psuCapacity
        {4, 4},  // psuVoltage1
        {4, 0},  // psuVoltage2
        // Table 18-2:
        {1, 7}, // dcOutStandby
        {4, 0}, // dcOutOutnumber
    };

    size_t index = static_cast<size_t>(p);
    uint16_t mask = (1 << ms[index].pos) - 1;
    return (r >> ms[index].pos) & mask;
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

    bool tachoPPR(void) const
    {
        return mrBits(binaryFlags, IpmiMultirecordParam::psuTachoPPR);
    }

    bool failurePinNegated(void) const
    {
        return mrBits(binaryFlags, IpmiMultirecordParam::psuFailPinPolarity);
    }

    bool hotSwap(void) const
    {
        return mrBits(binaryFlags, IpmiMultirecordParam::psuHotswap);
    }

    bool autoSwitch(void) const
    {
        return mrBits(binaryFlags, IpmiMultirecordParam::psuAutoswitch);
    }

    bool powerCorrection(void) const
    {
        return mrBits(binaryFlags, IpmiMultirecordParam::psuCorrection);
    }

    bool predFailSupport(void) const
    {
        return mrBits(binaryFlags, IpmiMultirecordParam::psuPredFailSupport);
    }

    uint16_t peakWattage;

    uint16_t holdUpTime(void) const
    {
        return mrBits(peakWattage, IpmiMultirecordParam::psuHoldup);
    }

    uint16_t peakCapacity(void) const
    {
        return mrBits(peakWattage, IpmiMultirecordParam::psuCapacity);
    }

    uint8_t combinedVoltages; // Contains two voltages encoded as per 18.1.14

    // Arguments for combined_voltage()
    static constexpr size_t voltage1 = 0;
    static constexpr size_t voltage2 = 1;
    int64_t combined_voltage(size_t vidx) const
    {
        // Section 18.1.14:
        // "A value of 0 for the combined voltage field indicates there are
        // no combined voltages for a power supply."
        if (!combinedVoltages && !totalWattage)
            return 0;

        using imp = IpmiMultirecordParam;
        imp param[2] = {imp::psuVoltage1, imp::psuVoltage2};

        if (vidx > voltage2)
            return 0;

        size_t voltageEncoded = mrBits(combinedVoltages, param[vidx]);
        return voltages[voltageEncoded];
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

class PowerSupplyClass : public MultirecordInfo
{
  public:
    PowerSupplyClass(const uint8_t* data)
    {
        std::memcpy(&(pData), data, sizeof(pData));
    }
    void updatePropertyMap(ipmi::vpd::PropertyMap& props) const
    {
        props.emplace("OverallCapacity",
                      static_cast<int64_t>(pData.overallCapacity));
        props.emplace("PeakVA", static_cast<int64_t>(pData.peakVa));
        props.emplace("InrushCurrent",
                      static_cast<int64_t>(pData.inrushCurrent));
        props.emplace("InrushInterval",
                      static_cast<int64_t>(pData.inrushInterval));
        props.emplace("LowInputVoltage1",
                      static_cast<int64_t>(pData.lowInVoltage1));
        props.emplace("HighInputVoltage1",
                      static_cast<int64_t>(pData.highInVoltage1));
        props.emplace("LowInputVoltage2",
                      static_cast<int64_t>(pData.lowInVoltage2));
        props.emplace("HighInputVoltage2",
                      static_cast<int64_t>(pData.highInVoltage2));
        props.emplace("LowInputFrequency",
                      static_cast<int64_t>(pData.lowInFrequency));
        props.emplace("HighInputFrequency",
                      static_cast<int64_t>(pData.highInFrequency));
        props.emplace("InDropoutTolerance",
                      static_cast<int64_t>(pData.inDropoutTolerance));
        props.emplace("HoldUpTime", static_cast<int64_t>(pData.holdUpTime()));
        props.emplace("PeakCapacity",
                      static_cast<int64_t>(pData.peakCapacity()));
        props.emplace("Voltage1", pData.combined_voltage(pData.voltage1));
        props.emplace("Voltage2", pData.combined_voltage(pData.voltage2));
        props.emplace("TotalWattage", static_cast<int64_t>(pData.totalWattage));
        props.emplace("TachometerLow",
                      static_cast<int64_t>(pData.tachometerLow));
        props.emplace("HotSwap", pData.hotSwap());
        if (pData.predFailSupport())
        {
            props.emplace("TachoUses2PulsesPerRotation", pData.tachoPPR());
        }
        else
        {
            props.emplace("PredictiveFailPinNegated",
                          pData.failurePinNegated());
        }
        props.emplace("Autoswitch", pData.autoSwitch());
        props.emplace("PowerFactor", pData.powerCorrection());
        props.emplace("PredictiveFailSupport", pData.predFailSupport());
    }
    virtual IpmiMultirecordType type() const
    {
        return IpmiMultirecordType::powerSupplyInfo;
    }

  private:
    PowerSupplyInfo pData;
};

class DcOutputClass : public MultirecordInfo
{
  public:
    DcOutputClass(const uint8_t* data)
    {
        std::memcpy(&(pData), data, sizeof(pData));
    }
    virtual void updatePropertyMap(ipmi::vpd::PropertyMap& props) const
    {
        props.emplace("OutputNumber", static_cast<int64_t>(pData.outNumber()));
        props.emplace("NominalVoltage",
                      static_cast<int64_t>(pData.nominalVoltage));
        props.emplace("MaxNegativeVoltage",
                      static_cast<int64_t>(pData.maxNegativeVoltage));
        props.emplace("MaxPositiveVoltage",
                      static_cast<int64_t>(pData.maxPositiveVoltage));
        props.emplace("RippleAndNoise",
                      static_cast<int64_t>(pData.rippleAndNoise));
        props.emplace("MinCurrentDraw",
                      static_cast<int64_t>(pData.minCurrentDraw));
        props.emplace("MaxCurrentDraw",
                      static_cast<int64_t>(pData.maxCurrentDraw));
        props.emplace("Standby", pData.standby());
    }
    virtual IpmiMultirecordType type() const
    {
        return IpmiMultirecordType::dcOutput;
    }

  private:
    DcOutputInfo pData;
};

static size_t recordSizes[static_cast<int>(IpmiMultirecordType::totalMrTypes)] =
    {[static_cast<int>(IpmiMultirecordType::powerSupplyInfo)] =
         sizeof(PowerSupplyInfo),
     [static_cast<int>(IpmiMultirecordType::dcOutput)] = sizeof(DcOutputInfo)};

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

template <typename T>
std::unique_ptr<T> create(const uint8_t* b)
{
    return std::make_unique<T>(T(b));
}

TypedRecord getMultirecordInfo(const IpmiMultirecordType recordType,
                               const uint8_t* recordData)
{
    TypedRecord info;
    switch (recordType)
    {
        case IpmiMultirecordType::powerSupplyInfo:
            info = create<PowerSupplyClass>(recordData);
            break;
        case IpmiMultirecordType::dcOutput:
            info = create<DcOutputClass>(recordData);
            break;
        default:
            break;
    }
    return info;
}

const uint8_t* parseRecord(const uint8_t* recordData, TypedRecord& record,
                           const size_t dataLen)
{
    constexpr size_t headerSize = sizeof(MultirecordHeader);

    if (dataLen < headerSize)
    {
        log<level::ERR>("Data shorter than header length");
        return NULL;
    }

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
    const auto header = reinterpret_cast<const MultirecordHeader*>(recordData);

    // set type
    IpmiMultirecordType recordType =
        static_cast<IpmiMultirecordType>(header->recordTypeId);

    if (recordType > maxParsedType)
    {
        log<level::ERR>("Unsupported record type");
        return NULL;
    }

    recordData += headerSize;

    // verify record length
    if (header->recordLength != recordSizes[header->recordTypeId])
    {
        log<level::ERR>("Invalid record length");
        return NULL;
    }

    if ((header->recordLength + headerSize) > dataLen)
    {
        log<level::ERR>("Data shorter than record lenght");
        return NULL;
    }

#ifdef IPMI_FRU_DEBUG
    log<level::DEBUG>(
        joinHex("Record data:", recordData, header->recordLength).c_str());
#endif

    // verify checksum
    checksum = calculateChecksum(recordData, header->recordLength) -
               header->recordChecksum;

    if (checksum)
    {
        log<level::ERR>("Invalid record checksum");
        return NULL;
    }

    // copy data
    record = getMultirecordInfo(recordType, recordData);

    if (header->endOfList())
    {
        recordData = NULL;
    }
    else
    {
        recordData += header->recordLength;
    }
    return recordData;
}

} // namespace multirecord

void parseMultirecord(const uint8_t* fruData, const size_t dataLen,
                      IPMIMultiInfo& info)
{
    using namespace multirecord;
    const uint8_t* recordData = fruData;
    TypedRecord record;
    while (recordData && static_cast<size_t>(recordData - fruData) <= dataLen)
    {
        recordData =
            parseRecord(recordData, record,
                        dataLen - static_cast<size_t>(recordData - fruData));
        if (record)
            info.push_back(std::move(record));
#ifdef IPMI_FRU_PARSER_DEBUG
        switch (record.type)
        {
            case IpmiMultirecordType::powerSupplyInfo:
                // clang-format off
                auto &psi = record.data.powerSupplyInfo;
                log<level::DEBUG>(
                    "Record PowerSupplyInfo",
                    entry("CAPACITY=" PRIu16, psi.overallCapacity),
                    entry("PEAK_VA=" PRIu16, psi.peakVa),
                    entry("INRUSH_CURRENT=" PRIu8, psi.inrushCurrent),
                    entry("INRUSH_INTERVAL=" PRIu8, psi.inrushInterval),
                    entry("LOW_IN_VOLTAGE1=" PRIi16, psi.lowInVoltage1),
                    entry("HIGH_IN_VOLTAGE1=" PRIi16, psi.highInVoltage1),
                    entry("LOW_IN_VOLTAGE2=" PRIi16, psi.lowInVoltage2),
                    entry("HIGH_IN_VOLTAGE2=" PRIi16, psi.highInVoltage2),
                    entry("LOW_IN_FREQUENCY=" PRIu8, psi.lowInFrequency),
                    entry("HIGH_IN_FREQUENCY=" PRIu8, psi.highInFrequency),
                    entry("IN_DROPOUT_TOLERANCE=" PRIu8, psi.inDropoutTolerance),
                    entry("PPR_PIN_POLARITY=%d", psi.PPRpinPolarity()),
                    entry("HOT_SWAP=%d", psi.hotSwap()),
                    entry("AUTOSWITCH=%d", psi.autoSwitch()),
                    entry("POWER_CORRECTION=%d", psi.powerCorrection()),
                    entry("PREDICTIVE_FAIL_SUPPORT=%d", psi.predFailSupport()),
                    entry("HOLD_UP_TIME=" PRIu16, psi.holdUpTime()),
                    entry("VOLTAGE1=" PRIu8, psi.combined_voltage(psi.voltage1)),
                    entry("VOLTAGE2=" PRIu8, psi.combined_voltage(psi.voltage2)),
                    entry("TOTAL_WATTAGE=" PRIu16, psi.totalWattage),
                    entry("TACHOMETER_LOW=" PRIu8, psi.tachometerLow);
                // clang-format on
                break;
            case IpmiMultirecordType::dcOutput:
                auto &dci = record.data.dcOutputInfo;
                log<level::DEBUG>(
                    "Record DcOutputInfo",
                    entry("NOMINAL_VOLTAGE=" PRIi16, dci.nominalVoltage),
                    entry("MAX_NEGATIVE_VOLTAGE=" PRIi16, dci.maxNegativeVoltage),
                    entry("MAX_POSITIVE_VOLTAGE=" PRIi16, dci.maxPositiveVoltage),
                    entry("RIPPLE_AND_NOISE=" PRIu16, dci.rippleAndNoise),
                    entry("MIN_CURRENT_DRAW=" PRIu16, dci.minCurrentDraw),
                    entry("MAX_CURRENT_DRAW=" PRIu16, dci.maxCurrentDraw),
                    entry("OUT_NUMBER=" PRIu8, dci.outNumber()),
                    entry("STANDBY=%d", dci.standby());
                break;
            default:
                log<level::DEBUG>("unsupported record");
                break;
        }
#endif
    }
}
