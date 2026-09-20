# Source ownership: src/isa/ops/logical owns AND/OR/XOR accumulator handlers.
ti_c54x_core_sources += src/isa/ops/logical/op_logical.c \
    src/isa/ops/logical/op_and_smem.c \
    src/isa/ops/logical/op_or_smem.c \
    src/isa/ops/logical/op_or_smem_absolute.c \
    src/isa/ops/logical/op_xor_smem.c
include src/isa/ops/logical/immediate/sources.mk
