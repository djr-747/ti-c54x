# Source ownership: isa/ops/compare owns MAX/MIN and CMPR semantics.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/op_compare.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_cmpr.c")
