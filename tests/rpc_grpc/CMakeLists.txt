cmake_minimum_required(VERSION 3.20)
# Placeholder module test directory for mirrored src structure.

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
            $<$<BOOL:${THEMIS_ENABLE_GRPC}>:gRPC::grpc++>
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
