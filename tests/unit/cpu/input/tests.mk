# Test ownership: tests/unit/cpu/input owns generic input-collection proofs.
$(eval $(call ti_c54x_add_test,cpu_input_collect,cpu/input,test_collect.c))
$(eval $(call ti_c54x_add_test,cpu_input_collect_tc,cpu/input,test_collect_tc.c))
