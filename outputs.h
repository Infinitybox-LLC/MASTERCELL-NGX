/*
 * FILE: outputs.h
 * MOSFET Output Control Interface for MASTERCELL NGX
 * 
 * Receives J1939 CAN messages (PGN 0xAF00 from inLINK) and controls
 * 8 MOSFET gate outputs based on byte 3 of the message.
 * 
 * Byte 3 bit mapping:
 *   Bit 0 → Output 1 (RB15)
 *   Bit 1 → Output 2 (RF2)
 *   Bit 2 → Output 3 (RF3)
 *   Bit 3 → Output 4 (RF4)
 *   Bit 4 → Output 5 (RF5)
 *   Bit 5 → Output 6 (RF6)
 *   Bit 6 → Output 7 (RG2)
 *   Bit 7 → Output 8 (RG3)
 */

#ifndef OUTPUTS_H
#define OUTPUTS_H

#include <xc.h>
#include <stdint.h>

// ============================================================================
// CAN MESSAGE CONFIGURATION
// ============================================================================

// Output control CAN message (from inLINK, broadcast 1Hz)
// Shares PGN with climate control - byte 3 contains output states
#define OUTPUTS_PGN             0xAF00
#define OUTPUTS_LOCAL_PGN       0xFF00  // From EEPROM cases (internal/local outputs)
#define OUTPUTS_SOURCE_ADDR     0x01    // Update to match inLINK source address

// Data byte position for output states
#define OUTPUTS_DATA_BYTE       3

// ============================================================================
// PIN DEFINITIONS - MOSFET GATE DRIVERS
// ============================================================================

// Output 1 - RB15
#define OUTPUT1_TRIS            TRISBbits.TRISB15
#define OUTPUT1_LAT             LATBbits.LATB15

// Output 2 - RF2
#define OUTPUT2_TRIS            TRISFbits.TRISF2
#define OUTPUT2_LAT             LATFbits.LATF2

// Output 3 - RF3
#define OUTPUT3_TRIS            TRISFbits.TRISF3
#define OUTPUT3_LAT             LATFbits.LATF3

// Output 4 - RF4
#define OUTPUT4_TRIS            TRISFbits.TRISF4
#define OUTPUT4_LAT             LATFbits.LATF4

// Output 5 - RF5
#define OUTPUT5_TRIS            TRISFbits.TRISF5
#define OUTPUT5_LAT             LATFbits.LATF5

// Output 6 - RF6
#define OUTPUT6_TRIS            TRISFbits.TRISF6
#define OUTPUT6_LAT             LATFbits.LATF6

// Output 7 - RG2
#define OUTPUT7_TRIS            TRISGbits.TRISG2
#define OUTPUT7_LAT             LATGbits.LATG2

// Output 8 - RG3
#define OUTPUT8_TRIS            TRISGbits.TRISG3
#define OUTPUT8_LAT             LATGbits.LATG3

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

/**
 * Initialize the MOSFET output control system
 * - Configures all 8 gate pins as outputs
 * - Sets all outputs to OFF (LOW)
 */
void Outputs_Init(void);

/**
 * Process an incoming output control CAN message
 * Extracts byte 3 and updates MOSFET output states
 * 
 * @param can_id Full 29-bit CAN ID
 * @param data Pointer to 8-byte data payload
 * @return 1 if message was processed, 0 if not an output control message
 */
uint8_t Outputs_ProcessMessage(uint32_t can_id, uint8_t *data);

/**
 * Set all outputs from a single byte (bits 0-7 → outputs 1-8)
 * @param states Byte where each bit controls one output
 */
void Outputs_SetAll(uint8_t states);

/**
 * Set a specific output state
 * @param output Output number (1-8)
 * @param state 0 = OFF, 1 = ON
 */
void Outputs_Set(uint8_t output, uint8_t state);

/**
 * Get current state of all outputs as a byte
 * @return Byte where each bit represents one output state
 */
uint8_t Outputs_GetAll(void);

/**
 * Get state of a specific output
 * @param output Output number (1-8)
 * @return 0 = OFF, 1 = ON
 */
uint8_t Outputs_Get(uint8_t output);

/**
 * Turn all outputs OFF
 */
void Outputs_AllOff(void);

// ============================================================================
// HARDCODED INPUT-TO-OUTPUT FUNCTIONS
// ============================================================================

/**
 * Update hardcoded outputs (OUT1-OUT6) based on input states
 * Call this from the main loop when inputs are scanned
 * 
 * Mappings:
 *   OUT1 ← IN03 (Left Turn) - uses pattern timing
 *   OUT2 ← IN04 (Right Turn) - uses pattern timing
 *   OUT3 ← IN07 (High Beams) - steady
 *   OUT4 ← IN06 (Parking Lights) - steady
 *   OUT5 ← IN01 (Ignition) - steady
 *   OUT6 ← Security (software controlled)
 */
void Outputs_UpdateFromInputs(void);

/**
 * Process pattern timing for turn signal outputs (OUT1/OUT2)
 * Call this every 250ms from the pattern timer
 */
void Outputs_PatternTick(void);

/**
 * Set the security output state (OUT6)
 * @param state 0 = OFF, 1 = ON
 */
void Outputs_SetSecurity(uint8_t state);

/**
 * Get the security output state
 * @return 0 = OFF, 1 = ON
 */
uint8_t Outputs_GetSecurity(void);

#endif // OUTPUTS_H

