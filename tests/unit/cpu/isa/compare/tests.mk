# Test ownership: this leaf owns public-CPU CMPR compare proofs.
$(eval $(call ti_c54x_add_test,cpu_cmpr,cpu/isa/compare,test_cpu_cmpr.c))
# Test ownership: this leaf owns public-CPU BIT Xmem compare proofs.
$(eval $(call ti_c54x_add_test,cpu_bit_xmem,cpu/isa/compare,test_cpu_bit_xmem.c))
