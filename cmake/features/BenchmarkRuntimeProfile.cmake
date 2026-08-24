# ThemisDB Benchmark Runtime Profile
# Ensures benchmark builds run with architecture-valid feature stacks.

if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

option(THEMIS_BENCHMARK_AUTO_ENABLE_CORE_RUNTIME
    "Force core runtime prerequisites for benchmark builds (GPU + HTTP server)"
    ON)

option(THEMIS_BENCHMARK_ENABLE_EXTENDED_STACK
    "Auto-enable extended benchmark stack (LLM + network protocol features)"
    ON)

set(_themis_bench_profile_changes "")

function(_themis_bench_profile_enable _feature _reason)
    if(NOT DEFINED ${_feature} OR NOT ${_feature})
        set(${_feature} ON CACHE BOOL "${_reason}" FORCE)
        set(_themis_bench_profile_changes "${_themis_bench_profile_changes};${_feature}=ON" PARENT_SCOPE)
    endif()
endfunction()

if(THEMIS_BENCHMARK_AUTO_ENABLE_CORE_RUNTIME)
    _themis_bench_profile_enable(THEMIS_ENABLE_GPU "Auto-enabled: benchmark core runtime requires GPU support")
    _themis_bench_profile_enable(THEMIS_ENABLE_HTTP_SERVER "Auto-enabled: benchmark core runtime requires HTTP server")
endif()

if(THEMIS_BENCHMARK_ENABLE_EXTENDED_STACK)
    _themis_bench_profile_enable(THEMIS_ENABLE_LLM "Auto-enabled: benchmark extended stack requires LLM")
    _themis_bench_profile_enable(THEMIS_ENABLE_GRPC "Auto-enabled: benchmark extended stack requires gRPC")
    _themis_bench_profile_enable(THEMIS_ENABLE_WEBSOCKET "Auto-enabled: benchmark extended stack requires WebSocket")
    _themis_bench_profile_enable(THEMIS_ENABLE_MQTT "Auto-enabled: benchmark extended stack requires MQTT")
    _themis_bench_profile_enable(THEMIS_ENABLE_MCP "Auto-enabled: benchmark extended stack requires MCP")
    _themis_bench_profile_enable(THEMIS_ENABLE_SSE "Auto-enabled: benchmark extended stack requires SSE")
    _themis_bench_profile_enable(THEMIS_ENABLE_GRAPHQL "Auto-enabled: benchmark extended stack requires GraphQL")
    _themis_bench_profile_enable(THEMIS_ENABLE_CONTENT "Auto-enabled: benchmark extended stack requires content processing")
endif()

list(REMOVE_ITEM _themis_bench_profile_changes "")
list(LENGTH _themis_bench_profile_changes _themis_bench_profile_change_count)
if(_themis_bench_profile_change_count GREATER 0)
    list(JOIN _themis_bench_profile_changes ", " _themis_bench_profile_change_msg)
    message(STATUS "Benchmark runtime profile: auto-enabled features -> ${_themis_bench_profile_change_msg}")
else()
    message(STATUS "Benchmark runtime profile: requested feature stacks already active")
endif()

if(THEMIS_BENCHMARK_AUTO_ENABLE_CORE_RUNTIME)
    if(NOT THEMIS_ENABLE_GPU OR NOT THEMIS_ENABLE_HTTP_SERVER)
        message(FATAL_ERROR
            "Benchmark runtime profile violation: THEMIS_ENABLE_GPU and THEMIS_ENABLE_HTTP_SERVER must be ON when THEMIS_BUILD_BENCHMARKS=ON.")
    endif()
endif()

if(THEMIS_BENCHMARK_ENABLE_EXTENDED_STACK)
    set(_themis_missing_extended "")
    foreach(_feature
        THEMIS_ENABLE_LLM
        THEMIS_ENABLE_GRPC
        THEMIS_ENABLE_WEBSOCKET
        THEMIS_ENABLE_MQTT
        THEMIS_ENABLE_MCP
        THEMIS_ENABLE_SSE
        THEMIS_ENABLE_GRAPHQL
        THEMIS_ENABLE_CONTENT)
        if(NOT DEFINED ${_feature} OR NOT ${_feature})
            list(APPEND _themis_missing_extended "${_feature}")
        endif()
    endforeach()

    list(LENGTH _themis_missing_extended _themis_missing_extended_count)
    if(_themis_missing_extended_count GREATER 0)
        list(JOIN _themis_missing_extended ", " _themis_missing_extended_msg)
        message(WARNING
            "Benchmark runtime profile: extended stack incomplete (${_themis_missing_extended_msg}). "
            "Some benchmark suites will be configuration-gated or produce degraded evidence.")
    endif()
endif()
