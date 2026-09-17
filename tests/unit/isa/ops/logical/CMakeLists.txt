# Test ownership: tests/unit/isa/ops/logical owns AND/OR/XOR accumulator
# semantic-handler and generated-dispatch proofs.
ti_c54x_add_unit_test(logical_accumulator test_logical_accumulator.c)
# P3-S15 RED contract: AND Smem.
ti_c54x_add_unit_test(and_smem_ops test_and_smem.c)
add_subdirectory(immediate)
