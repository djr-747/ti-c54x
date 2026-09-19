# Test ownership: tests/unit/isa/ops/load owns pure load semantic effects.
ti_c54x_add_unit_test(ld_status_fields_ops test_ld_status_fields.c)
ti_c54x_add_unit_test(ld_smem_ops test_ld_smem.c)
ti_c54x_add_unit_test(ld_smem_absolute_ops test_ld_smem_absolute.c)
ti_c54x_add_unit_test(ldu_smem_ops test_ldu_smem.c)
# P3-S66 RED contract: LDU DP-relative direct loads.
ti_c54x_add_unit_test(ldu_direct_ops test_ldu_direct.c)
ti_c54x_add_unit_test(ldm_smem_direct_ops test_ldm_smem_direct.c)
ti_c54x_add_unit_test(ld_t_smem_ops test_ld_t_smem.c)
ti_c54x_add_unit_test(ltd_smem_ops test_ltd_smem.c)
ti_c54x_add_unit_test(delay_smem_ops test_delay_smem.c)
ti_c54x_add_unit_test(ld_status_smem_ops test_ld_status_smem.c)
ti_c54x_add_unit_test(ld_smem_shift16_ops test_ld_smem_shift16.c)
ti_c54x_add_unit_test(ld_smem_shift test_ld_smem_shift.c)
ti_c54x_add_unit_test(ld_src_shift test_ld_src_shift.c)
ti_c54x_add_unit_test(ld_k test_ld_k.c)
ti_c54x_add_unit_test(ld_lk16 test_ld_lk16.c)
ti_c54x_add_unit_test(ld_lk_shift test_ld_lk_shift.c)
ti_c54x_add_unit_test(ldr_smem test_ldr_smem.c)
ti_c54x_add_unit_test(ld_xmem_shift test_ld_xmem_shift.c)
ti_c54x_add_unit_test(ld_src_asm test_ld_src_asm.c)
ti_c54x_add_unit_test(ld_smem_ts_ops test_ld_smem_ts.c)
ti_c54x_add_unit_test(standard_indirect_load_modes
                      test_standard_indirect_load_modes.c)
ti_c54x_add_unit_test(indexed_standard_indirect_load_effects
                      test_indexed_standard_indirect_load_effects.c)
ti_c54x_add_unit_test(circular_standard_indirect_load_effects
                      test_circular_standard_indirect_load_effects.c)
ti_c54x_add_unit_test(reverse_carry_standard_indirect_load_effects
                      test_reverse_carry_standard_indirect_load_effects.c)
ti_c54x_add_unit_test(dld_long32_mod13_ops test_dld_long32_mod13.c)
