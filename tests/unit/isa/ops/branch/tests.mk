# Test ownership: tests/unit/isa/ops/branch owns pure branch semantic effects.
$(eval $(call ti_c54x_add_test,b_pmad,isa/ops/branch,test_b_pmad.c))
# W68 RED contract: BD effects.
$(eval $(call ti_c54x_add_test,bd,isa/ops/branch,test_bd.c))
$(eval $(call ti_c54x_add_test,bc_pmad,isa/ops/branch,test_bc_pmad.c))
# W69 RED contract: BCD effects.
$(eval $(call ti_c54x_add_test,bcd,isa/ops/branch,test_bcd.c))
$(eval $(call ti_c54x_add_test,call_pmad,isa/ops/branch,test_call_pmad.c))
$(eval $(call ti_c54x_add_test,ret,isa/ops/branch,test_ret.c))
# W67B RED contract: RETD effects.
$(eval $(call ti_c54x_add_test,retd,isa/ops/branch,test_retd.c))
# W67C RED contract: CALLD effects.
$(eval $(call ti_c54x_add_test,calld,isa/ops/branch,test_calld.c))
