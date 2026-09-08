# Source ownership: src/foundation owns fixed-width arithmetic primitives.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/arithmetic40.c"
    "${CMAKE_CURRENT_LIST_DIR}/arithmetic40_addsub.c"
    "${CMAKE_CURRENT_LIST_DIR}/reverse_carry.c"
)
