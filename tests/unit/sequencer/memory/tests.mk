# Test ownership: tests/unit/sequencer/memory owns generic data-memory address
# resolution; it does not own opcode identity or CPU orchestration.
$(eval $(call ti_c54x_add_test,memory_address_resolution,sequencer/memory,test_memory_address_resolution.c))
$(eval $(call ti_c54x_add_test,memory_address_modes,sequencer/memory,test_memory_address_modes.c))
