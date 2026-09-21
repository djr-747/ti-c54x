# Test ownership: this leaf owns public-CPU syntax-6 dual-MOD0 ADD/SUB proofs.
ti_c54x_add_unit_test(cpu_add_sub_dual_mod0 test_cpu_add_sub_dual_mod0.c)
# W51C RED contract: public-CPU non-MOD0/MOD0 dual ADD/SUB retirement.
ti_c54x_add_unit_test(cpu_add_sub_dual_modes test_cpu_add_sub_dual_modes.c)
# W52C RED contract: public-CPU MOD3-involving dual ADD/SUB retirement.
ti_c54x_add_unit_test(cpu_add_sub_dual_mod3 test_cpu_add_sub_dual_mod3.c)
# P3-S3z RED contract: public-CPU ADD Smem syntaxes 1-3 retirement.
ti_c54x_add_unit_test(cpu_add_smem test_cpu_add_smem.c)
ti_c54x_add_unit_test(cpu_sub_smem_absolute test_cpu_sub_smem_absolute.c)
ti_c54x_add_unit_test(cpu_dadd test_cpu_dadd.c)
ti_c54x_add_unit_test(cpu_dadst test_cpu_dadst.c)
ti_c54x_add_unit_test(cpu_drsub test_cpu_drsub.c)
ti_c54x_add_unit_test(cpu_dsadt test_cpu_dsadt.c)
ti_c54x_add_unit_test(cpu_dsub test_cpu_dsub.c)
ti_c54x_add_unit_test(cpu_dsubt test_cpu_dsubt.c)
ti_c54x_add_unit_test(cpu_sqdst test_cpu_sqdst.c)
ti_c54x_add_unit_test(cpu_abdst test_cpu_abdst.c)
