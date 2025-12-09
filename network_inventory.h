/*
 * Network Inventory Module
 * Tracks devices on the CAN network by SA and PGN
 * Devices timeout after 60 seconds of no activity
 */

#ifndef NETWORK_INVENTORY_H
#define NETWORK_INVENTORY_H

#include <xc.h>
#include <stdint.h>

#define MAX_NETWORK_DEVICES 16
#define DEVICE_TIMEOUT_MS 60000  // 60 seconds

typedef struct {
    uint8_t source_addr;    // Source Address (SA)
    uint16_t pgn;          // PGN this device transmits
    uint32_t last_seen_ms; // Millisecond timestamp when last seen
    uint8_t active;        // 1 if this slot is in use, 0 if empty
} NetworkDevice;

// Function prototypes
void Network_Init(void);
void Network_UpdateDevice(uint8_t sa, uint16_t pgn, uint32_t timestamp_ms);
void Network_CheckTimeouts(uint32_t current_time_ms);
uint8_t Network_GetDeviceCount(void);
NetworkDevice* Network_GetDevice(uint8_t index);
void Network_Clear(void);

#endif // NETWORK_INVENTORY_H