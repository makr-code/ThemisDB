# Execution module focused test registration for Block A integration.
# Keep the canonical Block A target names aligned with the modular build snapshot.

set(EXECUTION_BLOCK_A_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_execution_engine.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_execution_performance.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_query_compilation.cpp"
)

foreach(_src IN LISTS EXECUTION_BLOCK_A_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_execution_${_stem}_focused")
    set(_ctest "${_stem}_ExecutionFocusedTests")

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

    themis_register_module_focused_test(
        MODULE execution
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS execution wave_10 block_a
    )
endforeach()

# Separate high-cardinality stress tests remain available as non-block-A
# validation. They are intentionally excluded from the focused Block A target set.
