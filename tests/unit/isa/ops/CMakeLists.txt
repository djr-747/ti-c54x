# Test ownership: tests/unit/isa/ops owns NOP semantic and dispatch seams.
ti_c54x_add_unit_test(nop test_nop.c)
add_subdirectory(compare)
add_subdirectory(accumulator)
add_subdirectory(load)
add_subdirectory(repeat)
