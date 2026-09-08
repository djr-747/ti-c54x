# Test ownership: tests/unit/cpu/memory owns CPU-facing data-endpoint
# lifecycle and resume/cancellation accounting proofs.
ti_c54x_add_unit_test(cpu_memory_lifecycle test_cpu_memory_lifecycle.c)
ti_c54x_add_unit_test(cpu_memory_pending test_cpu_memory_pending.c)
ti_c54x_add_unit_test(cpu_standard_indirect_modes
                      test_cpu_standard_indirect_modes.c)
