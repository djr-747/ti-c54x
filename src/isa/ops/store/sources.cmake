# Source ownership: isa/ops/store owns store opcode semantics.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/op_stl_smem.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_st_immediate_smem.c"
    "${CMAKE_CURRENT_LIST_DIR}/op_st_t_smem.c")
