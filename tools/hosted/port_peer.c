/*
 * Module contract
 *
 * Owns: the hosted port peer implementation: arming, the
 * fixed write journal with overflow accounting, and sticky
 * reads over the journal. See port_peer.h.
 * Does not own: core implementation, ISA semantics, buses,
 * CPU state, file loading, CLI parsing, timing, observers,
 * diagnostics, allocation, logging, or environment.
 */

#include "port_peer.h"

#include <stddef.h>
#include <stdint.h>

ti_c54x_port_peer_status
ti_c54x_port_peer_init(ti_c54x_port_peer_state *peer)
{
    size_t i;

    if (peer == NULL) {
        return TI_C54X_PORT_PEER_INVALID_ARGUMENT;
    }
    for (i = 0U; i < (size_t)TI_C54X_PORT_PEER_JOURNAL_WORDS; i++) {
        peer->journal[i].port = UINT16_C(0);
        peer->journal[i].value = UINT16_C(0);
    }
    peer->writes = UINT32_C(0);
    peer->reads = UINT32_C(0);
    peer->journaled = UINT32_C(0);
    peer->overflow = false;
    peer->armed = true;
    return TI_C54X_PORT_PEER_OK;
}

ti_c54x_port_peer_status
ti_c54x_port_peer_notify_write(ti_c54x_port_peer_state *peer,
                               uint16_t port, uint16_t value)
{
    if (peer == NULL || !peer->armed) {
        return TI_C54X_PORT_PEER_INVALID_ARGUMENT;
    }
    peer->writes++;
    if (peer->journaled >=
        (uint32_t)TI_C54X_PORT_PEER_JOURNAL_WORDS) {
        peer->overflow = true;
        return TI_C54X_PORT_PEER_OK;
    }
    peer->journal[peer->journaled].port = port;
    peer->journal[peer->journaled].value = value;
    peer->journaled++;
    return TI_C54X_PORT_PEER_OK;
}

ti_c54x_port_peer_status
ti_c54x_port_peer_notify_read(ti_c54x_port_peer_state *peer,
                              uint16_t port, uint16_t *value_out)
{
    uint32_t i;

    if (peer == NULL || value_out == NULL) {
        return TI_C54X_PORT_PEER_INVALID_ARGUMENT;
    }
    if (!peer->armed) {
        return TI_C54X_PORT_PEER_INVALID_ARGUMENT;
    }
    peer->reads++;
    *value_out = UINT16_C(0);
    /* Sticky: the journal holds every served write in order,
     * so the last match is the port's current value. */
    for (i = UINT32_C(0); i < peer->journaled; i++) {
        if (peer->journal[i].port == port) {
            *value_out = peer->journal[i].value;
        }
    }
    return TI_C54X_PORT_PEER_OK;
}
