# Test ownership: tests/unit/isa/ops/memory owns structural validation of the
# generic memory effect; it does not own address resolution or sequencing.
ti_c54x_add_unit_test(memory_effect_modes test_memory_effect_modes.c)
