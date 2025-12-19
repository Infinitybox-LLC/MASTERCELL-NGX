/*
 * FILE: outputs.c
 * MOSFET Output Control Implementation
 * 
 * Controls 8 MOSFET gate outputs:
 *   OUT1-OUT6: Hardcoded to specific inputs, but CAN can also turn them on
 *   OUT7-OUT8: CAN/EEPROM controlled only
 * 
 * Hardcoded Mappings (OR'd with CAN override):
 *   OUT1 ← IN03 (Left Turn) OR CAN bit 0 - flashing pattern from input
 *   OUT2 ← IN04 (Right Turn) OR CAN bit 1 - flashing pattern from input
 *   OUT3 ← IN07 (High Beams) OR CAN bit 2 - steady
 *   OUT4 ← IN06 (Parking Lights) OR CAN bit 3 - steady
 *   OUT5 ← IN01 (Ignition) OR CAN bit 4 - steady
 *   OUT6 ← Security OR CAN bit 5 - software controlled
 *   OUT7 ← CAN bit 6 only
 *   OUT8 ← CAN bit 7 only
 * 
 * CAN Message Format (PGN 0xAF00/0xFF00):
 *   Byte 3: bits 0-7 control outputs 1-8 (0-5 are OR'd with inputs)
 *   Byte 4: bit 0 = inLINK ignition (treated same as physical ignition input)
 */

#include "outputs.h"
#include "inputs.h"
#include "eeprom_cases.h"
#include <stddef.h>  // For NULL

// ============================================================================
// PRIVATE VARIABLES
// ============================================================================

// Current output states (for diagnostic readback)
static uint8_t current_output_states = 0;

// Pattern timing for turn signals (OUT1/OUT2)
// Read from EEPROM at init to match user-configured turn signal pattern
// Default: 750ms on, 750ms off (3 ticks each at 250ms per tick)
static uint8_t pattern_on_ticks = 3;   // Will be loaded from EEPROM
static uint8_t pattern_off_ticks = 3;  // Will be loaded from EEPROM

// Pattern state for each turn signal output
static uint8_t left_turn_pattern_state = 0;   // 0 = off phase, 1 = on phase
static uint8_t right_turn_pattern_state = 0;
static uint8_t left_turn_timer = 0;           // Countdown timer
static uint8_t right_turn_timer = 0;

// Security output state (OUT6)
static uint8_t security_output_state = 0;

// CAN override state - allows CAN messages to also turn on outputs
// Final output = (hardcoded input state) OR (CAN override state)
static uint8_t can_override_states = 0;

// ============================================================================
// PUBLIC FUNCTIONS - INITIALIZATION
// ============================================================================

void Outputs_Init(void) {
    // Configure all 8 gate pins as outputs
    OUTPUT1_TRIS = 0;
    OUTPUT2_TRIS = 0;
    OUTPUT3_TRIS = 0;
    OUTPUT4_TRIS = 0;
    OUTPUT5_TRIS = 0;
    OUTPUT6_TRIS = 0;
    OUTPUT7_TRIS = 0;
    OUTPUT8_TRIS = 0;
    
    // Set all outputs to OFF (LOW) - prevents floating gates
    Outputs_AllOff();
    
    // Read pattern timing from IN03 (Left Turn) first ON case
    // This ensures local outputs flash at the same rate as PowerCell outputs
    uint16_t case_addr = EEPROM_GetCaseAddress(IN03, 0, 1);  // IN03, case 0, ON case
    if (case_addr != 0xFFFF) {
        CaseData case_data;
        if (EEPROM_ReadCase(case_addr, &case_data)) {
            // Use pattern timing from EEPROM (default to 3 if zero)
            pattern_on_ticks = case_data.pattern_on_time > 0 ? case_data.pattern_on_time : 3;
            pattern_off_ticks = case_data.pattern_off_time > 0 ? case_data.pattern_off_time : 3;
        }
    }
}

// ============================================================================
// PUBLIC FUNCTIONS - CAN MESSAGE HANDLING
// ============================================================================

uint8_t Outputs_ProcessMessage(uint32_t can_id, uint8_t *data) {
    if (data == NULL) {
        return 0;
    }
    
    // Extract PGN and SA from CAN ID
    uint16_t pgn = (can_id >> 8) & 0xFFFF;
    uint8_t sa = can_id & 0xFF;
    
    // Check if this is an output control message
    // Accept both external (0xAF00) and local (0xFF00) PGNs
    if (pgn != OUTPUTS_PGN && pgn != OUTPUTS_LOCAL_PGN) {
        return 0;  // Not an output control message
    }
    
    // Extract output states from byte 3
    uint8_t output_states = data[OUTPUTS_DATA_BYTE];
    
    // Store CAN override states for OUT1-OUT6 (bits 0-5)
    // These will be OR'd with hardcoded input states in UpdateFromInputs/PatternTick
    can_override_states = output_states & 0x3F;  // Bits 0-5 for OUT1-OUT6
    
    // OUT7 and OUT8 are CAN-only, set them directly
    Outputs_Set(7, (output_states & 0x40) ? 1 : 0);
    Outputs_Set(8, (output_states & 0x80) ? 1 : 0);
    
    // Process byte 4 for inLINK ignition and security (only from external AF00, not local FF00)
    // Byte 4, bit 0: Ignition (1=ON, 0=OFF)
    // Byte 4, bit 1: Security (1=DISARMED, 0=ARMED)
    if (pgn == OUTPUTS_PGN) {
        uint8_t can_ignition = (data[4] & 0x01) ? 1 : 0;
        uint8_t can_security = (data[4] & 0x02) ? 1 : 0;
        Inputs_SetCANIgnition(can_ignition);
        Inputs_SetCANSecurity(can_security);
        
        // Drive OUT6 security indicator: DISARMED = light ON, ARMED = light OFF
        Outputs_SetSecurity(can_security);
    }
    
    return 1;  // Message processed
}

// ============================================================================
// PUBLIC FUNCTIONS - OUTPUT CONTROL
// ============================================================================

void Outputs_SetAll(uint8_t states) {
    current_output_states = states;
    
    // Set each output based on corresponding bit
    OUTPUT1_LAT = (states & 0x01) ? 1 : 0;  // Bit 0 → Output 1
    OUTPUT2_LAT = (states & 0x02) ? 1 : 0;  // Bit 1 → Output 2
    OUTPUT3_LAT = (states & 0x04) ? 1 : 0;  // Bit 2 → Output 3
    OUTPUT4_LAT = (states & 0x08) ? 1 : 0;  // Bit 3 → Output 4
    OUTPUT5_LAT = (states & 0x10) ? 1 : 0;  // Bit 4 → Output 5
    OUTPUT6_LAT = (states & 0x20) ? 1 : 0;  // Bit 5 → Output 6
    OUTPUT7_LAT = (states & 0x40) ? 1 : 0;  // Bit 6 → Output 7
    OUTPUT8_LAT = (states & 0x80) ? 1 : 0;  // Bit 7 → Output 8
}

void Outputs_Set(uint8_t output, uint8_t state) {
    if (output < 1 || output > 8) {
        return;  // Invalid output number
    }
    
    // Update the state tracking variable
    if (state) {
        current_output_states |= (1 << (output - 1));
    } else {
        current_output_states &= ~(1 << (output - 1));
    }
    
    // Set the actual output pin
    switch (output) {
        case 1: OUTPUT1_LAT = state ? 1 : 0; break;
        case 2: OUTPUT2_LAT = state ? 1 : 0; break;
        case 3: OUTPUT3_LAT = state ? 1 : 0; break;
        case 4: OUTPUT4_LAT = state ? 1 : 0; break;
        case 5: OUTPUT5_LAT = state ? 1 : 0; break;
        case 6: OUTPUT6_LAT = state ? 1 : 0; break;
        case 7: OUTPUT7_LAT = state ? 1 : 0; break;
        case 8: OUTPUT8_LAT = state ? 1 : 0; break;
    }
}

uint8_t Outputs_GetAll(void) {
    return current_output_states;
}

uint8_t Outputs_Get(uint8_t output) {
    if (output < 1 || output > 8) {
        return 0;  // Invalid output number
    }
    
    return (current_output_states >> (output - 1)) & 0x01;
}

void Outputs_AllOff(void) {
    current_output_states = 0;
    
    OUTPUT1_LAT = 0;
    OUTPUT2_LAT = 0;
    OUTPUT3_LAT = 0;
    OUTPUT4_LAT = 0;
    OUTPUT5_LAT = 0;
    OUTPUT6_LAT = 0;
    OUTPUT7_LAT = 0;
    OUTPUT8_LAT = 0;
}

// ============================================================================
// PUBLIC FUNCTIONS - HARDCODED INPUT-TO-OUTPUT MAPPING
// ============================================================================

void Outputs_UpdateFromInputs(void) {
    // Read input states
    uint8_t in01_ignition = Inputs_GetState(IN01);
    uint8_t in06_parking = Inputs_GetState(IN06);
    uint8_t in07_high_beam = Inputs_GetState(IN07);
    uint8_t ignition_on = Inputs_GetIgnitionState();
    
    // OUT3 ← IN07 (High Beams) - requires ignition (matches EEPROM case)
    // Only turn on if ignition is on, OR CAN override
    uint8_t out3_state = (in07_high_beam && ignition_on) || (can_override_states & 0x04);
    Outputs_Set(3, out3_state);
    
    // OUT4 ← IN06 (Parking Lights/Gauge Illumination) - no ignition required
    uint8_t out4_state = in06_parking || (can_override_states & 0x08);
    Outputs_Set(4, out4_state);
    
    // OUT5 ← IN01 (Ignition) OR CAN override - steady on/off
    // This IS the ignition indicator, so no ignition check needed
    uint8_t out5_state = in01_ignition || (can_override_states & 0x10);
    Outputs_Set(5, out5_state);
    
    // OUT6 ← Security (software controlled) OR CAN override
    uint8_t out6_state = security_output_state || (can_override_states & 0x20);
    Outputs_Set(6, out6_state);
    
    // OUT1/OUT2 are handled by the pattern tick function for turn signals
    // CAN override is also checked there
}

void Outputs_PatternTick(void) {
    // This function should be called every 250ms from the pattern timer
    
    // Read turn signal inputs
    uint8_t in03_left_turn = Inputs_GetState(IN03);
    uint8_t in04_right_turn = Inputs_GetState(IN04);
    uint8_t in08_hazards = Inputs_GetState(IN08);
    uint8_t ignition_on = Inputs_GetIgnitionState();
    
    // Check CAN override states (steady on, no pattern)
    uint8_t can_left = (can_override_states & 0x01) ? 1 : 0;
    uint8_t can_right = (can_override_states & 0x02) ? 1 : 0;
    
    // Determine if left turn should be active
    // IN03/IN04 require ignition, IN08 (hazards) does not
    uint8_t left_active = (in03_left_turn && ignition_on) || in08_hazards;
    // Determine if right turn should be active
    uint8_t right_active = (in04_right_turn && ignition_on) || in08_hazards;
    
    // --- LEFT TURN (OUT1) ---
    if (left_active) {
        // Turn signal is active - run pattern
        if (left_turn_timer > 0) {
            left_turn_timer--;
        } else {
            // Timer expired - toggle state
            if (left_turn_pattern_state) {
                // Was ON, now go OFF
                left_turn_pattern_state = 0;
                left_turn_timer = pattern_off_ticks - 1;
            } else {
                // Was OFF, now go ON
                left_turn_pattern_state = 1;
                left_turn_timer = pattern_on_ticks - 1;
            }
        }
        // Output is pattern state OR CAN override
        Outputs_Set(1, left_turn_pattern_state || can_left);
    } else {
        // Turn signal inactive - reset pattern
        left_turn_pattern_state = 0;
        left_turn_timer = 0;
        // Still check CAN override for steady-on
        Outputs_Set(1, can_left);
    }
    
    // --- RIGHT TURN (OUT2) ---
    if (right_active) {
        // Turn signal is active - run pattern
        if (right_turn_timer > 0) {
            right_turn_timer--;
        } else {
            // Timer expired - toggle state
            if (right_turn_pattern_state) {
                // Was ON, now go OFF
                right_turn_pattern_state = 0;
                right_turn_timer = pattern_off_ticks - 1;
            } else {
                // Was OFF, now go ON
                right_turn_pattern_state = 1;
                right_turn_timer = pattern_on_ticks - 1;
            }
        }
        // Output is pattern state OR CAN override
        Outputs_Set(2, right_turn_pattern_state || can_right);
    } else {
        // Turn signal inactive - reset pattern
        right_turn_pattern_state = 0;
        right_turn_timer = 0;
        // Still check CAN override for steady-on
        Outputs_Set(2, can_right);
    }
}

void Outputs_SetSecurity(uint8_t state) {
    security_output_state = state ? 1 : 0;
    Outputs_Set(6, security_output_state);
}

uint8_t Outputs_GetSecurity(void) {
    return security_output_state;
}

