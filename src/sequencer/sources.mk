# Source ownership: src/sequencer owns ordered effect validation/application.
ti_c54x_core_sources += \
	src/sequencer/address_effect_intrinsic.c \
	src/sequencer/address_effect_dependencies.c \
	src/sequencer/address_effect_validate.c \
	src/sequencer/address_effect_apply.c \
	src/sequencer/sequential_pc.c \
	src/sequencer/program_pc_bridge.c
include src/sequencer/repeat/sources.mk
include src/sequencer/status/sources.mk
