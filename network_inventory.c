/*
 * Network Inventory Module Implementation
 * Tracks devices on the CAN network by SA and PGN
 */

#include "network_inventory.h"
#include <string.h>

static NetworkDevice devices[MAX_NETWORK_DEVICES];
static uint8_t device_count = 0;

void Network_Init(void) {
    // Explicitly initialize all device slots
    for(uint8_t i = 0; i < MAX_NETWORK_DEVICES; i++) {
        devices[i].source_addr = 0;
        devices[i].pgn = 0;
        devices[i].last_seen_ms = 0;
        devices[i].active = 0;
        for(uint8_t j = 0; j < 8; j++) {
            devices[i].data[j] = 0;
        }
    }
    device_count = 0;
}

void Network_UpdateDevice(uint8_t sa, uint16_t pgn, uint32_t timestamp_ms, uint8_t *data) {
    // First, check if this SA+PGN combination already exists
    for(uint8_t i = 0; i < MAX_NETWORK_DEVICES; i++) {
        if(devices[i].active) {
            if(devices[i].source_addr == sa && devices[i].pgn == pgn) {
                // Found existing device with same SA and PGN - update timestamp and data
                devices[i].last_seen_ms = timestamp_ms;
                if(data != NULL) {
                    for(uint8_t j = 0; j < 8; j++) {
                        devices[i].data[j] = data[j];
                    }
                }
                return;
            }
        }
    }
    
    // Device not found - add new device
    for(uint8_t i = 0; i < MAX_NETWORK_DEVICES; i++) {
        if(devices[i].active == 0) {
            // Found empty slot
            devices[i].source_addr = sa;
            devices[i].pgn = pgn;
            devices[i].last_seen_ms = timestamp_ms;
            devices[i].active = 1;
            if(data != NULL) {
                for(uint8_t j = 0; j < 8; j++) {
                    devices[i].data[j] = data[j];
                }
            } else {
                for(uint8_t j = 0; j < 8; j++) {
                    devices[i].data[j] = 0;
                }
            }
            device_count++;
            return;
        }
    }
    
    // No empty slots available - network is full
    // Could implement LRU (Least Recently Used) replacement here if needed
}

void Network_CheckTimeouts(uint32_t current_time_ms) {
    for(uint8_t i = 0; i < MAX_NETWORK_DEVICES; i++) {
        if(devices[i].active) {
            // Check if device has timed out
            // Handle wrap-around of 32-bit timestamp
            uint32_t elapsed;
            if(current_time_ms >= devices[i].last_seen_ms) {
                elapsed = current_time_ms - devices[i].last_seen_ms;
            } else {
                // Wrap-around occurred
                elapsed = (0xFFFFFFFF - devices[i].last_seen_ms) + current_time_ms + 1;
            }
            
            if(elapsed > DEVICE_TIMEOUT_MS) {
                // Device has timed out - remove it
                devices[i].active = 0;
                device_count--;
            }
        }
    }
}

uint8_t Network_GetDeviceCount(void) {
    return device_count;
}

NetworkDevice* Network_GetDevice(uint8_t index) {
    uint8_t active_count = 0;
    
    // Find the Nth active device
    for(uint8_t i = 0; i < MAX_NETWORK_DEVICES; i++) {
        if(devices[i].active) {
            if(active_count == index) {
                return &devices[i];
            }
            active_count++;
        }
    }
    
    return NULL;  // Index out of range
}

NetworkDevice* Network_FindByPGN(uint16_t pgn) {
    for(uint8_t i = 0; i < MAX_NETWORK_DEVICES; i++) {
        if(devices[i].active && devices[i].pgn == pgn) {
            return &devices[i];
        }
    }
    return NULL;  // Not found
}

void Network_Clear(void) {
    memset(devices, 0, sizeof(devices));
    device_count = 0;
}