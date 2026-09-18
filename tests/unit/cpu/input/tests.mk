# Test ownership: tests/unit/cpu/input owns generic input-collection proofs.
$(eval $(call ti_c54x_add_test,cpu_input_collect,cpu/input,test_collect.c))
$(eval $(call ti_c54x_add_test,cpu_input_collect_tc,cpu/input,test_collect_tc.c))
$(eval $(call ti_c54x_add_test,cpu_input_collect_sxm,cpu/input,test_collect_sxm.c))
$(eval $(call ti_c54x_add_test,cpu_input_collect_sst_status,cpu/input,test_collect_sst_status.c))
$(eval $(call ti_c54x_add_test,collect_stack_pointer,cpu/input,test_collect_stack_pointer.c))
# W76 RED contract: return-register input collection.
$(eval $(call ti_c54x_add_test,collect_return_register,cpu/input,test_collect_return_register.c))
$(eval $(call ti_c54x_add_test,cpu_input_collect_ovm_status,cpu/input,test_collect_ovm_status.c))
$(eval $(call ti_c54x_add_test,cpu_input_collect_asm_status,cpu/input,test_collect_asm_status.c))
$(eval $(call ti_c54x_add_test,cpu_input_collect_frct_status,cpu/input,test_collect_frct_status.c))
$(eval $(call ti_c54x_add_test,cpu_input_collect_auxiliary_registers,cpu/input,test_collect_auxiliary_registers.c))
$(eval $(call ti_c54x_add_test,cpu_input_collect_t_register,cpu/input,test_collect_t_register.c))
$(eval $(call ti_c54x_add_test,cpu_input_collect_trn_register,cpu/input,test_collect_trn_register.c))
