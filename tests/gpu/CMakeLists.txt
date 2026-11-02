# gpu module tests

set(_gpu_graph_traversal_target module_gpu_test_gpu_graph_traversal_focused)
add_executable(${_gpu_graph_traversal_target} test_gpu_graph_traversal.cpp)
target_include_directories(${_gpu_graph_traversal_target} PRIVATE
    ${THEMIS_ROOT_DIR}
    ${THEMIS_ROOT_DIR}/include
    ${THEMIS_ROOT_DIR}/src
    ${THEMIS_ROOT_DIR}/tests
)
target_link_libraries(${_gpu_graph_traversal_target} PRIVATE
    ${TEST_LIBS}
    themis_core
    spdlog::spdlog
    Threads::Threads
)
target_compile_definitions(${_gpu_graph_traversal_target} PRIVATE THEMIS_TEST_BUILD=1)
themis_register_module_focused_test(
    MODULE gpu
    NAME test_gpu_graph_traversal_gpu_FocusedTests
    TARGET ${_gpu_graph_traversal_target}
    TIER unit
    TIMEOUT 120
    LABELS gpu graph bfs dfs cpu-fallback
)

# Only build GPU tests if GPU support is enabled
if(NOT THEMIS_ENABLE_GPU)
    message(STATUS "GPU tests disabled (THEMIS_ENABLE_GPU=OFF) - retaining CPU-fallback graph traversal test")
    return()
endif()

file(GLOB GPU_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

set(_gpu_legacy_discovery_stems
    test_gpu_admin_api
    test_gpu_alerts
    test_gpu_audit_log
    test_gpu_config
    test_gpu_device_discovery
    test_gpu_feature_flags
    test_gpu_graph_cache
    test_gpu_kernel_validator
    test_gpu_launcher
    test_gpu_memory_management
    test_gpu_module
    test_gpu_olap_accelerator
    test_gpu_policy
    test_gpu_query_accelerator
    test_gpu_safe_fail
    test_gpu_safe_fail_module
    test_gpu_stream_manager
    test_gpu_stubs_comprehensive
    test_gpu_tensor
    test_gpu_training_loop
    test_gpu_vector_index
    test_gpu_vram_allocation
)

foreach(_src IN LISTS GPU_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    if(_stem STREQUAL "test_gpu_graph_traversal")
        continue()
    endif()

    set(_target "module_gpu_${_stem}_focused")
    set(_ctest "${_stem}_gpu_FocusedTests")

    # Temporarily quarantine suites that still target a superseded
    # GPUErrorHandler API surface.
     if(_stem STREQUAL "test_gpu_error_handling"
         OR _stem STREQUAL "test_gpu_error_handling_comprehensive"
         OR _stem STREQUAL "test_gpu_memory_raii_batch1"
         OR _stem STREQUAL "test_gpu_phase_c_integration"
         OR _stem STREQUAL "test_gpu_phase2_phase3_focused"
         OR _stem STREQUAL "test_gpu_resource_exhaustion")
        continue()
    endif()

    if(TARGET ${_target})
        continue()
    endif()

    # test_cuda_wrapper_integration is intentionally CPU-stub safe and can run
    # without CUDA by validating the RAII lifecycle contract in the non-CUDA path.
    add_executable(${_target} "${_src}")
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
        ${THEMIS_ROOT_DIR}/tests
    )
    if(_stem STREQUAL "test_gpu_adversarial")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/llm/gpu_safe_fail.cpp
            ${THEMIS_ROOT_DIR}/tests/utils/fault_injector.cpp
        )
        target_link_libraries(${_target} PRIVATE
            GTest::gtest
            GTest::gtest_main
            fmt::fmt
            spdlog::spdlog
            Threads::Threads
        )
    else()
        target_link_libraries(${_target} PRIVATE
            ${TEST_LIBS}
            themis_core
            spdlog::spdlog
            Threads::Threads
        )
        if(_stem STREQUAL "test_cuda_error_hardening")
            if(TEST_GMOCK_LIB)
                target_link_libraries(${_target} PRIVATE ${TEST_GMOCK_LIB})
            else()
                message(WARNING "${_target}: gmock target not found; building without gmock linkage")
            endif()
        endif()
    endif()
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
    if(THEMIS_BUILD_MODULAR AND NOT _stem STREQUAL "test_gpu_adversarial")
        target_sources(${_target} PRIVATE ${THEMIS_ROOT_DIR}/src/acceleration/kernel_registry.cpp)
    endif()

    # Determine labels based on test file
    set(_labels "gpu")
    if(_stem MATCHES "test_gpu_error_handling_comprehensive|test_gpu_phase_c_integration")
        list(APPEND _labels "gpu_phase_c")
    endif()
    if(_stem MATCHES "test_gpu_resource_exhaustion|test_gpu_fallback_all_paths")
        list(APPEND _labels "wave_a" "phase_d" "release_critical")
    endif()
    if(_stem STREQUAL "test_gpu_wave_a_timeout_closure")
        list(APPEND _labels "wave_a" "timeout" "release_critical")
    endif()
    if(_stem STREQUAL "test_gpu_stream_raii")
        list(APPEND _labels "wave_a" "phase_c" "raii" "release_critical")
    endif()

    themis_register_module_focused_test(
        MODULE gpu
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS ${_labels}
    )

    if(_stem IN_LIST _gpu_legacy_discovery_stems)
        if(NOT _stem STREQUAL "test_gpu_vector_index" OR THEMIS_ENABLE_VECTOR_SEARCH)
            add_custom_target(${_stem} DEPENDS ${_target})
        endif()
    endif()

    if(NOT THEMIS_BUILD_MODULAR AND _stem IN_LIST _gpu_legacy_discovery_stems)
        gtest_discover_tests(${_target}
            DISCOVERY_MODE POST_BUILD
            WORKING_DIRECTORY $<TARGET_FILE_DIR:${_target}>
            EXTRA_ARGS --gtest_color=yes
            CONFIGURATIONS Release Debug
        )
    endif()
endforeach()