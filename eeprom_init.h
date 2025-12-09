/*
 * FILE: eeprom_init.h
 * EEPROM Initialization Functions - UPDATED FOR CONFIGURATION ALIGNMENT
 * 
 * Now includes proper configuration bytes matching IOX NGX structure:
 * - Byte 0-22: Configuration parameters (bitrate, PGNs, SAs, etc.)
 * - Byte 23-33: Reserved
 * - Byte 34+: Input cases (starting at word address 0x0022)
 */

#ifndef EEPROM_INIT_H
#define EEPROM_INIT_H

#include <xc.h>
#include <stdint.h>

/**
 * Check if EEPROM has been initialized
 * Looks for init stamp (0xA5) at byte address 7
 * @return 1 if initialized, 0 if blank
 */
uint8_t EEPROM_IsInitialized(void);

/**
 * Get number of write errors that occurred during initialization
 * @return Count of write errors (bounds check failures, verification failures)
 */
uint16_t EEPROM_GetWriteErrors(void);

/**
 * Get number of words successfully written to EEPROM
 * @return Count of words written
 */
uint16_t EEPROM_GetWordsWritten(void);

/**
 * Get last error type from write operation
 * @return 0=none, 1=WR timeout, 2=verify fail, 3=bounds error
 */
uint8_t EEPROM_GetLastErrorType(void);

/**
 * Get total number of write attempts (including failures)
 * @return Total write attempts
 */
uint16_t EEPROM_GetWriteAttempts(void);

/**
 * Write a single byte to EEPROM using the working boot-time write function
 * This function wraps the proven EEPROM_WriteWord function and is exposed
 * for runtime use by CAN configuration writes
 * 
 * @param byte_addr: Byte address to write (0-4095)
 * @param value: Byte value to write (0-255)
 * @return 1 on success, 0 on failure
 */
uint8_t EEPROM_Init_WriteByte(uint16_t byte_addr, uint8_t value);

/**
 * Configuration types for menu selection
 */
typedef enum {
    CONFIG_STD_FRONT_ENGINE = 0,
    CONFIG_STD_REAR_ENGINE = 1,
    CONFIG_CUSTOMER = 2
} eeprom_config_type_t;

/**
 * Initialize EEPROM with selected configuration
 */
void EEPROM_InitWithConfig(eeprom_config_type_t config_type);

/**
 * Select configuration through LCD menu
 * Returns the selected configuration type
 */
eeprom_config_type_t EEPROM_SelectConfiguration(void);

/**
 * Configuration loading functions for menu selection
 */
void EEPROM_LoadFrontEngine(void);
void EEPROM_LoadRearEngine(void);
void EEPROM_LoadCustomer(void);

/**
 * Helper functions used by configuration loaders
 * These are implemented in eeprom_init.c and used by the config files
 */
uint8_t EEPROM_WriteWord(uint16_t address, uint16_t data);
void EEPROM_WriteBytePair(uint16_t address, uint8_t lsb, uint8_t msb);
void EEPROM_WriteCase(uint16_t address, uint8_t priority, uint16_t pgn, 
                     uint8_t source_addr, uint8_t config_byte, 
                     uint8_t pattern_timing, uint8_t requires_ignition, 
                     uint8_t *data);
void EEPROM_WriteInvalidCase(uint16_t address);
void ParseCANID(const char *can_id_str, uint8_t *priority, uint16_t *pgn, uint8_t *sa);

#endif // EEPROM_INIT_H