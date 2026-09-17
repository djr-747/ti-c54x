# Test ownership: tests/unit/isa/ops/multiply owns multiply-accumulate seam proofs.
# P3-S3 RED contract: multiply core.
ti_c54x_add_unit_test(multiply_core test_multiply_core.c)
# P3-S3 RED contract: MAC Smem row.
ti_c54x_add_unit_test(mac_smem_ops test_mac_smem_ops.c)
ti_c54x_add_unit_test(mpy_smem_ops test_mpy_smem_ops.c)
ti_c54x_add_unit_test(mpya_smem_ops test_mpya_smem_ops.c)
ti_c54x_add_unit_test(mas_smem_ops test_mas_smem_ops.c)
# P3-S3 RED contract: MAC Xmem,Ymem dual rows.
ti_c54x_add_unit_test(mac_dual_ops test_mac_dual_ops.c)
# P3-S3 RED contract: MPY Xmem,Ymem dual rows.
ti_c54x_add_unit_test(mpy_dual_ops test_mpy_dual_ops.c)
# P3-S3 RED contract: MAS Xmem,Ymem dual rows.
ti_c54x_add_unit_test(mas_dual_ops test_mas_dual_ops.c)
# P3-S3 RED contract: MACSU Xmem,Ymem dual rows.
ti_c54x_add_unit_test(macsu_dual_ops test_macsu_dual_ops.c)
# P3-S3 RED contract: MPYA dst register-only row.
ti_c54x_add_unit_test(mpya_dst_ops test_mpya_dst_ops.c)
# P3-S3 RED contract: SQUR A,dst register-only row.
ti_c54x_add_unit_test(squr_adst_ops test_squr_adst_ops.c)
# P3-S3 RED contract: SQUR Smem square row.
ti_c54x_add_unit_test(squr_smem_ops test_squr_smem_ops.c)
# P3-S3 RED contract: SQURA/SQURS accumulate squares.
ti_c54x_add_unit_test(squr_acc_ops test_squr_acc_ops.c)
# P3-S3 RED contract: MPYU unsigned multiply.
ti_c54x_add_unit_test(mpyu_smem_ops test_mpyu_smem_ops.c)
# P3-S3 RED contract: MACA Smem accumulate-A row.
ti_c54x_add_unit_test(maca_smem_ops test_maca_smem_ops.c)
# P3-S3 RED contract: MACA T,src register-only row.
ti_c54x_add_unit_test(maca_t_ops test_maca_t_ops.c)
# P3-S3 RED contract: MASA Smem subtract-A row.
ti_c54x_add_unit_test(masa_smem_ops test_masa_smem_ops.c)
# P3-S3 RED contract: MASA T,src register-only row.
ti_c54x_add_unit_test(masa_t_ops test_masa_t_ops.c)
# P3-S3 RED contract: MACP Smem,Pmad row.
ti_c54x_add_unit_test(macp_smem_ops test_macp_smem_ops.c)
# P3-S3 RED contract: MACD Smem,Pmad row.
ti_c54x_add_unit_test(macd_smem_ops test_macd_smem_ops.c)
