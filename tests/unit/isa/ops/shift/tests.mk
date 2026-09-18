# Test ownership: tests/unit/isa/ops/shift owns SFTL semantic proofs.
$(eval $(call ti_c54x_add_test,sftl,isa/ops/shift,test_sftl.c))
$(eval $(call ti_c54x_add_test,sftc,isa/ops/shift,test_sftc.c))
$(eval $(call ti_c54x_add_test,sfta_acc,isa/ops/shift,test_sfta_acc.c))
