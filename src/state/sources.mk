# Source ownership: src/state owns architectural register storage/accessors.
ti_c54x_core_sources += \
	src/state/arch_state.c \
	src/state/block_size_register.c \
	src/state/program_counter_extension.c \
	src/state/repeat_counter_register.c \
	src/state/interrupt_flag_register.c \
	src/state/mmr_file.c
include src/state/status/sources.mk
include src/state/accumulator/sources.mk
include src/state/temporary/sources.mk
