# Test ownership: tests/unit/isa/ops/load owns immediate status-field effects.
$(eval $(call ti_c54x_add_test,ld_status_fields_ops,isa/ops/load,test_ld_status_fields.c))
$(eval $(call ti_c54x_add_test,ld_smem_ops,isa/ops/load,test_ld_smem.c))
