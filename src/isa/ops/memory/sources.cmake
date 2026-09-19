# Source ownership: src/isa/ops/memory owns generic memory effect validation.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/memory_effect.c"
    "${CMAKE_CURRENT_LIST_DIR}/memory_effect_long32_single.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_atomic_logical_memory.c"
)
