# Test ownership: tests/unit/isa/ops/multiply owns multiply-accumulate seam proofs.
# P3-S3 RED contract: multiply core.
$(eval $(call ti_c54x_add_test,multiply_core,isa/ops/multiply,test_multiply_core.c))
