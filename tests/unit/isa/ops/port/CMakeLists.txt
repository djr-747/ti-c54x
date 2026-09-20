# Test ownership: tests/unit/isa/ops/port owns pure PORTR/PORTW I/O-pair
# memory effects.
ti_c54x_add_unit_test(portr_ops test_portr.c)
ti_c54x_add_unit_test(portw_ops test_portw.c)
ti_c54x_add_unit_test(portw_smem_absolute_ops test_portw_smem_absolute.c)
ti_c54x_add_unit_test(portr_smem_absolute_ops test_portr_smem_absolute.c)
