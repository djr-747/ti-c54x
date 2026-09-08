# Source ownership: src/sequencer/instruction owns persistent root memory
# instruction transactions.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/memory_instruction_candidate_validate.c"
    "${CMAKE_CURRENT_LIST_DIR}/memory_instruction_transaction.c"
)
