# Test ownership: tests/unit/state/status owns status-register bank proofs.
$(eval $(call ti_c54x_add_test,status_registers,state/status,test_status_registers.c))
$(eval $(call ti_c54x_add_test,tc_field,state/status,test_tc_field.c))
$(eval $(call ti_c54x_add_test,sxm_field,state/status,test_sxm_field.c))
