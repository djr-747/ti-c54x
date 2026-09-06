# Source ownership: src/isa aggregates catalog, decode, and opcode-operation
# owners without flattening their nested manifests.
include(${CMAKE_CURRENT_LIST_DIR}/catalog/sources.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/decode/sources.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/ops/sources.cmake)
