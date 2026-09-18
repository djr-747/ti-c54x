# Source ownership: arithmetic accumulator semantics.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/op_arithmetic.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_dual_arithmetic.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_add_lk.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_add_smem.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_add_smem_shift.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_add_src_asm.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_sub_lk.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_sub_smem.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_sub_smem_shift.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_sub_src_asm.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_subb_smem.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_subs_smem.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_add_xmem.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_sub_xmem.c"
)
