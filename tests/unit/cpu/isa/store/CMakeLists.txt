# Test ownership: this leaf owns only public-CPU fixed-MOD2 STL proofs.
ti_c54x_add_unit_test(cpu_stl_smem test_cpu_stl_smem.c)
# P3 slice 10 RED contract: STH CPU retirement.
ti_c54x_add_unit_test(cpu_sth_smem test_cpu_sth_smem.c)
# P3 slice 11 RED contract: STH-by-ASM CPU retirement.
ti_c54x_add_unit_test(cpu_sth_asm_smem test_cpu_sth_asm_smem.c)
# P3 slice 12 RED contract: STH-by-SHFT CPU retirement.
ti_c54x_add_unit_test(cpu_sth_shft_xmem test_cpu_sth_shft_xmem.c)
ti_c54x_add_unit_test(cpu_stl_shft_xmem test_cpu_stl_shft_xmem.c)
# P3 slice 13 RED contract: STH-by-SHIFT direct CPU retirement.
ti_c54x_add_unit_test(cpu_sth_shift_direct test_cpu_sth_shift_direct.c)
ti_c54x_add_unit_test(cpu_st_immediate_smem test_cpu_st_immediate_smem.c)
ti_c54x_add_unit_test(cpu_mvdm test_cpu_mvdm.c)
ti_c54x_add_unit_test(cpu_mvmd
                      test_cpu_mvmd.c)
ti_c54x_add_unit_test(cpu_mvmm test_cpu_mvmm.c)
# P3-S7 RED contract: STM CPU retirement.
ti_c54x_add_unit_test(cpu_stm test_cpu_stm.c)
ti_c54x_add_unit_test(cpu_st_t_smem test_cpu_st_t_smem.c)
ti_c54x_add_unit_test(cpu_cmps_smem_direct test_cpu_cmps_smem_direct.c)
