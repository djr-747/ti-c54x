# Test ownership: tests/unit/isa/decode owns exhaustive first-word decode
# classification and catalog metadata coverage.
ti_c54x_add_unit_test(nop_decode test_nop_decode.c)
ti_c54x_add_unit_test(operand_field test_operand_field.c)
ti_c54x_add_unit_test(instruction_bundle test_instruction_bundle.c)
ti_c54x_add_unit_test(operand_layout_binding test_operand_layout_binding.c)
ti_c54x_add_unit_test(trusted_execution_packet test_trusted_execution_packet.c)
ti_c54x_add_unit_test(ld_status_fields_decode test_ld_status_fields.c)
ti_c54x_add_unit_test(max_min_decode test_max_min_decode.c)
ti_c54x_add_unit_test(cmpl_decode test_cmpl_decode.c)
ti_c54x_add_unit_test(rol_ror_decode test_rol_ror_decode.c)
ti_c54x_add_unit_test(sftl_decode test_sftl_decode.c)
ti_c54x_add_unit_test(logical_accumulator_decode test_logical_accumulator_decode.c)
ti_c54x_add_unit_test(roltc_decode test_roltc_decode.c)
ti_c54x_add_unit_test(sftc_decode test_sftc_decode.c)
ti_c54x_add_unit_test(exp_decode test_exp_decode.c)
ti_c54x_add_unit_test(logical_immediate_decode test_logical_immediate_decode.c)
ti_c54x_add_unit_test(fixed_mod2_memory_decode test_fixed_mod2_memory_decode.c)
ti_c54x_add_unit_test(ssbx_rsbx_decode test_ssbx_rsbx_decode.c)
ti_c54x_add_unit_test(cmpr_decode test_cmpr_decode.c)
ti_c54x_add_unit_test(ldu_smem_decode test_ldu_smem_decode.c)
ti_c54x_add_unit_test(ld_t_smem_decode test_ld_t_smem_decode.c)
ti_c54x_add_unit_test(ld_status_smem_decode test_ld_status_smem_decode.c)
ti_c54x_add_unit_test(ld_smem_shift16_decode test_ld_smem_shift16_decode.c)
ti_c54x_add_unit_test(ld_smem_shift_decode test_ld_smem_shift_decode.c)
ti_c54x_add_unit_test(ld_src_shift_decode test_ld_src_shift_decode.c)
ti_c54x_add_unit_test(ld_k_decode test_ld_k_decode.c)
ti_c54x_add_unit_test(ld_lk16_decode test_ld_lk16_decode.c)
ti_c54x_add_unit_test(ld_lk_shift_decode test_ld_lk_shift_decode.c)
ti_c54x_add_unit_test(b_pmad_decode test_b_pmad_decode.c)
# W68 RED contract: BD decode with near-miss audit.
ti_c54x_add_unit_test(bd_decode test_bd_decode.c)
ti_c54x_add_unit_test(bc_pmad_decode test_bc_pmad_decode.c)
# W69 RED contract: BCD decode with near-miss audit.
ti_c54x_add_unit_test(bcd_decode test_bcd_decode.c)
# W70 RED contract: BANZ decode with near-miss audit.
ti_c54x_add_unit_test(banz_decode test_banz_decode.c)
# W71 RED contract: BANZD decode with near-miss audit.
ti_c54x_add_unit_test(banzd_decode test_banzd_decode.c)
# W72 RED contract: CCD decode with near-miss audit.
ti_c54x_add_unit_test(ccd_decode test_ccd_decode.c)
# W73 RED contract: CC decode with near-miss audit.
ti_c54x_add_unit_test(cc_decode test_cc_decode.c)
ti_c54x_add_unit_test(ldr_smem_decode test_ldr_smem_decode.c)
ti_c54x_add_unit_test(ld_xmem_shift_decode test_ld_xmem_shift_decode.c)
ti_c54x_add_unit_test(ld_src_asm_decode test_ld_src_asm_decode.c)
ti_c54x_add_unit_test(ld_smem_ts_decode test_ld_smem_ts_decode.c)
ti_c54x_add_unit_test(st_immediate_smem_decode test_st_immediate_smem_decode.c)
ti_c54x_add_unit_test(st_t_smem_decode test_st_t_smem_decode.c)
ti_c54x_add_unit_test(standard_indirect_memory_decode
                      test_standard_indirect_memory_decode.c)
ti_c54x_add_unit_test(indexed_standard_indirect_decode
                      test_indexed_standard_indirect_decode.c)
ti_c54x_add_unit_test(circular_standard_indirect_decode
                      test_circular_standard_indirect_decode.c)
ti_c54x_add_unit_test(reverse_carry_standard_indirect_decode
                      test_reverse_carry_standard_indirect_decode.c)
ti_c54x_add_unit_test(atomic_logical_memory_decode
                      test_atomic_logical_memory_decode.c)
ti_c54x_add_unit_test(dld_dst_long32_mod13_decode
                      test_dld_dst_long32_mod13_decode.c)
ti_c54x_add_unit_test(add_sub_acc_shift_decode test_add_sub_acc_shift_decode.c)
ti_c54x_add_unit_test(add_sub_dual_decode test_add_sub_dual_decode.c)
# W51C RED contract: non-MOD0/MOD0 dual ADD/SUB decode.
ti_c54x_add_unit_test(add_sub_dual_modes_decode test_add_sub_dual_modes_decode.c)
# W52C RED contract: MOD3-involving dual ADD/SUB decode.
ti_c54x_add_unit_test(add_sub_dual_mod3_decode test_add_sub_dual_mod3_decode.c)
# W52A COMMON repeat-truth closure: TI-evidence-bound RPT decode contract.
ti_c54x_add_unit_test(rpt_immediate_decode_common test_rpt_immediate_decode.c)
# W65B RED contract: CALL pmad decode with near-miss audit.
ti_c54x_add_unit_test(call_pmad_decode test_call_pmad_decode.c)
# W66B RED contract: RET decode with near-miss audit.
ti_c54x_add_unit_test(ret_decode test_ret_decode.c)
# W74 RED contract: RETE decode with near-miss audit.
ti_c54x_add_unit_test(rete_decode test_rete_decode.c)
# W75 RED contract: RETED decode with near-miss audit.
ti_c54x_add_unit_test(reted_decode test_reted_decode.c)
# W76 RED contract: RETF decode.
ti_c54x_add_unit_test(retf_decode test_retf_decode.c)
# W77 RED contract: RC decode.
ti_c54x_add_unit_test(rc_decode test_rc_decode.c)
# P3-S7 RED contract: STM decode.
ti_c54x_add_unit_test(stm_decode test_stm_decode.c)
# P3-S2 RED contract: RPT immediate rows.
ti_c54x_add_unit_test(rpt_k_decode test_rpt_k_decode.c)
ti_c54x_add_unit_test(rpt_lk_decode test_rpt_lk_decode.c)
# P3-S3 RED contract: MAC Smem row.
ti_c54x_add_unit_test(mac_smem_decode test_mac_smem_decode.c)
# W78 RED contract: RCD decode.
ti_c54x_add_unit_test(rcd_decode test_rcd_decode.c)
# W67B RED contract: RETD decode with near-miss audit.
ti_c54x_add_unit_test(retd_decode test_retd_decode.c)
# W67C RED contract: CALLD decode with near-miss audit.
ti_c54x_add_unit_test(calld_decode test_calld_decode.c)
