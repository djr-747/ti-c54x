# Test ownership: tests/unit/isa/ops/arithmetic owns ADD/SUB accumulator proofs.
ti_c54x_add_unit_test(add_sub_acc_shift test_add_sub_acc_shift.c)
ti_c54x_add_unit_test(add_sub_dual test_add_sub_dual.c)
# W51C RED contract: non-MOD0/MOD0 dual ADD/SUB checked/trusted semantics.
ti_c54x_add_unit_test(add_sub_dual_modes test_add_sub_dual_modes.c)
