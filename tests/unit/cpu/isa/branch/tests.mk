# Test ownership: this leaf owns only public-CPU branch proofs.
$(eval $(call ti_c54x_add_test,cpu_b_pmad,cpu/isa/branch,test_cpu_b_pmad.c))
# W68 RED contract: BD CPU retirement.
$(eval $(call ti_c54x_add_test,cpu_bd,cpu/isa/branch,test_cpu_bd.c))
# P3-S103: RESET CPU retirement.
$(eval $(call ti_c54x_add_test,cpu_reset,cpu/isa/branch,test_cpu_reset.c))
# P3-S105: TRAP K retirement.
$(eval $(call ti_c54x_add_test,cpu_trap,cpu/isa/branch,test_cpu_trap.c))
# P3-S110: INTR K retirement.
$(eval $(call ti_c54x_add_test,cpu_intr,cpu/isa/branch,test_cpu_intr.c))
$(eval $(call ti_c54x_add_test,cpu_bc_pmad,cpu/isa/branch,test_cpu_bc_pmad.c))
# W69 RED contract: BCD CPU retirement.
$(eval $(call ti_c54x_add_test,cpu_bcd,cpu/isa/branch,test_cpu_bcd.c))
# W70 RED contract: BANZ CPU retirement.
$(eval $(call ti_c54x_add_test,cpu_banz,cpu/isa/branch,test_cpu_banz.c))
# W71 RED contract: BANZD CPU retirement.
$(eval $(call ti_c54x_add_test,cpu_banzd,cpu/isa/branch,test_cpu_banzd.c))
# W72 RED contract: CCD CPU retirement.
$(eval $(call ti_c54x_add_test,cpu_ccd,cpu/isa/branch,test_cpu_ccd.c))
# W73 RED contract: CC CPU retirement.
$(eval $(call ti_c54x_add_test,cpu_cc,cpu/isa/branch,test_cpu_cc.c))
$(eval $(call ti_c54x_add_test,cpu_call_pmad,cpu/isa/branch,test_cpu_call_pmad.c))
$(eval $(call ti_c54x_add_test,cpu_ret,cpu/isa/branch,test_cpu_ret.c))
# W74 RED contract: RETE CPU retirement.
# W75 RED contract: RETED CPU retirement.
$(eval $(call ti_c54x_add_test,cpu_reted,cpu/isa/branch,test_cpu_reted.c))
# W76 RED contract: RETF CPU retirement.
$(eval $(call ti_c54x_add_test,cpu_retf,cpu/isa/branch,test_cpu_retf.c))
# W77 RED contract: RC CPU retirement.
$(eval $(call ti_c54x_add_test,cpu_rc,cpu/isa/branch,test_cpu_rc.c))
# W78 RED contract: RCD CPU retirement.
$(eval $(call ti_c54x_add_test,cpu_rcd,cpu/isa/branch,test_cpu_rcd.c))
# P3-S4 RED contract: XC CPU retirement.
$(eval $(call ti_c54x_add_test,cpu_xc,cpu/isa/branch,test_cpu_xc.c))
# P3 slice 4 RED contract: BACC CPU retirement.
$(eval $(call ti_c54x_add_test,cpu_bacc,cpu/isa/branch,test_cpu_bacc.c))
# P3 slice 4 RED contract: BACCD CPU retirement.
$(eval $(call ti_c54x_add_test,cpu_baccd,cpu/isa/branch,test_cpu_baccd.c))
# P3 slice 9 RED contract: CALA CPU retirement.
$(eval $(call ti_c54x_add_test,cpu_cala,cpu/isa/branch,test_cpu_cala.c))
# P3 slice 9 RED contract: CALAD CPU retirement.
$(eval $(call ti_c54x_add_test,cpu_calad,cpu/isa/branch,test_cpu_calad.c))
$(eval $(call ti_c54x_add_test,cpu_rete,cpu/isa/branch,test_cpu_rete.c))
# W67B RED contract: RETD CPU retirement.
$(eval $(call ti_c54x_add_test,cpu_retd,cpu/isa/branch,test_cpu_retd.c))
# W67C RED contract: CALLD CPU retirement.
$(eval $(call ti_c54x_add_test,cpu_calld,cpu/isa/branch,test_cpu_calld.c))
# P3-S117 RED contract: FB CPU retirement with XPC commit.
$(eval $(call ti_c54x_add_test,cpu_fb,cpu/isa/branch,test_cpu_fb.c))
# P3-S117 RED contract: FBD CPU retirement with XPC commit.
$(eval $(call ti_c54x_add_test,cpu_fbd,cpu/isa/branch,test_cpu_fbd.c))
# P3-S117 RED contract: FBACC CPU retirement with XPC commit.
$(eval $(call ti_c54x_add_test,cpu_fbacc,cpu/isa/branch,test_cpu_fbacc.c))
# P3-S117 RED contract: FBACCD CPU retirement with XPC commit.
$(eval $(call ti_c54x_add_test,cpu_fbaccd,cpu/isa/branch,test_cpu_fbaccd.c))
# P3-S118 RED contract: FCALL CPU retirement with far double-push.
$(eval $(call ti_c54x_add_test,cpu_fcall,cpu/isa/branch,test_cpu_fcall.c))
# P3-S118 RED contract: FCALLD CPU retirement with far double-push.
$(eval $(call ti_c54x_add_test,cpu_fcalld,cpu/isa/branch,test_cpu_fcalld.c))
# P3-S118 RED contract: FCALA CPU retirement with far double-push.
$(eval $(call ti_c54x_add_test,cpu_fcala,cpu/isa/branch,test_cpu_fcala.c))
# P3-S118 RED contract: FCALAD CPU retirement with far double-push.
$(eval $(call ti_c54x_add_test,cpu_fcalad,cpu/isa/branch,test_cpu_fcalad.c))
# P3-S119 RED contract: FRETE CPU retirement with far double-pop.
$(eval $(call ti_c54x_add_test,cpu_frete,cpu/isa/branch,test_cpu_frete.c))
# P3-S119 RED contract: FRETD CPU retirement with far double-pop.
$(eval $(call ti_c54x_add_test,cpu_fretd,cpu/isa/branch,test_cpu_fretd.c))
