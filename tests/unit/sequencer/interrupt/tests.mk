# Test ownership: tests/unit/sequencer/interrupt owns pending-request proofs.
$(eval $(call ti_c54x_add_test,repeat_pending_request,sequencer/interrupt,test_repeat_pending_request.c))
$(eval $(call ti_c54x_add_test,repeat_pending_request_contract,sequencer/interrupt,test_repeat_pending_request_contract.c))
$(eval $(call ti_c54x_add_test,take_arbitration,sequencer/interrupt,test_take_arbitration.c))
$(eval $(call ti_c54x_add_test,take_complete,sequencer/interrupt,test_take_complete.c))
$(eval $(call ti_c54x_add_test,take_arbitration_maskable,sequencer/interrupt,test_take_arbitration_maskable.c))
