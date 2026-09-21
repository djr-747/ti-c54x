# Source ownership: src/isa/ops/stack owns stack-pointer effect construction.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/op_frame_k.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_trap.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_pshd_direct.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_popd_direct.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_pshm_direct.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_popm_direct.c"
)
