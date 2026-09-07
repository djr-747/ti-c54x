# Test ownership: this leaf owns public-CPU SSBX/RSBX status-bit proofs.
$(eval $(call ti_c54x_add_test,cpu_ssbx_rsbx,cpu/isa/status,test_cpu_ssbx_rsbx.c))
