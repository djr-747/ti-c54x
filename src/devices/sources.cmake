# Source ownership: src/devices owns documented optional on-chip
# peripherals by device.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/software_wait_state_register.c"
)
