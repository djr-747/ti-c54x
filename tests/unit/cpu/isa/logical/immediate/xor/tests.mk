# Test ownership: this leaf owns CPU-level XOR immediate proofs only.
$(eval $(call ti_c54x_add_test,cpu_xor_immediate,cpu/isa/logical/immediate/xor,test_cpu_xor.c))
