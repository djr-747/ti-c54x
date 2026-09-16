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
