/*
 * FILE: can_config.h
 * CAN Configuration Protocol for MASTERCELL NGX
 * 
 * Implements IOX NGX-compatible configuration protocol over J1939:
 * - Read EEPROM bytes via CAN
 * - Write EEPROM bytes via CAN
 * - Send response messages with status codes
 * 
 * Protocol Definition (matching IOX NGX):
 * 
 * READ REQUEST:
 *   ID: Configured Read Request PGN/SA (default 0x18FF2080)
 *   Data: [0x77] [ADDR_LSB] [ADDR_MSB] [0xFF] [0xFF] [0xFF] [0xFF] [0xFF]
 *   - Guard byte: 0x77
 *   - Address: 16-bit byte address (little-endian)
 *   - Remaining bytes: 0xFF (ignored)
 * 
 * WRITE REQUEST:
 *   ID: Configured Write Request PGN/SA (default 0x18FF1080)
 *   Data: [0x77] [ADDR_LSB] [ADDR_MSB] [VALUE] [0xFF] [0xFF] [0xFF] [0xFF]
 *   - Guard byte: 0x77
 *   - Address: 16-bit byte address (little-endian)
 *   - Value: Byte value to write
 *   - Remaining bytes: 0xFF (ignored)
 * 
 * RESPONSE:
 *   ID: Configured Response PGN/SA (default 0x18FF3080)
 *   Data: [FW_MAJ] [FW_MIN] [VALUE] [ADDR_LSB] [ADDR_MSB] [STATUS] [0x00] [0x00]
 *   - FW_MAJ: Firmware major version
 *   - FW_MIN: Firmware minor version
 *   - VALUE: Byte value read or written
 *   - Address: 16-bit byte address (little-endian)
 *   - Status: Result code
 *   - Remaining bytes: 0x00
 * 
 * Status Codes:
 *   0x01 = Success
 *   0xE1 = Bad Guard Byte (not 0x77)
 *   0xE5 = Write Verification Failed
 *   0xE6 = Address Out of Range
 */

#ifndef CAN_CONFIG_H
#define CAN_CONFIG_H

#include <xc.h>
#include <stdint.h>

// Guard byte for read/write requests
#define CAN_CONFIG_GUARD_BYTE       0x77

// Status codes for response messages
#define CAN_CONFIG_STATUS_SUCCESS           0x01    // Operation successful
#define CAN_CONFIG_STATUS_BAD_GUARD         0xE1    // Invalid guard byte
#define CAN_CONFIG_STATUS_VERIFY_FAILED     0xE5    // Write verification failed
#define CAN_CONFIG_STATUS_ADDR_OUT_OF_RANGE 0xE6    // Address out of valid range

// Maximum byte address for EEPROM access
// dsPIC30F6012A has 4096 bytes of EEPROM (0x0000 to 0x0FFF)
// Memory map:
//   0x0000-0x0016: Configuration bytes (23 bytes)
//   0x0017-0x0021: Reserved (11 bytes)
//   0x0022-0x0FE1: Case data (126 cases × 32 bytes = 4032 bytes)
//   0x0FE2-0x0FFF: Unused (30 bytes)
#define CAN_CONFIG_MAX_WRITE_ADDR   4095    // Can write entire EEPROM (0x0FFF)
#define CAN_CONFIG_MAX_READ_ADDR    4095    // Can read entire EEPROM (0x0FFF)

// Note: This matches the CAN_RxMessage structure from j1939.h
typedef struct {
    uint32_t id;            // 29-bit CAN ID
    uint8_t data[8];        // 8 bytes of data
    uint8_t dlc;            // Data length code (should be 8)
    uint8_t valid;          // 1 if message valid, 0 if not
} CAN_Message;

/**
 * Initialize the CAN configuration system
 * Loads current configuration from EEPROM into RAM cache
 */
void CAN_Config_Init(void);

/**
 * Process an incoming CAN message
 * Checks if message is a read or write request and handles accordingly
 * 
 * @param msg Pointer to received CAN message
 * @return 1 if message was handled, 0 if not a config message
 */
uint8_t CAN_Config_ProcessMessage(CAN_Message *msg);

/**
 * Handle a read request message
 * Reads byte from EEPROM and sends response
 * 
 * @param msg Pointer to received read request message
 */
void CAN_Config_HandleReadRequest(CAN_Message *msg);

/**
 * Handle a write request message
 * Writes byte to EEPROM, verifies, and sends response
 * 
 * @param msg Pointer to received write request message
 */
void CAN_Config_HandleWriteRequest(CAN_Message *msg);

/**
 * Send a response message
 * 
 * @param addr Byte address that was read/written
 * @param value Byte value that was read/written
 * @param status Status code (0x01=success, 0xE1=bad guard, etc.)
 */
void CAN_Config_SendResponse(uint16_t addr, uint8_t value, uint8_t status);

/**
 * Extract PGN from 29-bit CAN ID
 * 
 * @param can_id 29-bit extended CAN ID
 * @return 16-bit PGN value
 */
uint16_t CAN_Config_ExtractPGN(uint32_t can_id);

/**
 * Extract Source Address from 29-bit CAN ID
 * 
 * @param can_id 29-bit extended CAN ID
 * @return 8-bit source address
 */
uint8_t CAN_Config_ExtractSA(uint32_t can_id);

/**
 * Build 29-bit CAN ID from priority, PGN, and source address
 * 
 * @param priority Priority (0-7, 0 is highest)
 * @param pgn 16-bit PGN
 * @param source_addr 8-bit source address
 * @return 29-bit CAN ID
 */
uint32_t CAN_Config_BuildCANID(uint8_t priority, uint16_t pgn, uint8_t source_addr);

/**
 * Check if received CAN ID matches configured Read Request PGN/SA
 * 
 * @param can_id 29-bit CAN ID
 * @return 1 if matches, 0 if not
 */
uint8_t CAN_Config_IsReadRequest(uint32_t can_id);

/**
 * Check if received CAN ID matches configured Write Request PGN/SA
 * 
 * @param can_id 29-bit CAN ID
 * @return 1 if matches, 0 if not
 */
uint8_t CAN_Config_IsWriteRequest(uint32_t can_id);

/**
 * Reload configuration from EEPROM
 * Called after configuration bytes are modified via CAN
 * Updates cached PGN/SA values for read/write/response messages
 */
void CAN_Config_Reload(void);

/**
 * Get current cached Read Request PGN
 * @return 16-bit PGN
 */
uint16_t CAN_Config_GetReadPGN(void);

/**
 * Get current cached Write Request PGN
 * @return 16-bit PGN
 */
uint16_t CAN_Config_GetWritePGN(void);

/**
 * Get current cached Response PGN
 * @return 16-bit PGN
 */
uint16_t CAN_Config_GetResponsePGN(void);

/**
 * Get current cached Read Request SA
 * @return 8-bit source address
 */
uint8_t CAN_Config_GetReadSA(void);

/**
 * Get current cached Write Request SA
 * @return 8-bit source address
 */
uint8_t CAN_Config_GetWriteSA(void);

/**
 * Get current cached Response SA
 * @return 8-bit source address
 */
uint8_t CAN_Config_GetResponseSA(void);

/**
 * Get diagnostic information - number of read requests processed
 * @return Total read requests
 */
uint32_t CAN_Config_GetReadRequestCount(void);

/**
 * Get diagnostic information - number of write requests processed
 * @return Total write requests
 */
uint32_t CAN_Config_GetWriteRequestCount(void);

/**
 * Get diagnostic information - number of bad guard byte errors
 * @return Total bad guard errors
 */
uint16_t CAN_Config_GetBadGuardCount(void);

/**
 * Get diagnostic information - number of write verification failures
 * @return Total verify failures
 */
uint16_t CAN_Config_GetVerifyFailCount(void);

/**
 * Get diagnostic information - number of out-of-range address errors
 * @return Total address range errors
 */
uint16_t CAN_Config_GetAddrRangeErrorCount(void);

#endif // CAN_CONFIG_H