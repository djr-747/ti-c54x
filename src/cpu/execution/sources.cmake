# Source ownership: CPU execution owns step/run and the ordinary fetch loop.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/run.c"
    "${CMAKE_CURRENT_LIST_DIR}/repeat_activate.c"
)
