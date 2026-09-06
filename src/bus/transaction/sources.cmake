# Source ownership: src/bus/transaction owns the private resumable WORD16 reducer.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/word16_transaction.c"
    "${CMAKE_CURRENT_LIST_DIR}/word16_transaction_validate.c"
)
