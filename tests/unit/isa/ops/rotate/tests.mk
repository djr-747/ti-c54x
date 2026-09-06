# Test ownership: tests/unit/isa/ops/rotate owns ROL/ROR semantic proofs.
$(eval $(call ti_c54x_add_test,rol_ror,isa/ops/rotate,test_rol_ror.c))
$(eval $(call ti_c54x_add_test,roltc,isa/ops/rotate,test_roltc.c))
