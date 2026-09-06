# Source ownership: src/isa/ops owns opcode semantic handlers and dispatch.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/op_nop.c"
    "${CMAKE_CURRENT_LIST_DIR}/opcode_dispatch.c"
)
include(${CMAKE_CURRENT_LIST_DIR}/repeat/sources.cmake)
