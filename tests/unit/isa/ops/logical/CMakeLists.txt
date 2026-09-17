# Test ownership: tests/unit/isa/ops/logical owns AND/OR/XOR accumulator
# semantic-handler and generated-dispatch proofs.
ti_c54x_add_unit_test(logical_accumulator test_logical_accumulator.c)
# P3-S15 RED contract: AND Smem.
ti_c54x_add_unit_test(and_smem_ops test_and_smem.c)
# P3-S16 RED contract: OR Smem.
ti_c54x_add_unit_test(or_smem_ops test_or_smem.c)
# P3-S17 RED contract: XOR Smem.
ti_c54x_add_unit_test(xor_smem_ops test_xor_smem.c)
add_subdirectory(immediate)
