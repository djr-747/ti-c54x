# Source ownership: src/cpu owns lifecycle, fetch collection, and orchestration.
list(APPEND ti_c54x_core_sources "${CMAKE_CURRENT_LIST_DIR}/cpu.c")
list(APPEND ti_c54x_core_sources "${CMAKE_CURRENT_LIST_DIR}/instruction_fetch.c")
