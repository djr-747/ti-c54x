# Test ownership: tests/unit/bus aggregates private bus-owner tests.
ti_c54x_add_unit_test(bk_mmr test_bk_mmr.c)
add_subdirectory(adapter)
add_subdirectory(request)
add_subdirectory(byte_order)
add_subdirectory(transaction)
