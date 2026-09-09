# Test ownership: tests/unit/isa/ops/repeat owns immediate-RPT effects.
$(eval $(call ti_c54x_add_test,rpt_immediate_effect,isa/ops/repeat,test_rpt_immediate_effect.c))
# W52A COMMON repeat-truth closure: TI-evidence-bound RPT operation contract.
$(eval $(call ti_c54x_add_test,rpt_immediate_operation_common,isa/ops/repeat,test_rpt_immediate.c))
