# Test ownership: tests/unit/isa/decode owns exhaustive first-word decode
# classification and catalog metadata coverage.
ti_c54x_add_unit_test(nop_decode test_nop_decode.c)
ti_c54x_add_unit_test(operand_field test_operand_field.c)
ti_c54x_add_unit_test(instruction_bundle test_instruction_bundle.c)
ti_c54x_add_unit_test(operand_layout_binding test_operand_layout_binding.c)
ti_c54x_add_unit_test(ld_status_fields_decode test_ld_status_fields.c)
