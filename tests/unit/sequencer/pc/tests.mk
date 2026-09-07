# Test ownership: tests/unit/sequencer/pc owns trusted sequential-PC commit
# equivalence and checked-boundary proofs.
$(eval $(call ti_c54x_add_test,trusted_pc_commit,sequencer/pc,test_trusted_pc_commit.c))
