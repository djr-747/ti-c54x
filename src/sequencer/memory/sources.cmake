# Source ownership: src/sequencer/memory owns generic memory resolution.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/address_resolution.c"
    "${CMAKE_CURRENT_LIST_DIR}/dual_address_resolution.c"
    "${CMAKE_CURRENT_LIST_DIR}/mmr_access_plan.c"
)
