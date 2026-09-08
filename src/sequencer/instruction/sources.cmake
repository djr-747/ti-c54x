# Source ownership: src/sequencer/instruction owns persistent root memory
# instruction transactions.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/memory_instruction_candidate_validate.c"
    "${CMAKE_CURRENT_LIST_DIR}/memory_instruction_dual_candidate_validate.c"
    "${CMAKE_CURRENT_LIST_DIR}/memory_instruction_dual_commit.c"
    "${CMAKE_CURRENT_LIST_DIR}/memory_instruction_rmw.c"
    "${CMAKE_CURRENT_LIST_DIR}/memory_instruction_transaction.c"
    "${CMAKE_CURRENT_LIST_DIR}/memory_instruction_long32.c"
    "${CMAKE_CURRENT_LIST_DIR}/memory_instruction_dual.c"
    "${CMAKE_CURRENT_LIST_DIR}/memory_instruction_pair_dispatch.c"
    "${CMAKE_CURRENT_LIST_DIR}/dual_word16_read_transaction.c"
    "${CMAKE_CURRENT_LIST_DIR}/long32_pair_transaction.c"
    "${CMAKE_CURRENT_LIST_DIR}/long32_pair_transaction_validate.c"
)
