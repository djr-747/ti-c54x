# Test ownership: tests/unit/isa/ops/store owns pure STL memory effects.
$(eval $(call ti_c54x_add_test,stl_smem_ops,isa/ops/store,test_stl_smem.c))
# P3 slice 10 RED contract: STH Smem effects.
$(eval $(call ti_c54x_add_test,sth_smem_ops,isa/ops/store,test_sth_smem.c))
# P3 slice 11 RED contract: STH-by-ASM Smem effects.
$(eval $(call ti_c54x_add_test,sth_asm_smem_ops,isa/ops/store,test_sth_asm_smem.c))
# P3 slice 12 RED contract: STH-by-SHFT Xmem effects.
$(eval $(call ti_c54x_add_test,sth_shft_xmem_ops,isa/ops/store,test_sth_shft_xmem.c))
# P3 slice 13 RED contract: STH-by-SHIFT direct effects.
$(eval $(call ti_c54x_add_test,sth_shift_direct_ops,isa/ops/store,test_sth_shift_direct.c))
# P3 slice 13 RED contract: STH-by-SHIFT MOD effects.
$(eval $(call ti_c54x_add_test,sth_shift_smem_mod_ops,isa/ops/store,test_sth_shift_smem_mod.c))
# P3-S18 lane 2b RED contract: STH-by-SHIFT MOD0-MOD3 effects.
$(eval $(call ti_c54x_add_test,sth_shift_smem_mod0_3_ops,isa/ops/store,test_sth_shift_smem_mod0_3.c))
$(eval $(call ti_c54x_add_test,st_immediate_smem_ops,isa/ops/store,test_st_immediate_smem.c))
# P3-S7 RED contract: STM direct store effects.
$(eval $(call ti_c54x_add_test,stm_ops,isa/ops/store,test_stm.c))
$(eval $(call ti_c54x_add_test,st_t_smem_ops,isa/ops/store,test_st_t_smem.c))
$(eval $(call ti_c54x_add_test,st_t_smem_direct_ops,isa/ops/store,test_st_t_smem_direct.c))
# S36 RED contract: ST #lk Smem store effects.
$(eval $(call ti_c54x_add_test,st_lk_smem_ops,isa/ops/store,test_st_lk_smem.c))
$(eval $(call ti_c54x_add_test,standard_indirect_store_modes,isa/ops/store,test_standard_indirect_store_modes.c))
$(eval $(call ti_c54x_add_test,indexed_standard_indirect_store_effects,isa/ops/store,test_indexed_standard_indirect_store_effects.c))
$(eval $(call ti_c54x_add_test,circular_standard_indirect_store_effects,isa/ops/store,test_circular_standard_indirect_store_effects.c))
$(eval $(call ti_c54x_add_test,reverse_carry_standard_indirect_store_effects,isa/ops/store,test_reverse_carry_standard_indirect_store_effects.c))
$(eval $(call ti_c54x_add_test,dst_long32_mod13_ops,isa/ops/store,test_dst_long32_mod13.c))
