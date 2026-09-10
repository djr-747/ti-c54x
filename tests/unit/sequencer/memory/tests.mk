# Test ownership: tests/unit/sequencer/memory owns generic data-memory address
# resolution; it does not own opcode identity or CPU orchestration.
$(eval $(call ti_c54x_add_test,memory_address_resolution,sequencer/memory,test_memory_address_resolution.c))
$(eval $(call ti_c54x_add_test,memory_address_modes,sequencer/memory,test_memory_address_modes.c))
$(eval $(call ti_c54x_add_test,indexed_word16_memory_resolver,sequencer/memory,test_indexed_word16_memory_resolver.c))
$(eval $(call ti_c54x_add_test,circular_word16_address_resolution,sequencer/memory,test_circular_word16_address_resolution.c))
$(eval $(call ti_c54x_add_test,reverse_carry_word16_address_resolution,sequencer/memory,test_reverse_carry_word16_address_resolution.c))
$(eval $(call ti_c54x_add_test,dual_address_resolution,sequencer/memory,test_dual_address_resolution.c))
# W52B RED contract: dual-indirect MOD3 resolution and CMPT/ARP invariance.
$(eval $(call ti_c54x_add_test,dual_mod3_resolution,sequencer/memory,test_dual_mod3_resolution.c))
# W65A RED contract: stack-push WORD16 resolution and mode independence.
$(eval $(call ti_c54x_add_test,stack_push_resolution,sequencer/memory,test_stack_push_resolution.c))
# W66A RED contract: stack-pop WORD16 resolution and mode independence.
$(eval $(call ti_c54x_add_test,stack_pop_resolution,sequencer/memory,test_stack_pop_resolution.c))
