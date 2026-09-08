# Test ownership: tests/unit/isa/ops/memory owns structural validation of the
# generic memory effect; it does not own address resolution or sequencing.
ti_c54x_add_unit_test(memory_effect_modes test_memory_effect_modes.c)
ti_c54x_add_unit_test(indexed_word16_memory_effect
                      test_indexed_word16_memory_effect.c)
ti_c54x_add_unit_test(circular_word16_memory_effect
                      test_circular_word16_memory_effect.c)
ti_c54x_add_unit_test(reverse_carry_word16_memory_effect
                      test_reverse_carry_word16_memory_effect.c)
ti_c54x_add_unit_test(word16_rmw_memory_effect
                      test_word16_rmw_memory_effect.c)
