# Test ownership: commit file owns WAIT/READY architectural publication.
$(eval $(call ti_c54x_add_test,memory_instruction_commit,sequencer/instruction,test_memory_instruction_commit.c))
# Test ownership: lifecycle file owns preflight, identity, reset/cancel, faults.
$(eval $(call ti_c54x_add_test,memory_instruction_lifecycle,sequencer/instruction,test_memory_instruction_lifecycle.c))
# Test ownership: modes file owns MOD0--MOD3 transaction publication rules.
$(eval $(call ti_c54x_add_test,memory_instruction_modes,sequencer/instruction,test_memory_instruction_modes.c))
$(eval $(call ti_c54x_add_test,indexed_word16_memory_transaction,sequencer/instruction,test_indexed_word16_memory_transaction.c))
$(eval $(call ti_c54x_add_test,circular_word16_memory_transaction,sequencer/instruction,test_circular_word16_memory_transaction.c))
