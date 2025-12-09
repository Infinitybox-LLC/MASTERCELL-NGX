/*
 * J1939 CAN Communication Header
 */

#ifndef J1939_H
#define J1939_H

#include <xc.h>
#include <stdint.h>

// J1939 Configuration
#define J1939_SOURCE_ADDR 0x80
#define J1939_PGN 0xFF00
#define J1939_PRIORITY 6

// Buffer size for received messages
#define CAN_RX_BUFFER_SIZE 8

// CAN message structure
typedef struct {
    uint32_t id;
    uint8_t data[8];
    uint8_t dlc;
    uint8_t valid;
} CAN_RxMessage;

// Function prototypes
void J1939_Init(void);
void J1939_TransmitMessage(uint8_t priority, uint16_t pgn, uint8_t source_addr, uint8_t *data);
void J1939_TransmitHeartbeat(void);
uint8_t J1939_IsTxReady(void);
uint8_t J1939_ReceiveMessage(CAN_RxMessage *msg);
void J1939_ConfigureFilters(uint16_t read_pgn, uint8_t read_sa, uint16_t write_pgn, uint8_t write_sa);
void J1939_SetPromiscuousMode(void);
uint8_t J1939_HasRxOverflow(void);
void J1939_ClearRxOverflow(void);
uint8_t J1939_GetRxCount(void);
uint32_t J1939_GetRxMessageCount(void);
uint16_t J1939_GetRxOverflowCount(void);

// DEBUG functions
uint16_t J1939_GetDebugSID(void);
uint16_t J1939_GetDebugEID(void);
uint16_t J1939_GetDebugDLC(void);

#endif