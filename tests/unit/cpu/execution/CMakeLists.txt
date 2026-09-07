# Test ownership: tests/unit/cpu/execution owns the trusted packet execution
# loop and its public stop/retirement boundaries.
ti_c54x_add_unit_test(trusted_instruction_path test_trusted_instruction_path.c)
