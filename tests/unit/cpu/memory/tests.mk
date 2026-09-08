# Test ownership: tests/unit/cpu/memory owns CPU-facing data-endpoint
# lifecycle and resume/cancellation accounting proofs.
$(eval $(call ti_c54x_add_test,cpu_memory_lifecycle,cpu/memory,test_cpu_memory_lifecycle.c))
$(eval $(call ti_c54x_add_test,cpu_memory_pending,cpu/memory,test_cpu_memory_pending.c))
$(eval $(call ti_c54x_add_test,cpu_standard_indirect_modes,cpu/memory,test_cpu_standard_indirect_modes.c))
$(eval $(call ti_c54x_add_test,cpu_indexed_standard_indirect,cpu/memory,test_cpu_indexed_standard_indirect.c))
$(eval $(call ti_c54x_add_test,cpu_circular_standard_indirect,cpu/memory,test_cpu_circular_standard_indirect.c))
