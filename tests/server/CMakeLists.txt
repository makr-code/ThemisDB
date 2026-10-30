# Server module tests

find_package(benchmark QUIET CONFIG)

set(_server_benchmark_target "")
if(TARGET benchmark::benchmark)
    set(_server_benchmark_target benchmark::benchmark)
elseif(TARGET benchmark)
    set(_server_benchmark_target benchmark)
endif()

file(GLOB SERVER_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

find_package(benchmark QUIET CONFIG)
set(_server_benchmark_target "")
if(TARGET benchmark::benchmark)
    set(_server_benchmark_target benchmark::benchmark)
elseif(TARGET benchmark)
    set(_server_benchmark_target benchmark)
endif()

foreach(_src IN LISTS SERVER_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_server_${_stem}_focused")
    set(_ctest "${_stem}_server_FocusedTests")
    file(READ "${_src}" _src_contents)
    string(FIND "${_src_contents}" "BENCHMARK(" _has_benchmark_macro)

    if(_has_benchmark_macro GREATER -1 AND NOT _server_benchmark_target)
        message(STATUS "Skipping ${_target}: no valid benchmark target is available")
        continue()
    endif()

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    set(_server_test_libs ${TEST_LIBS})
    if(_stem STREQUAL "test_api_integration" AND TEST_GMOCK_LIB)
        list(REMOVE_ITEM _server_test_libs ${TEST_GMOCK_LIB})
    endif()
    target_link_libraries(${_target} PRIVATE
        ${_server_test_libs}
        ${_server_benchmark_target}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    if(_stem STREQUAL "test_server_phase2_focused")
        if(_server_benchmark_target)
            target_link_libraries(${_target} PRIVATE ${_server_benchmark_target})
            target_compile_definitions(${_target} PRIVATE THEMIS_HAS_BENCHMARK=1)
        else()
            target_compile_definitions(${_target} PRIVATE THEMIS_HAS_BENCHMARK=0)
        endif()
    endif()
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    if(_stem STREQUAL "test_wave4a_server_hardening2")
        themis_register_module_focused_test(
            MODULE server
            NAME ${_ctest}
            TARGET ${_target}
            TIER unit
            TIMEOUT 120
            LABELS wave_a release_critical server hardening
        )
    elseif(_stem STREQUAL "test_wave7_server_llm_hardening")
        themis_register_module_focused_test(
            MODULE server
            NAME ${_ctest}
            TARGET ${_target}
            TIER unit
            TIMEOUT 120
            LABELS wave_a release_critical wave_a_flow_server_llm server llm hardening
        )
    elseif(_stem STREQUAL "test_server_phase5_hardening")
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
            LABELS release_critical wave_a_flow_server_llm wave_a_flow_server_query_storage_transaction server phase2 gateway
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
    elseif(_stem STREQUAL "test_mcp_kg_tools")
        themis_register_module_focused_test(
            MODULE server
            NAME ${_ctest}
            TARGET ${_target}
            TIER unit
            TIMEOUT 120
            LABELS wave_b release_critical server mcp kg
        )
    elseif(_stem STREQUAL "test_mcp_search_tools")
        themis_register_module_focused_test(
            MODULE server
            NAME ${_ctest}
            TARGET ${_target}
            TIER unit
            TIMEOUT 120
            LABELS wave_b release_critical server mcp search
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