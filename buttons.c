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

// Track which buttons are "stuck" (were LOW at startup and never released)
static uint8_t stuck_buttons = 0;

// Initialize stuck button detection (call once at startup)
void Buttons_DetectStuck(void) {
    // Record any buttons that are currently pressed at startup
    stuck_buttons = Buttons_GetRawState();
}

// Clear a button from stuck list when it's released
static void UpdateStuckButtons(void) {
    uint8_t current = Buttons_GetRawState();
    // If a button was stuck but is now released, remove from stuck list
    stuck_buttons &= current;
}

// Scan for button press and return button ID
// Returns BTN_ID_NONE if no button pressed
// Buttons are active LOW (pressed = 0, not pressed = 1)
// NON-BLOCKING version - ignores stuck buttons
uint8_t Buttons_Scan(void) {
    // Update stuck button tracking
    UpdateStuckButtons();
    
    // Check each button (active low), skipping stuck ones
    // Check in order: HOME, DOWN, UP, SELECT (skip RADIO if stuck)
    
    if ((stuck_buttons & 0x02) == 0 && BTN_HOME_PIN == 0) {
        __delay_ms(DEBOUNCE_DELAY);
        if (BTN_HOME_PIN == 0) {
            return BTN_ID_HOME;
        }
    }
    
    if ((stuck_buttons & 0x04) == 0 && BTN_DOWN_PIN == 0) {
        __delay_ms(DEBOUNCE_DELAY);
        if (BTN_DOWN_PIN == 0) {
            return BTN_ID_DOWN;
        }
    }
    
    if ((stuck_buttons & 0x08) == 0 && BTN_UP_PIN == 0) {
        __delay_ms(DEBOUNCE_DELAY);
        if (BTN_UP_PIN == 0) {
            return BTN_ID_UP;
        }
    }
    
    if ((stuck_buttons & 0x10) == 0 && BTN_SELECT_PIN == 0) {
        __delay_ms(DEBOUNCE_DELAY);
        if (BTN_SELECT_PIN == 0) {
            return BTN_ID_SELECT;
        }
    }
    
    // Check RADIO last (often stuck on some boards)
    if ((stuck_buttons & 0x01) == 0 && BTN_RADIO_PIN == 0) {
        __delay_ms(DEBOUNCE_DELAY);
        if (BTN_RADIO_PIN == 0) {
            return BTN_ID_RADIO;
        }
    }
    
    return BTN_ID_NONE;
}

// Get raw button states (non-blocking, no debounce)
// Returns bitmask: bit0=RADIO, bit1=HOME, bit2=DOWN, bit3=UP, bit4=SELECT
// A set bit means button is pressed (pin is LOW)
uint8_t Buttons_GetRawState(void) {
    uint8_t state = 0;
    if (BTN_RADIO_PIN == 0)  state |= 0x01;
    if (BTN_HOME_PIN == 0)   state |= 0x02;
    if (BTN_DOWN_PIN == 0)   state |= 0x04;
    if (BTN_UP_PIN == 0)     state |= 0x08;
    if (BTN_SELECT_PIN == 0) state |= 0x10;
    return state;
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