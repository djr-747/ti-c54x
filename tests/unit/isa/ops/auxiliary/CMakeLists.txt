# Test ownership: tests/unit/isa/ops/auxiliary owns auxiliary-register
# semantic effects.
# P3-S77 RED contract: MAR Smem postdecrement leaf.
ti_c54x_add_unit_test(mar_smem_ops test_mar_smem.c)
