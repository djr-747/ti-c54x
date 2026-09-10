# Test ownership: this leaf owns only public-CPU branch proofs.
ti_c54x_add_unit_test(cpu_b_pmad test_cpu_b_pmad.c)
ti_c54x_add_unit_test(cpu_bc_pmad test_cpu_bc_pmad.c)
ti_c54x_add_unit_test(cpu_call_pmad test_cpu_call_pmad.c)
ti_c54x_add_unit_test(cpu_ret test_cpu_ret.c)
# W67B RED contract: RETD CPU retirement.
ti_c54x_add_unit_test(cpu_retd test_cpu_retd.c)
