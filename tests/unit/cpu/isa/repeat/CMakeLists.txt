# Test ownership: this leaf owns only public-CPU RPT activation proofs.
ti_c54x_add_unit_test(cpu_rpt test_cpu_rpt.c)
# P3-S3 RED contract: RPT-MAC replay.
ti_c54x_add_unit_test(cpu_rpt_mac_replay test_cpu_rpt_mac_replay.c)
ti_c54x_add_unit_test(cpu_rptb test_cpu_rptb.c)
