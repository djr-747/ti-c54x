# Test ownership: tests/unit/isa/catalog owns immutable catalog metadata
# identity/range and copied decode propagation proofs.
$(eval $(call ti_c54x_add_test,opcode_metadata,isa/catalog,test_opcode_metadata.c))
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
$(eval $(call ti_c54x_add_test,ldr_smem_metadata,isa/catalog,test_ldr_smem_metadata.c))
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
# W67B RED contract: RETD catalog row.
$(eval $(call ti_c54x_add_test,retd_metadata,isa/catalog,test_retd_metadata.c))
# W67C RED contract: CALLD catalog row.
$(eval $(call ti_c54x_add_test,calld_metadata,isa/catalog,test_calld_metadata.c))
