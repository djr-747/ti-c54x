# Source ownership: src/sequencer/instruction owns persistent root memory
# instruction transactions.
ti_c54x_core_sources += \
	src/sequencer/instruction/memory_instruction_candidate_validate.c \
	src/sequencer/instruction/memory_candidate_direct.c \
	src/sequencer/instruction/memory_candidate_single_leg.c \
	src/sequencer/instruction/memory_instruction_commit.c \
	src/sequencer/instruction/memory_instruction_data_program_candidate_validate.c \
	src/sequencer/instruction/memory_instruction_data_program_commit.c \
	src/sequencer/instruction/memory_instruction_dual_candidate_validate.c \
	src/sequencer/instruction/memory_instruction_dual_commit.c \
	src/sequencer/instruction/memory_instruction_rmw.c \
	src/sequencer/instruction/memory_instruction_stack_push.c \
	src/sequencer/instruction/memory_instruction_transaction.c \
	src/sequencer/instruction/memory_instruction_long32.c \
	src/sequencer/instruction/memory_instruction_dual.c \
	src/sequencer/instruction/memory_instruction_data_program.c \
	src/sequencer/instruction/memory_instruction_port.c \
	src/sequencer/instruction/memory_instruction_port_candidate_validate.c \
	src/sequencer/instruction/memory_instruction_pair_dispatch.c \
	src/sequencer/instruction/data_program_word16_read_transaction.c \
	src/sequencer/instruction/dual_word16_read_transaction.c \
	src/sequencer/instruction/long32_pair_transaction.c \
	src/sequencer/instruction/long32_pair_transaction_validate.c
