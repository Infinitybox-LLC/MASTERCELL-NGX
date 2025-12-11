/*
 * FILE: inlink.c
 * inLINK Message Aggregator Implementation
 * 
 * Standalone module for handling inLINK messages (AFXXXX -> FFXXXX translation)
 */

#include "inlink.h"
#include <string.h>

// inLINK message storage
// 16 entries x 12 bytes = 192 bytes RAM
static InLinkMessage inlink_messages[MAX_INLINK_MESSAGES];

// Debug counters
static uint16_t inlink_messages_received = 0;
static uint16_t inlink_messages_processed = 0;
static uint32_t last_inlink_id = 0;

void InLink_Init(void) {
    // Clear all inLINK message slots
    memset(inlink_messages, 0, sizeof(inlink_messages));
    inlink_messages_received = 0;
    inlink_messages_processed = 0;
    last_inlink_id = 0;
}

void InLink_ProcessMessage(uint32_t can_id, uint8_t *data) {
    if (data == NULL) {
        return;
    }
    
    // Extract PGN from CAN ID
    // CAN ID format: [Priority(3)][Reserved(1)][DP(1)][PF(8)][PS(8)][SA(8)]
    // PGN = bits 8-23 of CAN ID
    uint16_t pgn = (can_id >> 8) & 0xFFFF;
    uint8_t source_addr = can_id & 0xFF;
    
    // Check if PGN starts with 'A' (0xAXXX format)
    // PGN high byte must be 0xAX (where X is any nibble)
    uint8_t pgn_high_byte = (pgn >> 8) & 0xFF;
    if ((pgn_high_byte & 0xF0) != 0xA0) {
        // Not an inLINK message, ignore
        return;
    }
    
    // Exclude AF00 - it's handled by Climate and Outputs, not inLINK
    // Only process AF01, AF02, AF03, etc.
    if (pgn == 0xAF00) {
        return;
    }
    
    // Debug: Track inLINK message reception
    inlink_messages_received++;
    last_inlink_id = can_id;
    
    // Translate PGN: AFXXXX -> FFXXXX
    // Keep low 12 bits, replace high nibble with 0xF
    uint16_t translated_pgn = (pgn & 0x0FFF) | 0xF000;
    
    // Search for existing entry with same translated PGN and SA
    uint8_t found_index = 0xFF;
    for (uint8_t i = 0; i < MAX_INLINK_MESSAGES; i++) {
        if (inlink_messages[i].valid &&
            inlink_messages[i].pgn == translated_pgn &&
            inlink_messages[i].source_addr == source_addr) {
            found_index = i;
            break;
        }
    }
    
    // If found, update existing entry
    if (found_index != 0xFF) {
        memcpy(inlink_messages[found_index].data, data, 8);
        inlink_messages_processed++;
        return;
    }
    
    // Not found - need to add new entry
    // Search for first empty slot
    uint8_t empty_index = 0xFF;
    for (uint8_t i = 0; i < MAX_INLINK_MESSAGES; i++) {
        if (!inlink_messages[i].valid) {
            empty_index = i;
            break;
        }
    }
    
    // If no empty slot, overwrite oldest entry (index 0)
    if (empty_index == 0xFF) {
        empty_index = 0;
    }
    
    // Store the translated message
    inlink_messages[empty_index].pgn = translated_pgn;
    inlink_messages[empty_index].source_addr = source_addr;
    memcpy(inlink_messages[empty_index].data, data, 8);
    inlink_messages[empty_index].valid = 1;
    
    inlink_messages_processed++;
}

uint8_t InLink_GetMessageCount(void) {
    uint8_t count = 0;
    for (uint8_t i = 0; i < MAX_INLINK_MESSAGES; i++) {
        if (inlink_messages[i].valid) {
            count++;
        }
    }
    return count;
}

InLinkMessage* InLink_GetMessage(uint8_t index) {
    if (index >= MAX_INLINK_MESSAGES) {
        return NULL;
    }
    
    if (!inlink_messages[index].valid) {
        return NULL;
    }
    
    return &inlink_messages[index];
}

// Debug functions
uint16_t InLink_GetReceivedCount(void) {
    return inlink_messages_received;
}

uint16_t InLink_GetProcessedCount(void) {
    return inlink_messages_processed;
}

uint32_t InLink_GetLastID(void) {
    return last_inlink_id;
}