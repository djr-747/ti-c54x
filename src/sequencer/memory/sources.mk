# Source ownership: src/sequencer/memory owns generic memory resolution.
ti_c54x_core_sources += \
	src/sequencer/memory/address_resolution.c \
	src/sequencer/memory/address_resolution_absolute_data.c \
	src/sequencer/memory/address_resolution_atomic_absolute.c \
	src/sequencer/memory/address_resolution_long32_absolute_store.c \
	src/sequencer/memory/address_resolution_long32_mod13.c \
	src/sequencer/memory/dual_address_resolution.c \
	src/sequencer/memory/data_program_address_resolution.c \
	src/sequencer/memory/mmr_access_plan.c \
	src/sequencer/memory/port_access_plan.c \
	src/sequencer/memory/long32_access_plan.c
