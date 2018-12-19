#ifndef __IPMI_WRITE_FRU_DATA_H__
#define __IPMI_WRITE_FRU_DATA_H__

#include <sdbusplus/bus.hpp>

// IPMI commands for Storage net functions.
enum ipmi_netfn_storage_cmds
{
    IPMI_CMD_WRITE_FRU_DATA = 0x12
};

// Format of write fru data command
struct write_fru_data_t
{
    uint8_t frunum;
    uint8_t offsetls;
    uint8_t offsetms;
    uint8_t data;
} __attribute__((packed));

// Per IPMI v2.0 FRU specification
struct common_header
{
    uint8_t fixed;
    uint8_t internal_offset;
    uint8_t chassis_offset;
    uint8_t board_offset;
    uint8_t product_offset;
    uint8_t multi_offset;
    uint8_t pad;
    uint8_t crc;
} __attribute__((packed));

// first byte in header is 1h per IPMI V2 spec.
#define IPMI_FRU_HDR_BYTE_ZERO 1
#define IPMI_FRU_INTERNAL_OFFSET offsetof(struct common_header, internal_offset)
#define IPMI_FRU_CHASSIS_OFFSET offsetof(struct common_header, chassis_offset)
#define IPMI_FRU_BOARD_OFFSET offsetof(struct common_header, board_offset)
#define IPMI_FRU_PRODUCT_OFFSET offsetof(struct common_header, product_offset)
#define IPMI_FRU_MULTI_OFFSET offsetof(struct common_header, multi_offset)
#define IPMI_FRU_HDR_CRC_OFFSET offsetof(struct common_header, crc)

constexpr size_t ipmiFRUBlockSize = 8; // Size of FRU allocation block, in bytes
constexpr size_t ipmiFRUTailSize = 2;  // Trailing bytes at the the end of FRU
                                       // file and almost each FRU area
/**
 * Validate a FRU.
 *
 * @param[in] fruid - The ID to use for this FRU.
 * @param[in] fruFilename - the filename of the FRU.
 * @param[in] bus - an sdbusplus systemd bus for publishing the information.
 * @param[in] bmcOnlyFru - If a particular area accessible only by BMC.
 */
int validateFRUArea(const uint8_t fruid, const char* fruFilename,
                    sdbusplus::bus::bus& bus, const bool bmcOnlyFru);

/**
 * Take a pointer to an array of bytes, and a length,
 * and return the 8-bit checksum as per IPMI Platform Management Information
 * Storage Definition v1.0 document revision 1.2.
 *
 * @param[in] data - The pointer to the array of bytes
 * @param[in] len - The length of the array
 * @return the checksum value
 */
uint8_t calculateChecksum(const unsigned char* data, size_t len);
#endif
