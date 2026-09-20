.PHONY: all build test quality-test bench bench-release bench-smoke bench-record bench-compare bench-rmw bench-rmw-smoke bench-rmw-compare bench-dld-dst bench-dld-dst-smoke bench-add-sub bench-add-sub-smoke run-image run-image-selftest cmake-build install clean format

# Root policy: compiler flags, artifacts, orchestration, and packaging. Source
# and test ownership manifests live beside the files they register.
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
OBSERVER_ON_DIR := $(BUILD_DIR)/observer-on
OBSERVER_ON_LIB := $(OBSERVER_ON_DIR)/libti_c54x.a
OBSERVER_OFF_DIR := $(BUILD_DIR)/observer-off
OBSERVER_OFF_LIB := $(OBSERVER_OFF_DIR)/libti_c54x.a
BENCH_BUILD_DIR := build-bench
BENCH_LIB := $(BENCH_BUILD_DIR)/libti_c54x.a
BENCH_EXE := $(BENCH_BUILD_DIR)/ti_c54x_bench
RMW_BENCH_EXE := $(BENCH_BUILD_DIR)/ti_c54x_rmw_bench
DLD_DST_BENCH_EXE := $(BENCH_BUILD_DIR)/ti_c54x_dld_dst_bench
ADD_SUB_BENCH_EXE := $(BENCH_BUILD_DIR)/ti_c54x_add_sub_bench
DUAL_ADD_SUB_BENCH_EXE := $(BENCH_BUILD_DIR)/ti_c54x_dual_add_sub_bench
HOSTED_BUILD_DIR := build-hosted
HOSTED_LIB := $(HOSTED_BUILD_DIR)/libti_c54x.a
RUN_IMAGE_EXE := $(HOSTED_BUILD_DIR)/ti_c54x_run_image
BENCH_CPPFLAGS := -Iinclude -Isrc -DTI_C54X_ENABLE_OBSERVERS=0
BENCH_DIAGNOSTIC_CFLAGS := -O2 -DNDEBUG -std=c11 -Wall -Wextra -Wpedantic -Wconversion \
	-Wsign-conversion -Wshadow -Wundef -Wdouble-promotion -Werror
BENCH_RELEASE_CFLAGS := -O3 -flto -DNDEBUG -std=c11 -Wall -Wextra -Wpedantic \
	-Wconversion -Wsign-conversion -Wshadow -Wundef -Wdouble-promotion -Werror
BENCH_CFLAGS ?= $(BENCH_DIAGNOSTIC_CFLAGS)
BENCH_OBJECTS = $(patsubst src/%.c,$(BENCH_BUILD_DIR)/obj/%.o,$(ti_c54x_core_sources))
HOSTED_OBJECTS = $(patsubst src/%.c,$(HOSTED_BUILD_DIR)/obj/%.o,$(ti_c54x_core_sources))

# Each owner appends its source files; this is the sole source inventory used
# by normal, observer-on, and observer-off static-library builds.
ti_c54x_core_sources :=
include src/foundation/sources.mk
include src/address/sources.mk
include src/bus/sources.mk
include src/devices/sources.mk
include src/sequencer/sources.mk
include src/state/sources.mk
include src/cpu/sources.mk
include src/debug/sources.mk
include src/isa/sources.mk

LIB_OBJECTS := $(patsubst src/%.c,$(BUILD_DIR)/obj/%.o,$(ti_c54x_core_sources))
OBSERVER_ON_OBJECTS := $(patsubst src/%.c,$(OBSERVER_ON_DIR)/obj/%.o,$(ti_c54x_core_sources))
OBSERVER_OFF_OBJECTS := $(patsubst src/%.c,$(OBSERVER_OFF_DIR)/obj/%.o,$(ti_c54x_core_sources))

OBSERVER_TEST := $(BUILD_DIR)/test_observer
OBSERVER_OFF_TEST := $(BUILD_DIR)/test_observer_off
MINIMAL := $(BUILD_DIR)/minimal
ti_c54x_test_targets :=
ti_c54x_test_depfiles :=

all: test

bench: $(BENCH_EXE)
	$(PYTHON) tools/perf/capture.py --benchmark $(BENCH_EXE) --root . --allow-dirty \
		--compiler "$(CC)" --build-flags "$(BENCH_CPPFLAGS) $(BENCH_CFLAGS)"

bench-release:
	$(MAKE) -B BENCH_BUILD_DIR=build-bench-release \
		BENCH_CFLAGS='$(BENCH_RELEASE_CFLAGS)' bench

bench-smoke: bench

bench-record: $(BENCH_EXE)
	$(PYTHON) tools/perf/capture.py --benchmark $(BENCH_EXE) --root . \
		--history bench/history/hosted-v1.jsonl --compiler "$(CC)" \
		--build-flags "$(BENCH_CPPFLAGS) $(BENCH_CFLAGS)"

bench-compare: $(BENCH_EXE)
	$(PYTHON) tools/perf/capture.py --benchmark $(BENCH_EXE) --root . --allow-dirty \
		--compiler "$(CC)" --build-flags "$(BENCH_CPPFLAGS) $(BENCH_CFLAGS)" | \
	$(PYTHON) tools/perf/compare.py --history bench/history/hosted-v1.jsonl --current-file -

bench-rmw: $(RMW_BENCH_EXE)
	$(PYTHON) tools/perf/rmw_capture.py --benchmark $(RMW_BENCH_EXE) --root . --allow-dirty \
		--compiler "$(CC)" --profile portable-o2-no-lto \
		--build-flags "$(BENCH_CPPFLAGS) $(BENCH_CFLAGS)"

bench-rmw-smoke: $(RMW_BENCH_EXE)
	$(PYTHON) tools/perf/rmw_capture.py --benchmark $(RMW_BENCH_EXE) --root . --allow-dirty \
		--compiler "$(CC)" --profile portable-o2-no-lto \
		--build-flags "$(BENCH_CPPFLAGS) $(BENCH_CFLAGS)"

bench-rmw-compare: $(RMW_BENCH_EXE)
	$(PYTHON) tools/perf/rmw_capture.py --benchmark $(RMW_BENCH_EXE) --root . --allow-dirty \
		--compiler "$(CC)" --profile portable-o2-no-lto \
		--build-flags "$(BENCH_CPPFLAGS) $(BENCH_CFLAGS)" | \
	$(PYTHON) tools/perf/rmw_compare.py \
		--baseline bench/audits/p2-w46b-rmw-baseline.json --current-file -

bench-dld-dst: $(DLD_DST_BENCH_EXE)
	$(PYTHON) tools/perf/dld_dst_capture.py --benchmark $(DLD_DST_BENCH_EXE) --root . --allow-dirty \
		--compiler "$(CC)" --profile portable-o2-no-lto \
		--build-flags "$(BENCH_CPPFLAGS) $(BENCH_CFLAGS)"

bench-dld-dst-smoke: $(DLD_DST_BENCH_EXE)
	$(PYTHON) tools/perf/dld_dst_capture.py --benchmark $(DLD_DST_BENCH_EXE) --root . --allow-dirty \
		--compiler "$(CC)" --profile portable-o2-no-lto \
		--build-flags "$(BENCH_CPPFLAGS) $(BENCH_CFLAGS)"

bench-add-sub: $(ADD_SUB_BENCH_EXE)
	$(PYTHON) tools/perf/add_sub_capture.py --benchmark $(ADD_SUB_BENCH_EXE) --root . --allow-dirty \
		--compiler "$(CC)" --profile portable-o2-no-lto \
		--build-flags "$(BENCH_CPPFLAGS) $(BENCH_CFLAGS)"

bench-add-sub-smoke: $(ADD_SUB_BENCH_EXE)
	$(PYTHON) tools/perf/add_sub_capture.py --benchmark $(ADD_SUB_BENCH_EXE) --root . --allow-dirty \
		--compiler "$(CC)" --profile portable-o2-no-lto \
		--build-flags "$(BENCH_CPPFLAGS) $(BENCH_CFLAGS)"

bench-dual-add-sub: $(DUAL_ADD_SUB_BENCH_EXE)
	$(PYTHON) tools/perf/dual_add_sub_capture.py --benchmark $(DUAL_ADD_SUB_BENCH_EXE) --root . --allow-dirty \
		--compiler "$(CC)" --profile portable-o2-no-lto \
		--build-flags "$(BENCH_CPPFLAGS) $(BENCH_CFLAGS)"

bench-dual-add-sub-smoke: bench-dual-add-sub

# Root owns the common direct-Make test construction policy. Owner manifests
# provide only target identity and source path, so test registration follows
# the ownership hierarchy without duplicating compiler policy.
define ti_c54x_add_test
ti_c54x_test_targets += $(BUILD_DIR)/test_$(1)
ti_c54x_test_depfiles += $(BUILD_DIR)/dep/test_$(1).d

$(BUILD_DIR)/test_$(1): tests/unit/$(2)/$(3) $(LIB)
	@mkdir -p $$(@D)
	@mkdir -p $(BUILD_DIR)/dep
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -MF $(BUILD_DIR)/dep/test_$(1).d $$< $(LIB) $(LDFLAGS) $(LDLIBS) -o $$@
endef

include tests/unit/cpu/tests.mk
include tests/unit/foundation/tests.mk
include tests/unit/address/tests.mk
include tests/unit/bus/tests.mk
include tests/unit/devices/tests.mk
include tests/unit/state/tests.mk
include tests/unit/sequencer/tests.mk
include tests/unit/debug/tests.mk
include tests/unit/isa/tests.mk
include tests/unit/hosted/tests.mk

build: $(ti_c54x_test_targets) $(MINIMAL) $(OBSERVER_OFF_LIB)

test: build quality-test
	@set -e; $(foreach test,$(ti_c54x_test_targets),$(test);)
	$(MINIMAL)

quality-test: $(OBSERVER_OFF_LIB)
	$(PYTHON) -m unittest discover -s tests/contract/quality -t .
	$(PYTHON) tools/quality/check_knobs.py --root .
	$(PYTHON) tools/quality/check_source_boundaries.py --root .
	$(PYTHON) tools/quality/check_build_manifests.py --root .
	$(PYTHON) tools/quality/check_evidence.py --root .
	$(PYTHON) tools/quality/check_isa_inventory.py --root .
	$(PYTHON) tools/quality/check_observer_artifact.py \
		--artifact $(OBSERVER_OFF_LIB) --nm $(NM)

$(LIB): $(LIB_OBJECTS)
	@mkdir -p $(@D)
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DIR)/obj/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(OBSERVER_ON_DIR)/obj/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -UTI_C54X_ENABLE_OBSERVERS \
		-DTI_C54X_ENABLE_OBSERVERS=1 -MMD -MP -c $< -o $@

$(OBSERVER_ON_LIB): $(OBSERVER_ON_OBJECTS)
	@mkdir -p $(@D)
	$(AR) $(ARFLAGS) $@ $^

$(OBSERVER_OFF_DIR)/obj/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -UTI_C54X_ENABLE_OBSERVERS \
		-DTI_C54X_ENABLE_OBSERVERS=0 -MMD -MP -c $< -o $@

$(OBSERVER_OFF_LIB): $(OBSERVER_OFF_OBJECTS)
	@mkdir -p $(@D)
	$(AR) $(ARFLAGS) $@ $^

$(BENCH_LIB): $(BENCH_OBJECTS)
	@mkdir -p $(@D)
	$(AR) $(ARFLAGS) $@ $^

$(BENCH_EXE): bench/ti_c54x_bench.c $(BENCH_LIB)
	@mkdir -p $(@D)
	$(CC) $(BENCH_CPPFLAGS) $(BENCH_CFLAGS) $< $(BENCH_LIB) -o $@

$(RMW_BENCH_EXE): bench/ti_c54x_rmw_bench.c $(BENCH_LIB)
	@mkdir -p $(@D)
	$(CC) $(BENCH_CPPFLAGS) $(BENCH_CFLAGS) $< $(BENCH_LIB) -o $@

$(DLD_DST_BENCH_EXE): bench/ti_c54x_dld_dst_bench.c $(BENCH_LIB)
	@mkdir -p $(@D)
	$(CC) $(BENCH_CPPFLAGS) $(BENCH_CFLAGS) $< $(BENCH_LIB) -o $@

$(ADD_SUB_BENCH_EXE): bench/ti_c54x_add_sub_bench.c $(BENCH_LIB)
	@mkdir -p $(@D)
	$(CC) $(BENCH_CPPFLAGS) $(BENCH_CFLAGS) $< $(BENCH_LIB) -o $@

$(DUAL_ADD_SUB_BENCH_EXE): bench/ti_c54x_dual_add_sub_bench.c $(BENCH_LIB)
	@mkdir -p $(@D)
	$(CC) $(BENCH_CPPFLAGS) $(BENCH_CFLAGS) $< $(BENCH_LIB) -o $@

$(BENCH_BUILD_DIR)/obj/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(BENCH_CPPFLAGS) $(BENCH_CFLAGS) -MMD -MP -c $< -o $@

# Hosted DSP-image smoke runner. Like the benchmarks it links the static
# core archive but is not part of any test gate: it reads caller-supplied
# image files at run time and never embeds firmware bytes.
run-image: $(RUN_IMAGE_EXE)
	$(RUN_IMAGE_EXE) $(IMAGE) $(PROG_BASE) $(ENTRY) $(BUDGET) $(ORDER) \
		$(if $(DATA_IMAGE),$(DATA_IMAGE) $(DATA_BASE))

run-image-selftest: $(RUN_IMAGE_EXE)
	$(RUN_IMAGE_EXE) --selftest

$(HOSTED_LIB): $(HOSTED_OBJECTS)
	@mkdir -p $(@D)
	$(AR) $(ARFLAGS) $@ $^

$(RUN_IMAGE_EXE): tools/hosted/ti_c54x_run_image.c tools/hosted/mcu_upload_peer.c tools/hosted/port_peer.c $(HOSTED_LIB)
	@mkdir -p $(@D)
	$(CC) $(BENCH_CPPFLAGS) $(BENCH_CFLAGS) tools/hosted/ti_c54x_run_image.c tools/hosted/mcu_upload_peer.c tools/hosted/port_peer.c $(HOSTED_LIB) -o $@

$(HOSTED_BUILD_DIR)/obj/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(BENCH_CPPFLAGS) $(BENCH_CFLAGS) -MMD -MP -c $< -o $@

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
		src/isa/catalog/*.c src/isa/catalog/*.h \
		src/isa/decode/*.c src/isa/decode/*.h \
		src/isa/ops/*.c src/isa/ops/*.h \
		src/state/*.c src/state/*.h \
		include/ti_c54x/*.h tests/unit/cpu/*.c tests/unit/debug/*.c \
		bench/*.c \
		tests/unit/foundation/*.c tests/unit/address/*.c \
		tests/unit/state/*.c tests/unit/sequencer/*.c \
		tests/unit/isa/decode/*.c tests/unit/isa/ops/*.c examples/*.c

clean:
	@rm -rf -- build-fast build-bench build-bench-release build-cmake build-bench-cmake build-hosted

-include $(LIB_OBJECTS:.o=.d) $(OBSERVER_ON_OBJECTS:.o=.d) \
	$(OBSERVER_OFF_OBJECTS:.o=.d) $(BENCH_OBJECTS:.o=.d) \
	$(HOSTED_OBJECTS:.o=.d) \
	$(ti_c54x_test_depfiles)
