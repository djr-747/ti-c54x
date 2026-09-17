# Source ownership: isa/ops/port owns PORT I/O-pair opcode semantics.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/op_portr.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_portw.c"
)
