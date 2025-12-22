/*
 * FILE: inreserve.c
 * inRESERVE Battery Disconnect Feature Implementation
 * 
 * Monitors PowerCell voltage and activates a latching solenoid output
 * to disconnect the battery after sustained low voltage condition.
 */

#include "inreserve.h"
#include "eeprom_config.h"
#include "j1939.h"
#include <string.h>
#include <stdio.h>

// ============================================================================
// MODULE STATE
// ============================================================================

static InReserveConfig config;
static InReserveState state;

// System tick (imported from main.c)
extern volatile uint32_t system_time_ms;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Convert time code to seconds
 */
static uint32_t TimeCodeToSeconds(uint8_t time_code) {
    // 0=30sec (dev), 1=15min, 2=20min
    switch (time_code) {
        case 0: return 30;        // 30 seconds (dev/test)
        case 1: return 15 * 60;   // 15 minutes
        case 2: return 20 * 60;   // 20 minutes
        default: return 15 * 60;  // Default to 15 min
    }
}

/**
 * Convert voltage code to millivolts
 */
static uint16_t VoltageCodeToMV(uint8_t voltage_code) {
    // 0=12.1V, 1=12.2V, 2=12.3V
    // Formula: 12100 + (code * 100)
    return 12100 + (voltage_code * 100);
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void InReserve_Init(void) {
    // Clear state
    memset(&state, 0, sizeof(state));
    
    // Load configuration from EEPROM
    InReserve_LoadConfig();
}

void InReserve_LoadConfig(void) {
    // Read byte 1: [CellID][Output]
    uint8_t byte1 = EEPROM_Config_ReadByte(EEPROM_CFG_INRESERVE_1);
    
    // Read byte 2: [Time][Voltage]
    uint8_t byte2 = EEPROM_Config_ReadByte(EEPROM_CFG_INRESERVE_2);
    
    // Parse configuration
    config.cell_id = (byte1 >> 4) & 0x0F;
    config.output = byte1 & 0x0F;
    config.time_code = (byte2 >> 4) & 0x0F;
    config.voltage_code = byte2 & 0x0F;
    
    // Validate and limit values
    if (config.cell_id > 6) config.cell_id = 0;  // Invalid cell ID, disable
    if (config.output < 1 || config.output > 10) config.output = 9;  // Default to output 9
    if (config.time_code > 2) config.time_code = 1;  // Default to 15min if invalid
    if (config.voltage_code > 2) config.voltage_code = 2;  // Default to 12.3V if invalid
    
    // Calculate derived values
    config.enabled = (config.cell_id != 0) ? 1 : 0;
    config.time_seconds = TimeCodeToSeconds(config.time_code);
    config.voltage_mv = VoltageCodeToMV(config.voltage_code);
}

void InReserve_SaveConfig(void) {
    // Build byte 1: [CellID][Output]
    uint8_t byte1 = ((config.cell_id & 0x0F) << 4) | (config.output & 0x0F);
    
    // Build byte 2: [Time][Voltage]
    uint8_t byte2 = ((config.time_code & 0x0F) << 4) | (config.voltage_code & 0x0F);
    
    // Write to EEPROM
    EEPROM_Config_WriteByte(EEPROM_CFG_INRESERVE_1, byte1);
    EEPROM_Config_WriteByte(EEPROM_CFG_INRESERVE_2, byte2);
    
    // Update derived values
    config.enabled = (config.cell_id != 0) ? 1 : 0;
    config.time_seconds = TimeCodeToSeconds(config.time_code);
    config.voltage_mv = VoltageCodeToMV(config.voltage_code);
}

// ============================================================================
// CONFIGURATION ACCESS
// ============================================================================

InReserveConfig* InReserve_GetConfig(void) {
    return &config;
}

InReserveState* InReserve_GetState(void) {
    return &state;
}

void InReserve_SetCellID(uint8_t cell_id) {
    config.cell_id = cell_id & 0x0F;
    config.enabled = (config.cell_id != 0) ? 1 : 0;
    
    // Re-validate output for new cell's valid range
    if (config.cell_id != 0) {
        uint8_t min_out = InReserve_GetMinOutput(config.cell_id);
        uint8_t max_out = InReserve_GetMaxOutput(config.cell_id);
        if (config.output < min_out) config.output = min_out;
        if (config.output > max_out) config.output = max_out;
    }
}

void InReserve_SetOutput(uint8_t output) {
    uint8_t min_out = InReserve_GetMinOutput(config.cell_id);
    uint8_t max_out = InReserve_GetMaxOutput(config.cell_id);
    if (output < min_out) output = min_out;
    if (output > max_out) output = max_out;
    config.output = output;
}

void InReserve_SetTime(uint8_t time_code) {
    if (time_code > 2) time_code = 1;  // Default to 15min
    config.time_code = time_code;
    config.time_seconds = TimeCodeToSeconds(time_code);
}

void InReserve_SetVoltage(uint8_t voltage_code) {
    if (voltage_code > 2) voltage_code = 2;  // Default to 12.3V
    config.voltage_code = voltage_code;
    config.voltage_mv = VoltageCodeToMV(voltage_code);
}

// ============================================================================
// RUNTIME UPDATE
// ============================================================================

void InReserve_Update(uint16_t current_voltage_mv) {
    // Store last voltage for display
    state.last_voltage_mv = current_voltage_mv;
    
    // If disabled, do nothing
    if (!config.enabled) {
        return;
    }
    
    // Check if voltage is below or equal to threshold
    if (current_voltage_mv <= config.voltage_mv) {
        // Voltage is low
        if (!state.timer_active) {
            // Start the timer
            state.timer_active = 1;
            state.timer_start_ms = system_time_ms;
        }
        
        // Check if time threshold reached
        uint32_t elapsed_ms = system_time_ms - state.timer_start_ms;
        uint32_t threshold_ms = config.time_seconds * 1000;
        
        if (elapsed_ms >= threshold_ms) {
            // Time threshold reached - trigger the output!
            state.triggered = 1;  // Mark that we've triggered (for display)
            
            // Send CAN message to activate the PowerCell output
            // Build message for the configured PowerCell
            uint8_t data[8] = {0, 0, 0, 0, 0, 0, 0, 0};
            
            // Set the output bit
            // Outputs 1-8 are in byte 0: bit 7=out1, bit 6=out2, ..., bit 0=out8
            // Outputs 9-10 are in byte 1: bit 7=out9, bit 6=out10
            if (config.output <= 8) {
                data[0] = (1 << (8 - config.output));  // Bit 7=out1, bit 0=out8
            } else {
                data[1] = (1 << (16 - config.output)); // Bit 7=out9, bit 6=out10
            }
            
            // Determine PGN based on PowerCell ID
            // Front (cell 1): FF01, Rear (cell 2): FF02, etc.
            uint16_t pgn = 0xFF00 + config.cell_id;
            
            // Transmit the message (priority 6, SA 0x1E like other MASTERCELL messages)
            J1939_TransmitMessage(6, pgn, 0x1E, data);
            
            // Reset timer to try again if still powered
            state.timer_start_ms = system_time_ms;
        }
    } else {
        // Voltage is OK - reset timer and triggered flag
        state.timer_active = 0;
        state.triggered = 0;
    }
}

void InReserve_Reset(void) {
    state.timer_active = 0;
    state.triggered = 0;
    state.timer_start_ms = 0;
}

// ============================================================================
// STRING HELPERS FOR MENU DISPLAY
// ============================================================================

const char* InReserve_GetCellName(uint8_t cell_id) {
    switch (cell_id) {
        case 0: return "OFF";
        case 1: return "Front PC";
        case 2: return "Rear PC";
        case 3: return "Powercell 3";
        case 4: return "Powercell 4";
        case 5: return "Powercell 5";
        case 6: return "Powercell 6";
        default: return "?";
    }
}

const char* InReserve_GetTimeString(uint8_t time_code) {
    switch (time_code) {
        case 0: return "30 sec";
        case 1: return "15 min";
        case 2: return "20 min";
        default: return "?";
    }
}

void InReserve_GetVoltageString(uint8_t voltage_code, char* buffer) {
    // Calculate voltage: 12.1 + (code * 0.1)
    uint16_t mv = VoltageCodeToMV(voltage_code);
    uint8_t volts = mv / 1000;
    uint8_t tenths = (mv % 1000) / 100;
    sprintf(buffer, "%d.%dV", volts, tenths);
}

uint8_t InReserve_GetMinOutput(uint8_t cell_id) {
    switch (cell_id) {
        case 1: return 7;   // Front PC: outputs 7-10
        case 2: return 4;   // Rear PC: outputs 4-9
        default: return 1;  // Others: outputs 1-10
    }
}

uint8_t InReserve_GetMaxOutput(uint8_t cell_id) {
    switch (cell_id) {
        case 1: return 10;  // Front PC: outputs 7-10
        case 2: return 9;   // Rear PC: outputs 4-9
        default: return 10; // Others: outputs 1-10
    }
}

uint8_t InReserve_GetOutputCount(uint8_t cell_id) {
    return InReserve_GetMaxOutput(cell_id) - InReserve_GetMinOutput(cell_id) + 1;
}
