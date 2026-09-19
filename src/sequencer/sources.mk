# Source ownership: src/sequencer owns ordered effect validation/application.
ti_c54x_core_sources += \
	src/sequencer/address_effect_intrinsic.c \
	src/sequencer/address_effect_dependencies.c \
	src/sequencer/address_effect_validate.c \
	src/sequencer/address_effect_apply.c \
	src/sequencer/sequential_pc.c \
	src/sequencer/program_pc_bridge.c \
	src/sequencer/effect_transaction.c \
	src/sequencer/program_pc_target.c
include src/sequencer/delayed_branch/sources.mk
include src/sequencer/repeat/sources.mk
include src/sequencer/interrupt/sources.mk
include src/sequencer/status/sources.mk
include src/sequencer/accumulator/sources.mk
include src/sequencer/temporary/sources.mk
include src/sequencer/stack/sources.mk
include src/sequencer/auxiliary/sources.mk
include src/sequencer/memory/sources.mk
include src/sequencer/instruction/sources.mk
