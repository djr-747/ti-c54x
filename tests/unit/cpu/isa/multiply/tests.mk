# Test ownership: this leaf owns only public-CPU MAC retirement proofs.
$(eval $(call ti_c54x_add_test,cpu_mac_smem,cpu/isa/multiply,test_cpu_mac_smem.c))
$(eval $(call ti_c54x_add_test,cpu_mpy_smem,cpu/isa/multiply,test_cpu_mpy_smem.c))
$(eval $(call ti_c54x_add_test,cpu_mpya_smem,cpu/isa/multiply,test_cpu_mpya_smem.c))
$(eval $(call ti_c54x_add_test,cpu_mas_smem,cpu/isa/multiply,test_cpu_mas_smem.c))
$(eval $(call ti_c54x_add_test,cpu_mac_dual,cpu/isa/multiply,test_cpu_mac_dual.c))
$(eval $(call ti_c54x_add_test,cpu_mpy_dual,cpu/isa/multiply,test_cpu_mpy_dual.c))
$(eval $(call ti_c54x_add_test,cpu_mas_dual,cpu/isa/multiply,test_cpu_mas_dual.c))
# P3-S3 RED contract: MACSU Xmem,Ymem dual rows.
$(eval $(call ti_c54x_add_test,cpu_macsu_dual,cpu/isa/multiply,test_cpu_macsu_dual.c))
# P3-S3 RED contract: MPYA dst register-only row.
$(eval $(call ti_c54x_add_test,cpu_mpya_dst,cpu/isa/multiply,test_cpu_mpya_dst.c))
# P3-S3 RED contract: SQUR A,dst register-only row.
$(eval $(call ti_c54x_add_test,cpu_squr_adst,cpu/isa/multiply,test_cpu_squr_adst.c))
# P3-S3 RED contract: SQUR Smem square row.
$(eval $(call ti_c54x_add_test,cpu_squr_smem,cpu/isa/multiply,test_cpu_squr_smem.c))
# P3-S3 RED contract: SQURA/SQURS accumulate squares.
$(eval $(call ti_c54x_add_test,cpu_squr_acc,cpu/isa/multiply,test_cpu_squr_acc.c))
# P3-S3 RED contract: MPYU unsigned multiply.
$(eval $(call ti_c54x_add_test,cpu_mpyu_smem,cpu/isa/multiply,test_cpu_mpyu_smem.c))
# P3-S3 RED contract: MACA Smem accumulate-A row.
$(eval $(call ti_c54x_add_test,cpu_maca_smem,cpu/isa/multiply,test_cpu_maca_smem.c))
# P3-S3 RED contract: MACA T,src register-only row.
$(eval $(call ti_c54x_add_test,cpu_maca_t,cpu/isa/multiply,test_cpu_maca_t.c))
# P3-S3 RED contract: MACP Smem,Pmad row.
$(eval $(call ti_c54x_add_test,cpu_macp_smem,cpu/isa/multiply,test_cpu_macp_smem.c))
# P3-S3 RED contract: MACD Smem,Pmad row.
$(eval $(call ti_c54x_add_test,cpu_macd_smem,cpu/isa/multiply,test_cpu_macd_smem.c))
