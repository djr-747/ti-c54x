# Test ownership: tests/unit/sequencer/memory owns generic data-memory address
# resolution; it does not own opcode identity or CPU orchestration.
ti_c54x_add_unit_test(memory_address_resolution
                      test_memory_address_resolution.c)
ti_c54x_add_unit_test(memory_address_modes
                      test_memory_address_modes.c)
