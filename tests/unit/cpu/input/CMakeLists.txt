# Test ownership: tests/unit/cpu/input owns generic input-collection proofs.
ti_c54x_add_unit_test(cpu_input_collect test_collect.c)
ti_c54x_add_unit_test(cpu_input_collect_tc test_collect_tc.c)
ti_c54x_add_unit_test(cpu_input_collect_sxm test_collect_sxm.c)
ti_c54x_add_unit_test(cpu_input_collect_sst_status test_collect_sst_status.c)
ti_c54x_add_unit_test(collect_stack_pointer test_collect_stack_pointer.c)
ti_c54x_add_unit_test(cpu_input_collect_ovm_status test_collect_ovm_status.c)
ti_c54x_add_unit_test(cpu_input_collect_asm_status test_collect_asm_status.c)
ti_c54x_add_unit_test(cpu_input_collect_auxiliary_registers
                      test_collect_auxiliary_registers.c)
ti_c54x_add_unit_test(cpu_input_collect_t_register test_collect_t_register.c)
