# Test ownership: tests/unit/isa/ops/memory owns structural validation of the
# generic memory effect; it does not own address resolution or sequencing.
$(eval $(call ti_c54x_add_test,memory_effect_modes,isa/ops/memory,test_memory_effect_modes.c))
$(eval $(call ti_c54x_add_test,indexed_word16_memory_effect,isa/ops/memory,test_indexed_word16_memory_effect.c))
$(eval $(call ti_c54x_add_test,circular_word16_memory_effect,isa/ops/memory,test_circular_word16_memory_effect.c))
$(eval $(call ti_c54x_add_test,reverse_carry_word16_memory_effect,isa/ops/memory,test_reverse_carry_word16_memory_effect.c))
$(eval $(call ti_c54x_add_test,word16_rmw_memory_effect,isa/ops/memory,test_word16_rmw_memory_effect.c))
$(eval $(call ti_c54x_add_test,atomic_logical_memory_effects,isa/ops/memory,test_atomic_logical_memory_effects.c))
# P3-S32 RED contract: ADDM standard-indirect WORD16 ADD-transform leaf.
$(eval $(call ti_c54x_add_test,addm_smem_memory_effect,isa/ops/memory,test_addm_smem_memory_effect.c))
# W47B1 RED contract: DLD/DST MOD13 LONG32 carrier and pure resolver.
$(eval $(call ti_c54x_add_test,dld_dst_long32_memory_carrier,isa/ops/memory,test_dld_dst_long32_memory_carrier.c))
$(eval $(call ti_c54x_add_test,dual_word16_memory_effect,isa/ops/memory,test_dual_word16_memory_effect.c))
# P3-S5 RED contract: accumulator-targeted stack-push effect shapes.
$(eval $(call ti_c54x_add_test,accumulator_push_memory_effect,isa/ops/memory,test_accumulator_push_memory_effect.c))
