# Test ownership: tests/unit/isa/ops/memory owns structural validation of the
# generic memory effect; it does not own address resolution or sequencing.
$(eval $(call ti_c54x_add_test,memory_effect_modes,isa/ops/memory,test_memory_effect_modes.c))
