/*
 * FILE: inputs.c
 * Multiplexed Input Handler with Debouncing, Ignition Flag, One-Button Start, and Track Ignition
 * 
 * ONE-BUTTON START ARCHITECTURE:
 * - Physical button state is MOMENTARY (triggers actions)
 * - Ignition/starter state is LATCHING (persists until toggled)
 * - Uses EEPROM_SetManualCase() to control CAN broadcasts
 * - Does NOT call EEPROM_HandleInputChange() for one-button start inputs
 * 
 * TRACK IGNITION FEATURE:
 * - Cases with byte 4 bits 6-7 = 0x01 follow ignition flag, not physical input
 * - Calls EEPROM_UpdateIgnitionTrackedCases() whenever ignition_flag changes
 * - Track ignition cases work on any input, independent of physical state
 */

#include "inputs.h"
#include "eeprom_cases.h"
#include <stdio.h>

// Define FCY for delay macros
#define FCY 16000000UL
#include <libpic30.h>

// ============================================================================
// ONE-BUTTON START CONFIGURATION - EASY TO MODIFY
// ============================================================================
#define ONE_BUTTON_QUICK_PRESS_MS       500     // Press < 500ms = toggle ignition only
#define ONE_BUTTON_FUEL_PUMP_DELAY_MS   1000    // Fuel pump priming time: 1 second from ignition to starter
                                                 // Ignition turns on IMMEDIATELY when button held
                                                 // Starter engages after 1000ms total hold time

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

// Array to store current STABLE state of all 44 inputs (0 = off/high, 1 = on/low)
static uint8_t input_states[INPUT_COUNT];

// Array to store raw (un-debounced) readings
static uint8_t input_raw[INPUT_COUNT];

// Debounce counters - counts consecutive scans with same reading
static uint8_t debounce_count[INPUT_COUNT];

// Global ignition flag (RAM-based, resets on power cycle)
static uint8_t ignition_flag = 0;

// CAN-based ignition state from inLINK (PGN 0xAF00, byte 4, bit 0)
// This is OR'd with physical ignition inputs
static uint8_t can_ignition_state = 0;

// CAN-based security state from inLINK (PGN 0xAF00, byte 4, bit 1)
// 1 = DISARMED (security OK to start), 0 = ARMED (security blocks start)
static uint8_t can_security_state = 0;

// One-button start state tracking (one per input that is configured as one-button start)
typedef struct {
    uint8_t input_num;              // Which input this is tracking
    uint8_t active;                 // Is this state machine active?
    uint32_t press_start_time;      // Timestamp when button was pressed (in ms)
    uint8_t ignition_was_on;        // Was ignition on when we started this press?
    uint8_t ignition_is_on;         // Current latching ignition state
    uint8_t starter_is_on;          // Current starter state
    uint8_t ignition_set_this_press; // Flag: Did we already set ignition during this press?
    uint8_t neutral_was_on;         // Was neutral safety ON when button was pressed? (sequence requirement)
} OneButtonStartState;

#define MAX_ONE_BUTTON_INPUTS   8   // Support up to 8 one-button start inputs
static OneButtonStartState one_button_states[MAX_ONE_BUTTON_INPUTS];
static uint8_t one_button_count = 0;

// System tick counter (incremented every 10ms by Timer1)
static volatile uint32_t system_tick_ms = 0;

// Flag to indicate one-button start state changed (needs CAN transmission)
static uint8_t one_button_state_changed = 0;

// ============================================================================
// INPUT MAPPING TABLE
// ============================================================================

// Mapping table: [input_number] = {mux_number, channel}
typedef struct {
    uint8_t mux;      // Which MUX (1-6)
    uint8_t channel;  // Which channel (0-7) corresponding to S1-S8
} InputMapping;

static const InputMapping input_map[INPUT_COUNT] = {
    // IN01-IN04 on MUX01 S1-S4
    {1, 0}, {1, 1}, {1, 2}, {1, 3},
    
    // IN05-IN08 on MUX02 S4,S2,S7,S8
    {2, 3}, {2, 1}, {2, 6}, {2, 7},
    
    // IN09-IN12 on MUX03 S4,S3,S2,S1
    {3, 3}, {3, 2}, {3, 1}, {3, 0},
    
    // IN13-IN16 on MUX03 S8,S7,S6,S5
    {3, 7}, {3, 6}, {3, 5}, {3, 4},
    
    // IN17-IN20 on MUX01 S5-S8
    {1, 4}, {1, 5}, {1, 6}, {1, 7},
    
    // IN21-IN22 on MUX02 S3,S1
    {2, 2}, {2, 0},
    
    // IN23-IN26 on MUX04 S4,S3,S2,S1 (SWAPPED: IN23 and IN24 reversed in hardware)
    {4, 2}, {4, 3}, {4, 1}, {4, 0},
    
    // IN27-IN30 on MUX06 S5,S6,S7,S8
    {6, 4}, {6, 5}, {6, 6}, {6, 7},
    
    // IN31-IN34 on MUX06 S4,S3,S2,S1
    {6, 3}, {6, 2}, {6, 1}, {6, 0},
    
    // IN35-IN38 on MUX05 S5,S6,S8,S7
    {5, 4}, {5, 5}, {5, 7}, {5, 6},
    
    // HSIN01-HSIN06 on MUX02 S5,S6 and MUX04 S5,S6,S7,S8
    {2, 4}, {2, 5},
    {4, 4}, {4, 5}, {4, 6}, {4, 7}
};

// ============================================================================
// HELPER FUNCTIONS - EEPROM CONFIGURATION CHECKS
// ============================================================================

// Check if an input is configured as an ignition input
// Returns 1 if this input is configured as ignition, 0 otherwise
static uint8_t IsIgnitionInput(uint8_t input_num) {
    if(input_num >= TOTAL_INPUTS) {
        return 0;
    }
    
    // Calculate EEPROM address for this input's first ON case
    uint16_t base_address = EEPROM_GetCaseAddress(input_num, 0, 1);  // case_num=0, is_on_case=1
    
    if(base_address == 0xFFFF) {
        return 0;  // Invalid address
    }
    
    // Read byte 4 (configuration byte)
    uint8_t config_byte = ReadEEPROMByte(base_address + 4);
    
    // Check if bits 0-1 equal 0x01
    uint8_t set_ignition_bits = config_byte & 0x03;
    
    return (set_ignition_bits == 0x01) ? 1 : 0;
}

// ============================================================================
// ONE-BUTTON START FUNCTIONS
// ============================================================================

/**
 * Find the one-button start state for a given input
 * Returns pointer to state if found, NULL if not found
 */
static OneButtonStartState* FindOneButtonState(uint8_t input_num) {
    for(uint8_t i = 0; i < one_button_count; i++) {
        if(one_button_states[i].input_num == input_num) {
            return &one_button_states[i];
        }
    }
    return NULL;
}

/**
 * Handle one-button start state machine for a single input
 * Called when the input state changes or periodically to check timers
 */
static void HandleOneButtonStart(uint8_t input_num) {
    // Find or create state for this input
    OneButtonStartState *state = FindOneButtonState(input_num);
    
    if(state == NULL) {
        // Create new state if we have room
        if(one_button_count < MAX_ONE_BUTTON_INPUTS) {
            state = &one_button_states[one_button_count];
            state->input_num = input_num;
            state->ignition_was_on = 0;
            state->ignition_is_on = 0;
            state->starter_is_on = 0;
            state->active = 0;
            state->ignition_set_this_press = 0;
            one_button_count++;
        } else {
            return;  // No room for more one-button inputs
        }
    }
    
    uint8_t current_button_state = input_states[input_num];
    
    // STATE MACHINE
    if(current_button_state == 1 && !state->active) {
        // ====================================================================
        // BUTTON JUST PRESSED - Start tracking
        // ====================================================================
        state->active = 1;
        state->press_start_time = system_tick_ms;
        state->ignition_was_on = state->ignition_is_on;  // Remember current latching state
        state->ignition_set_this_press = 0;  // Reset flag - haven't set ignition yet for this press
        
        // Don't change anything yet, wait to see if it's a quick press or hold
    }
    else if(current_button_state == 0 && state->active) {
        // ====================================================================
        // BUTTON RELEASED
        // ====================================================================
        uint32_t press_duration = system_tick_ms - state->press_start_time;
        
        if(press_duration < ONE_BUTTON_QUICK_PRESS_MS) {
            // QUICK PRESS - Toggle ignition only (no neutral safety check for ignition)
            // Use the ignition_was_on value that was captured when button was pressed
            if(state->ignition_was_on) {
                // Ignition WAS on when we pressed, so turn it OFF
                state->ignition_is_on = 0;
                state->starter_is_on = 0;
                EEPROM_ClearManualCase(input_num);
                ignition_flag = 0;
                EEPROM_UpdateIgnitionTrackedCases(ignition_flag);  // Update track ignition cases
                one_button_state_changed = 1;  // Flag for main.c to transmit
            } else {
                // Ignition WAS off when we pressed, so turn it ON
                // (Ignition always works - no neutral safety requirement)
                state->ignition_is_on = 1;
                state->starter_is_on = 0;
                EEPROM_SetManualCase(input_num, 1, 0);
                ignition_flag = 1;
                EEPROM_UpdateIgnitionTrackedCases(ignition_flag);  // Update track ignition cases
                one_button_state_changed = 1;  // Flag for main.c to transmit
            }
        } else {
            // LONG PRESS RELEASED
            if(state->ignition_was_on) {
                // Ignition WAS on when we pressed, so turn it OFF
                state->ignition_is_on = 0;
                state->starter_is_on = 0;
                EEPROM_ClearManualCase(input_num);
                ignition_flag = 0;
                EEPROM_UpdateIgnitionTrackedCases(ignition_flag);  // Update track ignition cases
                one_button_state_changed = 1;  // Flag for main.c to transmit
            } else {
                // Ignition WAS off - turn off starter, leave ignition on
                // (Ignition always works - starter was already handled during hold)
                state->starter_is_on = 0;
                state->ignition_is_on = 1;
                EEPROM_SetManualCase(input_num, 1, 0);  // Ignition ON, starter OFF
                ignition_flag = 1;
                EEPROM_UpdateIgnitionTrackedCases(ignition_flag);  // Update track ignition cases
                one_button_state_changed = 1;  // Flag for main.c to transmit
            }
        }
        // Reset state
        state->active = 0;
    }
    else if(current_button_state == 1 && state->active) {
        // ====================================================================
        // BUTTON STILL HELD - Check if we need to engage starter
        // ====================================================================
        uint32_t press_duration = system_tick_ms - state->press_start_time;
        
        // On the FIRST scan after button press (when starting from ignition OFF),
        // turn on ignition IMMEDIATELY. Only do this ONCE per button press.
        // This gives the fuel pump time to prime before starter engages
        // (Ignition always works - no neutral safety requirement)
        if(!state->ignition_was_on && !state->ignition_set_this_press) {
            // Just started holding, turn on ignition now
            state->ignition_is_on = 1;
            state->starter_is_on = 0;
            state->ignition_set_this_press = 1;  // Mark that we've set it
            EEPROM_SetManualCase(input_num, 1, 0);
            ignition_flag = 1;
            EEPROM_UpdateIgnitionTrackedCases(ignition_flag);  // Update track ignition cases
            one_button_state_changed = 1;  // Flag for main.c to transmit
        }
        
        // After holding for 1000ms (1 second), engage starter
        // This gives fuel pump 1 full second to prime
        // NEUTRAL SAFETY CHECK: Check neutral NOW (after ignition has been on for 1 second)
        // This allows neutral safety switches that require ignition to be ON to work
        if(press_duration >= ONE_BUTTON_FUEL_PUMP_DELAY_MS && !state->ignition_was_on) {
            // Engage starter (only if we started with ignition off AND neutral is currently ON)
            if(!state->starter_is_on && input_states[15]) {  // IN16 = index 15 = neutral safety
                state->starter_is_on = 1;
                state->ignition_is_on = 1;
                EEPROM_SetManualCase(input_num, 1, 1);  // Ignition ON, starter ON
                one_button_state_changed = 1;  // Flag for main.c to transmit
            }
        }
    }
}

// ============================================================================
// HARDWARE INTERFACE FUNCTIONS
// ============================================================================

// Set multiplexer channel (0-7)
static void Inputs_SetMuxChannel(uint8_t channel) {
    LATGbits.LATG12 = (channel & 0x01) ? 1 : 0;  // A0
    LATGbits.LATG13 = (channel & 0x02) ? 1 : 0;  // A1
    LATGbits.LATG14 = (channel & 0x04) ? 1 : 0;  // A2
    
    __delay_ms(1);
}

// Read all 6 multiplexer outputs
static void Inputs_ReadMuxOutputs(uint8_t mux_readings[6]) {
    mux_readings[0] = MUX01MC;
    mux_readings[1] = MUX02MC;
    mux_readings[2] = MUX03MC;
    mux_readings[3] = MUX04MC;
    mux_readings[4] = MUX05MC;
    mux_readings[5] = MUX06MC;
}

// ============================================================================
// PUBLIC INTERFACE FUNCTIONS
// ============================================================================

// Initialize input system
void Inputs_Init(void) {
    // Set MUX control pins as outputs FIRST
    TRISGbits.TRISG15 = 0;  // MUX_EN
    TRISGbits.TRISG12 = 0;  // MUX_A0
    TRISGbits.TRISG13 = 0;  // MUX_A1
    TRISGbits.TRISG14 = 0;  // MUX_A2
    
    // Disable MUXes first (EN = LOW)
    LATGbits.LATG15 = 0;
    __delay_ms(10);
    
    // Set channels to 0
    LATGbits.LATG12 = 0;
    LATGbits.LATG13 = 0;
    LATGbits.LATG14 = 0;
    __delay_ms(10);
    
    // Now ENABLE MUXes (EN = HIGH - ACTIVE HIGH!)
    LATGbits.LATG15 = 1;
    __delay_ms(10);
    
    // Set MUX output pins as inputs
    TRISDbits.TRISD7 = 1;   // MUX01MC
    TRISDbits.TRISD9 = 1;   // MUX02MC
    TRISDbits.TRISD8 = 1;   // MUX03MC
    TRISBbits.TRISB14 = 1;  // MUX04MC
    TRISCbits.TRISC1 = 1;   // MUX05MC
    TRISCbits.TRISC2 = 1;   // MUX06MC
    
    // Initialize all states and debounce counters
    for(uint8_t i = 0; i < INPUT_COUNT; i++) {
        input_states[i] = 0;
        input_raw[i] = 0;
        debounce_count[i] = 0;
    }
    
    // Initialize ignition flag to off
    ignition_flag = 0;
    EEPROM_UpdateIgnitionTrackedCases(ignition_flag);  // Initialize track ignition cases
    
    // Initialize one-button start states
    one_button_count = 0;
    for(uint8_t i = 0; i < MAX_ONE_BUTTON_INPUTS; i++) {
        one_button_states[i].active = 0;
        one_button_states[i].input_num = 0xFF;
        one_button_states[i].ignition_was_on = 0;
        one_button_states[i].ignition_is_on = 0;
        one_button_states[i].starter_is_on = 0;
        one_button_states[i].ignition_set_this_press = 0;
        one_button_states[i].neutral_was_on = 0;
    }
    
    // Initialize system tick
    system_tick_ms = 0;
}

// Scan all inputs through multiplexers WITH DEBOUNCING
void Inputs_Scan(void) {
    uint8_t mux_readings[6];
    uint8_t any_ignition_input_changed = 0;
    
    // Increment system tick - called every ~30ms in practice (scan_timer = 10, but 1ms timer seems to be 3ms)
    system_tick_ms += 30;
    
    // Scan through all 8 channels
    for(uint8_t channel = 0; channel < 8; channel++) {
        // Set all MUXes to this channel
        Inputs_SetMuxChannel(channel);
        
        // Read all MUX outputs
        Inputs_ReadMuxOutputs(mux_readings);
        
        // Process all inputs on this channel
        for(uint8_t input = 0; input < INPUT_COUNT; input++) {
            if(input_map[input].channel == channel) {
                uint8_t mux_idx = input_map[input].mux - 1;  // Convert to 0-based
                
                // Raw reading: 1 = not pressed/off, 0 = pressed/on
                // Convert to: 1 = on/active, 0 = off/inactive
                uint8_t new_reading = (mux_readings[mux_idx] == 0) ? 1 : 0;
                
                // Store previous stable state to detect changes
                uint8_t prev_state = input_states[input];
                
                // DEBOUNCE LOGIC
                if(new_reading != input_raw[input]) {
                    // Reading changed - reset debounce counter and update raw value
                    input_raw[input] = new_reading;
                    debounce_count[input] = 0;
                } else {
                    // Reading is stable (same as last time)
                    if(debounce_count[input] < DEBOUNCE_SCANS) {
                        debounce_count[input]++;
                    }
                    
                    // If stable for required number of scans, update stable state
                    if(debounce_count[input] >= DEBOUNCE_SCANS) {
                        input_states[input] = new_reading;
                        
                        // Check if state changed
                        if(input_states[input] != prev_state) {
                            // Check if this is a one-button start input
                            if(EEPROM_IsOneButtonStartInput(input)) {
                                HandleOneButtonStart(input);
                            }
                            // Check if this is a regular ignition input
                            else if(IsIgnitionInput(input)) {
                                any_ignition_input_changed = 1;
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Update ignition flag if any regular ignition input changed
    if(any_ignition_input_changed) {
        if(Inputs_UpdateIgnitionFlag()) {
            // Ignition flag changed - track ignition cases were updated
            // Need to transmit the updated messages (including clearing messages)
            one_button_state_changed = 1;  // Reuse this flag to trigger transmission in main.c
        }
    }
    
    // Process one-button start inputs that are currently active (for timer checks)
    for(uint8_t i = 0; i < one_button_count; i++) {
        if(one_button_states[i].active) {
            HandleOneButtonStart(one_button_states[i].input_num);
        }
    }
}

// Get STABLE state of specific input (returns 1 if active/on, 0 if inactive/off)
uint8_t Inputs_GetState(uint8_t input_num) {
    if(input_num >= INPUT_COUNT) {
        return 0;
    }
    return input_states[input_num];
}

// Get name of input
const char* Inputs_GetName(uint8_t input_num) {
    static char name_buffer[8];
    
    if(input_num < 38) {
        sprintf(name_buffer, "IN%02d", input_num + 1);
    } else {
        sprintf(name_buffer, "HSIN%02d", input_num - 37);
    }
    
    return name_buffer;
}

// Get the ignition flag state
// Returns 1 if ignition is on from physical inputs OR CAN (inLINK)
uint8_t Inputs_GetIgnitionState(void) {
    return ignition_flag || can_ignition_state;
}

// Set the CAN-based ignition state from inLINK (PGN 0xAF00, byte 4, bit 0)
void Inputs_SetCANIgnition(uint8_t state) {
    uint8_t old_state = can_ignition_state;
    can_ignition_state = state ? 1 : 0;
    
    // If CAN ignition state changed, update ignition-tracked cases
    if (old_state != can_ignition_state) {
        EEPROM_UpdateIgnitionTrackedCases(Inputs_GetIgnitionState());
    }
}

// Get the security state from inLINK (PGN 0xAF00, byte 4, bit 1)
// Returns 1 if DISARMED (OK to start), 0 if ARMED (blocks start)
uint8_t Inputs_GetSecurityState(void) {
    return can_security_state;
}

// Set the CAN-based security state from inLINK (PGN 0xAF00, byte 4, bit 1)
// state: 1 = DISARMED, 0 = ARMED
void Inputs_SetCANSecurity(uint8_t state) {
    can_security_state = state ? 1 : 0;
}

// Update the ignition flag based on all ignition inputs
// Returns 1 if ignition flag changed (requiring CAN transmission), 0 if unchanged
uint8_t Inputs_UpdateIgnitionFlag(void) {
    uint8_t any_ignition_on = 0;
    
    // Check all inputs
    for(uint8_t i = 0; i < TOTAL_INPUTS; i++) {
        // Is this input configured as a regular ignition input (not one-button start)?
        if(IsIgnitionInput(i) && !EEPROM_IsOneButtonStartInput(i)) {
            // Is this ignition input currently ON?
            if(input_states[i] == 1) {
                any_ignition_on = 1;
                break;  // Found at least one ON, can stop checking
            }
        }
    }
    
    // Set or clear the ignition flag
    uint8_t old_ignition_flag = ignition_flag;
    ignition_flag = any_ignition_on ? 1 : 0;
    
    // Update track ignition cases if ignition flag changed
    if(old_ignition_flag != ignition_flag) {
        EEPROM_UpdateIgnitionTrackedCases(ignition_flag);
        return 1;  // Flag changed - need to transmit
    }
    
    return 0;  // No change
}

// Initialize the ignition flag on system startup
void Inputs_InitIgnitionFlag(void) {
    // Simply update the flag based on current input states
    // This should be called after EEPROM is loaded and at least one input scan has completed
    Inputs_UpdateIgnitionFlag();
}

// Check if an input is configured as a one-button start input (wrapper for EEPROM function)
uint8_t Inputs_IsOneButtonStartInput(uint8_t input_num) {
    return EEPROM_IsOneButtonStartInput(input_num);
}

// Check if one-button start state changed (requires CAN transmission)
// This function returns 1 if changed and automatically clears the flag
uint8_t Inputs_OneButtonStartStateChanged(void) {
    if(one_button_state_changed) {
        one_button_state_changed = 0;
        return 1;
    }
    return 0;
}