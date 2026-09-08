# Test ownership: tests/unit/isa/ops/store owns pure STL memory effects.
$(eval $(call ti_c54x_add_test,stl_smem_ops,isa/ops/store,test_stl_smem.c))
$(eval $(call ti_c54x_add_test,st_immediate_smem_ops,isa/ops/store,test_st_immediate_smem.c))
$(eval $(call ti_c54x_add_test,st_t_smem_ops,isa/ops/store,test_st_t_smem.c))
$(eval $(call ti_c54x_add_test,standard_indirect_store_modes,isa/ops/store,test_standard_indirect_store_modes.c))
$(eval $(call ti_c54x_add_test,indexed_standard_indirect_store_effects,isa/ops/store,test_indexed_standard_indirect_store_effects.c))
$(eval $(call ti_c54x_add_test,circular_standard_indirect_store_effects,isa/ops/store,test_circular_standard_indirect_store_effects.c))
