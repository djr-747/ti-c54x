# Source ownership: src/isa/ops/load owns immediate status-field handlers.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/op_ld_status_fields.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_ld_smem.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_ldu_smem.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_ld_t_smem.c"
)
