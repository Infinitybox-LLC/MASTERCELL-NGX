/*
 * FILE: main.c
 * Shows raw CAN register values
 * WITH PATTERN TIMING SUPPORT - Continuous transmission when patterns active
 * WITH MENU SYSTEM - HOME button navigation
 * FIXED: Initialize unused pins (especially MOSFET gates) to prevent floating pins
 */

 #include <xc.h>
 #include <stdint.h>
 #include <stdio.h>
 #include "lcd.h"
 #include "buttons.h"
 #include "inputs.h"
 #include "j1939.h"
 #include "inlink.h"
 #include "eeprom_cases.h"
 #include "eeprom_init.h"
 #include "eeprom_config.h"
#include "can_config.h"
#include "network_inventory.h"
#include "climate.h"
 
 // Debug variables from eeprom_cases.c
 
 _FOSC(CSW_FSCM_OFF & XT_PLL8);
 _FWDT(WDT_OFF);
 _FBORPOR(MCLR_EN & PWRT_OFF);
 _FGS(GWRP_OFF);
 _FICD(ICS_PGD);
 
 #define FCY 16000000UL
 #include <libpic30.h>
 
 #define LED_PIN LATGbits.LATG0
 #define LED_TRIS TRISGbits.TRISG0
 
 // Screen states
 #define SCREEN_MAIN         0
 #define SCREEN_MENU         1
 #define SCREEN_SWITCH       2
 #define SCREEN_INVENTORY    3
 #define SCREEN_SYSTEM_INFO  4
 #define SCREEN_DEBUG        5
 
 // Menu items
 #define MENU_SWITCH_STATES  0
 #define MENU_SYSTEM_INVENTORY 1
 #define MENU_SYSTEM_INFO    2
 #define MENU_DEBUG          3
 #define MENU_HOME_SCREEN    4
 #define MENU_COUNT          5
 
 volatile uint16_t scan_timer = 10;
 volatile uint16_t display_timer = 500;
 volatile uint16_t j1939_timer = 1000;
 volatile uint16_t button_debounce_timer = 0;
 volatile uint16_t led_on_timer = 0;
 volatile uint16_t pattern_timer = 0;
 volatile uint16_t backlight_timer = 0;
 volatile uint8_t pattern_changed = 0;
 volatile uint8_t heartbeat_pending = 0;
 volatile uint8_t state_changed = 0;    // PHASE 2: Flag for input/inLINK state changes
 
 uint8_t prev_input_states[44] = {0};
 volatile uint8_t last_input_triggered = 0xFF;
 volatile uint8_t last_msg_count = 0;
 volatile uint32_t last_rx_can_id = 0;
 volatile uint16_t last_rx_pgn = 0;
 
 // PHASE 1: Storage for previous broadcast to detect data changes
 typedef struct {
     uint16_t pgn;
     uint8_t source_addr;
     uint8_t data[8];
     uint8_t valid;
 } PreviousMessage;
 
 PreviousMessage prev_messages[MAX_UNIQUE_MESSAGES];
 uint8_t prev_msg_count = 0;
 
 uint8_t current_screen = SCREEN_MAIN;
 uint8_t menu_selection = 0;
 uint8_t menu_scroll_position = 0;
 uint8_t last_button = BTN_ID_NONE;
 uint8_t inventory_scroll_position = 0;
 volatile uint32_t system_time_ms = 0;
 
 // PHASE 3: Broadcast reason codes
 #define BROADCAST_REASON_PATTERN_TICK   0
 #define BROADCAST_REASON_STATE_CHANGE   1
 
 void Timer1_Init(void);
 void TransmitAggregatedMessages(uint8_t reason);  // PHASE 3: Added reason parameter
 void DisplayMainScreen(void);
 void DisplayMenuScreen(void);
 void DisplaySwitchScreen(void);
 void DisplayInventoryScreen(void);
 void DisplaySystemInfoScreen(void);
 void DisplayDebugScreen(void);
 void HandleButtonPress(uint8_t button);
 void InitUnusedPins(void);
 
 int main(void) {
     char display_buffer[17];
     uint16_t read_pgn;
     uint8_t read_sa;
     uint16_t write_pgn;
     uint8_t write_sa;
     
     // CRITICAL: Initialize unused pins FIRST before anything else
     // This prevents floating MOSFET gates from turning on and drawing excessive current
     InitUnusedPins();
     
     LED_TRIS = 0;
     LED_PIN = 0;
     ADPCFG = 0xFFFF;
     
    Inputs_Init();
    LCD_Init();
    Buttons_Init();
    J1939_Init();
    InLink_Init();
    Network_Init();
    Climate_Init();
     
     LCD_Clear();
     LCD_SetCursor(0, 0);
     LCD_Print("MASTERCELL NGX  ");
     LCD_SetCursor(1, 0);
     LCD_Print("Initializing... ");
     __delay_ms(1000);
     
     // Check if EEPROM needs initialization
     uint8_t needs_init = 0;
     
     // REASON 1: EEPROM not initialized (no 0xA5 stamp found)
     if(!EEPROM_IsInitialized()) {
         LCD_Clear();
         LCD_SetCursor(0, 0);
         LCD_Print("EEPROM Empty!   ");
         LCD_SetCursor(1, 0);
         LCD_Print("Auto-Init...    ");
         __delay_ms(1000);
         needs_init = 1;
     }
     
     // REASON 2: SELECT button held during boot (force reinit)
     if((PORTB & 0x2000) == 0) {
         LCD_Clear();
         LCD_SetCursor(0, 0);
         LCD_Print("FORCE REINIT!   ");
         LCD_SetCursor(1, 0);
         LCD_Print("Release button..");
         while((PORTB & 0x2000) == 0);
         __delay_ms(500);
         needs_init = 1;
     }
     
     // Perform EEPROM initialization if needed
     if(needs_init) {
         LCD_Clear();
         LCD_SetCursor(0, 0);
         LCD_Print("EEPROM Init...  ");
         LCD_SetCursor(1, 0);
         LCD_Print("Please wait...  ");
         
         LED_PIN = 1;
         // Configuration selection menu
        eeprom_config_type_t selected_config = CONFIG_STD_FRONT_ENGINE;
        uint8_t done = 0;
        uint8_t button_id;
        
        // Wait for buttons to be released first
        while(Buttons_Scan() != BTN_ID_NONE) {
            __delay_ms(10);
        }
        
        LCD_Clear();
        LCD_SetCursor(0, 0);
        LCD_Print("Select Config:  ");
        
        while (!done) {
            // Display current selection
            LCD_SetCursor(1, 0);
            switch(selected_config) {
                case CONFIG_STD_FRONT_ENGINE:
                    LCD_Print(">Front Engine   ");
                    LCD_SetCursor(2, 0);
                    LCD_Print(" Rear Engine    ");
                    LCD_SetCursor(3, 0);
                    LCD_Print(" Customer       ");
                    break;
                case CONFIG_STD_REAR_ENGINE:
                    LCD_Print(" Front Engine   ");
                    LCD_SetCursor(2, 0);
                    LCD_Print(">Rear Engine    ");
                    LCD_SetCursor(3, 0);
                    LCD_Print(" Customer       ");
                    break;
                case CONFIG_CUSTOMER:
                    LCD_Print(" Front Engine   ");
                    LCD_SetCursor(2, 0);
                    LCD_Print(" Rear Engine    ");
                    LCD_SetCursor(3, 0);
                    LCD_Print(">Customer       ");
                    break;
            }
            
            // Read buttons
            button_id = Buttons_Scan();
            
            // Process button press
            if (button_id == BTN_ID_DOWN) {
                if (selected_config < CONFIG_CUSTOMER) {
                    selected_config++;
                }
                // Wait for release
                while(Buttons_Scan() == BTN_ID_DOWN) {
                    __delay_ms(10);
                }
                __delay_ms(50);  // Debounce
            }
            else if (button_id == BTN_ID_UP) {
                if (selected_config > CONFIG_STD_FRONT_ENGINE) {
                    selected_config--;
                }
                // Wait for release
                while(Buttons_Scan() == BTN_ID_UP) {
                    __delay_ms(10);
                }
                __delay_ms(50);  // Debounce
            }
            else if (button_id == BTN_ID_SELECT) {
                done = 1;
                // Wait for release
                while(Buttons_Scan() == BTN_ID_SELECT) {
                    __delay_ms(10);
                }
            }
            __delay_ms(10);
        }
        
        // Show loading message
        LCD_Clear();
        LCD_SetCursor(0, 0);
        LCD_Print("Loading Config: ");
        LCD_SetCursor(1, 0);
        switch(selected_config) {
            case CONFIG_STD_FRONT_ENGINE:
                LCD_Print("Front Engine    ");
                break;
            case CONFIG_STD_REAR_ENGINE:
                LCD_Print("Rear Engine     ");
                break;
            case CONFIG_CUSTOMER:
                LCD_Print("Customer        ");
                break;
        }
        LCD_SetCursor(2, 0);
        LCD_Print("Please wait...  ");
        
        EEPROM_InitWithConfig(selected_config);
         LED_PIN = 0;
         
         LCD_SetCursor(1, 0);
         sprintf(display_buffer, "Complete!       ");
         LCD_Print(display_buffer);
         __delay_ms(2000);
     }
     
     LCD_Clear();
     LCD_SetCursor(0, 0);
     LCD_Print("Init CAN Config ");
     CAN_Config_Init();
     __delay_ms(500);
     
     LCD_SetCursor(1, 0);
     LCD_Print("Config Filters..");
     read_pgn = CAN_Config_GetReadPGN();
     read_sa = CAN_Config_GetReadSA();
     write_pgn = CAN_Config_GetWritePGN();
     write_sa = CAN_Config_GetWriteSA();
     
     J1939_ConfigureFilters(read_pgn, read_sa, write_pgn, write_sa);
     __delay_ms(500);
     
     LCD_Clear();
     LCD_SetCursor(0, 0);
     LCD_Print("Loading Cases...");
     EEPROM_Cases_Init();
     __delay_ms(500);
     
     // Scan inputs at startup and broadcast initial state
     LCD_SetCursor(1, 0);
     LCD_Print("Scan Inputs...  ");
     Inputs_Scan();
     __delay_ms(100);
     
     // Process any inputs that are already ON at startup
     for(uint8_t i = 0; i < 44; i++) {
         prev_input_states[i] = Inputs_GetState(i);
         if(prev_input_states[i] == 1) {  // Input is ON
             if(!Inputs_IsOneButtonStartInput(i)) {
                 EEPROM_HandleInputChange(i, 1);
             }
         }
     }
     
     // Initialize previous messages and broadcast all active messages once
     AggregatedMessage initial_messages[MAX_UNIQUE_MESSAGES];
     uint8_t initial_count = EEPROM_GetAggregatedMessages(initial_messages, MAX_UNIQUE_MESSAGES);
     
     // Store as previous (baseline for future comparisons)
     prev_msg_count = (initial_count < MAX_UNIQUE_MESSAGES) ? initial_count : MAX_UNIQUE_MESSAGES;
     for(uint8_t i = 0; i < prev_msg_count; i++) {
         prev_messages[i].pgn = initial_messages[i].pgn;
         prev_messages[i].source_addr = initial_messages[i].source_addr;
         for(uint8_t k = 0; k < 8; k++) {
             prev_messages[i].data[k] = initial_messages[i].data[k];
         }
         prev_messages[i].valid = initial_messages[i].valid;
     }
     
     // Broadcast all active messages once at startup
     for(uint8_t i = 0; i < initial_count; i++) {
         if(initial_messages[i].valid) {
             J1939_TransmitMessage(initial_messages[i].priority,
                                  initial_messages[i].pgn,
                                  initial_messages[i].source_addr,
                                  initial_messages[i].data);
         }
     }
     __delay_ms(100);
     
     LCD_Clear();
     LCD_SetCursor(0, 0);
     LCD_Print("MASTERCELL NGX  ");
     LCD_SetCursor(1, 0);
     LCD_Print("Ready!          ");
     __delay_ms(1000);
     
     Timer1_Init();
     
     // Start on main screen
     current_screen = SCREEN_MAIN;
     LCD_Clear();
     LCD_Backlight(1);
     backlight_timer = 5000;  // Start 5-second timer for main screen at startup
     DisplayMainScreen();
     
     while(1) {
         CAN_RxMessage can_msg;
         if (J1939_ReceiveMessage(&can_msg)) {
             last_rx_can_id = can_msg.id;
             last_rx_pgn = (can_msg.id >> 8) & 0xFFFF;
             
             uint8_t rx_sa = can_msg.id & 0xFF;
             uint16_t rx_pgn = (can_msg.id >> 8) & 0xFFFF;
             Network_UpdateDevice(rx_sa, rx_pgn, system_time_ms);
             
            if (CAN_Config_ProcessMessage((CAN_Message*)&can_msg)) {
                IEC0bits.T1IE = 0;
                if (led_on_timer == 0) {
                    led_on_timer = 50;
                }
                IEC0bits.T1IE = 1;
            }
            
            // Process climate control messages (PGN 0xFF99)
            if (Climate_ProcessMessage(can_msg.id, can_msg.data)) {
                IEC0bits.T1IE = 0;
                if (led_on_timer == 0) {
                    led_on_timer = 50;
                }
                IEC0bits.T1IE = 1;
            }
            
            // Process inLINK messages and trigger immediate broadcast if detected
             // Check if this is an inLINK message (PGN starting with A)
             uint16_t pgn = (can_msg.id >> 8) & 0xFFFF;
             uint8_t pgn_high = (pgn >> 8) & 0xFF;
             uint8_t inlink_detected = ((pgn_high & 0xF0) == 0xA0) ? 1 : 0;
             
             InLink_ProcessMessage(can_msg.id, can_msg.data);
             
             // Trigger broadcast if this was an inLINK message
             if(inlink_detected) {
                 // PHASE 3: Just set flag, remove redundant immediate call
                 IEC0bits.T1IE = 0;
                 state_changed = 1;
                 IEC0bits.T1IE = 1;
             }
         }
         
         if(led_on_timer > 0) {
             LED_PIN = 1;
         } else {
             LED_PIN = 0;
         }
         
         if(button_debounce_timer == 0) {
             uint8_t button = Buttons_Scan();
             if(button != BTN_ID_NONE && button != last_button) {
                 HandleButtonPress(button);
                 button_debounce_timer = 50;
                 display_timer = 0;
                 
                 // Turn on backlight and start timer for MAIN or MENU screens
                 if(current_screen == SCREEN_MAIN || current_screen == SCREEN_MENU) {
                     LCD_Backlight(1);
                     backlight_timer = 5000;  // 5 seconds
                 }
             }
             last_button = button;
         }
         
         // Manage backlight timeout for MAIN and MENU screens
         if((current_screen == SCREEN_MAIN || current_screen == SCREEN_MENU) && backlight_timer == 0) {
             LCD_Backlight(0);  // Turn off backlight after timeout
         }
         
         if(pattern_changed) {
             IEC0bits.T1IE = 0;
             pattern_changed = 0;
             IEC0bits.T1IE = 1;
             
             // PHASE 3: Pass PATTERN_TICK reason
             TransmitAggregatedMessages(BROADCAST_REASON_PATTERN_TICK);
         }
         
         // PHASE 2: Check for state changes (input or inLINK)
         if(state_changed) {
             IEC0bits.T1IE = 0;
             state_changed = 0;
             IEC0bits.T1IE = 1;
             
             // PHASE 3: Pass STATE_CHANGE reason
             TransmitAggregatedMessages(BROADCAST_REASON_STATE_CHANGE);
         }
         
         // Check if heartbeat should be sent (set in timer interrupt)
         if(heartbeat_pending) {
             IEC0bits.T1IE = 0;
             heartbeat_pending = 0;
             IEC0bits.T1IE = 1;
             
             J1939_TransmitHeartbeat();
         }
         
         if(scan_timer == 0) {
             Inputs_Scan();
             scan_timer = 10;
             
             if(Inputs_OneButtonStartStateChanged()) {
                 // PHASE 3: Just set flag, remove redundant immediate call
                 IEC0bits.T1IE = 0;
                 state_changed = 1;
                 IEC0bits.T1IE = 1;
             }
             
             for(uint8_t i = 0; i < 44; i++) {
                 uint8_t current_state = Inputs_GetState(i);
                 
                 if(current_state != prev_input_states[i]) {
                     prev_input_states[i] = current_state;
                     last_input_triggered = i;
                     
                     IEC0bits.T1IE = 0;
                     if(led_on_timer == 0) {
                         led_on_timer = 50;
                     }
                     IEC0bits.T1IE = 1;
                     
                     if(!Inputs_IsOneButtonStartInput(i)) {
                         EEPROM_HandleInputChange(i, current_state);
                     }
                     
                     // PHASE 3: Just set flag, remove redundant immediate call
                     IEC0bits.T1IE = 0;
                     state_changed = 1;
                     IEC0bits.T1IE = 1;
                 }
             }
         }
         
         if(display_timer == 0) {
             display_timer = 500;
             
             Network_CheckTimeouts(system_time_ms);
             
             switch(current_screen) {
                 case SCREEN_MAIN:
                     DisplayMainScreen();
                     break;
                 case SCREEN_MENU:
                     DisplayMenuScreen();
                     break;
                 case SCREEN_SWITCH:
                     DisplaySwitchScreen();
                     break;
                 case SCREEN_INVENTORY:
                     DisplayInventoryScreen();
                     break;
                 case SCREEN_SYSTEM_INFO:
                     DisplaySystemInfoScreen();
                     break;
                 case SCREEN_DEBUG:
                     DisplayDebugScreen();
                     break;
             }
         }
     }
     
     return 0;
 }
 
 void InitUnusedPins(void) {
     // ========================================================================
     // MOSFET Gate Drivers - CRITICAL: Must be driven LOW to turn off MOSFETs
     // Floating gates can partially turn on MOSFETs causing excessive current
     // ========================================================================
     
     TRISBbits.TRISB15 = 0;  LATBbits.LATB15 = 0;  // RB15 - MOSFET gate OFF
     TRISFbits.TRISF2 = 0;   LATFbits.LATF2 = 0;   // RF2 - MOSFET gate OFF
     TRISFbits.TRISF3 = 0;   LATFbits.LATF3 = 0;   // RF3 - MOSFET gate OFF
     TRISFbits.TRISF4 = 0;   LATFbits.LATF4 = 0;   // RF4 - MOSFET gate OFF
     TRISFbits.TRISF5 = 0;   LATFbits.LATF5 = 0;   // RF5 - MOSFET gate OFF
     TRISFbits.TRISF6 = 0;   LATFbits.LATF6 = 0;   // RF6 - MOSFET gate OFF
     TRISGbits.TRISG2 = 0;   LATGbits.LATG2 = 0;   // RG2 - MOSFET gate OFF
     TRISGbits.TRISG3 = 0;   LATGbits.LATG3 = 0;   // RG3 - MOSFET gate OFF
     
    // ========================================================================
    // Digital Potentiometer - Now handled by Climate_Init()
    // Pins RG6, RG7, RG8 (SPI2) and RB2, RB3, RB4 (control)
    // are configured in climate.c for MCP4341 control
    // ========================================================================
     
     // ========================================================================
     // Voltage-to-Frequency Converters & Analog Input
     // ========================================================================
     
     TRISBbits.TRISB8 = 1;  // RB8 - Frequency input
     TRISBbits.TRISB9 = 1;  // RB9 - Frequency input
     TRISBbits.TRISB1 = 1;  // RB1 - Analog input
 }
 
 void HandleButtonPress(uint8_t button) {
     switch(current_screen) {
         case SCREEN_MAIN:
             if(button == BTN_ID_HOME) {
                 current_screen = SCREEN_MENU;
                 menu_selection = 0;
                 menu_scroll_position = 0;
                 LCD_Clear();
                 LCD_Backlight(1);
                 backlight_timer = 5000;  // Start 5-second timer for menu
                 DisplayMenuScreen();
             }
             break;
             
         case SCREEN_MENU:
             if(button == BTN_ID_UP) {
                 if(menu_selection > 0) {
                     menu_selection--;
                     if(menu_selection < menu_scroll_position) {
                         menu_scroll_position = menu_selection;
                     }
                     DisplayMenuScreen();
                 }
             } else if(button == BTN_ID_DOWN) {
                 if(menu_selection < MENU_COUNT - 1) {
                     menu_selection++;
                     if(menu_selection >= menu_scroll_position + 3) {
                         menu_scroll_position = menu_selection - 2;
                     }
                     DisplayMenuScreen();
                 }
             } else if(button == BTN_ID_SELECT) {
                 switch(menu_selection) {
                     case MENU_SWITCH_STATES:
                         current_screen = SCREEN_SWITCH;
                         LCD_Clear();
                         DisplaySwitchScreen();
                         break;
                     case MENU_SYSTEM_INVENTORY:
                         current_screen = SCREEN_INVENTORY;
                         inventory_scroll_position = 0;
                         LCD_Clear();
                         DisplayInventoryScreen();
                         break;
                     case MENU_SYSTEM_INFO:
                         current_screen = SCREEN_SYSTEM_INFO;
                         LCD_Clear();
                         DisplaySystemInfoScreen();
                         break;
                     case MENU_DEBUG:
                         current_screen = SCREEN_DEBUG;
                         LCD_Clear();
                         DisplayDebugScreen();
                         break;
                     case MENU_HOME_SCREEN:
                         current_screen = SCREEN_MAIN;
                         LCD_Clear();
                         LCD_Backlight(1);
                         backlight_timer = 5000;  // Start 5-second timer for main screen
                         DisplayMainScreen();
                         break;
                 }
             } else if(button == BTN_ID_HOME) {
                 current_screen = SCREEN_MAIN;
                 LCD_Clear();
                 LCD_Backlight(1);
                 backlight_timer = 5000;  // Start 5-second timer for main screen
                 DisplayMainScreen();
             }
             break;
             
         case SCREEN_SWITCH:
         case SCREEN_INVENTORY:
         case SCREEN_SYSTEM_INFO:
             if(button == BTN_ID_HOME) {
                 current_screen = SCREEN_MENU;
                 inventory_scroll_position = 0;
                 LCD_Clear();
                 LCD_Backlight(1);
                 backlight_timer = 5000;  // Start 5-second timer for menu
                 DisplayMenuScreen();
             } else if(current_screen == SCREEN_INVENTORY) {
                 uint8_t device_count = Network_GetDeviceCount();
                 if(button == BTN_ID_UP) {
                     if(inventory_scroll_position > 0) {
                         inventory_scroll_position--;
                         DisplayInventoryScreen();
                     }
                 } else if(button == BTN_ID_DOWN) {
                     if(device_count > 3 && inventory_scroll_position < device_count - 3) {
                         inventory_scroll_position++;
                         DisplayInventoryScreen();
                     }
                 }
             }
             break;
             
         case SCREEN_DEBUG:
             if(button == BTN_ID_HOME) {
                 current_screen = SCREEN_MAIN;
                 LCD_Clear();
                 LCD_Backlight(1);
                 backlight_timer = 5000;  // Start 5-second timer for main screen
                 DisplayMainScreen();
             }
             break;
     }
 }
 
 void DisplayMainScreen(void) {
     LCD_SetCursor(0, 0);
     LCD_Print("  INFINITYBOX   ");
     
     LCD_SetCursor(1, 0);
     LCD_Print("IPM POWER SYSTEM");
     
     LCD_SetCursor(2, 0);
     uint8_t tx_ok = J1939_IsTxReady();
     uint8_t rx_ok = !J1939_HasRxOverflow();
     
     if(tx_ok && rx_ok) {
         LCD_Print("CAN: TX-OK RX-OK");
     } else if(!tx_ok && rx_ok) {
         LCD_Print("CAN: TX-ER RX-OK");
     } else if(tx_ok && !rx_ok) {
         LCD_Print("CAN: TX-OK RX-OV");
     } else {
         LCD_Print("CAN: TX-ER RX-OV");
     }
     
     LCD_SetCursor(3, 0);
     uint8_t ignition = Inputs_GetIgnitionState();
     if(ignition) {
         LCD_Print("IGN: ON  SEC:OFF");
     } else {
         LCD_Print("IGN: OFF SEC:OFF");
     }
 }
 
 void DisplayMenuScreen(void) {
     LCD_SetCursor(0, 0);
     LCD_Print("--- MAIN MENU --");
     
     for(uint8_t line = 0; line < 3; line++) {
         LCD_SetCursor(line + 1, 0);
         uint8_t item = menu_scroll_position + line;
         
         if(item >= MENU_COUNT) {
             LCD_Print("                ");
             continue;
         }
         
         char cursor = (item == menu_selection) ? '>' : ' ';
         
         switch(item) {
             case MENU_SWITCH_STATES:
                 LCD_Print(cursor == '>' ? ">SWITCH STATES  " : " SWITCH STATES  ");
                 break;
             case MENU_SYSTEM_INVENTORY:
                 LCD_Print(cursor == '>' ? ">SYSTEM INV     " : " SYSTEM INV     ");
                 break;
             case MENU_SYSTEM_INFO:
                 LCD_Print(cursor == '>' ? ">SYSTEM INFO    " : " SYSTEM INFO    ");
                 break;
             case MENU_DEBUG:
                 LCD_Print(cursor == '>' ? ">DEBUG          " : " DEBUG          ");
                 break;
             case MENU_HOME_SCREEN:
                 LCD_Print(cursor == '>' ? ">HOME SCREEN    " : " HOME SCREEN    ");
                 break;
         }
     }
 }
 
 void DisplaySwitchScreen(void) {
     char display_buffer[17];
     
     // Keep backlight on for sub-menu screens
     LCD_Backlight(1);
     backlight_timer = 0;  // Disable timer so it stays on
     
     LCD_SetCursor(0, 0);
     LCD_Print("SWITCH STATES   ");
     
     LCD_SetCursor(1, 0);
     for(uint8_t i = 0; i < 16; i++) {
         display_buffer[i] = Inputs_GetState(i) ? '1' : '0';
     }
     display_buffer[16] = '\0';
     LCD_Print(display_buffer);
     
     LCD_SetCursor(2, 0);
     for(uint8_t i = 16; i < 32; i++) {
         display_buffer[i-16] = Inputs_GetState(i) ? '1' : '0';
     }
     display_buffer[16] = '\0';
     LCD_Print(display_buffer);
     
     LCD_SetCursor(3, 0);
     for(uint8_t i = 32; i < 38; i++) {
         display_buffer[i-32] = Inputs_GetState(i) ? '1' : '0';
     }
     display_buffer[6] = ' ';
     for(uint8_t i = 38; i < 44; i++) {
         display_buffer[i-31] = Inputs_GetState(i) ? '1' : '0';
     }
     for(uint8_t i = 13; i < 16; i++) {
         display_buffer[i] = ' ';
     }
     display_buffer[16] = '\0';
     LCD_Print(display_buffer);
 }
 
 void DisplayInventoryScreen(void) {
     char display_buffer[17];
     uint8_t device_count = Network_GetDeviceCount();
     
     // Keep backlight on for sub-menu screens
     LCD_Backlight(1);
     backlight_timer = 0;  // Disable timer so it stays on
     
     // Build a friendly device list
     // We'll create "virtual" devices for display purposes
     typedef struct {
         uint16_t display_pgn;  // PGN to show (may be virtual like FF01 for POWERCELL)
         char name[12];         // Friendly name
         uint8_t valid;         // 1 if this entry should be displayed
     } DisplayDevice;
     
     DisplayDevice display_list[16];
     uint8_t display_count = 0;
     
     // Track which POWERCELLs we've found
     uint8_t found_ff11 = 0, found_ff21 = 0;  // Front POWERCELL
     uint8_t found_ff12 = 0, found_ff22 = 0;  // Rear POWERCELL
     
     // First pass: identify all devices and check for POWERCELL pairs
     for(uint8_t i = 0; i < device_count; i++) {
         NetworkDevice* device = Network_GetDevice(i);
         if(device != NULL && device->active) {
             if(device->pgn == 0xFF11) found_ff11 = 1;
             if(device->pgn == 0xFF21) found_ff21 = 1;
             if(device->pgn == 0xFF12) found_ff12 = 1;
             if(device->pgn == 0xFF22) found_ff22 = 1;
         }
     }
     
     // Second pass: build display list
     for(uint8_t i = 0; i < device_count && display_count < 16; i++) {
         NetworkDevice* device = Network_GetDevice(i);
         if(device == NULL || !device->active) continue;
         
         // Filter out inLINK devices (PGNs starting with AF)
         if((device->pgn & 0xFF00) == 0xAF00) {
             continue;  // Skip inLINK devices
         }
         
         // Check for inMOTION devices (FF33-FF36)
         if(device->pgn == 0xFF33) {
             display_list[display_count].display_pgn = 0xFF03;
             sprintf(display_list[display_count].name, "DF inMOT");
             display_list[display_count].valid = 1;
             display_count++;
         }
         else if(device->pgn == 0xFF34) {
             display_list[display_count].display_pgn = 0xFF04;
             sprintf(display_list[display_count].name, "PF inMOT");
             display_list[display_count].valid = 1;
             display_count++;
         }
         else if(device->pgn == 0xFF35) {
             display_list[display_count].display_pgn = 0xFF05;
             sprintf(display_list[display_count].name, "DR inMOT");
             display_list[display_count].valid = 1;
             display_count++;
         }
         else if(device->pgn == 0xFF36) {
             display_list[display_count].display_pgn = 0xFF06;
             sprintf(display_list[display_count].name, "PR inMOT");
             display_list[display_count].valid = 1;
             display_count++;
         }
         // Check for Front POWERCELL (FF11 is first message, only add entry once)
         else if(device->pgn == 0xFF11 && found_ff21) {
             // Only add if we have both FF11 and FF21
             // Check if we already added this POWERCELL
             uint8_t already_added = 0;
             for(uint8_t j = 0; j < display_count; j++) {
                 if(display_list[j].display_pgn == 0xFF01) {
                     already_added = 1;
                     break;
                 }
             }
             if(!already_added) {
                 display_list[display_count].display_pgn = 0xFF01;
                 sprintf(display_list[display_count].name, "FRONT PC");
                 display_list[display_count].valid = 1;
                 display_count++;
             }
         }
         // Check for Rear POWERCELL (FF12 is first message, only add entry once)
         else if(device->pgn == 0xFF12 && found_ff22) {
             // Only add if we have both FF12 and FF22
             // Check if we already added this POWERCELL
             uint8_t already_added = 0;
             for(uint8_t j = 0; j < display_count; j++) {
                 if(display_list[j].display_pgn == 0xFF02) {
                     already_added = 1;
                     break;
                 }
             }
             if(!already_added) {
                 display_list[display_count].display_pgn = 0xFF02;
                 sprintf(display_list[display_count].name, "REAR PC");
                 display_list[display_count].valid = 1;
                 display_count++;
             }
         }
         // Skip FF21, FF22 as they're part of POWERCELL pairs
         else if(device->pgn == 0xFF21 || device->pgn == 0xFF22) {
             // Already handled by FF11/FF12 logic above
             continue;
         }
         // For now, only show POWERCELLs and inMOTION devices
         // All other devices are filtered out
     }
     
     // Display the list
     LCD_SetCursor(0, 0);
     sprintf(display_buffer, "SYSTEM INV (%d) ", display_count);
     LCD_Print(display_buffer);
     
     for(uint8_t line = 0; line < 3; line++) {
         LCD_SetCursor(line + 1, 0);
         
         uint8_t device_index = inventory_scroll_position + line;
         if(device_index < display_count && display_list[device_index].valid) {
             if(line == 0) {
                 sprintf(display_buffer, ">%04X %-10s", 
                         display_list[device_index].display_pgn,
                         display_list[device_index].name);
             } else {
                 sprintf(display_buffer, " %04X %-10s", 
                         display_list[device_index].display_pgn,
                         display_list[device_index].name);
             }
             LCD_Print(display_buffer);
         } else {
             LCD_Print("                ");
         }
     }
 }
 
 void DisplaySystemInfoScreen(void) {
     char display_buffer[17];
     uint8_t fw_major, fw_minor;
     uint8_t customer_name[4];
     
     // Keep backlight on for sub-menu screens
     LCD_Backlight(1);
     backlight_timer = 0;  // Disable timer so it stays on
     
     // Read firmware version from EEPROM
     fw_major = EEPROM_Config_ReadByte(EEPROM_CFG_FW_MAJOR);
     fw_minor = EEPROM_Config_ReadByte(EEPROM_CFG_FW_MINOR);
     
     // Read customer name from EEPROM
     customer_name[0] = EEPROM_Config_ReadByte(EEPROM_CFG_CUSTOMER_NAME_1);
     customer_name[1] = EEPROM_Config_ReadByte(EEPROM_CFG_CUSTOMER_NAME_2);
     customer_name[2] = EEPROM_Config_ReadByte(EEPROM_CFG_CUSTOMER_NAME_3);
     customer_name[3] = EEPROM_Config_ReadByte(EEPROM_CFG_CUSTOMER_NAME_4);
     
     // Line 1: SYSTEM INFO
     LCD_SetCursor(0, 0);
     LCD_Print("SYSTEM INFO     ");
     
     // Line 2: Software Ver: X
     LCD_SetCursor(1, 0);
     sprintf(display_buffer, "Software Ver: %u ", fw_major);
     LCD_Print(display_buffer);
     
     // Line 3: Customer Ver: X
     LCD_SetCursor(2, 0);
     sprintf(display_buffer, "Customer Ver: %u ", fw_minor);
     LCD_Print(display_buffer);
     
     // Line 4: CUSTOMER: XXXX
     LCD_SetCursor(3, 0);
     sprintf(display_buffer, "CUSTOMER: %c%c%c%c  ",
             customer_name[0], customer_name[1], 
             customer_name[2], customer_name[3]);
     LCD_Print(display_buffer);
 }
 
 void DisplayDebugScreen(void) {
     char display_buffer[17];
     
     // Keep backlight on for sub-menu screens
     LCD_Backlight(1);
     backlight_timer = 0;  // Disable timer so it stays on
     
     // Display prev_messages status for FF03-FF06 (inLINK messages)
     // This helps diagnose the stuck message issue
     
     // Line 0: FF03 status
     LCD_SetCursor(0, 0);
     sprintf(display_buffer, "FF03:");
     LCD_Print(display_buffer);
     
     // Search for FF03 in prev_messages
     uint8_t found = 0;
     for(uint8_t i = 0; i < prev_msg_count; i++) {
         if(prev_messages[i].valid && 
            prev_messages[i].pgn == 0xFF03 && 
            prev_messages[i].source_addr == 0x1A) {
             LCD_SetCursor(0, 5);
             sprintf(display_buffer, "%02X %02X %02X", 
                     prev_messages[i].data[0],
                     prev_messages[i].data[1],
                     prev_messages[i].data[2]);
             LCD_Print(display_buffer);
             found = 1;
             break;
         }
     }
     if(!found) {
         LCD_SetCursor(0, 5);
         LCD_Print("-- -- --   ");
     }
     
     // Line 1: FF04 status
     LCD_SetCursor(1, 0);
     sprintf(display_buffer, "FF04:");
     LCD_Print(display_buffer);
     
     found = 0;
     for(uint8_t i = 0; i < prev_msg_count; i++) {
         if(prev_messages[i].valid && 
            prev_messages[i].pgn == 0xFF04 && 
            prev_messages[i].source_addr == 0x1A) {
             LCD_SetCursor(1, 5);
             sprintf(display_buffer, "%02X %02X %02X", 
                     prev_messages[i].data[0],
                     prev_messages[i].data[1],
                     prev_messages[i].data[2]);
             LCD_Print(display_buffer);
             found = 1;
             break;
         }
     }
     if(!found) {
         LCD_SetCursor(1, 5);
         LCD_Print("-- -- --   ");
     }
     
     // Line 2: FF05 status
     LCD_SetCursor(2, 0);
     sprintf(display_buffer, "FF05:");
     LCD_Print(display_buffer);
     
     found = 0;
     for(uint8_t i = 0; i < prev_msg_count; i++) {
         if(prev_messages[i].valid && 
            prev_messages[i].pgn == 0xFF05 && 
            prev_messages[i].source_addr == 0x1A) {
             LCD_SetCursor(2, 5);
             sprintf(display_buffer, "%02X %02X %02X", 
                     prev_messages[i].data[0],
                     prev_messages[i].data[1],
                     prev_messages[i].data[2]);
             LCD_Print(display_buffer);
             found = 1;
             break;
         }
     }
     if(!found) {
         LCD_SetCursor(2, 5);
         LCD_Print("-- -- --   ");
     }
     
     // Line 3: FF06 status
     LCD_SetCursor(3, 0);
     sprintf(display_buffer, "FF06:");
     LCD_Print(display_buffer);
     
     found = 0;
     for(uint8_t i = 0; i < prev_msg_count; i++) {
         if(prev_messages[i].valid && 
            prev_messages[i].pgn == 0xFF06 && 
            prev_messages[i].source_addr == 0x1A) {
             LCD_SetCursor(3, 5);
             sprintf(display_buffer, "%02X %02X %02X", 
                     prev_messages[i].data[0],
                     prev_messages[i].data[1],
                     prev_messages[i].data[2]);
             LCD_Print(display_buffer);
             found = 1;
             break;
         }
     }
     if(!found) {
         LCD_SetCursor(3, 5);
         LCD_Print("-- -- --   ");
     }
     
     // Display debug counts on right side
     LCD_SetCursor(0, 14);
 }
 
 void TransmitAggregatedMessages(uint8_t reason) {
     AggregatedMessage messages[MAX_UNIQUE_MESSAGES];
     uint8_t msg_count = EEPROM_GetAggregatedMessages(messages, MAX_UNIQUE_MESSAGES);
     last_msg_count = msg_count;
     
     // Compare with previous messages to detect changes
     for(uint8_t i = 0; i < msg_count; i++) {
         if(messages[i].valid) {
             // Look for this PGN/SA in previous messages
             uint8_t found_prev = 0;
             for(uint8_t j = 0; j < prev_msg_count; j++) {
                 if(prev_messages[j].valid &&
                    prev_messages[j].pgn == messages[i].pgn &&
                    prev_messages[j].source_addr == messages[i].source_addr) {
                     // Found matching previous message - compare data
                     found_prev = 1;
                     uint8_t data_differs = 0;
                     for(uint8_t k = 0; k < 8; k++) {
                         if(prev_messages[j].data[k] != messages[i].data[k]) {
                             data_differs = 1;
                             break;
                         }
                     }
                     messages[i].data_changed = data_differs;
                     break;
                 }
             }
             // If not found in previous, it's a new message (data changed by definition)
             if(!found_prev) {
                 messages[i].data_changed = 1;
             }
         }
     }
     
     // PHASE 3: CONDITIONAL TRANSMISSION LOGIC
     // Transmit and track what was actually sent
     uint8_t transmitted_count = 0;
     PreviousMessage transmitted_this_cycle[MAX_UNIQUE_MESSAGES];
     
     for(uint8_t i = 0; i < msg_count; i++) {
         if(messages[i].valid) {
             uint8_t should_transmit = 0;
             
            if(reason == BROADCAST_REASON_PATTERN_TICK) {
                // Pattern timer: Always transmit pattern messages when timer fires
                // The pattern timer firing IS the trigger to update the output
                if(messages[i].has_pattern) {
                    should_transmit = 1;
                }
            }
             else if(reason == BROADCAST_REASON_STATE_CHANGE) {
                 // State change: Only transmit messages that actually changed
                 if(messages[i].data_changed) {
                     should_transmit = 1;
                 }
             }
             
             if(should_transmit) {
                 J1939_TransmitMessage(messages[i].priority, 
                                      messages[i].pgn, 
                                      messages[i].source_addr,
                                      messages[i].data);
                 
                 // FIX: Store this message as it was actually transmitted
                 if(transmitted_count < MAX_UNIQUE_MESSAGES) {
                     transmitted_this_cycle[transmitted_count].pgn = messages[i].pgn;
                     transmitted_this_cycle[transmitted_count].source_addr = messages[i].source_addr;
                     for(uint8_t k = 0; k < 8; k++) {
                         transmitted_this_cycle[transmitted_count].data[k] = messages[i].data[k];
                     }
                     transmitted_this_cycle[transmitted_count].valid = 1;
                     transmitted_count++;
                 }
             }
         }
     }
     
     // FIX: Update prev_messages with only what was transmitted
     // Keep untransmitted messages in prev_messages (they haven't changed)
     for(uint8_t i = 0; i < transmitted_count; i++) {
         // Find this PGN/SA in prev_messages and update it
         uint8_t found = 0;
         for(uint8_t j = 0; j < prev_msg_count; j++) {
             if(prev_messages[j].valid &&
                prev_messages[j].pgn == transmitted_this_cycle[i].pgn &&
                prev_messages[j].source_addr == transmitted_this_cycle[i].source_addr) {
                 // Update existing entry
                 for(uint8_t k = 0; k < 8; k++) {
                     prev_messages[j].data[k] = transmitted_this_cycle[i].data[k];
                 }
                 found = 1;
                 break;
             }
         }
         
         // If not found, add new entry
         if(!found && prev_msg_count < MAX_UNIQUE_MESSAGES) {
             prev_messages[prev_msg_count].pgn = transmitted_this_cycle[i].pgn;
             prev_messages[prev_msg_count].source_addr = transmitted_this_cycle[i].source_addr;
             for(uint8_t k = 0; k < 8; k++) {
                 prev_messages[prev_msg_count].data[k] = transmitted_this_cycle[i].data[k];
             }
             prev_messages[prev_msg_count].valid = 1;
             prev_msg_count++;
         }
     }
     
     // Remove one-shot cases (OFF/clearing cases) after transmission
     EEPROM_RemoveMarkedCases();
 }
 
 void Timer1_Init(void) {
     T1CON = 0x0000;
     T1CONbits.TCKPS = 2;
     TMR1 = 0;
     PR1 = 249;
     IFS0bits.T1IF = 0;
     IEC0bits.T1IE = 1;
     T1CONbits.TON = 1;
 }
 
 void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void) {
     IFS0bits.T1IF = 0;
     
     system_time_ms++;
     
     if(led_on_timer > 0) led_on_timer--;
     if(scan_timer > 0) scan_timer--;
     if(display_timer > 0) display_timer--;
     if(button_debounce_timer > 0) button_debounce_timer--;
     if(backlight_timer > 0) backlight_timer--;
     
     pattern_timer++;
     if(pattern_timer >= 250) {
         pattern_timer = 0;
         EEPROM_Pattern_UpdateTimers();
         pattern_changed = 1;
     }
     
     if(j1939_timer > 0) {
         j1939_timer--;
         if(j1939_timer == 0) {
             j1939_timer = 1000;
             led_on_timer = 50;
             heartbeat_pending = 1;  // Set flag instead of transmitting in ISR
         }
     }
 }