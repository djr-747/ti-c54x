/*
 * Module contract
 *
 * Owns: the hosted port peer's public identity: a fixed
 * 32-entry IO write journal (port plus value), served
 * read/write counts, and the peer state and status
 * vocabulary. Reads are sticky (last value the DSP wrote
 * to that port, else zero): a staged simplification for
 * boot advance, documented, never hardware truth.
 * Borrowed storage is used for one call only under
 * caller-owned lifetimes; the peer allocates nothing
 * and performs no I/O.
 * Does not own: core implementation, ISA semantics, buses,
 * CPU state, file loading, CLI parsing, timing, observers,
 * diagnostics, allocation, logging, or environment.
 */

#ifndef TI_C54X_HOSTED_PORT_PEER_H
#define TI_C54X_HOSTED_PORT_PEER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    /* Journal capacity (port-plus-value entries). Boot writes a
     * handful of ports; overflow still counts and flags. */
    TI_C54X_PORT_PEER_JOURNAL_WORDS = 32
};

typedef enum ti_c54x_port_peer_status {
    TI_C54X_PORT_PEER_OK = 0,
    TI_C54X_PORT_PEER_INVALID_ARGUMENT = 1
} ti_c54x_port_peer_status;

typedef struct ti_c54x_port_peer_entry {
    uint16_t port;
    uint16_t value;
} ti_c54x_port_peer_entry;

typedef struct ti_c54x_port_peer_state {
    ti_c54x_port_peer_entry journal[TI_C54X_PORT_PEER_JOURNAL_WORDS];
    uint32_t writes;
    uint32_t reads;
    uint32_t journaled;
    bool overflow;
    bool armed;
} ti_c54x_port_peer_state;

/* Zero the counters, empty the journal, and arm the peer.
 * A null peer rejects. */
ti_c54x_port_peer_status
ti_c54x_port_peer_init(ti_c54x_port_peer_state *peer);

/* Post one IO write: count it, journal it while capacity
 * remains, flag overflow past capacity. A null or
 * disarmed peer rejects and counts nothing. */
ti_c54x_port_peer_status
ti_c54x_port_peer_notify_write(ti_c54x_port_peer_state *peer,
                               uint16_t port, uint16_t value);

/* Serve one IO read: the last value written to the port,
 * else zero. A null peer or null value slot rejects and
 * counts nothing. */
ti_c54x_port_peer_status
ti_c54x_port_peer_notify_read(ti_c54x_port_peer_state *peer,
                              uint16_t port, uint16_t *value_out);

#ifdef __cplusplus
}
#endif

#endif /* TI_C54X_HOSTED_PORT_PEER_H */
