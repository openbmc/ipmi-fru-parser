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

// Enumeration of supported bit-field MultiRecord FRU Area parameters.
//
// These are defined in various tables within IPMI FRU Information Storage
// Definition v1.0 revision 1.3.
//
// clang-format off
enum class IpmiMultirecordParam
{
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
};
// clang-format on

// A helper to convert Volts to 10mV units.
// Most voltage units in Power Supply Information record are in 10mV units
// per specification, except for combined wattage voltages, which are just
// indexed values.
//
// In order to not mix the units in which the voltages are represented across
// the object properties in d-bus, we're converting the voltage indices into
// common units.
static constexpr int64_t Vto10mV(const double V)
{
    return static_cast<int64_t>(V * 100);
}

// Indices and values are per IPMI FRU Definition 1.0 rev 1.3,
// section 18.1.14:
static constexpr int64_t voltages[] = {
    Vto10mV(12.0),  // [0b0000]
    Vto10mV(-12.0), // [0b0001]
    Vto10mV(5.0),   // [0b0010]
    Vto10mV(3.3)    // [0b0011]
};

// A helper function to extract bitfield values using mask and shift
static int64_t mrBits(const int64_t r, const IpmiMultirecordParam& p)
{
    struct MaskShift
    {
        uint8_t nbits; // Number of bits for parameter p, LSB at pos
        uint8_t pos;   // Position of LSB in a word or byte r
    } maskShift;

    // Bit sizes and positions for bit-field parameters indexed by p.
    // The masks are taken from corresponding tables in
    // IPMI FRU Information Storage Definition v1.0 revision 1.3
    using mrp = IpmiMultirecordParam; // For brevity
    static const std::map<mrp, MaskShift> maskShiftMap = {
        // Table 16-1:
        {mrp::paramEolst, {1, 7}},
        {mrp::paramFormat, {4, 0}},
        // Table 18-1:
        {mrp::psuTachoPPR, {1, 4}},
        {mrp::psuHotswap, {1, 3}},
        {mrp::psuAutoswitch, {1, 2}},
        {mrp::psuCorrection, {1, 1}},
        {mrp::psuPredFailSupport, {1, 0}},
        {mrp::psuHoldup, {4, 11}},
        {mrp::psuCapacity, {12, 0}},
        {mrp::psuVoltage1, {4, 4}},
        {mrp::psuVoltage2, {4, 0}},
        // Table 18-2:
        {mrp::dcOutStandby, {1, 7}},
        {mrp::dcOutOutnumber, {4, 0}}};

    maskShift = maskShiftMap.find(p)->second;
    int64_t mask = (1LL << maskShift.nbits) - 1;
    return (r >> maskShift.pos) & mask;
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
    inline int64_t recordFormat(void) const
    {
        return mrBits(recordParams, IpmiMultirecordParam::paramFormat);
    }

    uint8_t recordLength;
    uint8_t recordChecksum;
    uint8_t headerChecksum;

} __attribute__((packed));

// This structure is initialized from the raw byte array of FRU data
// and is used to parse that raw data into parts. Hence, the fields here
// are defined in the order given in IPMI Platform Management FRU Information
// Storage Definition v1.0, section 18.1, and must not be reordered!
struct PowerSupplyInfoData
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
    uint16_t peakWattage;
    uint8_t combinedVoltages;
    uint16_t totalWattage;
    uint8_t tachometerLow;
} __attribute__((packed));

// This structure is initialized from the raw byte array of FRU data
// and is used to parse that raw data into parts. Hence, the fields here
// are defined in the order given in IPMI Platform Management FRU Information
// Storage Definition v1.0, section 18.2, and must not be reordered!
struct DCOutputInfoData
{
    uint8_t outputInformation;
    int16_t nominalVoltage;     /* 10 mV */
    int16_t maxNegativeVoltage; /* 10 mV */
    int16_t maxPositiveVoltage; /* 10 mV */
    uint16_t rippleAndNoise;    /* mV */
    uint16_t minCurrentDraw;    /* mA */
    uint16_t maxCurrentDraw;    /* mA */
} __attribute__((packed));

class PowerSupplyInfo : public MultirecordInfo
{
  public:
    PowerSupplyInfo() = delete;
    // This constructor is called only from parseRecord()
    // where it is guaranteed that `data` has enough bytes to fill
    // the pData structure.
    explicit PowerSupplyInfo(const uint8_t* data) :
        pData(*reinterpret_cast<const PowerSupplyInfoData*>(data))
    {
        // Perform the IPMI-to-host endianness conversion for
        // multi-byte data fields:
        pData.overallCapacity = le16toh(pData.overallCapacity);
        pData.peakVa = le16toh(pData.peakVa);
        pData.lowInVoltage1 = le16toh(pData.lowInVoltage1);
        pData.highInVoltage1 = le16toh(pData.highInVoltage1);
        pData.lowInVoltage2 = le16toh(pData.lowInVoltage2);
        pData.highInVoltage2 = le16toh(pData.highInVoltage2);
        pData.peakWattage = le16toh(pData.peakWattage);
        pData.totalWattage = le16toh(pData.totalWattage);
    }
    virtual ipmi::vpd::PropertyMap updatePropertyMap() const final
    {
        ipmi::vpd::PropertyMap props;
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
        props.emplace("HoldUpTime", holdUpTime());
        props.emplace("PeakCapacity", peakCapacity());
        props.emplace("Voltage1", combinedVoltage(voltage1));
        props.emplace("Voltage2", combinedVoltage(voltage2));
        props.emplace("TotalWattage", static_cast<int64_t>(pData.totalWattage));
        props.emplace("TachometerLow",
                      static_cast<int64_t>(pData.tachometerLow));
        props.emplace("HotSwap", hotSwap());
        if (predFailSupport())
        {
            props.emplace("TachoPulsesPerRotation", tachoPPR() ? 2LL : 1LL);
            props.emplace("PredictiveFailPinNegated", false);
        }
        else
        {
            props.emplace("TachoPulsesPerRotation", 0LL);
            props.emplace("PredictiveFailPinNegated", failurePinNegated());
        }
        props.emplace("Autoswitch", autoSwitch());
        props.emplace("PowerFactor", powerCorrection());
        props.emplace("PredictiveFailSupport", predFailSupport());
        return props;
    }
    virtual IpmiMultirecordType type() const final
    {
        return IpmiMultirecordType::powerSupplyInfo;
    }

  private:
    PowerSupplyInfoData pData;

    // Below go the parsers for bit-mapped fields of pData, as well as
    // helper functions for those parsers.

    using mrp = IpmiMultirecordParam; // For brevity

    bool tachoPPR(void) const
    {
        return mrBits(pData.binaryFlags, mrp::psuTachoPPR);
    }

    bool failurePinNegated(void) const
    {
        return mrBits(pData.binaryFlags, mrp::psuFailPinPolarity);
    }

    bool hotSwap(void) const
    {
        return mrBits(pData.binaryFlags, mrp::psuHotswap);
    }

    bool autoSwitch(void) const
    {
        return mrBits(pData.binaryFlags, mrp::psuAutoswitch);
    }

    bool powerCorrection(void) const
    {
        return mrBits(pData.binaryFlags, mrp::psuCorrection);
    }

    bool predFailSupport(void) const
    {
        return mrBits(pData.binaryFlags, mrp::psuPredFailSupport);
    }

    // Arguments for combined_voltage()
    static constexpr size_t voltage1 = 0;
    static constexpr size_t voltage2 = 1;
    int64_t combinedVoltage(size_t vidx) const
    {
        // Section 18.1.14:
        // "A value of 0 for the combined voltage field indicates there are
        // no combined voltages for a power supply."
        if (!pData.combinedVoltages && !pData.totalWattage)
            return 0;

        mrp param[2] = {mrp::psuVoltage1, mrp::psuVoltage2};

        if (vidx > voltage2)
            return 0;

        size_t voltageEncoded = mrBits(pData.combinedVoltages, param[vidx]);
        return voltages[voltageEncoded];
    }

    int64_t holdUpTime(void) const
    {
        return mrBits(pData.peakWattage, mrp::psuHoldup);
    }

    int64_t peakCapacity(void) const
    {
        return mrBits(pData.peakWattage, mrp::psuCapacity);
    }
};

class DCOutputInfo : public MultirecordInfo
{
  public:
    DCOutputInfo() = delete;
    // This constructor is called only from parseRecord()
    // where it is guaranteed that `data` has enough bytes to fill
    // the pData structure.
    explicit DCOutputInfo(const uint8_t* data) :
        pData(*reinterpret_cast<const DCOutputInfoData*>(data))
    {
        // Perform the IPMI-to-host endianness conversion for
        // multi-byte data fields:
        pData.nominalVoltage = le16toh(pData.nominalVoltage);
        pData.maxNegativeVoltage = le16toh(pData.maxNegativeVoltage);
        pData.maxPositiveVoltage = le16toh(pData.maxPositiveVoltage);
        pData.rippleAndNoise = le16toh(pData.rippleAndNoise);
        pData.minCurrentDraw = le16toh(pData.minCurrentDraw);
        pData.maxCurrentDraw = le16toh(pData.maxCurrentDraw);
    }
    virtual ipmi::vpd::PropertyMap updatePropertyMap() const final
    {
        ipmi::vpd::PropertyMap props;
        props.emplace("OutputNumber", static_cast<int64_t>(outNumber()));
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
        props.emplace("Standby", standby());
        return props;
    }
    virtual IpmiMultirecordType type() const final
    {
        return IpmiMultirecordType::dcOutput;
    }

  private:
    DCOutputInfoData pData;

    // Below go the parsers for bit-mapped fields of pData, as well as
    // helper functions for those parsers.
    using mrp = IpmiMultirecordParam; // For brevity

    bool standby(void) const
    {
        return mrBits(pData.outputInformation, mrp::dcOutStandby);
    }
    int64_t outNumber(void) const
    {
        return mrBits(pData.outputInformation, mrp::dcOutOutnumber);
    }
};

#ifdef IPMI_FRU_DEBUG
static std::string joinHex(const std::string& prefix, const uint8_t* data,
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
static std::unique_ptr<T> unique(const uint8_t* b)
{
    return std::make_unique<T>(T(b));
}

///
/// @brief Parse a single multirecord area record and advance the data pointer
///
/// On success, this function will create a new typed object from parsed
/// data and will store it in the \p record parameter. On parse errors
/// the function will not touch \p record, and if further FRU data processing
/// is impossible will also set \p recordData to nullptr.
///
/// @param[in] recordData Reference to the pointer to a FRU byte array.
///                       Points at the record to parse within that array.
/// @param[out] recordData The pointer referenced by this parameter will be
///                        set to point at the next record after the parsed one.
/// @param[out] record Will contain the new object created from the parsed
///                    data unless there was a parsing error, in which case
///                    this reference will be left untouched.
/// @param[in] dataLen The remaining length in bytes till the end of the
///                    FRU data buffer to which the \p recordData parameter
///                    is pointing.
///
static void parseRecord(const uint8_t*& recordData, TypedRecord& record,
                        const size_t dataLen)
{
    constexpr size_t headerSize = sizeof(MultirecordHeader);

    if (dataLen < headerSize)
    {
        log<level::ERR>("Data shorter than header length");
        recordData = nullptr; // Bail out, indicate that further data
                              // processing is impossible.
        return;
    }

#ifdef IPMI_FRU_DEBUG
    log<level::DEBUG>(
        joinHex("Record header:", recordData, headerSize).c_str());
#endif
    // verify header checksum
    uint8_t checksum = calculateChecksum(recordData, headerSize);

    if (checksum)
    {
        // We can't trust this header and so we can't process to the
        // next record as we don't know how to find its head.
        log<level::ERR>("Invalid header checksum");
        recordData = nullptr; // Bail out, indicate that further data
                              // processing is impossible.
        return;
    }

    // parse header
    const auto header = reinterpret_cast<const MultirecordHeader*>(recordData);
    const auto recordType =
        static_cast<IpmiMultirecordType>(header->recordTypeId);

    recordData += headerSize;

    // If this record type is not supported, log a warning regarding
    // detection of unsupported recort type.
    size_t expectedLength = 0;
    switch (recordType)
    {
        case IpmiMultirecordType::powerSupplyInfo:
            expectedLength = sizeof(PowerSupplyInfoData);
            break;
        case IpmiMultirecordType::dcOutput:
            expectedLength = sizeof(DCOutputInfoData);
            break;
        default:
            log<level::INFO>("Unsupported MultiRecord Area Record type",
                             entry("MR_RECORD_TYPE=" PRIu8, recordType));
            // Just leave the expectedLength zero
    }

    // If the record type is not supported and thus expectedLength is zero,
    // just skip the record as we still want to process further ones that
    // may have supported types.
    if (expectedLength)
    {
        // verify record length
        if (header->recordLength != expectedLength)
        {
            // If the record length in the header does not match
            // the record length for this record type as we know it
            // from the specification, that means the FRU data is corrupt
            // and we don't know how to process further, so bail out.
            log<level::ERR>("Invalid record length");
            recordData = nullptr;
            return;
        }

        if ((header->recordLength + headerSize) > dataLen)
        {
            // We are apparently out of data, so bail out.
            log<level::ERR>("Data shorter than record length");
            recordData = nullptr;
            return;
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
            // There is still a chance that the next record will be fine,
            // so don't bail out.
            log<level::ERR>("Invalid record checksum");
        }
        else
        {
            // Finally, all meta-info is fine, create the record object
            // from the supplied data.
            switch (recordType)
            {
                case IpmiMultirecordType::powerSupplyInfo:
                    record = unique<PowerSupplyInfo>(recordData);
                    break;
                case IpmiMultirecordType::dcOutput:
                    record = unique<DCOutputInfo>(recordData);
                    break;
                default:
                    // We've already checked before that recordType is
                    // supported, so we should never hit this. This is here
                    // just to keep the compiler calm about not all enum
                    // values covered by switch case labels.
                    break;
            }
        }
    }

    // If this record says it's the last one, indicate that to the caller.
    // Otherwise, prepare the pointer for processing of the next record.
    if (header->endOfList())
    {
        recordData = nullptr;
    }
    else
    {
        recordData += header->recordLength;
    }
}

#ifdef IPMI_FRU_PARSER_DEBUG
static inline void debugRecordParser(TypeRecord& record)
{
    switch (record.type)
    {
        case IpmiMultirecordType::powerSupplyInfo:
            auto& psi = record.data.powerSupplyInfo;
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
                entry("TACHOMETER_LOW=" PRIu8, psi.tachometerLow));
            break;
        case IpmiMultirecordType::dcOutput:
            auto& dci = record.data.dcOutputInfo;
            log<level::DEBUG>(
                "Record DcOutputInfo",
                entry("NOMINAL_VOLTAGE=" PRIi16, dci.nominalVoltage),
                entry("MAX_NEGATIVE_VOLTAGE=" PRIi16, dci.maxNegativeVoltage),
                entry("MAX_POSITIVE_VOLTAGE=" PRIi16, dci.maxPositiveVoltage),
                entry("RIPPLE_AND_NOISE=" PRIu16, dci.rippleAndNoise),
                entry("MIN_CURRENT_DRAW=" PRIu16, dci.minCurrentDraw),
                entry("MAX_CURRENT_DRAW=" PRIu16, dci.maxCurrentDraw),
                entry("OUT_NUMBER=" PRIu8, dci.outNumber()),
                entry("STANDBY=%d", dci.standby()));
            break;
        default:
            log<level::DEBUG>("unsupported record");
            break;
    }
}
#endif

} // namespace multirecord

///
/// @brief Parse multirecord area at \p fruData
///
/// This function attempts to parse a FRU MultiRecord area record by record,
/// and store the parsed typed objects in the \p info vector.
///
/// @param[in] fruData Pointer to a FRU byte array. Must point at the
///                    head of MultiRecord area.
/// @param[in] dataLen The length in bytes of the FRU byte array pointed
///                    to by \p fruData.
/// @param[out] info A vector where all parsed records will be stored.
///
void parseMultirecord(const uint8_t* fruData, const size_t dataLen,
                      IPMIMultiInfo& info)
{
    using namespace multirecord;
    const uint8_t* recordData = fruData;

    while (recordData && static_cast<size_t>(recordData - fruData) <= dataLen)
    {
        TypedRecord record;
        parseRecord(recordData, record,
                    dataLen - static_cast<size_t>(recordData - fruData));
        if (record)
        {
#ifdef IPMI_FRU_PARSER_DEBUG
            debugRecordParser(record);
#endif
            info.push_back(std::move(record));
        }
    }
}
