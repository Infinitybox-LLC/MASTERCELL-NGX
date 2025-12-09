/*
 * Button Handler Implementation for MASTERCELL NGX
 */

#include "buttons.h"

// Define FCY for delay macros
#define FCY 16000000UL
#include <libpic30.h>

// Debounce delay in milliseconds
#define DEBOUNCE_DELAY 20

// Initialize button pins
void Buttons_Init(void) {
    // Set all button pins as inputs
    BTN_RADIO_TRIS = 1;
    BTN_HOME_TRIS = 1;
    BTN_DOWN_TRIS = 1;
    BTN_UP_TRIS = 1;
    BTN_SELECT_TRIS = 1;
}

// Scan for button press and return button ID
// Returns BTN_ID_NONE if no button pressed
// Buttons are active LOW (pressed = 0, not pressed = 1)
uint8_t Buttons_Scan(void) {
    // Check each button (active low)
    if (BTN_RADIO_PIN == 0) {
        __delay_ms(DEBOUNCE_DELAY);
        if (BTN_RADIO_PIN == 0) {
            while (BTN_RADIO_PIN == 0);
            __delay_ms(DEBOUNCE_DELAY);
            return BTN_ID_RADIO;
        }
    }
    
    if (BTN_HOME_PIN == 0) {
        __delay_ms(DEBOUNCE_DELAY);
        if (BTN_HOME_PIN == 0) {
            while (BTN_HOME_PIN == 0);
            __delay_ms(DEBOUNCE_DELAY);
            return BTN_ID_HOME;
        }
    }
    
    if (BTN_DOWN_PIN == 0) {
        __delay_ms(DEBOUNCE_DELAY);
        if (BTN_DOWN_PIN == 0) {
            while (BTN_DOWN_PIN == 0);
            __delay_ms(DEBOUNCE_DELAY);
            return BTN_ID_DOWN;
        }
    }
    
    if (BTN_UP_PIN == 0) {
        __delay_ms(DEBOUNCE_DELAY);
        if (BTN_UP_PIN == 0) {
            while (BTN_UP_PIN == 0);
            __delay_ms(DEBOUNCE_DELAY);
            return BTN_ID_UP;
        }
    }
    
    if (BTN_SELECT_PIN == 0) {
        __delay_ms(DEBOUNCE_DELAY);
        if (BTN_SELECT_PIN == 0) {
            while (BTN_SELECT_PIN == 0);
            __delay_ms(DEBOUNCE_DELAY);
            return BTN_ID_SELECT;
        }
    }
    
    return BTN_ID_NONE;
}

// Get button name string
const char* Buttons_GetName(uint8_t button) {
    switch(button) {
        case BTN_ID_RADIO:
            return "RADIO";
        case BTN_ID_HOME:
            return "HOME";
        case BTN_ID_DOWN:
            return "SCROLL DOWN";
        case BTN_ID_UP:
            return "SCROLL UP";
        case BTN_ID_SELECT:
            return "SELECT";
        default:
            return "NONE";
    }
}