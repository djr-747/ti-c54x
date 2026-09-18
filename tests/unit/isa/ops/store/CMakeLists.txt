# Test ownership: tests/unit/isa/ops/store owns pure STL memory effects.
ti_c54x_add_unit_test(stl_smem_ops test_stl_smem.c)
# P3 slice 10 RED contract: STH Smem effects.
ti_c54x_add_unit_test(sth_smem_ops test_sth_smem.c)
# P3 slice 11 RED contract: STH-by-ASM Smem effects.
ti_c54x_add_unit_test(sth_asm_smem_ops test_sth_asm_smem.c)
# P3 slice 12 RED contract: STH-by-SHFT Xmem effects.
ti_c54x_add_unit_test(sth_shft_xmem_ops test_sth_shft_xmem.c)
# P3 slice 13 RED contract: STH-by-SHIFT direct effects.
ti_c54x_add_unit_test(sth_shift_direct_ops test_sth_shift_direct.c)
# P3 slice 13 RED contract: STH-by-SHIFT MOD effects.
ti_c54x_add_unit_test(sth_shift_smem_mod_ops test_sth_shift_smem_mod.c)
# P3-S18 lane 2b RED contract: STH-by-SHIFT MOD0-MOD3 effects.
ti_c54x_add_unit_test(sth_shift_smem_mod0_3_ops test_sth_shift_smem_mod0_3.c)
ti_c54x_add_unit_test(st_immediate_smem_ops test_st_immediate_smem.c)
# P3-S7 RED contract: STM direct store effects.
ti_c54x_add_unit_test(stm_ops test_stm.c)
ti_c54x_add_unit_test(st_t_smem_ops test_st_t_smem.c)
ti_c54x_add_unit_test(st_t_smem_direct_ops test_st_t_smem_direct.c)
ti_c54x_add_unit_test(stlm_smem_direct_ops test_stlm_smem_direct.c)
ti_c54x_add_unit_test(stl_smem_direct_ops test_stl_smem_direct.c)
ti_c54x_add_unit_test(stl_asm_smem_direct_ops test_stl_asm_smem_direct.c)
ti_c54x_add_unit_test(st_trn_smem_direct_ops test_st_trn_smem_direct.c)
# S36 RED contract: ST #lk Smem store effects.
ti_c54x_add_unit_test(st_lk_smem_ops test_st_lk_smem.c)
ti_c54x_add_unit_test(standard_indirect_store_modes
                      test_standard_indirect_store_modes.c)
ti_c54x_add_unit_test(indexed_standard_indirect_store_effects
                      test_indexed_standard_indirect_store_effects.c)
ti_c54x_add_unit_test(circular_standard_indirect_store_effects
                      test_circular_standard_indirect_store_effects.c)
ti_c54x_add_unit_test(reverse_carry_standard_indirect_store_effects
                      test_reverse_carry_standard_indirect_store_effects.c)
ti_c54x_add_unit_test(dst_long32_mod13_ops test_dst_long32_mod13.c)
