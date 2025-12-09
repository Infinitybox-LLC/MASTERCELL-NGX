/*
 * Multiplexed Input Handler for MASTERCELL NGX
 * Manages 44 inputs through 6 multiplexers
 * Includes software debouncing, ignition flag tracking, and one-button start
 */

#ifndef INPUTS_H
#define INPUTS_H

#include <xc.h>
#include <stdint.h>

// Debounce configuration
// Scan interval is 10ms, so DEBOUNCE_SCANS × 10ms = debounce time
// Default: 3 scans × 10ms = 30ms debounce
// Adjust this value to tune debounce time:
//   2 = 20ms, 3 = 30ms, 4 = 40ms, 5 = 50ms, etc.
#define DEBOUNCE_SCANS  3

// Multiplexer control pins (from Appendix 1)
#define MUX_EN_TRIS     TRISGbits.TRISG15
#define MUX_EN          LATGbits.LATG15

#define MUX_A0_TRIS     TRISGbits.TRISG12
#define MUX_A0          LATGbits.LATG12

#define MUX_A1_TRIS     TRISGbits.TRISG13
#define MUX_A1          LATGbits.LATG13

#define MUX_A2_TRIS     TRISGbits.TRISG14
#define MUX_A2          LATGbits.LATG14

// Multiplexer output pins (readings from each MUX)
#define MUX01MC_TRIS    TRISDbits.TRISD7
#define MUX01MC         PORTDbits.RD7

#define MUX02MC_TRIS    TRISDbits.TRISD9
#define MUX02MC         PORTDbits.RD9

#define MUX03MC_TRIS    TRISDbits.TRISD8
#define MUX03MC         PORTDbits.RD8

#define MUX04MC_TRIS    TRISBbits.TRISB14
#define MUX04MC         PORTBbits.RB14

#define MUX05MC_TRIS    TRISCbits.TRISC1
#define MUX05MC         PORTCbits.RC1

#define MUX06MC_TRIS    TRISCbits.TRISC2
#define MUX06MC         PORTCbits.RC2

// Input array indices (IN01 = index 0, IN38 = index 37, HSIN01 = index 38, etc.)
#define INPUT_COUNT     44

// Named input indices for convenience
#define IN01    0
#define IN02    1
#define IN03    2
#define IN04    3
#define IN05    4
#define IN06    5
#define IN07    6
#define IN08    7
#define IN09    8
#define IN10    9
#define IN11    10
#define IN12    11
#define IN13    12
#define IN14    13
#define IN15    14
#define IN16    15
#define IN17    16
#define IN18    17
#define IN19    18
#define IN20    19
#define IN21    20
#define IN22    21
#define IN23    22
#define IN24    23
#define IN25    24
#define IN26    25
#define IN27    26
#define IN28    27
#define IN29    28
#define IN30    29
#define IN31    30
#define IN32    31
#define IN33    32
#define IN34    33
#define IN35    34
#define IN36    35
#define IN37    36
#define IN38    37
#define HSIN01  38
#define HSIN02  39
#define HSIN03  40
#define HSIN04  41
#define HSIN05  42
#define HSIN06  43

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

/**
 * Initialize the input system
 * - Configures multiplexer control pins
 * - Enables multiplexers
 * - Sets up input reading pins
 * - Initializes debounce counters
 * - Initializes ignition flag
 * - Initializes one-button start state machines
 */
void Inputs_Init(void);

/**
 * Scan all inputs through the multiplexers
 * - Reads all 44 inputs with debouncing
 * - Updates input states when stable
 * - Handles one-button start state machines
 * - Updates ignition flag when ignition inputs change
 * 
 * This function should be called periodically (every 10ms recommended)
 */
void Inputs_Scan(void);

/**
 * Get the stable state of a specific input
 * @param input_num Input number (0-43)
 * @return 1 if input is active/on, 0 if inactive/off
 */
uint8_t Inputs_GetState(uint8_t input_num);

/**
 * Get the name string for an input
 * @param input_num Input number (0-43)
 * @return Pointer to string like "IN01" or "HSIN01"
 */
const char* Inputs_GetName(uint8_t input_num);

/**
 * Get the global ignition flag state
 * The ignition flag is set when:
 * - Any regular ignition input (byte 4 bits 0-1 = 0x01) is ON, OR
 * - Any one-button start input has turned on the ignition
 * 
 * @return 1 if ignition is on, 0 if ignition is off
 */
uint8_t Inputs_GetIgnitionState(void);

/**
 * Update the ignition flag based on current input states and EEPROM configuration
 * This checks all regular ignition inputs (byte 4 bits 0-1 = 0x01, NOT one-button start)
 * and sets the ignition flag if ANY are ON, clears if ALL are OFF
 * 
 * NOTE: This does NOT check one-button start inputs, as they manage the
 * ignition flag directly in their state machine.
 */
/**
 * Update the ignition flag based on all ignition inputs
 * Returns 1 if ignition flag changed (requiring CAN transmission), 0 if unchanged
 */
uint8_t Inputs_UpdateIgnitionFlag(void);

/**
 * Initialize the ignition flag on system startup
 * Call this after EEPROM is loaded and input scanning has started
 */
void Inputs_InitIgnitionFlag(void);

/**
 * Check if an input is configured as a one-button start input
 * This is a wrapper for EEPROM_IsOneButtonStartInput()
 * 
 * @param input_num Input number (0-43)
 * @return 1 if configured as one-button start, 0 otherwise
 */
uint8_t Inputs_IsOneButtonStartInput(uint8_t input_num);

/**
 * Check if one-button start state changed (requires CAN transmission)
 * This function returns 1 if the state changed and automatically clears the flag
 * Call this after Inputs_Scan() and transmit messages if it returns 1
 * 
 * @return 1 if state changed and transmission needed, 0 otherwise
 */
uint8_t Inputs_OneButtonStartStateChanged(void);

#endif // INPUTS_H