.PHONY: all build test cmake-build install clean format

# The default path is deliberately independent of CMake and the emulator. It
# gives the foundations a quick compile/link/run loop while CMake remains the
# packaging and IDE/CI project generator.
CC ?= cc
AR ?= ar
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
LIB_OBJECTS := \
	$(BUILD_DIR)/obj/foundation/arithmetic40.o \
	$(BUILD_DIR)/obj/cpu/cpu.o \
	$(BUILD_DIR)/obj/debug/observer.o
CPU_TEST := $(BUILD_DIR)/test_cpu_api
ARITHMETIC_TEST := $(BUILD_DIR)/test_arithmetic40
OBSERVER_TEST := $(BUILD_DIR)/test_observer
OBSERVER_OFF_TEST := $(BUILD_DIR)/test_observer_off
MINIMAL := $(BUILD_DIR)/minimal

all: test

build: $(CPU_TEST) $(ARITHMETIC_TEST) $(OBSERVER_TEST) \
	$(OBSERVER_OFF_TEST) $(MINIMAL)

test: build
	$(CPU_TEST)
	$(ARITHMETIC_TEST)
	$(OBSERVER_TEST)
	$(OBSERVER_OFF_TEST)
	$(MINIMAL)

$(LIB): $(LIB_OBJECTS)
	@mkdir -p $(@D)
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DIR)/obj/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(CPU_TEST): tests/unit/cpu/test_cpu_api.c $(LIB)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(ARITHMETIC_TEST): tests/unit/foundation/test_arithmetic40.c $(LIB)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(OBSERVER_TEST): tests/unit/debug/test_observer.c src/cpu/cpu.c \
	src/debug/observer.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -UTI_C54X_ENABLE_OBSERVERS \
		-DTI_C54X_ENABLE_OBSERVERS=1 \
		-Iinclude -Isrc $^ $(LDFLAGS) $(LDLIBS) -o $@

$(OBSERVER_OFF_TEST): tests/unit/debug/test_observer_off.c src/cpu/cpu.c \
	src/debug/observer.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -UTI_C54X_ENABLE_OBSERVERS \
		-DTI_C54X_ENABLE_OBSERVERS=0 \
		-Iinclude -Isrc $^ $(LDFLAGS) $(LDLIBS) -o $@

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
		include/ti_c54x/*.h tests/unit/cpu/*.c tests/unit/debug/*.c \
		tests/unit/foundation/*.c examples/*.c

clean:
	@rm -rf -- build-fast build-cmake

-include $(LIB_OBJECTS:.o=.d)
