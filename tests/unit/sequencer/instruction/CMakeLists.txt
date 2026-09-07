# Test ownership: commit file owns WAIT/READY architectural publication.
ti_c54x_add_unit_test(memory_instruction_commit
                      test_memory_instruction_commit.c)
# Test ownership: lifecycle file owns preflight, identity, reset/cancel, faults.
ti_c54x_add_unit_test(memory_instruction_lifecycle
                      test_memory_instruction_lifecycle.c)
