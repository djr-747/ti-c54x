# Source ownership: src/sequencer owns ordered effect validation/application.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/address_effect_intrinsic.c"
    "${CMAKE_CURRENT_LIST_DIR}/address_effect_dependencies.c"
    "${CMAKE_CURRENT_LIST_DIR}/address_effect_validate.c"
    "${CMAKE_CURRENT_LIST_DIR}/address_effect_apply.c"
    "${CMAKE_CURRENT_LIST_DIR}/sequential_pc.c"
    "${CMAKE_CURRENT_LIST_DIR}/program_pc_bridge.c"
    "${CMAKE_CURRENT_LIST_DIR}/effect_transaction.c"
)
include(${CMAKE_CURRENT_LIST_DIR}/repeat/sources.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/status/sources.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/accumulator/sources.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/temporary/sources.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/memory/sources.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/instruction/sources.cmake)
