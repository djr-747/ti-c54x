# Test ownership: this leaf owns CPU-level PORTW absolute-Smem proofs only.
$(eval $(call ti_c54x_add_test,cpu_portw_smem_absolute,cpu/isa/peripheral/port,test_cpu_portw_smem_absolute.c))
