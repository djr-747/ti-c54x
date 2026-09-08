# Test ownership: tests/unit/isa/ops/store owns pure STL memory effects.
ti_c54x_add_unit_test(stl_smem_ops test_stl_smem.c)
ti_c54x_add_unit_test(st_immediate_smem_ops test_st_immediate_smem.c)
ti_c54x_add_unit_test(st_t_smem_ops test_st_t_smem.c)
ti_c54x_add_unit_test(standard_indirect_store_modes
                      test_standard_indirect_store_modes.c)
