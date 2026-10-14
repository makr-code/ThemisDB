# observability module tests

# Some focused tests register Google Benchmark cases. Keep benchmark optional so
# unrelated CI lanes can still configure when benchmark package config is absent.
find_package(benchmark QUIET CONFIG)

set(_observability_benchmark_target "")
if(TARGET benchmark::benchmark)
    set(_observability_benchmark_target benchmark::benchmark)
elseif(TARGET benchmark)
    set(_observability_benchmark_target benchmark)
endif()

file(GLOB OBSERVABILITY_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS OBSERVABILITY_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_observability_${_stem}_focused")
    set(_ctest "${_stem}_observability_FocusedTests")
    set(_labels observability)
    file(READ "${_src}" _src_contents)
    string(FIND "${_src_contents}" "BENCHMARK(" _has_benchmark_macro)

    if(TARGET ${_target})
        continue()
    endif()

    if(_has_benchmark_macro GREATER -1 AND NOT _observability_benchmark_target)
        message(STATUS "Skipping ${_target}: no valid benchmark target is available")
        continue()
    endif()

    add_executable(${_target} "${_src}")
    if(_stem STREQUAL "test_field_diagnostics")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/observability/field_diagnostics_collector.cpp
        )
    endif()
    if(_stem STREQUAL "test_provenance_store")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/observability/provenance_store.cpp
        )
    endif()
    if(_stem STREQUAL "test_distributed_tracing_sdk")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/observability/distributed_tracing_sdk.cpp
        )
    endif()
    if(_stem STREQUAL "test_tracing_integration_transaction" OR
       _stem STREQUAL "test_tracing_integration_sharding" OR
       _stem STREQUAL "test_tracing_integration_replication")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/observability/distributed_tracing_sdk.cpp
            ${THEMIS_ROOT_DIR}/src/observability/distributed_trace_span.cpp
            ${THEMIS_ROOT_DIR}/src/observability/trace_instrumentation.cpp
        )
    endif()
    if(_stem STREQUAL "test_high_cardinality_metrics")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/observability/high_cardinality_metrics.cpp
            ${THEMIS_ROOT_DIR}/src/observability/wave_d_high_cardinality_metrics.cpp
        )
    endif()
    if(_stem STREQUAL "test_otel_trace_overhead")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/observability/distributed_tracing_sdk.cpp
            ${THEMIS_ROOT_DIR}/src/observability/distributed_trace_span.cpp
            ${THEMIS_ROOT_DIR}/src/observability/trace_instrumentation.cpp
        )
    endif()
    if(_stem STREQUAL "test_otel_exporter_stress")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/observability/exporter_stress_framework.cpp
            ${THEMIS_ROOT_DIR}/src/observability/distributed_trace_span.cpp
            ${THEMIS_ROOT_DIR}/src/observability/distributed_tracing_sdk.cpp
            ${THEMIS_ROOT_DIR}/src/observability/trace_instrumentation.cpp
        )
    endif()
    if(_stem STREQUAL "test_operator_remediation_engine")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/observability/operator_remediation_engine.cpp
        )
    endif()
    if(_stem STREQUAL "test_observability_phase3_continuation_focused")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/observability/operator_remediation_engine.cpp
        )
    endif()
    if(_stem STREQUAL "test_exporter_stress_framework")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/observability/exporter_stress_framework.cpp
        )
    endif()
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    if(TARGET RocksDB::rocksdb)
        set(_rocksdb_link_target RocksDB::rocksdb)
    elseif(TARGET rocksdb)
        set(_rocksdb_link_target rocksdb)
    elseif(DEFINED THEMIS_ROCKSDB_TARGET AND THEMIS_ROCKSDB_TARGET)
        set(_rocksdb_link_target ${THEMIS_ROCKSDB_TARGET})
    else()
        set(_rocksdb_link_target RocksDB::rocksdb)
    endif()

    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        themis_core
        ${_rocksdb_link_target}
        nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )
    if(TARGET Snappy::snappy)
        target_link_libraries(${_target} PRIVATE Snappy::snappy)
    endif()

    if(_has_benchmark_macro GREATER -1)
        if(NOT _observability_benchmark_target)
            message(STATUS "Skipping ${_target}: benchmark target became unavailable during configuration")
            continue()
        endif()
        target_link_libraries(${_target} PRIVATE ${_observability_benchmark_target})
    endif()

    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    if(_stem STREQUAL "test_observability_contract_hardening_focused" OR
       _stem STREQUAL "test_tracing_integration_transaction" OR
       _stem STREQUAL "test_tracing_integration_sharding")
        list(APPEND _labels wave_b wave_b_boundary_runtime_ops)
    endif()

    themis_register_module_focused_test(
        MODULE observability
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS ${_labels}
    )
endforeach()