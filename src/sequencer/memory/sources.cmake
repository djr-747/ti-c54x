# Source ownership: src/sequencer/memory owns generic memory resolution.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/address_resolution.c"
    "${CMAKE_CURRENT_LIST_DIR}/address_resolution_absolute_data.c"
    "${CMAKE_CURRENT_LIST_DIR}/address_resolution_long32_absolute_store.c"
    "${CMAKE_CURRENT_LIST_DIR}/address_resolution_long32_mod13.c"
    "${CMAKE_CURRENT_LIST_DIR}/dual_address_resolution.c"
    "${CMAKE_CURRENT_LIST_DIR}/data_program_address_resolution.c"
    "${CMAKE_CURRENT_LIST_DIR}/mmr_access_plan.c"
    "${CMAKE_CURRENT_LIST_DIR}/port_access_plan.c"
    "${CMAKE_CURRENT_LIST_DIR}/long32_access_plan.c"
)
