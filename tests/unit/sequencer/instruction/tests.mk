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
# P3-S1 RED contract: MMR-routed root transaction retirement.
$(eval $(call ti_c54x_add_test,mmr_transaction,sequencer/instruction,test_mmr_memory_transaction.c))
# W47A RED contracts: canonical LONG32 pair shape and two-word lifecycle.
$(eval $(call ti_c54x_add_test,long32_pair_shape,sequencer/instruction,test_long32_pair_shape.c))
$(eval $(call ti_c54x_add_test,long32_pair_read_transaction,sequencer/instruction,test_long32_pair_read_transaction.c))
$(eval $(call ti_c54x_add_test,long32_pair_write_transaction,sequencer/instruction,test_long32_pair_write_transaction.c))
$(eval $(call ti_c54x_add_test,dual_word16_read_transaction,sequencer/instruction,test_dual_word16_read_transaction.c))
$(eval $(call ti_c54x_add_test,dual_memory_instruction_transaction,sequencer/instruction,test_dual_memory_instruction_transaction.c))
# P3-S3 RED contracts: DATA+PROGRAM pair reducer and root.
$(eval $(call ti_c54x_add_test,data_program_word16_read_transaction,sequencer/instruction,test_data_program_word16_read_transaction.c))
$(eval $(call ti_c54x_add_test,data_program_memory_instruction_transaction,sequencer/instruction,test_data_program_memory_instruction_transaction.c))
# W51B RED contracts: dual address commit persistence and atomic publication.
$(eval $(call ti_c54x_add_test,dual_memory_instruction_w51b_contract,sequencer/instruction,test_dual_memory_instruction_w51b_contract.c))
# W52B RED contract: dual-indirect MOD3 root commit validation.
$(eval $(call ti_c54x_add_test,dual_mod3_root,sequencer/instruction,test_dual_mod3_root.c))
# W47B1 RED contract: DLD/DST MOD13 LONG32 root failure lifecycle.
$(eval $(call ti_c54x_add_test,dld_dst_long32_failure_lifecycle,sequencer/instruction,test_dld_dst_long32_failure_lifecycle.c))
# W65A RED contract: stack-push WORD16 root transaction lifecycle.
$(eval $(call ti_c54x_add_test,stack_push_root,sequencer/instruction,test_stack_push_root.c))
# W66A RED contract: stack-pop WORD16 root transaction lifecycle.
$(eval $(call ti_c54x_add_test,stack_pop_root,sequencer/instruction,test_stack_pop_root.c))
# W67A RED contracts: delayed stack-pop/push WORD16 root lifecycles.
$(eval $(call ti_c54x_add_test,delayed_stack_pop_root,sequencer/instruction,test_delayed_stack_pop_root.c))
$(eval $(call ti_c54x_add_test,delayed_stack_push_root,sequencer/instruction,test_delayed_stack_push_root.c))
