# Source ownership: src/isa/ops owns opcode semantic handlers and dispatch.
ti_c54x_core_sources += \
    src/isa/ops/op_nop.c \
    src/isa/ops/opcode_dispatch.c
include src/isa/ops/dispatch/sources.mk
include src/isa/ops/repeat/sources.mk
include src/isa/ops/multiply/sources.mk
include src/isa/ops/load/sources.mk
include src/isa/ops/store/sources.mk
include src/isa/ops/status/sources.mk
include src/isa/ops/compare/sources.mk
include src/isa/ops/accumulator/sources.mk
include src/isa/ops/rotate/sources.mk
include src/isa/ops/shift/sources.mk
include src/isa/ops/logical/sources.mk
include src/isa/ops/arithmetic/sources.mk
include src/isa/ops/memory/sources.mk
include src/isa/ops/branch/sources.mk
include src/isa/ops/port/sources.mk
