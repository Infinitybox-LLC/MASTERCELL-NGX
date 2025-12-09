/*
 * FILE: eeprom_config.c
 * Byte-Level EEPROM Configuration Access Implementation
 * 
 * Implements byte-level read/write operations on dsPIC30F EEPROM
 * which natively supports only 16-bit word operations.
 * 
 * Strategy: Read-Modify-Write
 * - To read a byte: Read the containing word, extract LSB or MSB
 * - To write a byte: Read word, modify target byte, write word back
 */

#include "eeprom_config.h"
#include <string.h>

#define FCY 16000000UL
#include <libpic30.h>

// Diagnostic counters
static uint32_t byte_read_count = 0;
static uint32_t byte_write_count = 0;
static uint16_t write_failures = 0;

/*
 * Low-level word read from EEPROM
 * Reads a 16-bit word from EEPROM using Table Read
 * 
 * @param word_addr Word address (must be even)
 * @return 16-bit word value
 */
static uint16_t EEPROM_ReadWord(uint16_t word_addr) {
    // Validate word address is even
    if (word_addr & 0x01) {
        return 0xFFFF;  // Invalid address
    }
    
    // Set up table page for EEPROM access (0x7F)
    TBLPAG = 0x7F;
    
    // Read from EEPROM space (0xF000 offset + word address)
    uint16_t value = __builtin_tblrdl(0xF000 + word_addr);
    
    return value;
}

/*
 * Low-level word write to EEPROM
 * Writes a 16-bit word to EEPROM with erase-then-write sequence
 * 
 * @param word_addr Word address (must be even)
 * @param data 16-bit word value
 * @return 1 if successful, 0 if failed
 */
static uint8_t EEPROM_WriteWord(uint16_t word_addr, uint16_t data) {
    uint16_t timeout;
    
    // Validate word address is even and in valid range
    if (word_addr & 0x01) {
        write_failures++;
        return 0;
    }
    if (word_addr >= 0x1000) {
        write_failures++;
        return 0;
    }
    
    // ==========================================
    // STEP 1: ERASE the word
    // ==========================================
    TBLPAG = 0x7F;
    __builtin_tblwtl(0xF000 + word_addr, 0xFFFF);
    
    NVMCON = 0x4044;  // Word erase operation
    
    // Unlock sequence
    asm volatile ("disi #5");
    asm volatile ("mov #0x55, W0");
    asm volatile ("mov W0, NVMKEY");
    asm volatile ("mov #0xAA, W0");
    asm volatile ("mov W0, NVMKEY");
    asm volatile ("bset NVMCON, #15");
    asm volatile ("nop");
    asm volatile ("nop");
    
    // Wait for erase to complete
    timeout = 30000;
    while ((NVMCON & 0x8000) && timeout > 0) {
        timeout--;
    }
    
    if (timeout == 0) {
        write_failures++;
        return 0;
    }
    
    __delay_ms(3);  // Delay after erase
    
    // ==========================================
    // STEP 2: WRITE the data
    // ==========================================
    TBLPAG = 0x7F;
    __builtin_tblwtl(0xF000 + word_addr, data);
    
    NVMCON = 0x4004;  // Word write operation
    
    // Unlock sequence
    asm volatile ("disi #5");
    asm volatile ("mov #0x55, W0");
    asm volatile ("mov W0, NVMKEY");
    asm volatile ("mov #0xAA, W0");
    asm volatile ("mov W0, NVMKEY");
    asm volatile ("bset NVMCON, #15");
    asm volatile ("nop");
    asm volatile ("nop");
    
    // Wait for write to complete
    timeout = 30000;
    while ((NVMCON & 0x8000) && timeout > 0) {
        timeout--;
    }
    
    if (timeout == 0) {
        write_failures++;
        return 0;
    }
    
    __delay_ms(3);  // Delay after write
    
    // ==========================================
    // STEP 3: VERIFY the write
    // ==========================================
    TBLPAG = 0x7F;
    uint16_t verify = __builtin_tblrdl(0xF000 + word_addr);
    
    if (verify != data) {
        write_failures++;
        return 0;
    }
    
    return 1;
}

/**
 * Read a single byte from EEPROM
 * Converts byte address to word address and extracts LSB or MSB
 */
uint8_t EEPROM_Config_ReadByte(uint16_t byte_addr) {
    // Convert byte address to word address (clear LSB)
    uint16_t word_addr = byte_addr & 0xFFFE;
    
    // Read the 16-bit word
    uint16_t word_value = EEPROM_ReadWord(word_addr);
    
    // Increment diagnostic counter
    byte_read_count++;
    
    // Extract the requested byte
    if (byte_addr & 0x01) {
        // Odd address = MSB (upper byte)
        return (uint8_t)((word_value >> 8) & 0xFF);
    } else {
        // Even address = LSB (lower byte)
        return (uint8_t)(word_value & 0xFF);
    }
}

/**
 * Write a single byte to EEPROM
 * Uses read-modify-write: read word, modify target byte, write word back
 */
uint8_t EEPROM_Config_WriteByte(uint16_t byte_addr, uint8_t value) {
    // Convert byte address to word address (clear LSB)
    uint16_t word_addr = byte_addr & 0xFFFE;
    
    // Read the current word value
    uint16_t word_value = EEPROM_ReadWord(word_addr);
    
    // Modify the appropriate byte
    if (byte_addr & 0x01) {
        // Odd address = MSB (upper byte)
        word_value = (word_value & 0x00FF) | ((uint16_t)value << 8);
    } else {
        // Even address = LSB (lower byte)
        word_value = (word_value & 0xFF00) | (uint16_t)value;
    }
    
    // Write the modified word back
    uint8_t result = EEPROM_WriteWord(word_addr, word_value);
    
    // Increment diagnostic counter
    if (result) {
        byte_write_count++;
    }
    
    return result;
}

/**
 * Read multiple consecutive bytes from EEPROM
 */
uint16_t EEPROM_Config_ReadBytes(uint16_t start_addr, uint8_t *buffer, uint16_t length) {
    uint16_t bytes_read = 0;
    
    if (buffer == NULL || length == 0) {
        return 0;
    }
    
    for (uint16_t i = 0; i < length; i++) {
        buffer[i] = EEPROM_Config_ReadByte(start_addr + i);
        bytes_read++;
    }
    
    return bytes_read;
}

/**
 * Write multiple consecutive bytes to EEPROM
 */
uint16_t EEPROM_Config_WriteBytes(uint16_t start_addr, const uint8_t *buffer, uint16_t length) {
    uint16_t bytes_written = 0;
    
    if (buffer == NULL || length == 0) {
        return 0;
    }
    
    for (uint16_t i = 0; i < length; i++) {
        if (EEPROM_Config_WriteByte(start_addr + i, buffer[i])) {
            bytes_written++;
        } else {
            // Write failed, stop and return count so far
            break;
        }
    }
    
    return bytes_written;
}

/**
 * Check if configuration EEPROM has been initialized
 * Looks for Init Stamp (0xA5) at byte address 7
 */
uint8_t EEPROM_Config_IsInitialized(void) {
    uint8_t init_stamp = EEPROM_Config_ReadByte(EEPROM_CFG_INIT_STAMP);
    return (init_stamp == DEFAULT_INIT_STAMP) ? 1 : 0;
}

/**
 * Read a 16-bit PGN from two consecutive bytes
 */
uint16_t EEPROM_Config_ReadPGN(uint16_t pgn_a_addr) {
    uint8_t pgn_a = EEPROM_Config_ReadByte(pgn_a_addr);      // High byte
    uint8_t pgn_b = EEPROM_Config_ReadByte(pgn_a_addr + 1);  // Low byte
    
    return ((uint16_t)pgn_a << 8) | (uint16_t)pgn_b;
}

/**
 * Write a 16-bit PGN to two consecutive bytes
 */
uint8_t EEPROM_Config_WritePGN(uint16_t pgn_a_addr, uint16_t pgn) {
    uint8_t pgn_a = (pgn >> 8) & 0xFF;  // High byte
    uint8_t pgn_b = pgn & 0xFF;         // Low byte
    
    // Write both bytes
    if (!EEPROM_Config_WriteByte(pgn_a_addr, pgn_a)) {
        return 0;
    }
    if (!EEPROM_Config_WriteByte(pgn_a_addr + 1, pgn_b)) {
        return 0;
    }
    
    return 1;
}

/**
 * Get diagnostic information - byte read count
 */
uint32_t EEPROM_Config_GetByteReadCount(void) {
    return byte_read_count;
}

/**
 * Get diagnostic information - byte write count
 */
uint32_t EEPROM_Config_GetByteWriteCount(void) {
    return byte_write_count;
}

/**
 * Get diagnostic information - write failures
 */
uint16_t EEPROM_Config_GetWriteFailures(void) {
    return write_failures;
}