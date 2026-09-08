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
ti_c54x_add_unit_test(atomic_logical_memory_effects
                      test_atomic_logical_memory_effects.c)
# W47B1 RED contract: DLD/DST MOD13 LONG32 carrier and pure resolver.
ti_c54x_add_unit_test(dld_dst_long32_memory_carrier
                      test_dld_dst_long32_memory_carrier.c)
