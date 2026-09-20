### P2-W27: AND/OR/XOR register-only logical accumulator vertical slice (implemented; local verification green)

State: implemented; targeted proof and full local and remote gates are green.
Keep the immutable `(planned)` heading as the evidence anchor. The clean
catalog-thirteen performance record is captured at `ef87c94`; it shows no
confirmed regression. This is a hosted implementation observation only, not a
DSP timing claim.
P2-W27 adds three register-only one-word logical accumulator families without
introducing architectural state, input resources, effect kinds, sequencing
seams, public APIs, memory, RPT/IRQ, parallel execution, or timing claims.

The normative forms are TI `SPRU172C` §4 AND (printed p. 4-11), OR (p. 4-123),
and XOR (p. 4-201):

- `AND_ACC_SHIFT`/`and_acc_shift`: value `0xF080`, mask `0xFCE0`, opcode
  `1111 00SD 100S HIFT`, one word, 128 encoded first words.
- `OR_ACC_SHIFT`/`or_acc_shift`: value `0xF0A0`, mask `0xFCE0`, opcode
  `1111 00SD 101S HIFT`, one word, 128 encoded first words.
- `XOR_ACC_SHIFT`/`xor_acc_shift`: value `0xF0C0`, mask `0xFCE0`, opcode
  `1111 00SD 110S HIFT`, one word, 128 encoded first words.

The fixed bits 7:5 are AND=`100`, OR=`101`, XOR=`110`, disjoint from the
existing SFTL=`111` row under the same mask. An omitted destination is only the
assembler alias `D=S`, never an additional catalog row. The post-slice catalog
expectation is 13 rows and 1077 populated first-word encodings; those entries
are encodings rather than instruction count, and reset-runnable coverage stays
four of 13.

`SPRU131G` §2.3.1/§2.3.3 (printed pp. 2-8–2-9) supplies the 40-bit Boolean
ALU, accumulator, and barrel-shifter foundation. Decode reuses
`TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_SOURCE_DESTINATION_SHIFT5`: S is bit 9, D
is bit 8, and raw shift bits 4:0 sign-extend to `q=-16..15`. The existing
`ACCUMULATORS` input and generic accumulator-write effect remain the only
resource/effect path.

Pure handlers in `src/isa/ops/logical/` operate on the pre-instruction
destination and shifted source. For `q<0`, logically right-shift the full
40-bit source by `-q`; for `q=0`, use identity; for `q>0`, left-shift the
full-40 source and mask to 40 bits. Combine with `&`, `|`, or `^`. Guard bits
participate, the result is canonical 40-bit with no sign extension, and
`SPRU131G` §4.2.3 (p. 4-12) requires `C_PRESERVE` with all other status
unchanged. Both participating accumulators must be known/canonical; if `S==D`,
unknown data in the unselected accumulator is allowed. Invalid inputs and
effects reject transactionally at the PC boundary without mutation.

Ownership mirrors the repository hierarchy: `src/isa/catalog/` owns rows and
resource metadata; `src/isa/decode/` owns the generic layout/extraction;
`src/isa/ops/logical/` owns pure handlers; `src/cpu/input/` and
`src/sequencer/accumulator/` remain owners of input collection and generic
effects; and `src/cpu/` only orchestrates generic retirement. Phase 1 RED tests
are independently owned at
`tests/unit/isa/catalog/test_logical_accumulator_metadata.c`,
`tests/unit/isa/decode/test_logical_accumulator_decode.c`,
`tests/unit/isa/ops/logical/test_logical_accumulator.c`, and
`tests/unit/cpu/test_cpu_logical_accumulator.c`. Phase 2 production starts only
after those contracts fail for the missing family rows/handlers; the
orchestrator serializes catalog, decode, generated, and shared changes. Any
architecture gap is a hard stop/report under the fanout policy above.

Targeted proof is green across all 384 family encodings and all 65,536 first words,
and exercises q boundaries `-16,-1,0,1,15`, guard-bearing full-40 vectors, every
source/destination combination, fixed-bit collision rejection, the S==D
unknownness rule, canonical/unknown transactional rejection, C and all-status
preservation, exactly one attempt/retirement with PC+1, and PC `0xffff`
rollback. Repeatability remains `UNRESOLVED`: Table 2-28 (printed pp. 2-19–
2-21) does not positively authorize these register forms, and no summary cycle
field is timing evidence. GNU binutils material, if consulted, is
nonnormative corroboration only. The concrete outcome is 13 catalog rows, 1077
populated first words, 384 new logical encodings, and four of 13 rows in the
reset-runnable workload. Existing layout, input, effect, and CPU seams only were
used; `cpu.c`, public API, architectural state, memory, RPT/IRQ, and timing are
unchanged. Canonical 40-bit carrier validation remains distinct from raw input
knownness, and C plus all other status fields are preserved.
