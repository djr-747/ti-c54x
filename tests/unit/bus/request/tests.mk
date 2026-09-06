# Test ownership: tests/unit/bus/request owns WORD16 request coverage.
$(eval $(call ti_c54x_add_test,bus_word16_request,bus/request,test_word16_request.c))
