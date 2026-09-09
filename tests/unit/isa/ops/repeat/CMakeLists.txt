# Test ownership: tests/unit/isa/ops/repeat owns immediate-RPT effects.
ti_c54x_add_unit_test(rpt_immediate_effect test_rpt_immediate_effect.c)
# W52A COMMON repeat-truth closure: TI-evidence-bound RPT operation contract.
ti_c54x_add_unit_test(rpt_immediate_operation_common test_rpt_immediate.c)
