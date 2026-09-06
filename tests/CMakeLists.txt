# Root test policy: common target construction, observer artifacts, quality
# checks, and ownership-directory orchestration. Individual test registrations
# live under tests/unit/<owner>/CMakeLists.txt.
find_package(Python3 COMPONENTS Interpreter REQUIRED)

function(ti_c54x_add_unit_test test_name source_name)
    set(target "ti_c54x_test_${test_name}")
    add_executable(${target} "${CMAKE_CURRENT_SOURCE_DIR}/${source_name}")
    target_link_libraries(${target} PRIVATE ti_c54x::core)
    target_compile_features(${target} PRIVATE c_std_11)
    target_include_directories(${target} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src
    )
    ti_c54x_enable_warnings(${target})
    ti_c54x_enable_sanitizers(${target})
    add_test(NAME "ti_c54x.${test_name}" COMMAND ${target})
endfunction()

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

# Ownership hierarchy is mirrored directly in the test tree.
add_subdirectory(unit/cpu)
add_subdirectory(unit/foundation)
add_subdirectory(unit/address)
add_subdirectory(unit/state)
add_subdirectory(unit/sequencer)
add_subdirectory(unit/debug)
add_subdirectory(unit/isa)

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
    NAME ti_c54x.quality.build_manifests
    COMMAND "${Python3_EXECUTABLE}" tools/quality/check_build_manifests.py
        --root "${PROJECT_SOURCE_DIR}"
)
set_tests_properties(ti_c54x.quality.build_manifests PROPERTIES
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
