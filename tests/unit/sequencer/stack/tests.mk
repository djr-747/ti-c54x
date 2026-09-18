# Test ownership: tests/unit/sequencer/stack owns SP-effect application coverage.
$(eval $(call ti_c54x_add_test,sp_write_apply,sequencer/stack,test_sp_write_apply.c))
