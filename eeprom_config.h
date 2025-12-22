/*
 * FILE: eeprom_config.h
 * Byte-Level EEPROM Configuration Access for MASTERCELL NGX
 * 
 * Provides byte-level read/write access to configuration EEPROM
 * using read-modify-write operations on the underlying 16-bit word architecture.
 * 
 * EEPROM Configuration Map (Byte Addresses 0-26):
 * 0:  Bitrate (0x01=250k, 0x02=500k, 0x03=1M)
 * 1:  Heartbeat PGN A (high byte)
 * 2:  Heartbeat PGN B (low byte)
 * 3:  Heartbeat SA
 * 4:  Firmware Major Version
 * 5:  Firmware Minor Version
 * 6:  Rebroadcast Mode (0x01=edges, 0x02=periodic)
 * 7:  Init Stamp (0xA5 = initialized)
 * 8:  Reserved
 * 9:  Reserved
 * 10: Write Request PGN A
 * 11: Write Request PGN B
 * 12: Write Request SA
 * 13: Read Request PGN A
 * 14: Read Request PGN B
 * 15: Read Request SA
 * 16: Response PGN A
 * 17: Response PGN B
 * 18: Response SA
 * 19: Diagnostic PGN A
 * 20: Diagnostic PGN B
 * 21: Diagnostic SA
 * 22: Serial Number
 * 23: Customer Name Character 1 (ASCII)
 * 24: Customer Name Character 2 (ASCII)
 * 25: Customer Name Character 3 (ASCII)
 * 26: Customer Name Character 4 (ASCII)
 * 27-33: Reserved (for word alignment)
 * 34+: Input Cases (32 bytes each, starting at word address 0x0022)
 */

#ifndef EEPROM_CONFIG_H
#define EEPROM_CONFIG_H

#include <xc.h>
#include <stdint.h>

// EEPROM Configuration Address Definitions (Byte Addresses)
#define EEPROM_CFG_BITRATE              0
#define EEPROM_CFG_HEARTBEAT_PGN_A      1
#define EEPROM_CFG_HEARTBEAT_PGN_B      2
#define EEPROM_CFG_HEARTBEAT_SA         3
#define EEPROM_CFG_FW_MAJOR             4
#define EEPROM_CFG_FW_MINOR             5
#define EEPROM_CFG_REBROADCAST_MODE     6
#define EEPROM_CFG_INIT_STAMP           7
// inRESERVE configuration (2 bytes)
// Byte 8: [XXXX][YYYY] - X=PowerCell ID (0=disabled, 1=Front, 2=Rear, 3+), Y=Output (1-10)
// Byte 9: [ZZZZ][QQQQ] - Z=Time (0=5min, 1=10min, 2=15min, 3=20min), Q=Voltage (0=11.9V, 1=12.0V, ... B=13.0V)
#define EEPROM_CFG_INRESERVE_1          8   // PowerCell ID (upper nibble) + Output (lower nibble)
#define EEPROM_CFG_INRESERVE_2          9   // Time threshold (upper nibble) + Voltage threshold (lower nibble)
#define EEPROM_CFG_WRITE_REQ_PGN_A      10
#define EEPROM_CFG_WRITE_REQ_PGN_B      11
#define EEPROM_CFG_WRITE_REQ_SA         12
#define EEPROM_CFG_READ_REQ_PGN_A       13
#define EEPROM_CFG_READ_REQ_PGN_B       14
#define EEPROM_CFG_READ_REQ_SA          15
#define EEPROM_CFG_RESPONSE_PGN_A       16
#define EEPROM_CFG_RESPONSE_PGN_B       17
#define EEPROM_CFG_RESPONSE_SA          18
#define EEPROM_CFG_DIAGNOSTIC_PGN_A     19
#define EEPROM_CFG_DIAGNOSTIC_PGN_B     20
#define EEPROM_CFG_DIAGNOSTIC_SA        21
#define EEPROM_CFG_SERIAL_NUMBER        22
#define EEPROM_CFG_CUSTOMER_NAME_1      23
#define EEPROM_CFG_CUSTOMER_NAME_2      24
#define EEPROM_CFG_CUSTOMER_NAME_3      25
#define EEPROM_CFG_CUSTOMER_NAME_4      26

// Configuration value ranges
#define EEPROM_CFG_SIZE                 27      // Total configuration bytes (0-26)

// Default configuration values
#define DEFAULT_BITRATE                 0x01    // 250 kbps
#define DEFAULT_HB_PGN_A                0xFF
#define DEFAULT_HB_PGN_B                0x00
#define DEFAULT_HB_SA                   0x80
#define DEFAULT_FW_MAJOR                0x01
#define DEFAULT_FW_MINOR                0x00
#define DEFAULT_REBROADCAST_MODE        0x01    // Edge-triggered
#define DEFAULT_INIT_STAMP              0xA5    // Magic marker
#define DEFAULT_WRITE_REQ_PGN_A         0xFF
#define DEFAULT_WRITE_REQ_PGN_B         0x10
#define DEFAULT_WRITE_REQ_SA            0x80
#define DEFAULT_READ_REQ_PGN_A          0xFF
#define DEFAULT_READ_REQ_PGN_B          0x20
#define DEFAULT_READ_REQ_SA             0x80
#define DEFAULT_RESPONSE_PGN_A          0xFF
#define DEFAULT_RESPONSE_PGN_B          0x30
#define DEFAULT_RESPONSE_SA             0x80
#define DEFAULT_DIAGNOSTIC_PGN_A        0xFF
#define DEFAULT_DIAGNOSTIC_PGN_B        0x40
#define DEFAULT_DIAGNOSTIC_SA           0x80
#define DEFAULT_SERIAL_NUMBER           0x42    // Demo value
#define DEFAULT_CUSTOMER_NAME_1         0x20    // ASCII space
#define DEFAULT_CUSTOMER_NAME_2         0x20    // ASCII space
#define DEFAULT_CUSTOMER_NAME_3         0x20    // ASCII space
#define DEFAULT_CUSTOMER_NAME_4         0x20    // ASCII space

// inRESERVE default values
// Default: DISABLED (0x0), Output 9 (0x9), 30 sec (0x0), 12.3V (0x2)
#define DEFAULT_INRESERVE_1             0x09    // PowerCell 0 (DISABLED), Output 9
#define DEFAULT_INRESERVE_2             0x02    // 30 seconds (code 0), 12.3V (code 2)

// Bitrate codes
#define BITRATE_250K                    0x01
#define BITRATE_500K                    0x02
#define BITRATE_1M                      0x03

// Rebroadcast mode codes
#define REBROADCAST_EDGES               0x01    // Change-of-state only
#define REBROADCAST_PERIODIC            0x02    // Rebroadcast every heartbeat

/**
 * Read a single byte from EEPROM configuration area
 * Uses read-modify approach to access individual bytes from 16-bit words
 * 
 * @param byte_addr Byte address (0-26 for config, 27+ for reserved/cases)
 * @return Byte value at the specified address
 */
uint8_t EEPROM_Config_ReadByte(uint16_t byte_addr);

/**
 * Write a single byte to EEPROM configuration area
 * Uses read-modify-write to update individual bytes in 16-bit words
 * 
 * @param byte_addr Byte address (0-26 for config)
 * @param value Byte value to write
 * @return 1 if successful, 0 if failed
 */
uint8_t EEPROM_Config_WriteByte(uint16_t byte_addr, uint8_t value);

/**
 * Read multiple consecutive bytes from EEPROM
 * 
 * @param start_addr Starting byte address
 * @param buffer Buffer to store read bytes
 * @param length Number of bytes to read
 * @return Number of bytes successfully read
 */
uint16_t EEPROM_Config_ReadBytes(uint16_t start_addr, uint8_t *buffer, uint16_t length);

/**
 * Write multiple consecutive bytes to EEPROM
 * 
 * @param start_addr Starting byte address
 * @param buffer Buffer containing bytes to write
 * @param length Number of bytes to write
 * @return Number of bytes successfully written
 */
uint16_t EEPROM_Config_WriteBytes(uint16_t start_addr, const uint8_t *buffer, uint16_t length);

/**
 * Check if configuration EEPROM has been initialized
 * Checks for Init Stamp (0xA5) at byte address 7
 * 
 * @return 1 if initialized, 0 if blank
 */
uint8_t EEPROM_Config_IsInitialized(void);

/**
 * Read a 16-bit PGN from two consecutive bytes
 * 
 * @param pgn_a_addr Byte address of PGN high byte
 * @return Combined 16-bit PGN value (PGN_A << 8 | PGN_B)
 */
uint16_t EEPROM_Config_ReadPGN(uint16_t pgn_a_addr);

/**
 * Write a 16-bit PGN to two consecutive bytes
 * 
 * @param pgn_a_addr Byte address of PGN high byte (PGN_B is at pgn_a_addr + 1)
 * @param pgn 16-bit PGN value to write
 * @return 1 if successful, 0 if failed
 */
uint8_t EEPROM_Config_WritePGN(uint16_t pgn_a_addr, uint16_t pgn);

/**
 * Get diagnostic information - number of byte read operations
 * @return Total byte reads since init
 */
uint32_t EEPROM_Config_GetByteReadCount(void);

/**
 * Get diagnostic information - number of byte write operations
 * @return Total byte writes since init
 */
uint32_t EEPROM_Config_GetByteWriteCount(void);

/**
 * Get diagnostic information - number of write failures
 * @return Total write failures
 */
uint16_t EEPROM_Config_GetWriteFailures(void);

#endif // EEPROM_CONFIG_H