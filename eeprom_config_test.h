/*
 * FILE: eeprom_config_test.h
 * Test Functions for Byte-Level EEPROM Configuration Access
 * 
 * Provides test functions to verify:
 * - Byte read/write at all configuration addresses (0-22)
 * - Word boundary handling (odd/even addresses)
 * - Read-modify-write correctness
 * - Multi-byte read/write operations
 */

#ifndef EEPROM_CONFIG_TEST_H
#define EEPROM_CONFIG_TEST_H

#include <xc.h>
#include <stdint.h>

/**
 * Test basic byte read/write operations
 * Writes and reads back test patterns to all configuration addresses
 * 
 * @return Number of tests passed
 */
uint16_t EEPROM_Config_Test_BasicReadWrite(void);

/**
 * Test word boundary handling
 * Verifies that writing to odd/even addresses correctly modifies
 * only the target byte without affecting the adjacent byte
 * 
 * @return Number of tests passed
 */
uint16_t EEPROM_Config_Test_WordBoundary(void);

/**
 * Test multi-byte read/write operations
 * 
 * @return Number of tests passed
 */
uint16_t EEPROM_Config_Test_MultiByteAccess(void);

/**
 * Test PGN read/write helper functions
 * 
 * @return Number of tests passed
 */
uint16_t EEPROM_Config_Test_PGNAccess(void);

/**
 * Run all EEPROM configuration tests
 * 
 * @return Total number of tests passed
 */
uint16_t EEPROM_Config_Test_RunAll(void);

/**
 * Print test results to debug output
 * 
 * @param test_name Name of the test
 * @param passed Number of tests passed
 * @param total Total number of tests
 */
void EEPROM_Config_Test_PrintResults(const char *test_name, uint16_t passed, uint16_t total);

#endif // EEPROM_CONFIG_TEST_H