# Test ownership: tests/unit/sequencer/repeat owns pure repeat-policy proofs.
$(eval $(call ti_c54x_add_test,repeat_cardinality_contract,sequencer/repeat,test_repeat_cardinality_contract.c))
$(eval $(call ti_c54x_add_test,repeat_body_admission_contract,sequencer/repeat,test_repeat_body_admission_contract.c))
$(eval $(call ti_c54x_add_test,repeat_request_gate_contract,sequencer/repeat,test_repeat_request_gate_contract.c))
$(eval $(call ti_c54x_add_test,repeat_progress_contract,sequencer/repeat,test_repeat_progress_contract.c))
$(eval $(call ti_c54x_add_test,single_repeat_rc_observable_progression,sequencer/repeat,test_single_repeat_rc_observable_progression.c))
