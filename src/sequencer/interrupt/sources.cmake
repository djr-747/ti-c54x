# Source ownership: src/sequencer/interrupt owns repeat-pending request state.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/repeat_pending_request.c"
)
