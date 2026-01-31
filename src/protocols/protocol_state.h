#ifndef PROTOCOL_STATE_H
#define PROTOCOL_STATE_H

#include "protocol_manager.h"  // Now in same directory

/**
 * Protocol State Definitions
 * 
 * Shared definitions for protocol state management.
 * Used by both main.cpp and usb_comm.cpp.
 */

// Protocol listening state enum (maps to ProtocolId)
// Maps to ProtocolId: LISTENING_MESHCORE = PROTOCOL_MESHCORE, LISTENING_MESHTASTIC = PROTOCOL_MESHTASTIC
typedef enum {
    LISTENING_MESHCORE = PROTOCOL_MESHCORE,
    LISTENING_MESHTASTIC = PROTOCOL_MESHTASTIC
} ProtocolState;

#endif // PROTOCOL_STATE_H
