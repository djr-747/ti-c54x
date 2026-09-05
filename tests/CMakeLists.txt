find_package(Python3 COMPONENTS Interpreter REQUIRED)

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

add_executable(ti_c54x_test_direct_dp unit/address/test_direct_dp.c)
target_compile_features(ti_c54x_test_direct_dp PRIVATE c_std_11)
target_include_directories(ti_c54x_test_direct_dp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
)
ti_c54x_enable_warnings(ti_c54x_test_direct_dp)
ti_c54x_enable_sanitizers(ti_c54x_test_direct_dp)
add_test(NAME ti_c54x.direct_dp COMMAND ti_c54x_test_direct_dp)

add_executable(ti_c54x_test_indirect_single_word
    unit/address/test_indirect_single_word.c
)
target_compile_features(ti_c54x_test_indirect_single_word PRIVATE c_std_11)
target_include_directories(ti_c54x_test_indirect_single_word PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(ti_c54x_test_indirect_single_word PRIVATE ti_c54x::core)
ti_c54x_enable_warnings(ti_c54x_test_indirect_single_word)
ti_c54x_enable_sanitizers(ti_c54x_test_indirect_single_word)
add_test(NAME ti_c54x.indirect_single_word
    COMMAND ti_c54x_test_indirect_single_word
)

add_executable(ti_c54x_test_address_effect_contract
    unit/address/test_address_effect_contract.c
)
target_compile_features(ti_c54x_test_address_effect_contract PRIVATE c_std_11)
target_include_directories(ti_c54x_test_address_effect_contract PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(ti_c54x_test_address_effect_contract PRIVATE ti_c54x::core)
ti_c54x_enable_warnings(ti_c54x_test_address_effect_contract)
ti_c54x_enable_sanitizers(ti_c54x_test_address_effect_contract)
add_test(NAME ti_c54x.address_effect_contract
    COMMAND ti_c54x_test_address_effect_contract
)

add_library(ti_c54x_observer_off_artifact STATIC ${ti_c54x_core_sources})
target_compile_features(ti_c54x_observer_off_artifact PRIVATE c_std_11)
target_compile_definitions(ti_c54x_observer_off_artifact PRIVATE
    TI_C54X_ENABLE_OBSERVERS=0
)
target_include_directories(ti_c54x_observer_off_artifact
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src
)
ti_c54x_enable_warnings(ti_c54x_observer_off_artifact)
ti_c54x_enable_sanitizers(ti_c54x_observer_off_artifact)

add_library(ti_c54x_observer_enabled STATIC ${ti_c54x_core_sources})
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
)
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

add_test(
    NAME ti_c54x.quality.python
    COMMAND "${Python3_EXECUTABLE}" -m unittest discover
        -s "${PROJECT_SOURCE_DIR}/tests/contract/quality"
        -t "${PROJECT_SOURCE_DIR}"
)
set_tests_properties(ti_c54x.quality.python PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME ti_c54x.quality.knobs
    COMMAND "${Python3_EXECUTABLE}" tools/quality/check_knobs.py
        --root "${PROJECT_SOURCE_DIR}"
)
set_tests_properties(ti_c54x.quality.knobs PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME ti_c54x.quality.source_boundaries
    COMMAND "${Python3_EXECUTABLE}" tools/quality/check_source_boundaries.py
        --root "${PROJECT_SOURCE_DIR}"
)
set_tests_properties(ti_c54x.quality.source_boundaries PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME ti_c54x.quality.evidence
    COMMAND "${Python3_EXECUTABLE}" tools/quality/check_evidence.py
        --root "${PROJECT_SOURCE_DIR}"
)
set_tests_properties(ti_c54x.quality.evidence PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

if(NOT CMAKE_NM)
    set(CMAKE_NM nm)
endif()
add_test(
    NAME ti_c54x.quality.observer_off_artifact
    COMMAND "${Python3_EXECUTABLE}" tools/quality/check_observer_artifact.py
        --artifact "$<TARGET_FILE:ti_c54x_observer_off_artifact>"
        --nm "${CMAKE_NM}"
)
set_tests_properties(ti_c54x.quality.observer_off_artifact PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)
