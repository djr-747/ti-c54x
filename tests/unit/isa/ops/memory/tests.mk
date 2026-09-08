# Test ownership: tests/unit/isa/ops/memory owns structural validation of the
# generic memory effect; it does not own address resolution or sequencing.
$(eval $(call ti_c54x_add_test,memory_effect_modes,isa/ops/memory,test_memory_effect_modes.c))
$(eval $(call ti_c54x_add_test,indexed_word16_memory_effect,isa/ops/memory,test_indexed_word16_memory_effect.c))
$(eval $(call ti_c54x_add_test,circular_word16_memory_effect,isa/ops/memory,test_circular_word16_memory_effect.c))
