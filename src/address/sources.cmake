# Source ownership: src/address owns effective-address and operand planning.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/data_operand.c"
    "${CMAKE_CURRENT_LIST_DIR}/direct_dp.c"
    "${CMAKE_CURRENT_LIST_DIR}/direct_sp.c"
    "${CMAKE_CURRENT_LIST_DIR}/indirect_single.c"
    "${CMAKE_CURRENT_LIST_DIR}/indirect_indexed.c"
    "${CMAKE_CURRENT_LIST_DIR}/indirect_long_offset.c"
    "${CMAKE_CURRENT_LIST_DIR}/indirect_reverse_carry.c"
    "${CMAKE_CURRENT_LIST_DIR}/indirect_circular.c"
)
