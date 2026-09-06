# Test ownership: tests/unit/bus/byte_order owns WORD16 byte-order coverage.
$(eval $(call ti_c54x_add_test,bus_word16_codec,bus/byte_order,test_word16_codec.c))
