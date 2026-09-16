# Test ownership: tests/unit/isa/ops/multiply owns multiply-accumulate seam proofs.
# P3-S3 RED contract: multiply core.
$(eval $(call ti_c54x_add_test,multiply_core,isa/ops/multiply,test_multiply_core.c))
# P3-S3 RED contract: MAC Smem row.
$(eval $(call ti_c54x_add_test,mac_smem_ops,isa/ops/multiply,test_mac_smem_ops.c))
$(eval $(call ti_c54x_add_test,mpy_smem_ops,isa/ops/multiply,test_mpy_smem_ops.c))
