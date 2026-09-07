# Test ownership: commit file owns WAIT/READY architectural publication.
$(eval $(call ti_c54x_add_test,memory_instruction_commit,sequencer/instruction,test_memory_instruction_commit.c))
# Test ownership: lifecycle file owns preflight, identity, reset/cancel, faults.
$(eval $(call ti_c54x_add_test,memory_instruction_lifecycle,sequencer/instruction,test_memory_instruction_lifecycle.c))
