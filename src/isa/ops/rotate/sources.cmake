# Source ownership: src/isa/ops/rotate owns ROL/ROR/ROLTC accumulator semantics.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/op_rotate.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_roltc.c")
