# Test ownership: tests/unit/isa/ops/arithmetic owns ADD/SUB accumulator proofs.
$(eval $(call ti_c54x_add_test,add_sub_acc_shift,isa/ops/arithmetic,test_add_sub_acc_shift.c))
$(eval $(call ti_c54x_add_test,add_sub_dual,isa/ops/arithmetic,test_add_sub_dual.c))
$(eval $(call ti_c54x_add_test,sqdst_xmem_ymem,isa/ops/arithmetic,test_sqdst_xmem_ymem.c))
$(eval $(call ti_c54x_add_test,abdst_xmem_ymem,isa/ops/arithmetic,test_abdst_xmem_ymem.c))
# W51C RED contract: non-MOD0/MOD0 dual ADD/SUB checked/trusted semantics.
$(eval $(call ti_c54x_add_test,add_sub_dual_modes,isa/ops/arithmetic,test_add_sub_dual_modes.c))
# W52C RED contract: MOD3-involving dual ADD/SUB checked/trusted semantics.
$(eval $(call ti_c54x_add_test,add_sub_dual_mod3,isa/ops/arithmetic,test_add_sub_dual_mod3.c))
# WAVE5-A RED contract: ADD #lk syntaxes 7-8.
$(eval $(call ti_c54x_add_test,add_lk_ops,isa/ops/arithmetic,test_add_lk.c))
# P3-S3z RED contract: ADD Smem syntaxes 1-3.
$(eval $(call ti_c54x_add_test,add_smem_ops,isa/ops/arithmetic,test_add_smem_ops.c))
$(eval $(call ti_c54x_add_test,dadd_ops,isa/ops/arithmetic,test_dadd.c))
$(eval $(call ti_c54x_add_test,dadst_ops,isa/ops/arithmetic,test_dadst.c))
$(eval $(call ti_c54x_add_test,drsub_ops,isa/ops/arithmetic,test_drsub.c))
$(eval $(call ti_c54x_add_test,dsadt_ops,isa/ops/arithmetic,test_dsadt.c))
$(eval $(call ti_c54x_add_test,dsub_ops,isa/ops/arithmetic,test_dsub.c))
$(eval $(call ti_c54x_add_test,dsubt_ops,isa/ops/arithmetic,test_dsubt.c))
$(eval $(call ti_c54x_add_test,add_smem_shift_ops,isa/ops/arithmetic,test_add_smem_shift.c))
# W5C RED contract: ADD src,ASM.
$(eval $(call ti_c54x_add_test,add_src_asm_ops,isa/ops/arithmetic,test_add_src_asm.c))
# WAVE5-A RED contract: SUB #lk syntaxes 7-8.
$(eval $(call ti_c54x_add_test,sub_lk_ops,isa/ops/arithmetic,test_sub_lk.c))
# P3-S14 RED contract: SUB Smem syntaxes 1-3.
$(eval $(call ti_c54x_add_test,sub_smem_ops,isa/ops/arithmetic,test_sub_smem.c))
# P3-S82 RED contract: SUB absolute Smem.
$(eval $(call ti_c54x_add_test,sub_smem_absolute_ops,isa/ops/arithmetic,test_sub_smem_absolute.c))
$(eval $(call ti_c54x_add_test,sub_smem_shift_ops,isa/ops/arithmetic,test_sub_smem_shift.c))
# W5C RED contract: SUB src,ASM.
$(eval $(call ti_c54x_add_test,sub_src_asm_ops,isa/ops/arithmetic,test_sub_src_asm.c))
# FORM-T2-02-13 RED contract: SUBS Smem.
$(eval $(call ti_c54x_add_test,subs_smem_ops,isa/ops/arithmetic,test_subs_smem.c))
# FORM-T2-02-11 RED contract: SUBB Smem.
$(eval $(call ti_c54x_add_test,subb_smem_ops,isa/ops/arithmetic,test_subb_smem.c))
# W7B RED contract: ADDC/ADDS/SUBC Smem.
$(eval $(call ti_c54x_add_test,addc_smem_ops,isa/ops/arithmetic,test_addc_smem.c))
$(eval $(call ti_c54x_add_test,adds_smem_ops,isa/ops/arithmetic,test_adds_smem.c))
$(eval $(call ti_c54x_add_test,subc_smem_ops,isa/ops/arithmetic,test_subc_smem.c))
$(eval $(call ti_c54x_add_test,add_xmem_ops,isa/ops/arithmetic,test_add_xmem.c))
$(eval $(call ti_c54x_add_test,sub_xmem_ops,isa/ops/arithmetic,test_sub_xmem.c))
# W7A RED contract: RND src[,dst] and SAT src accumulator semantics.
$(eval $(call ti_c54x_add_test,rnd_acc_ops,isa/ops/arithmetic,test_rnd_acc_ops.c))
$(eval $(call ti_c54x_add_test,sat_acc_ops,isa/ops/arithmetic,test_sat_acc_ops.c))
$(eval $(call ti_c54x_add_test,abs_acc_ops,isa/ops/arithmetic,test_abs_acc.c))
$(eval $(call ti_c54x_add_test,neg_acc_ops,isa/ops/arithmetic,test_neg_acc.c))
$(eval $(call ti_c54x_add_test,norm_acc_ops,isa/ops/arithmetic,test_norm_acc.c))
