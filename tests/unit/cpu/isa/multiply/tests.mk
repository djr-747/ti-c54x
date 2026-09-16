# Test ownership: this leaf owns only public-CPU MAC retirement proofs.
$(eval $(call ti_c54x_add_test,cpu_mac_smem,cpu/isa/multiply,test_cpu_mac_smem.c))
$(eval $(call ti_c54x_add_test,cpu_mpy_smem,cpu/isa/multiply,test_cpu_mpy_smem.c))
$(eval $(call ti_c54x_add_test,cpu_mpya_smem,cpu/isa/multiply,test_cpu_mpya_smem.c))
$(eval $(call ti_c54x_add_test,cpu_mas_smem,cpu/isa/multiply,test_cpu_mas_smem.c))
$(eval $(call ti_c54x_add_test,cpu_mac_dual,cpu/isa/multiply,test_cpu_mac_dual.c))
$(eval $(call ti_c54x_add_test,cpu_mpy_dual,cpu/isa/multiply,test_cpu_mpy_dual.c))
