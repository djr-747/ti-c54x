# Source ownership: src/isa/ops/logical owns AND/OR/XOR accumulator handlers.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/op_logical.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_and_smem.c")
include(${CMAKE_CURRENT_LIST_DIR}/immediate/sources.cmake)
