#ifndef MESHCORE_PROTOCOL_IMPL_H
#define MESHCORE_PROTOCOL_IMPL_H

#include "../protocol_interface.h"
#include "meshcore_handler.h"
#include "config.h"

/**
 * MeshCore Protocol Implementation
 * 
 * Implements the ProtocolInterfaceImpl for MeshCore protocol.
 */

// Get MeshCore protocol interface implementation
ProtocolInterfaceImpl* meshcore_getProtocolInterface();

#endif // MESHCORE_PROTOCOL_IMPL_H
