# Test ownership: this leaf owns public-CPU SSBX/RSBX status-bit proofs.
$(eval $(call ti_c54x_add_test,cpu_ssbx_rsbx,cpu/isa/status,test_cpu_ssbx_rsbx.c))
$(eval $(call ti_c54x_add_test,cpu_interrupt_snapshot,cpu/isa/status,test_cpu_interrupt_snapshot.c))
$(eval $(call ti_c54x_add_test,cpu_interrupt_arrival,cpu/isa/status,test_cpu_interrupt_arrival.c))
$(eval $(call ti_c54x_add_test,cpu_nmi_take,cpu/isa/status,test_cpu_nmi_take.c))
$(eval $(call ti_c54x_add_test,cpu_maskable_take,cpu/isa/status,test_cpu_maskable_take.c))
$(eval $(call ti_c54x_add_test,cpu_hpint_hop,cpu/isa/status,test_cpu_hpint_hop.c))
