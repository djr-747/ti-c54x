# Source ownership: src/sequencer/interrupt owns repeat-pending request state.
ti_c54x_core_sources += \
	src/sequencer/interrupt/repeat_pending_request.c \
	src/sequencer/interrupt/take_arbitration.c \
	src/sequencer/interrupt/take_complete.c
