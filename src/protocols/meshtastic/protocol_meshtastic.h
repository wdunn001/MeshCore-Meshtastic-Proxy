#ifndef MESHTASTIC_PROTOCOL_IMPL_H
#define MESHTASTIC_PROTOCOL_IMPL_H

#include "../protocol_interface.h"
#include "meshtastic_handler.h"
#include "config.h"

/**
 * Meshtastic Protocol Implementation
 * 
 * Implements the ProtocolInterfaceImpl for Meshtastic protocol.
 */

// Get Meshtastic protocol interface implementation
ProtocolInterfaceImpl* meshtastic_getProtocolInterface();

#endif // MESHTASTIC_PROTOCOL_IMPL_H
