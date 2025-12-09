/*
 * Button Handler for MASTERCELL NGX
 * 5 buttons below LCD screen
 */

#ifndef BUTTONS_H
#define BUTTONS_H

#include <xc.h>
#include <stdint.h>

// Button pin definitions (from processor pin map)
#define BTN_RADIO_PIN       PORTBbits.RB0
#define BTN_RADIO_TRIS      TRISBbits.TRISB0

#define BTN_HOME_PIN        PORTBbits.RB10
#define BTN_HOME_TRIS       TRISBbits.TRISB10

#define BTN_DOWN_PIN        PORTBbits.RB11
#define BTN_DOWN_TRIS       TRISBbits.TRISB11

#define BTN_UP_PIN          PORTBbits.RB12
#define BTN_UP_TRIS         TRISBbits.TRISB12

#define BTN_SELECT_PIN      PORTBbits.RB13
#define BTN_SELECT_TRIS     TRISBbits.TRISB13

// Button identifiers (return values from Buttons_Scan)
#define BTN_ID_NONE     0
#define BTN_ID_RADIO    1
#define BTN_ID_HOME     2
#define BTN_ID_DOWN     3
#define BTN_ID_UP       4
#define BTN_ID_SELECT   5

// Function prototypes
void Buttons_Init(void);
uint8_t Buttons_Scan(void);
const char* Buttons_GetName(uint8_t button);

#endif // BUTTONS_H