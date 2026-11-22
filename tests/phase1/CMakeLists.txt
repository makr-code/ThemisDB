# phase1 module tests

file(GLOB PHASE1_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS PHASE1_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_phase1_${_stem}_focused")
    set(_ctest "${_stem}_phase1_FocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
        ${THEMIS_ROOT_DIR}/tests
    )
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    if(THEMIS_ENABLE_LLM AND
       (_stem STREQUAL "test_phase1_flash_attention" OR
        _stem STREQUAL "test_phase1_kv_cache_reuse"))
        if(WIN32)
            themis_sync_runtime_dlls_for_target(${_target})
        endif()
        gtest_discover_tests(${_target}
            DISCOVERY_MODE POST_BUILD
            WORKING_DIRECTORY $<TARGET_FILE_DIR:${_target}>
            EXTRA_ARGS --gtest_color=yes
            CONFIGURATIONS Release Debug
        )
    endif()

    themis_register_module_focused_test(
        MODULE phase1
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS phase1
    )
endforeach()
