cmake_minimum_required(VERSION 3.21)

project(ti_c54x
    VERSION 0.1.0
    DESCRIPTION "Standalone TI C54x DSP core"
    LANGUAGES C
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(TI_C54X_BUILD_TESTS "Build the standalone unit tests" ON)
option(TI_C54X_BUILD_EXAMPLES "Build small standalone examples" OFF)
option(TI_C54X_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(TI_C54X_ENABLE_OBSERVERS "Compile structured observer callbacks into the core" OFF)
option(TI_C54X_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

if(NOT CMAKE_C_STANDARD)
    set(CMAKE_C_STANDARD 11)
endif()
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

function(ti_c54x_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
        if(TI_C54X_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wsign-conversion
            -Wshadow
            -Wundef
            -Wdouble-promotion
        )
        if(TI_C54X_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

function(ti_c54x_enable_sanitizers target)
    if(TI_C54X_ENABLE_SANITIZERS)
        if(MSVC)
            message(FATAL_ERROR "TI_C54X_ENABLE_SANITIZERS requires Clang or GCC")
        endif()
        # Keep this PRIVATE so installed consumers do not inherit validation flags.
        target_compile_options(${target} PRIVATE
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
        )
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
endfunction()

set(ti_c54x_core_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/src/foundation/arithmetic40.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/foundation/reverse_carry.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/address/data_operand.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/address/direct_dp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/address/direct_sp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/address/indirect_single.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/address/indirect_indexed.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/address/indirect_long_offset.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/address/indirect_reverse_carry.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sequencer/address_effect_intrinsic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sequencer/address_effect_dependencies.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sequencer/address_effect_validate.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sequencer/address_effect_apply.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/state/arch_state.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/state/block_size_register.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cpu/cpu.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/debug/observer.c
)

add_library(ti_c54x STATIC
    ${ti_c54x_core_sources}
)
add_library(ti_c54x::core ALIAS ti_c54x)

target_compile_features(ti_c54x PUBLIC c_std_11)
target_include_directories(ti_c54x
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(ti_c54x PROPERTIES
    EXPORT_NAME core
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)
ti_c54x_enable_warnings(ti_c54x)
ti_c54x_enable_sanitizers(ti_c54x)
if(TI_C54X_ENABLE_OBSERVERS)
    target_compile_definitions(ti_c54x PRIVATE TI_C54X_ENABLE_OBSERVERS=1)
else()
    target_compile_definitions(ti_c54x PRIVATE TI_C54X_ENABLE_OBSERVERS=0)
endif()

if(TI_C54X_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(TI_C54X_BUILD_EXAMPLES)
    add_executable(ti_c54x_minimal examples/minimal.c)
    target_link_libraries(ti_c54x_minimal PRIVATE ti_c54x::core)
    target_compile_features(ti_c54x_minimal PRIVATE c_std_11)
    ti_c54x_enable_warnings(ti_c54x_minimal)
    ti_c54x_enable_sanitizers(ti_c54x_minimal)
endif()

install(TARGETS ti_c54x
    EXPORT ti_c54x_targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT ti_c54x_targets
    FILE ti_c54xTargets.cmake
    NAMESPACE ti_c54x::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ti_c54x
)

configure_package_config_file(
    cmake/ti_c54xConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/ti_c54xConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ti_c54x
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/ti_c54xConfigVersion.cmake
    COMPATIBILITY SameMajorVersion
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/ti_c54xConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/ti_c54xConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ti_c54x
)
