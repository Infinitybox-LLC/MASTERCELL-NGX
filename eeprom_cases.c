/*
 * FILE: eeprom_cases.c
 * EEPROM Case Management - WITH PATTERN TIMING SUPPORT
 * 
 * Pattern Timing - Option 2: Mixed Pattern/Non-Pattern Cases
 * - Each input has ONE pattern timer shared across all its cases
 * - Cases with pattern timing (byte 7 non-zero) follow the timer (flash on/off)
 * - Cases without pattern timing (byte 7 = 0x00) always transmit (solid)
 * - Multiple cases for same input can mix pattern and non-pattern
 * 
 * Pattern Timing Details:
 * - Byte 7 of EEPROM case: Upper nibble = ON time, Lower nibble = OFF time
 * - Each count = 250ms (0x44 = 1s ON, 1s OFF)
 * - Patterns repeat continuously while input is active
 * - Pattern bits are OR'd with non-pattern bits during aggregation
 */

 #include "eeprom_cases.h"
 #include "inputs.h"
 #include "inlink.h"
 #include <string.h>
 
 // Case count lookup tables - RESTRUCTURED LAYOUT (106 ON + 20 OFF)
 const uint8_t input_on_case_count[TOTAL_INPUTS] = {
     4, 2, 4, 4, 2, 6, 1, 6,      // IN01-IN08: Ignition, Starter, Turns, Headlights, Parking, High, 4-Ways
     1, 2, 2, 2, 2, 2, 6, 2,      // IN09-IN16: Horn, Fan, Brakes, Fuel, Open, OneButton, Neutral
     2, 6, 2, 2, 2, 2, 6, 6,      // IN17-IN24: Backup, Interior, Opens, Door Lock/Unlock
     2, 2, 2, 2, 2, 2, 2, 2,      // IN25-IN32: Window controls (all 2 cases each)
     1, 1, 1, 1, 1, 1,            // IN33-IN38: Aux inputs
     2, 2, 1, 1, 1, 1             // HSIN01-HSIN06: HS Cooling, HS Fuel, HS Aux
 };
 
 const uint8_t input_off_case_count[TOTAL_INPUTS] = {
     2, 2, 0, 0, 0, 0, 0, 0,      // IN01-IN08: Ignition & Starter have OFF cases
     0, 0, 0, 0, 0, 0, 0, 0,      // IN09-IN16: No OFF cases
     0, 0, 0, 0, 0, 0, 0, 0,      // IN17-IN24: No OFF cases
     2, 2, 2, 2, 2, 2, 2, 2,      // IN25-IN32: All window controls have OFF cases
     0, 0, 0, 0, 0, 0,            // IN33-IN38: No OFF cases
     0, 0, 0, 0, 0, 0             // HSIN01-HSIN06: No OFF cases
 };
 
 // Pre-calculated BYTE offsets from start of ON/OFF case regions - RESTRUCTURED LAYOUT
 // ON cases start at 0x0022, total 106 cases (3392 bytes)
 static const uint16_t on_case_offsets[TOTAL_INPUTS] = {
     0, 128, 192, 320, 448, 512, 704, 736,
     928, 960, 1024, 1088, 1152, 1216, 1280, 1472,
     1536, 1600, 1792, 1856, 1920, 1984, 2048, 2240,
     2432, 2496, 2560, 2624, 2688, 2752, 2816, 2880,
     2944, 2976, 3008, 3040, 3072, 3104, 3136, 3200,
     3264, 3296, 3328, 3360
 };
 
 // OFF cases start at 0x0D62, total 20 cases (640 bytes)
 // Note: There's a 32-byte offset in EEPROM - IN25-32 start 32 bytes later than originally calculated
 static const uint16_t off_case_offsets[TOTAL_INPUTS] = {
     0, 64, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
     0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
     0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
     160, 224, 288, 352, 416, 480, 544, 608,  // IN25-32 corrected offsets (added 32 to each)
     0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
     0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF
 };
 
 // Active cases tracking
 static ActiveCase active_cases[MAX_ACTIVE_CASES];
 static uint8_t active_case_count = 0;
 
 // Pattern timers for each input (0-43)
 static PatternTimer pattern_timers[TOTAL_INPUTS];
 
 // Diagnostic counters
 static uint16_t eeprom_read_count = 0;
 static uint16_t bounds_errors = 0;
 
 void EEPROM_Cases_Init(void) {
     EEPROM_ClearActiveCases();
     eeprom_read_count = 0;
     bounds_errors = 0;
     
     // Initialize all pattern timers to inactive
     for(uint8_t i = 0; i < TOTAL_INPUTS; i++) {
         pattern_timers[i].state = PATTERN_STATE_INACTIVE;
         pattern_timers[i].timer = 0;
         pattern_timers[i].on_time = 0;
         pattern_timers[i].off_time = 0;
         pattern_timers[i].has_pattern = 0;
     }
     
     // At startup, all inputs are OFF, so load all valid OFF cases
 // COMMENTED OUT:     for(uint8_t input_num = 0; input_num < TOTAL_INPUTS; input_num++) {
 // COMMENTED OUT:         uint8_t num_off_cases = input_off_case_count[input_num];
 // COMMENTED OUT:         
 // COMMENTED OUT:         // Skip inputs with no OFF cases
 // COMMENTED OUT:         if(num_off_cases == 0) {
 // COMMENTED OUT:             continue;
 // COMMENTED OUT:         }
 // COMMENTED OUT:         
 // COMMENTED OUT:         // Limit to prevent buffer overflow
 // COMMENTED OUT:         if(num_off_cases > MAX_OFF_CASES_PER_INPUT) {
 // COMMENTED OUT:             num_off_cases = MAX_OFF_CASES_PER_INPUT;
 // COMMENTED OUT:         }
 // COMMENTED OUT:         
 // COMMENTED OUT:         for(uint8_t i = 0; i < num_off_cases; i++) {
 // COMMENTED OUT:             // Check if we have room before processing
 // COMMENTED OUT:             if(active_case_count >= MAX_ACTIVE_CASES) {
 // COMMENTED OUT:                 return;  // Can't add more cases
 // COMMENTED OUT:             }
 // COMMENTED OUT:             
 // COMMENTED OUT:             // Get OFF case address
 // COMMENTED OUT:             uint16_t addr = EEPROM_GetCaseAddress(input_num, i, 0);
 // COMMENTED OUT:             
 // COMMENTED OUT:             // Skip if address is invalid
 // COMMENTED OUT:             if(addr == 0xFFFF) {
 // COMMENTED OUT:                 continue;
 // COMMENTED OUT:             }
 // COMMENTED OUT:             
 // COMMENTED OUT:             // Final bounds check before read
 // COMMENTED OUT:             if(addr >= 0x1000) {
 // COMMENTED OUT:                 bounds_errors++;
 // COMMENTED OUT:                 continue;
 // COMMENTED OUT:             }
 // COMMENTED OUT:             
 // COMMENTED OUT:             // Try to read the case
 // COMMENTED OUT:             CaseData case_data;
 // COMMENTED OUT:             
 // COMMENTED OUT:             if(EEPROM_ReadCase(addr, &case_data)) {
 // COMMENTED OUT:                 // Successfully read valid OFF case - add to active list
 // COMMENTED OUT:                 active_cases[active_case_count].input_num = input_num;
 // COMMENTED OUT:                 active_cases[active_case_count].case_num = i;
 // COMMENTED OUT:                 active_cases[active_case_count].is_on_case = 0;  // OFF case
 // COMMENTED OUT:                 active_cases[active_case_count].case_data = case_data;
 // COMMENTED OUT:                 active_case_count++;
 // COMMENTED OUT:             }
 // COMMENTED OUT:         }
 // COMMENTED OUT:     }
 }
 
 uint16_t EEPROM_GetCaseAddress(uint8_t input_num, uint8_t case_num, uint8_t is_on_case) {
     // Validate input number FIRST (fast exit)
     if(input_num >= TOTAL_INPUTS) {
         bounds_errors++;
         return 0xFFFF;
     }
     
     uint16_t address;
     
     if(is_on_case) {
         // Validate case number (fast exit)
         if(case_num >= input_on_case_count[input_num]) {
             bounds_errors++;
             return 0xFFFF;
         }
         
         // Calculate BYTE address
         address = EEPROM_CASES_START + on_case_offsets[input_num] + (case_num * CASE_SIZE);
     } else {
         // OFF case logic
         if(case_num >= input_off_case_count[input_num]) {
             bounds_errors++;
             return 0xFFFF;
         }
         
         if(off_case_offsets[input_num] == 0xFFFF) {
             return 0xFFFF;  // No OFF cases for this input
         }
         
         // Calculate BYTE address
         address = 0x0D62 + off_case_offsets[input_num] + (case_num * CASE_SIZE);
     }
     
     // CRITICAL: Bounds check the calculated address
     if(address >= 0x1000) {  // Beyond 4KB EEPROM
         bounds_errors++;
         return 0xFFFF;
     }
     
     if(address < EEPROM_CASES_START && address < 0x0D62) {
         bounds_errors++;
         return 0xFFFF;
     }
     
     // Verify word alignment
     if(address & 0x01) {
         bounds_errors++;
         return 0xFFFF;
     }
     
     return address;
 }
 
 /*
  * CRITICAL FIX: Corrected EEPROM read function
  * 
  * dsPIC30F EEPROM memory map:
  * - EEPROM is at 0x7FF000 to 0x7FFFFE
  * - TBLPAG must be 0x7F
  * - Offset within page is 0xF000 to 0xFFFF
  */
 static uint16_t ReadEEPROMWord(uint16_t byte_address) {
     // CRITICAL: Bounds check FIRST
     if(byte_address >= 0x1000) {
         bounds_errors++;
         return 0xFFFF;
     }
     
     // Round down to even address (word boundary)
     uint16_t word_address = byte_address & 0xFFFE;
     
     // Increment read counter
     eeprom_read_count++;
     
     // Set up EEPROM access
     uint16_t old_tblpag = TBLPAG;
     TBLPAG = 0x7F;
     
     // Calculate offset within page
     uint16_t offset = 0xF000 + word_address;
     
     // Read word using table read
     uint16_t result;
     __asm__ volatile (
         "tblrdl [%1], %0"
         : "=r" (result)
         : "r" (offset)
     );
     
     // Restore TBLPAG
     TBLPAG = old_tblpag;
     
     return result;
 }
 
 uint8_t ReadEEPROMByte(uint16_t byte_address) {
     // Bounds check
     if(byte_address >= 0x1000) {
         bounds_errors++;
         return 0xFF;
     }
     
     // Read the word containing this byte
     uint16_t word = ReadEEPROMWord(byte_address);
     
     // Extract the correct byte based on odd/even address
     if(byte_address & 0x01) {
         // Odd address - return high byte
         return (uint8_t)((word >> 8) & 0xFF);
     } else {
         // Even address - return low byte
         return (uint8_t)(word & 0xFF);
     }
 }
 
 // Read case from EEPROM with comprehensive error checking
 uint8_t EEPROM_ReadCase(uint16_t address, CaseData *case_data) {
     // Increment read counter
     eeprom_read_count++;
     
     // Validate pointer
     if(case_data == 0) {
         bounds_errors++;
         return 0;
     }
     
    // Initialize to invalid
    case_data->valid = 0;
    case_data->pattern_on_time = 0;
    case_data->pattern_off_time = 0;
    case_data->can_be_overridden = 0;
    
    // Initialize conditional logic arrays
    for(uint8_t i = 0; i < 8; i++) {
        case_data->must_be_on[i] = 0;
        case_data->must_be_off[i] = 0;
    }
     
     // Validate address (FAST EXIT)
     if(address == 0xFFFF || address >= 0x1000) {
         bounds_errors++;
         return 0;
     }
     
     // Address must be word-aligned
     if(address & 0x01) {
         bounds_errors++;
         return 0;
     }
     
     // Address must be within case data region
     if(address < EEPROM_CASES_START && address < 0x0D62) {
         bounds_errors++;
         return 0;
     }
     
     // Read case header (first 4 bytes)
     uint8_t priority = ReadEEPROMByte(address + 0);
     uint8_t pgn_high = ReadEEPROMByte(address + 1);
     uint8_t pgn_low = ReadEEPROMByte(address + 2);
     uint8_t source_addr = ReadEEPROMByte(address + 3);
     
     // Check if valid (all 0xFF means invalid/blank case)
     if(priority == 0xFF && pgn_high == 0xFF && pgn_low == 0xFF && source_addr == 0xFF) {
         return 0;  // Invalid case, not an error
     }
     
    // Read configuration byte (byte 4)
    uint8_t config_byte = ReadEEPROMByte(address + CASE_OFFSET_CONFIG);
    
    // Read pattern timing byte (byte 7)
    uint8_t pattern_byte = ReadEEPROMByte(address + CASE_OFFSET_PATTERN_TIMING);
    
    // Read conditional logic bytes
     // Must Be On: bytes 8-15 (zero-indexed)
     for(uint8_t i = 0; i < 8; i++) {
         case_data->must_be_on[i] = ReadEEPROMByte(address + 8 + i);
     }
     
     // Must Be Off: bytes 16-23 (zero-indexed)
     for(uint8_t i = 0; i < 8; i++) {
         case_data->must_be_off[i] = ReadEEPROMByte(address + 16 + i);
     }
     
     // Read data bytes (bytes 24-31)
     for(uint8_t i = 0; i < 8; i++) {
         case_data->data[i] = ReadEEPROMByte(address + 24 + i);
     }
     
     // Fill structure
     case_data->priority = priority & 0x07;
     case_data->pgn = ((uint16_t)pgn_high << 8) | pgn_low;
     case_data->source_addr = source_addr;
     
    // Extract pattern timing from byte 7
    case_data->pattern_on_time = (pattern_byte >> 4) & 0x0F;  // Upper nibble = ON time
    case_data->pattern_off_time = pattern_byte & 0x0F;         // Lower nibble = OFF time
    
    // Extract can_be_overridden from byte 4 (bits 2-3)
    // Single filament brake lights use this to allow turn signals to override
    case_data->can_be_overridden = ((config_byte & CONFIG_CAN_BE_OVERRIDDEN_MASK) == CONFIG_CAN_BE_OVERRIDDEN_VALUE) ? 1 : 0;
    
    // Mark as valid
    case_data->valid = 1;
     
     return 1;  // Success
 }
 
 void EEPROM_HandleInputChange(uint8_t input_num, uint8_t new_state) {
     // Validate input number FIRST
     if(input_num >= TOTAL_INPUTS) {
         bounds_errors++;
         return;
     }
     
     if(new_state) {
         // INPUT TURNED ON
         // STEP 0: Capture PGN/SA combinations from existing ON cases BEFORE removing them
         // This is needed for generating clearing messages if any of these PGN/SA are later turned off
         typedef struct {
             uint16_t pgn;
             uint8_t source_addr;
             uint8_t priority;
         } PGN_SA_Pair;
         
         PGN_SA_Pair clearing_list[MAX_ON_CASES_PER_INPUT];
         uint8_t clearing_count = 0;
         
         // Scan current active cases (both ON and OFF) for this input and capture their PGN/SA
         for(uint8_t i = 0; i < active_case_count; i++) {
             if(active_cases[i].input_num == input_num ) {
                 uint16_t pgn = active_cases[i].case_data.pgn;
                 uint8_t sa = active_cases[i].case_data.source_addr;
                 uint8_t priority = active_cases[i].case_data.priority;
                 
                 // Check if this PGN/SA is already in clearing list
                 uint8_t already_listed = 0;
                 for(uint8_t j = 0; j < clearing_count; j++) {
                     if(clearing_list[j].pgn == pgn && clearing_list[j].source_addr == sa) {
                         already_listed = 1;
                         break;
                     }
                 }
                 
                 // Add to clearing list if not already present
                 if(!already_listed && clearing_count < MAX_ON_CASES_PER_INPUT) {
                     clearing_list[clearing_count].pgn = pgn;
                     clearing_list[clearing_count].source_addr = sa;
                     clearing_list[clearing_count].priority = priority;
                     clearing_count++;
                 }
             }
         }
         
         // STEP 1: Remove existing cases for this input
         // Compact the array by overwriting entries to be removed
         uint8_t write_idx = 0;
         for(uint8_t read_idx = 0; read_idx < active_case_count; read_idx++) {
             // Keep cases that DON'T match this input
             if(active_cases[read_idx].input_num != input_num) {
                 if(write_idx != read_idx) {
                     active_cases[write_idx] = active_cases[read_idx];
                 }
                 write_idx++;
             }
         }
         active_case_count = write_idx;
         
         
         // STEP 1.5: Generate clearing messages for any OFF cases that were just removed
         // This ensures OFF cases behave symmetrically to ON cases
         for(uint8_t i = 0; i < clearing_count; i++) {
             if(active_case_count >= MAX_ACTIVE_CASES) {
                 break;  // Can't add more cases
             }
             
             // Create a clearing case with all zeros
             active_cases[active_case_count].input_num = input_num;
             active_cases[active_case_count].case_num = i;
             active_cases[active_case_count].is_on_case = 0;  // Clearing case
             active_cases[active_case_count].needs_removal_after_send = 1;  // Remove after transmission
             
             // Fill with clearing data (all zeros)
             active_cases[active_case_count].case_data.priority = clearing_list[i].priority;
             active_cases[active_case_count].case_data.pgn = clearing_list[i].pgn;
             active_cases[active_case_count].case_data.source_addr = clearing_list[i].source_addr;
             active_cases[active_case_count].case_data.pattern_on_time = 0;
             active_cases[active_case_count].case_data.pattern_off_time = 0;
             active_cases[active_case_count].case_data.valid = 1;
             
             // All data bytes = 0 (clearing)
             for(uint8_t j = 0; j < 8; j++) {
                 active_cases[active_case_count].case_data.data[j] = 0x00;
             }
             
             // Initialize conditional logic arrays to zero
             for(uint8_t j = 0; j < 8; j++) {
                 active_cases[active_case_count].case_data.must_be_on[j] = 0x00;
                 active_cases[active_case_count].case_data.must_be_off[j] = 0x00;
             }
             
             active_case_count++;
         }
         // STEP 2: Load ON cases for this input
         uint8_t num_on_cases = input_on_case_count[input_num];
         
         // Prevent overflow
         if(num_on_cases > MAX_ON_CASES_PER_INPUT) {
             num_on_cases = MAX_ON_CASES_PER_INPUT;
         }
         
         // Track if this input has any pattern timing configured
         uint8_t has_pattern = 0;
         uint8_t on_time = 0;
         uint8_t off_time = 0;
         
         for(uint8_t i = 0; i < num_on_cases; i++) {
             // Check bounds
             if(active_case_count >= MAX_ACTIVE_CASES) {
                 break;  // Can't add more cases
             }
             
             // Get ON case address
             uint16_t addr = EEPROM_GetCaseAddress(input_num, i, 1);
             
             // Skip if address is invalid
             if(addr == 0xFFFF) {
                 continue;
             }
             
             // Final bounds check before read
             if(addr >= 0x1000) {
                 bounds_errors++;
                 continue;
             }
             
             // Try to read the case
             CaseData case_data;
             
             if(EEPROM_ReadCase(addr, &case_data)) {
                 // Check if this case has pattern timing (Case 1 sets the pattern for all cases)
                 if(i == 0 && (case_data.pattern_on_time != 0 || case_data.pattern_off_time != 0)) {
                     has_pattern = 1;
                     on_time = case_data.pattern_on_time;
                     off_time = case_data.pattern_off_time;
                 }
                 
                 // Add to active list
                 active_cases[active_case_count].input_num = input_num;
                 active_cases[active_case_count].case_num = i;
                 active_cases[active_case_count].is_on_case = 1;  // ON case
                 active_cases[active_case_count].needs_removal_after_send = 0;  // Keep until input turns off
                 active_cases[active_case_count].case_data = case_data;
                 active_case_count++;
             }
         }
         
         // Configure pattern timer if needed
         if(has_pattern) {
             pattern_timers[input_num].has_pattern = 1;
             pattern_timers[input_num].on_time = on_time;
             pattern_timers[input_num].off_time = off_time;
             pattern_timers[input_num].state = PATTERN_STATE_ON_PHASE;  // Start in ON phase
             pattern_timers[input_num].timer = on_time;  // Load ON time
         } else {
             // No pattern timing - clear the pattern timer
             pattern_timers[input_num].has_pattern = 0;
             pattern_timers[input_num].state = PATTERN_STATE_INACTIVE;
             pattern_timers[input_num].timer = 0;
         }
     } else {
         // INPUT TURNED OFF - Generate clearing messages for all ON case PGN/SA combinations
         pattern_timers[input_num].state = PATTERN_STATE_INACTIVE;
         pattern_timers[input_num].has_pattern = 0;
         pattern_timers[input_num].timer = 0;
         
         // STEP 0: Capture PGN/SA combinations from existing ON cases BEFORE removing them
         typedef struct {
             uint16_t pgn;
             uint8_t source_addr;
             uint8_t priority;
         } PGN_SA_Pair;
         
         PGN_SA_Pair clearing_list[MAX_ON_CASES_PER_INPUT];
         uint8_t clearing_count = 0;
         
         // Scan current active ON cases for this input and capture their PGN/SA
         for(uint8_t i = 0; i < active_case_count; i++) {
             if(active_cases[i].input_num == input_num && active_cases[i].is_on_case) {
                 uint16_t pgn = active_cases[i].case_data.pgn;
                 uint8_t sa = active_cases[i].case_data.source_addr;
                 uint8_t priority = active_cases[i].case_data.priority;
                 
                 // Check if this PGN/SA is already in clearing list
                 uint8_t already_listed = 0;
                 for(uint8_t j = 0; j < clearing_count; j++) {
                     if(clearing_list[j].pgn == pgn && clearing_list[j].source_addr == sa) {
                         already_listed = 1;
                         break;
                     }
                 }
                 
                 // Add to clearing list if not already present
                 if(!already_listed && clearing_count < MAX_ON_CASES_PER_INPUT) {
                     clearing_list[clearing_count].pgn = pgn;
                     clearing_list[clearing_count].source_addr = sa;
                     clearing_list[clearing_count].priority = priority;
                     clearing_count++;
                 }
             }
         }
         
         // STEP 1: Remove all cases for this input (both ON and OFF)
         uint8_t write_idx = 0;
         for(uint8_t read_idx = 0; read_idx < active_case_count; read_idx++) {
             // Keep cases that DON'T match this input
             if(active_cases[read_idx].input_num != input_num) {
                 if(write_idx != read_idx) {
                     active_cases[write_idx] = active_cases[read_idx];
                 }
                 write_idx++;
             }
         }
         active_case_count = write_idx;
         
         // STEP 2: ON cases were already removed in STEP 1 of HandleInputChange
         // The clearing_list was captured in STEP 0 before removal
         // Now generate clearing cases with all-zero data for each unique PGN/SA
         for(uint8_t i = 0; i < clearing_count; i++) {
             if(active_case_count >= MAX_ACTIVE_CASES) {
                 break;  // Can't add more cases
             }
             
             // Create a clearing case with all zeros
             active_cases[active_case_count].input_num = input_num;
             active_cases[active_case_count].case_num = i;
             active_cases[active_case_count].is_on_case = 0;  // OFF case
             active_cases[active_case_count].needs_removal_after_send = 1;  // Remove after transmission
             
             // Fill with clearing data (all zeros)
             active_cases[active_case_count].case_data.priority = clearing_list[i].priority;
             active_cases[active_case_count].case_data.pgn = clearing_list[i].pgn;
             active_cases[active_case_count].case_data.source_addr = clearing_list[i].source_addr;
             active_cases[active_case_count].case_data.pattern_on_time = 0;
             active_cases[active_case_count].case_data.pattern_off_time = 0;
             active_cases[active_case_count].case_data.valid = 1;
             
             // All data bytes = 0 (clearing)
             for(uint8_t j = 0; j < 8; j++) {
                 active_cases[active_case_count].case_data.data[j] = 0x00;
             }
             
             // Initialize conditional logic arrays to zero
             for(uint8_t j = 0; j < 8; j++) {
                 active_cases[active_case_count].case_data.must_be_on[j] = 0x00;
                 active_cases[active_case_count].case_data.must_be_off[j] = 0x00;
             }
             
             active_case_count++;
         }
         
         // STEP 3: Load OFF cases for this input
         uint8_t num_off_cases = input_off_case_count[input_num];
         
         // Skip if no OFF cases
         if(num_off_cases > 0) {
             // Prevent overflow
             if(num_off_cases > MAX_OFF_CASES_PER_INPUT) {
                 num_off_cases = MAX_OFF_CASES_PER_INPUT;
             }
             
             for(uint8_t i = 0; i < num_off_cases; i++) {
                 // Check bounds
                 if(active_case_count >= MAX_ACTIVE_CASES) {
                     break;  // Can't add more cases
                 }
                 
                 // Get OFF case address
                 uint16_t addr = EEPROM_GetCaseAddress(input_num, i, 0);
                 
                 // Skip if address is invalid
                 if(addr == 0xFFFF) {
                     continue;
                 }
                 
                 // Final bounds check before read
                 if(addr >= 0x1000) {
                     bounds_errors++;
                     continue;
                 }
                 
                 // Try to read the case
                 CaseData case_data;
                 
                 if(EEPROM_ReadCase(addr, &case_data)) {
                     // Successfully read valid OFF case - add to active list
                     // FIX: Skip positions with valid IN23/24 data
                     while(active_case_count < MAX_ACTIVE_CASES &&
                           (active_cases[active_case_count].input_num == 22 ||
                            active_cases[active_case_count].input_num == 23) &&
                           active_cases[active_case_count].case_data.valid) {
                         active_case_count++;
                     }
                     if(active_case_count >= MAX_ACTIVE_CASES) break;
                     active_cases[active_case_count].input_num = input_num;
                     active_cases[active_case_count].case_num = i;
                     active_cases[active_case_count].is_on_case = 0;  // OFF case
                     active_cases[active_case_count].needs_removal_after_send = 1;  // Remove after transmission
                     active_cases[active_case_count].case_data = case_data;
                     active_case_count++;
                 }
             }
         }
     }
 }

// ============================================================================
// CONDITIONAL LOGIC - MUST BE ON / MUST BE OFF INPUT CHECKING
// ============================================================================

/**
 * Check if all must_be_on and must_be_off conditions are met for a case
 * 
 * Bit layout for must_be_on/must_be_off (8 bytes each):
 *   Bytes 0-5: Inputs 1-44 (bits 0-43)
 *     - Byte 0: Inputs 1-8 (bit 0 = input 1, bit 7 = input 8)
 *     - Byte 1: Inputs 9-16
 *     - Byte 2: Inputs 17-24
 *     - Byte 3: Inputs 25-32
 *     - Byte 4: Inputs 33-40
 *     - Byte 5 bits 0-3: Inputs 41-44
 *   Byte 5 bit 4: Security condition (0x10)
 *   Byte 5 bit 5: Ignition condition (0x20)
 *   Bytes 6-7: Frequency/Analog thresholds (future - not implemented)
 * 
 * @param must_be_on 8-byte array of conditions that must be ON/true
 * @param must_be_off 8-byte array of conditions that must be OFF/false
 * @return 1 if all conditions are met, 0 if any condition fails
 */
static uint8_t CheckInputConditions(uint8_t *must_be_on, uint8_t *must_be_off) {
    // Check inputs 1-44 (bits 0-43 across bytes 0-5)
    // Input numbers are 0-indexed internally (input 0 = physical input 1)
    for(uint8_t input = 0; input < TOTAL_INPUTS; input++) {
        uint8_t byte_idx = input / 8;
        uint8_t bit_mask = 1 << (input % 8);
        
        // Get current state of this input (1 = ON/active, 0 = OFF/inactive)
        uint8_t input_state = Inputs_GetState(input);
        
        // Check must_be_on: if bit is set, input must be ON
        if((must_be_on[byte_idx] & bit_mask) && !input_state) {
            return 0;  // Condition failed - input should be ON but is OFF
        }
        
        // Check must_be_off: if bit is set, input must be OFF
        if((must_be_off[byte_idx] & bit_mask) && input_state) {
            return 0;  // Condition failed - input should be OFF but is ON
        }
    }
    
    // Check ignition condition (byte 5, bit 5 = 0x20)
    if((must_be_on[5] & 0x20) && !Inputs_GetIgnitionState()) {
        return 0;  // Must be on requires ignition, but ignition is OFF
    }
    if((must_be_off[5] & 0x20) && Inputs_GetIgnitionState()) {
        return 0;  // Must be off requires no ignition, but ignition is ON
    }
    
    // TODO: Check security condition (byte 5, bit 4 = 0x10)
    // Security state needs to be implemented
    
    // TODO: Check frequency/analog thresholds (bytes 6-7)
    // These are for future expansion
    
    return 1;  // All conditions met
}
 
uint8_t EEPROM_GetAggregatedMessages(AggregatedMessage *messages, uint8_t max_messages) {
    uint8_t msg_count = 0;
    
    // Validate parameters
    if(messages == 0 || max_messages == 0) {
        return 0;
    }
    
    // Initialize message array
    for(uint8_t i = 0; i < max_messages; i++) {
        messages[i].valid = 0;
        messages[i].priority = 0;
        messages[i].pgn = 0;
        messages[i].source_addr = 0;
        memset(messages[i].data, 0, 8);
        messages[i].has_pattern = 0;      // PHASE 1: Initialize pattern flag
        messages[i].data_changed = 0;     // PHASE 1: Initialize change flag
    }
    
    // ========================================================================
    // SINGLE FILAMENT BRAKE LIGHT OVERRIDE LOGIC
    // ========================================================================
    // For single filament brake lights, the brake and turn signals share the
    // same output bits. When braking + turning, the turn signal pattern should
    // override the brake on the turning side, while the other side stays steady.
    //
    // Implementation:
    // PASS 1: Build a "pattern mask" for each PGN/SA - bits controlled by pattern cases
    // PASS 2: Aggregate with override - overridable cases exclude pattern-controlled bits
    // ========================================================================
    
    // Temporary structure to track pattern masks per PGN/SA
    typedef struct {
        uint16_t pgn;
        uint8_t source_addr;
        uint8_t pattern_mask[8];  // Bits controlled by any active pattern case
        uint8_t valid;
    } PatternMaskEntry;
    
    PatternMaskEntry pattern_masks[MAX_UNIQUE_MESSAGES];
    uint8_t pattern_mask_count = 0;
    
    // Initialize pattern mask array
    for(uint8_t i = 0; i < MAX_UNIQUE_MESSAGES; i++) {
        pattern_masks[i].valid = 0;
        pattern_masks[i].pgn = 0;
        pattern_masks[i].source_addr = 0;
        memset(pattern_masks[i].pattern_mask, 0, 8);
    }
    
    // PASS 1: Identify all bits controlled by pattern cases (turn signals, hazards)
    // These are the bits that can override "overridable" cases (brakes)
    for(uint8_t i = 0; i < active_case_count; i++) {
        if(i >= MAX_ACTIVE_CASES) break;
        
        ActiveCase *ac = &active_cases[i];
        
        if(!ac->case_data.valid) continue;
        
        // Only process cases that have pattern timing (turn signals, hazards)
        if(ac->input_num >= TOTAL_INPUTS) continue;
        if(!pattern_timers[ac->input_num].has_pattern) continue;
        
        // Find or create pattern mask entry for this PGN/SA
        uint8_t found = 0;
        for(uint8_t j = 0; j < pattern_mask_count; j++) {
            if(pattern_masks[j].pgn == ac->case_data.pgn && 
               pattern_masks[j].source_addr == ac->case_data.source_addr) {
                // OR in this case's data bits to the pattern mask
                for(uint8_t k = 0; k < 8; k++) {
                    pattern_masks[j].pattern_mask[k] |= ac->case_data.data[k];
                }
                found = 1;
                break;
            }
        }
        
        // Create new entry if not found
        if(!found && pattern_mask_count < MAX_UNIQUE_MESSAGES) {
            pattern_masks[pattern_mask_count].pgn = ac->case_data.pgn;
            pattern_masks[pattern_mask_count].source_addr = ac->case_data.source_addr;
            for(uint8_t k = 0; k < 8; k++) {
                pattern_masks[pattern_mask_count].pattern_mask[k] = ac->case_data.data[k];
            }
            pattern_masks[pattern_mask_count].valid = 1;
            pattern_mask_count++;
        }
    }
    
    // PASS 2: Aggregate all cases with override logic
    for(uint8_t i = 0; i < active_case_count; i++) {
        // Safety check
        if(i >= MAX_ACTIVE_CASES) {
            break;
        }
        
        ActiveCase *ac = &active_cases[i];
        
        // Skip invalid cases
        if(!ac->case_data.valid) {
            continue;
        }
        
        // PATTERN TIMING: All cases for an input share the pattern timer
        // If the input has ANY pattern timer active (set in Case 1), ALL cases follow it
        // This allows multiple cases (different PGNs) to flash together in sync
        // Skip pattern check for special cases (input_num = 0xFF for track ignition clearing)
        uint8_t in_off_phase = 0;
        uint8_t has_pattern = 0;
        if(ac->input_num < TOTAL_INPUTS && pattern_timers[ac->input_num].has_pattern) {
            has_pattern = 1;
            // This input has a pattern timer active - check the phase
            if(!EEPROM_Pattern_IsInOnPhase(ac->input_num)) {
                in_off_phase = 1;  // Mark that we're in OFF phase - will send zeros
            }
        }
        // If no pattern timer is active for this input, always transmit (solid)
        
        // CONDITIONAL LOGIC: Check all must_be_on and must_be_off conditions
        // This includes input states (1-44), ignition, security, etc.
        if(!CheckInputConditions(ac->case_data.must_be_on, ac->case_data.must_be_off)) {
            continue;  // Skip this case - conditions not met
        }
        
        // SINGLE FILAMENT OVERRIDE: Get the pattern mask for this PGN/SA
        uint8_t override_mask[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        if(ac->case_data.can_be_overridden) {
            // This is an overridable case (brake) - find the pattern mask
            for(uint8_t j = 0; j < pattern_mask_count; j++) {
                if(pattern_masks[j].pgn == ac->case_data.pgn && 
                   pattern_masks[j].source_addr == ac->case_data.source_addr) {
                    // Copy the pattern mask - these bits will be excluded from brake
                    for(uint8_t k = 0; k < 8; k++) {
                        override_mask[k] = pattern_masks[j].pattern_mask[k];
                    }
                    break;
                }
            }
        }
        
        // Calculate the effective data for this case
        uint8_t effective_data[8];
        for(uint8_t k = 0; k < 8; k++) {
            if(in_off_phase) {
                // Pattern case in OFF phase - contribute zeros
                effective_data[k] = 0x00;
            } else if(ac->case_data.can_be_overridden) {
                // Overridable case - exclude bits controlled by pattern cases
                effective_data[k] = ac->case_data.data[k] & ~override_mask[k];
            } else {
                // Normal case - use data as-is
                effective_data[k] = ac->case_data.data[k];
            }
        }
        
        // Look for existing message with same PGN/SA
        uint8_t found = 0;
        for(uint8_t j = 0; j < msg_count; j++) {
            if(messages[j].pgn == ac->case_data.pgn && 
               messages[j].source_addr == ac->case_data.source_addr) {
                // OR the effective data bytes together
                for(uint8_t k = 0; k < 8; k++) {
                    messages[j].data[k] |= effective_data[k];
                }
                // PHASE 1: Set has_pattern flag if this case's input has a pattern
                if(has_pattern) {
                    messages[j].has_pattern = 1;
                }
                found = 1;
                break;
            }
        }
        
        // If not found and we have room, add new message
        if(!found && msg_count < max_messages) {
            messages[msg_count].priority = ac->case_data.priority;
            messages[msg_count].pgn = ac->case_data.pgn;
            messages[msg_count].source_addr = ac->case_data.source_addr;
            for(uint8_t k = 0; k < 8; k++) {
                messages[msg_count].data[k] = effective_data[k];
            }
            messages[msg_count].valid = 1;
            // PHASE 1: Set has_pattern flag if this case's input has a pattern
            if(has_pattern) {
                messages[msg_count].has_pattern = 1;
            }
            msg_count++;
        }
    }
    
   // STEP 2: Aggregate inLINK messages
    // FIX: Iterate over all possible indices, not just count
    // InLink_GetMessageCount returns the number of valid messages, but they
    // may not be stored at contiguous indices starting from 0
    for(uint8_t i = 0; i < MAX_INLINK_MESSAGES; i++) {
        InLinkMessage* inlink_msg = InLink_GetMessage(i);
        
        if(inlink_msg == NULL || !inlink_msg->valid) {
            continue;
        }
         
         // Look for existing message with same PGN/SA
         uint8_t found = 0;
         for(uint8_t j = 0; j < msg_count; j++) {
             if(messages[j].pgn == inlink_msg->pgn && 
                messages[j].source_addr == inlink_msg->source_addr) {
                 // OR the inLINK data bytes with existing MASTERCELL data
                 for(uint8_t k = 0; k < 8; k++) {
                     messages[j].data[k] |= inlink_msg->data[k];
                 }
                 found = 1;
                 break;
             }
         }
         
         // If not found and we have room, add new message with inLINK data only
         if(!found && msg_count < max_messages) {
             messages[msg_count].priority = 6;  // Default J1939 priority
             messages[msg_count].pgn = inlink_msg->pgn;
             messages[msg_count].source_addr = inlink_msg->source_addr;
             for(uint8_t k = 0; k < 8; k++) {
                 messages[msg_count].data[k] = inlink_msg->data[k];
             }
             messages[msg_count].valid = 1;
             msg_count++;
         }
     }
     
     return msg_count;
 }
 
 void EEPROM_ClearActiveCases(void) {
     active_case_count = 0;
     memset(active_cases, 0, sizeof(active_cases));
 }
 
 void EEPROM_RemoveMarkedCases(void) {
     // Remove all cases marked with needs_removal_after_send = 1
     // Compact the array by skipping marked cases
     uint8_t write_idx = 0;
     for(uint8_t read_idx = 0; read_idx < active_case_count; read_idx++) {
         // Keep cases that are NOT marked for removal
         if(!active_cases[read_idx].needs_removal_after_send) {
             if(write_idx != read_idx) {
                 active_cases[write_idx] = active_cases[read_idx];
             }
             write_idx++;
         }
     }
     active_case_count = write_idx;
 }
 
 // Diagnostic functions
 uint16_t EEPROM_GetReadCount(void) {
     return eeprom_read_count;
 }
 
 uint16_t EEPROM_GetBoundsErrors(void) {
     return bounds_errors;
 }
 
 uint8_t EEPROM_GetActiveCaseCount(void) {
     return active_case_count;
 }
 
void EEPROM_Pattern_UpdateTimers(void) {
    // Called every 250ms from timer interrupt
    // Must execute quickly and not block
    
    for(uint8_t i = 0; i < TOTAL_INPUTS; i++) {
        // Skip inactive pattern timers
        if(pattern_timers[i].state == PATTERN_STATE_INACTIVE) {
            continue;
        }
        
        // Skip if no pattern timing configured
        if(!pattern_timers[i].has_pattern) {
            continue;
        }
        
        // Check if timer expired FIRST (before decrementing)
        if(pattern_timers[i].timer == 0) {
            // Timer expired - toggle state and reload
            if(pattern_timers[i].state == PATTERN_STATE_ON_PHASE) {
                // Switch to OFF phase
                pattern_timers[i].state = PATTERN_STATE_OFF_PHASE;
                pattern_timers[i].timer = pattern_timers[i].off_time;
            } else if(pattern_timers[i].state == PATTERN_STATE_OFF_PHASE) {
                // Switch to ON phase
                pattern_timers[i].state = PATTERN_STATE_ON_PHASE;
                pattern_timers[i].timer = pattern_timers[i].on_time;
            }
        }
        
        // Decrement timer for next cycle
        if(pattern_timers[i].timer > 0) {
            pattern_timers[i].timer--;
        }
    }
}
 
 uint8_t EEPROM_Pattern_IsInOnPhase(uint8_t input_num) {
     // Validate input number
     if(input_num >= TOTAL_INPUTS) {
         return 0;
     }
     
     // If no pattern timer active for this input, always return 1 (always transmit)
     // This allows non-pattern cases to work even if input has pattern timer
     if(!pattern_timers[input_num].has_pattern) {
         return 1;
     }
     
     // Return 1 if in ON phase, 0 if in OFF phase
     // Inactive state returns 0 (don't transmit if input just turned off)
     return (pattern_timers[input_num].state == PATTERN_STATE_ON_PHASE);
 }
 // ============================================================================
 // ONE-BUTTON START MANUAL CASE CONTROL FUNCTIONS
 // ============================================================================
 
 /**
  * Helper function to extract the ignition output bit position from case data
  * Finds which bit in B0 is set (should only be one bit for one-button start)
  * 
  * @param case_data Pointer to CaseData structure
  * @return Bit position (0-7), or 0xFF if no bit is set or multiple bits set
  */
 static uint8_t GetIgnitionBitPosition(CaseData *case_data) {
     uint8_t data_b0 = case_data->data[0];
     uint8_t bit_count = 0;
     uint8_t bit_pos = 0xFF;
     
     // Count how many bits are set and remember the position
     for(uint8_t i = 0; i < 8; i++) {
         if(data_b0 & (1 << i)) {
             bit_count++;
             bit_pos = i;
         }
     }
     
     // Only return valid if exactly one bit is set
     if(bit_count == 1) {
         return bit_pos;
     }
     
     return 0xFF;  // Invalid - no bit or multiple bits set
 }
 
 uint8_t EEPROM_IsOneButtonStartInput(uint8_t input_num) {
     // Validate input number
     if(input_num >= TOTAL_INPUTS) {
         return 0;
     }
     
     // Get the address of the first ON case for this input
     uint16_t base_address = EEPROM_GetCaseAddress(input_num, 0, 1);
     if(base_address == 0xFFFF) {
         return 0;  // Invalid address
     }
     
     // Read byte 4 (configuration byte)
     uint8_t config_byte = ReadEEPROMByte(base_address + 4);
     
     // Check if bits 4-5 equal 0x01 (which is 0x10 when shifted to bits 4-5)
     // Bits 4-5 mask: 0x30
     // One-button start value: 0x10 (binary: 0001 0000)
     uint8_t one_button_bits = config_byte & 0x30;
     
     return (one_button_bits == 0x10) ? 1 : 0;
 }
 
 uint8_t EEPROM_SetManualCase(uint8_t input_num, uint8_t ignition_on, uint8_t starter_on) {
     // Validate input number
     if(input_num >= TOTAL_INPUTS) {
         bounds_errors++;
         return 0;
     }
     
     // STEP 1: Remove any existing active cases for this input
     EEPROM_ClearManualCase(input_num);
     
     // STEP 2: Read the case configuration from EEPROM
     uint16_t base_address = EEPROM_GetCaseAddress(input_num, 0, 1);
     if(base_address == 0xFFFF) {
         return 0;  // Invalid address
     }
     
     CaseData case_data;
     if(!EEPROM_ReadCase(base_address, &case_data)) {
         return 0;  // Failed to read case
     }
     
     // STEP 3: Determine the ignition and starter bit positions
     uint8_t ignition_bit = GetIgnitionBitPosition(&case_data);
     if(ignition_bit == 0xFF) {
         return 0;  // Error: can't determine ignition bit
     }
     
     // Starter is always one bit position higher (wraps from 0 to 7)
     uint8_t starter_bit;
     if(ignition_bit == 0) {
         starter_bit = 7;  // Wrap around
     } else {
         starter_bit = ignition_bit - 1;
     }
     
     // STEP 4: Build the desired data byte B0
     case_data.data[0] = 0x00;  // Clear all bits first
     
     if(ignition_on) {
         case_data.data[0] |= (1 << ignition_bit);
     }
     
     if(starter_on) {
         case_data.data[0] |= (1 << starter_bit);
     }
     
     // Clear all other data bytes (B1-B7)
     for(uint8_t i = 1; i < 8; i++) {
         case_data.data[i] = 0x00;
     }
     
     // STEP 5: Add the modified case to active cases list
     if(active_case_count >= MAX_ACTIVE_CASES) {
         return 0;  // No room for more cases
     }
     
     active_cases[active_case_count].input_num = input_num;
     active_cases[active_case_count].case_num = 0;
     active_cases[active_case_count].is_on_case = 1;
     active_cases[active_case_count].needs_removal_after_send = 0;  // Keep until manually cleared
     active_cases[active_case_count].case_data = case_data;
     active_case_count++;
     
     return 1;  // Success
 }
 
 void EEPROM_ClearManualCase(uint8_t input_num) {
     // Validate input number
     if(input_num >= TOTAL_INPUTS) {
         bounds_errors++;
         return;
     }
     
     // STEP 1: Remove all existing active cases for this input
     uint8_t write_idx = 0;
     for(uint8_t read_idx = 0; read_idx < active_case_count; read_idx++) {
         if(active_cases[read_idx].input_num != input_num) {
             // Keep this case (different input)
             if(write_idx != read_idx) {
                 active_cases[write_idx] = active_cases[read_idx];
             }
             write_idx++;
         }
     }
     active_case_count = write_idx;
     
     // STEP 2: Add a CLEARING case with all zeros
     // This ensures the CAN bus gets an explicit "turn off" message
     // Read the case configuration to get PGN, SA, priority
     uint16_t base_address = EEPROM_GetCaseAddress(input_num, 0, 1);
     if(base_address != 0xFFFF) {
         CaseData clearing_case;
         if(EEPROM_ReadCase(base_address, &clearing_case)) {
             // Keep the PGN, SA, and priority, but clear all data bytes
             for(uint8_t i = 0; i < 8; i++) {
                 clearing_case.data[i] = 0x00;
             }
             
             // Add the clearing case to active list if we have room
             if(active_case_count < MAX_ACTIVE_CASES) {
                 active_cases[active_case_count].input_num = input_num;
                 active_cases[active_case_count].case_num = 0;
                 active_cases[active_case_count].is_on_case = 0;  // Mark as OFF case
                 active_cases[active_case_count].needs_removal_after_send = 1;  // Remove after transmission
                 active_cases[active_case_count].case_data = clearing_case;
                 active_case_count++;
             }
         }
     }
     
     // STEP 3: Clear pattern timer for this input
     if(input_num < TOTAL_INPUTS) {
         pattern_timers[input_num].state = PATTERN_STATE_INACTIVE;
         pattern_timers[input_num].has_pattern = 0;
         pattern_timers[input_num].timer = 0;
     }
 }
 
 // ============================================================================
 // TRACK IGNITION FUNCTIONS
 // ============================================================================
 
 uint8_t EEPROM_IsTrackIgnitionCase(uint8_t input_num, uint8_t case_num) {
     // Validate input number
     if(input_num >= TOTAL_INPUTS) {
         return 0;
     }
     
     // Get the address of the specified ON case for this input
     uint16_t base_address = EEPROM_GetCaseAddress(input_num, case_num, 1);
     if(base_address == 0xFFFF) {
         return 0;  // Invalid address
     }
     
     // Read byte 4 (configuration byte)
     uint8_t config_byte = ReadEEPROMByte(base_address + 4);
     
     // Check if bits 6-7 equal 0x01 (which is 0x40 when shifted to bits 6-7)
     // Bits 6-7 mask: 0xC0
     // Track ignition value: 0x40 (binary: 0100 0000)
     uint8_t track_ignition_bits = config_byte & 0xC0;
     
     return (track_ignition_bits == 0x40) ? 1 : 0;
 }
 
 void EEPROM_UpdateIgnitionTrackedCases(uint8_t ignition_flag) {
     // This function manages all cases that have track ignition enabled
     // It scans all inputs and cases, and activates/deactivates them based on ignition_flag
     
     // STEP 1: Remove all existing track ignition cases from active list
     // We need to clear them first so we can rebuild based on new ignition state
     uint8_t write_idx = 0;
     for(uint8_t read_idx = 0; read_idx < active_case_count; read_idx++) {
         // Check if this active case is a track ignition case
         uint8_t is_track_ignition = EEPROM_IsTrackIgnitionCase(
             active_cases[read_idx].input_num, 
             active_cases[read_idx].case_num
         );
         
         if(!is_track_ignition) {
             // Keep this case (it's not a track ignition case)
             if(write_idx != read_idx) {
                 active_cases[write_idx] = active_cases[read_idx];
             }
             write_idx++;
         }
         // If it IS a track ignition case, skip it (don't copy it to write position)
     }
     active_case_count = write_idx;
     
     // STEP 2: If ignition is ON, scan all inputs/cases and add track ignition cases
     if(ignition_flag) {
         // Scan all inputs
         for(uint8_t input_num = 0; input_num < TOTAL_INPUTS; input_num++) {
             uint8_t num_cases = input_on_case_count[input_num];
             
             // Limit to prevent buffer overflow
             if(num_cases > MAX_ON_CASES_PER_INPUT) {
                 num_cases = MAX_ON_CASES_PER_INPUT;
             }
             
             // Scan all ON cases for this input
             for(uint8_t case_num = 0; case_num < num_cases; case_num++) {
                 // Check if we have room for more cases
                 if(active_case_count >= MAX_ACTIVE_CASES) {
                     return;  // No more room
                 }
                 
                 // Check if this case has track ignition enabled
                 if(EEPROM_IsTrackIgnitionCase(input_num, case_num)) {
                     // This case tracks ignition - read it and add to active list
                     uint16_t addr = EEPROM_GetCaseAddress(input_num, case_num, 1);
                     
                     // Skip if address is invalid
                     if(addr == 0xFFFF || addr >= 0x1000) {
                         continue;
                     }
                     
                     // Read the case from EEPROM
                     CaseData case_data;
                     if(EEPROM_ReadCase(addr, &case_data)) {
                         // Add to active list
                         active_cases[active_case_count].input_num = input_num;
                         active_cases[active_case_count].case_num = case_num;
                         active_cases[active_case_count].is_on_case = 1;
                         active_cases[active_case_count].needs_removal_after_send = 0;  // Keep while ignition is on
                         active_cases[active_case_count].case_data = case_data;
                         active_case_count++;
                     }
                 }
             }
         }
     } else {
         // STEP 3: Ignition is OFF - Generate clearing messages for all track ignition cases
         // We need to find all unique PGN/SA combinations from track ignition cases
         // and add clearing cases (all zeros) for each
         
         typedef struct {
             uint16_t pgn;
             uint8_t source_addr;
             uint8_t priority;
         } PGN_SA_Pair;
         
         PGN_SA_Pair clearing_list[MAX_ACTIVE_CASES];
         uint8_t clearing_count = 0;
         
         // Scan all inputs to find track ignition cases and collect their PGN/SA
         for(uint8_t input_num = 0; input_num < TOTAL_INPUTS; input_num++) {
             uint8_t num_cases = input_on_case_count[input_num];
             
             if(num_cases > MAX_ON_CASES_PER_INPUT) {
                 num_cases = MAX_ON_CASES_PER_INPUT;
             }
             
             for(uint8_t case_num = 0; case_num < num_cases; case_num++) {
                 if(EEPROM_IsTrackIgnitionCase(input_num, case_num)) {
                     // Read this track ignition case to get its PGN/SA
                     uint16_t addr = EEPROM_GetCaseAddress(input_num, case_num, 1);
                     
                     if(addr != 0xFFFF && addr < 0x1000) {
                         CaseData case_data;
                         if(EEPROM_ReadCase(addr, &case_data)) {
                             // Check if this PGN/SA is already in clearing list
                             uint8_t already_listed = 0;
                             for(uint8_t i = 0; i < clearing_count; i++) {
                                 if(clearing_list[i].pgn == case_data.pgn && 
                                    clearing_list[i].source_addr == case_data.source_addr) {
                                     already_listed = 1;
                                     break;
                                 }
                             }
                             
                             // Add to clearing list if not already present
                             if(!already_listed && clearing_count < MAX_ACTIVE_CASES) {
                                 clearing_list[clearing_count].pgn = case_data.pgn;
                                 clearing_list[clearing_count].source_addr = case_data.source_addr;
                                 clearing_list[clearing_count].priority = case_data.priority;
                                 clearing_count++;
                             }
                         }
                     }
                 }
             }
         }
         
         // Now add clearing cases for each unique PGN/SA
         for(uint8_t i = 0; i < clearing_count; i++) {
             if(active_case_count >= MAX_ACTIVE_CASES) {
                 break;  // No more room
             }
             
             // Create a clearing case with all zeros
             active_cases[active_case_count].input_num = 0xFF;  // Special marker for track ignition clearing
             active_cases[active_case_count].case_num = i;
             active_cases[active_case_count].is_on_case = 0;  // OFF case
             active_cases[active_case_count].needs_removal_after_send = 1;  // Remove after transmission
             
             // Fill with clearing data (all zeros)
             active_cases[active_case_count].case_data.priority = clearing_list[i].priority;
             active_cases[active_case_count].case_data.pgn = clearing_list[i].pgn;
             active_cases[active_case_count].case_data.source_addr = clearing_list[i].source_addr;
             active_cases[active_case_count].case_data.pattern_on_time = 0;
             active_cases[active_case_count].case_data.pattern_off_time = 0;
             active_cases[active_case_count].case_data.valid = 1;
             
             // All data bytes = 0 (clearing)
             for(uint8_t j = 0; j < 8; j++) {
                 active_cases[active_case_count].case_data.data[j] = 0x00;
             }
             
             // Initialize conditional logic arrays to zero
             for(uint8_t j = 0; j < 8; j++) {
                 active_cases[active_case_count].case_data.must_be_on[j] = 0x00;
                 active_cases[active_case_count].case_data.must_be_off[j] = 0x00;
             }
             
             active_case_count++;
         }
     }
 }
 // ============================================================================
 // DEBUG FUNCTIONS - Access active_cases and pattern_timers for LCD display
 // ============================================================================
 
 /**
  * Get pattern timing from an active case in RAM
  * Searches active_cases[] for a specific input and case number
  * 
  * @param input_num Input number (0-43)
  * @param case_num Case number (0-7)
  * @param pattern_on Pointer to store ON time (can be NULL)
  * @param pattern_off Pointer to store OFF time (can be NULL)
  * @return 1 if case found, 0 if not found
  */
 uint8_t EEPROM_Debug_GetActiveCasePattern(uint8_t input_num, uint8_t case_num, 
                                           uint8_t *pattern_on, uint8_t *pattern_off) {
     // Search active_cases for matching input_num and case_num
     for(uint8_t i = 0; i < active_case_count; i++) {
         if(active_cases[i].input_num == input_num && 
            active_cases[i].case_num == case_num &&
            active_cases[i].is_on_case == 1) {
             
             // Found it - return the pattern timing
             if(pattern_on != NULL) {
                 *pattern_on = active_cases[i].case_data.pattern_on_time;
             }
             if(pattern_off != NULL) {
                 *pattern_off = active_cases[i].case_data.pattern_off_time;
             }
             return 1;
         }
     }
     
     return 0;  // Not found
 }
 
 /**
  * Get pattern timer state for an input
  * 
  * @param input_num Input number (0-43)
  * @param state Pointer to store state (PATTERN_STATE_xxx)
  * @param timer Pointer to store current timer value
  * @param on_time Pointer to store ON time
  * @param off_time Pointer to store OFF time
  * @param has_pattern Pointer to store has_pattern flag
  * @return 1 if valid input, 0 if out of bounds
  */
 uint8_t EEPROM_Debug_GetPatternTimer(uint8_t input_num, uint8_t *state, uint8_t *timer,
                                      uint8_t *on_time, uint8_t *off_time, uint8_t *has_pattern) {
     if(input_num >= TOTAL_INPUTS) {
         return 0;
     }
     
     if(state != NULL) *state = pattern_timers[input_num].state;
     if(timer != NULL) *timer = pattern_timers[input_num].timer;
     if(on_time != NULL) *on_time = pattern_timers[input_num].on_time;
     if(off_time != NULL) *off_time = pattern_timers[input_num].off_time;
     if(has_pattern != NULL) *has_pattern = pattern_timers[input_num].has_pattern;
     
     return 1;
 }
 
 /**
  * Get input_num and case_num for an active case by index
  * @param active_case_index Index in active_cases array (0-MAX_ACTIVE_CASES)
  * @param input_num Pointer to store input number
  * @param case_num Pointer to store case number
  * @return 1 if valid index, 0 if out of bounds
  */
 uint8_t EEPROM_Debug_GetActiveCaseInfo(uint8_t active_case_index, uint8_t *input_num, uint8_t *case_num) {
     if(active_case_index >= active_case_count) {
         return 0;
     }
     
     if(input_num != NULL) *input_num = active_cases[active_case_index].input_num;
     if(case_num != NULL) *case_num = active_cases[active_case_index].case_num;
     
     return 1;
 }