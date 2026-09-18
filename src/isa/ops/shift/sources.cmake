# Source ownership: src/isa/ops/shift owns SFTL/SFTC accumulator semantics.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/op_shift.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_sftc.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_sfta_acc.c")
