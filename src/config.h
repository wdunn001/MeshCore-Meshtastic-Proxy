#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/**
 * Generic Application Configuration
 * 
 * This file contains only generic, application-wide settings that are not
 * specific to any platform, radio, or protocol.
 * 
 * Platform-specific settings: src/platforms/{platformName}/config.h
 * Radio-specific settings: src/radio/{radioName}/config.h
 * Protocol-specific settings: src/protocols/{protocolName}/config.h
 */

// ============================================================================
// Time-slicing Configuration
// ============================================================================
// Controls how the proxy switches between listening to MeshCore and Meshtastic

#define PROTOCOL_SWITCH_INTERVAL_MS_DEFAULT 100  // Default: Switch between protocols every 100ms
#define PROTOCOL_SWITCH_INTERVAL_MS_MIN 50       // Minimum: 50ms
#define PROTOCOL_SWITCH_INTERVAL_MS_MAX 1000     // Maximum: 1000ms (1 second)

#endif // CONFIG_H
