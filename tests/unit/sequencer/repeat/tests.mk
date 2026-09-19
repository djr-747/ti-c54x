# Test ownership: tests/unit/sequencer/repeat owns pure repeat-policy proofs.
$(eval $(call ti_c54x_add_test,repeat_cardinality_contract,sequencer/repeat,test_repeat_cardinality_contract.c))
$(eval $(call ti_c54x_add_test,repeat_body_admission_contract,sequencer/repeat,test_repeat_body_admission_contract.c))
$(eval $(call ti_c54x_add_test,repeat_request_gate_contract,sequencer/repeat,test_repeat_request_gate_contract.c))
$(eval $(call ti_c54x_add_test,repeat_progress_contract,sequencer/repeat,test_repeat_progress_contract.c))
$(eval $(call ti_c54x_add_test,single_repeat_rc_observable_progression,sequencer/repeat,test_single_repeat_rc_observable_progression.c))
# W52A COMMON repeat-truth closure: TI-evidence-bound repeat contracts.
$(eval $(call ti_c54x_add_test,repeat_cardinality_common,sequencer/repeat,test_single_repeat_cardinality.c))
$(eval $(call ti_c54x_add_test,repeat_request_gate_common,sequencer/repeat,test_single_repeat_irq_gate.c))
$(eval $(call ti_c54x_add_test,repeat_body_eligibility_common,sequencer/repeat,test_single_repeat_body_eligibility.c))
# P3-S2 RED contract: single-repeat driver composition.
$(eval $(call ti_c54x_add_test,repeat_single_flow,sequencer/repeat,test_repeat_single_flow.c))
$(eval $(call ti_c54x_add_test,repeat_block_flow,sequencer/repeat,test_repeat_block_flow.c))
