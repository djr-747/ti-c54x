# Test ownership: tests/unit/isa/decode owns exhaustive first-word decode
# classification and catalog metadata coverage.
$(eval $(call ti_c54x_add_test,nop_decode,isa/decode,test_nop_decode.c))
$(eval $(call ti_c54x_add_test,operand_field,isa/decode,test_operand_field.c))
$(eval $(call ti_c54x_add_test,instruction_bundle,isa/decode,test_instruction_bundle.c))
$(eval $(call ti_c54x_add_test,operand_layout_binding,isa/decode,test_operand_layout_binding.c))
$(eval $(call ti_c54x_add_test,ld_status_fields_decode,isa/decode,test_ld_status_fields.c))
$(eval $(call ti_c54x_add_test,max_min_decode,isa/decode,test_max_min_decode.c))
$(eval $(call ti_c54x_add_test,cmpl_decode,isa/decode,test_cmpl_decode.c))
$(eval $(call ti_c54x_add_test,rol_ror_decode,isa/decode,test_rol_ror_decode.c))
$(eval $(call ti_c54x_add_test,sftl_decode,isa/decode,test_sftl_decode.c))
$(eval $(call ti_c54x_add_test,logical_accumulator_decode,isa/decode,test_logical_accumulator_decode.c))
$(eval $(call ti_c54x_add_test,roltc_decode,isa/decode,test_roltc_decode.c))
$(eval $(call ti_c54x_add_test,sftc_decode,isa/decode,test_sftc_decode.c))
