# Test ownership: tests/unit/isa/ops/branch owns pure branch semantic effects.
$(eval $(call ti_c54x_add_test,b_pmad,isa/ops/branch,test_b_pmad.c))
# W68 RED contract: BD effects.
$(eval $(call ti_c54x_add_test,bd,isa/ops/branch,test_bd.c))
$(eval $(call ti_c54x_add_test,bc_pmad,isa/ops/branch,test_bc_pmad.c))
# W69 RED contract: BCD effects.
$(eval $(call ti_c54x_add_test,bcd,isa/ops/branch,test_bcd.c))
# W70 RED contract: BANZ effects.
$(eval $(call ti_c54x_add_test,banz,isa/ops/branch,test_banz.c))
# W71 RED contract: BANZD effects.
$(eval $(call ti_c54x_add_test,banzd,isa/ops/branch,test_banzd.c))
# W72 RED contract: CCD effects.
$(eval $(call ti_c54x_add_test,ccd,isa/ops/branch,test_ccd.c))
# W73 RED contract: CC effects.
$(eval $(call ti_c54x_add_test,cc,isa/ops/branch,test_cc.c))
$(eval $(call ti_c54x_add_test,call_pmad,isa/ops/branch,test_call_pmad.c))
$(eval $(call ti_c54x_add_test,ret,isa/ops/branch,test_ret.c))
# W74 RED contract: RETE effects.
# W75 RED contract: RETED effects.
$(eval $(call ti_c54x_add_test,reted,isa/ops/branch,test_reted.c))
# W76 RED contract: RETF effects and parity.
$(eval $(call ti_c54x_add_test,retf,isa/ops/branch,test_retf.c))
# W77 RED contract: RC effects.
$(eval $(call ti_c54x_add_test,rc,isa/ops/branch,test_rc.c))
# W78 RED contract: RCD effects.
$(eval $(call ti_c54x_add_test,rcd,isa/ops/branch,test_rcd.c))
# P3-S4 RED contract: XC effects.
$(eval $(call ti_c54x_add_test,xc,isa/ops/branch,test_xc.c))
# P3 slice 4 RED contracts: BACC/BACCD effects.
$(eval $(call ti_c54x_add_test,bacc,isa/ops/branch,test_bacc.c))
$(eval $(call ti_c54x_add_test,baccd,isa/ops/branch,test_baccd.c))
# P3-S117 RED contracts: FB/FBD/FBACC/FBACCD effects.
$(eval $(call ti_c54x_add_test,fb_pmad,isa/ops/branch,test_fb_pmad.c))
$(eval $(call ti_c54x_add_test,fbd,isa/ops/branch,test_fbd.c))
$(eval $(call ti_c54x_add_test,fbacc,isa/ops/branch,test_fbacc.c))
$(eval $(call ti_c54x_add_test,fbaccd,isa/ops/branch,test_fbaccd.c))
# P3 slice 9 RED contracts: CALA/CALAD effects.
$(eval $(call ti_c54x_add_test,cala,isa/ops/branch,test_cala.c))
$(eval $(call ti_c54x_add_test,calad,isa/ops/branch,test_calad.c))
$(eval $(call ti_c54x_add_test,rete,isa/ops/branch,test_rete.c))
# W67B RED contract: RETD effects.
$(eval $(call ti_c54x_add_test,retd,isa/ops/branch,test_retd.c))
# W67C RED contract: CALLD effects.
$(eval $(call ti_c54x_add_test,calld,isa/ops/branch,test_calld.c))
$(eval $(call ti_c54x_add_test,fcall_pmad,isa/ops/branch,test_fcall_pmad.c))
$(eval $(call ti_c54x_add_test,fcalld,isa/ops/branch,test_fcalld.c))
$(eval $(call ti_c54x_add_test,fcala,isa/ops/branch,test_fcala.c))
$(eval $(call ti_c54x_add_test,fcalad,isa/ops/branch,test_fcalad.c))
