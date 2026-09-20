# Test ownership: this leaf owns only public-CPU fixed-MOD2 STL proofs.
$(eval $(call ti_c54x_add_test,cpu_stl_smem,cpu/isa/store,test_cpu_stl_smem.c))
# P3 slice 10 RED contract: STH CPU retirement.
$(eval $(call ti_c54x_add_test,cpu_sth_smem,cpu/isa/store,test_cpu_sth_smem.c))
# P3-S83 RED contract: STH absolute CPU retirement.
$(eval $(call ti_c54x_add_test,cpu_sth_smem_absolute,cpu/isa/store,test_cpu_sth_smem_absolute.c))
# P3 slice 11 RED contract: STH-by-ASM CPU retirement.
$(eval $(call ti_c54x_add_test,cpu_sth_asm_smem,cpu/isa/store,test_cpu_sth_asm_smem.c))
# P3 slice 12 RED contract: STH-by-SHFT CPU retirement.
$(eval $(call ti_c54x_add_test,cpu_sth_shft_xmem,cpu/isa/store,test_cpu_sth_shft_xmem.c))
$(eval $(call ti_c54x_add_test,cpu_stl_shft_xmem,cpu/isa/store,test_cpu_stl_shft_xmem.c))
# P3 slice 13 RED contract: STH-by-SHIFT direct CPU retirement.
$(eval $(call ti_c54x_add_test,cpu_sth_shift_direct,cpu/isa/store,test_cpu_sth_shift_direct.c))
$(eval $(call ti_c54x_add_test,cpu_st_immediate_smem,cpu/isa/store,test_cpu_st_immediate_smem.c))
$(eval $(call ti_c54x_add_test,cpu_mvmd,cpu/isa/store,test_cpu_mvmd.c))
$(eval $(call ti_c54x_add_test,cpu_mvmm,cpu/isa/store,test_cpu_mvmm.c))
$(eval $(call ti_c54x_add_test,cpu_mvdk,cpu/isa/store,test_cpu_mvdk.c))
$(eval $(call ti_c54x_add_test,cpu_mvkd,cpu/isa/store,test_cpu_mvkd.c))
$(eval $(call ti_c54x_add_test,cpu_mvpd,cpu/isa/store,test_cpu_mvpd.c))
$(eval $(call ti_c54x_add_test,cpu_mvdp,cpu/isa/store,test_cpu_mvdp.c))
$(eval $(call ti_c54x_add_test,cpu_mvdp_smem_absolute,cpu/isa/store,test_cpu_mvdp_smem_absolute.c))
$(eval $(call ti_c54x_add_test,cpu_mvpd_smem_absolute,cpu/isa/store,test_cpu_mvpd_smem_absolute.c))
$(eval $(call ti_c54x_add_test,cpu_dst_smem_absolute,cpu/isa/store,test_cpu_dst_smem_absolute.c))
$(eval $(call ti_c54x_add_test,cpu_reada,cpu/isa/store,test_cpu_reada.c))
$(eval $(call ti_c54x_add_test,cpu_writa,cpu/isa/store,test_cpu_writa.c))
$(eval $(call ti_c54x_add_test,cpu_mvdm,cpu/isa/store,test_cpu_mvdm.c))
# P3-S7 RED contract: STM CPU retirement.
$(eval $(call ti_c54x_add_test,cpu_stm,cpu/isa/store,test_cpu_stm.c))
# P3-S67 RED contract: SWWSR peripheral-cell CPU round trip.
$(eval $(call ti_c54x_add_test,cpu_swwsr,cpu/isa/store,test_cpu_swwsr.c))
# P3-S68 RED contract: CLKMD peripheral-cell CPU retirement.
$(eval $(call ti_c54x_add_test,cpu_clkmd,cpu/isa/store,test_cpu_clkmd.c))
# P3-S70 RED contract: absolute ST-lk CPU retirement.
$(eval $(call ti_c54x_add_test,cpu_st_lk_absolute,cpu/isa/store,test_cpu_st_lk_absolute.c))
$(eval $(call ti_c54x_add_test,cpu_st_t_smem,cpu/isa/store,test_cpu_st_t_smem.c))
$(eval $(call ti_c54x_add_test,cpu_cmps_smem_direct,cpu/isa/store,test_cpu_cmps_smem_direct.c))
