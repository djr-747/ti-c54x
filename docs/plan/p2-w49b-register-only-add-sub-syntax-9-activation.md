### P2-W49B: register-only ADD/SUB syntax-9 activation (planned)

This immutable `(planned)` anchor records the bounded arithmetic vertical slice;
the status is now implemented. The bounded
forms are `FORM-T2-01-09` ADD `src [, SHIFT] [, dst]` and `FORM-T2-02-09` SUB
`src [, SHIFT] [, dst]`, both one program word and class 1. Their catalog rows
are `ADD_ACC_SHIFT` at value `0xF400`, mask `0xFCE0`, and `SUB_ACC_SHIFT` at
value `0xF420`, mask `0xFCE0`. S bit 9 selects source A/B, D bit 8 selects
destination A/B, and the existing
`ACCUMULATOR_SOURCE_DESTINATION_SHIFT5` layout exposes raw SHIFT bits 4:0 as
the signed range -16 through 15. Syntax-10 ASM forms, all memory forms, and
sign-suppressed variants remain outside this piece.

SPRU172C §§ADD and SUB (printed pp. 4-4–4-7 and 4-187–4-190) define the
execution expressions `(dst or src) + (src << SHIFT) -> dst` and
`dst - (src << SHIFT) -> dst`; the shifter runs before the ALU. SPRU131G
§4.4 (printed pp. 4-17–4-18) supplies the 40-bit accumulator shifter, signed
immediate count, and SXM sign/zero fill. W49A owns the checked modulo-40
result, bit-32 carry/no-borrow, signed-32 overflow direction, OVM/OVA/OVB
access, and atomic sticky-overflow effect. W49B composes these facts into
read-before-write accumulator semantics: source/destination aliasing uses the
original source value, OVM clear retains the modulo-40 result, OVM set clamps
positive/negative overflow to `00 7FFF FFFFh`/`FF 8000 0000h`, and no overflow
preserves the destination's sticky OVA/OVB flag.

The semantic requirement is deliberately split from decode, trusted binding,
CPU retirement, inventory projection, and performance. Focused executable tests
cover
all 128 S/D/SHIFT encodings, fixed-bit near misses, SHIFT -16/-1/0/+1/+15,
SXM sign and zero fill, source/destination aliasing, bit-32 carry/no-borrow
independent of guard bits, signed-32 overflow with OVM clear/set, sticky
destination OVA/OVB preservation/set, malformed decoded inputs, unknown required
inputs, and PC+1 atomic retirement. Repeatability remains `UNRESOLVED`: Table
2-28 does not positively authorize omitted forms, so this slice must not
activate runtime RPT.

The acceptance axis includes an observer-off public-CPU benchmark against the
clean W49A baseline. The implementation preserves the trusted packet and outer
effect-carrier sizes, remains allocation-free, adds no opcode-specific CPU
branch, and makes no TI timing, device, profile, memory, interrupt, or public-ABI
claim. Focused C/Make/CMake and Python/generator/inventory checks are GREEN. A
six-pair interleaved comparison against W49A found fixed-scenario paired median
deltas of +3.51% NOP, +4.79% LD-DP, +0.19% LD-ASM, and +0.70% LD-ARP amid wider
host spread, with matching semantic hashes and retirement counts. The dedicated
observer-off ADD/SUB benchmark is also GREEN at 70.569/68.553 ns per instruction
on this host with semantic hash `dd671e98fa2ca1dd`. The realized catalog is 102
rows with ID_COUNT 103,
2,967 populated first-word lookup entries, 102 inventory bindings, family
coverage 42/205 (20.488%), and strict complete-form coverage 27/205
(13.171%). The implementation commit is recorded in the development ledger as
`5cbacda4076a43616be014ba0f528d7e95228e73`. The implemented registry records are
`REQ-COMMON-P2-W49B-ADD-SUB-ACC-SHIFT5-DECODE`,
`REQ-COMMON-P2-W49B-ADD-SUB-ACC-SHIFT5-EFFECTS`,
`REQ-PROJECT-P2-W49B-ADD-SUB-ACC-SHIFT5-TRUSTED-BINDING`,
`REQ-PROJECT-P2-W49B-ADD-SUB-ACC-SHIFT5-CPU-RETIREMENT`,
`REQ-PROJECT-P2-W49B-ADD-SUB-ACC-SHIFT5-INVENTORY`, and
`REQ-PROJECT-P2-W49B-ADD-SUB-ACC-SHIFT5-HOT-PATH-PERFORMANCE`; the separate
`REQ-COMMON-P2-W49B-ADD-SUB-ACC-SHIFT5-REPEATABILITY` remains unresolved.
