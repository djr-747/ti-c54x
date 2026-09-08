# Source ownership: src/sequencer/instruction owns persistent root memory
# instruction transactions.
ti_c54x_core_sources += \
	src/sequencer/instruction/memory_instruction_candidate_validate.c \
	src/sequencer/instruction/memory_instruction_rmw.c \
	src/sequencer/instruction/memory_instruction_transaction.c \
	src/sequencer/instruction/long32_pair_transaction.c \
	src/sequencer/instruction/long32_pair_transaction_validate.c
