# Test ownership: tests/unit/isa/ops/dispatch owns trusted binding and
# packet-only dispatch proofs.
ti_c54x_add_unit_test(trusted_opcode_binding test_trusted_opcode_binding.c)
ti_c54x_add_unit_test(trusted_semantic_dispatch test_trusted_semantic_dispatch.c)
