# Test ownership: this leaf owns only public-CPU fixed-MOD2 LD proofs.
$(eval $(call ti_c54x_add_test,cpu_ld_smem,cpu/isa/load,test_cpu_ld_smem.c))
$(eval $(call ti_c54x_add_test,cpu_ldu_smem,cpu/isa/load,test_cpu_ldu_smem.c))
$(eval $(call ti_c54x_add_test,cpu_ld_t_smem,cpu/isa/load,test_cpu_ld_t_smem.c))
$(eval $(call ti_c54x_add_test,cpu_ld_status_smem,cpu/isa/load,test_cpu_ld_status_smem.c))
