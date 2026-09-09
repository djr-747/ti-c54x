# Test ownership: tests/unit/isa/ops/arithmetic owns ADD/SUB accumulator proofs.
$(eval $(call ti_c54x_add_test,add_sub_acc_shift,isa/ops/arithmetic,test_add_sub_acc_shift.c))
$(eval $(call ti_c54x_add_test,add_sub_dual,isa/ops/arithmetic,test_add_sub_dual.c))
# W51C RED contract: non-MOD0/MOD0 dual ADD/SUB checked/trusted semantics.
$(eval $(call ti_c54x_add_test,add_sub_dual_modes,isa/ops/arithmetic,test_add_sub_dual_modes.c))
