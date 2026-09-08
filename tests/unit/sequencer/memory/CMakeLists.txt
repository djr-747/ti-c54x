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
