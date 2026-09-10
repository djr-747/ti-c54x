# Source ownership: src/isa/ops/branch owns pure branch semantic handlers.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/op_b_pmad.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_bc_pmad.c"
)
