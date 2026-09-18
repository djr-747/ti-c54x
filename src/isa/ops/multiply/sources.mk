# Source ownership: src/isa/ops/multiply owns multiply-accumulate core construction.
ti_c54x_core_sources += \
    src/isa/ops/multiply/op_multiply.c \
    src/isa/ops/multiply/op_mac_lk.c \
    src/isa/ops/multiply/op_mac_smem.c \
    src/isa/ops/multiply/op_mpy_smem.c \
    src/isa/ops/multiply/op_mpya_smem.c \
    src/isa/ops/multiply/op_mas_smem.c \
    src/isa/ops/multiply/op_mac_dual.c \
    src/isa/ops/multiply/op_macsu_dual.c \
    src/isa/ops/multiply/op_mpy_dual.c \
    src/isa/ops/multiply/op_mas_dual.c \
    src/isa/ops/multiply/op_mpya_dst.c \
    src/isa/ops/multiply/op_squr_smem.c \
    src/isa/ops/multiply/op_squr_adst.c \
    src/isa/ops/multiply/op_squr_acc.c \
    src/isa/ops/multiply/op_mpyu_smem.c \
    src/isa/ops/multiply/op_maca_smem.c \
    src/isa/ops/multiply/op_maca_t.c \
    src/isa/ops/multiply/op_masa_smem.c \
    src/isa/ops/multiply/op_masa_t.c \
    src/isa/ops/multiply/op_macd_smem.c \
    src/isa/ops/multiply/op_macp_smem.c \
    src/isa/ops/multiply/op_mpy_lk.c \
    src/isa/ops/multiply/op_mpy_smem_lk.c \
    src/isa/ops/multiply/op_mac_smem_lk.c
