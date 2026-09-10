# Test ownership: tests/unit/isa/ops/branch owns pure branch semantic effects.
$(eval $(call ti_c54x_add_test,b_pmad,isa/ops/branch,test_b_pmad.c))
$(eval $(call ti_c54x_add_test,bc_pmad,isa/ops/branch,test_bc_pmad.c))
$(eval $(call ti_c54x_add_test,call_pmad,isa/ops/branch,test_call_pmad.c))
