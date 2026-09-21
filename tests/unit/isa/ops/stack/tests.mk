# Test ownership: tests/unit/isa/ops/stack owns stack-pointer semantic effects.
$(eval $(call ti_c54x_add_test,frame_k_ops,isa/ops/stack,test_frame_k.c))
# P3-S105 RED contract: TRAP K effects.
$(eval $(call ti_c54x_add_test,trap_ops,isa/ops/stack,test_trap.c))
$(eval $(call ti_c54x_add_test,pshd_popd_direct_ops,isa/ops/stack,test_pshd_popd_direct.c))
