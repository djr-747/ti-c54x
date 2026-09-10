# Test ownership: tests/unit/isa/catalog owns immutable catalog metadata
# identity/range and copied decode propagation proofs.
ti_c54x_add_unit_test(opcode_metadata test_opcode_metadata.c)
ti_c54x_add_unit_test(rol_ror_metadata test_rol_ror_metadata.c)
ti_c54x_add_unit_test(sftl_metadata test_sftl_metadata.c)
ti_c54x_add_unit_test(logical_accumulator_metadata test_logical_accumulator_metadata.c)
ti_c54x_add_unit_test(roltc_metadata test_roltc_metadata.c)
ti_c54x_add_unit_test(sftc_metadata test_sftc_metadata.c)
ti_c54x_add_unit_test(exp_metadata test_exp_metadata.c)
ti_c54x_add_unit_test(logical_immediate_metadata test_logical_immediate_metadata.c)
ti_c54x_add_unit_test(fixed_mod2_memory_metadata test_fixed_mod2_memory_metadata.c)
ti_c54x_add_unit_test(ssbx_rsbx_metadata test_ssbx_rsbx_metadata.c)
ti_c54x_add_unit_test(cmpr_metadata test_cmpr_metadata.c)
ti_c54x_add_unit_test(ldu_smem_metadata test_ldu_smem_metadata.c)
ti_c54x_add_unit_test(ld_t_smem_metadata test_ld_t_smem_metadata.c)
ti_c54x_add_unit_test(ld_status_smem_metadata test_ld_status_smem_metadata.c)
ti_c54x_add_unit_test(ld_smem_shift16_metadata test_ld_smem_shift16_metadata.c)
ti_c54x_add_unit_test(ld_smem_shift_metadata test_ld_smem_shift_metadata.c)
ti_c54x_add_unit_test(ld_src_shift_metadata test_ld_src_shift_metadata.c)
ti_c54x_add_unit_test(ld_k_metadata test_ld_k_metadata.c)
ti_c54x_add_unit_test(ld_lk16_metadata test_ld_lk16_metadata.c)
ti_c54x_add_unit_test(ld_lk_shift_metadata test_ld_lk_shift_metadata.c)
ti_c54x_add_unit_test(b_pmad_metadata test_b_pmad_metadata.c)
# W68 RED contract: BD row metadata.
ti_c54x_add_unit_test(bd_metadata test_bd_metadata.c)
ti_c54x_add_unit_test(bc_pmad_metadata test_bc_pmad_metadata.c)
# W69 RED contract: BCD row metadata.
ti_c54x_add_unit_test(bcd_metadata test_bcd_metadata.c)
ti_c54x_add_unit_test(ldr_smem_metadata test_ldr_smem_metadata.c)
ti_c54x_add_unit_test(ld_xmem_shift_metadata test_ld_xmem_shift_metadata.c)
ti_c54x_add_unit_test(ld_src_asm_metadata test_ld_src_asm_metadata.c)
ti_c54x_add_unit_test(ld_smem_ts_metadata test_ld_smem_ts_metadata.c)
ti_c54x_add_unit_test(st_immediate_smem_metadata test_st_immediate_smem_metadata.c)
ti_c54x_add_unit_test(st_t_smem_metadata test_st_t_smem_metadata.c)
ti_c54x_add_unit_test(standard_indirect_memory_metadata
                      test_standard_indirect_memory_metadata.c)
ti_c54x_add_unit_test(indexed_standard_indirect_metadata
                      test_indexed_standard_indirect_metadata.c)
ti_c54x_add_unit_test(circular_standard_indirect_metadata
                      test_circular_standard_indirect_metadata.c)
ti_c54x_add_unit_test(reverse_carry_standard_indirect_metadata
                      test_reverse_carry_standard_indirect_metadata.c)
ti_c54x_add_unit_test(atomic_logical_memory_metadata
                      test_atomic_logical_memory_metadata.c)
ti_c54x_add_unit_test(dld_dst_long32_metadata
                      test_dld_dst_long32_metadata.c)
ti_c54x_add_unit_test(dld_repeatability_contract
                      test_dld_repeatability_contract.c)
ti_c54x_add_unit_test(add_sub_acc_shift_metadata
                      test_add_sub_acc_shift_metadata.c)
ti_c54x_add_unit_test(add_sub_dual_metadata
                      test_add_sub_dual_metadata.c)
# W51C RED contract: non-MOD0/MOD0 dual ADD/SUB row metadata.
ti_c54x_add_unit_test(add_sub_dual_modes_metadata
                      test_add_sub_dual_modes_metadata.c)
# W52C RED contract: MOD3-involving dual ADD/SUB row metadata.
ti_c54x_add_unit_test(add_sub_dual_mod3_metadata
                      test_add_sub_dual_mod3_metadata.c)
# W65B RED contract: CALL pmad row metadata.
ti_c54x_add_unit_test(call_pmad_metadata
                      test_call_pmad_metadata.c)
# W66B RED contract: RET row metadata.
ti_c54x_add_unit_test(ret_metadata
                      test_ret_metadata.c)
# W67B RED contract: RETD row metadata.
ti_c54x_add_unit_test(retd_metadata
                      test_retd_metadata.c)
# W67C RED contract: CALLD row metadata.
ti_c54x_add_unit_test(calld_metadata
                      test_calld_metadata.c)
