/*
 * FILE: eeprom_init.c
 * EEPROM Initialization - Custom Configuration for CVR Customs V1.6
 * 
 * TEST PATTERN:
 * - All ON cases are INVALID (0xFF) so they don't broadcast
 * - OFF cases broadcast their input number in ALL 8 data bytes
 * - Each input's OFF cases use UNIQUE CAN IDs to avoid aggregation
 * - IN01: 18FF011E, 18FF021E (data: 0x01)
 * - IN02: 18FF031E, 18FF041E (data: 0x02)
 * - IN25: 18FF051E, 18FF061E (data: 0x19)
 * - IN26: 18FF071E, 18FF081E (data: 0x1A)
 * - etc.
 * 
 * FIXED:
 * - Uses EEPROM_Init_WriteByte (original function name)
 * - Each input has unique CAN IDs for OFF cases
 * - No aggregation/OR issues
 */

#include "eeprom_init.h"
#include "eeprom_cases.h"
#include "eeprom_config.h"
#include <string.h>

#define FCY 16000000UL
#include <libpic30.h>

// Write verification counter
static uint16_t write_errors = 0;
static uint16_t words_written = 0;
static uint8_t last_error_type = 0;  // 0=none, 1=timeout, 2=verify, 3=bounds
static uint16_t write_attempts = 0;

/*
 * Write a word to EEPROM
 * Returns: 1=success, 0=failure
 */
uint8_t EEPROM_WriteWord(uint16_t address, uint16_t data) {
    write_attempts++;
    
    // Address must be even and in valid range
    if(address & 0x01) {
        write_errors++;
        last_error_type = 3;  // Bounds error
        return 0;
    }
    if(address >= 0x1000) {
        write_errors++;
        last_error_type = 3;  // Bounds error
        return 0;
    }
    
    // ==========================================
    // STEP 1: ERASE the word
    // ==========================================
    TBLPAG = 0x7F;
    __builtin_tblwtl(0xF000 + address, 0xFFFF);
    
    NVMCON = 0x4044;  // Word erase operation
    
    asm volatile ("disi #5");
    asm volatile ("mov #0x55, W0");
    asm volatile ("mov W0, NVMKEY");
    asm volatile ("mov #0xAA, W0");
    asm volatile ("mov W0, NVMKEY");
    asm volatile ("bset NVMCON, #15");
    asm volatile ("nop");
    asm volatile ("nop");
    
    uint16_t timeout = 30000;
    while((NVMCON & 0x8000) && timeout > 0) {
        timeout--;
    }
    
    if(timeout == 0) {
        write_errors++;
        last_error_type = 1;  // Timeout on ERASE
        return 0;
    }
    
    __delay_ms(3);
    
    // ==========================================
    // STEP 2: WRITE the data
    // ==========================================
    TBLPAG = 0x7F;
    __builtin_tblwtl(0xF000 + address, data);
    
    NVMCON = 0x4004;  // Word write operation
    
    asm volatile ("disi #5");
    asm volatile ("mov #0x55, W0");
    asm volatile ("mov W0, NVMKEY");
    asm volatile ("mov #0xAA, W0");
    asm volatile ("mov W0, NVMKEY");
    asm volatile ("bset NVMCON, #15");
    asm volatile ("nop");
    asm volatile ("nop");
    
    timeout = 30000;
    while((NVMCON & 0x8000) && timeout > 0) {
        timeout--;
    }
    
    if(timeout == 0) {
        write_errors++;
        last_error_type = 1;  // Timeout on WRITE
        return 0;
    }
    
    __delay_ms(3);
    
    // Verify
    TBLPAG = 0x7F;
    uint16_t verify = __builtin_tblrdl(0xF000 + address);
    
    if(verify != data) {
        write_errors++;
        last_error_type = 2;  // Verify fail
        return 0;
    }
    
    words_written++;
    last_error_type = 0;  // Success
    
    return 1;
}

void EEPROM_WriteBytePair(uint16_t address, uint8_t byte0, uint8_t byte1) {
    if(address & 0x01) {
        write_errors++;
        last_error_type = 3;
        return;
    }
    if(address >= 0x1000) {
        write_errors++;
        last_error_type = 3;
        return;
    }
    
    uint16_t word = byte0 | ((uint16_t)byte1 << 8);
    EEPROM_WriteWord(address, word);
}

/*
 * Write a complete 32-byte case to EEPROM with full conditional support
 * 
 * @param address EEPROM address to write to
 * @param priority J1939 priority (0-7)
 * @param pgn Parameter Group Number
 * @param source_addr Source Address
 * @param config_byte Configuration flags (byte 4)
 * @param pattern_timing Pattern ON/OFF timing (byte 7)
 * @param must_be_on 8-byte array of must_be_on conditions (NULL = no conditions)
 * @param must_be_off 8-byte array of must_be_off conditions (NULL = no conditions)
 * @param data 8-byte CAN data payload
 */
void EEPROM_WriteCaseEx(uint16_t address, uint8_t priority, uint16_t pgn, 
                        uint8_t source_addr, uint8_t config_byte, uint8_t pattern_timing,
                        uint8_t *must_be_on, uint8_t *must_be_off, uint8_t *data) {
    // Validate address
    if(address & 0x01) {
        write_errors++;
        last_error_type = 3;
        return;
    }
    if(address >= 0x1000) {
        write_errors++;
        last_error_type = 3;
        return;
    }
    
    uint8_t case_buffer[32];
    memset(case_buffer, 0x00, 32);
    
    // Bytes 0-3: Priority, PGN high, PGN low, Source Address
    case_buffer[0] = priority & 0x07;
    case_buffer[1] = (pgn >> 8) & 0xFF;
    case_buffer[2] = pgn & 0xFF;
    case_buffer[3] = source_addr;
    
    // Byte 4: Configuration flags
    case_buffer[4] = config_byte;
    
    // Bytes 5-6: Reserved (0x00)
    case_buffer[5] = 0x00;
    case_buffer[6] = 0x00;
    
    // Byte 7: Pattern timing
    case_buffer[7] = pattern_timing;
    
    // Bytes 8-15: Must Be On (conditional logic)
    if(must_be_on != NULL) {
        for(uint8_t i = 0; i < 8; i++) {
            case_buffer[8 + i] = must_be_on[i];
        }
    }
    
    // Bytes 16-23: Must Be Off (conditional logic)
    if(must_be_off != NULL) {
        for(uint8_t i = 0; i < 8; i++) {
            case_buffer[16 + i] = must_be_off[i];
        }
    }
    
    // Bytes 24-31: CAN Data payload
    for(uint8_t i = 0; i < 8; i++) {
        case_buffer[24 + i] = data[i];
    }
    
    // Write case word by word
    for(uint8_t i = 0; i < 32; i += 2) {
        if(address + i >= 0x1000) {
            write_errors++;
            last_error_type = 3;
            return;
        }
        uint16_t word = case_buffer[i] | ((uint16_t)case_buffer[i+1] << 8);
        EEPROM_WriteWord(address + i, word);
    }
}

/*
 * Write a complete 32-byte case to EEPROM (legacy function for backward compatibility)
 */
void EEPROM_WriteCase(uint16_t address, uint8_t priority, uint16_t pgn, 
                             uint8_t source_addr, uint8_t config_byte, uint8_t pattern_timing,
                             uint8_t requires_ignition, uint8_t *data) {
    // Build must_be_on array with just ignition requirement if needed
    uint8_t must_be_on[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    
    // Byte 5 bit 5 (0x20) = Requires Ignition
    if(requires_ignition) {
        must_be_on[5] = 0x20;
    }
    
    // Call the extended function
    EEPROM_WriteCaseEx(address, priority, pgn, source_addr, config_byte, pattern_timing,
                       requires_ignition ? must_be_on : NULL, NULL, data);
}

void EEPROM_WriteInvalidCase(uint16_t address) {
    // Validate address
    if(address & 0x01) {
        write_errors++;
        last_error_type = 3;
        return;
    }
    if(address >= 0x1000) {
        write_errors++;
        last_error_type = 3;
        return;
    }
    
    // Write all 0xFF to mark as invalid
    for(uint8_t i = 0; i < 32; i += 2) {
        if(address + i >= 0x1000) {
            write_errors++;
            last_error_type = 3;
            return;
        }
        EEPROM_WriteWord(address + i, 0xFFFF);
    }
}

// Read back a word to verify
static uint16_t EEPROM_ReadWord(uint16_t address) {
    if(address >= 0x1000) return 0xFFFF;
    if(address & 0x01) return 0xFFFF;
    
    TBLPAG = 0x7F;
    return __builtin_tblrdl(0xF000 + address);
}

/**
 * Initialize EEPROM with selected configuration
 */
void EEPROM_InitWithConfig(eeprom_config_type_t config_type) {
    // Reset counters
    write_errors = 0;
    words_written = 0;
    last_error_type = 0;
    write_attempts = 0;
    
    // Call the appropriate configuration loader
    switch(config_type) {
        case CONFIG_STD_FRONT_ENGINE:
            EEPROM_LoadFrontEngine();
            break;
        case CONFIG_STD_REAR_ENGINE:
            EEPROM_LoadRearEngine();
            break;
        case CONFIG_CUSTOMER:
            EEPROM_LoadCustomer();
            break;
        default:
            // Default to front engine if invalid selection
            EEPROM_LoadFrontEngine();
            break;
    }
}

/**
 * Simple configuration selection - returns CONFIG_STD_FRONT_ENGINE for now
 * Main.c handles the actual menu display
 */
eeprom_config_type_t EEPROM_SelectConfiguration(void) {
    // For now, just return front engine
    // The actual menu selection is handled in main.c
    return CONFIG_STD_FRONT_ENGINE;
}

uint8_t EEPROM_IsInitialized(void) {
    // Check for init stamp (0xA5) at byte address 7
    // Byte 7 is in word address 0x0006, MSB position
    TBLPAG = 0x7F;
    uint16_t word = __builtin_tblrdl(0xF000 + 0x0006);
    uint8_t init_stamp = (uint8_t)((word >> 8) & 0xFF);
    
    return (init_stamp == DEFAULT_INIT_STAMP);
}

/*
 * Parse CAN ID string to extract priority, PGN, and source address
 * Format: "18FF011E" = Priority 6, PGN 0xFF01, SA 0x1E
 */
void ParseCANID(const char *can_id_str, uint8_t *priority, uint16_t *pgn, uint8_t *sa) {
    // Convert hex string to 32-bit value
    uint32_t can_id = 0;
    for(uint8_t i = 0; i < 8; i++) {
        can_id = (can_id << 4);
        char c = can_id_str[i];
        if(c >= '0' && c <= '9') can_id |= (c - '0');
        else if(c >= 'A' && c <= 'F') can_id |= (c - 'A' + 10);
        else if(c >= 'a' && c <= 'f') can_id |= (c - 'a' + 10);
    }
    
    // Extract fields from 29-bit CAN ID
    // Bits 28-26: Priority
    // Bits 25-8: PGN (18 bits, but we use bits 16-8 as PGN for J1939)
    // Bits 7-0: Source Address
    *priority = (can_id >> 26) & 0x07;
    *pgn = (can_id >> 8) & 0xFFFF;  // Extract full 16-bit PGN portion
    *sa = can_id & 0xFF;
}

/**
 * Write a single byte to EEPROM using the working boot-time write function
 * This function wraps the proven EEPROM_WriteWord function and is exposed
 * for runtime use by CAN configuration writes
 * 
 * @param byte_addr: Byte address to write (0-4095)
 * @param value: Byte value to write (0-255)
 * @return 1 on success, 0 on failure
 */
uint8_t EEPROM_Init_WriteByte(uint16_t byte_addr, uint8_t value) {
    // Validate byte address is in range
    if(byte_addr >= 0x1000) {
        write_errors++;
        last_error_type = 3;
        return 0;
    }
    
    // Convert byte address to word address
    uint16_t word_addr = byte_addr & 0xFFFE;  // Clear bit 0
    
    // Read current word value
    TBLPAG = 0x7F;
    uint16_t current_word = __builtin_tblrdl(0xF000 + word_addr);
    
    // Modify the appropriate byte
    uint16_t new_word;
    if(byte_addr & 0x01) {
        // Odd address = upper byte (MSB)
        new_word = (current_word & 0x00FF) | ((uint16_t)value << 8);
    } else {
        // Even address = lower byte (LSB)
        new_word = (current_word & 0xFF00) | value;
    }
    
    // Write the modified word back
    return EEPROM_WriteWord(word_addr, new_word);
}

// Diagnostics
uint16_t EEPROM_GetWriteErrors(void) {
    return write_errors;
}

uint16_t EEPROM_GetWordsWritten(void) {
    return words_written;
}

uint8_t EEPROM_GetLastErrorType(void) {
    return last_error_type;
}

uint16_t EEPROM_GetWriteAttempts(void) {
    return write_attempts;
}