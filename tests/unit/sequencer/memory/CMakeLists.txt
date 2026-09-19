# Test ownership: tests/unit/sequencer/memory owns generic data-memory address
# resolution; it does not own opcode identity or CPU orchestration.
ti_c54x_add_unit_test(memory_address_resolution
                      test_memory_address_resolution.c)
ti_c54x_add_unit_test(memory_address_modes
                      test_memory_address_modes.c)
ti_c54x_add_unit_test(indexed_word16_memory_resolver
                      test_indexed_word16_memory_resolver.c)
ti_c54x_add_unit_test(circular_word16_address_resolution
                      test_circular_word16_address_resolution.c)
ti_c54x_add_unit_test(reverse_carry_word16_address_resolution
                      test_reverse_carry_word16_address_resolution.c)
ti_c54x_add_unit_test(dual_address_resolution
                      test_dual_address_resolution.c)
# W52B RED contract: dual-indirect MOD3 resolution and CMPT/ARP invariance.
ti_c54x_add_unit_test(dual_mod3_resolution
                      test_dual_mod3_resolution.c)
# W65A RED contract: stack-push WORD16 resolution and mode independence.
ti_c54x_add_unit_test(stack_push_resolution
                      test_stack_push_resolution.c)
# P3-S5 RED contract: accumulator-targeted push resolution.
ti_c54x_add_unit_test(accumulator_push_resolution
                      test_accumulator_push_resolution.c)
# W66A RED contract: stack-pop WORD16 resolution and mode independence.
ti_c54x_add_unit_test(stack_pop_resolution
                      test_stack_pop_resolution.c)
# P3-S1 RED contract: MMR access planning and knownness gating.
ti_c54x_add_unit_test(mmr_resolver
                      test_mmr_address_resolution.c)
# P3-S6 RED contract: PORT IO-leg planning.
ti_c54x_add_unit_test(port_resolver
                      test_port_address_resolution.c)
# P3-S55 RED contract: single-word Lmem LONG32 planning.
ti_c54x_add_unit_test(long32_single_resolver
                      test_long32_single_access_plan.c)
