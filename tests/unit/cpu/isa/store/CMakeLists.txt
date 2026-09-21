# Test ownership: this leaf owns only public-CPU fixed-MOD2 STL proofs.
ti_c54x_add_unit_test(cpu_stl_smem test_cpu_stl_smem.c)
ti_c54x_add_unit_test(cpu_stl_smem_absolute test_cpu_stl_smem_absolute.c)
# P3 slice 10 RED contract: STH CPU retirement.
ti_c54x_add_unit_test(cpu_sth_smem test_cpu_sth_smem.c)
ti_c54x_add_unit_test(cpu_sth_smem_absolute test_cpu_sth_smem_absolute.c)
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
ti_c54x_add_unit_test(cpu_mvdk test_cpu_mvdk.c)
ti_c54x_add_unit_test(cpu_mvkd test_cpu_mvkd.c)
ti_c54x_add_unit_test(cpu_mvpd test_cpu_mvpd.c)
ti_c54x_add_unit_test(cpu_mvdp test_cpu_mvdp.c)
# P3-S104: MVDD dual-move pair retirement.
ti_c54x_add_unit_test(cpu_mvdd test_cpu_mvdd.c)
ti_c54x_add_unit_test(cpu_mvdp_smem_absolute test_cpu_mvdp_smem_absolute.c)
ti_c54x_add_unit_test(cpu_mvpd_smem_absolute test_cpu_mvpd_smem_absolute.c)
ti_c54x_add_unit_test(cpu_dst_smem_absolute test_cpu_dst_smem_absolute.c)
ti_c54x_add_unit_test(cpu_reada test_cpu_reada.c)
ti_c54x_add_unit_test(cpu_writa test_cpu_writa.c)
# P3-S7 RED contract: STM CPU retirement.
ti_c54x_add_unit_test(cpu_stm test_cpu_stm.c)
# P3-S67 RED contract: SWWSR peripheral-cell CPU round trip.
ti_c54x_add_unit_test(cpu_swwsr test_cpu_swwsr.c)
# P3-S68 RED contract: CLKMD peripheral-cell CPU retirement.
ti_c54x_add_unit_test(cpu_clkmd test_cpu_clkmd.c)
# P3-S70 RED contract: absolute ST-lk CPU retirement.
ti_c54x_add_unit_test(cpu_st_lk_absolute test_cpu_st_lk_absolute.c)
ti_c54x_add_unit_test(cpu_st_t_smem test_cpu_st_t_smem.c)
ti_c54x_add_unit_test(cpu_cmps_smem_direct test_cpu_cmps_smem_direct.c)
