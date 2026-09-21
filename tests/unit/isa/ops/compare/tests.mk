$(eval $(call ti_c54x_add_test,max_min_compare,isa/ops/compare,test_max_min.c))
$(eval $(call ti_c54x_add_test,cmpr_compare,isa/ops/compare,test_cmpr.c))
# S36 RED contract: BITF/CMPM Smem-lk and BITT Smem TC effects.
$(eval $(call ti_c54x_add_test,bitf_compare,isa/ops/compare,test_bitf_smem_lk.c))
$(eval $(call ti_c54x_add_test,cmpm_compare,isa/ops/compare,test_cmpm_smem_lk.c))
$(eval $(call ti_c54x_add_test,bitt_compare,isa/ops/compare,test_bitt_smem.c))
$(eval $(call ti_c54x_add_test,bit_xmem_compare,isa/ops/compare,test_bit_xmem.c))
