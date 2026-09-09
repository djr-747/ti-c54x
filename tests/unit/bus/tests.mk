# Test ownership: tests/unit/bus aggregates private bus-owner tests.
$(eval $(call ti_c54x_add_test,bk_mmr,bus,test_bk_mmr.c))
include tests/unit/bus/adapter/tests.mk
include tests/unit/bus/request/tests.mk
include tests/unit/bus/byte_order/tests.mk
include tests/unit/bus/transaction/tests.mk
