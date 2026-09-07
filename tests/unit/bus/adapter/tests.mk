# Test ownership: tests/unit/bus/adapter owns public WORD16 endpoint and
# external visibility protocol coverage.
$(eval $(call ti_c54x_add_test,bus_word16_endpoint_contract,bus/adapter,test_word16_endpoint_contract.c))
$(eval $(call ti_c54x_add_test,bus_word16_exactly_once,bus/adapter,test_word16_exactly_once.c))
