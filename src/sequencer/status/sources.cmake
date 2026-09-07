# Source ownership: src/sequencer/status owns status effect application.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/field_effect_apply.c"
    "${CMAKE_CURRENT_LIST_DIR}/bit_effect_apply.c"
)
