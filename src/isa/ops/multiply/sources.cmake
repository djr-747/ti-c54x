# Source ownership: src/isa/ops/multiply owns multiply-accumulate core construction.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/op_multiply.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_mac_smem.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_mpy_smem.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_mpya_smem.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_mas_smem.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_mac_dual.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_mpy_dual.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_mas_dual.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_mpya_dst.c"
)
