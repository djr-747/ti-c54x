# Test ownership: tests/unit/cpu/memory owns CPU-facing data-endpoint
# lifecycle and resume/cancellation accounting proofs.
$(eval $(call ti_c54x_add_test,cpu_memory_lifecycle,cpu/memory,test_cpu_memory_lifecycle.c))
$(eval $(call ti_c54x_add_test,cpu_memory_pending,cpu/memory,test_cpu_memory_pending.c))
$(eval $(call ti_c54x_add_test,cpu_standard_indirect_modes,cpu/memory,test_cpu_standard_indirect_modes.c))
$(eval $(call ti_c54x_add_test,cpu_indexed_standard_indirect,cpu/memory,test_cpu_indexed_standard_indirect.c))
$(eval $(call ti_c54x_add_test,cpu_circular_standard_indirect,cpu/memory,test_cpu_circular_standard_indirect.c))
$(eval $(call ti_c54x_add_test,cpu_reverse_carry_standard_indirect,cpu/memory,test_cpu_reverse_carry_standard_indirect.c))
$(eval $(call ti_c54x_add_test,cpu_atomic_logical_memory,cpu/memory,test_cpu_atomic_logical_memory.c))
$(eval $(call ti_c54x_add_test,cpu_dld_long32_mod13,cpu/memory,test_cpu_dld_long32_mod13.c))
$(eval $(call ti_c54x_add_test,cpu_dst_long32_mod13,cpu/memory,test_cpu_dst_long32_mod13.c))
$(eval $(call ti_c54x_add_test,cpu_addm_lk_smem_absolute,cpu/memory,test_cpu_addm_lk_smem_absolute.c))
