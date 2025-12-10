/*
 * FILE: climate.h
 * Climate Control Interface for MASTERCELL NGX
 * 
 * Receives J1939 CAN messages (PGN 0xFF99) and controls
 * MCP4341 quad digital potentiometer for Vintage Air Gen-IV/V systems.
 * 
 * Three analog outputs (0-5V) via op-amp buffered potentiometer wipers:
 * - Temperature control
 * - Fan speed control  
 * - Blend door position
 * 
 * Hardware:
 * - MCP4341-104E/ST (100kΩ, 7-bit, 129 steps)
 * - SPI2: RG6 (SCK), RG7 (SDI), RG8 (SDO)
 * - Control: RB3 (CS_), RB4 (WP_), RB2 (RESET_)
 */

#ifndef CLIMATE_H
#define CLIMATE_H

#include <xc.h>
#include <stdint.h>

// ============================================================================
// CAN MESSAGE CONFIGURATION
// ============================================================================

// Climate control CAN message
// CAN ID: 0x18FF9901 (Priority 6, PGN 0xFF99, SA 0x01)
#define CLIMATE_PGN             0xAF00
#define CLIMATE_SOURCE_ADDR     0x01

// ============================================================================
// MCP4341 WIPER ADDRESSES
// ============================================================================

// MCP4341 has 4 wipers with non-contiguous addresses
#define MCP4341_WIPER0          0x00    // Volatile Wiper 0
#define MCP4341_WIPER1          0x01    // Volatile Wiper 1
#define MCP4341_WIPER2          0x06    // Volatile Wiper 2
#define MCP4341_WIPER3          0x07    // Volatile Wiper 3

// Command codes (bits 3:2 of command byte)
#define MCP4341_CMD_WRITE       0x00    // Write data
#define MCP4341_CMD_INCREMENT   0x01    // Increment wiper
#define MCP4341_CMD_DECREMENT   0x02    // Decrement wiper
#define MCP4341_CMD_READ        0x03    // Read data

// Wiper value range
#define MCP4341_WIPER_MIN       0
#define MCP4341_WIPER_MAX       128     // 7-bit resolution (0-128)

// ============================================================================
// CLIMATE CHANNEL MAPPING
// ============================================================================
// Note: Only 3 of 4 MCP4341 wipers are routed to external pins via op-amps.
// Wiper 2 (address 0x06) appears to not be connected.
// Testing shows:
// - Wiper 0 output → Physical "Fan Speed" pin
// - Wiper 1 output → Physical "Blend" pin  
// - Wiper 3 output → Physical "Temperature" pin (middle connector pin)

#define CLIMATE_TEMP_WIPER      MCP4341_WIPER3  // Temperature → Wiper 3 (middle pin)
#define CLIMATE_FAN_WIPER       MCP4341_WIPER0  // Fan Speed → Wiper 0
#define CLIMATE_BLEND_WIPER     MCP4341_WIPER1  // Blend Position → Wiper 1
// Wiper 2 is not connected to external pin

// ============================================================================
// PIN DEFINITIONS
// ============================================================================

// SPI2 pins (directly connected to MCP4341)
#define DIGIPOT_SCK_TRIS        TRISGbits.TRISG6
#define DIGIPOT_SDI_TRIS        TRISGbits.TRISG7
#define DIGIPOT_SDO_TRIS        TRISGbits.TRISG8

// Control pins
#define DIGIPOT_CS_TRIS         TRISBbits.TRISB3
#define DIGIPOT_CS              LATBbits.LATB3      // Chip Select (active LOW)

#define DIGIPOT_WP_TRIS         TRISBbits.TRISB4
#define DIGIPOT_WP              LATBbits.LATB4      // Write Protect (HIGH = allow writes)

#define DIGIPOT_RESET_TRIS      TRISBbits.TRISB2
#define DIGIPOT_RESET           LATBbits.LATB2      // Reset (HIGH = normal operation)

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

/**
 * Initialize the climate control system
 * - Configures SPI2 peripheral
 * - Sets up control pins
 * - Resets MCP4341 and sets all wipers to 0 (0V output)
 */
void Climate_Init(void);

/**
 * Process an incoming climate control CAN message
 * Extracts temperature, fan, and blend values and updates potentiometer wipers
 * 
 * @param can_id Full 29-bit CAN ID
 * @param data Pointer to 8-byte data payload
 * @return 1 if message was processed, 0 if not a climate message
 */
uint8_t Climate_ProcessMessage(uint32_t can_id, uint8_t *data);

/**
 * Set a specific wiper to a value
 * 
 * @param wiper_addr Wiper address (MCP4341_WIPER0, WIPER1, WIPER2, WIPER3)
 * @param value Wiper value (0-128, where 0=0V, 128=5V)
 */
void Climate_SetWiper(uint8_t wiper_addr, uint8_t value);

/**
 * Set temperature output (convenience function)
 * @param value CAN value (0-15), automatically scaled to 0-128
 */
void Climate_SetTemperature(uint8_t value);

/**
 * Set fan speed output (convenience function)
 * @param value CAN value (0-15), automatically scaled to 0-128
 */
void Climate_SetFanSpeed(uint8_t value);

/**
 * Set blend position output (convenience function)
 * @param value CAN value (0-15), automatically scaled to 0-128
 */
void Climate_SetBlend(uint8_t value);

/**
 * Set all climate outputs to 0V (default/off state)
 */
void Climate_SetAllOff(void);

// ============================================================================
// DIAGNOSTIC FUNCTIONS
// ============================================================================

/**
 * Get current temperature wiper value
 * @return Current wiper value (0-128)
 */
uint8_t Climate_GetTemperature(void);

/**
 * Get current fan speed wiper value
 * @return Current wiper value (0-128)
 */
uint8_t Climate_GetFanSpeed(void);

/**
 * Get current blend position wiper value
 * @return Current wiper value (0-128)
 */
uint8_t Climate_GetBlend(void);

#endif // CLIMATE_H

