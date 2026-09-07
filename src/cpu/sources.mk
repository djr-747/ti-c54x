# Source ownership: src/cpu owns lifecycle, fetch collection, and orchestration.
ti_c54x_core_sources += src/cpu/cpu.c
ti_c54x_core_sources += src/cpu/instruction_fetch.c
include src/cpu/input/sources.mk
include src/cpu/memory/sources.mk
include src/cpu/execution/sources.mk
include src/cpu/trace/sources.mk
