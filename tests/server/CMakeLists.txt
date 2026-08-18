# Server module tests

file(GLOB SERVER_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS SERVER_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_server_${_stem}_focused")
    set(_ctest "${_stem}_server_FocusedTests")

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

    if(_stem STREQUAL "test_server_phase5_hardening")
        themis_register_module_focused_test(
            MODULE server
            NAME ${_ctest}
            TARGET ${_target}
            TIER unit
            TIMEOUT 120
            LABELS server hardening phase5
        )
    elseif(_stem STREQUAL "test_server_phase1_hardening")
        themis_register_module_focused_test(
            MODULE server
            NAME ${_ctest}
            TARGET ${_target}
            TIER unit
            TIMEOUT 120
            LABELS release_critical server phase1
        )
    elseif(_stem STREQUAL "test_server_http3_stress_focused")
        themis_register_module_focused_test(
            MODULE server
            NAME ${_ctest}
            TARGET ${_target}
            TIER unit
            TIMEOUT 120
            LABELS release_critical server phase2 http3
        )
    elseif(_stem STREQUAL "test_server_gateway_resilience_focused")
        themis_register_module_focused_test(
            MODULE server
            NAME ${_ctest}
            TARGET ${_target}
            TIER unit
            TIMEOUT 120
            LABELS release_critical server phase2 gateway
        )
    elseif(_stem STREQUAL "test_server_openapi_drift_focused")
        themis_register_module_focused_test(
            MODULE server
            NAME ${_ctest}
            TARGET ${_target}
            TIER unit
            TIMEOUT 120
            LABELS release_critical server phase3 schema
        )
    elseif(_stem STREQUAL "test_server_compat_contract_focused")
        themis_register_module_focused_test(
            MODULE server
            NAME ${_ctest}
            TARGET ${_target}
            TIER unit
            TIMEOUT 120
            LABELS release_critical server phase3 compat
        )
    elseif(_stem STREQUAL "test_server_phase2_focused")
        themis_register_module_focused_test(
            MODULE server
            NAME ${_ctest}
            TARGET ${_target}
            TIER unit
            TIMEOUT 120
            LABELS release_critical server phase2 performance
        )
    else()
        themis_register_module_focused_test(
            MODULE server
            NAME ${_ctest}
            TARGET ${_target}
            TIER unit
            TIMEOUT 120
            LABELS server api
        )
    endif()
endforeach()