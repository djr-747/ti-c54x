# Test ownership: tests/unit/cpu/memory owns CPU-facing data-endpoint
# lifecycle and resume/cancellation accounting proofs.
ti_c54x_add_unit_test(cpu_memory_lifecycle test_cpu_memory_lifecycle.c)
ti_c54x_add_unit_test(cpu_memory_pending test_cpu_memory_pending.c)
ti_c54x_add_unit_test(cpu_standard_indirect_modes
                      test_cpu_standard_indirect_modes.c)
ti_c54x_add_unit_test(cpu_indexed_standard_indirect
                      test_cpu_indexed_standard_indirect.c)
ti_c54x_add_unit_test(cpu_circular_standard_indirect
                      test_cpu_circular_standard_indirect.c)
