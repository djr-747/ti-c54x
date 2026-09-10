# Test ownership: tests/unit/isa/ops/branch owns pure branch semantic effects.
ti_c54x_add_unit_test(b_pmad test_b_pmad.c)
ti_c54x_add_unit_test(bc_pmad test_bc_pmad.c)
ti_c54x_add_unit_test(call_pmad test_call_pmad.c)
ti_c54x_add_unit_test(ret test_ret.c)
# W67B RED contract: RETD effects.
ti_c54x_add_unit_test(retd test_retd.c)
# W67C RED contract: CALLD effects.
ti_c54x_add_unit_test(calld test_calld.c)
