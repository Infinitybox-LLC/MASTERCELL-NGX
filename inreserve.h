/*
 * FILE: inreserve.h
 * inRESERVE Battery Disconnect Feature for MASTERCELL NGX
 * 
 * Monitors PowerCell voltage and activates a latching solenoid output
 * to disconnect the battery after sustained low voltage condition.
 * 
 * Configuration stored in EEPROM (2 bytes):
 *   Byte 1: [XXXX][YYYY]
 *     XXXX = PowerCell ID (0=disabled, 1=Front, 2=Rear, 3+)
 *     YYYY = Output number (1-10)
 *   Byte 2: [ZZZZ][QQQQ]
 *     ZZZZ = Time threshold (0=5min, 1=10min, 2=15min, 3=20min)
 *     QQQQ = Voltage threshold (0=11.9V, 1=12.0V, ... 0xB=13.0V)
 */

#ifndef INRESERVE_H
#define INRESERVE_H

#include <xc.h>
#include <stdint.h>

// ============================================================================
// CONFIGURATION ENCODING
// ============================================================================

// PowerCell ID encoding (upper nibble of byte 1)
#define INRESERVE_CELL_DISABLED     0x0
#define INRESERVE_CELL_FRONT        0x1
#define INRESERVE_CELL_REAR         0x2
// 0x3+ = PowerCell 3, 4, etc.

// Time threshold encoding (upper nibble of byte 2)
// 0=30sec (dev), 1=15min, 2=20min
#define INRESERVE_TIME_30SEC        0x0  // Dev/test mode
#define INRESERVE_TIME_15MIN        0x1
#define INRESERVE_TIME_20MIN        0x2

// Voltage threshold encoding (lower nibble of byte 2)
// 0=12.1V, 1=12.2V, 2=12.3V
#define INRESERVE_VOLTAGE_12_1V     0x0
#define INRESERVE_VOLTAGE_12_2V     0x1
#define INRESERVE_VOLTAGE_12_3V     0x2

// ============================================================================
// CONFIGURATION STRUCTURE
// ============================================================================

typedef struct {
    uint8_t enabled;            // 1 if enabled (cell_id != 0), 0 if disabled
    uint8_t cell_id;            // PowerCell ID (0=disabled, 1=Front, 2=Rear, 3+)
    uint8_t output;             // Output number (1-10)
    uint8_t time_code;          // Time threshold code (0-4)
    uint8_t voltage_code;       // Voltage threshold code (0-11)
    uint32_t time_seconds;      // Calculated time in seconds
    uint16_t voltage_mv;        // Calculated voltage in millivolts
} InReserveConfig;

// ============================================================================
// STATE STRUCTURE
// ============================================================================

typedef struct {
    uint8_t timer_active;       // 1 if countdown timer is running
    uint32_t timer_start_ms;    // System time when timer started
    uint8_t triggered;          // 1 if output has been activated
    uint16_t last_voltage_mv;   // Last read voltage in millivolts
} InReserveState;

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

/**
 * Initialize inRESERVE module
 * Loads configuration from EEPROM
 */
void InReserve_Init(void);

/**
 * Load configuration from EEPROM
 */
void InReserve_LoadConfig(void);

/**
 * Save configuration to EEPROM
 */
void InReserve_SaveConfig(void);

/**
 * Get current configuration
 * @return Pointer to current configuration structure
 */
InReserveConfig* InReserve_GetConfig(void);

/**
 * Get current state
 * @return Pointer to current state structure
 */
InReserveState* InReserve_GetState(void);

/**
 * Update inRESERVE state - call periodically from main loop
 * Monitors voltage and manages timer
 * @param current_voltage_mv Current PowerCell voltage in millivolts
 */
void InReserve_Update(uint16_t current_voltage_mv);

/**
 * Reset the inRESERVE trigger (after battery reconnect)
 */
void InReserve_Reset(void);

/**
 * Set PowerCell ID
 * @param cell_id 0=disabled, 1=Front, 2=Rear, 3+
 */
void InReserve_SetCellID(uint8_t cell_id);

/**
 * Set output number
 * @param output Output number 1-10
 */
void InReserve_SetOutput(uint8_t output);

/**
 * Set time threshold
 * @param time_code 0=5min, 1=10min, 2=15min, 3=20min
 */
void InReserve_SetTime(uint8_t time_code);

/**
 * Set voltage threshold
 * @param voltage_code 0=11.9V, 1=12.0V, ... 11=13.0V
 */
void InReserve_SetVoltage(uint8_t voltage_code);

/**
 * Get PowerCell name string
 * @param cell_id PowerCell ID
 * @return String like "Front", "Rear", "3", etc.
 */
const char* InReserve_GetCellName(uint8_t cell_id);

/**
 * Get time string
 * @param time_code Time code
 * @return String like "5 min", "10 min", etc.
 */
const char* InReserve_GetTimeString(uint8_t time_code);

/**
 * Get voltage as string (in format "12.3V")
 * @param voltage_code Voltage code
 * @param buffer Buffer to write string to (at least 6 chars)
 */
void InReserve_GetVoltageString(uint8_t voltage_code, char* buffer);

/**
 * Get minimum valid output for a PowerCell
 * @param cell_id PowerCell ID (1=Front, 2=Rear, 3-6=others)
 * @return Minimum output number (1-10)
 */
uint8_t InReserve_GetMinOutput(uint8_t cell_id);

/**
 * Get maximum valid output for a PowerCell
 * @param cell_id PowerCell ID (1=Front, 2=Rear, 3-6=others)
 * @return Maximum output number (1-10)
 */
uint8_t InReserve_GetMaxOutput(uint8_t cell_id);

/**
 * Get number of valid outputs for a PowerCell
 * @param cell_id PowerCell ID
 * @return Number of valid output options
 */
uint8_t InReserve_GetOutputCount(uint8_t cell_id);

#endif // INRESERVE_H
