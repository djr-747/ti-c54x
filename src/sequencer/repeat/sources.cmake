# Source ownership: src/sequencer/repeat owns repeat admission policy leaves.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/repeat_cardinality.c"
    "${CMAKE_CURRENT_LIST_DIR}/repeat_body_eligibility.c"
    "${CMAKE_CURRENT_LIST_DIR}/repeat_request_gate.c"
    "${CMAKE_CURRENT_LIST_DIR}/repeat_progress.c"
    "${CMAKE_CURRENT_LIST_DIR}/repeat_single.c"
)
