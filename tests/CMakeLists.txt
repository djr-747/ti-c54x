add_executable(ti_c54x_test_cpu_api unit/cpu/test_cpu_api.c)
target_link_libraries(ti_c54x_test_cpu_api PRIVATE ti_c54x::core)
target_compile_features(ti_c54x_test_cpu_api PRIVATE c_std_11)
target_include_directories(ti_c54x_test_cpu_api PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src
)
ti_c54x_enable_warnings(ti_c54x_test_cpu_api)
ti_c54x_enable_sanitizers(ti_c54x_test_cpu_api)
add_test(NAME ti_c54x.cpu_api COMMAND ti_c54x_test_cpu_api)

add_executable(ti_c54x_test_arithmetic40 unit/foundation/test_arithmetic40.c)
target_link_libraries(ti_c54x_test_arithmetic40 PRIVATE ti_c54x::core)
target_compile_features(ti_c54x_test_arithmetic40 PRIVATE c_std_11)
target_include_directories(ti_c54x_test_arithmetic40 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src
)
ti_c54x_enable_warnings(ti_c54x_test_arithmetic40)
ti_c54x_enable_sanitizers(ti_c54x_test_arithmetic40)
add_test(NAME ti_c54x.arithmetic40 COMMAND ti_c54x_test_arithmetic40)

add_library(ti_c54x_observer_enabled STATIC
    ${PROJECT_SOURCE_DIR}/src/cpu/cpu.c
    ${PROJECT_SOURCE_DIR}/src/debug/observer.c
)
target_compile_features(ti_c54x_observer_enabled PRIVATE c_std_11)
target_compile_definitions(ti_c54x_observer_enabled PRIVATE
    TI_C54X_ENABLE_OBSERVERS=1
)
target_include_directories(ti_c54x_observer_enabled
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)
ti_c54x_enable_warnings(ti_c54x_observer_enabled)
ti_c54x_enable_sanitizers(ti_c54x_observer_enabled)

add_executable(ti_c54x_test_observer unit/debug/test_observer.c)
target_link_libraries(ti_c54x_test_observer PRIVATE ti_c54x_observer_enabled)
target_compile_features(ti_c54x_test_observer PRIVATE c_std_11)
target_include_directories(ti_c54x_test_observer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src
)
ti_c54x_enable_warnings(ti_c54x_test_observer)
ti_c54x_enable_sanitizers(ti_c54x_test_observer)
add_test(NAME ti_c54x.observer COMMAND ti_c54x_test_observer)

add_executable(ti_c54x_test_observer_off
    unit/debug/test_observer_off.c
    ${PROJECT_SOURCE_DIR}/src/cpu/cpu.c
    ${PROJECT_SOURCE_DIR}/src/debug/observer.c
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
