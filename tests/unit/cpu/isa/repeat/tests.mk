# Test ownership: this leaf owns only public-CPU RPT activation proofs.
$(eval $(call ti_c54x_add_test,cpu_rpt,cpu/isa/repeat,test_cpu_rpt.c))
# P3-S3 RED contract: RPT-MAC replay.
$(eval $(call ti_c54x_add_test,cpu_rpt_mac_replay,cpu/isa/repeat,test_cpu_rpt_mac_replay.c))
$(eval $(call ti_c54x_add_test,cpu_rptb,cpu/isa/repeat,test_cpu_rptb.c))
$(eval $(call ti_c54x_add_test,cpu_rpt_smem,cpu/isa/repeat,test_cpu_rpt_smem.c))
