# Test ownership: this leaf owns CPU-level OR-immediate proofs only.
$(eval $(call ti_c54x_add_test,cpu_or_immediate,cpu/isa/logical/immediate/or,test_cpu_or.c))
