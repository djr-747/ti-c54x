# Test ownership: this leaf owns only public-CPU fixed-MOD2 STL proofs.
$(eval $(call ti_c54x_add_test,cpu_stl_smem,cpu/isa/store,test_cpu_stl_smem.c))
# P3 slice 10 RED contract: STH CPU retirement.
$(eval $(call ti_c54x_add_test,cpu_sth_smem,cpu/isa/store,test_cpu_sth_smem.c))
# P3 slice 11 RED contract: STH-by-ASM CPU retirement.
$(eval $(call ti_c54x_add_test,cpu_sth_asm_smem,cpu/isa/store,test_cpu_sth_asm_smem.c))
# P3 slice 12 RED contract: STH-by-SHFT CPU retirement.
$(eval $(call ti_c54x_add_test,cpu_sth_shft_xmem,cpu/isa/store,test_cpu_sth_shft_xmem.c))
$(eval $(call ti_c54x_add_test,cpu_st_immediate_smem,cpu/isa/store,test_cpu_st_immediate_smem.c))
# P3-S7 RED contract: STM CPU retirement.
$(eval $(call ti_c54x_add_test,cpu_stm,cpu/isa/store,test_cpu_stm.c))
$(eval $(call ti_c54x_add_test,cpu_st_t_smem,cpu/isa/store,test_cpu_st_t_smem.c))
