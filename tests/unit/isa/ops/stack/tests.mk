# Test ownership: tests/unit/isa/ops/stack owns stack-pointer semantic effects.
$(eval $(call ti_c54x_add_test,frame_k_ops,isa/ops/stack,test_frame_k.c))
