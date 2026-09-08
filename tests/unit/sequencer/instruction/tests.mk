# Test ownership: commit file owns WAIT/READY architectural publication.
$(eval $(call ti_c54x_add_test,memory_instruction_commit,sequencer/instruction,test_memory_instruction_commit.c))
# Test ownership: lifecycle file owns preflight, identity, reset/cancel, faults.
$(eval $(call ti_c54x_add_test,memory_instruction_lifecycle,sequencer/instruction,test_memory_instruction_lifecycle.c))
# Test ownership: modes file owns MOD0--MOD3 transaction publication rules.
$(eval $(call ti_c54x_add_test,memory_instruction_modes,sequencer/instruction,test_memory_instruction_modes.c))
$(eval $(call ti_c54x_add_test,indexed_word16_memory_transaction,sequencer/instruction,test_indexed_word16_memory_transaction.c))
$(eval $(call ti_c54x_add_test,circular_word16_memory_transaction,sequencer/instruction,test_circular_word16_memory_transaction.c))
$(eval $(call ti_c54x_add_test,reverse_carry_word16_memory_transaction,sequencer/instruction,test_reverse_carry_word16_memory_transaction.c))
$(eval $(call ti_c54x_add_test,word16_rmw_memory_transaction,sequencer/instruction,test_word16_rmw_memory_transaction.c))
# W47A RED contracts: canonical LONG32 pair shape and two-word lifecycle.
$(eval $(call ti_c54x_add_test,long32_pair_shape,sequencer/instruction,test_long32_pair_shape.c))
$(eval $(call ti_c54x_add_test,long32_pair_read_transaction,sequencer/instruction,test_long32_pair_read_transaction.c))
$(eval $(call ti_c54x_add_test,long32_pair_write_transaction,sequencer/instruction,test_long32_pair_write_transaction.c))
# W47B1 RED contract: DLD/DST MOD13 LONG32 root failure lifecycle.
$(eval $(call ti_c54x_add_test,dld_dst_long32_failure_lifecycle,sequencer/instruction,test_dld_dst_long32_failure_lifecycle.c))
