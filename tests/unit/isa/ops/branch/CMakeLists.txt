# Test ownership: tests/unit/isa/ops/branch owns pure branch semantic effects.
ti_c54x_add_unit_test(b_pmad test_b_pmad.c)
# W68 RED contract: BD effects.
ti_c54x_add_unit_test(bd test_bd.c)
ti_c54x_add_unit_test(bc_pmad test_bc_pmad.c)
# W69 RED contract: BCD effects.
ti_c54x_add_unit_test(bcd test_bcd.c)
# W70 RED contract: BANZ effects.
ti_c54x_add_unit_test(banz test_banz.c)
# W71 RED contract: BANZD effects.
ti_c54x_add_unit_test(banzd test_banzd.c)
# W72 RED contract: CCD effects.
ti_c54x_add_unit_test(ccd test_ccd.c)
# W73 RED contract: CC effects.
ti_c54x_add_unit_test(cc test_cc.c)
ti_c54x_add_unit_test(call_pmad test_call_pmad.c)
ti_c54x_add_unit_test(ret test_ret.c)
# W74 RED contract: RETE effects.
ti_c54x_add_unit_test(rete test_rete.c)
# W75 RED contract: RETED effects.
ti_c54x_add_unit_test(reted test_reted.c)
# W76 RED contract: RETF effects and parity.
ti_c54x_add_unit_test(retf test_retf.c)
# W77 RED contract: RC effects.
ti_c54x_add_unit_test(rc test_rc.c)
# W78 RED contract: RCD effects.
ti_c54x_add_unit_test(rcd test_rcd.c)
# P3-S4 RED contract: XC effects.
ti_c54x_add_unit_test(xc test_xc.c)
# P3 slice 4 RED contracts: BACC/BACCD effects.
ti_c54x_add_unit_test(bacc test_bacc.c)
ti_c54x_add_unit_test(baccd test_baccd.c)
# P3-S117 RED contracts: FB/FBD/FBACC/FBACCD effects.
ti_c54x_add_unit_test(fb_pmad test_fb_pmad.c)
ti_c54x_add_unit_test(fbd test_fbd.c)
ti_c54x_add_unit_test(fbacc test_fbacc.c)
ti_c54x_add_unit_test(fbaccd test_fbaccd.c)
# P3 slice 9 RED contracts: CALA/CALAD effects.
ti_c54x_add_unit_test(cala test_cala.c)
ti_c54x_add_unit_test(calad test_calad.c)
# W67B RED contract: RETD effects.
ti_c54x_add_unit_test(retd test_retd.c)
# W67C RED contract: CALLD effects.
ti_c54x_add_unit_test(calld test_calld.c)
