cmake_minimum_required(VERSION 3.20)
# Placeholder module test directory for mirrored src structure.

# Wave D: high-cardinality stress test for stable_diffusion module.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_stable_diffusion_highcardinality_stress.cpp")
    set(_sd_stress_target "module_stable_diffusion_test_stable_diffusion_highcardinality_stress")
    if(NOT TARGET ${_sd_stress_target})
        add_executable(${_sd_stress_target}
            test_stable_diffusion_highcardinality_stress.cpp
        )
        target_include_directories(${_sd_stress_target} PRIVATE
            ${THEMIS_ROOT_DIR}/include
            ${THEMIS_ROOT_DIR}/src
        )
        target_link_libraries(${_sd_stress_target} PRIVATE
            ${TEST_LIBS}
            themis_core
            spdlog::spdlog
            Threads::Threads
        )
        target_compile_definitions(${_sd_stress_target} PRIVATE THEMIS_TEST_BUILD=1)
        themis_register_module_focused_test(
            MODULE stable_diffusion
            NAME   test_stable_diffusion_highcardinality_stress_FocusedTests
            TARGET ${_sd_stress_target}
            TIER   stress
            TIMEOUT 120
            LABELS  stable_diffusion wave_d stress not_release_critical
        )
    endif()
endif()
