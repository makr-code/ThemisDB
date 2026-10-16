cmake_minimum_required(VERSION 3.20)

# Wave 10 Block A: stable_diffusion focused tests.
file(GLOB STABLE_DIFFUSION_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS STABLE_DIFFUSION_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_stable_diffusion_${_stem}_focused")
    set(_ctest "${_stem}_StableDiffusionFocusedTests")
    set(_extra_sources
        ${THEMIS_ROOT_DIR}/src/stable_diffusion/sd_config.cpp
        ${THEMIS_ROOT_DIR}/src/stable_diffusion/sd_prompt_sanitizer.cpp
        ${THEMIS_ROOT_DIR}/src/stable_diffusion/sd_plugin.cpp
        ${THEMIS_ROOT_DIR}/src/stable_diffusion/sd_plugin_registrar.cpp
    )

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target}
        "${_src}"
        ${_extra_sources}
    )
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
        ${THEMIS_ROOT_DIR}/tests
    )
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )
    if(TARGET stable_diffusion_plugin)
        target_link_libraries(${_target} PRIVATE stable_diffusion_plugin)
    endif()
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE stable_diffusion
        NAME   ${_ctest}
        TARGET ${_target}
        TIER   unit
        TIMEOUT 120
        LABELS stable_diffusion wave_10 block_a
    )
endforeach()
