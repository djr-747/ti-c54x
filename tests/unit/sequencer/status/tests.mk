# Test ownership: tests/unit/sequencer/status owns status-field effect proofs.
$(eval $(call ti_c54x_add_test,status_field_effect_apply,sequencer/status,test_status_field_effect_apply.c))
