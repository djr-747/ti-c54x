# Test ownership: tests/unit/isa/ops/arithmetic owns ADD/SUB accumulator proofs.
$(eval $(call ti_c54x_add_test,add_sub_acc_shift,isa/ops/arithmetic,test_add_sub_acc_shift.c))
$(eval $(call ti_c54x_add_test,add_sub_dual,isa/ops/arithmetic,test_add_sub_dual.c))
# W51C RED contract: non-MOD0/MOD0 dual ADD/SUB checked/trusted semantics.
$(eval $(call ti_c54x_add_test,add_sub_dual_modes,isa/ops/arithmetic,test_add_sub_dual_modes.c))
# W52C RED contract: MOD3-involving dual ADD/SUB checked/trusted semantics.
$(eval $(call ti_c54x_add_test,add_sub_dual_mod3,isa/ops/arithmetic,test_add_sub_dual_mod3.c))
# WAVE5-A RED contract: ADD #lk syntaxes 7-8.
$(eval $(call ti_c54x_add_test,add_lk_ops,isa/ops/arithmetic,test_add_lk.c))
# P3-S3z RED contract: ADD Smem syntaxes 1-3.
$(eval $(call ti_c54x_add_test,add_smem_ops,isa/ops/arithmetic,test_add_smem_ops.c))
# W5C RED contract: ADD src,ASM.
$(eval $(call ti_c54x_add_test,add_src_asm_ops,isa/ops/arithmetic,test_add_src_asm.c))
# WAVE5-A RED contract: SUB #lk syntaxes 7-8.
$(eval $(call ti_c54x_add_test,sub_lk_ops,isa/ops/arithmetic,test_sub_lk.c))
# P3-S14 RED contract: SUB Smem syntaxes 1-3.
$(eval $(call ti_c54x_add_test,sub_smem_ops,isa/ops/arithmetic,test_sub_smem.c))
# W5C RED contract: SUB src,ASM.
$(eval $(call ti_c54x_add_test,sub_src_asm_ops,isa/ops/arithmetic,test_sub_src_asm.c))
# FORM-T2-02-13 RED contract: SUBS Smem.
$(eval $(call ti_c54x_add_test,subs_smem_ops,isa/ops/arithmetic,test_subs_smem.c))
# FORM-T2-02-11 RED contract: SUBB Smem.
$(eval $(call ti_c54x_add_test,subb_smem_ops,isa/ops/arithmetic,test_subb_smem.c))
$(eval $(call ti_c54x_add_test,add_xmem_ops,isa/ops/arithmetic,test_add_xmem.c))
$(eval $(call ti_c54x_add_test,sub_xmem_ops,isa/ops/arithmetic,test_sub_xmem.c))
