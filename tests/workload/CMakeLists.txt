# workload module tests

file(GLOB WORKLOAD_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS WORKLOAD_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_workload_${_stem}_focused")
    set(_ctest "${_stem}_workload_FocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")
    if(_stem STREQUAL "test_workload_adaptive_optimizer")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/performance/workload_adaptive_optimizer.cpp
        )
    elseif(_stem STREQUAL "test_workload_fingerprint_engine")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/server/workload_fingerprint_engine.cpp
        )
    endif()
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

    if(_stem STREQUAL "test_workload_adaptive_optimizer" OR
       _stem STREQUAL "test_workload_fingerprint_engine")
        gtest_discover_tests(${_target}
            DISCOVERY_MODE POST_BUILD
            WORKING_DIRECTORY $<TARGET_FILE_DIR:${_target}>
            EXTRA_ARGS --gtest_color=yes
        )
    endif()

    themis_register_module_focused_test(
        MODULE workload
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS workload
    )
endforeach()
