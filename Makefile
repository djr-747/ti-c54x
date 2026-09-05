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
	$(BUILD_DIR)/obj/address/indirect_single.o \
	$(BUILD_DIR)/obj/state/arch_state.o \
	$(BUILD_DIR)/obj/cpu/cpu.o \
	$(BUILD_DIR)/obj/debug/observer.o
CPU_TEST := $(BUILD_DIR)/test_cpu_api
ARITHMETIC_TEST := $(BUILD_DIR)/test_arithmetic40
DIRECT_DP_TEST := $(BUILD_DIR)/test_direct_dp
INDIRECT_SINGLE_WORD_TEST := $(BUILD_DIR)/test_indirect_single_word
ADDRESS_EFFECT_CONTRACT_TEST := $(BUILD_DIR)/test_address_effect_contract
ARCH_STATE_RESET_TEST := $(BUILD_DIR)/test_arch_state_reset
ARCH_STATE_KNOWNNESS_TEST := $(BUILD_DIR)/test_arch_state_knownness
OBSERVER_TEST := $(BUILD_DIR)/test_observer
OBSERVER_OFF_TEST := $(BUILD_DIR)/test_observer_off
MINIMAL := $(BUILD_DIR)/minimal
OBSERVER_OFF_OBJECTS := \
	$(OBSERVER_OFF_DIR)/obj/address/indirect_single.o \
	$(OBSERVER_OFF_DIR)/obj/foundation/arithmetic40.o \
	$(OBSERVER_OFF_DIR)/obj/state/arch_state.o \
	$(OBSERVER_OFF_DIR)/obj/cpu/cpu.o \
	$(OBSERVER_OFF_DIR)/obj/debug/observer.o

all: test

build: $(CPU_TEST) $(ARITHMETIC_TEST) $(DIRECT_DP_TEST) \
	$(INDIRECT_SINGLE_WORD_TEST) $(ADDRESS_EFFECT_CONTRACT_TEST) \
	$(ARCH_STATE_RESET_TEST) $(ARCH_STATE_KNOWNNESS_TEST) \
	$(OBSERVER_TEST) \
	$(OBSERVER_OFF_TEST) $(MINIMAL) $(OBSERVER_OFF_LIB)

test: build quality-test
	$(CPU_TEST)
	$(ARITHMETIC_TEST)
	$(DIRECT_DP_TEST)
	$(INDIRECT_SINGLE_WORD_TEST)
	$(ADDRESS_EFFECT_CONTRACT_TEST)
	$(ARCH_STATE_RESET_TEST)
	$(ARCH_STATE_KNOWNNESS_TEST)
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

$(DIRECT_DP_TEST): tests/unit/address/test_direct_dp.c \
	tests/support/test_harness.h src/address/direct_dp.h \
	include/ti_c54x/address.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) $(LDLIBS) -o $@

$(INDIRECT_SINGLE_WORD_TEST): tests/unit/address/test_indirect_single_word.c \
	tests/support/test_harness.h $(LIB) \
	src/address/indirect_single.h src/address/address_effect.h \
	include/ti_c54x/address.h
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
		src/state/*.c src/state/*.h \
		include/ti_c54x/*.h tests/unit/cpu/*.c tests/unit/debug/*.c \
		tests/unit/foundation/*.c tests/unit/address/*.c \
		tests/unit/state/*.c examples/*.c

clean:
	@rm -rf -- build-fast build-cmake

-include $(LIB_OBJECTS:.o=.d) $(OBSERVER_OFF_OBJECTS:.o=.d)
