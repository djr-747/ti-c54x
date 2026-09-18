# Source ownership: src/isa/decode owns pure instruction classification and
# operand-field extraction.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/opcode_decode.c"
    "${CMAKE_CURRENT_LIST_DIR}/operand_field.c"
    "${CMAKE_CURRENT_LIST_DIR}/operand_layout.c"
    "${CMAKE_CURRENT_LIST_DIR}/operand_layout_sets.c"
    "${CMAKE_CURRENT_LIST_DIR}/operand_layout_sets_smem_lk.c"
    "${CMAKE_CURRENT_LIST_DIR}/operand_layout_sets_branch.c"
    "${CMAKE_CURRENT_LIST_DIR}/trusted_execution_packet.c"
)
