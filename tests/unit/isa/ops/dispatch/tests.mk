# Test ownership: tests/unit/isa/ops/dispatch owns trusted binding and
# packet-only dispatch proofs.
$(eval $(call ti_c54x_add_test,trusted_opcode_binding,isa/ops/dispatch,test_trusted_opcode_binding.c))
$(eval $(call ti_c54x_add_test,trusted_semantic_dispatch,isa/ops/dispatch,test_trusted_semantic_dispatch.c))
