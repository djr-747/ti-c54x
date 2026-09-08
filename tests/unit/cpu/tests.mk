# Test ownership: tests/unit/cpu owns lifecycle, fetch, and execution coverage.
$(eval $(call ti_c54x_add_test,cpu_api,cpu,test_cpu_api.c))
$(eval $(call ti_c54x_add_test,cpu_nop_execution,cpu,test_cpu_nop_execution.c))
$(eval $(call ti_c54x_add_test,cpu_nop_execution_contract,cpu,test_cpu_nop_execution_contract.c))
$(eval $(call ti_c54x_add_test,cpu_execution_budget,cpu,test_cpu_execution_budget.c))
$(eval $(call ti_c54x_add_test,cpu_instruction_extension_fetch,cpu,test_instruction_extension_fetch.c))
$(eval $(call ti_c54x_add_test,cpu_ld_status_fields,cpu,test_cpu_ld_status_fields.c))
$(eval $(call ti_c54x_add_test,cpu_max_min,cpu,test_cpu_max_min.c))
$(eval $(call ti_c54x_add_test,cpu_cmpl,cpu,test_cpu_cmpl.c))
$(eval $(call ti_c54x_add_test,cpu_rol_ror,cpu,test_cpu_rol_ror.c))
$(eval $(call ti_c54x_add_test,cpu_sftl,cpu,test_cpu_sftl.c))
$(eval $(call ti_c54x_add_test,cpu_logical_accumulator,cpu,test_cpu_logical_accumulator.c))
$(eval $(call ti_c54x_add_test,cpu_add_sub_acc_shift,cpu,test_cpu_add_sub_acc_shift.c))
$(eval $(call ti_c54x_add_test,cpu_roltc,cpu,test_cpu_roltc.c))
$(eval $(call ti_c54x_add_test,cpu_sftc,cpu,test_cpu_sftc.c))
$(eval $(call ti_c54x_add_test,cpu_exp,cpu,test_cpu_exp.c))
include tests/unit/cpu/execution/tests.mk
include tests/unit/cpu/input/tests.mk
include tests/unit/cpu/isa/tests.mk
include tests/unit/cpu/state/tests.mk
include tests/unit/cpu/memory/tests.mk
