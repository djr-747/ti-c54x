# Source ownership: src/isa/ops/stack owns stack-pointer effect construction.
ti_c54x_core_sources += \
    src/isa/ops/stack/op_frame_k.c \
    src/isa/ops/stack/op_trap.c \
    src/isa/ops/stack/op_pshd_direct.c \
    src/isa/ops/stack/op_popd_direct.c \
    src/isa/ops/stack/op_pshm_direct.c \
    src/isa/ops/stack/op_popm_direct.c
