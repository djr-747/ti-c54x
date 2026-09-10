# Test ownership: this leaf owns only public-CPU branch proofs.
$(eval $(call ti_c54x_add_test,cpu_b_pmad,cpu/isa/branch,test_cpu_b_pmad.c))
$(eval $(call ti_c54x_add_test,cpu_bc_pmad,cpu/isa/branch,test_cpu_bc_pmad.c))
$(eval $(call ti_c54x_add_test,cpu_call_pmad,cpu/isa/branch,test_cpu_call_pmad.c))
$(eval $(call ti_c54x_add_test,cpu_ret,cpu/isa/branch,test_cpu_ret.c))
# W67B RED contract: RETD CPU retirement.
$(eval $(call ti_c54x_add_test,cpu_retd,cpu/isa/branch,test_cpu_retd.c))
