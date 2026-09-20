# Test ownership: this leaf owns CPU-level OR absolute-Smem proofs only.
$(eval $(call ti_c54x_add_test,cpu_or_smem_absolute,cpu/isa/logical/smem/or,test_cpu_or_smem_absolute.c))
