# Source ownership: src/sequencer/stack owns stack-pointer effect application.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/sp_write_apply.c"
)
