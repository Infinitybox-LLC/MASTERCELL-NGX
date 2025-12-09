/*
 * FILE: eeprom_config_test.c
 * Test Implementation for Byte-Level EEPROM Configuration Access
 */

#include "eeprom_config_test.h"
#include "eeprom_config.h"
#include <stdio.h>

/**
 * Test basic byte read/write operations
 */
uint16_t EEPROM_Config_Test_BasicReadWrite(void) {
    uint16_t passed = 0;
    uint16_t total = 0;
    
    // Test pattern: incrementing values
    const uint8_t test_pattern[] = {
        0x01, 0xFF, 0x00, 0x80,  // Addresses 0-3
        0x01, 0x00, 0x01, 0xA5,  // Addresses 4-7
        0xFF, 0xFF, 0xFF, 0x10,  // Addresses 8-11
        0x80, 0xFF, 0x20, 0x80,  // Addresses 12-15
        0xFF, 0x30, 0x80, 0xFF,  // Addresses 16-19
        0x40, 0x80, 0x42         // Addresses 20-22
    };
    
    // Write test pattern to all configuration addresses
    for (uint16_t addr = 0; addr < EEPROM_CFG_SIZE; addr++) {
        total++;
        if (EEPROM_Config_WriteByte(addr, test_pattern[addr])) {
            passed++;
        }
    }
    
    // Read back and verify
    for (uint16_t addr = 0; addr < EEPROM_CFG_SIZE; addr++) {
        total++;
        uint8_t value = EEPROM_Config_ReadByte(addr);
        if (value == test_pattern[addr]) {
            passed++;
        }
    }
    
    return passed;
}

/**
 * Test word boundary handling
 * Verify that writing to one byte doesn't affect the adjacent byte
 */
uint16_t EEPROM_Config_Test_WordBoundary(void) {
    uint16_t passed = 0;
    uint16_t total = 0;
    
    // Test at word address 0x0000 (bytes 0 and 1)
    // Write to byte 0 (LSB), verify byte 1 (MSB) unchanged
    total++;
    EEPROM_Config_WriteByte(0, 0xAA);
    EEPROM_Config_WriteByte(1, 0x55);
    
    uint8_t byte0 = EEPROM_Config_ReadByte(0);
    uint8_t byte1 = EEPROM_Config_ReadByte(1);
    
    if (byte0 == 0xAA && byte1 == 0x55) {
        passed++;
    }
    
    // Modify byte 0, verify byte 1 unchanged
    total++;
    EEPROM_Config_WriteByte(0, 0xCC);
    byte0 = EEPROM_Config_ReadByte(0);
    byte1 = EEPROM_Config_ReadByte(1);
    
    if (byte0 == 0xCC && byte1 == 0x55) {
        passed++;
    }
    
    // Modify byte 1, verify byte 0 unchanged
    total++;
    EEPROM_Config_WriteByte(1, 0xDD);
    byte0 = EEPROM_Config_ReadByte(0);
    byte1 = EEPROM_Config_ReadByte(1);
    
    if (byte0 == 0xCC && byte1 == 0xDD) {
        passed++;
    }
    
    // Test at word address 0x0002 (bytes 2 and 3)
    total++;
    EEPROM_Config_WriteByte(2, 0x11);
    EEPROM_Config_WriteByte(3, 0x22);
    
    uint8_t byte2 = EEPROM_Config_ReadByte(2);
    uint8_t byte3 = EEPROM_Config_ReadByte(3);
    
    if (byte2 == 0x11 && byte3 == 0x22) {
        passed++;
    }
    
    // Modify byte 3, verify byte 2 unchanged
    total++;
    EEPROM_Config_WriteByte(3, 0x33);
    byte2 = EEPROM_Config_ReadByte(2);
    byte3 = EEPROM_Config_ReadByte(3);
    
    if (byte2 == 0x11 && byte3 == 0x33) {
        passed++;
    }
    
    return passed;
}

/**
 * Test multi-byte read/write operations
 */
uint16_t EEPROM_Config_Test_MultiByteAccess(void) {
    uint16_t passed = 0;
    uint16_t total = 0;
    
    // Test writing 8 bytes starting at address 0
    const uint8_t write_buffer[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
    uint8_t read_buffer[8];
    
    total++;
    uint16_t bytes_written = EEPROM_Config_WriteBytes(0, write_buffer, 8);
    if (bytes_written == 8) {
        passed++;
    }
    
    total++;
    uint16_t bytes_read = EEPROM_Config_ReadBytes(0, read_buffer, 8);
    if (bytes_read == 8) {
        passed++;
    }
    
    // Verify all bytes match
    total++;
    uint8_t all_match = 1;
    for (uint8_t i = 0; i < 8; i++) {
        if (read_buffer[i] != write_buffer[i]) {
            all_match = 0;
            break;
        }
    }
    if (all_match) {
        passed++;
    }
    
    // Test writing to odd starting address
    const uint8_t write_buffer2[] = {0xAA, 0xBB, 0xCC};
    uint8_t read_buffer2[3];
    
    total++;
    bytes_written = EEPROM_Config_WriteBytes(5, write_buffer2, 3);
    if (bytes_written == 3) {
        passed++;
    }
    
    total++;
    bytes_read = EEPROM_Config_ReadBytes(5, read_buffer2, 3);
    if (bytes_read == 3) {
        passed++;
    }
    
    total++;
    all_match = 1;
    for (uint8_t i = 0; i < 3; i++) {
        if (read_buffer2[i] != write_buffer2[i]) {
            all_match = 0;
            break;
        }
    }
    if (all_match) {
        passed++;
    }
    
    return passed;
}

/**
 * Test PGN read/write helper functions
 */
uint16_t EEPROM_Config_Test_PGNAccess(void) {
    uint16_t passed = 0;
    uint16_t total = 0;
    
    // Test writing Heartbeat PGN (0xFF00) at addresses 1-2
    total++;
    if (EEPROM_Config_WritePGN(EEPROM_CFG_HEARTBEAT_PGN_A, 0xFF00)) {
        passed++;
    }
    
    total++;
    uint16_t pgn = EEPROM_Config_ReadPGN(EEPROM_CFG_HEARTBEAT_PGN_A);
    if (pgn == 0xFF00) {
        passed++;
    }
    
    // Verify individual bytes
    total++;
    uint8_t pgn_a = EEPROM_Config_ReadByte(EEPROM_CFG_HEARTBEAT_PGN_A);
    uint8_t pgn_b = EEPROM_Config_ReadByte(EEPROM_CFG_HEARTBEAT_PGN_B);
    if (pgn_a == 0xFF && pgn_b == 0x00) {
        passed++;
    }
    
    // Test writing Write Request PGN (0xFF10) at addresses 10-11
    total++;
    if (EEPROM_Config_WritePGN(EEPROM_CFG_WRITE_REQ_PGN_A, 0xFF10)) {
        passed++;
    }
    
    total++;
    pgn = EEPROM_Config_ReadPGN(EEPROM_CFG_WRITE_REQ_PGN_A);
    if (pgn == 0xFF10) {
        passed++;
    }
    
    // Test writing Read Request PGN (0xFF20) at addresses 13-14
    total++;
    if (EEPROM_Config_WritePGN(EEPROM_CFG_READ_REQ_PGN_A, 0xFF20)) {
        passed++;
    }
    
    total++;
    pgn = EEPROM_Config_ReadPGN(EEPROM_CFG_READ_REQ_PGN_A);
    if (pgn == 0xFF20) {
        passed++;
    }
    
    // Test writing Response PGN (0xFF30) at addresses 16-17
    total++;
    if (EEPROM_Config_WritePGN(EEPROM_CFG_RESPONSE_PGN_A, 0xFF30)) {
        passed++;
    }
    
    total++;
    pgn = EEPROM_Config_ReadPGN(EEPROM_CFG_RESPONSE_PGN_A);
    if (pgn == 0xFF30) {
        passed++;
    }
    
    // Test writing Diagnostic PGN (0xFF40) at addresses 19-20
    total++;
    if (EEPROM_Config_WritePGN(EEPROM_CFG_DIAGNOSTIC_PGN_A, 0xFF40)) {
        passed++;
    }
    
    total++;
    pgn = EEPROM_Config_ReadPGN(EEPROM_CFG_DIAGNOSTIC_PGN_A);
    if (pgn == 0xFF40) {
        passed++;
    }
    
    return passed;
}

/**
 * Run all EEPROM configuration tests
 */
uint16_t EEPROM_Config_Test_RunAll(void) {
    uint16_t total_passed = 0;
    
    // Test 1: Basic Read/Write
    uint16_t test1_passed = EEPROM_Config_Test_BasicReadWrite();
    total_passed += test1_passed;
    EEPROM_Config_Test_PrintResults("Basic Read/Write", test1_passed, 46);
    
    // Test 2: Word Boundary
    uint16_t test2_passed = EEPROM_Config_Test_WordBoundary();
    total_passed += test2_passed;
    EEPROM_Config_Test_PrintResults("Word Boundary", test2_passed, 5);
    
    // Test 3: Multi-Byte Access
    uint16_t test3_passed = EEPROM_Config_Test_MultiByteAccess();
    total_passed += test3_passed;
    EEPROM_Config_Test_PrintResults("Multi-Byte Access", test3_passed, 6);
    
    // Test 4: PGN Access
    uint16_t test4_passed = EEPROM_Config_Test_PGNAccess();
    total_passed += test4_passed;
    EEPROM_Config_Test_PrintResults("PGN Access", test4_passed, 10);
    
    // Print overall results
    EEPROM_Config_Test_PrintResults("OVERALL", total_passed, 67);
    
    return total_passed;
}

/**
 * Print test results to debug output
 */
void EEPROM_Config_Test_PrintResults(const char *test_name, uint16_t passed, uint16_t total) {
    // This would typically print to UART or LCD
    // For now, just a placeholder that could be implemented based on available hardware
    
    // Example implementation if printf/UART available:
    // printf("%s: %u/%u tests passed\n", test_name, passed, total);
    
    // Could also toggle LED patterns to indicate pass/fail
    // Or store results in RAM for later retrieval
}