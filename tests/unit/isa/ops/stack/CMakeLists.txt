# Test ownership: tests/unit/isa/ops/stack owns stack-pointer semantic effects.
ti_c54x_add_unit_test(frame_k_ops test_frame_k.c)
# P3-S105 RED contract: TRAP K effects.
ti_c54x_add_unit_test(trap_ops test_trap.c)
# P3-S110 RED contract: INTR K effects.
ti_c54x_add_unit_test(intr_ops test_intr.c)
ti_c54x_add_unit_test(pshd_popd_direct_ops test_pshd_popd_direct.c)
