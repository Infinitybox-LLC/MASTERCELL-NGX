/*
 * J1939 CAN Communication Implementation
 * FIXED: Correct bit positions for dsPIC30F6012A
 */

#include "j1939.h"
#include "eeprom_config.h"
#include "inputs.h"
#include <string.h>

#define FCY 16000000UL
#include <libpic30.h>

static CAN_RxMessage rx_buffer[CAN_RX_BUFFER_SIZE];
static uint8_t rx_write_index = 0;
static uint8_t rx_read_index = 0;
static uint8_t rx_count = 0;
static uint8_t rx_overflow_flag = 0;

static uint32_t rx_message_count = 0;
static uint16_t rx_overflow_count = 0;

volatile uint16_t debug_sid_reg = 0;
volatile uint16_t debug_eid_reg = 0;
volatile uint16_t debug_dlc_reg = 0;

void J1939_Init(void) {
    uint16_t timeout;
    
    memset(rx_buffer, 0, sizeof(rx_buffer));
    rx_write_index = 0;
    rx_read_index = 0;
    rx_count = 0;
    rx_overflow_flag = 0;
    rx_message_count = 0;
    rx_overflow_count = 0;
    
    C1CTRLbits.REQOP = 4;
    timeout = 10000;
    while(C1CTRLbits.OPMODE != 4 && timeout > 0) timeout--;
    if (timeout == 0) return;
    
    C1CFG1bits.BRP = 7;
    C1CFG1bits.SJW = 0;
    
    C1CFG2bits.PRSEG = 6;
    C1CFG2bits.SEG1PH = 3;
    C1CFG2bits.SEG2PHTS = 1;
    C1CFG2bits.SEG2PH = 3;
    C1CFG2bits.SAM = 0;
    
    C1CTRLbits.CANCKS = 0;
    
    C1TX0CONbits.TXPRI = 0b11;
    
    C1RX0CONbits.DBEN = 0;
    
    J1939_SetPromiscuousMode();
    
    C1INTEbits.RX0IE = 1;
    
    IEC1 &= ~0x0002;
    IFS1 &= ~0x0002;
    
    C1CTRLbits.REQOP = 0;
    timeout = 10000;
    while(C1CTRLbits.OPMODE != 0 && timeout > 0) timeout--;
    
    __delay_ms(10);
}

void __attribute__((interrupt, no_auto_psv)) _C1Interrupt(void) {
    if (C1RX0CONbits.RXFUL) {
        if (rx_count >= CAN_RX_BUFFER_SIZE) {
            rx_overflow_flag = 1;
            rx_overflow_count++;
            C1RX0CONbits.RXFUL = 0;
            IFS1 &= ~0x0002;
            return;
        }
        
        uint16_t sid_reg = C1RX0SID;
        uint16_t eid_reg = C1RX0EID;
        uint16_t dlc_reg = C1RX0DLC;
        
        // CORRECT extraction for dsPIC30F6012A:
        // C1RX0SID[12:2] = EID[28:18] (11 bits)
        uint16_t eid_28_18 = (sid_reg >> 2) & 0x7FF;
        
        // C1RX0EID[11:0] = EID[17:6] (12 bits)
        uint16_t eid_17_6 = eid_reg & 0xFFF;
        
        // C1RX0DLC[15:10] = EID[5:0] (6 bits)
        uint8_t eid_5_0 = (dlc_reg >> 10) & 0x3F;
        
        // Reconstruct 29-bit extended ID
        uint32_t msg_id = ((uint32_t)eid_28_18 << 18) |
                          ((uint32_t)eid_17_6 << 6) |
                          eid_5_0;
        
        // C1RX0DLC[3:0] = DLC
        uint8_t dlc = dlc_reg & 0x0F;
        if (dlc > 8) dlc = 8;
        
        rx_buffer[rx_write_index].id = msg_id;
        rx_buffer[rx_write_index].dlc = dlc;
        rx_buffer[rx_write_index].valid = 1;
        
        uint16_t data_word;
        data_word = C1RX0B1;
        rx_buffer[rx_write_index].data[0] = data_word & 0xFF;
        rx_buffer[rx_write_index].data[1] = (data_word >> 8) & 0xFF;
        
        data_word = C1RX0B2;
        rx_buffer[rx_write_index].data[2] = data_word & 0xFF;
        rx_buffer[rx_write_index].data[3] = (data_word >> 8) & 0xFF;
        
        data_word = C1RX0B3;
        rx_buffer[rx_write_index].data[4] = data_word & 0xFF;
        rx_buffer[rx_write_index].data[5] = (data_word >> 8) & 0xFF;
        
        data_word = C1RX0B4;
        rx_buffer[rx_write_index].data[6] = data_word & 0xFF;
        rx_buffer[rx_write_index].data[7] = (data_word >> 8) & 0xFF;
        
        rx_write_index = (rx_write_index + 1) % CAN_RX_BUFFER_SIZE;
        rx_count++;
        rx_message_count++;
        
        C1RX0CONbits.RXFUL = 0;
    }
    
    IFS1 &= ~0x0002;
}

uint8_t J1939_ReceiveMessage(CAN_RxMessage *msg) {
    if (msg == NULL) {
        return 0;
    }
    
    if (C1RX0CONbits.RXFUL) {
        uint16_t sid_reg = C1RX0SID;
        uint16_t eid_reg = C1RX0EID;
        uint16_t dlc_reg = C1RX0DLC;
        
        debug_sid_reg = sid_reg;
        debug_eid_reg = eid_reg;
        debug_dlc_reg = dlc_reg;
        
        // CORRECT extraction for dsPIC30F6012A:
        // C1RX0SID[12:2] = EID[28:18] (11 bits)
        uint16_t eid_28_18 = (sid_reg >> 2) & 0x7FF;
        
        // C1RX0EID[11:0] = EID[17:6] (12 bits)
        uint16_t eid_17_6 = eid_reg & 0xFFF;
        
        // C1RX0DLC[15:10] = EID[5:0] (6 bits)
        uint8_t eid_5_0 = (dlc_reg >> 10) & 0x3F;
        
        // Reconstruct 29-bit extended ID
        msg->id = ((uint32_t)eid_28_18 << 18) |
                  ((uint32_t)eid_17_6 << 6) |
                  eid_5_0;
        
        // C1RX0DLC[3:0] = DLC
        msg->dlc = dlc_reg & 0x0F;
        if (msg->dlc > 8) msg->dlc = 8;
        msg->valid = 1;
        
        uint16_t data_word;
        data_word = C1RX0B1;
        msg->data[0] = data_word & 0xFF;
        msg->data[1] = (data_word >> 8) & 0xFF;
        data_word = C1RX0B2;
        msg->data[2] = data_word & 0xFF;
        msg->data[3] = (data_word >> 8) & 0xFF;
        data_word = C1RX0B3;
        msg->data[4] = data_word & 0xFF;
        msg->data[5] = (data_word >> 8) & 0xFF;
        data_word = C1RX0B4;
        msg->data[6] = data_word & 0xFF;
        msg->data[7] = (data_word >> 8) & 0xFF;
        
        C1RX0CONbits.RXFUL = 0;
        rx_message_count++;
        
        return 1;
    }
    
    if (rx_count == 0) {
        return 0;
    }
    
    IEC1 &= ~0x0002;
    
    memcpy(msg, &rx_buffer[rx_read_index], sizeof(CAN_RxMessage));
    
    rx_buffer[rx_read_index].valid = 0;
    
    rx_read_index = (rx_read_index + 1) % CAN_RX_BUFFER_SIZE;
    rx_count--;
    
    IEC1 |= 0x0002;
    
    return 1;
}

void J1939_ConfigureFilters(uint16_t read_pgn, uint8_t read_sa, 
                            uint16_t write_pgn, uint8_t write_sa) {
    J1939_SetPromiscuousMode();
}

void J1939_SetPromiscuousMode(void) {
    uint16_t timeout;
    
    C1CTRLbits.REQOP = 4;
    timeout = 10000;
    while(C1CTRLbits.OPMODE != 4 && timeout > 0) timeout--;
    if (timeout == 0) return;
    
    C1RXM0SID = 0x0000;
    C1RXM0EIDH = 0x0000;
    C1RXM0EIDL = 0x0000;
    C1RXM1SID = 0x0000;
    C1RXM1EIDH = 0x0000;
    C1RXM1EIDL = 0x0000;
    
    C1RXF0SID = 0x0001;
    C1RXF0EIDH = 0x0000;
    C1RXF0EIDL = 0x0000;
    
    C1RXF0SID |= (0 << 1);
    
    C1CTRLbits.REQOP = 0;
    timeout = 10000;
    while(C1CTRLbits.OPMODE != 0 && timeout > 0) timeout--;
}

uint8_t J1939_IsTxReady(void) {
    return (C1TX0CONbits.TXREQ == 0);
}

void J1939_TransmitMessage(uint8_t priority, uint16_t pgn, uint8_t source_addr, uint8_t *data) {
    uint16_t timeout = 10000;
    while(C1TX0CONbits.TXREQ && timeout > 0) {
        timeout--;
    }
    
    if(timeout == 0) {
        return;
    }
    
    uint8_t pf = (pgn >> 8) & 0xFF;
    uint8_t ps = pgn & 0xFF;
    
    uint32_t msg_id = ((uint32_t)priority << 26) | 
                      ((uint32_t)pf << 16) | 
                      ((uint32_t)ps << 8) | 
                      source_addr;
    
    uint16_t sid = (msg_id >> 18) & 0x7FF;
    uint8_t sid_10_6 = (sid >> 6) & 0x1F;
    uint8_t sid_5_0 = sid & 0x3F;
    
    uint32_t eid = msg_id & 0x3FFFF;
    uint8_t eid_17_14 = (eid >> 14) & 0x0F;
    uint8_t eid_13_6 = (eid >> 6) & 0xFF;
    uint8_t eid_5_0 = eid & 0x3F;
    
    C1TX0SID = ((uint16_t)sid_10_6 << 11) |
               ((uint16_t)sid_5_0 << 2) |
               (1 << 1) |
               (1 << 0);
    
    C1TX0EID = ((uint16_t)eid_17_14 << 12) |
               ((uint16_t)eid_13_6 << 0);
    
    C1TX0DLC = ((uint16_t)eid_5_0 << 10) |
               (0 << 9) |
               (0 << 8) |
               (0 << 7) |
               (8 << 3);
    
    C1TX0B1 = ((uint16_t)data[1] << 8) | data[0];
    C1TX0B2 = ((uint16_t)data[3] << 8) | data[2];
    C1TX0B3 = ((uint16_t)data[5] << 8) | data[4];
    C1TX0B4 = ((uint16_t)data[7] << 8) | data[6];
    
    C1TX0CONbits.TXREQ = 1;
}

void J1939_TransmitHeartbeat(void) {
    // Read heartbeat PGN and SA from EEPROM configuration
    uint16_t heartbeat_pgn = EEPROM_Config_ReadPGN(EEPROM_CFG_HEARTBEAT_PGN_A);
    uint8_t heartbeat_sa = EEPROM_Config_ReadByte(EEPROM_CFG_HEARTBEAT_SA);
    
    // Build heartbeat data
    uint8_t heartbeat_data[8];
    
    // Byte 0 (B0): Ignition status in bit 0, rest are 0
    // Bit 0: 0x00 = ignition off, 0x01 = ignition on
    // Bits 1-7: 0x00 (reserved for future use)
    uint8_t ignition_status = Inputs_GetIgnitionState();
    heartbeat_data[0] = ignition_status & 0x01;  // Only use bit 0
    
    // Bytes 1-7: Set to 0x00 for now (can be used for other status info later)
    heartbeat_data[1] = 0x00;
    heartbeat_data[2] = 0x00;
    heartbeat_data[3] = 0x00;
    heartbeat_data[4] = 0x00;
    heartbeat_data[5] = 0x00;
    heartbeat_data[6] = 0x00;
    heartbeat_data[7] = 0x00;
    
    // Transmit heartbeat with configured PGN and SA
    J1939_TransmitMessage(J1939_PRIORITY, heartbeat_pgn, heartbeat_sa, heartbeat_data);
}

uint8_t J1939_HasRxOverflow(void) {
    return rx_overflow_flag;
}

void J1939_ClearRxOverflow(void) {
    rx_overflow_flag = 0;
}

uint8_t J1939_GetRxCount(void) {
    return rx_count;
}

uint32_t J1939_GetRxMessageCount(void) {
    return rx_message_count;
}

uint16_t J1939_GetRxOverflowCount(void) {
    return rx_overflow_count;
}

uint16_t J1939_GetDebugSID(void) {
    return debug_sid_reg;
}

uint16_t J1939_GetDebugEID(void) {
    return debug_eid_reg;
}

uint16_t J1939_GetDebugDLC(void) {
    return debug_dlc_reg;
}