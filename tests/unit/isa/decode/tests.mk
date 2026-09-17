# Test ownership: tests/unit/isa/decode owns exhaustive first-word decode
# classification and catalog metadata coverage.
$(eval $(call ti_c54x_add_test,nop_decode,isa/decode,test_nop_decode.c))
$(eval $(call ti_c54x_add_test,operand_field,isa/decode,test_operand_field.c))
$(eval $(call ti_c54x_add_test,instruction_bundle,isa/decode,test_instruction_bundle.c))
$(eval $(call ti_c54x_add_test,operand_layout_binding,isa/decode,test_operand_layout_binding.c))
$(eval $(call ti_c54x_add_test,trusted_execution_packet,isa/decode,test_trusted_execution_packet.c))
$(eval $(call ti_c54x_add_test,ld_status_fields_decode,isa/decode,test_ld_status_fields.c))
$(eval $(call ti_c54x_add_test,max_min_decode,isa/decode,test_max_min_decode.c))
$(eval $(call ti_c54x_add_test,cmpl_decode,isa/decode,test_cmpl_decode.c))
$(eval $(call ti_c54x_add_test,rol_ror_decode,isa/decode,test_rol_ror_decode.c))
$(eval $(call ti_c54x_add_test,sftl_decode,isa/decode,test_sftl_decode.c))
$(eval $(call ti_c54x_add_test,logical_accumulator_decode,isa/decode,test_logical_accumulator_decode.c))
$(eval $(call ti_c54x_add_test,roltc_decode,isa/decode,test_roltc_decode.c))
$(eval $(call ti_c54x_add_test,sftc_decode,isa/decode,test_sftc_decode.c))
$(eval $(call ti_c54x_add_test,exp_decode,isa/decode,test_exp_decode.c))
$(eval $(call ti_c54x_add_test,logical_immediate_decode,isa/decode,test_logical_immediate_decode.c))
$(eval $(call ti_c54x_add_test,fixed_mod2_memory_decode,isa/decode,test_fixed_mod2_memory_decode.c))
$(eval $(call ti_c54x_add_test,ssbx_rsbx_decode,isa/decode,test_ssbx_rsbx_decode.c))
$(eval $(call ti_c54x_add_test,cmpr_decode,isa/decode,test_cmpr_decode.c))
$(eval $(call ti_c54x_add_test,ldu_smem_decode,isa/decode,test_ldu_smem_decode.c))
$(eval $(call ti_c54x_add_test,ld_t_smem_decode,isa/decode,test_ld_t_smem_decode.c))
$(eval $(call ti_c54x_add_test,ld_status_smem_decode,isa/decode,test_ld_status_smem_decode.c))
$(eval $(call ti_c54x_add_test,ld_smem_shift16_decode,isa/decode,test_ld_smem_shift16_decode.c))
$(eval $(call ti_c54x_add_test,ld_smem_shift_decode,isa/decode,test_ld_smem_shift_decode.c))
$(eval $(call ti_c54x_add_test,ld_src_shift_decode,isa/decode,test_ld_src_shift_decode.c))
$(eval $(call ti_c54x_add_test,ld_k_decode,isa/decode,test_ld_k_decode.c))
$(eval $(call ti_c54x_add_test,ld_lk16_decode,isa/decode,test_ld_lk16_decode.c))
$(eval $(call ti_c54x_add_test,ld_lk_shift_decode,isa/decode,test_ld_lk_shift_decode.c))
$(eval $(call ti_c54x_add_test,b_pmad_decode,isa/decode,test_b_pmad_decode.c))
# W68 RED contract: BD decode.
$(eval $(call ti_c54x_add_test,bd_decode,isa/decode,test_bd_decode.c))
$(eval $(call ti_c54x_add_test,bc_pmad_decode,isa/decode,test_bc_pmad_decode.c))
# W69 RED contract: BCD decode.
$(eval $(call ti_c54x_add_test,bcd_decode,isa/decode,test_bcd_decode.c))
# W70 RED contract: BANZ decode.
$(eval $(call ti_c54x_add_test,banz_decode,isa/decode,test_banz_decode.c))
# W71 RED contract: BANZD decode.
$(eval $(call ti_c54x_add_test,banzd_decode,isa/decode,test_banzd_decode.c))
# W72 RED contract: CCD decode.
$(eval $(call ti_c54x_add_test,ccd_decode,isa/decode,test_ccd_decode.c))
# W73 RED contract: CC decode.
$(eval $(call ti_c54x_add_test,cc_decode,isa/decode,test_cc_decode.c))
$(eval $(call ti_c54x_add_test,ldr_smem_decode,isa/decode,test_ldr_smem_decode.c))
$(eval $(call ti_c54x_add_test,ld_xmem_shift_decode,isa/decode,test_ld_xmem_shift_decode.c))
$(eval $(call ti_c54x_add_test,ld_src_asm_decode,isa/decode,test_ld_src_asm_decode.c))
$(eval $(call ti_c54x_add_test,ld_smem_ts_decode,isa/decode,test_ld_smem_ts_decode.c))
$(eval $(call ti_c54x_add_test,st_immediate_smem_decode,isa/decode,test_st_immediate_smem_decode.c))
$(eval $(call ti_c54x_add_test,st_t_smem_decode,isa/decode,test_st_t_smem_decode.c))
$(eval $(call ti_c54x_add_test,standard_indirect_memory_decode,isa/decode,test_standard_indirect_memory_decode.c))
$(eval $(call ti_c54x_add_test,indexed_standard_indirect_decode,isa/decode,test_indexed_standard_indirect_decode.c))
$(eval $(call ti_c54x_add_test,circular_standard_indirect_decode,isa/decode,test_circular_standard_indirect_decode.c))
$(eval $(call ti_c54x_add_test,reverse_carry_standard_indirect_decode,isa/decode,test_reverse_carry_standard_indirect_decode.c))
$(eval $(call ti_c54x_add_test,atomic_logical_memory_decode,isa/decode,test_atomic_logical_memory_decode.c))
$(eval $(call ti_c54x_add_test,dld_dst_long32_mod13_decode,isa/decode,test_dld_dst_long32_mod13_decode.c))
$(eval $(call ti_c54x_add_test,add_sub_acc_shift_decode,isa/decode,test_add_sub_acc_shift_decode.c))
$(eval $(call ti_c54x_add_test,add_sub_dual_decode,isa/decode,test_add_sub_dual_decode.c))
# W51C RED contract: non-MOD0/MOD0 dual ADD/SUB decode.
$(eval $(call ti_c54x_add_test,add_sub_dual_modes_decode,isa/decode,test_add_sub_dual_modes_decode.c))
# W52C RED contract: MOD3-involving dual ADD/SUB decode.
$(eval $(call ti_c54x_add_test,add_sub_dual_mod3_decode,isa/decode,test_add_sub_dual_mod3_decode.c))
# W52A COMMON repeat-truth closure: TI-evidence-bound RPT decode contract.
$(eval $(call ti_c54x_add_test,rpt_immediate_decode_common,isa/decode,test_rpt_immediate_decode.c))
# W65B RED contract: CALL pmad decode with near-miss audit.
$(eval $(call ti_c54x_add_test,call_pmad_decode,isa/decode,test_call_pmad_decode.c))
# W66B RED contract: RET decode with near-miss audit.
$(eval $(call ti_c54x_add_test,ret_decode,isa/decode,test_ret_decode.c))
# W74 RED contract: RETE decode.
# W75 RED contract: RETED decode.
$(eval $(call ti_c54x_add_test,reted_decode,isa/decode,test_reted_decode.c))
# W76 RED contract: RETF decode.
$(eval $(call ti_c54x_add_test,retf_decode,isa/decode,test_retf_decode.c))
# W77 RED contract: RC decode.
$(eval $(call ti_c54x_add_test,rc_decode,isa/decode,test_rc_decode.c))
# P3-S7 RED contract: STM decode.
$(eval $(call ti_c54x_add_test,stm_decode,isa/decode,test_stm_decode.c))
# P3-S2 RED contract: RPT immediate rows.
$(eval $(call ti_c54x_add_test,rpt_k_decode,isa/decode,test_rpt_k_decode.c))
$(eval $(call ti_c54x_add_test,rpt_lk_decode,isa/decode,test_rpt_lk_decode.c))
# P3-S3 RED contract: MAC Smem row.
$(eval $(call ti_c54x_add_test,mac_smem_decode,isa/decode,test_mac_smem_decode.c))
$(eval $(call ti_c54x_add_test,mpy_smem_decode,isa/decode,test_mpy_smem_decode.c))
$(eval $(call ti_c54x_add_test,indirect_addr7_layout,isa/decode,test_indirect_addr7_layout.c))
# P3-S3 RED contract: SQUR Smem square row plus destination layout.
$(eval $(call ti_c54x_add_test,squr_smem_decode,isa/decode,test_squr_smem_decode.c))
$(eval $(call ti_c54x_add_test,destination_indirect_addr7_layout,isa/decode,test_destination_indirect_addr7_layout.c))
# P3-S3 RED contract: SQURA/SQURS accumulate squares.
$(eval $(call ti_c54x_add_test,squr_acc_decode,isa/decode,test_squr_acc_decode.c))
# P3-S3 RED contract: MPYU unsigned multiply.
$(eval $(call ti_c54x_add_test,mpyu_smem_decode,isa/decode,test_mpyu_smem_decode.c))
# P3-S3 RED contract: MACA T,src register-only row.
$(eval $(call ti_c54x_add_test,maca_t_decode,isa/decode,test_maca_t_decode.c))
# P3-S3 RED contract: MACA Smem accumulate-A row.
$(eval $(call ti_c54x_add_test,maca_smem_decode,isa/decode,test_maca_smem_decode.c))
# P3-S3 RED contract: MACA Smem round/indirect layout.
$(eval $(call ti_c54x_add_test,round_indirect_addr7_layout,isa/decode,test_round_indirect_addr7_layout.c))
# P3-S3 RED contract: MACA T,src source/destination/round layout.
$(eval $(call ti_c54x_add_test,source_destination_round_layout,isa/decode,test_source_destination_round_layout.c))
# P3-S3 RED contract: MACP Smem,Pmad row.
$(eval $(call ti_c54x_add_test,macp_smem_decode,isa/decode,test_macp_smem_decode.c))
# P3-S3 RED contract: MACP Smem destination/indirect/addr7/pmad16 layout.
$(eval $(call ti_c54x_add_test,destination_indirect_addr7_pmad16_layout,isa/decode,test_destination_indirect_addr7_pmad16_layout.c))
$(eval $(call ti_c54x_add_test,mpya_smem_decode,isa/decode,test_mpya_smem_decode.c))
$(eval $(call ti_c54x_add_test,mas_smem_decode,isa/decode,test_mas_smem_decode.c))
# P3-S3 RED contract: MPYA dst register-only row.
$(eval $(call ti_c54x_add_test,mpya_dst_decode,isa/decode,test_mpya_dst_decode.c))
# P3-S3 RED contract: SQUR A,dst register-only row.
$(eval $(call ti_c54x_add_test,squr_adst_decode,isa/decode,test_squr_adst_decode.c))
# W78 RED contract: RCD decode.
$(eval $(call ti_c54x_add_test,rcd_decode,isa/decode,test_rcd_decode.c))
$(eval $(call ti_c54x_add_test,rete_decode,isa/decode,test_rete_decode.c))
# W67B RED contract: RETD decode.
$(eval $(call ti_c54x_add_test,retd_decode,isa/decode,test_retd_decode.c))
# W67C RED contract: CALLD decode.
$(eval $(call ti_c54x_add_test,calld_decode,isa/decode,test_calld_decode.c))
