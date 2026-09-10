# Test ownership: tests/unit/isa/ops/branch owns pure branch semantic effects.
ti_c54x_add_unit_test(b_pmad test_b_pmad.c)
# W68 RED contract: BD effects.
ti_c54x_add_unit_test(bd test_bd.c)
ti_c54x_add_unit_test(bc_pmad test_bc_pmad.c)
# W69 RED contract: BCD effects.
ti_c54x_add_unit_test(bcd test_bcd.c)
# W70 RED contract: BANZ effects.
ti_c54x_add_unit_test(banz test_banz.c)
ti_c54x_add_unit_test(call_pmad test_call_pmad.c)
ti_c54x_add_unit_test(ret test_ret.c)
# W67B RED contract: RETD effects.
ti_c54x_add_unit_test(retd test_retd.c)
# W67C RED contract: CALLD effects.
ti_c54x_add_unit_test(calld test_calld.c)
