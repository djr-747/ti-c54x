# Test ownership: tests/unit/state owns architectural state coverage.
$(eval $(call ti_c54x_add_test,arch_state_reset,state,test_arch_state_reset.c))
$(eval $(call ti_c54x_add_test,arch_state_knownness,state,test_arch_state_knownness.c))
$(eval $(call ti_c54x_add_test,arch_state_address_fields,state,test_arch_state_address_fields.c))
$(eval $(call ti_c54x_add_test,arch_state_address_field_contract,state,test_arch_state_address_field_contract.c))
$(eval $(call ti_c54x_add_test,block_size_register,state,test_block_size_register.c))
$(eval $(call ti_c54x_add_test,block_size_register_contract,state,test_block_size_register_contract.c))
include tests/unit/state/accumulator/tests.mk
include tests/unit/state/status/tests.mk
