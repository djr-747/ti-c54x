# Test ownership: this leaf owns only public-CPU fixed-MOD2 STL proofs.
ti_c54x_add_unit_test(cpu_stl_smem test_cpu_stl_smem.c)
ti_c54x_add_unit_test(cpu_st_immediate_smem test_cpu_st_immediate_smem.c)
# P3-S7 RED contract: STM CPU retirement.
ti_c54x_add_unit_test(cpu_stm test_cpu_stm.c)
ti_c54x_add_unit_test(cpu_st_t_smem test_cpu_st_t_smem.c)
