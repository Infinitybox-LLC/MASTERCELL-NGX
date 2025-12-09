/*
 * FILE: can_config_test.h
 * Test Functions for CAN Configuration Protocol
 * 
 * Provides test functions to verify:
 * - CAN ID extraction (PGN and SA)
 * - Message filtering (read/write request detection)
 * - Read request handling
 * - Write request handling
 * - Response message formatting
 * - Guard byte validation
 * - Address range validation
 */

#ifndef CAN_CONFIG_TEST_H
#define CAN_CONFIG_TEST_H

#include <xc.h>
#include <stdint.h>

/**
 * Test CAN ID extraction functions
 * Verifies PGN and SA extraction from 29-bit CAN IDs
 * 
 * @return Number of tests passed
 */
uint16_t CAN_Config_Test_CANIDExtraction(void);

/**
 * Test message filtering
 * Verifies detection of read/write request messages
 * 
 * @return Number of tests passed
 */
uint16_t CAN_Config_Test_MessageFiltering(void);

/**
 * Test read request handling
 * Simulates read requests and verifies responses
 * 
 * @return Number of tests passed
 */
uint16_t CAN_Config_Test_ReadRequest(void);

/**
 * Test write request handling
 * Simulates write requests and verifies responses
 * 
 * @return Number of tests passed
 */
uint16_t CAN_Config_Test_WriteRequest(void);

/**
 * Test guard byte validation
 * Verifies that invalid guard bytes are rejected
 * 
 * @return Number of tests passed
 */
uint16_t CAN_Config_Test_GuardByte(void);

/**
 * Test address range validation
 * Verifies that out-of-range addresses are rejected
 * 
 * @return Number of tests passed
 */
uint16_t CAN_Config_Test_AddressRange(void);

/**
 * Test hot-reload functionality
 * Verifies that configuration updates are applied immediately
 * 
 * @return Number of tests passed
 */
uint16_t CAN_Config_Test_HotReload(void);

/**
 * Run all CAN configuration tests
 * 
 * @return Total number of tests passed
 */
uint16_t CAN_Config_Test_RunAll(void);

/**
 * Print test results to debug output
 * 
 * @param test_name Name of the test
 * @param passed Number of tests passed
 * @param total Total number of tests
 */
void CAN_Config_Test_PrintResults(const char *test_name, uint16_t passed, uint16_t total);

#endif // CAN_CONFIG_TEST_H