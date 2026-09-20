# Source ownership: src/isa/ops/repeat owns immediate-RPT effect construction.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/op_rpt_immediate.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_rpt_k.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_rpt_lk.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_rpt_smem.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_rptb.c"
)
