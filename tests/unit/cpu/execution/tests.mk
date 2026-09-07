# Test ownership: tests/unit/cpu/execution owns the trusted packet execution
# loop and its public stop/retirement boundaries.
$(eval $(call ti_c54x_add_test,trusted_instruction_path,cpu/execution,test_trusted_instruction_path.c))
