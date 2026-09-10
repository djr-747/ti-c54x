# Source ownership: src/sequencer/delayed_branch owns the pure delay-countdown value.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/delayed_branch.c"
)
