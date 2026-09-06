# Test ownership: tests/unit/isa/ops/logical owns AND/OR/XOR accumulator
# semantic-handler and generated-dispatch proofs.
$(eval $(call ti_c54x_add_test,logical_accumulator,isa/ops/logical,test_logical_accumulator.c))
include tests/unit/isa/ops/logical/immediate/tests.mk
