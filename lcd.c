/*
 * LCD Driver Implementation for CFAH1604B-TMI-ET
 * 8-bit parallel mode, ST7066U controller
 */

#include "lcd.h"

// Define FCY for delay macros
#define FCY 16000000UL
#include <libpic30.h>

// Timing delays (in microseconds)
#define LCD_DELAY_ENABLE    1     // Enable pulse width
#define LCD_DELAY_COMMAND   2000  // Command execution time
#define LCD_DELAY_CLEAR     5000  // Clear/Home command time

// Private function to write 8 bits to data bus
static void LCD_WriteByte(uint8_t byte) {
    // Write each bit to corresponding pin
    LCD_D0 = (byte & 0x01) ? 1 : 0;
    LCD_D1 = (byte & 0x02) ? 1 : 0;
    LCD_D2 = (byte & 0x04) ? 1 : 0;
    LCD_D3 = (byte & 0x08) ? 1 : 0;
    LCD_D4 = (byte & 0x10) ? 1 : 0;
    LCD_D5 = (byte & 0x20) ? 1 : 0;
    LCD_D6 = (byte & 0x40) ? 1 : 0;
    LCD_D7 = (byte & 0x80) ? 1 : 0;
}

// Private function to pulse the Enable pin
static void LCD_EnablePulse(void) {
    LCD_E = 1;
    __delay_us(LCD_DELAY_ENABLE);
    LCD_E = 0;
    __delay_us(LCD_DELAY_ENABLE);
}

// Initialize LCD pins as outputs
static void LCD_InitPins(void) {
    // Set all LCD pins as outputs
    LCD_D0_TRIS = 0;
    LCD_D1_TRIS = 0;
    LCD_D2_TRIS = 0;
    LCD_D3_TRIS = 0;
    LCD_D4_TRIS = 0;
    LCD_D5_TRIS = 0;
    LCD_D6_TRIS = 0;
    LCD_D7_TRIS = 0;
    
    LCD_RS_TRIS = 0;
    LCD_RW_TRIS = 0;
    LCD_E_TRIS = 0;
    LCD_BL_TRIS = 0;
    
    // Initialize control pins to safe state
    LCD_RS = 0;  // Command mode
    LCD_RW = 0;  // Write mode
    LCD_E = 0;   // Enable low
    LCD_BL = 0;  // Backlight off initially
}

// Send command to LCD
void LCD_Command(uint8_t cmd) {
    LCD_RS = 0;  // Command mode
    LCD_RW = 0;  // Write mode
    
    LCD_WriteByte(cmd);
    LCD_EnablePulse();
    
    // Wait for command to execute
    if (cmd == LCD_CLEAR || cmd == LCD_HOME) {
        __delay_us(LCD_DELAY_CLEAR);
    } else {
        __delay_us(LCD_DELAY_COMMAND);
    }
}

// Send data to LCD
void LCD_Data(uint8_t data) {
    LCD_RS = 1;  // Data mode
    LCD_RW = 0;  // Write mode
    
    LCD_WriteByte(data);
    LCD_EnablePulse();
    
    __delay_us(LCD_DELAY_COMMAND);
}

// Initialize LCD in 8-bit mode
void LCD_Init(void) {
    // Initialize pins
    LCD_InitPins();
    
    // Wait for LCD to power up
    __delay_ms(50);
    
    // Initialization sequence for 8-bit mode
    // Function set: 8-bit mode
    LCD_Command(0x30);
    __delay_ms(5);
    
    LCD_Command(0x30);
    __delay_us(150);
    
    LCD_Command(0x30);
    __delay_us(150);
    
    // Function set: 8-bit, 2 lines (4 lines use 2-line mode), 5x8 font
    LCD_Command(LCD_FUNCTION_SET);
    
    // Display off
    LCD_Command(LCD_DISPLAY_OFF);
    
    // Clear display
    LCD_Command(LCD_CLEAR);
    
    // Entry mode: increment cursor, no display shift
    LCD_Command(LCD_ENTRY_MODE);
    
    // Display on, cursor off, blink off
    LCD_Command(LCD_DISPLAY_ON);
    
    // Turn on backlight
    LCD_BL = 1;
}

// Clear the display
void LCD_Clear(void) {
    LCD_Command(LCD_CLEAR);
}

// Set cursor position (row: 0-3, col: 0-15)
void LCD_SetCursor(uint8_t row, uint8_t col) {
    uint8_t address;
    
    switch(row) {
        case 0:
            address = LCD_LINE1 + col;
            break;
        case 1:
            address = LCD_LINE2 + col;
            break;
        case 2:
            address = LCD_LINE3 + col;
            break;
        case 3:
            address = LCD_LINE4 + col;
            break;
        default:
            address = LCD_LINE1 + col;
            break;
    }
    
    LCD_Command(LCD_DDRAM_ADDR | address);
}

// Print a string to LCD
void LCD_Print(const char* str) {
    while(*str) {
        LCD_Data(*str++);
    }
}

// Print a single character
void LCD_PrintChar(char c) {
    LCD_Data(c);
}

// Control backlight (1 = on, 0 = off)
void LCD_Backlight(uint8_t state) {
    LCD_BL = state ? 1 : 0;
}