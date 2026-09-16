# Source ownership: src/bus/mmr owns the private BK memory-mapped route.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/bk_route.c"
    "${CMAKE_CURRENT_LIST_DIR}/mmr_route.c"
)
