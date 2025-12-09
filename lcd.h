/*
 * LCD Driver for CFAH1604B-TMI-ET (16x4 Character LCD)
 * Controller: ST7066U (HD44780 compatible)
 * Mode: 8-bit parallel interface
 */

#ifndef LCD_H
#define LCD_H

#include <xc.h>
#include <stdint.h>

// LCD Commands
#define LCD_CLEAR           0x01
#define LCD_HOME            0x02
#define LCD_ENTRY_MODE      0x06  // Increment cursor, no shift
#define LCD_DISPLAY_ON      0x0C  // Display on, cursor off, blink off
#define LCD_DISPLAY_OFF     0x08
#define LCD_CURSOR_ON       0x0E  // Display on, cursor on, blink off
#define LCD_FUNCTION_SET    0x38  // 8-bit mode, 2 lines, 5x8 font
#define LCD_CGRAM_ADDR      0x40
#define LCD_DDRAM_ADDR      0x80

// Line addresses for 16x4 display
#define LCD_LINE1           0x00
#define LCD_LINE2           0x40
#define LCD_LINE3           0x10
#define LCD_LINE4           0x50

// Pin definitions from processor pin map
// Data pins
#define LCD_D0_TRIS         TRISCbits.TRISC13
#define LCD_D0              LATCbits.LATC13
#define LCD_D1_TRIS         TRISCbits.TRISC14
#define LCD_D1              LATCbits.LATC14
#define LCD_D2_TRIS         TRISDbits.TRISD1
#define LCD_D2              LATDbits.LATD1
#define LCD_D3_TRIS         TRISDbits.TRISD2
#define LCD_D3              LATDbits.LATD2
#define LCD_D4_TRIS         TRISDbits.TRISD3
#define LCD_D4              LATDbits.LATD3
#define LCD_D5_TRIS         TRISDbits.TRISD4
#define LCD_D5              LATDbits.LATD4
#define LCD_D6_TRIS         TRISDbits.TRISD5
#define LCD_D6              LATDbits.LATD5
#define LCD_D7_TRIS         TRISDbits.TRISD6
#define LCD_D7              LATDbits.LATD6

// Control pins
#define LCD_RS_TRIS         TRISDbits.TRISD10
#define LCD_RS              LATDbits.LATD10
#define LCD_RW_TRIS         TRISDbits.TRISD11
#define LCD_RW              LATDbits.LATD11
#define LCD_E_TRIS          TRISDbits.TRISD0
#define LCD_E               LATDbits.LATD0

// Backlight control
#define LCD_BL_TRIS         TRISBbits.TRISB5
#define LCD_BL              LATBbits.LATB5

// Function prototypes
void LCD_Init(void);
void LCD_Command(uint8_t cmd);
void LCD_Data(uint8_t data);
void LCD_Clear(void);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_Print(const char* str);
void LCD_PrintChar(char c);
void LCD_Backlight(uint8_t state);  // 1 = on, 0 = off

#endif // LCD_H