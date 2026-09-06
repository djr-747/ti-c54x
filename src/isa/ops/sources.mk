# Source ownership: src/isa/ops owns opcode semantic handlers and dispatch.
ti_c54x_core_sources += \
    src/isa/ops/op_nop.c \
    src/isa/ops/opcode_dispatch.c
include src/isa/ops/repeat/sources.mk
include src/isa/ops/load/sources.mk
include src/isa/ops/compare/sources.mk
