# Source ownership: src/isa/ops owns opcode semantic handlers and dispatch.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/op_nop.c"
    "${CMAKE_CURRENT_LIST_DIR}/opcode_dispatch.c"
)
include(${CMAKE_CURRENT_LIST_DIR}/dispatch/sources.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/repeat/sources.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/multiply/sources.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/load/sources.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/store/sources.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/status/sources.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/compare/sources.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/accumulator/sources.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/rotate/sources.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/shift/sources.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/logical/sources.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/arithmetic/sources.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/memory/sources.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/branch/sources.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/port/sources.cmake)
