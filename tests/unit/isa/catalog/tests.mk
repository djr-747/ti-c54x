# Test ownership: tests/unit/isa/catalog owns immutable catalog metadata
# identity/range and copied decode propagation proofs.
$(eval $(call ti_c54x_add_test,opcode_metadata,isa/catalog,test_opcode_metadata.c))
$(eval $(call ti_c54x_add_test,idle_metadata,isa/catalog,test_idle_metadata.c))
# P3-S18 lane 2b: qualified-slot match/mismatch vectors.
$(eval $(call ti_c54x_add_test,qualified_slots,isa/catalog,test_qualified_slots.c))
# P3-S18 lane 2b RED contract: STH MOD0-MOD3 qualified rows.
$(eval $(call ti_c54x_add_test,sth_shift_smem_mod0_3_metadata,isa/catalog,test_sth_shift_smem_mod0_3_metadata.c))
$(eval $(call ti_c54x_add_test,rol_ror_metadata,isa/catalog,test_rol_ror_metadata.c))
$(eval $(call ti_c54x_add_test,sftl_metadata,isa/catalog,test_sftl_metadata.c))
$(eval $(call ti_c54x_add_test,logical_accumulator_metadata,isa/catalog,test_logical_accumulator_metadata.c))
$(eval $(call ti_c54x_add_test,roltc_metadata,isa/catalog,test_roltc_metadata.c))
$(eval $(call ti_c54x_add_test,sftc_metadata,isa/catalog,test_sftc_metadata.c))
$(eval $(call ti_c54x_add_test,exp_metadata,isa/catalog,test_exp_metadata.c))
$(eval $(call ti_c54x_add_test,logical_immediate_metadata,isa/catalog,test_logical_immediate_metadata.c))
$(eval $(call ti_c54x_add_test,fixed_mod2_memory_metadata,isa/catalog,test_fixed_mod2_memory_metadata.c))
$(eval $(call ti_c54x_add_test,ssbx_rsbx_metadata,isa/catalog,test_ssbx_rsbx_metadata.c))
$(eval $(call ti_c54x_add_test,cmpr_metadata,isa/catalog,test_cmpr_metadata.c))
$(eval $(call ti_c54x_add_test,ldu_smem_metadata,isa/catalog,test_ldu_smem_metadata.c))
$(eval $(call ti_c54x_add_test,ldu_direct_metadata,isa/catalog,test_ldu_direct_metadata.c))
$(eval $(call ti_c54x_add_test,ld_t_smem_metadata,isa/catalog,test_ld_t_smem_metadata.c))
$(eval $(call ti_c54x_add_test,ld_status_smem_metadata,isa/catalog,test_ld_status_smem_metadata.c))
$(eval $(call ti_c54x_add_test,ld_smem_shift16_metadata,isa/catalog,test_ld_smem_shift16_metadata.c))
$(eval $(call ti_c54x_add_test,ld_smem_shift_metadata,isa/catalog,test_ld_smem_shift_metadata.c))
$(eval $(call ti_c54x_add_test,ld_src_shift_metadata,isa/catalog,test_ld_src_shift_metadata.c))
$(eval $(call ti_c54x_add_test,ld_k_metadata,isa/catalog,test_ld_k_metadata.c))
$(eval $(call ti_c54x_add_test,ld_lk16_metadata,isa/catalog,test_ld_lk16_metadata.c))
$(eval $(call ti_c54x_add_test,ld_lk_shift_metadata,isa/catalog,test_ld_lk_shift_metadata.c))
$(eval $(call ti_c54x_add_test,b_pmad_metadata,isa/catalog,test_b_pmad_metadata.c))
$(eval $(call ti_c54x_add_test,bc_pmad_metadata,isa/catalog,test_bc_pmad_metadata.c))
# W69 RED contract: BCD catalog row.
$(eval $(call ti_c54x_add_test,bcd_metadata,isa/catalog,test_bcd_metadata.c))
# W70 RED contract: BANZ catalog row.
$(eval $(call ti_c54x_add_test,banz_metadata,isa/catalog,test_banz_metadata.c))
# W71 RED contract: BANZD catalog row.
$(eval $(call ti_c54x_add_test,banzd_metadata,isa/catalog,test_banzd_metadata.c))
# W72 RED contract: CCD catalog row.
$(eval $(call ti_c54x_add_test,ccd_metadata,isa/catalog,test_ccd_metadata.c))
# W73 RED contract: CC catalog row.
$(eval $(call ti_c54x_add_test,cc_metadata,isa/catalog,test_cc_metadata.c))
$(eval $(call ti_c54x_add_test,ldr_smem_metadata,isa/catalog,test_ldr_smem_metadata.c))
$(eval $(call ti_c54x_add_test,ltd_smem_metadata,isa/catalog,test_ltd_smem_metadata.c))
$(eval $(call ti_c54x_add_test,delay_smem_metadata,isa/catalog,test_delay_smem_metadata.c))
$(eval $(call ti_c54x_add_test,ld_xmem_shift_metadata,isa/catalog,test_ld_xmem_shift_metadata.c))
$(eval $(call ti_c54x_add_test,ld_src_asm_metadata,isa/catalog,test_ld_src_asm_metadata.c))
$(eval $(call ti_c54x_add_test,ld_smem_ts_metadata,isa/catalog,test_ld_smem_ts_metadata.c))
$(eval $(call ti_c54x_add_test,st_immediate_smem_metadata,isa/catalog,test_st_immediate_smem_metadata.c))
$(eval $(call ti_c54x_add_test,st_t_smem_metadata,isa/catalog,test_st_t_smem_metadata.c))
$(eval $(call ti_c54x_add_test,standard_indirect_memory_metadata,isa/catalog,test_standard_indirect_memory_metadata.c))
$(eval $(call ti_c54x_add_test,indexed_standard_indirect_metadata,isa/catalog,test_indexed_standard_indirect_metadata.c))
$(eval $(call ti_c54x_add_test,circular_standard_indirect_metadata,isa/catalog,test_circular_standard_indirect_metadata.c))
$(eval $(call ti_c54x_add_test,reverse_carry_standard_indirect_metadata,isa/catalog,test_reverse_carry_standard_indirect_metadata.c))
$(eval $(call ti_c54x_add_test,atomic_logical_memory_metadata,isa/catalog,test_atomic_logical_memory_metadata.c))
$(eval $(call ti_c54x_add_test,dld_dst_long32_metadata,isa/catalog,test_dld_dst_long32_metadata.c))
$(eval $(call ti_c54x_add_test,dld_repeatability_contract,isa/catalog,test_dld_repeatability_contract.c))
$(eval $(call ti_c54x_add_test,add_sub_acc_shift_metadata,isa/catalog,test_add_sub_acc_shift_metadata.c))
$(eval $(call ti_c54x_add_test,add_sub_dual_metadata,isa/catalog,test_add_sub_dual_metadata.c))
# W51C RED contract: non-MOD0/MOD0 dual ADD/SUB row metadata.
$(eval $(call ti_c54x_add_test,add_sub_dual_modes_metadata,isa/catalog,test_add_sub_dual_modes_metadata.c))
# W52C RED contract: MOD3-involving dual ADD/SUB row metadata.
$(eval $(call ti_c54x_add_test,add_sub_dual_mod3_metadata,isa/catalog,test_add_sub_dual_mod3_metadata.c))
# W65B RED contract: CALL pmad row metadata.
$(eval $(call ti_c54x_add_test,call_pmad_metadata,isa/catalog,test_call_pmad_metadata.c))
# W68 RED contract: BD catalog row.
$(eval $(call ti_c54x_add_test,bd_metadata,isa/catalog,test_bd_metadata.c))
# W66B RED contract: RET row metadata.
$(eval $(call ti_c54x_add_test,ret_metadata,isa/catalog,test_ret_metadata.c))
# W74 RED contract: RETE catalog row.
# W75 RED contract: RETED catalog row.
$(eval $(call ti_c54x_add_test,reted_metadata,isa/catalog,test_reted_metadata.c))
# W76 RED contract: RETF catalog row.
$(eval $(call ti_c54x_add_test,retf_metadata,isa/catalog,test_retf_metadata.c))
# W77 RED contract: RC catalog row.
$(eval $(call ti_c54x_add_test,rc_metadata,isa/catalog,test_rc_metadata.c))
# W78 RED contract: RCD catalog row.
$(eval $(call ti_c54x_add_test,rcd_metadata,isa/catalog,test_rcd_metadata.c))
# P3-S7 RED contract: STM catalog row.
$(eval $(call ti_c54x_add_test,stm_metadata,isa/catalog,test_stm_metadata.c))
$(eval $(call ti_c54x_add_test,mvdm_metadata,isa/catalog,test_mvdm_metadata.c))
$(eval $(call ti_c54x_add_test,mvmd_metadata,isa/catalog,test_mvmd_metadata.c))
$(eval $(call ti_c54x_add_test,mvmm_metadata,isa/catalog,test_mvmm_metadata.c))
$(eval $(call ti_c54x_add_test,mvdk_direct_metadata,isa/catalog,test_mvdk_direct_metadata.c))
$(eval $(call ti_c54x_add_test,mvdk_smem_metadata,isa/catalog,test_mvdk_smem_metadata.c))
$(eval $(call ti_c54x_add_test,mvkd_direct_metadata,isa/catalog,test_mvkd_direct_metadata.c))
$(eval $(call ti_c54x_add_test,mvkd_smem_metadata,isa/catalog,test_mvkd_smem_metadata.c))
$(eval $(call ti_c54x_add_test,mvpd_direct_metadata,isa/catalog,test_mvpd_direct_metadata.c))
$(eval $(call ti_c54x_add_test,mvpd_smem_metadata,isa/catalog,test_mvpd_smem_metadata.c))
$(eval $(call ti_c54x_add_test,mvdp_direct_metadata,isa/catalog,test_mvdp_direct_metadata.c))
$(eval $(call ti_c54x_add_test,mvdp_smem_metadata,isa/catalog,test_mvdp_smem_metadata.c))
$(eval $(call ti_c54x_add_test,mvdp_smem_absolute_metadata,isa/catalog,test_mvdp_smem_absolute_metadata.c))
$(eval $(call ti_c54x_add_test,mvpd_smem_absolute_metadata,isa/catalog,test_mvpd_smem_absolute_metadata.c))
# P3-S104 RED contract: MVDD dual-move pair row.
$(eval $(call ti_c54x_add_test,mvdd_metadata,isa/catalog,test_mvdd_metadata.c))
$(eval $(call ti_c54x_add_test,addm_lk_smem_absolute_metadata,isa/catalog,test_addm_lk_smem_absolute_metadata.c))
$(eval $(call ti_c54x_add_test,ld_smem_absolute_metadata,isa/catalog,test_ld_smem_absolute_metadata.c))
$(eval $(call ti_c54x_add_test,dst_smem_absolute_metadata,isa/catalog,test_dst_smem_absolute_metadata.c))
$(eval $(call ti_c54x_add_test,dld_smem_absolute_metadata,isa/catalog,test_dld_smem_absolute_metadata.c))
$(eval $(call ti_c54x_add_test,ld_smem_absolute_shift16_metadata,isa/catalog,test_ld_smem_absolute_shift16_metadata.c))
$(eval $(call ti_c54x_add_test,or_smem_absolute_metadata,isa/catalog,test_or_smem_absolute_metadata.c))
$(eval $(call ti_c54x_add_test,sub_smem_absolute_metadata,isa/catalog,test_sub_smem_absolute_metadata.c))
$(eval $(call ti_c54x_add_test,sth_smem_absolute_metadata,isa/catalog,test_sth_smem_absolute_metadata.c))
$(eval $(call ti_c54x_add_test,stl_smem_absolute_metadata,isa/catalog,test_stl_smem_absolute_metadata.c))
$(eval $(call ti_c54x_add_test,portw_smem_absolute_metadata,isa/catalog,test_portw_smem_absolute_metadata.c))
$(eval $(call ti_c54x_add_test,portr_smem_absolute_metadata,isa/catalog,test_portr_smem_absolute_metadata.c))
$(eval $(call ti_c54x_add_test,reada_direct_metadata,isa/catalog,test_reada_direct_metadata.c))
$(eval $(call ti_c54x_add_test,reada_smem_metadata,isa/catalog,test_reada_smem_metadata.c))
$(eval $(call ti_c54x_add_test,writa_direct_metadata,isa/catalog,test_writa_direct_metadata.c))
$(eval $(call ti_c54x_add_test,writa_smem_metadata,isa/catalog,test_writa_smem_metadata.c))
# P3-S2 RED contract: RPT immediate rows.
$(eval $(call ti_c54x_add_test,rpt_k_metadata,isa/catalog,test_rpt_k_metadata.c))
$(eval $(call ti_c54x_add_test,rpt_lk_metadata,isa/catalog,test_rpt_lk_metadata.c))
$(eval $(call ti_c54x_add_test,rptb_metadata,isa/catalog,test_rptb_metadata.c))
# P3-S3 RED contract: MAC Smem row.
$(eval $(call ti_c54x_add_test,mac_smem_metadata,isa/catalog,test_mac_smem_metadata.c))
$(eval $(call ti_c54x_add_test,mpy_smem_metadata,isa/catalog,test_mpy_smem_metadata.c))
$(eval $(call ti_c54x_add_test,mpya_smem_metadata,isa/catalog,test_mpya_smem_metadata.c))
# P3-S33 RED contract: MPY/MAC Smem,#lk syntax rows.
$(eval $(call ti_c54x_add_test,mpy_smem_lk_metadata,isa/catalog,test_mpy_smem_lk_metadata.c))
$(eval $(call ti_c54x_add_test,mac_smem_lk_metadata,isa/catalog,test_mac_smem_lk_metadata.c))
$(eval $(call ti_c54x_add_test,mas_smem_metadata,isa/catalog,test_mas_smem_metadata.c))
# P3-S3 RED contract: MPYA dst register-only row.
$(eval $(call ti_c54x_add_test,mpya_dst_metadata,isa/catalog,test_mpya_dst_metadata.c))
# P3-S3 RED contract: SQUR A,dst register-only row.
$(eval $(call ti_c54x_add_test,squr_adst_metadata,isa/catalog,test_squr_adst_metadata.c))
# P3-S3 RED contract: SQUR Smem square row.
$(eval $(call ti_c54x_add_test,squr_smem_metadata,isa/catalog,test_squr_smem_metadata.c))
# P3-S3 RED contract: SQURA/SQURS accumulate squares.
$(eval $(call ti_c54x_add_test,squr_acc_metadata,isa/catalog,test_squr_acc_metadata.c))
# P3-S3 RED contract: MPYU unsigned multiply.
$(eval $(call ti_c54x_add_test,mpyu_smem_metadata,isa/catalog,test_mpyu_smem_metadata.c))
# P3-S3 RED contract: MACA T,src register-only row.
$(eval $(call ti_c54x_add_test,maca_t_metadata,isa/catalog,test_maca_t_metadata.c))
# P3-S3 RED contract: MACA Smem accumulate-A row.
$(eval $(call ti_c54x_add_test,maca_smem_metadata,isa/catalog,test_maca_smem_metadata.c))
$(eval $(call ti_c54x_add_test,poly_smem_metadata,isa/catalog,test_poly_smem_metadata.c))
# P3-S114 RED contract: LD parallel-MAC/MAS metadata.
$(eval $(call ti_c54x_add_test,ld_parallel_metadata,isa/catalog,test_ld_parallel_metadata.c))
# P3-S3 RED contract: MACP Smem,Pmad row.
$(eval $(call ti_c54x_add_test,macp_smem_metadata,isa/catalog,test_macp_smem_metadata.c))
# P3-S3 RED contract: MACD Smem,Pmad row.
$(eval $(call ti_c54x_add_test,macd_smem_metadata,isa/catalog,test_macd_smem_metadata.c))
$(eval $(call ti_c54x_add_test,rete_metadata,isa/catalog,test_rete_metadata.c))
# W67B RED contract: RETD catalog row.
$(eval $(call ti_c54x_add_test,retd_metadata,isa/catalog,test_retd_metadata.c))
# W67C RED contract: CALLD catalog row.
$(eval $(call ti_c54x_add_test,calld_metadata,isa/catalog,test_calld_metadata.c))
# W7A RED contract: RND/SAT accumulator rows.
$(eval $(call ti_c54x_add_test,rnd_acc_metadata,isa/catalog,test_rnd_acc_metadata.c))
$(eval $(call ti_c54x_add_test,sat_acc_metadata,isa/catalog,test_sat_acc_metadata.c))
# S36 RED contract: BITF/CMPM/BITT/ST-lk rows.
$(eval $(call ti_c54x_add_test,bitf_smem_lk_metadata,isa/catalog,test_bitf_smem_lk_metadata.c))
$(eval $(call ti_c54x_add_test,cmpm_smem_lk_metadata,isa/catalog,test_cmpm_smem_lk_metadata.c))
$(eval $(call ti_c54x_add_test,bitt_smem_metadata,isa/catalog,test_bitt_smem_metadata.c))
$(eval $(call ti_c54x_add_test,bit_xmem_metadata,isa/catalog,test_bit_xmem_metadata.c))
$(eval $(call ti_c54x_add_test,sqdst_xmem_ymem_metadata,isa/catalog,test_sqdst_xmem_ymem.c))
$(eval $(call ti_c54x_add_test,abdst_xmem_ymem_metadata,isa/catalog,test_abdst_xmem_ymem.c))
# P3-S106 RED contract: LMS dual-memory row.
$(eval $(call ti_c54x_add_test,lms_xmem_ymem_metadata,isa/catalog,test_lms_metadata.c))
# P3-S108 RED contract: FIRS dual-filter row.
$(eval $(call ti_c54x_add_test,firs_xmem_ymem_pmad_metadata,isa/catalog,test_firs_metadata.c))
$(eval $(call ti_c54x_add_test,reset_metadata,isa/catalog,test_reset_metadata.c))
# P3-S105 RED contract: TRAP K row.
$(eval $(call ti_c54x_add_test,trap_metadata,isa/catalog,test_trap_metadata.c))
# P3-S110 RED contract: INTR K row.
$(eval $(call ti_c54x_add_test,intr_metadata,isa/catalog,test_intr_metadata.c))
$(eval $(call ti_c54x_add_test,st_lk_smem_metadata,isa/catalog,test_st_lk_smem_metadata.c))
$(eval $(call ti_c54x_add_test,st_lk_smem_absolute_metadata,isa/catalog,test_st_lk_smem_absolute_metadata.c))
$(eval $(call ti_c54x_add_test,st_t_smem_direct_metadata,isa/catalog,test_st_t_smem_direct_metadata.c))
# P3-S54 RED contract: STL-by-SHFT Xmem row.
$(eval $(call ti_c54x_add_test,stl_shft_xmem_metadata,isa/catalog,test_stl_shft_xmem_metadata.c))
# P3-S55 lane 2 RED contract: DADD Lmem row.
$(eval $(call ti_c54x_add_test,dadd_metadata,isa/catalog,test_dadd_metadata.c))
$(eval $(call ti_c54x_add_test,dadst_metadata,isa/catalog,test_dadst_metadata.c))
$(eval $(call ti_c54x_add_test,drsub_metadata,isa/catalog,test_drsub_metadata.c))
$(eval $(call ti_c54x_add_test,dsadt_metadata,isa/catalog,test_dsadt_metadata.c))
$(eval $(call ti_c54x_add_test,dsub_metadata,isa/catalog,test_dsub_metadata.c))
$(eval $(call ti_c54x_add_test,dsubt_metadata,isa/catalog,test_dsubt_metadata.c))
$(eval $(call ti_c54x_add_test,stlm_smem_direct_metadata,isa/catalog,test_stlm_smem_direct_metadata.c))
$(eval $(call ti_c54x_add_test,cmps_smem_direct_metadata,isa/catalog,test_cmps_smem_direct_metadata.c))
# P3-S51 RED contract: FRAME metadata.
$(eval $(call ti_c54x_add_test,frame_k_metadata,isa/catalog,test_frame_k_metadata.c))
$(eval $(call ti_c54x_add_test,pshd_popd_direct_metadata,isa/catalog,test_pshd_popd_direct_metadata.c))
$(eval $(call ti_c54x_add_test,ldm_smem_direct_metadata,isa/catalog,test_ldm_smem_direct_metadata.c))
$(eval $(call ti_c54x_add_test,stl_smem_direct_metadata,isa/catalog,test_stl_smem_direct_metadata.c))
$(eval $(call ti_c54x_add_test,stl_asm_smem_direct_metadata,isa/catalog,test_stl_asm_smem_direct_metadata.c))
$(eval $(call ti_c54x_add_test,st_trn_smem_direct_metadata,isa/catalog,test_st_trn_smem_direct_metadata.c))
# W7 RED contract: ABS/NEG/NORM accumulator rows.
$(eval $(call ti_c54x_add_test,abs_acc_metadata,isa/catalog,test_abs_acc_metadata.c))
$(eval $(call ti_c54x_add_test,neg_acc_metadata,isa/catalog,test_neg_acc_metadata.c))
$(eval $(call ti_c54x_add_test,norm_acc_metadata,isa/catalog,test_norm_acc_metadata.c))
$(eval $(call ti_c54x_add_test,sfta_acc_metadata,isa/catalog,test_sfta_acc_metadata.c))
$(eval $(call ti_c54x_add_test,addc_smem_metadata,isa/catalog,test_addc_smem_metadata.c))
$(eval $(call ti_c54x_add_test,adds_smem_metadata,isa/catalog,test_adds_smem_metadata.c))
$(eval $(call ti_c54x_add_test,subc_smem_metadata,isa/catalog,test_subc_smem_metadata.c))
# P3-S77 RED contract: MAR Smem metadata.
$(eval $(call ti_c54x_add_test,mar_smem_metadata,isa/catalog,test_mar_smem_metadata.c))
# P3-S115 RED contract: ST parallel-ADD/SUB metadata.
$(eval $(call ti_c54x_add_test,st_parallel_metadata,isa/catalog,test_st_parallel_metadata.c))
