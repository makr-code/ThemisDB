cmake_minimum_required(VERSION 3.20)
# Placeholder module test directory for mirrored src structure.

# ---------------------------------------------------------------------------
# GRPC-INT integration tests: server start, loopback call, graceful shutdown.
# ---------------------------------------------------------------------------
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_rpc_grpc_integration_focused.cpp")
    set(_int_target "module_rpc_grpc_integration_focused")
    if(NOT TARGET ${_int_target})
        add_executable(${_int_target}
            test_rpc_grpc_integration_focused.cpp
            ${THEMIS_ROOT_DIR}/src/rpc_grpc/grpc_plugin.cpp
        )
        target_include_directories(${_int_target} PRIVATE
            ${THEMIS_ROOT_DIR}/include
            ${THEMIS_ROOT_DIR}/src
            ${THEMIS_ROOT_DIR}/src/rpc_grpc
        )
        set(_int_link_libs ${TEST_LIBS} Threads::Threads)
        if(TARGET gRPC::grpc++)
            list(APPEND _int_link_libs gRPC::grpc++)
            target_compile_definitions(${_int_target} PRIVATE THEMIS_ENABLE_GRPC=1)
        endif()
        target_link_libraries(${_int_target} PRIVATE ${_int_link_libs})
        target_compile_definitions(${_int_target} PRIVATE THEMIS_TEST_BUILD=1)
        themis_register_module_focused_test(
            MODULE  rpc_grpc
            NAME    rpc_grpc_GrpcPluginIntegration
            TARGET  ${_int_target}
            TIER    integration
            TIMEOUT 60
            LABELS  rpc_grpc;integration;grpc
        )
    endif()
endif()

# ---------------------------------------------------------------------------
# CB focused tests: CircuitBreaker state machine, probe, stats, callback.
# ---------------------------------------------------------------------------
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_rpc_grpc_circuit_breaker_focused.cpp")
    set(_cb_target "module_rpc_grpc_circuit_breaker_focused")
    if(NOT TARGET ${_cb_target})
        add_executable(${_cb_target}
            test_rpc_grpc_circuit_breaker_focused.cpp
            ${THEMIS_ROOT_DIR}/src/rpc_grpc/circuit_breaker.cpp
        )
        target_include_directories(${_cb_target} PRIVATE
            ${THEMIS_ROOT_DIR}/include
            ${THEMIS_ROOT_DIR}/src
        )
        target_link_libraries(${_cb_target} PRIVATE
            ${TEST_LIBS}
            Threads::Threads
        )
        target_compile_definitions(${_cb_target} PRIVATE THEMIS_TEST_BUILD=1)
        themis_register_module_focused_test(
            MODULE  rpc_grpc
            NAME    rpc_grpc_CircuitBreakerFocused
            TARGET  ${_cb_target}
            TIER    unit
            TIMEOUT 60
            LABELS  rpc_grpc;circuit_breaker
        )
    endif()
endif()

# Phase 4 contract-hardening focused tests.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_rpc_grpc_contract_hardening_focused.cpp")
    set(_target "module_rpc_grpc_test_rpc_grpc_contract_hardening_focused_focused")
    if(NOT TARGET ${_target})
        add_executable(${_target}
            test_rpc_grpc_contract_hardening_focused.cpp
        )
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
            MODULE rpc_grpc
            NAME   test_rpc_grpc_contract_hardening_focused_rpc_grpc_FocusedTests
            TARGET ${_target}
            TIER   unit
            TIMEOUT 120
            LABELS  rpc_grpc
        )
    endif()
endif()
