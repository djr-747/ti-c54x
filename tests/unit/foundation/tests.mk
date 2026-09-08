# Test ownership: tests/unit/foundation owns arithmetic primitive coverage.
$(eval $(call ti_c54x_add_test,arithmetic40,foundation,test_arithmetic40.c))
$(eval $(call ti_c54x_add_test,arithmetic40_addsub,foundation,test_arithmetic40_addsub.c))
$(eval $(call ti_c54x_add_test,reverse_carry,foundation,test_reverse_carry.c))
$(eval $(call ti_c54x_add_test,reverse_carry_contract,foundation,test_reverse_carry_contract.c))
