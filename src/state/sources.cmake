# Source ownership: src/state owns architectural register storage/accessors.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/arch_state.c"
    "${CMAKE_CURRENT_LIST_DIR}/block_size_register.c"
)
include(${CMAKE_CURRENT_LIST_DIR}/status/sources.cmake)
