# Test ownership: tests/unit/hosted owns hosted-tooling peer proofs.
$(eval $(call ti_c54x_add_test,mcu_upload,hosted,test_mcu_upload.c))
$(eval $(call ti_c54x_add_test,port_peer,hosted,test_port_peer.c))
