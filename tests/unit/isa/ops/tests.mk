# Test ownership: tests/unit/isa/ops owns NOP semantic and dispatch seams.
$(eval $(call ti_c54x_add_test,nop,isa/ops,test_nop.c))
include tests/unit/isa/ops/compare/tests.mk
include tests/unit/isa/ops/accumulator/tests.mk
include tests/unit/isa/ops/load/tests.mk
include tests/unit/isa/ops/repeat/tests.mk
