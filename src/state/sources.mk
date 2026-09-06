# Source ownership: src/state owns architectural register storage/accessors.
ti_c54x_core_sources += \
	src/state/arch_state.c \
	src/state/block_size_register.c
include src/state/status/sources.mk
include src/state/accumulator/sources.mk
