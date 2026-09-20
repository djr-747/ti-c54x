# Test ownership: this leaf owns public-CPU syntax-6 dual-MOD0 ADD/SUB proofs.
$(eval $(call ti_c54x_add_test,cpu_add_sub_dual_mod0,cpu/isa/arithmetic,test_cpu_add_sub_dual_mod0.c))
# W51C RED contract: public-CPU non-MOD0/MOD0 dual ADD/SUB retirement.
$(eval $(call ti_c54x_add_test,cpu_add_sub_dual_modes,cpu/isa/arithmetic,test_cpu_add_sub_dual_modes.c))
# W52C RED contract: public-CPU MOD3-involving dual ADD/SUB retirement.
$(eval $(call ti_c54x_add_test,cpu_add_sub_dual_mod3,cpu/isa/arithmetic,test_cpu_add_sub_dual_mod3.c))
# P3-S3z RED contract: public-CPU ADD Smem syntaxes 1-3 retirement.
$(eval $(call ti_c54x_add_test,cpu_add_smem,cpu/isa/arithmetic,test_cpu_add_smem.c))
# P3-S82 RED contract: public-CPU SUB absolute Smem retirement.
$(eval $(call ti_c54x_add_test,cpu_sub_smem_absolute,cpu/isa/arithmetic,test_cpu_sub_smem_absolute.c))
$(eval $(call ti_c54x_add_test,cpu_dadd,cpu/isa/arithmetic,test_cpu_dadd.c))
$(eval $(call ti_c54x_add_test,cpu_dadst,cpu/isa/arithmetic,test_cpu_dadst.c))
$(eval $(call ti_c54x_add_test,cpu_drsub,cpu/isa/arithmetic,test_cpu_drsub.c))
$(eval $(call ti_c54x_add_test,cpu_dsadt,cpu/isa/arithmetic,test_cpu_dsadt.c))
$(eval $(call ti_c54x_add_test,cpu_dsub,cpu/isa/arithmetic,test_cpu_dsub.c))
$(eval $(call ti_c54x_add_test,cpu_dsubt,cpu/isa/arithmetic,test_cpu_dsubt.c))
