# Source ownership: src/isa/ops/multiply owns multiply-accumulate core construction.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/op_multiply.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_mac_smem.c"
)
