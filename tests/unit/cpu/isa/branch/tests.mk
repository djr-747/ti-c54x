# Test ownership: this leaf owns only public-CPU branch proofs.
$(eval $(call ti_c54x_add_test,cpu_b_pmad,cpu/isa/branch,test_cpu_b_pmad.c))
# W68 RED contract: BD CPU retirement.
$(eval $(call ti_c54x_add_test,cpu_bd,cpu/isa/branch,test_cpu_bd.c))
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
$(eval $(call ti_c54x_add_test,cpu_rete,cpu/isa/branch,test_cpu_rete.c))
# W67B RED contract: RETD CPU retirement.
$(eval $(call ti_c54x_add_test,cpu_retd,cpu/isa/branch,test_cpu_retd.c))
# W67C RED contract: CALLD CPU retirement.
$(eval $(call ti_c54x_add_test,cpu_calld,cpu/isa/branch,test_cpu_calld.c))
