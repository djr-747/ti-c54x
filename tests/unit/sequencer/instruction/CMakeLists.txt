# Test ownership: commit file owns WAIT/READY architectural publication.
ti_c54x_add_unit_test(memory_instruction_commit
                      test_memory_instruction_commit.c)
# Test ownership: lifecycle file owns preflight, identity, reset/cancel, faults.
ti_c54x_add_unit_test(memory_instruction_lifecycle
                      test_memory_instruction_lifecycle.c)
# Test ownership: modes file owns MOD0--MOD3 transaction publication rules.
ti_c54x_add_unit_test(memory_instruction_modes
                      test_memory_instruction_modes.c)
ti_c54x_add_unit_test(indexed_word16_memory_transaction
                      test_indexed_word16_memory_transaction.c)
ti_c54x_add_unit_test(circular_word16_memory_transaction
                      test_circular_word16_memory_transaction.c)
ti_c54x_add_unit_test(reverse_carry_word16_memory_transaction
                      test_reverse_carry_word16_memory_transaction.c)
ti_c54x_add_unit_test(word16_rmw_memory_transaction
                      test_word16_rmw_memory_transaction.c)
# P3-S32 RED contract: RMW-ADD 16-bit reducer vectors.
ti_c54x_add_unit_test(rmw_add_transform
                      test_rmw_add_transform.c)
# P3-S32 RED contract: RMW-ADD root commit with sticky C/OVA.
ti_c54x_add_unit_test(rmw_add_memory_transaction
                      test_rmw_add_memory_transaction.c)
# P3-S1 RED contract: MMR-routed root transaction retirement.
ti_c54x_add_unit_test(mmr_transaction
                      test_mmr_memory_transaction.c)
# P3-S6 RED contract: PORT root transaction hook.
ti_c54x_add_unit_test(port_transaction
                      test_port_memory_transaction.c)
# W47A RED contracts: canonical LONG32 pair shape and two-word lifecycle.
ti_c54x_add_unit_test(long32_pair_shape
                      test_long32_pair_shape.c)
ti_c54x_add_unit_test(long32_pair_read_transaction
                      test_long32_pair_read_transaction.c)
ti_c54x_add_unit_test(long32_pair_write_transaction
                      test_long32_pair_write_transaction.c)
ti_c54x_add_unit_test(dual_word16_read_transaction
                      test_dual_word16_read_transaction.c)
ti_c54x_add_unit_test(dual_memory_instruction_transaction
                      test_dual_memory_instruction_transaction.c)
# P3-S3 RED contracts: DATA+PROGRAM pair reducer and root.
ti_c54x_add_unit_test(data_program_word16_read_transaction
                      test_data_program_word16_read_transaction.c)
ti_c54x_add_unit_test(data_program_memory_instruction_transaction
                      test_data_program_memory_instruction_transaction.c)
# P3-S3 RED contract: MACD Smem delay write-back root stage.
ti_c54x_add_unit_test(data_program_delay_write
                      test_data_program_delay_write.c)
# W51B RED contracts: dual address commit persistence and atomic publication.
ti_c54x_add_unit_test(dual_memory_instruction_w51b_contract
                      test_dual_memory_instruction_w51b_contract.c)
# W52B RED contract: dual-indirect MOD3 root commit validation.
ti_c54x_add_unit_test(dual_mod3_root
                      test_dual_mod3_root.c)
# W47B1 RED contract: DLD/DST MOD13 LONG32 root failure lifecycle.
ti_c54x_add_unit_test(dld_dst_long32_failure_lifecycle
                      test_dld_dst_long32_failure_lifecycle.c)
# W65A RED contract: stack-push WORD16 root transaction lifecycle.
ti_c54x_add_unit_test(stack_push_root
                      test_stack_push_root.c)
# W66A RED contract: stack-pop WORD16 root transaction lifecycle.
ti_c54x_add_unit_test(stack_pop_root
                      test_stack_pop_root.c)
# W67A RED contracts: delayed stack-pop/push WORD16 root lifecycles.
ti_c54x_add_unit_test(delayed_stack_pop_root
                      test_delayed_stack_pop_root.c)
ti_c54x_add_unit_test(delayed_stack_push_root
                      test_delayed_stack_push_root.c)
# P3-S5 RED contract: accumulator-targeted push root lifecycle.
ti_c54x_add_unit_test(accumulator_push_root
                      test_accumulator_push_root.c)
