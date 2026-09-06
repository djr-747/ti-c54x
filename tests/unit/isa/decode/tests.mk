# Test ownership: tests/unit/isa/decode owns exhaustive first-word decode
# classification and catalog metadata coverage.
$(eval $(call ti_c54x_add_test,nop_decode,isa/decode,test_nop_decode.c))
$(eval $(call ti_c54x_add_test,operand_field,isa/decode,test_operand_field.c))
$(eval $(call ti_c54x_add_test,instruction_bundle,isa/decode,test_instruction_bundle.c))
