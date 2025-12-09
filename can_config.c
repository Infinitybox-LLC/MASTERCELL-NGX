/*
 * FILE: can_config.c
 * CAN Configuration Protocol Implementation
 * 
 * Implements read/write EEPROM configuration over J1939 CAN bus
 * 
 * UPDATED: Now accepts configuration messages from ANY source address
 */

#include "can_config.h"
#include "eeprom_config.h"
#include "j1939.h"
#include "eeprom_init.h"  // For working EEPROM_Init_WriteByte function
#include <string.h>

// Cached configuration values (loaded from EEPROM)
static uint16_t cached_read_pgn;
static uint16_t cached_write_pgn;
static uint16_t cached_response_pgn;
static uint8_t cached_read_sa;
static uint8_t cached_write_sa;
static uint8_t cached_response_sa;
static uint8_t cached_fw_major;
static uint8_t cached_fw_minor;

// Diagnostic counters
static uint32_t read_request_count = 0;
static uint32_t write_request_count = 0;
static uint16_t bad_guard_count = 0;
static uint16_t verify_fail_count = 0;
static uint16_t addr_range_error_count = 0;

/**
 * Initialize the CAN configuration system
 */
void CAN_Config_Init(void) {
    // Load configuration from EEPROM into cache
    CAN_Config_Reload();
}

/**
 * Reload configuration from EEPROM
 */
void CAN_Config_Reload(void) {
    // Read PGN/SA values from EEPROM
    cached_read_pgn = EEPROM_Config_ReadPGN(EEPROM_CFG_READ_REQ_PGN_A);
    cached_write_pgn = EEPROM_Config_ReadPGN(EEPROM_CFG_WRITE_REQ_PGN_A);
    cached_response_pgn = EEPROM_Config_ReadPGN(EEPROM_CFG_RESPONSE_PGN_A);
    
    cached_read_sa = EEPROM_Config_ReadByte(EEPROM_CFG_READ_REQ_SA);
    cached_write_sa = EEPROM_Config_ReadByte(EEPROM_CFG_WRITE_REQ_SA);
    cached_response_sa = EEPROM_Config_ReadByte(EEPROM_CFG_RESPONSE_SA);
    
    cached_fw_major = EEPROM_Config_ReadByte(EEPROM_CFG_FW_MAJOR);
    cached_fw_minor = EEPROM_Config_ReadByte(EEPROM_CFG_FW_MINOR);
}

/**
 * Extract PGN from 29-bit CAN ID
 * CAN ID format: [Priority(3)] [Reserved(1)] [DataPage(1)] [PDU_Format(8)] [PDU_Specific(8)] [SourceAddr(8)]
 * PGN = PDU_Format(8) | PDU_Specific(8) combined into 16 bits
 */
uint16_t CAN_Config_ExtractPGN(uint32_t can_id) {
    // Extract bits 8-23 (PDU Format and PDU Specific)
    uint16_t pgn = (uint16_t)((can_id >> 8) & 0xFFFF);
    return pgn;
}

/**
 * Extract Source Address from 29-bit CAN ID
 */
uint8_t CAN_Config_ExtractSA(uint32_t can_id) {
    // Extract bits 0-7 (Source Address)
    return (uint8_t)(can_id & 0xFF);
}

/**
 * Build 29-bit CAN ID from priority, PGN, and source address
 * Format: [Priority(3)] [Reserved(1)] [DataPage(1)] [PGN(16)] [SA(8)]
 */
uint32_t CAN_Config_BuildCANID(uint8_t priority, uint16_t pgn, uint8_t source_addr) {
    uint32_t can_id = 0;
    
    // Priority (bits 26-28)
    can_id |= ((uint32_t)(priority & 0x07) << 26);
    
    // PGN (bits 8-23)
    can_id |= ((uint32_t)pgn << 8);
    
    // Source Address (bits 0-7)
    can_id |= (uint32_t)source_addr;
    
    return can_id;
}

/**
 * Check if received CAN ID matches configured Read Request PGN
 * UPDATED: Now accepts from ANY source address for configuration flexibility
 */
uint8_t CAN_Config_IsReadRequest(uint32_t can_id) {
    uint16_t pgn = CAN_Config_ExtractPGN(can_id);
    // Only check PGN, accept from any SA
    // (cached_read_sa is kept for backwards compatibility but not used for filtering)
    return (pgn == cached_read_pgn);
}

/**
 * Check if received CAN ID matches configured Write Request PGN
 * UPDATED: Now accepts from ANY source address for configuration flexibility
 */
uint8_t CAN_Config_IsWriteRequest(uint32_t can_id) {
    uint16_t pgn = CAN_Config_ExtractPGN(can_id);
    // Only check PGN, accept from any SA
    // (cached_write_sa is kept for backwards compatibility but not used for filtering)
    return (pgn == cached_write_pgn);
}

/**
 * Process an incoming CAN message
 */
uint8_t CAN_Config_ProcessMessage(CAN_Message *msg) {
    if (msg == NULL || !msg->valid) {
        return 0;
    }
    
    // Check if this is a read request
    if (CAN_Config_IsReadRequest(msg->id)) {
        CAN_Config_HandleReadRequest(msg);
        return 1;
    }
    
    // Check if this is a write request
    if (CAN_Config_IsWriteRequest(msg->id)) {
        CAN_Config_HandleWriteRequest(msg);
        return 1;
    }
    
    return 0;  // Not a config message
}

/**
 * Handle a read request message
 */
void CAN_Config_HandleReadRequest(CAN_Message *msg) {
    read_request_count++;
    
    // Validate guard byte
    if (msg->data[0] != CAN_CONFIG_GUARD_BYTE) {
        bad_guard_count++;
        CAN_Config_SendResponse(0, 0, CAN_CONFIG_STATUS_BAD_GUARD);
        return;
    }
    
    // Extract address (little-endian)
    uint16_t addr = (uint16_t)msg->data[1] | ((uint16_t)msg->data[2] << 8);
    
    // Validate address range
    if (addr > CAN_CONFIG_MAX_READ_ADDR) {
        addr_range_error_count++;
        CAN_Config_SendResponse(addr, 0, CAN_CONFIG_STATUS_ADDR_OUT_OF_RANGE);
        return;
    }
    
    // Read byte from EEPROM
    uint8_t value = EEPROM_Config_ReadByte(addr);
    
    // Send response with success status
    CAN_Config_SendResponse(addr, value, CAN_CONFIG_STATUS_SUCCESS);
}

/**
 * Handle a write request message
 */
void CAN_Config_HandleWriteRequest(CAN_Message *msg) {
    write_request_count++;
    
    // Validate guard byte
    if (msg->data[0] != CAN_CONFIG_GUARD_BYTE) {
        bad_guard_count++;
        CAN_Config_SendResponse(0, 0, CAN_CONFIG_STATUS_BAD_GUARD);
        return;
    }
    
    // Extract address (little-endian)
    uint16_t addr = (uint16_t)msg->data[1] | ((uint16_t)msg->data[2] << 8);
    
    // Extract value to write
    uint8_t value = msg->data[3];
    
    // Validate address range (more restrictive for writes)
    if (addr > CAN_CONFIG_MAX_WRITE_ADDR) {
        addr_range_error_count++;
        CAN_Config_SendResponse(addr, value, CAN_CONFIG_STATUS_ADDR_OUT_OF_RANGE);
        return;
    }
    
    // Write byte to EEPROM
    uint8_t write_success = EEPROM_Init_WriteByte(addr, value);
    
    if (!write_success) {
        verify_fail_count++;
        CAN_Config_SendResponse(addr, value, CAN_CONFIG_STATUS_VERIFY_FAILED);
        return;
    }
    
    // Verify the write by reading back
    uint8_t verify_value = EEPROM_Config_ReadByte(addr);
    
    if (verify_value != value) {
        verify_fail_count++;
        CAN_Config_SendResponse(addr, verify_value, CAN_CONFIG_STATUS_VERIFY_FAILED);
        return;
    }
    
    // Hot-reload configuration if we modified config bytes
    if (addr <= EEPROM_CFG_SERIAL_NUMBER) {
        CAN_Config_Reload();
    }
    
    // Send response with success status
    CAN_Config_SendResponse(addr, verify_value, CAN_CONFIG_STATUS_SUCCESS);
}

/**
 * Send a response message
 */
void CAN_Config_SendResponse(uint16_t addr, uint8_t value, uint8_t status) {
    uint8_t response_data[8];
    
    // Build response message
    response_data[0] = cached_fw_major;         // Firmware major version
    response_data[1] = cached_fw_minor;         // Firmware minor version
    response_data[2] = value;                   // Value read or written
    response_data[3] = (uint8_t)(addr & 0xFF);  // Address LSB
    response_data[4] = (uint8_t)(addr >> 8);    // Address MSB
    response_data[5] = status;                  // Status code
    response_data[6] = 0x00;                    // Reserved
    response_data[7] = 0x00;                    // Reserved
    
    // Send via J1939
    // Use priority 3 for response messages
    J1939_TransmitMessage(3, cached_response_pgn, cached_response_sa, response_data);
}

/**
 * Get current cached Read Request PGN
 */
uint16_t CAN_Config_GetReadPGN(void) {
    return cached_read_pgn;
}

/**
 * Get current cached Write Request PGN
 */
uint16_t CAN_Config_GetWritePGN(void) {
    return cached_write_pgn;
}

/**
 * Get current cached Response PGN
 */
uint16_t CAN_Config_GetResponsePGN(void) {
    return cached_response_pgn;
}

/**
 * Get current cached Read Request SA
 */
uint8_t CAN_Config_GetReadSA(void) {
    return cached_read_sa;
}

/**
 * Get current cached Write Request SA
 */
uint8_t CAN_Config_GetWriteSA(void) {
    return cached_write_sa;
}

/**
 * Get current cached Response SA
 */
uint8_t CAN_Config_GetResponseSA(void) {
    return cached_response_sa;
}

/**
 * Get diagnostic information
 */
uint32_t CAN_Config_GetReadRequestCount(void) {
    return read_request_count;
}

uint32_t CAN_Config_GetWriteRequestCount(void) {
    return write_request_count;
}

uint16_t CAN_Config_GetBadGuardCount(void) {
    return bad_guard_count;
}

uint16_t CAN_Config_GetVerifyFailCount(void) {
    return verify_fail_count;
}

uint16_t CAN_Config_GetAddrRangeErrorCount(void) {
    return addr_range_error_count;
}