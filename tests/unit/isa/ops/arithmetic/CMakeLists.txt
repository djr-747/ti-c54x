# Test ownership: tests/unit/isa/ops/arithmetic owns ADD/SUB accumulator proofs.
ti_c54x_add_unit_test(add_sub_acc_shift test_add_sub_acc_shift.c)
ti_c54x_add_unit_test(add_sub_dual test_add_sub_dual.c)
# W51C RED contract: non-MOD0/MOD0 dual ADD/SUB checked/trusted semantics.
ti_c54x_add_unit_test(add_sub_dual_modes test_add_sub_dual_modes.c)
# W52C RED contract: MOD3-involving dual ADD/SUB checked/trusted semantics.
ti_c54x_add_unit_test(add_sub_dual_mod3 test_add_sub_dual_mod3.c)
# P3-S3z RED contract: ADD Smem syntaxes 1-3.
ti_c54x_add_unit_test(add_smem_ops test_add_smem_ops.c)
# P3-S14 RED contract: SUB Smem syntaxes 1-3.
ti_c54x_add_unit_test(sub_smem_ops test_sub_smem.c)
