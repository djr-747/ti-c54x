# Test ownership: this leaf owns only public-CPU MAR proofs.
# P3-S77 RED contract: MAR Smem postdecrement retirement.
$(eval $(call ti_c54x_add_test,cpu_mar_smem,cpu/isa/auxiliary,test_cpu_mar_smem.c))
