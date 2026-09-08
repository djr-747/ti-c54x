# Source ownership: src/isa/ops/load owns pure load semantic handlers.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/op_ld_status_fields.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_ld_smem.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_ld_smem_indexed.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_ld_smem_common.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_ldu_smem.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_ld_t_smem.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_ld_status_smem.c"
)
