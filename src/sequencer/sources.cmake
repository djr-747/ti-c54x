# Source ownership: src/sequencer owns ordered effect validation/application.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/address_effect_intrinsic.c"
    "${CMAKE_CURRENT_LIST_DIR}/address_effect_dependencies.c"
    "${CMAKE_CURRENT_LIST_DIR}/address_effect_validate.c"
    "${CMAKE_CURRENT_LIST_DIR}/address_effect_apply.c"
    "${CMAKE_CURRENT_LIST_DIR}/sequential_pc.c"
    "${CMAKE_CURRENT_LIST_DIR}/program_pc_bridge.c"
)
