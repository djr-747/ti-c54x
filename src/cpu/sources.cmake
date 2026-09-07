# Source ownership: src/cpu owns lifecycle, fetch collection, and orchestration.
list(APPEND ti_c54x_core_sources "${CMAKE_CURRENT_LIST_DIR}/cpu.c")
list(APPEND ti_c54x_core_sources "${CMAKE_CURRENT_LIST_DIR}/instruction_fetch.c")
include(${CMAKE_CURRENT_LIST_DIR}/input/sources.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/memory/sources.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/execution/sources.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/trace/sources.cmake)
