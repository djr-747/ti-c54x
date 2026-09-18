# Test ownership: tests/unit/isa/ops/arithmetic owns ADD/SUB accumulator proofs.
ti_c54x_add_unit_test(add_sub_acc_shift test_add_sub_acc_shift.c)
ti_c54x_add_unit_test(add_sub_dual test_add_sub_dual.c)
# W51C RED contract: non-MOD0/MOD0 dual ADD/SUB checked/trusted semantics.
ti_c54x_add_unit_test(add_sub_dual_modes test_add_sub_dual_modes.c)
# W52C RED contract: MOD3-involving dual ADD/SUB checked/trusted semantics.
ti_c54x_add_unit_test(add_sub_dual_mod3 test_add_sub_dual_mod3.c)
# WAVE5-A RED contract: ADD #lk syntaxes 7-8.
ti_c54x_add_unit_test(add_lk_ops test_add_lk.c)
# P3-S3z RED contract: ADD Smem syntaxes 1-3.
ti_c54x_add_unit_test(add_smem_ops test_add_smem_ops.c)
ti_c54x_add_unit_test(add_smem_shift_ops test_add_smem_shift.c)
# W5C RED contract: ADD src,ASM.
ti_c54x_add_unit_test(add_src_asm_ops test_add_src_asm.c)
# WAVE5-A RED contract: SUB #lk syntaxes 7-8.
ti_c54x_add_unit_test(sub_lk_ops test_sub_lk.c)
# P3-S14 RED contract: SUB Smem syntaxes 1-3.
ti_c54x_add_unit_test(sub_smem_ops test_sub_smem.c)
ti_c54x_add_unit_test(sub_smem_shift_ops test_sub_smem_shift.c)
# W5C RED contract: SUB src,ASM.
ti_c54x_add_unit_test(sub_src_asm_ops test_sub_src_asm.c)
# FORM-T2-02-13 RED contract: SUBS Smem.
ti_c54x_add_unit_test(subs_smem_ops test_subs_smem.c)
# FORM-T2-02-11 RED contract: SUBB Smem.
ti_c54x_add_unit_test(subb_smem_ops test_subb_smem.c)
ti_c54x_add_unit_test(add_xmem_ops test_add_xmem.c)
ti_c54x_add_unit_test(sub_xmem_ops test_sub_xmem.c)
