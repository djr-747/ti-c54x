# Source ownership: src/isa/catalog owns the declarative opcode identity,
# immutable metadata, and direct first-word lookup generated from one catalog.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/opcode_catalog.c"
)
