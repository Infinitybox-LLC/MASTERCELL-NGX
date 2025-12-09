/*
 * FILE: climate.c
 * Climate Control Interface Implementation
 * 
 * Controls MCP4341 quad digital potentiometer via SPI2
 * for Vintage Air Gen-IV/V HVAC system integration.
 * 
 * CAN Message Format (PGN 0xFF99, SA 0x01):
 *   Byte 0: Temperature (0-15) → 0-5V on Wiper 3 (middle physical pin)
 *   Byte 1: Fan Speed (0-15) → 0-5V on Wiper 0
 *   Byte 2: Blend Position (0-15) → 0-5V on Wiper 1
 *   Bytes 3-7: Reserved (0x00)
 * 
 * Physical Pin Mapping (verified by testing):
 *   Wiper 0 → Fan Speed physical pin
 *   Wiper 1 → Blend physical pin
 *   Wiper 2 → NOT CONNECTED to external pin
 *   Wiper 3 → Temperature physical pin (middle)
 */

#include "climate.h"

#define FCY 16000000UL
#include <libpic30.h>

// ============================================================================
// PRIVATE VARIABLES
// ============================================================================

// Current wiper values (for diagnostic readback)
static uint8_t current_temp_value = 0;
static uint8_t current_fan_value = 0;
static uint8_t current_blend_value = 0;

// ============================================================================
// PRIVATE FUNCTIONS - SPI2 LOW-LEVEL
// ============================================================================

/**
 * Initialize SPI2 peripheral for MCP4341 communication
 * Mode 0,0 (CPOL=0, CPHA=0), 8-bit, Master mode
 */
static void SPI2_Init(void) {
    // Disable SPI2 during configuration
    SPI2STATbits.SPIEN = 0;
    
    // Configure SPI2 pins
    DIGIPOT_SCK_TRIS = 0;   // SCK2 as output
    DIGIPOT_SDO_TRIS = 0;   // SDO2 as output
    DIGIPOT_SDI_TRIS = 1;   // SDI2 as input
    
    // SPI2CON configuration:
    // - Master mode
    // - 8-bit mode
    // - Clock idle low (CKP = 0)
    // - Data sampled at middle of data output time (CKE = 1)
    // - Primary prescaler 64:1, secondary 1:1 → ~250kHz @ 16MHz FCY
    SPI2CONbits.MSTEN = 1;      // Master mode
    SPI2CONbits.CKP = 0;        // Clock idle state is LOW
    SPI2CONbits.CKE = 1;        // Data changes on idle→active clock edge
    SPI2CONbits.SMP = 0;        // Sample at middle of data output time
    SPI2CONbits.MODE16 = 0;     // 8-bit mode
    SPI2CONbits.PPRE = 0b00;    // Primary prescaler 64:1
    SPI2CONbits.SPRE = 0b111;   // Secondary prescaler 1:1
    
    // Clear any pending data
    uint16_t dummy = SPI2BUF;
    (void)dummy;
    
    // Enable SPI2
    SPI2STATbits.SPIEN = 1;
}

/**
 * Send a byte via SPI2 and receive response
 * @param data Byte to send
 * @return Received byte
 */
static uint8_t SPI2_Transfer(uint8_t data) {
    // Wait until TX buffer is empty
    while (SPI2STATbits.SPITBF);
    
    // Send data
    SPI2BUF = data;
    
    // Wait until RX buffer has data
    while (!SPI2STATbits.SPIRBF);
    
    // Return received data
    return (uint8_t)SPI2BUF;
}

// ============================================================================
// PRIVATE FUNCTIONS - MCP4341 COMMANDS
// ============================================================================

/**
 * Write a value to a MCP4341 wiper
 * 
 * Command format (16 bits total):
 *   Byte 0: [AD3:AD0][C1:C0][D9:D8] = [Address][00][Data MSB]
 *   Byte 1: [D7:D0] = Data LSB
 * 
 * @param wiper_addr Wiper address (0x00, 0x01, 0x06, 0x07)
 * @param value Wiper value (0-128)
 */
static void MCP4341_WriteWiper(uint8_t wiper_addr, uint8_t value) {
    // Clamp value to valid range
    if (value > MCP4341_WIPER_MAX) {
        value = MCP4341_WIPER_MAX;
    }
    
    // Build command byte: [AD3:AD0][C1:C0][D9:D8]
    // Address is in bits 7:4
    // Command (write) is 00 in bits 3:2
    // D9:D8 are in bits 1:0 (for 10-bit data, but we only use 7-bit so these are 0)
    uint8_t cmd_byte = (wiper_addr << 4) | (MCP4341_CMD_WRITE << 2) | 0x00;
    
    // Data byte is just the value (D7:D0)
    uint8_t data_byte = value;
    
    // Assert chip select (active LOW)
    DIGIPOT_CS = 0;
    __delay_us(1);
    
    // Send command and data
    SPI2_Transfer(cmd_byte);
    SPI2_Transfer(data_byte);
    
    // Deassert chip select
    __delay_us(1);
    DIGIPOT_CS = 1;
    __delay_us(1);
}

// ============================================================================
// PUBLIC FUNCTIONS - INITIALIZATION
// ============================================================================

void Climate_Init(void) {
    // Configure control pins as outputs
    DIGIPOT_CS_TRIS = 0;
    DIGIPOT_WP_TRIS = 0;
    DIGIPOT_RESET_TRIS = 0;
    
    // Set initial states:
    // CS = HIGH (not selected)
    // WP = HIGH (allow writes)
    // RESET = HIGH (normal operation)
    DIGIPOT_CS = 1;
    DIGIPOT_WP = 1;
    DIGIPOT_RESET = 1;
    
    __delay_ms(1);
    
    // Pulse RESET to ensure known state
    // After reset, MCP4341 defaults:
    // - Wipers set to mid-scale (0x40)
    // - TCON registers = 0xFF (all terminals connected)
    DIGIPOT_RESET = 0;
    __delay_ms(1);
    DIGIPOT_RESET = 1;
    __delay_ms(5);  // Wait for chip to stabilize after reset
    
    // Initialize SPI2 peripheral
    SPI2_Init();
    
    // TCON registers default to 0xFF (all terminals enabled)
    // No need to write to them - just set the wiper values
    
    // Set all outputs to 0V (off state)
    Climate_SetAllOff();
}

// ============================================================================
// PUBLIC FUNCTIONS - CAN MESSAGE HANDLING
// ============================================================================

uint8_t Climate_ProcessMessage(uint32_t can_id, uint8_t *data) {
    if (data == NULL) {
        return 0;
    }
    
    // Extract PGN and SA from CAN ID
    uint16_t pgn = (can_id >> 8) & 0xFFFF;
    uint8_t sa = can_id & 0xFF;
    
    // Check if this is a climate control message
    if (pgn != CLIMATE_PGN || sa != CLIMATE_SOURCE_ADDR) {
        return 0;  // Not a climate message
    }
    
    // Extract values from data bytes (lower nibble only, 0-15 range)
    uint8_t temp_value = data[0] & 0x0F;
    uint8_t fan_value = data[1] & 0x0F;
    uint8_t blend_value = data[2] & 0x0F;
    
    // Set the outputs
    Climate_SetTemperature(temp_value);
    Climate_SetFanSpeed(fan_value);
    Climate_SetBlend(blend_value);
    
    return 1;  // Message processed
}

// ============================================================================
// PUBLIC FUNCTIONS - WIPER CONTROL
// ============================================================================

void Climate_SetWiper(uint8_t wiper_addr, uint8_t value) {
    MCP4341_WriteWiper(wiper_addr, value);
}

/**
 * Scale CAN value (0-15) to wiper value (0-128)
 * Formula: wiper = (can_value * 128) / 15
 * Using integer math: wiper = can_value * 8 + can_value / 2 (approximate)
 * More accurate: wiper = (can_value * 128 + 7) / 15 (with rounding)
 */
static uint8_t ScaleToWiper(uint8_t can_value) {
    if (can_value > 15) {
        can_value = 15;
    }
    
    // Scale 0-15 to 0-128
    // Using: (can_value * 128 + 7) / 15 for rounding
    uint16_t scaled = ((uint16_t)can_value * 128 + 7) / 15;
    
    if (scaled > 128) {
        scaled = 128;
    }
    
    return (uint8_t)scaled;
}

void Climate_SetTemperature(uint8_t value) {
    uint8_t wiper_value = ScaleToWiper(value);
    current_temp_value = wiper_value;
    MCP4341_WriteWiper(CLIMATE_TEMP_WIPER, wiper_value);
}

void Climate_SetFanSpeed(uint8_t value) {
    uint8_t wiper_value = ScaleToWiper(value);
    current_fan_value = wiper_value;
    MCP4341_WriteWiper(CLIMATE_FAN_WIPER, wiper_value);
}

void Climate_SetBlend(uint8_t value) {
    uint8_t wiper_value = ScaleToWiper(value);
    current_blend_value = wiper_value;
    MCP4341_WriteWiper(CLIMATE_BLEND_WIPER, wiper_value);
}

void Climate_SetAllOff(void) {
    current_temp_value = 0;
    current_fan_value = 0;
    current_blend_value = 0;
    
    MCP4341_WriteWiper(CLIMATE_TEMP_WIPER, 0);
    MCP4341_WriteWiper(CLIMATE_FAN_WIPER, 0);
    MCP4341_WriteWiper(CLIMATE_BLEND_WIPER, 0);
}

// ============================================================================
// PUBLIC FUNCTIONS - DIAGNOSTICS
// ============================================================================

uint8_t Climate_GetTemperature(void) {
    return current_temp_value;
}

uint8_t Climate_GetFanSpeed(void) {
    return current_fan_value;
}

uint8_t Climate_GetBlend(void) {
    return current_blend_value;
}

