# Test ownership: tests/unit/isa/ops/port owns pure PORTR/PORTW I/O-pair
# memory effects.
$(eval $(call ti_c54x_add_test,portr_ops,isa/ops/port,test_portr.c))
$(eval $(call ti_c54x_add_test,portw_ops,isa/ops/port,test_portw.c))
