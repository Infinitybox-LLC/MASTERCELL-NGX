/*
 * FILE: eeprom_init_front_engine.c
 * Standard Front Engine Configuration for MASTERCELL NGX
 * 
 * This contains the complete Front Engine configuration that was
 * previously in EEPROM_InitMockData()
 */

#include "eeprom_init.h"
#include "eeprom_cases.h"
#include "eeprom_config.h"
#include <string.h>
#include <stdio.h>

#define FCY 16000000UL
#include <libpic30.h>

/*
 * Load Standard Front Engine Configuration
 */
void EEPROM_LoadFrontEngine(void) {
    uint16_t addr;
    uint8_t data[8];
    uint8_t priority, source_addr;
    uint16_t pgn;
    uint8_t config_byte, pattern_timing;
    uint8_t requires_ignition;
    char can_id_str[9];
    
    // ========================================
    // Configuration Bytes (0-22)
    // ========================================
    
    // Byte 0-1: Bitrate (0x01) + Heartbeat PGN A (0xFF)
    EEPROM_WriteBytePair(0x0000, DEFAULT_BITRATE, 0xFF);
    
    // Byte 2-3: Heartbeat PGN B (0x00) + Heartbeat SA (0x80)
    EEPROM_WriteBytePair(0x0002, 0x00, 0x80);
    
    // Byte 4-5: Firmware Major (0x01) + Firmware Minor (0x07)
    EEPROM_WriteBytePair(0x0004, DEFAULT_FW_MAJOR, 0x07);
    
    // Byte 6-7: Rebroadcast Mode (0x01) + Init Stamp (0xA5)
    EEPROM_WriteBytePair(0x0006, DEFAULT_REBROADCAST_MODE, DEFAULT_INIT_STAMP);
    
    // Byte 8-9: Reserved (0xFF) + Reserved (0xFF)
    EEPROM_WriteBytePair(0x0008, 0xFF, 0xFF);
    
    // Byte 10-11: Write Request PGN A (0xFF) + Write Request PGN B (0x10)
    EEPROM_WriteBytePair(0x000A, 0xFF, 0x10);
    
    // Byte 12-13: Write Request SA (0x80) + Read Request PGN A (0xFF)
    EEPROM_WriteBytePair(0x000C, 0x80, 0xFF);
    
    // Byte 14-15: Read Request PGN B (0x20) + Read Request SA (0x80)
    EEPROM_WriteBytePair(0x000E, 0x20, 0x80);
    
    // Byte 16-17: Response PGN A (0xFF) + Response PGN B (0x30)
    EEPROM_WriteBytePair(0x0010, 0xFF, 0x30);
    
    // Byte 18-19: Response SA (0x80) + Diagnostic PGN A (0xFF)
    EEPROM_WriteBytePair(0x0012, 0x80, 0xFF);
    
    // Byte 20-21: Diagnostic PGN B (0x40) + Diagnostic SA (0x80)
    EEPROM_WriteBytePair(0x0014, 0x40, 0x80);
    
    // Byte 22-23: Serial Number (0x42) + Customer Name 1 (0x46 'F')
    EEPROM_WriteBytePair(0x0016, DEFAULT_SERIAL_NUMBER, 0x46);
    
    // Byte 24-25: Customer Name 2 (0x52 'R') + Customer Name 3 (0x4E 'N')
    EEPROM_WriteBytePair(0x0018, 0x52, 0x4E);
    
    // Byte 26-27: Customer Name 4 (0x54 'T') + Reserved (0xFF)
    EEPROM_WriteBytePair(0x001A, 0x54, 0xFF);
    
    // Byte 28-33: Reserved bytes (fill with 0xFF)
    for(uint8_t i = 0x001C; i < 0x0022; i += 2) {
        EEPROM_WriteBytePair(i, 0xFF, 0xFF);
    }
    
    // ========================================
    // ON Cases Configuration
    // ========================================
    
    // Starting address for ON cases
    addr = 0x0022;

    // IN01 - 4 ON cases - Ignition
    ParseCANID("18FF011E", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x20;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x01, 0x00, 0, data);
    addr += 32;
    
    EEPROM_WriteInvalidCase(addr);
    addr += 32;
    
    EEPROM_WriteInvalidCase(addr);
    addr += 32;
	
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN02 - 2 ON cases - Starter
    ParseCANID("18FF011E", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x10;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN03 - 4 ON cases - Left Turn Signal
    ParseCANID("18FF011E", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x80;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x33, 1, data);  // Pattern timing 0x33
    addr += 32;
    
    ParseCANID("18FF021E", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x80;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x33, 1, data);  // Pattern timing 0x33
    addr += 32;
    
    EEPROM_WriteInvalidCase(addr);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN04 - 4 ON cases - Right Turn Signal
    ParseCANID("18FF011E", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x40;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x33, 1, data);  // Pattern timing 0x33
    addr += 32;
    
    ParseCANID("18FF021E", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x40;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x33, 1, data);  // Pattern timing 0x33
    addr += 32;
    
    EEPROM_WriteInvalidCase(addr);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN05 - 2 ON cases - Headlights
    ParseCANID("18FF011E", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x08;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN06 - 6 ON cases - Parking Lights
    ParseCANID("18FF011E", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x04;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    
    ParseCANID("18FF021E", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x04;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    
    EEPROM_WriteInvalidCase(addr);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN07 - 1 ON case - High Beams
    ParseCANID("18FF011E", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x02;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;

    // IN08 - 6 ON cases - Hazards/4-Way
    ParseCANID("18FF011E", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0xC0;  // Both turn signals
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x33, 0, data);  
    addr += 32;
    
    ParseCANID("18FF021E", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0xC0;  // Both rear turn signals
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x33, 0, data);  
    addr += 32;
    
    EEPROM_WriteInvalidCase(addr);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN09 - 1 ON case - Horn
    ParseCANID("18FF011E", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x00;
    data[1] = 0x80;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;

    // IN10 - 2 ON cases - Cooling Fan
    ParseCANID("18FF011E", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x00;
    data[1] = 0x40;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);  
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN11 - 2 ON cases- Brake Light- 1 Filament
    ParseCANID("18FF021E", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0xC0;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x02, 0x00, 0, data);  
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN12 - 2 ON cases - Brake Light- Multi
    ParseCANID("18FF021E", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x20;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);  
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN13 - 2 ON cases Fuel Pump
    ParseCANID("18FF021E", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x00;
    data[1] = 0x40;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);  
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN14 - 2 ON cases
    EEPROM_WriteInvalidCase(addr);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN15 - 6 ON cases - One Button Start
    ParseCANID("18FF011E", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x20;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x80;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x11, 0x00, 0, data);  // One-button start
    addr += 32;
    
    ParseCANID("18FF011E", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x02;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x01, 0x1E, 0, data);  // 30x100ms delay
    addr += 32;
    
    EEPROM_WriteInvalidCase(addr);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN16 - 2 ON cases
    EEPROM_WriteInvalidCase(addr);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN17 - 2 ON cases - Backup Lights  
    ParseCANID("18FF021E", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x08;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 1, data);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN18 - 6 ON cases - Interior Lights
    ParseCANID("18FF021E", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x10;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    
    // IN18 - Remaining 5 invalid cases (6 total, 1 valid above)
    EEPROM_WriteInvalidCase(addr);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN19 - 2 ON cases- OPEN
    ParseCANID("18FF011E", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x01;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN20 - 2 ON cases- OPEN
    ParseCANID("18FF021E", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x02;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN21 - 2 ON cases- OPEN
    ParseCANID("18FF021E", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x01;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN22 - 2 ON cases- OPEN
    ParseCANID("18FF021E", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x00;
    data[1] = 0x80;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN23 - 6 ON cases
    ParseCANID("18FF031A", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x00;
    data[1] = 0x00;
    data[2] = 0xA2;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    
    ParseCANID("18FF041A", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x00;
    data[1] = 0x00;
    data[2] = 0xA2;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    
    ParseCANID("18FF051A", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x00;
    data[1] = 0x00;
    data[2] = 0xA2;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    
    ParseCANID("18FF061A", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x00;
    data[1] = 0x00;
    data[2] = 0xA2;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    
    EEPROM_WriteInvalidCase(addr);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN24 - 6 ON cases
    ParseCANID("18FF031A", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x00;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0xA2;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    
    ParseCANID("18FF041A", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x00;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0xA2;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    
    ParseCANID("18FF051A", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x00;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0xA2;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    
    ParseCANID("18FF061A", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x00;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0xA2;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    
    EEPROM_WriteInvalidCase(addr);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN25 - 2 ON cases - Window (Driver Front)
    ParseCANID("18FF031A", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x90;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN26 - 2 ON cases - Window (Passenger Front)
    ParseCANID("18FF031A", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x00;
    data[1] = 0x90;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN27 - 2 ON cases - Window (Driver Rear)
    ParseCANID("18FF041A", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x90;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN28 - 2 ON cases - Window (Passenger Rear)
    ParseCANID("18FF041A", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x00;
    data[1] = 0x90;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN29 - 2 ON cases - Window (Driver Front) DOWN
    ParseCANID("18FF051A", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x90;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN30 - 2 ON cases - Window (Passenger Front) DOWN
    ParseCANID("18FF051A", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x00;
    data[1] = 0x90;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN31 - 2 ON cases - Window (Driver Rear) DOWN
    ParseCANID("18FF061A", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x90;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN32 - 2 ON cases - Window (Passenger Rear) DOWN
    ParseCANID("18FF061A", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x00;
    data[1] = 0x90;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN33 - 1 ON case - Aux Input
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN34 - 1 ON case - Aux Input
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN35 - 1 ON case - Aux Input
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN36 - 1 ON case - Aux Input
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN37 - 1 ON case - Aux Input
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN38 - 1 ON case - Aux Input
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // HSIN01 - 2 ON cases - High Side Cooling
    ParseCANID("18FF011E", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x00;
    data[1] = 0x40;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // HSIN02 - 2 ON cases - High Side Fuel
    ParseCANID("18FF021E", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x00;
    data[1] = 0x40;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // HSIN03 - 1 ON case - High Side Aux
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // HSIN04 - 1 ON case - High Side Aux
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // HSIN05 - 1 ON case - High Side Aux
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // HSIN06 - 1 ON case - High Side Aux
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // ========================================
    // OFF Cases Configuration 
    // Starting at 0x0D62
    // ========================================
    
    addr = 0x0D62;

    // IN01 - 2 OFF cases at offset 0 - Ignition OFF
    ParseCANID("18FF021A", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x00;  // Clear ignition bit
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);  // Second case invalid for now
    addr += 32;

    // IN02 - 2 OFF cases at offset 64 - Starter OFF
    ParseCANID("18FF021A", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[1] = 0x00;  // Clear starter bit
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);  // Second case invalid for now
    addr += 32;

    // CRITICAL: IN03-IN24 have NO OFF cases in EEPROM
    // Add 32-byte padding to align with offset 160 for IN25
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN25 - 2 OFF cases at offset 160 - Window UP Stop
    ParseCANID("18FF031A", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x80;  // Window stop command
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN26 - 2 OFF cases at offset 224 - Window UP Stop
    ParseCANID("18FF031A", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[1] = 0x80;  // Window stop command
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN27 - 2 OFF cases at offset 288 - Window DOWN Stop
    ParseCANID("18FF041A", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x80;  // Window stop command
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN28 - 2 OFF cases at offset 352 - Window DOWN Stop
    ParseCANID("18FF041A", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[1] = 0x80;  // Window stop command
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN29 - 2 OFF cases at offset 416 - Window UP Stop
    ParseCANID("18FF051A", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x80;  // Window stop command
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN30 - 2 OFF cases at offset 480 - Window UP Stop
    ParseCANID("18FF051A", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[1] = 0x80;  // Window stop command
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN31 - 2 OFF cases at offset 544 - Window DOWN Stop
    ParseCANID("18FF061A", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[0] = 0x80;  // Window stop command
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;

    // IN32 - 2 OFF cases at offset 608 - Window DOWN Stop
    ParseCANID("18FF061A", &priority, &pgn, &source_addr);
    memset(data, 0x00, 8);
    data[1] = 0x80;  // Window stop command
    EEPROM_WriteCase(addr, priority, pgn, source_addr, 0x00, 0x00, 0, data);
    addr += 32;
    EEPROM_WriteInvalidCase(addr);
    addr += 32;
    
    // End of OFF cases - IN33-IN44 and HSIN01-HSIN06 have no OFF cases
}