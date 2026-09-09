# Test ownership: this leaf owns public-CPU syntax-6 dual-MOD0 ADD/SUB proofs.
ti_c54x_add_unit_test(cpu_add_sub_dual_mod0 test_cpu_add_sub_dual_mod0.c)
# W51C RED contract: public-CPU non-MOD0/MOD0 dual ADD/SUB retirement.
ti_c54x_add_unit_test(cpu_add_sub_dual_modes test_cpu_add_sub_dual_modes.c)
# W52C RED contract: public-CPU MOD3-involving dual ADD/SUB retirement.
ti_c54x_add_unit_test(cpu_add_sub_dual_mod3 test_cpu_add_sub_dual_mod3.c)
