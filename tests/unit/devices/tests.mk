# Test ownership: tests/unit/devices owns documented-peripheral proofs.
$(eval $(call ti_c54x_add_test,devices_swwsr,devices,test_software_wait_state_register.c))
$(eval $(call ti_c54x_add_test,devices_clkmd,devices,test_clock_mode_register.c))
