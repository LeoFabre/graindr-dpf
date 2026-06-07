# Standalone-capable test suite.
# Built two ways:
#   1. On its own (fast, no DPF) during DSP module TDD:
#        cmake -S plugins/graindr-dpf/test -B plugins/graindr-dpf/build-test
#   2. Included from the root CMakeLists via add_subdirectory(test) once the
#      DPF plugin sources exist.
# Paths resolve relative to THIS file (CMAKE_CURRENT_SOURCE_DIR) so both modes work.
cmake_minimum_required(VERSION 3.22)
if(NOT DEFINED PROJECT_NAME)
    project(graindr_tests CXX)
    enable_testing()
endif()

set(GRAINDR_DSP_DIR    ${CMAKE_CURRENT_SOURCE_DIR}/../dsp)
set(GRAINDR_PLUGIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../plugin)

function(graindr_add_test name)
    add_executable(${name} ${name}.cpp)
    target_include_directories(${name} PRIVATE
        ${GRAINDR_DSP_DIR}
        ${GRAINDR_PLUGIN_DIR})
    target_compile_features(${name} PRIVATE cxx_std_17)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

graindr_add_test(test_dsp_math)
graindr_add_test(test_fastmath)
graindr_add_test(test_smoothed)
graindr_add_test(test_circular)
graindr_add_test(test_vasv_filter)
graindr_add_test(test_envelope)
graindr_add_test(test_lfo)
graindr_add_test(test_granular)
graindr_add_test(test_container)
graindr_add_test(test_drywet)
graindr_add_test(test_plugin_offline)
