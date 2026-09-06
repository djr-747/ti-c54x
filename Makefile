.PHONY: all build test quality-test cmake-build install clean format

# The default path is deliberately independent of CMake and the emulator. It
# gives the foundations a quick compile/link/run loop while CMake remains the
# packaging and IDE/CI project generator.
CC ?= cc
AR ?= ar
PYTHON ?= python3
NM ?= nm
override BUILD_DIR := build-fast
ARFLAGS := rcs

CPPFLAGS += -Iinclude -Isrc
CFLAGS += -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion \
	-Wshadow -Wundef -Wdouble-promotion -Werror

ifeq ($(TI_C54X_ENABLE_SANITIZERS),1)
CFLAGS += -g -fsanitize=address,undefined -fno-omit-frame-pointer
LDFLAGS += -fsanitize=address,undefined
endif

ifeq ($(TI_C54X_ENABLE_OBSERVERS),1)
CPPFLAGS += -DTI_C54X_ENABLE_OBSERVERS=1
else
CPPFLAGS += -DTI_C54X_ENABLE_OBSERVERS=0
endif

LIB := $(BUILD_DIR)/libti_c54x.a
OBSERVER_OFF_DIR := $(BUILD_DIR)/observer-off
OBSERVER_OFF_LIB := $(OBSERVER_OFF_DIR)/libti_c54x.a
LIB_OBJECTS := \
	$(BUILD_DIR)/obj/foundation/arithmetic40.o \
	$(BUILD_DIR)/obj/foundation/reverse_carry.o \
	$(BUILD_DIR)/obj/address/data_operand.o \
	$(BUILD_DIR)/obj/address/direct_dp.o \
	$(BUILD_DIR)/obj/address/direct_sp.o \
	$(BUILD_DIR)/obj/address/indirect_single.o \
	$(BUILD_DIR)/obj/address/indirect_indexed.o \
	$(BUILD_DIR)/obj/address/indirect_long_offset.o \
	$(BUILD_DIR)/obj/address/indirect_reverse_carry.o \
	$(BUILD_DIR)/obj/address/indirect_circular.o \
	$(BUILD_DIR)/obj/sequencer/address_effect_intrinsic.o \
	$(BUILD_DIR)/obj/sequencer/address_effect_dependencies.o \
	$(BUILD_DIR)/obj/sequencer/address_effect_validate.o \
	$(BUILD_DIR)/obj/sequencer/address_effect_apply.o \
	$(BUILD_DIR)/obj/state/arch_state.o \
	$(BUILD_DIR)/obj/state/block_size_register.o \
	$(BUILD_DIR)/obj/cpu/cpu.o \
	$(BUILD_DIR)/obj/debug/observer.o
CPU_TEST := $(BUILD_DIR)/test_cpu_api
ARITHMETIC_TEST := $(BUILD_DIR)/test_arithmetic40
REVERSE_CARRY_TEST := $(BUILD_DIR)/test_reverse_carry
REVERSE_CARRY_CONTRACT_TEST := $(BUILD_DIR)/test_reverse_carry_contract
DIRECT_DP_TEST := $(BUILD_DIR)/test_direct_dp
DIRECT_SP_TEST := $(BUILD_DIR)/test_direct_sp
DIRECT_ADDRESS_CONTRACT_TEST := $(BUILD_DIR)/test_direct_address_contract
DIRECT_SOURCE_APPLY_TEST := $(BUILD_DIR)/test_direct_source_apply
DIRECT_SOURCE_APPLY_CONTRACT_TEST := $(BUILD_DIR)/test_direct_source_apply_contract
INDIRECT_SINGLE_WORD_TEST := $(BUILD_DIR)/test_indirect_single_word
DATA_OPERAND_TEST := $(BUILD_DIR)/test_data_operand
DATA_OPERAND_CONTRACT_TEST := $(BUILD_DIR)/test_data_operand_contract
INDIRECT_SINGLE_LONG32_TEST := $(BUILD_DIR)/test_indirect_single_long32
INDIRECT_INDEXED_WORD_TEST := $(BUILD_DIR)/test_indirect_indexed_word
INDIRECT_INDEXED_CONTRACT_TEST := $(BUILD_DIR)/test_indirect_indexed_contract
INDIRECT_LONG_OFFSET_MOD13_TEST := $(BUILD_DIR)/test_indirect_long_offset_mod13
INDIRECT_LONG_OFFSET_CONTRACT_TEST := $(BUILD_DIR)/test_indirect_long_offset_contract
INDIRECT_REVERSE_CARRY_WORD_TEST := $(BUILD_DIR)/test_indirect_reverse_carry_word
INDIRECT_REVERSE_CARRY_CONTRACT_TEST := $(BUILD_DIR)/test_indirect_reverse_carry_contract
ADDRESS_EFFECT_CONTRACT_TEST := $(BUILD_DIR)/test_address_effect_contract
ARCH_STATE_RESET_TEST := $(BUILD_DIR)/test_arch_state_reset
ARCH_STATE_KNOWNNESS_TEST := $(BUILD_DIR)/test_arch_state_knownness
ARCH_STATE_ADDRESS_FIELDS_TEST := $(BUILD_DIR)/test_arch_state_address_fields
ARCH_STATE_ADDRESS_FIELD_CONTRACT_TEST := $(BUILD_DIR)/test_arch_state_address_field_contract
BLOCK_SIZE_REGISTER_TEST := $(BUILD_DIR)/test_block_size_register
BLOCK_SIZE_REGISTER_CONTRACT_TEST := $(BUILD_DIR)/test_block_size_register_contract
ADDRESS_EFFECT_APPLY_TEST := $(BUILD_DIR)/test_address_effect_apply
ADDRESS_EFFECT_APPLY_CONTRACT_TEST := $(BUILD_DIR)/test_address_effect_apply_contract
ADDRESS_EFFECT_VALIDATE_CONTRACT_TEST := $(BUILD_DIR)/test_address_effect_validate_contract
ADDRESS_EFFECT_DEPENDENCY_CONTRACT_TEST := $(BUILD_DIR)/test_address_effect_dependency_contract
INDIRECT_CMPT0_STATE_TEST := $(BUILD_DIR)/test_indirect_cmpt0_state
ADDRESS_STATE_POLICY_CONTRACT_TEST := $(BUILD_DIR)/test_address_state_policy_contract
INDIRECT_LONG32_EFFECT_APPLY_TEST := $(BUILD_DIR)/test_indirect_long32_effect_apply
INDIRECT_INDEXED_EFFECT_APPLY_TEST := $(BUILD_DIR)/test_indirect_indexed_effect_apply
INDIRECT_INDEXED_EFFECT_APPLY_CONTRACT_TEST := $(BUILD_DIR)/test_indirect_indexed_effect_apply_contract
INDIRECT_LONG_OFFSET_EFFECT_APPLY_TEST := $(BUILD_DIR)/test_indirect_long_offset_effect_apply
INDIRECT_LONG_OFFSET_EFFECT_APPLY_CONTRACT_TEST := $(BUILD_DIR)/test_indirect_long_offset_effect_apply_contract
INDIRECT_REVERSE_CARRY_EFFECT_APPLY_TEST := $(BUILD_DIR)/test_indirect_reverse_carry_effect_apply
INDIRECT_REVERSE_CARRY_EFFECT_APPLY_CONTRACT_TEST := $(BUILD_DIR)/test_indirect_reverse_carry_effect_apply_contract
INDIRECT_CIRCULAR_WORD_TEST := $(BUILD_DIR)/test_indirect_circular_word
INDIRECT_CIRCULAR_CONTRACT_TEST := $(BUILD_DIR)/test_indirect_circular_contract
INDIRECT_CIRCULAR_EFFECT_APPLY_TEST := $(BUILD_DIR)/test_indirect_circular_effect_apply
INDIRECT_CIRCULAR_EFFECT_APPLY_CONTRACT_TEST := $(BUILD_DIR)/test_indirect_circular_effect_apply_contract
ADDRESS_EFFECT_STATE_CONTRACT_TEST := $(BUILD_DIR)/test_address_effect_state_contract
ADDRESS_OPERAND_APPLY_CONTRACT_TEST := $(BUILD_DIR)/test_address_operand_apply_contract
OBSERVER_TEST := $(BUILD_DIR)/test_observer
OBSERVER_OFF_TEST := $(BUILD_DIR)/test_observer_off
MINIMAL := $(BUILD_DIR)/minimal
OBSERVER_OFF_OBJECTS := \
	$(OBSERVER_OFF_DIR)/obj/address/data_operand.o \
	$(OBSERVER_OFF_DIR)/obj/address/direct_dp.o \
	$(OBSERVER_OFF_DIR)/obj/address/direct_sp.o \
	$(OBSERVER_OFF_DIR)/obj/address/indirect_single.o \
	$(OBSERVER_OFF_DIR)/obj/address/indirect_indexed.o \
	$(OBSERVER_OFF_DIR)/obj/address/indirect_long_offset.o \
	$(OBSERVER_OFF_DIR)/obj/address/indirect_reverse_carry.o \
	$(OBSERVER_OFF_DIR)/obj/address/indirect_circular.o \
	$(OBSERVER_OFF_DIR)/obj/sequencer/address_effect_intrinsic.o \
	$(OBSERVER_OFF_DIR)/obj/sequencer/address_effect_dependencies.o \
	$(OBSERVER_OFF_DIR)/obj/sequencer/address_effect_validate.o \
	$(OBSERVER_OFF_DIR)/obj/sequencer/address_effect_apply.o \
	$(OBSERVER_OFF_DIR)/obj/foundation/arithmetic40.o \
	$(OBSERVER_OFF_DIR)/obj/foundation/reverse_carry.o \
	$(OBSERVER_OFF_DIR)/obj/state/arch_state.o \
	$(OBSERVER_OFF_DIR)/obj/state/block_size_register.o \
	$(OBSERVER_OFF_DIR)/obj/cpu/cpu.o \
	$(OBSERVER_OFF_DIR)/obj/debug/observer.o

all: test

build: $(CPU_TEST) $(ARITHMETIC_TEST) $(REVERSE_CARRY_TEST) \
	$(REVERSE_CARRY_CONTRACT_TEST) $(DIRECT_DP_TEST) $(DIRECT_SP_TEST) \
	$(DIRECT_ADDRESS_CONTRACT_TEST) $(DIRECT_SOURCE_APPLY_TEST) \
	$(DIRECT_SOURCE_APPLY_CONTRACT_TEST) \
	$(INDIRECT_SINGLE_WORD_TEST) $(ADDRESS_EFFECT_CONTRACT_TEST) \
	$(DATA_OPERAND_TEST) $(DATA_OPERAND_CONTRACT_TEST) \
	$(INDIRECT_SINGLE_LONG32_TEST) \
	$(INDIRECT_INDEXED_WORD_TEST) $(INDIRECT_INDEXED_CONTRACT_TEST) \
	$(INDIRECT_LONG_OFFSET_MOD13_TEST) $(INDIRECT_LONG_OFFSET_CONTRACT_TEST) \
	$(INDIRECT_REVERSE_CARRY_WORD_TEST) $(INDIRECT_REVERSE_CARRY_CONTRACT_TEST) \
	$(ARCH_STATE_RESET_TEST) $(ARCH_STATE_KNOWNNESS_TEST) \
	$(ARCH_STATE_ADDRESS_FIELDS_TEST) $(ARCH_STATE_ADDRESS_FIELD_CONTRACT_TEST) \
	$(BLOCK_SIZE_REGISTER_TEST) $(BLOCK_SIZE_REGISTER_CONTRACT_TEST) \
	$(ADDRESS_EFFECT_APPLY_TEST) $(ADDRESS_EFFECT_APPLY_CONTRACT_TEST) \
	$(ADDRESS_EFFECT_VALIDATE_CONTRACT_TEST) \
	$(ADDRESS_EFFECT_DEPENDENCY_CONTRACT_TEST) \
	$(INDIRECT_CMPT0_STATE_TEST) $(ADDRESS_STATE_POLICY_CONTRACT_TEST) \
	$(INDIRECT_LONG32_EFFECT_APPLY_TEST) \
	$(INDIRECT_INDEXED_EFFECT_APPLY_TEST) $(INDIRECT_INDEXED_EFFECT_APPLY_CONTRACT_TEST) \
	$(INDIRECT_LONG_OFFSET_EFFECT_APPLY_TEST) \
	$(INDIRECT_LONG_OFFSET_EFFECT_APPLY_CONTRACT_TEST) \
	$(INDIRECT_REVERSE_CARRY_EFFECT_APPLY_TEST) \
	$(INDIRECT_REVERSE_CARRY_EFFECT_APPLY_CONTRACT_TEST) \
	$(INDIRECT_CIRCULAR_WORD_TEST) \
	$(INDIRECT_CIRCULAR_CONTRACT_TEST) \
	$(INDIRECT_CIRCULAR_EFFECT_APPLY_TEST) \
	$(INDIRECT_CIRCULAR_EFFECT_APPLY_CONTRACT_TEST) \
	$(ADDRESS_EFFECT_STATE_CONTRACT_TEST) \
	$(ADDRESS_OPERAND_APPLY_CONTRACT_TEST) \
	$(OBSERVER_TEST) \
	$(OBSERVER_OFF_TEST) $(MINIMAL) $(OBSERVER_OFF_LIB)

test: build quality-test
	$(CPU_TEST)
	$(ARITHMETIC_TEST)
	$(REVERSE_CARRY_TEST)
	$(REVERSE_CARRY_CONTRACT_TEST)
	$(DIRECT_DP_TEST)
	$(DIRECT_SP_TEST)
	$(DIRECT_ADDRESS_CONTRACT_TEST)
	$(DIRECT_SOURCE_APPLY_TEST)
	$(DIRECT_SOURCE_APPLY_CONTRACT_TEST)
	$(INDIRECT_SINGLE_WORD_TEST)
	$(ADDRESS_EFFECT_CONTRACT_TEST)
	$(DATA_OPERAND_TEST)
	$(DATA_OPERAND_CONTRACT_TEST)
	$(INDIRECT_SINGLE_LONG32_TEST)
	$(INDIRECT_INDEXED_WORD_TEST)
	$(INDIRECT_INDEXED_CONTRACT_TEST)
	$(INDIRECT_LONG_OFFSET_MOD13_TEST)
	$(INDIRECT_LONG_OFFSET_CONTRACT_TEST)
	$(INDIRECT_REVERSE_CARRY_WORD_TEST)
	$(INDIRECT_REVERSE_CARRY_CONTRACT_TEST)
	$(ARCH_STATE_RESET_TEST)
	$(ARCH_STATE_KNOWNNESS_TEST)
	$(ARCH_STATE_ADDRESS_FIELDS_TEST)
	$(ARCH_STATE_ADDRESS_FIELD_CONTRACT_TEST)
	$(BLOCK_SIZE_REGISTER_TEST)
	$(BLOCK_SIZE_REGISTER_CONTRACT_TEST)
	$(ADDRESS_EFFECT_APPLY_TEST)
	$(ADDRESS_EFFECT_APPLY_CONTRACT_TEST)
	$(ADDRESS_EFFECT_VALIDATE_CONTRACT_TEST)
	$(ADDRESS_EFFECT_DEPENDENCY_CONTRACT_TEST)
	$(INDIRECT_CMPT0_STATE_TEST)
	$(ADDRESS_STATE_POLICY_CONTRACT_TEST)
	$(INDIRECT_LONG32_EFFECT_APPLY_TEST)
	$(INDIRECT_INDEXED_EFFECT_APPLY_TEST)
	$(INDIRECT_INDEXED_EFFECT_APPLY_CONTRACT_TEST)
	$(INDIRECT_LONG_OFFSET_EFFECT_APPLY_TEST)
	$(INDIRECT_LONG_OFFSET_EFFECT_APPLY_CONTRACT_TEST)
	$(INDIRECT_REVERSE_CARRY_EFFECT_APPLY_TEST)
	$(INDIRECT_REVERSE_CARRY_EFFECT_APPLY_CONTRACT_TEST)
	$(INDIRECT_CIRCULAR_WORD_TEST)
	$(INDIRECT_CIRCULAR_CONTRACT_TEST)
	$(INDIRECT_CIRCULAR_EFFECT_APPLY_TEST)
	$(INDIRECT_CIRCULAR_EFFECT_APPLY_CONTRACT_TEST)
	$(ADDRESS_EFFECT_STATE_CONTRACT_TEST)
	$(ADDRESS_OPERAND_APPLY_CONTRACT_TEST)
	$(OBSERVER_TEST)
	$(OBSERVER_OFF_TEST)
	$(MINIMAL)

quality-test: $(OBSERVER_OFF_LIB)
	$(PYTHON) -m unittest discover -s tests/contract/quality -t .
	$(PYTHON) tools/quality/check_knobs.py --root .
	$(PYTHON) tools/quality/check_source_boundaries.py --root .
	$(PYTHON) tools/quality/check_evidence.py --root .
	$(PYTHON) tools/quality/check_observer_artifact.py \
		--artifact $(OBSERVER_OFF_LIB) --nm $(NM)

$(LIB): $(LIB_OBJECTS)
	@mkdir -p $(@D)
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DIR)/obj/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(OBSERVER_OFF_DIR)/obj/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -UTI_C54X_ENABLE_OBSERVERS \
		-DTI_C54X_ENABLE_OBSERVERS=0 -MMD -MP -c $< -o $@

$(OBSERVER_OFF_LIB): $(OBSERVER_OFF_OBJECTS)
	@mkdir -p $(@D)
	$(AR) $(ARFLAGS) $@ $^

$(CPU_TEST): tests/unit/cpu/test_cpu_api.c $(LIB)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(ARITHMETIC_TEST): tests/unit/foundation/test_arithmetic40.c $(LIB)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(REVERSE_CARRY_TEST): tests/unit/foundation/test_reverse_carry.c \
	tests/support/test_harness.h $(LIB) src/foundation/reverse_carry.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(REVERSE_CARRY_CONTRACT_TEST): tests/unit/foundation/test_reverse_carry_contract.c \
	tests/support/test_harness.h $(LIB) src/foundation/reverse_carry.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(DIRECT_DP_TEST): tests/unit/address/test_direct_dp.c \
	tests/support/test_harness.h $(LIB) src/address/direct_dp.h \
	include/ti_c54x/address.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(DIRECT_SP_TEST): tests/unit/address/test_direct_sp.c \
	tests/support/test_harness.h $(LIB) src/address/direct_sp.h \
	src/address/direct_dp.h src/address/address_effect.h include/ti_c54x/address.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(DIRECT_ADDRESS_CONTRACT_TEST): tests/unit/address/test_direct_address_contract.c \
	tests/support/test_harness.h $(LIB) src/address/direct_dp.h \
	src/address/direct_sp.h src/address/address_effect.h include/ti_c54x/address.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(DIRECT_SOURCE_APPLY_TEST): tests/unit/sequencer/test_direct_source_apply.c \
	tests/support/test_harness.h $(LIB) src/sequencer/address_effect_apply.h \
	src/address/direct_dp.h src/address/direct_sp.h src/address/address_effect.h \
	src/state/arch_state.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(DIRECT_SOURCE_APPLY_CONTRACT_TEST): tests/unit/sequencer/test_direct_source_apply_contract.c \
	tests/support/test_harness.h $(LIB) src/sequencer/address_effect_apply.h \
	src/address/address_effect.h src/address/data_operand.h src/state/arch_state.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(INDIRECT_SINGLE_WORD_TEST): tests/unit/address/test_indirect_single_word.c \
	tests/support/test_harness.h $(LIB) \
	src/address/indirect_single.h src/address/address_effect.h \
	include/ti_c54x/address.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) \
		$(LDFLAGS) $(LDLIBS) -o $@

$(DATA_OPERAND_TEST): tests/unit/address/test_data_operand.c \
	tests/support/test_harness.h $(LIB) \
	src/address/data_operand.h include/ti_c54x/address.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) \
		$(LDFLAGS) $(LDLIBS) -o $@

$(DATA_OPERAND_CONTRACT_TEST): tests/unit/address/test_data_operand_contract.c \
	tests/support/test_harness.h $(LIB) \
	src/address/data_operand.h include/ti_c54x/address.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) \
		$(LDFLAGS) $(LDLIBS) -o $@

$(INDIRECT_SINGLE_LONG32_TEST): tests/unit/address/test_indirect_single_long32.c \
	tests/support/test_harness.h $(LIB) \
	src/address/data_operand.h src/address/indirect_single.h \
	src/address/address_effect.h include/ti_c54x/address.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) \
		$(LDFLAGS) $(LDLIBS) -o $@

$(INDIRECT_INDEXED_WORD_TEST): tests/unit/address/test_indirect_indexed_word.c \
	tests/support/test_harness.h $(LIB) src/address/indirect_indexed.h \
	src/address/address_effect.h include/ti_c54x/address.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) \
		$(LDFLAGS) $(LDLIBS) -o $@

$(INDIRECT_INDEXED_CONTRACT_TEST): tests/unit/address/test_indirect_indexed_contract.c \
	tests/support/test_harness.h $(LIB) src/address/indirect_indexed.h \
	src/address/address_effect.h include/ti_c54x/address.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) \
		$(LDFLAGS) $(LDLIBS) -o $@

$(INDIRECT_LONG_OFFSET_MOD13_TEST): tests/unit/address/test_indirect_long_offset_mod13.c \
	tests/support/test_harness.h $(LIB) src/address/indirect_long_offset.h \
	src/address/address_effect.h src/address/data_operand.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) \
		$(LDFLAGS) $(LDLIBS) -o $@

$(INDIRECT_LONG_OFFSET_CONTRACT_TEST): tests/unit/address/test_indirect_long_offset_contract.c \
	tests/support/test_harness.h $(LIB) src/address/indirect_long_offset.h \
	src/address/address_effect.h src/address/data_operand.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) \
		$(LDFLAGS) $(LDLIBS) -o $@

$(INDIRECT_REVERSE_CARRY_WORD_TEST): tests/unit/address/test_indirect_reverse_carry_word.c \
	tests/support/test_harness.h $(LIB) src/address/indirect_reverse_carry.h \
	src/address/address_effect.h src/foundation/reverse_carry.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) \
		$(LDFLAGS) $(LDLIBS) -o $@

$(INDIRECT_REVERSE_CARRY_CONTRACT_TEST): tests/unit/address/test_indirect_reverse_carry_contract.c \
	tests/support/test_harness.h $(LIB) src/address/indirect_reverse_carry.h \
	src/address/address_effect.h src/foundation/reverse_carry.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) \
		$(LDFLAGS) $(LDLIBS) -o $@

$(ADDRESS_EFFECT_CONTRACT_TEST): tests/unit/address/test_address_effect_contract.c \
	tests/support/test_harness.h $(LIB) src/address/indirect_single.h \
	src/address/address_effect.h include/ti_c54x/address.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(ARCH_STATE_RESET_TEST): tests/unit/state/test_arch_state_reset.c \
	tests/support/test_harness.h src/state/arch_state.h \
	src/state/register_layout.h $(LIB)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(ARCH_STATE_KNOWNNESS_TEST): tests/unit/state/test_arch_state_knownness.c \
	tests/support/test_harness.h src/state/arch_state.h \
	src/state/register_layout.h $(LIB)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(ARCH_STATE_ADDRESS_FIELDS_TEST): tests/unit/state/test_arch_state_address_fields.c \
	tests/support/test_harness.h src/state/arch_state.h \
	src/state/register_layout.h $(LIB)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(ARCH_STATE_ADDRESS_FIELD_CONTRACT_TEST): tests/unit/state/test_arch_state_address_field_contract.c \
	tests/support/test_harness.h src/state/arch_state.h \
	src/state/register_layout.h $(LIB)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(BLOCK_SIZE_REGISTER_TEST): tests/unit/state/test_block_size_register.c \
	tests/support/test_harness.h src/state/arch_state.h \
	src/state/block_size_register.h $(LIB)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(BLOCK_SIZE_REGISTER_CONTRACT_TEST): tests/unit/state/test_block_size_register_contract.c \
	tests/support/test_harness.h src/state/arch_state.h \
	src/state/block_size_register.h $(LIB)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(ADDRESS_EFFECT_APPLY_TEST): tests/unit/sequencer/test_address_effect_apply.c \
	tests/support/test_harness.h src/sequencer/address_effect_apply.h \
	src/address/address_effect.h src/address/indirect_single.h \
	src/state/arch_state.h $(LIB)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(ADDRESS_EFFECT_APPLY_CONTRACT_TEST): tests/unit/sequencer/test_address_effect_apply_contract.c \
	tests/support/test_harness.h src/sequencer/address_effect_apply.h \
	src/address/address_effect.h src/address/indirect_single.h \
	src/state/arch_state.h $(LIB)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(ADDRESS_EFFECT_VALIDATE_CONTRACT_TEST): tests/unit/sequencer/test_address_effect_validate_contract.c \
	tests/support/test_harness.h src/sequencer/address_effect_validate.h \
	src/sequencer/address_effect_apply.h src/address/address_effect.h \
	src/state/arch_state.h $(LIB)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(ADDRESS_EFFECT_DEPENDENCY_CONTRACT_TEST): tests/unit/sequencer/test_address_effect_dependency_contract.c \
	tests/support/test_harness.h src/sequencer/address_effect_validate.h \
	src/sequencer/address_effect_apply.h src/address/address_effect.h \
	src/state/arch_state.h $(LIB)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(INDIRECT_CMPT0_STATE_TEST): tests/unit/sequencer/test_indirect_cmpt0_state.c \
	tests/support/test_harness.h src/sequencer/address_effect_apply.h \
	src/address/address_effect.h src/address/indirect_single.h \
	src/state/arch_state.h $(LIB)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(ADDRESS_STATE_POLICY_CONTRACT_TEST): tests/unit/sequencer/test_address_state_policy_contract.c \
	tests/support/test_harness.h src/sequencer/address_effect_apply.h \
	src/address/address_effect.h src/state/arch_state.h $(LIB)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(INDIRECT_LONG32_EFFECT_APPLY_TEST): tests/unit/sequencer/test_indirect_long32_effect_apply.c \
	tests/support/test_harness.h $(LIB) \
	src/sequencer/address_effect_apply.h src/address/data_operand.h \
	src/address/indirect_single.h src/address/address_effect.h \
	src/state/arch_state.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(INDIRECT_INDEXED_EFFECT_APPLY_TEST): tests/unit/sequencer/test_indirect_indexed_effect_apply.c \
	tests/support/test_harness.h $(LIB) src/sequencer/address_effect_apply.h \
	src/address/indirect_indexed.h src/address/address_effect.h \
	src/state/arch_state.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(INDIRECT_INDEXED_EFFECT_APPLY_CONTRACT_TEST): tests/unit/sequencer/test_indirect_indexed_effect_apply_contract.c \
	tests/support/test_harness.h $(LIB) src/sequencer/address_effect_apply.h \
	src/address/indirect_indexed.h src/address/address_effect.h \
	src/state/arch_state.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(INDIRECT_LONG_OFFSET_EFFECT_APPLY_TEST): tests/unit/sequencer/test_indirect_long_offset_effect_apply.c \
	tests/support/test_harness.h $(LIB) src/sequencer/address_effect_apply.h \
	src/address/indirect_long_offset.h src/address/address_effect.h \
	src/state/arch_state.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(INDIRECT_LONG_OFFSET_EFFECT_APPLY_CONTRACT_TEST): tests/unit/sequencer/test_indirect_long_offset_effect_apply_contract.c \
	tests/support/test_harness.h $(LIB) src/sequencer/address_effect_apply.h \
	src/address/indirect_long_offset.h src/address/address_effect.h \
	src/state/arch_state.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(INDIRECT_REVERSE_CARRY_EFFECT_APPLY_TEST): tests/unit/sequencer/test_indirect_reverse_carry_effect_apply.c \
	tests/support/test_harness.h $(LIB) src/sequencer/address_effect_apply.h \
	src/address/indirect_reverse_carry.h src/address/address_effect.h \
	src/state/arch_state.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(INDIRECT_REVERSE_CARRY_EFFECT_APPLY_CONTRACT_TEST): tests/unit/sequencer/test_indirect_reverse_carry_effect_apply_contract.c \
	tests/support/test_harness.h $(LIB) src/sequencer/address_effect_apply.h \
	src/address/indirect_reverse_carry.h src/address/address_effect.h \
	src/state/arch_state.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(INDIRECT_CIRCULAR_WORD_TEST): tests/unit/address/test_indirect_circular_word.c \
	tests/support/test_harness.h $(LIB) src/address/indirect_circular.h \
	src/address/address_effect.h include/ti_c54x/address.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(INDIRECT_CIRCULAR_CONTRACT_TEST): tests/unit/address/test_indirect_circular_contract.c \
	tests/support/test_harness.h $(LIB) src/address/indirect_circular.h \
	src/address/address_effect.h include/ti_c54x/address.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(INDIRECT_CIRCULAR_EFFECT_APPLY_TEST): tests/unit/sequencer/test_indirect_circular_effect_apply.c \
	tests/support/test_harness.h $(LIB) src/sequencer/address_effect_apply.h \
	src/address/indirect_circular.h src/address/address_effect.h \
	src/state/arch_state.h src/state/block_size_register.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(INDIRECT_CIRCULAR_EFFECT_APPLY_CONTRACT_TEST): tests/unit/sequencer/test_indirect_circular_effect_apply_contract.c \
	tests/support/test_harness.h $(LIB) src/sequencer/address_effect_apply.h \
	src/address/indirect_circular.h src/address/address_effect.h \
	src/state/arch_state.h src/state/block_size_register.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(ADDRESS_EFFECT_STATE_CONTRACT_TEST): tests/unit/sequencer/test_address_effect_state_contract.c \
	tests/support/test_harness.h $(LIB) \
	src/sequencer/address_effect_apply.h src/address/address_effect.h \
	src/address/data_operand.h src/state/arch_state.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(ADDRESS_OPERAND_APPLY_CONTRACT_TEST): tests/unit/sequencer/test_address_operand_apply_contract.c \
	tests/support/test_harness.h $(LIB) \
	src/sequencer/address_effect_apply.h src/address/data_operand.h \
	src/address/indirect_single.h src/address/address_effect.h \
	src/state/arch_state.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(OBSERVER_TEST): tests/unit/debug/test_observer.c src/cpu/cpu.c \
	src/debug/observer.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -UTI_C54X_ENABLE_OBSERVERS \
		-DTI_C54X_ENABLE_OBSERVERS=1 \
		-Iinclude -Isrc $^ $(LDFLAGS) $(LDLIBS) -o $@

$(OBSERVER_OFF_TEST): tests/unit/debug/test_observer_off.c $(OBSERVER_OFF_LIB)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -UTI_C54X_ENABLE_OBSERVERS \
		-DTI_C54X_ENABLE_OBSERVERS=0 -Iinclude -Isrc $< \
		$(OBSERVER_OFF_LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(MINIMAL): examples/minimal.c $(LIB)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

# CMake remains available for package/install validation without making the
# fast unit-test path depend on it.
cmake-build:
	cmake -S . -B build-cmake -DTI_C54X_BUILD_TESTS=ON -DTI_C54X_BUILD_EXAMPLES=ON
	cmake --build build-cmake --parallel

install: cmake-build
	cmake --install build-cmake --prefix $(DESTDIR)/usr/local

format:
	clang-format -i src/cpu/*.c src/debug/*.c src/foundation/*.c \
		src/address/*.c src/address/*.h \
		src/sequencer/*.c src/sequencer/*.h \
		src/state/*.c src/state/*.h \
		include/ti_c54x/*.h tests/unit/cpu/*.c tests/unit/debug/*.c \
		tests/unit/foundation/*.c tests/unit/address/*.c \
		tests/unit/state/*.c tests/unit/sequencer/*.c examples/*.c

clean:
	@rm -rf -- build-fast build-cmake

-include $(LIB_OBJECTS:.o=.d) $(OBSERVER_OFF_OBJECTS:.o=.d)
