# Test ownership: tests/unit/isa/catalog owns immutable catalog metadata
# identity/range and copied decode propagation proofs.
ti_c54x_add_unit_test(opcode_metadata test_opcode_metadata.c)
# P3-S18 lane 2b: qualified-slot match/mismatch vectors.
ti_c54x_add_unit_test(qualified_slots test_qualified_slots.c)
# P3-S18 lane 2b RED contract: STH MOD0-MOD3 qualified rows.
ti_c54x_add_unit_test(sth_shift_smem_mod0_3_metadata test_sth_shift_smem_mod0_3_metadata.c)
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
# W70 RED contract: BANZ row metadata.
ti_c54x_add_unit_test(banz_metadata test_banz_metadata.c)
# W71 RED contract: BANZD row metadata.
ti_c54x_add_unit_test(banzd_metadata test_banzd_metadata.c)
# W72 RED contract: CCD row metadata.
ti_c54x_add_unit_test(ccd_metadata test_ccd_metadata.c)
# W73 RED contract: CC row metadata.
ti_c54x_add_unit_test(cc_metadata test_cc_metadata.c)
ti_c54x_add_unit_test(ldr_smem_metadata test_ldr_smem_metadata.c)
ti_c54x_add_unit_test(ltd_smem_metadata test_ltd_smem_metadata.c)
ti_c54x_add_unit_test(delay_smem_metadata test_delay_smem_metadata.c)
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
# W74 RED contract: RETE row metadata.
ti_c54x_add_unit_test(rete_metadata
                      test_rete_metadata.c)
# W75 RED contract: RETED row metadata.
ti_c54x_add_unit_test(reted_metadata
                      test_reted_metadata.c)
# W76 RED contract: RETF catalog row.
ti_c54x_add_unit_test(retf_metadata
                      test_retf_metadata.c)
# W77 RED contract: RC catalog row.
ti_c54x_add_unit_test(rc_metadata
                      test_rc_metadata.c)
# W78 RED contract: RCD catalog row.
ti_c54x_add_unit_test(rcd_metadata
                      test_rcd_metadata.c)
# P3-S7 RED contract: STM catalog row.
ti_c54x_add_unit_test(stm_metadata
                      test_stm_metadata.c)
# P3-S2 RED contract: RPT immediate rows.
ti_c54x_add_unit_test(rpt_k_metadata
                      test_rpt_k_metadata.c)
ti_c54x_add_unit_test(rpt_lk_metadata
                      test_rpt_lk_metadata.c)
# P3-S3 RED contract: MAC Smem row.
ti_c54x_add_unit_test(mac_smem_metadata
                      test_mac_smem_metadata.c)
ti_c54x_add_unit_test(mpy_smem_metadata
                      test_mpy_smem_metadata.c)
ti_c54x_add_unit_test(mpya_smem_metadata
                      test_mpya_smem_metadata.c)
# P3-S33 RED contract: MPY/MAC Smem,#lk syntax rows.
ti_c54x_add_unit_test(mpy_smem_lk_metadata
                      test_mpy_smem_lk_metadata.c)
ti_c54x_add_unit_test(mac_smem_lk_metadata
                      test_mac_smem_lk_metadata.c)
ti_c54x_add_unit_test(mas_smem_metadata
                      test_mas_smem_metadata.c)
# P3-S3 RED contract: MPYA dst register-only row.
ti_c54x_add_unit_test(mpya_dst_metadata
                      test_mpya_dst_metadata.c)
# P3-S3 RED contract: SQUR A,dst register-only row.
ti_c54x_add_unit_test(squr_adst_metadata
                      test_squr_adst_metadata.c)
# P3-S3 RED contract: SQUR Smem square row.
ti_c54x_add_unit_test(squr_smem_metadata
                      test_squr_smem_metadata.c)
# P3-S3 RED contract: SQURA/SQURS accumulate squares.
ti_c54x_add_unit_test(squr_acc_metadata
                      test_squr_acc_metadata.c)
# P3-S3 RED contract: MPYU unsigned multiply.
ti_c54x_add_unit_test(mpyu_smem_metadata
                      test_mpyu_smem_metadata.c)
# P3-S3 RED contract: MACA T,src register-only row.
ti_c54x_add_unit_test(maca_t_metadata
                      test_maca_t_metadata.c)
# P3-S3 RED contract: MACA Smem accumulate-A row.
ti_c54x_add_unit_test(maca_smem_metadata
                      test_maca_smem_metadata.c)
# P3-S3 RED contract: MACP Smem,Pmad row.
ti_c54x_add_unit_test(macp_smem_metadata
                      test_macp_smem_metadata.c)
# P3-S3 RED contract: MACD Smem,Pmad row.
ti_c54x_add_unit_test(macd_smem_metadata
                      test_macd_smem_metadata.c)
# W67B RED contract: RETD row metadata.
ti_c54x_add_unit_test(retd_metadata
                      test_retd_metadata.c)
# W67C RED contract: CALLD row metadata.
ti_c54x_add_unit_test(calld_metadata
                      test_calld_metadata.c)
# W7A RED contract: RND/SAT accumulator rows.
ti_c54x_add_unit_test(rnd_acc_metadata
                      test_rnd_acc_metadata.c)
ti_c54x_add_unit_test(sat_acc_metadata
                      test_sat_acc_metadata.c)
# S36 RED contract: BITF/CMPM/BITT/ST-lk rows.
ti_c54x_add_unit_test(bitf_smem_lk_metadata
                      test_bitf_smem_lk_metadata.c)
ti_c54x_add_unit_test(cmpm_smem_lk_metadata
                      test_cmpm_smem_lk_metadata.c)
ti_c54x_add_unit_test(bitt_smem_metadata
                      test_bitt_smem_metadata.c)
ti_c54x_add_unit_test(st_lk_smem_metadata
                      test_st_lk_smem_metadata.c)
ti_c54x_add_unit_test(st_t_smem_direct_metadata
                      test_st_t_smem_direct_metadata.c)
ti_c54x_add_unit_test(stlm_smem_direct_metadata
                      test_stlm_smem_direct_metadata.c)
ti_c54x_add_unit_test(ldm_smem_direct_metadata
                      test_ldm_smem_direct_metadata.c)
ti_c54x_add_unit_test(stl_smem_direct_metadata
                      test_stl_smem_direct_metadata.c)
ti_c54x_add_unit_test(stl_asm_smem_direct_metadata
                      test_stl_asm_smem_direct_metadata.c)
ti_c54x_add_unit_test(st_trn_smem_direct_metadata
                      test_st_trn_smem_direct_metadata.c)
# W7 RED contract: ABS/NEG/NORM accumulator rows.
ti_c54x_add_unit_test(abs_acc_metadata
                      test_abs_acc_metadata.c)
ti_c54x_add_unit_test(neg_acc_metadata
                      test_neg_acc_metadata.c)
ti_c54x_add_unit_test(norm_acc_metadata
                      test_norm_acc_metadata.c)
ti_c54x_add_unit_test(sfta_acc_metadata
                      test_sfta_acc_metadata.c)
ti_c54x_add_unit_test(addc_smem_metadata
                      test_addc_smem_metadata.c)
ti_c54x_add_unit_test(adds_smem_metadata
                      test_adds_smem_metadata.c)
ti_c54x_add_unit_test(subc_smem_metadata
                      test_subc_smem_metadata.c)
