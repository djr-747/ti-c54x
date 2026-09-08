# Test ownership: tests/unit/isa/ops/load owns pure load semantic effects.
ti_c54x_add_unit_test(ld_status_fields_ops test_ld_status_fields.c)
ti_c54x_add_unit_test(ld_smem_ops test_ld_smem.c)
ti_c54x_add_unit_test(ldu_smem_ops test_ldu_smem.c)
ti_c54x_add_unit_test(ld_t_smem_ops test_ld_t_smem.c)
ti_c54x_add_unit_test(ld_status_smem_ops test_ld_status_smem.c)
ti_c54x_add_unit_test(ld_smem_shift16_ops test_ld_smem_shift16.c)
ti_c54x_add_unit_test(ld_smem_ts_ops test_ld_smem_ts.c)
ti_c54x_add_unit_test(standard_indirect_load_modes
                      test_standard_indirect_load_modes.c)
ti_c54x_add_unit_test(indexed_standard_indirect_load_effects
                      test_indexed_standard_indirect_load_effects.c)
