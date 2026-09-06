# Test ownership: tests/unit/debug owns observer-on/off artifact coverage.
ti_c54x_test_targets += $(OBSERVER_TEST) $(OBSERVER_OFF_TEST)

$(OBSERVER_TEST): tests/unit/debug/test_observer.c src/cpu/cpu.c \
	src/debug/observer.c
	@mkdir -p $(@D)
	@mkdir -p $(BUILD_DIR)/dep
	$(CC) $(CPPFLAGS) $(CFLAGS) -UTI_C54X_ENABLE_OBSERVERS \
		-DTI_C54X_ENABLE_OBSERVERS=1 -MMD -MP \
		-MF $(BUILD_DIR)/dep/test_observer.d -Iinclude -Isrc $^ \
		$(LDFLAGS) $(LDLIBS) -o $@

$(OBSERVER_OFF_TEST): tests/unit/debug/test_observer_off.c \
	$(OBSERVER_OFF_LIB)
	@mkdir -p $(@D)
	@mkdir -p $(BUILD_DIR)/dep
	$(CC) $(CPPFLAGS) $(CFLAGS) -UTI_C54X_ENABLE_OBSERVERS \
		-DTI_C54X_ENABLE_OBSERVERS=0 -MMD -MP \
		-MF $(BUILD_DIR)/dep/test_observer_off.d -Iinclude -Isrc $< \
		$(OBSERVER_OFF_LIB) $(LDFLAGS) $(LDLIBS) -o $@

ti_c54x_test_depfiles += \
	$(BUILD_DIR)/dep/test_observer.d \
	$(BUILD_DIR)/dep/test_observer_off.d
