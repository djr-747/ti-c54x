# Source ownership: src/isa/ops/dispatch owns immutable trusted bindings and
# packet-only semantic dispatch.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/trusted_opcode_binding.c"
)
