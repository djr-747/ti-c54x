# Test ownership: tests/unit/isa/ops/auxiliary owns auxiliary-register
# semantic effects.
# P3-S77 RED contract: MAR Smem postdecrement leaf.
$(eval $(call ti_c54x_add_test,mar_smem_ops,isa/ops/auxiliary,test_mar_smem.c))
