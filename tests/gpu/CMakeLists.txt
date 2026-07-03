# gpu module tests

file(GLOB GPU_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS GPU_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_gpu_${_stem}_focused")
    set(_ctest "${_stem}_gpu_FocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    # Suppress compiler warnings for test targets (per-target to avoid
    # cl D9025: '/W4' overwritten by '/W0' when global flags conflict).
    if(DEFINED _THEMIS_TESTS_SUPPRESS_WARNINGS_MVC)
        target_compile_options(${_target} PRIVATE /W0)
    elseif(DEFINED _THEMIS_TESTS_SUPPRESS_WARNINGS_UNIX)
        target_compile_options(${_target} PRIVATE -w)
    endif()

    # In modular builds `themis_core` may be an INTERFACE target without
    # implementation objects. To ensure focused GPU tests have access to the
    # kernel registry implementation when modules are split, compile the
    # central kernel_registry.cpp directly into the focused test target.
    if(THEMIS_BUILD_MODULAR)
        target_sources(${_target} PRIVATE ${THEMIS_ROOT_DIR}/src/acceleration/kernel_registry.cpp)
    endif()

    themis_register_module_focused_test(
        MODULE gpu
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS gpu
    )
endforeach()