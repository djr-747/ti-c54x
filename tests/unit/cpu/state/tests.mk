# Test ownership: this leaf owns the project CPU architectural-state
# composition proof.  The parent CPU manifest includes this file.
$(eval $(call ti_c54x_add_test,cpu_arch_state_composition,cpu/state,test_cpu_arch_state_composition.c))
