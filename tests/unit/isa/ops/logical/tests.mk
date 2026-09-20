# Test ownership: tests/unit/isa/ops/logical owns AND/OR/XOR accumulator
# semantic-handler and generated-dispatch proofs.
$(eval $(call ti_c54x_add_test,logical_accumulator,isa/ops/logical,test_logical_accumulator.c))
# P3-S15 RED contract: AND Smem.
$(eval $(call ti_c54x_add_test,and_smem_ops,isa/ops/logical,test_and_smem.c))
# P3-S16 RED contract: OR Smem.
$(eval $(call ti_c54x_add_test,or_smem_ops,isa/ops/logical,test_or_smem.c))
# P3-S17 RED contract: XOR Smem.
$(eval $(call ti_c54x_add_test,xor_smem_ops,isa/ops/logical,test_xor_smem.c))
# P3-S81 RED contract: OR absolute Smem.
$(eval $(call ti_c54x_add_test,or_smem_absolute_ops,isa/ops/logical,test_or_smem_absolute.c))
include tests/unit/isa/ops/logical/immediate/tests.mk
