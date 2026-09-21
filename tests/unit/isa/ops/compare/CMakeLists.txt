ti_c54x_add_unit_test(max_min_compare test_max_min.c)
ti_c54x_add_unit_test(cmpr_compare test_cmpr.c)
# S36 RED contract: BITF/CMPM Smem-lk and BITT Smem TC effects.
ti_c54x_add_unit_test(bitf_compare test_bitf_smem_lk.c)
ti_c54x_add_unit_test(cmpm_compare test_cmpm_smem_lk.c)
ti_c54x_add_unit_test(bitt_compare test_bitt_smem.c)
ti_c54x_add_unit_test(bit_xmem_compare test_bit_xmem.c)
