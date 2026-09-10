# Source ownership: src/isa/ops/branch owns pure branch semantic handlers.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/op_b_pmad.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_bc_pmad.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_bcd.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_bd.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_branch_cond_common.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_call_pmad.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_calld.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_ret.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_retd.c"
)
