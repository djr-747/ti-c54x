# Test ownership: tests/unit/debug owns observer-on/off artifact coverage.
add_executable(ti_c54x_test_observer test_observer.c)
target_link_libraries(ti_c54x_test_observer PRIVATE ti_c54x_observer_enabled)
target_compile_features(ti_c54x_test_observer PRIVATE c_std_11)
target_include_directories(ti_c54x_test_observer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src
)
ti_c54x_enable_warnings(ti_c54x_test_observer)
ti_c54x_enable_sanitizers(ti_c54x_test_observer)
add_test(NAME ti_c54x.observer COMMAND ti_c54x_test_observer)

add_executable(ti_c54x_test_observer_off test_observer_off.c)
target_link_libraries(ti_c54x_test_observer_off PRIVATE
    ti_c54x_observer_off_artifact
)
target_compile_features(ti_c54x_test_observer_off PRIVATE c_std_11)
target_compile_definitions(ti_c54x_test_observer_off PRIVATE
    TI_C54X_ENABLE_OBSERVERS=0
)
target_include_directories(ti_c54x_test_observer_off PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
)
ti_c54x_enable_warnings(ti_c54x_test_observer_off)
ti_c54x_enable_sanitizers(ti_c54x_test_observer_off)
add_test(NAME ti_c54x.observer_off COMMAND ti_c54x_test_observer_off)
