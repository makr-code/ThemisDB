# observability module tests

file(GLOB OBSERVABILITY_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS OBSERVABILITY_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_observability_${_stem}_focused")
    set(_ctest "${_stem}_observability_FocusedTests")

    if(TARGET ${_target})
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
    if(_stem STREQUAL "test_high_cardinality_metrics")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/observability/high_cardinality_metrics.cpp
            ${THEMIS_ROOT_DIR}/src/observability/wave_d_high_cardinality_metrics.cpp
        )
    endif()
    if(_stem STREQUAL "test_otel_trace_overhead")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/observability/distributed_trace_span.cpp
            ${THEMIS_ROOT_DIR}/src/observability/trace_instrumentation.cpp
        )
    endif()
    if(_stem STREQUAL "test_otel_exporter_stress")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/observability/exporter_stress_framework.cpp
            ${THEMIS_ROOT_DIR}/src/observability/distributed_trace_span.cpp
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
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        themis_core
        rocksdb
        nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE observability
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS observability
    )
endforeach()