# Test ownership: this leaf owns public-CPU SSBX/RSBX status-bit proofs.
ti_c54x_add_unit_test(cpu_ssbx_rsbx test_cpu_ssbx_rsbx.c)
ti_c54x_add_unit_test(cpu_interrupt_snapshot test_cpu_interrupt_snapshot.c)
ti_c54x_add_unit_test(cpu_interrupt_arrival test_cpu_interrupt_arrival.c)
