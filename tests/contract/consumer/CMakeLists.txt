cmake_minimum_required(VERSION 3.21)

project(ti_c54x_consumer LANGUAGES C)

find_package(ti_c54x CONFIG REQUIRED)

add_executable(ti_c54x_consumer_smoke smoke.c)
target_compile_features(ti_c54x_consumer_smoke PRIVATE c_std_11)
target_link_libraries(ti_c54x_consumer_smoke PRIVATE ti_c54x::core)
