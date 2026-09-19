### P3-S58: MVMD MMR-to-Smem move slice (planned)

`MVMD MMR, dmad` (FORM-T2-26-06) moves one memory-mapped
register into a DATA word. It is the `MVDM` mirror and the
second 5110 census MMR move (139 occurrences).

Encoding (TI SPRU172C p. 4-115, vendored text): two words; word
0 pins `0x7300` with the 5-bit MMR source at bits 4:0 (bits
7:5 zero, same `I`-bit convention as `MVDM` — `I=1` words stay
undecoded), word 1 the 16-bit absolute DATA address. No
indirect form exists (syntax is absolute-only, Class 19A only).

Root design (decided during implementation): the sidecar
moves from S56's effect-level pair into the absolute DATA spec
arm (`mmr_sidecar_kind` + `mmr_sidecar_number`, repurposing the
arm's reserved half — both effect ABIs unchanged), with kinds
NONE/STORE_WITH_READ/LOAD_WITH_WRITE. `MVMD` stages an absolute
DATA WRITE with a LOAD leg and a zero staged value; on first
issue the resume path snapshots the MMR through the router
(decline/unknown reject transactionally before any bus
traffic), patches the prepared DATA WRITE request, and issues
it through the ordinary endpoint flow, so neither resolution
nor commit grows an endpoint leg. The leaf stays input-free
(no snapshot/input-resource plumbing); values never
re-validate live (single-writer snapshot, stable across WAIT
retries). Absolute address planning, the ABSOLUTE source, and
the shape modules are reused; the MMR5_ABSOLUTE16 layout is
direction-neutral and shared.

Out of scope: MVMM/MVDK/MVKD/MVPD/MVDP/READA/WRITA (own
slices in census order), peripheral MMR modeling behind the
Smem accesses (explicit faults stay), timing.
