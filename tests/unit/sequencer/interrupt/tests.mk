# Test ownership: tests/unit/sequencer/interrupt owns pending-request proofs.
$(eval $(call ti_c54x_add_test,repeat_pending_request,sequencer/interrupt,test_repeat_pending_request.c))
$(eval $(call ti_c54x_add_test,repeat_pending_request_contract,sequencer/interrupt,test_repeat_pending_request_contract.c))
