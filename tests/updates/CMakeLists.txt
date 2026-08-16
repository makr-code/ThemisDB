# Auto-generated module tests for prefix 'updates'

file(GLOB UPDATES_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_updates_*.cpp"
)

foreach(_src IN LISTS UPDATES_PREFIX_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_updates_${_stem}_focused")
    set(_ctest "${_stem}_updates_FocusedTests")

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

    # Operator diagnostics tests have shorter timeout (Phase 6)
    if(_stem MATCHES "operator_diagnostics")
        themis_register_module_focused_test(
            MODULE updates
            NAME ${_ctest}
            TARGET ${_target}
            TIER unit
            TIMEOUT 60
            LABELS "release_critical;updates;operator_diagnostics"
        )
    else()
        themis_register_module_focused_test(
            MODULE updates
            NAME ${_ctest}
            TARGET ${_target}
            TIER unit
            TIMEOUT 120
            LABELS "release_critical;updates"
        )
    endif()
endforeach()