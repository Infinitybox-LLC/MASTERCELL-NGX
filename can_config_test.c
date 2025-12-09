/*
 * FILE: can_config_test.c
 * Test Implementation for CAN Configuration Protocol
 */

#include "can_config_test.h"
#include "can_config.h"
#include "eeprom_config.h"
#include <stdio.h>
#include <string.h>

/**
 * Test CAN ID extraction functions
 */
uint16_t CAN_Config_Test_CANIDExtraction(void) {
    uint16_t passed = 0;
    uint16_t total = 0;
    
    // Test 1: Extract PGN from Heartbeat ID (0x18FF0080)
    total++;
    uint32_t hb_id = 0x18FF0080;
    uint16_t pgn = CAN_Config_ExtractPGN(hb_id);
    if (pgn == 0xFF00) {
        passed++;
    }
    
    // Test 2: Extract SA from Heartbeat ID
    total++;
    uint8_t sa = CAN_Config_ExtractSA(hb_id);
    if (sa == 0x80) {
        passed++;
    }
    
    // Test 3: Extract PGN from Write Request (0x18FF1080)
    total++;
    uint32_t write_id = 0x18FF1080;
    pgn = CAN_Config_ExtractPGN(write_id);
    if (pgn == 0xFF10) {
        passed++;
    }
    
    // Test 4: Extract SA from Write Request
    total++;
    sa = CAN_Config_ExtractSA(write_id);
    if (sa == 0x80) {
        passed++;
    }
    
    // Test 5: Extract PGN from Read Request (0x18FF2080)
    total++;
    uint32_t read_id = 0x18FF2080;
    pgn = CAN_Config_ExtractPGN(read_id);
    if (pgn == 0xFF20) {
        passed++;
    }
    
    // Test 6: Extract SA from Read Request
    total++;
    sa = CAN_Config_ExtractSA(read_id);
    if (sa == 0x80) {
        passed++;
    }
    
    // Test 7: Build CAN ID and verify
    total++;
    uint32_t built_id = CAN_Config_BuildCANID(3, 0xFF00, 0x80);
    // Priority 3 should be in bits 26-28 = 0x0C000000
    // PGN 0xFF00 in bits 8-23 = 0x00FF0000
    // SA 0x80 in bits 0-7 = 0x00000080
    // Expected: 0x0CFF0080, but priority 6 gives 0x18FF0080
    // Let's test with priority 6 (standard J1939 priority)
    built_id = CAN_Config_BuildCANID(6, 0xFF00, 0x80);
    if (built_id == 0x18FF0080) {
        passed++;
    }
    
    return passed;
}

/**
 * Test message filtering
 */
uint16_t CAN_Config_Test_MessageFiltering(void) {
    uint16_t passed = 0;
    uint16_t total = 0;
    
    // Initialize CAN config to load default values
    CAN_Config_Init();
    
    // Test 1: Read Request ID should be recognized
    total++;
    uint32_t read_id = 0x18FF2080;  // Default Read Request
    if (CAN_Config_IsReadRequest(read_id)) {
        passed++;
    }
    
    // Test 2: Write Request ID should be recognized
    total++;
    uint32_t write_id = 0x18FF1080;  // Default Write Request
    if (CAN_Config_IsWriteRequest(write_id)) {
        passed++;
    }
    
    // Test 3: Heartbeat ID should NOT be recognized as read request
    total++;
    uint32_t hb_id = 0x18FF0080;
    if (!CAN_Config_IsReadRequest(hb_id)) {
        passed++;
    }
    
    // Test 4: Heartbeat ID should NOT be recognized as write request
    total++;
    if (!CAN_Config_IsWriteRequest(hb_id)) {
        passed++;
    }
    
    // Test 5: Wrong SA should not match
    total++;
    uint32_t wrong_sa = 0x18FF2081;  // Read PGN but SA=0x81
    if (!CAN_Config_IsReadRequest(wrong_sa)) {
        passed++;
    }
    
    // Test 6: Wrong PGN should not match
    total++;
    uint32_t wrong_pgn = 0x18FF2180;  // Different PGN
    if (!CAN_Config_IsReadRequest(wrong_pgn)) {
        passed++;
    }
    
    return passed;
}

/**
 * Test read request handling
 */
uint16_t CAN_Config_Test_ReadRequest(void) {
    uint16_t passed = 0;
    uint16_t total = 0;
    
    CAN_Message msg;
    
    // First, write a known value to EEPROM
    EEPROM_Config_WriteByte(EEPROM_CFG_BITRATE, 0x01);
    
    // Test 1: Valid read request for bitrate
    total++;
    msg.id = 0x18FF2080;  // Read Request ID
    msg.data[0] = 0x77;   // Guard byte
    msg.data[1] = 0x00;   // Address LSB (byte 0)
    msg.data[2] = 0x00;   // Address MSB
    msg.data[3] = 0xFF;
    msg.data[4] = 0xFF;
    msg.data[5] = 0xFF;
    msg.data[6] = 0xFF;
    msg.data[7] = 0xFF;
    msg.dlc = 8;
    msg.valid = 1;
    
    uint8_t handled = CAN_Config_ProcessMessage(&msg);
    if (handled) {
        passed++;
    }
    
    // Test 2: Read firmware major version
    total++;
    msg.data[1] = EEPROM_CFG_FW_MAJOR;  // Address = 4
    msg.data[2] = 0x00;
    
    handled = CAN_Config_ProcessMessage(&msg);
    if (handled) {
        passed++;
    }
    
    // Test 3: Read init stamp (should be 0xA5)
    total++;
    msg.data[1] = EEPROM_CFG_INIT_STAMP;  // Address = 7
    msg.data[2] = 0x00;
    
    handled = CAN_Config_ProcessMessage(&msg);
    if (handled) {
        passed++;
    }
    
    return passed;
}

/**
 * Test write request handling
 */
uint16_t CAN_Config_Test_WriteRequest(void) {
    uint16_t passed = 0;
    uint16_t total = 0;
    
    CAN_Message msg;
    
    // Test 1: Valid write request to serial number
    total++;
    msg.id = 0x18FF1080;  // Write Request ID
    msg.data[0] = 0x77;   // Guard byte
    msg.data[1] = EEPROM_CFG_SERIAL_NUMBER;  // Address = 22
    msg.data[2] = 0x00;   // Address MSB
    msg.data[3] = 0xAB;   // Value to write
    msg.data[4] = 0xFF;
    msg.data[5] = 0xFF;
    msg.data[6] = 0xFF;
    msg.data[7] = 0xFF;
    msg.dlc = 8;
    msg.valid = 1;
    
    uint8_t handled = CAN_Config_ProcessMessage(&msg);
    if (handled) {
        passed++;
    }
    
    // Verify the write
    total++;
    uint8_t verify = EEPROM_Config_ReadByte(EEPROM_CFG_SERIAL_NUMBER);
    if (verify == 0xAB) {
        passed++;
    }
    
    // Test 2: Write to rebroadcast mode
    total++;
    msg.data[1] = EEPROM_CFG_REBROADCAST_MODE;  // Address = 6
    msg.data[3] = 0x02;  // Change to periodic mode
    
    handled = CAN_Config_ProcessMessage(&msg);
    if (handled) {
        passed++;
    }
    
    // Verify
    total++;
    verify = EEPROM_Config_ReadByte(EEPROM_CFG_REBROADCAST_MODE);
    if (verify == 0x02) {
        passed++;
    }
    
    // Test 3: Write to heartbeat PGN (should trigger hot-reload)
    total++;
    msg.data[1] = EEPROM_CFG_HEARTBEAT_PGN_B;  // Address = 2
    msg.data[3] = 0x01;  // Change PGN low byte
    
    handled = CAN_Config_ProcessMessage(&msg);
    if (handled) {
        passed++;
    }
    
    return passed;
}

/**
 * Test guard byte validation
 */
uint16_t CAN_Config_Test_GuardByte(void) {
    uint16_t passed = 0;
    uint16_t total = 0;
    
    CAN_Message msg;
    
    // Test 1: Invalid guard byte on read request
    total++;
    msg.id = 0x18FF2080;  // Read Request ID
    msg.data[0] = 0x00;   // WRONG guard byte (should be 0x77)
    msg.data[1] = 0x00;   // Address LSB
    msg.data[2] = 0x00;   // Address MSB
    msg.data[3] = 0xFF;
    msg.data[4] = 0xFF;
    msg.data[5] = 0xFF;
    msg.data[6] = 0xFF;
    msg.data[7] = 0xFF;
    msg.dlc = 8;
    msg.valid = 1;
    
    uint16_t bad_guard_before = CAN_Config_GetBadGuardCount();
    CAN_Config_ProcessMessage(&msg);
    uint16_t bad_guard_after = CAN_Config_GetBadGuardCount();
    
    if (bad_guard_after > bad_guard_before) {
        passed++;
    }
    
    // Test 2: Invalid guard byte on write request
    total++;
    msg.id = 0x18FF1080;  // Write Request ID
    msg.data[0] = 0x55;   // WRONG guard byte
    msg.data[1] = EEPROM_CFG_SERIAL_NUMBER;
    msg.data[2] = 0x00;
    msg.data[3] = 0xCD;
    
    bad_guard_before = CAN_Config_GetBadGuardCount();
    CAN_Config_ProcessMessage(&msg);
    bad_guard_after = CAN_Config_GetBadGuardCount();
    
    if (bad_guard_after > bad_guard_before) {
        passed++;
    }
    
    // Test 3: Valid guard byte should pass
    total++;
    msg.data[0] = 0x77;   // CORRECT guard byte
    
    bad_guard_before = CAN_Config_GetBadGuardCount();
    CAN_Config_ProcessMessage(&msg);
    bad_guard_after = CAN_Config_GetBadGuardCount();
    
    if (bad_guard_after == bad_guard_before) {
        passed++;
    }
    
    return passed;
}

/**
 * Test address range validation
 */
uint16_t CAN_Config_Test_AddressRange(void) {
    uint16_t passed = 0;
    uint16_t total = 0;
    
    CAN_Message msg;
    
    // Test 1: Write to address beyond valid range
    total++;
    msg.id = 0x18FF1080;  // Write Request ID
    msg.data[0] = 0x77;   // Guard byte
    msg.data[1] = 0xFF;   // Address LSB = 0x1FF (beyond max write addr)
    msg.data[2] = 0x01;   // Address MSB
    msg.data[3] = 0x12;   // Value
    msg.data[4] = 0xFF;
    msg.data[5] = 0xFF;
    msg.data[6] = 0xFF;
    msg.data[7] = 0xFF;
    msg.dlc = 8;
    msg.valid = 1;
    
    uint16_t range_err_before = CAN_Config_GetAddrRangeErrorCount();
    CAN_Config_ProcessMessage(&msg);
    uint16_t range_err_after = CAN_Config_GetAddrRangeErrorCount();
    
    if (range_err_after > range_err_before) {
        passed++;
    }
    
    // Test 2: Read from very high address (beyond max read addr)
    total++;
    msg.id = 0x18FF2080;  // Read Request ID
    msg.data[1] = 0xFF;   // Address = 0xFFFF
    msg.data[2] = 0xFF;
    
    range_err_before = CAN_Config_GetAddrRangeErrorCount();
    CAN_Config_ProcessMessage(&msg);
    range_err_after = CAN_Config_GetAddrRangeErrorCount();
    
    if (range_err_after > range_err_before) {
        passed++;
    }
    
    // Test 3: Valid address should pass
    total++;
    msg.data[1] = 0x00;   // Address = 0
    msg.data[2] = 0x00;
    
    range_err_before = CAN_Config_GetAddrRangeErrorCount();
    CAN_Config_ProcessMessage(&msg);
    range_err_after = CAN_Config_GetAddrRangeErrorCount();
    
    if (range_err_after == range_err_before) {
        passed++;
    }
    
    return passed;
}

/**
 * Test hot-reload functionality
 */
uint16_t CAN_Config_Test_HotReload(void) {
    uint16_t passed = 0;
    uint16_t total = 0;
    
    // Test 1: Write to Write Request PGN and verify it's reloaded
    total++;
    uint16_t old_write_pgn = CAN_Config_GetWritePGN();
    
    // Write new PGN to EEPROM (change to 0xFF11)
    EEPROM_Config_WriteByte(EEPROM_CFG_WRITE_REQ_PGN_B, 0x11);
    
    // Manually call reload
    CAN_Config_Reload();
    
    uint16_t new_write_pgn = CAN_Config_GetWritePGN();
    if (new_write_pgn == 0xFF11 && new_write_pgn != old_write_pgn) {
        passed++;
    }
    
    // Restore original value
    EEPROM_Config_WriteByte(EEPROM_CFG_WRITE_REQ_PGN_B, 0x10);
    CAN_Config_Reload();
    
    // Test 2: Write to Response SA and verify reload
    total++;
    uint8_t old_response_sa = CAN_Config_GetResponseSA();
    
    EEPROM_Config_WriteByte(EEPROM_CFG_RESPONSE_SA, 0x81);
    CAN_Config_Reload();
    
    uint8_t new_response_sa = CAN_Config_GetResponseSA();
    if (new_response_sa == 0x81 && new_response_sa != old_response_sa) {
        passed++;
    }
    
    // Restore
    EEPROM_Config_WriteByte(EEPROM_CFG_RESPONSE_SA, 0x80);
    CAN_Config_Reload();
    
    return passed;
}

/**
 * Run all CAN configuration tests
 */
uint16_t CAN_Config_Test_RunAll(void) {
    uint16_t total_passed = 0;
    
    // Test 1: CAN ID Extraction
    uint16_t test1_passed = CAN_Config_Test_CANIDExtraction();
    total_passed += test1_passed;
    CAN_Config_Test_PrintResults("CAN ID Extraction", test1_passed, 7);
    
    // Test 2: Message Filtering
    uint16_t test2_passed = CAN_Config_Test_MessageFiltering();
    total_passed += test2_passed;
    CAN_Config_Test_PrintResults("Message Filtering", test2_passed, 6);
    
    // Test 3: Read Request
    uint16_t test3_passed = CAN_Config_Test_ReadRequest();
    total_passed += test3_passed;
    CAN_Config_Test_PrintResults("Read Request", test3_passed, 3);
    
    // Test 4: Write Request
    uint16_t test4_passed = CAN_Config_Test_WriteRequest();
    total_passed += test4_passed;
    CAN_Config_Test_PrintResults("Write Request", test4_passed, 5);
    
    // Test 5: Guard Byte
    uint16_t test5_passed = CAN_Config_Test_GuardByte();
    total_passed += test5_passed;
    CAN_Config_Test_PrintResults("Guard Byte", test5_passed, 3);
    
    // Test 6: Address Range
    uint16_t test6_passed = CAN_Config_Test_AddressRange();
    total_passed += test6_passed;
    CAN_Config_Test_PrintResults("Address Range", test6_passed, 3);
    
    // Test 7: Hot Reload
    uint16_t test7_passed = CAN_Config_Test_HotReload();
    total_passed += test7_passed;
    CAN_Config_Test_PrintResults("Hot Reload", test7_passed, 2);
    
    // Print overall results
    CAN_Config_Test_PrintResults("OVERALL", total_passed, 29);
    
    return total_passed;
}

/**
 * Print test results to debug output
 */
void CAN_Config_Test_PrintResults(const char *test_name, uint16_t passed, uint16_t total) {
    // This would typically print to UART or LCD
    // For now, just a placeholder
    
    // Example implementation if printf/UART available:
    // printf("%s: %u/%u tests passed\n", test_name, passed, total);
}