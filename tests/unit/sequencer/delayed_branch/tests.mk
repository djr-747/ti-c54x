# Test ownership: tests/unit/sequencer/delayed_branch owns pure delay-countdown proofs.
$(eval $(call ti_c54x_add_test,delayed_branch,sequencer/delayed_branch,test_delayed_branch.c))
