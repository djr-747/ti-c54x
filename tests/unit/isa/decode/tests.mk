# Test ownership: tests/unit/isa/decode owns exhaustive first-word decode
# classification and catalog metadata coverage.
$(eval $(call ti_c54x_add_test,nop_decode,isa/decode,test_nop_decode.c))
