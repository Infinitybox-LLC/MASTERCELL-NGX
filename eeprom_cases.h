/*
 * FILE: eeprom_cases.h
 * EEPROM Case Management for MASTERCELL NGX
 * Handles reading input cases and aggregating CAN messages
 * WITH PATTERN TIMING SUPPORT, ONE-BUTTON START, AND TRACK IGNITION
 */

#ifndef EEPROM_CASES_H
#define EEPROM_CASES_H

#include <xc.h>
#include <stdint.h>

// EEPROM Memory Map
#define EEPROM_SETUP_BYTES_START    0x0000
#define EEPROM_SETUP_BYTES_SIZE     23
#define EEPROM_RESERVED_START       0x0017
#define EEPROM_RESERVED_SIZE        10
#define EEPROM_CASES_START          0x0022  // Must be word-aligned (even address)

// Case structure size
#define CASE_SIZE                   32

// Case data offsets within 32-byte structure
#define CASE_OFFSET_PRIORITY        0
#define CASE_OFFSET_PGN_HIGH        1
#define CASE_OFFSET_PGN_LOW         2
#define CASE_OFFSET_SOURCE_ADDR     3
#define CASE_OFFSET_CONFIG          4   // Byte 4: Configuration byte with track ignition bits
#define CASE_OFFSET_PATTERN_TIMING  7   // Byte 7: Upper nibble = ON time, Lower nibble = OFF time
#define CASE_OFFSET_DATA_START      24  // CAN data is in bytes 24-31
#define CASE_DATA_SIZE              8

// Byte 4 bit masks (configuration byte)
#define CONFIG_CAN_BE_OVERRIDDEN_MASK   0x0C  // Bits 2-3: Can be overridden by pattern cases
#define CONFIG_CAN_BE_OVERRIDDEN_VALUE  0x04  // Bits 2-3 = 01 for can be overridden
#define CONFIG_ONE_BUTTON_MASK      0x30  // Bits 4-5: One-button start mode
#define CONFIG_ONE_BUTTON_VALUE     0x10  // Bits 4-5 = 01 for one-button start
#define CONFIG_TRACK_IGNITION_MASK  0xC0  // Bits 6-7: Track ignition mode
#define CONFIG_TRACK_IGNITION_VALUE 0x40  // Bits 6-7 = 01 for track ignition

// Pattern states for inputs
#define PATTERN_STATE_INACTIVE      0   // Input is off, no pattern running
#define PATTERN_STATE_ON_PHASE      1   // Pattern is in ON phase (transmitting data)
#define PATTERN_STATE_OFF_PHASE     2   // Pattern is in OFF phase (not transmitting)

// Maximum number of cases per input
#define MAX_ON_CASES_PER_INPUT      8
#define MAX_OFF_CASES_PER_INPUT     2

// Total inputs
#define TOTAL_INPUTS                44

// Case count lookup tables (hard-coded from EEPROM structure)
extern const uint8_t input_on_case_count[TOTAL_INPUTS];
extern const uint8_t input_off_case_count[TOTAL_INPUTS];

// Structure to hold a parsed case from EEPROM
typedef struct {
    uint8_t priority;
    uint16_t pgn;           // Combined PGN (PGN_HIGH << 8 | PGN_LOW)
    uint8_t source_addr;
    uint8_t data[8];        // CAN payload data
    uint8_t pattern_on_time;   // Pattern ON time in 250ms counts (0-15)
    uint8_t pattern_off_time;  // Pattern OFF time in 250ms counts (0-15)
    uint8_t must_be_on[8];     // Bytes 8-15 (zero-indexed): Conditional logic - inputs that must be ON
    uint8_t must_be_off[8];    // Bytes 16-23 (zero-indexed): Conditional logic - inputs that must be OFF
    uint8_t valid;          // 1 = valid case, 0 = invalid (all 0xFF)
    uint8_t can_be_overridden; // 1 = can be overridden by flashing pattern cases (single filament brake)
} CaseData;

// Structure to track an active input case
typedef struct {
    uint8_t input_num;      // Which input (0-43)
    uint8_t case_num;       // Which case for this input
    uint8_t is_on_case;     // 1 = ON case, 0 = OFF case
    uint8_t needs_removal_after_send;  // 1 = remove after first transmission (for OFF/clearing cases)
    CaseData case_data;     // The parsed case data
} ActiveCase;

// Pattern timer structure for each input
typedef struct {
    uint8_t state;              // PATTERN_STATE_INACTIVE, ON_PHASE, or OFF_PHASE
    uint8_t timer;              // Current countdown timer (in 250ms ticks)
    uint8_t on_time;            // ON duration to reload (in 250ms ticks)
    uint8_t off_time;           // OFF duration to reload (in 250ms ticks)
    uint8_t has_pattern;        // 1 if this input has at least one pattern case active
} PatternTimer;

// Maximum active cases at once
// Allows all 44 inputs to be active simultaneously
// Each case is ~16 bytes, so 44 cases = ~704 bytes (safe for 2KB RAM)
#define MAX_ACTIVE_CASES    64

// Structure to hold aggregated message
typedef struct {
    uint8_t priority;
    uint16_t pgn;
    uint8_t source_addr;
    uint8_t data[8];        // OR'd result of all active cases
    uint8_t valid;          // 1 = message ready to send
    uint8_t has_pattern;    // 1 if ANY contributing case has pattern timing (PHASE 1)
    uint8_t data_changed;   // 1 if data differs from previous broadcast (PHASE 1)
} AggregatedMessage;

// Maximum unique PGN/SA combinations
// Increased to handle more EEPROM cases + inLINK messages
#define MAX_UNIQUE_MESSAGES 24

// Function prototypes

/**
 * Initialize the EEPROM case system
 */
void EEPROM_Cases_Init(void);

/**
 * Calculate EEPROM address for a specific input case
 * @param input_num Input number (0-43)
 * @param case_num Case number for this input
 * @param is_on_case 1 for ON case, 0 for OFF case
 * @return EEPROM address of the case, or 0xFFFF if invalid
 */
uint16_t EEPROM_GetCaseAddress(uint8_t input_num, uint8_t case_num, uint8_t is_on_case);

/**
 * Read a case from EEPROM with comprehensive bounds checking
 * @param address EEPROM address of the case
 * @param case_data Pointer to CaseData structure to fill
 * @return 1 if valid case, 0 if invalid (all 0xFF)
 */
uint8_t EEPROM_ReadCase(uint16_t address, CaseData *case_data);

/**
 * Handle input state change
 * @param input_num Input number (0-43)
 * @param new_state New state (1 = ON/active/low, 0 = OFF/inactive/high)
 */
void EEPROM_HandleInputChange(uint8_t input_num, uint8_t new_state);

/**
 * Get aggregated messages ready to transmit
 * @param messages Array to hold aggregated messages
 * @param max_messages Maximum number of messages array can hold
 * @return Number of messages to transmit
 */
uint8_t EEPROM_GetAggregatedMessages(AggregatedMessage *messages, uint8_t max_messages);

/**
 * Clear all active cases (for initialization)
 */
void EEPROM_ClearActiveCases(void);

/**
 * Remove all active cases marked with needs_removal_after_send flag
 * Called after transmission to clean up one-shot OFF/clearing cases
 */
void EEPROM_RemoveMarkedCases(void);

/**
 * Get diagnostic information - number of EEPROM reads performed
 * @return Total EEPROM read operations since init
 */
uint16_t EEPROM_GetReadCount(void);

/**
 * Get diagnostic information - number of bounds check failures
 * @return Total bounds errors detected
 */
uint16_t EEPROM_GetBoundsErrors(void);

/**
 * Get current number of active cases
 * @return Number of cases currently in active list
 */
uint8_t EEPROM_GetActiveCaseCount(void);

/**
 * Pattern timing functions - called from 250ms timer interrupt
 */

/**
 * Update pattern timers (called every 250ms from timer interrupt)
 * This is non-blocking and executes quickly
 */
void EEPROM_Pattern_UpdateTimers(void);

/**
 * Check if an input's pattern is currently in ON phase
 * @param input_num Input number (0-43)
 * @return 1 if in ON phase, 0 if in OFF phase or inactive
 */
uint8_t EEPROM_Pattern_IsInOnPhase(uint8_t input_num);

// ============================================================================
// ONE-BUTTON START MANUAL CASE CONTROL FUNCTIONS
// ============================================================================

/**
 * Set a manual case for one-button start control
 * This allows direct control of what data is broadcast, independent of physical input state
 * 
 * This function:
 * 1. Removes any existing active cases for this input
 * 2. Reads the case configuration from EEPROM (PGN, SA, priority)
 * 3. Modifies the CAN data bytes based on ignition_on/starter_on parameters
 * 4. Adds the modified case to the active cases list
 * 
 * The case will remain active until cleared with EEPROM_ClearManualCase() or
 * updated with another call to EEPROM_SetManualCase()
 * 
 * @param input_num Input number (0-43) - must be configured as one-button start
 * @param ignition_on 1 to set ignition bit, 0 to clear it
 * @param starter_on 1 to set starter bit, 0 to clear it
 * @return 1 if successful, 0 if failed (invalid input, EEPROM read error, etc.)
 */
uint8_t EEPROM_SetManualCase(uint8_t input_num, uint8_t ignition_on, uint8_t starter_on);

/**
 * Clear the manual case for an input
 * This removes all active cases for the specified input, effectively turning off
 * all broadcasts for that input
 * 
 * @param input_num Input number (0-43)
 */
void EEPROM_ClearManualCase(uint8_t input_num);

/**
 * Check if an input is configured as a one-button start input
 * Reads byte 4 of the input's first ON case from EEPROM
 * 
 * @param input_num Input number (0-43)
 * @return 1 if configured as one-button start (bits 4-5 = 0x01), 0 otherwise
 */
uint8_t EEPROM_IsOneButtonStartInput(uint8_t input_num);

// ============================================================================
// TRACK IGNITION FUNCTIONS
// ============================================================================

/**
 * Update all track ignition cases based on current ignition flag state
 * This function scans all inputs and cases for track ignition configuration.
 * Cases with track ignition set (byte 4 bits 6-7 = 0x01) will be activated
 * or deactivated based on the ignition_flag parameter.
 * 
 * Track ignition cases:
 * - Ignore their physical input state
 * - Follow the global ignition flag instead
 * - Override pattern timing (track ignition takes precedence)
 * - Use normal aggregation for CAN message broadcasting
 * 
 * Call this function whenever the ignition flag changes state.
 * 
 * @param ignition_flag Current ignition state (1 = ON, 0 = OFF)
 */
void EEPROM_UpdateIgnitionTrackedCases(uint8_t ignition_flag);

/**
 * Check if a specific case is configured for track ignition mode
 * Reads byte 4 bits 6-7 from EEPROM for the specified case
 * 
 * @param input_num Input number (0-43)
 * @param case_num Case number for this input
 * @return 1 if track ignition is enabled (bits 6-7 = 0x01), 0 otherwise
 */
uint8_t EEPROM_IsTrackIgnitionCase(uint8_t input_num, uint8_t case_num);

// ============================================================================
// DEBUG FUNCTIONS
// ============================================================================

/**
 * Get pattern timing from an active case in RAM
 * @param input_num Input number (0-43)
 * @param case_num Case number (0-7)
 * @param pattern_on Pointer to store ON time
 * @param pattern_off Pointer to store OFF time
 * @return 1 if case found, 0 if not found
 */
uint8_t EEPROM_Debug_GetActiveCasePattern(uint8_t input_num, uint8_t case_num, 
                                          uint8_t *pattern_on, uint8_t *pattern_off);

/**
 * Get pattern timer state for an input
 * @param input_num Input number (0-43)
 * @param state Pointer to store state
 * @param timer Pointer to store timer value
 * @param on_time Pointer to store ON time
 * @param off_time Pointer to store OFF time
 * @param has_pattern Pointer to store has_pattern flag
 * @return 1 if valid, 0 if out of bounds
 */
uint8_t EEPROM_Debug_GetPatternTimer(uint8_t input_num, uint8_t *state, uint8_t *timer,
                                     uint8_t *on_time, uint8_t *off_time, uint8_t *has_pattern);


/**
 * Get input_num and case_num for an active case by index
 * @param active_case_index Index in active_cases array
 * @param input_num Pointer to store input number
 * @param case_num Pointer to store case number
 * @return 1 if valid, 0 if out of bounds
 */
uint8_t EEPROM_Debug_GetActiveCaseInfo(uint8_t active_case_index, uint8_t *input_num, uint8_t *case_num);

#endif // EEPROM_CASES_H

/**
 * Read a single byte from EEPROM
 * @param byte_address Byte address to read (0-4095)
 * @return Byte value, or 0xFF if address out of bounds
 */
uint8_t ReadEEPROMByte(uint16_t byte_address);