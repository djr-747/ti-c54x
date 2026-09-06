# Test ownership: tests/unit/cpu owns CPU lifecycle API coverage.
$(eval $(call ti_c54x_add_test,cpu_api,cpu,test_cpu_api.c))
$(eval $(call ti_c54x_add_test,cpu_nop_execution,cpu,test_cpu_nop_execution.c))
$(eval $(call ti_c54x_add_test,cpu_nop_execution_contract,cpu,test_cpu_nop_execution_contract.c))
$(eval $(call ti_c54x_add_test,cpu_execution_budget,cpu,test_cpu_execution_budget.c))
