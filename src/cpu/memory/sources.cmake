# Source ownership: src/cpu/memory owns the CPU-facing pending root seam.
list(APPEND ti_c54x_core_sources "${CMAKE_CURRENT_LIST_DIR}/pending_work.c")
