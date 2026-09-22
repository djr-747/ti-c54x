# Test ownership: this leaf owns only public-CPU branch proofs.
ti_c54x_add_unit_test(cpu_b_pmad test_cpu_b_pmad.c)
# W68 RED contract: BD CPU retirement.
ti_c54x_add_unit_test(cpu_bd test_cpu_bd.c)
# P3-S103: RESET CPU retirement.
ti_c54x_add_unit_test(cpu_reset test_cpu_reset.c)
# P3-S105: TRAP K retirement.
ti_c54x_add_unit_test(cpu_trap test_cpu_trap.c)
# P3-S110: INTR K retirement.
ti_c54x_add_unit_test(cpu_intr test_cpu_intr.c)
ti_c54x_add_unit_test(cpu_bc_pmad test_cpu_bc_pmad.c)
# W69 RED contract: BCD CPU retirement.
ti_c54x_add_unit_test(cpu_bcd test_cpu_bcd.c)
# W70 RED contract: BANZ CPU retirement.
ti_c54x_add_unit_test(cpu_banz test_cpu_banz.c)
# W71 RED contract: BANZD CPU retirement.
ti_c54x_add_unit_test(cpu_banzd test_cpu_banzd.c)
# W72 RED contract: CCD CPU retirement.
ti_c54x_add_unit_test(cpu_ccd test_cpu_ccd.c)
# W73 RED contract: CC CPU retirement.
ti_c54x_add_unit_test(cpu_cc test_cpu_cc.c)
ti_c54x_add_unit_test(cpu_call_pmad test_cpu_call_pmad.c)
ti_c54x_add_unit_test(cpu_ret test_cpu_ret.c)
# W74 RED contract: RETE CPU retirement.
ti_c54x_add_unit_test(cpu_rete test_cpu_rete.c)
# W75 RED contract: RETED CPU retirement.
ti_c54x_add_unit_test(cpu_reted test_cpu_reted.c)
# W76 RED contract: RETF CPU retirement.
ti_c54x_add_unit_test(cpu_retf test_cpu_retf.c)
# W77 RED contract: RC CPU retirement.
ti_c54x_add_unit_test(cpu_rc test_cpu_rc.c)
# W78 RED contract: RCD CPU retirement.
ti_c54x_add_unit_test(cpu_rcd test_cpu_rcd.c)
# P3-S4 RED contract: XC CPU retirement.
ti_c54x_add_unit_test(cpu_xc test_cpu_xc.c)
# P3 slice 4 RED contract: BACC CPU retirement.
ti_c54x_add_unit_test(cpu_bacc test_cpu_bacc.c)
# P3 slice 4 RED contract: BACCD CPU retirement.
ti_c54x_add_unit_test(cpu_baccd test_cpu_baccd.c)
# P3 slice 9 RED contract: CALA CPU retirement.
ti_c54x_add_unit_test(cpu_cala test_cpu_cala.c)
# P3 slice 9 RED contract: CALAD CPU retirement.
ti_c54x_add_unit_test(cpu_calad test_cpu_calad.c)
# W67B RED contract: RETD CPU retirement.
ti_c54x_add_unit_test(cpu_retd test_cpu_retd.c)
# W67C RED contract: CALLD CPU retirement.
ti_c54x_add_unit_test(cpu_calld test_cpu_calld.c)
# P3-S117 RED contract: FB CPU retirement with XPC commit.
ti_c54x_add_unit_test(cpu_fb test_cpu_fb.c)
# P3-S117 RED contract: FBD CPU retirement with XPC commit.
ti_c54x_add_unit_test(cpu_fbd test_cpu_fbd.c)
# P3-S117 RED contract: FBACC CPU retirement with XPC commit.
ti_c54x_add_unit_test(cpu_fbacc test_cpu_fbacc.c)
# P3-S117 RED contract: FBACCD CPU retirement with XPC commit.
ti_c54x_add_unit_test(cpu_fbaccd test_cpu_fbaccd.c)
# P3-S118 RED contract: FCALL CPU retirement with far double-push.
ti_c54x_add_unit_test(cpu_fcall test_cpu_fcall.c)
# P3-S118 RED contract: FCALLD CPU retirement with far double-push.
ti_c54x_add_unit_test(cpu_fcalld test_cpu_fcalld.c)
# P3-S118 RED contract: FCALA CPU retirement with far double-push.
ti_c54x_add_unit_test(cpu_fcala test_cpu_fcala.c)
# P3-S118 RED contract: FCALAD CPU retirement with far double-push.
ti_c54x_add_unit_test(cpu_fcalad test_cpu_fcalad.c)
