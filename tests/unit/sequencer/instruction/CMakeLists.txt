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
