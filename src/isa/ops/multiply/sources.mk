# Source ownership: src/isa/ops/multiply owns multiply-accumulate core construction.
ti_c54x_core_sources += \
    src/isa/ops/multiply/op_multiply.c \
    src/isa/ops/multiply/op_mac_smem.c \
    src/isa/ops/multiply/op_mpy_smem.c \
    src/isa/ops/multiply/op_mpya_smem.c \
    src/isa/ops/multiply/op_mas_smem.c \
    src/isa/ops/multiply/op_mac_dual.c \
    src/isa/ops/multiply/op_mpy_dual.c
