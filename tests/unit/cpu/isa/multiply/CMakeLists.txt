# Test ownership: this leaf owns only public-CPU MAC retirement proofs.
ti_c54x_add_unit_test(cpu_mac_smem test_cpu_mac_smem.c)
ti_c54x_add_unit_test(cpu_mpy_smem test_cpu_mpy_smem.c)
ti_c54x_add_unit_test(cpu_mpya_smem test_cpu_mpya_smem.c)
ti_c54x_add_unit_test(cpu_mas_smem test_cpu_mas_smem.c)
ti_c54x_add_unit_test(cpu_mac_dual test_cpu_mac_dual.c)
ti_c54x_add_unit_test(cpu_mpy_dual test_cpu_mpy_dual.c)
ti_c54x_add_unit_test(cpu_mas_dual test_cpu_mas_dual.c)
# P3-S3 RED contract: MACSU Xmem,Ymem dual rows.
ti_c54x_add_unit_test(cpu_macsu_dual test_cpu_macsu_dual.c)
# P3-S3 RED contract: MPYA dst register-only row.
ti_c54x_add_unit_test(cpu_mpya_dst test_cpu_mpya_dst.c)
# P3-S3 RED contract: SQUR A,dst register-only row.
ti_c54x_add_unit_test(cpu_squr_adst test_cpu_squr_adst.c)
# P3-S3 RED contract: SQUR Smem square row.
ti_c54x_add_unit_test(cpu_squr_smem test_cpu_squr_smem.c)
# P3-S3 RED contract: SQURA/SQURS accumulate squares.
ti_c54x_add_unit_test(cpu_squr_acc test_cpu_squr_acc.c)
# P3-S3 RED contract: MPYU unsigned multiply.
ti_c54x_add_unit_test(cpu_mpyu_smem test_cpu_mpyu_smem.c)
# P3-S3 RED contract: MACA Smem accumulate-A row.
ti_c54x_add_unit_test(cpu_maca_smem test_cpu_maca_smem.c)
# P3-S111 RED contract: POLY Smem retirement.
ti_c54x_add_unit_test(cpu_poly_smem test_cpu_poly_smem.c)
# P3-S114 RED contract: LD parallel-MAC/MAS retirement.
ti_c54x_add_unit_test(cpu_ld_parallel test_cpu_ld_parallel.c)
# P3-S3 RED contract: MACA T,src register-only row.
ti_c54x_add_unit_test(cpu_maca_t test_cpu_maca_t.c)
# P3-S3 RED contract: MASA Smem subtract-A row.
ti_c54x_add_unit_test(cpu_masa_smem test_cpu_masa_smem.c)
# P3-S3 RED contract: MASA T,src register-only row.
ti_c54x_add_unit_test(cpu_masa_t test_cpu_masa_t.c)
# P3-S3 RED contract: MACP Smem,Pmad row.
ti_c54x_add_unit_test(cpu_macp_smem test_cpu_macp_smem.c)
# P3-S3 RED contract: MACD Smem,Pmad row.
ti_c54x_add_unit_test(cpu_macd_smem test_cpu_macd_smem.c)
