# Test ownership: tests/unit/isa aggregates decode and opcode-operation tests
# while each nested owner keeps its own source registration.
include tests/unit/isa/decode/tests.mk
include tests/unit/isa/catalog/tests.mk
include tests/unit/isa/ops/tests.mk
