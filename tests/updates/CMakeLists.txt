# Auto-generated module tests for prefix 'updates'

file(GLOB UPDATES_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_updates_*.cpp"
)

list(FILTER UPDATES_PREFIX_TEST_SOURCES EXCLUDE REGEX "/test_updates_diagnostics_focused\.cpp$")

foreach(_src IN LISTS UPDATES_PREFIX_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_updates_${_stem}_focused")
    set(_ctest "${_stem}_updates_FocusedTests")
    set(_test_includes
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")
    target_include_directories(${_target} PRIVATE ${_test_includes})
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    set(_updates_test_libs ${TEST_LIBS})
    if(_stem STREQUAL "test_updates_production")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/tests/test_gtest_main.cpp
        )
        list(REMOVE_ITEM _updates_test_libs GTest::gtest_main)
    endif()

    target_link_libraries(${_target} PRIVATE
        ${_updates_test_libs}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

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