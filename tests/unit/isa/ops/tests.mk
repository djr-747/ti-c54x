# Test ownership: tests/unit/isa/ops owns NOP semantic and dispatch seams.
$(eval $(call ti_c54x_add_test,nop,isa/ops,test_nop.c))
include tests/unit/isa/ops/dispatch/tests.mk
include tests/unit/isa/ops/compare/tests.mk
include tests/unit/isa/ops/accumulator/tests.mk
include tests/unit/isa/ops/rotate/tests.mk
include tests/unit/isa/ops/shift/tests.mk
include tests/unit/isa/ops/logical/tests.mk
include tests/unit/isa/ops/load/tests.mk
include tests/unit/isa/ops/memory/tests.mk
include tests/unit/isa/ops/store/tests.mk
include tests/unit/isa/ops/status/tests.mk
include tests/unit/isa/ops/repeat/tests.mk
include tests/unit/isa/ops/arithmetic/tests.mk
include tests/unit/isa/ops/multiply/tests.mk
include tests/unit/isa/ops/branch/tests.mk
include tests/unit/isa/ops/port/tests.mk
include tests/unit/isa/ops/stack/tests.mk
include tests/unit/isa/ops/auxiliary/tests.mk
