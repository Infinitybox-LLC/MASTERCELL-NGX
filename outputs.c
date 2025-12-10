/*
 * FILE: outputs.c
 * MOSFET Output Control Implementation
 * 
 * Controls 8 MOSFET gate outputs based on CAN messages from inLINK.
 * 
 * CAN Message Format (PGN 0xAF00 from inLINK, broadcast 1Hz):
 *   Byte 0: Temperature (handled by climate.c)
 *   Byte 1: Fan Speed (handled by climate.c)
 *   Byte 2: Blend Position (handled by climate.c)
 *   Byte 3: MOSFET Outputs - bits 0-7 → outputs 1-8
 *   Bytes 4-7: Reserved
 */

#include "outputs.h"
#include <stddef.h>  // For NULL

// ============================================================================
// PRIVATE VARIABLES
// ============================================================================

// Current output states (for diagnostic readback)
static uint8_t current_output_states = 0;

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
    // Only check PGN, accept any source address for now
    if (pgn != OUTPUTS_PGN) {
        return 0;  // Not an output control message
    }
    
    // Extract output states from byte 3
    uint8_t output_states = data[OUTPUTS_DATA_BYTE];
    
    // Set all outputs based on the byte
    Outputs_SetAll(output_states);
    
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

