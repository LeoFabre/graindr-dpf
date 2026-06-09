cmake_minimum_required(VERSION 3.22)
project(Graindr VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(GRAINDR_BUILD_UI    "Build the optional NanoVG UI" ON)
option(GRAINDR_BUILD_TESTS "Build offline DSP tests"      ON)

add_subdirectory(dpf)

if(GRAINDR_BUILD_UI)
    dpf_add_plugin(Graindr
        TARGETS vst3 lv2
        UI_TYPE opengl
        FILES_DSP plugin/GraindrPlugin.cpp
        FILES_UI  plugin/GraindrUI.cpp)
    target_compile_definitions(Graindr PUBLIC DISTRHO_PLUGIN_HAS_UI=1)
else()
    dpf_add_plugin(Graindr
        TARGETS vst3 lv2
        FILES_DSP plugin/GraindrPlugin.cpp)
    target_compile_definitions(Graindr PUBLIC DISTRHO_PLUGIN_HAS_UI=0)
endif()

# Tier 3 — targeted fast-math subset on the DSP objects only. DPF compiles the
# FILES_DSP into a static lib named Graindr-dsp; scoping the flag there keeps it
# off the DGL/UI/framework code. -funsafe-math-optimizations enables
# reassociation / reciprocal / no-math-errno / FMA WITHOUT -ffinite-math-only
# (NaN/Inf semantics preserved) and WITHOUT linking crtfastmath (so the explicit
# FTZ in DenormalGuard.hpp still governs denormals). Verified output-equivalent
# within -80 dB vs -O3 (tools/regression/ab.sh graindr --cxxflags ...).
if(TARGET Graindr-dsp)
    target_compile_options(Graindr-dsp PRIVATE -funsafe-math-optimizations)
endif()

target_include_directories(Graindr PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin
    ${CMAKE_CURRENT_SOURCE_DIR}/dsp)

if(GRAINDR_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
