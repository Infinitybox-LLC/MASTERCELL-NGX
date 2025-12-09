/*
 * FILE: inlink.h
 * inLINK Message Aggregator for MASTERCELL NGX
 * 
 * Receives CAN messages with PGN format AFXXXX, translates to FFXXXX,
 * and stores for aggregation with MASTERCELL messages.
 */

#ifndef INLINK_H
#define INLINK_H

#include <xc.h>
#include <stdint.h>

// Maximum simultaneous inLINK messages to track
#define MAX_INLINK_MESSAGES 16

// inLINK message structure
// Stores translated PGN (FFXXXX), SA, and data bytes
// Total size: 12 bytes per entry
typedef struct {
    uint16_t pgn;           // Translated PGN (FFXXXX format)
    uint8_t source_addr;    // Source address from original message
    uint8_t data[8];        // Data bytes from inLINK message
    uint8_t valid;          // 1 = entry contains valid data, 0 = empty slot
} InLinkMessage;

// Function prototypes
void InLink_Init(void);
void InLink_ProcessMessage(uint32_t can_id, uint8_t *data);
uint8_t InLink_GetMessageCount(void);
InLinkMessage* InLink_GetMessage(uint8_t index);

// Debug functions
uint16_t InLink_GetReceivedCount(void);
uint16_t InLink_GetProcessedCount(void);
uint32_t InLink_GetLastID(void);

#endif