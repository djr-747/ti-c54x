# Test ownership: tests/unit/bus/adapter owns public WORD16 endpoint and
# external visibility protocol coverage.
ti_c54x_add_unit_test(bus_word16_endpoint_contract test_word16_endpoint_contract.c)
ti_c54x_add_unit_test(bus_word16_exactly_once test_word16_exactly_once.c)
