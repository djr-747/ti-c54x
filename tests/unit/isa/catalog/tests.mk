# Test ownership: tests/unit/isa/catalog owns immutable catalog metadata
# identity/range and copied decode propagation proofs.
$(eval $(call ti_c54x_add_test,opcode_metadata,isa/catalog,test_opcode_metadata.c))
$(eval $(call ti_c54x_add_test,rol_ror_metadata,isa/catalog,test_rol_ror_metadata.c))
