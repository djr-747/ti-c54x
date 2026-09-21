# Source ownership: CPU execution owns step/run and the ordinary fetch loop.
ti_c54x_core_sources += \
	src/cpu/execution/run.c \
	src/cpu/execution/repeat_activate.c \
	src/cpu/execution/retire_pc_write.c \
	src/cpu/execution/run_support.c \
	src/cpu/execution/take_issue.c
