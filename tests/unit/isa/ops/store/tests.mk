# Test ownership: tests/unit/isa/ops/store owns pure STL memory effects.
$(eval $(call ti_c54x_add_test,stl_smem_ops,isa/ops/store,test_stl_smem.c))
