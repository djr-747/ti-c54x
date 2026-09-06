# Test ownership: tests/unit/bus/transaction owns WORD16 transaction coverage.
$(eval $(call ti_c54x_add_test,bus_word16_transaction,bus/transaction,test_word16_transaction.c))
$(eval $(call ti_c54x_add_test,bus_word16_transaction_contract,bus/transaction,test_word16_transaction_contract.c))
