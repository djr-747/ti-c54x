# Test ownership: tests/unit/isa/ops/multiply owns multiply-accumulate seam proofs.
# P3-S3 RED contract: multiply core.
$(eval $(call ti_c54x_add_test,multiply_core,isa/ops/multiply,test_multiply_core.c))
# P3-S3 RED contract: MAC Smem row.
$(eval $(call ti_c54x_add_test,mac_smem_ops,isa/ops/multiply,test_mac_smem_ops.c))
$(eval $(call ti_c54x_add_test,mpy_smem_ops,isa/ops/multiply,test_mpy_smem_ops.c))
$(eval $(call ti_c54x_add_test,mpya_smem_ops,isa/ops/multiply,test_mpya_smem_ops.c))
$(eval $(call ti_c54x_add_test,mas_smem_ops,isa/ops/multiply,test_mas_smem_ops.c))
$(eval $(call ti_c54x_add_test,mac_dual_ops,isa/ops/multiply,test_mac_dual_ops.c))
$(eval $(call ti_c54x_add_test,mpy_dual_ops,isa/ops/multiply,test_mpy_dual_ops.c))
$(eval $(call ti_c54x_add_test,mas_dual_ops,isa/ops/multiply,test_mas_dual_ops.c))
# P3-S3 RED contract: MPYA dst register-only row.
$(eval $(call ti_c54x_add_test,mpya_dst_ops,isa/ops/multiply,test_mpya_dst_ops.c))
# P3-S3 RED contract: SQUR A,dst register-only row.
$(eval $(call ti_c54x_add_test,squr_adst_ops,isa/ops/multiply,test_squr_adst_ops.c))
# P3-S3 RED contract: SQUR Smem square row.
$(eval $(call ti_c54x_add_test,squr_smem_ops,isa/ops/multiply,test_squr_smem_ops.c))
# P3-S3 RED contract: SQURA/SQURS accumulate squares.
$(eval $(call ti_c54x_add_test,squr_acc_ops,isa/ops/multiply,test_squr_acc_ops.c))
# P3-S3 RED contract: MPYU unsigned multiply.
$(eval $(call ti_c54x_add_test,mpyu_smem_ops,isa/ops/multiply,test_mpyu_smem_ops.c))
# P3-S3 RED contract: MACA Smem accumulate-A row.
$(eval $(call ti_c54x_add_test,maca_smem_ops,isa/ops/multiply,test_maca_smem_ops.c))
# P3-S3 RED contract: MACA T,src register-only row.
$(eval $(call ti_c54x_add_test,maca_t_ops,isa/ops/multiply,test_maca_t_ops.c))
# P3-S3 RED contract: MACP Smem,Pmad row.
$(eval $(call ti_c54x_add_test,macp_smem_ops,isa/ops/multiply,test_macp_smem_ops.c))
# P3-S3 RED contract: MACD Smem,Pmad row.
$(eval $(call ti_c54x_add_test,macd_smem_ops,isa/ops/multiply,test_macd_smem_ops.c))
