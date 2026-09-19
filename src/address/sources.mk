# Source ownership: src/address owns effective-address and operand planning.
ti_c54x_core_sources += \
	src/address/absolute.c \
	src/address/data_operand.c \
	src/address/direct_dp.c \
	src/address/direct_page0.c \
	src/address/direct_sp.c \
	src/address/indirect_single.c \
	src/address/indirect_indexed.c \
	src/address/indirect_long_offset.c \
	src/address/indirect_dual.c \
	src/address/indirect_dual_mod3.c \
	src/address/indirect_reverse_carry.c \
	src/address/indirect_circular.c \
	src/address/stack_push.c \
	src/address/stack_pop.c
