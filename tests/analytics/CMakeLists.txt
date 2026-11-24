# Analytics focused module tests (flat hierarchy under tests/analytics)

file(GLOB ANALYTICS_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS ANALYTICS_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_analytics_${_stem}_focused")
    set(_ctest "${_stem}_AnalyticsFocusedTests")
    set(_extra_sources)
    string(REGEX REPLACE "^test_" "" _impl_stem "${_stem}")
    set(_impl_src "${THEMIS_ROOT_DIR}/src/analytics/${_impl_stem}.cpp")
    set(_link_only_impl_stems
        distributed_analytics
        process_pattern_matcher
        arrow_export
        arrow_flight
        incremental_view
        jit_aggregation
        ml_serving
        model_serving
    )
    if(EXISTS "${_impl_src}" AND NOT _impl_stem IN_LIST _link_only_impl_stems)
        list(APPEND _extra_sources "${_impl_src}")
    endif()

    if(_stem STREQUAL "test_expert_system_engine")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/analytics/knowledge_base.cpp
            ${THEMIS_ROOT_DIR}/src/analytics/model_serving.cpp
            ${THEMIS_ROOT_DIR}/src/analytics/automl.cpp
        )
    endif()

    if(_stem STREQUAL "test_streaming_join")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/analytics/columnar_execution.cpp
        )
    endif()

    if(_stem STREQUAL "test_analytics_gap_closure")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/analytics/expert_system_engine.cpp
            ${THEMIS_ROOT_DIR}/src/analytics/knowledge_base.cpp
            ${THEMIS_ROOT_DIR}/src/analytics/model_serving.cpp
            ${THEMIS_ROOT_DIR}/src/analytics/automl.cpp
        )
    endif()

    if(_stem STREQUAL "test_analytics_bounded_execution_policy")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/analytics/llm_process_analyzer.cpp
        )
    endif()

    if(_stem STREQUAL "test_wave_next_analytics_hardening")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/analytics/forecasting.cpp
        )
    endif()

    if(_stem STREQUAL "test_analytics_memory_pool")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/analytics/columnar_execution.cpp
            ${THEMIS_ROOT_DIR}/src/analytics/cep_engine.cpp
            ${THEMIS_ROOT_DIR}/src/analytics/olap.cpp
            ${THEMIS_ROOT_DIR}/src/gpu/query_accelerator.cpp
            ${THEMIS_ROOT_DIR}/src/gpu/graph_cache.cpp
        )
    elseif(_stem STREQUAL "test_automl" OR _stem STREQUAL "test_automl_onnx")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/analytics/anomaly_detection.cpp
        )
    elseif(_stem STREQUAL "test_distributed_analytics")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/analytics/distributed_analytics.cpp
            ${THEMIS_ROOT_DIR}/src/analytics/olap.cpp
            ${THEMIS_ROOT_DIR}/src/gpu/query_accelerator.cpp
            ${THEMIS_ROOT_DIR}/src/gpu/graph_cache.cpp
        )
    elseif(_stem STREQUAL "test_forecasting_sarima_prophet")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/analytics/forecasting.cpp
        )
    elseif(_stem STREQUAL "test_incremental_view")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/analytics/incremental_view.cpp
            ${THEMIS_ROOT_DIR}/src/analytics/olap.cpp
            ${THEMIS_ROOT_DIR}/src/gpu/query_accelerator.cpp
            ${THEMIS_ROOT_DIR}/src/gpu/graph_cache.cpp
        )
    elseif(_stem STREQUAL "test_jit_aggregation")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/analytics/jit_aggregation.cpp
            ${THEMIS_ROOT_DIR}/src/analytics/columnar_execution.cpp
        )
    elseif(_stem STREQUAL "test_ml_serving")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/analytics/ml_serving.cpp
            ${THEMIS_ROOT_DIR}/src/analytics/anomaly_detection.cpp
        )
    elseif(_stem STREQUAL "test_model_serving")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/analytics/model_serving.cpp
            ${THEMIS_ROOT_DIR}/src/analytics/automl.cpp
            ${THEMIS_ROOT_DIR}/src/analytics/anomaly_detection.cpp
        )
    elseif(_stem STREQUAL "test_olap_lru_cache")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/analytics/olap.cpp
            ${THEMIS_ROOT_DIR}/src/gpu/query_accelerator.cpp
            ${THEMIS_ROOT_DIR}/src/gpu/graph_cache.cpp
        )
    elseif(_stem STREQUAL "test_arrow_export")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/analytics/arrow_export.cpp
            ${THEMIS_ROOT_DIR}/src/analytics/analytics_export.cpp
        )
    elseif(_stem STREQUAL "test_arrow_flight")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/analytics/arrow_flight.cpp
            ${THEMIS_ROOT_DIR}/src/analytics/arrow_export.cpp
        )
    elseif(_stem STREQUAL "test_process_pattern_matcher")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/analytics/process_pattern_matcher.cpp
            ${THEMIS_ROOT_DIR}/src/analytics/process_mining.cpp
        )
    endif()
    list(REMOVE_DUPLICATES _extra_sources)

    add_executable(${_target}
        "${_src}"
        ${_extra_sources}
    )
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
        ${THEMIS_ROOT_DIR}/tests
    )
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        themis_core
        $<$<TARGET_EXISTS:themis_analytics>:themis_analytics>
        $<$<TARGET_EXISTS:themis_query>:themis_query>
        $<$<TARGET_EXISTS:themis_llm>:themis_llm>
        $<$<TARGET_EXISTS:themis_storage>:themis_storage>
        spdlog::spdlog
        Threads::Threads
    )
    if(_stem STREQUAL "test_process_pattern_matcher")
        foreach(_mod themis_base themis_security)
            if(TARGET ${_mod})
                target_link_libraries(${_target} PRIVATE ${_mod})
            endif()
        endforeach()
        target_link_libraries(${_target} PRIVATE
            nlohmann_json::nlohmann_json
            RocksDB::rocksdb
        )
    elseif(_stem STREQUAL "test_ml_serving" AND TARGET onnxruntime::onnxruntime)
        target_link_libraries(${_target} PRIVATE onnxruntime::onnxruntime)
    elseif(_stem STREQUAL "test_arrow_export")
        set(_themis_arrow_export_linked FALSE)
        if(DEFINED THEMIS_ARROW_TARGET AND TARGET ${THEMIS_ARROW_TARGET})
            target_link_libraries(${_target} PRIVATE ${THEMIS_ARROW_TARGET})
            set(_themis_arrow_export_linked TRUE)
        elseif(TARGET Arrow::arrow_shared)
            target_link_libraries(${_target} PRIVATE Arrow::arrow_shared)
            set(_themis_arrow_export_linked TRUE)
        elseif(TARGET Arrow::arrow_static)
            target_link_libraries(${_target} PRIVATE Arrow::arrow_static)
            set(_themis_arrow_export_linked TRUE)
        elseif(TARGET Arrow::arrow)
            target_link_libraries(${_target} PRIVATE Arrow::arrow)
            set(_themis_arrow_export_linked TRUE)
        elseif(EXISTS "${_themis_vcpkg_lib_dir}/arrow.lib")
            target_link_libraries(${_target} PRIVATE "${_themis_vcpkg_lib_dir}/arrow.lib")
            set(_themis_arrow_export_linked TRUE)
        endif()
        if(_themis_arrow_export_linked)
            target_compile_definitions(${_target} PRIVATE THEMIS_HAS_ARROW=1)
        endif()

        set(_themis_parquet_export_linked FALSE)
        if(DEFINED THEMIS_PARQUET_TARGET AND TARGET ${THEMIS_PARQUET_TARGET})
            target_link_libraries(${_target} PRIVATE ${THEMIS_PARQUET_TARGET})
            set(_themis_parquet_export_linked TRUE)
        elseif(TARGET Parquet::parquet_shared)
            target_link_libraries(${_target} PRIVATE Parquet::parquet_shared)
            set(_themis_parquet_export_linked TRUE)
        elseif(TARGET Parquet::parquet_static)
            target_link_libraries(${_target} PRIVATE Parquet::parquet_static)
            set(_themis_parquet_export_linked TRUE)
        elseif(TARGET Parquet::parquet)
            target_link_libraries(${_target} PRIVATE Parquet::parquet)
            set(_themis_parquet_export_linked TRUE)
        elseif(EXISTS "${_themis_vcpkg_lib_dir}/parquet.lib")
            target_link_libraries(${_target} PRIVATE "${_themis_vcpkg_lib_dir}/parquet.lib")
            set(_themis_parquet_export_linked TRUE)
        endif()
        if(_themis_parquet_export_linked)
            target_compile_definitions(${_target} PRIVATE THEMIS_HAS_PARQUET=1)
        endif()
    elseif(_stem STREQUAL "test_arrow_flight")
        set(_themis_arrow_core_linked FALSE)
        if(DEFINED THEMIS_ARROW_TARGET AND TARGET ${THEMIS_ARROW_TARGET})
            target_link_libraries(${_target} PRIVATE ${THEMIS_ARROW_TARGET})
            set(_themis_arrow_core_linked TRUE)
        elseif(TARGET Arrow::arrow_shared)
            target_link_libraries(${_target} PRIVATE Arrow::arrow_shared)
            set(_themis_arrow_core_linked TRUE)
        elseif(TARGET Arrow::arrow_static)
            target_link_libraries(${_target} PRIVATE Arrow::arrow_static)
            set(_themis_arrow_core_linked TRUE)
        elseif(TARGET Arrow::arrow)
            target_link_libraries(${_target} PRIVATE Arrow::arrow)
            set(_themis_arrow_core_linked TRUE)
        elseif(TARGET arrow_shared)
            target_link_libraries(${_target} PRIVATE arrow_shared)
            set(_themis_arrow_core_linked TRUE)
        elseif(TARGET arrow_static)
            target_link_libraries(${_target} PRIVATE arrow_static)
            set(_themis_arrow_core_linked TRUE)
        elseif(TARGET arrow)
            target_link_libraries(${_target} PRIVATE arrow)
            set(_themis_arrow_core_linked TRUE)
        elseif(EXISTS "${_themis_vcpkg_lib_dir}/arrow.lib")
            target_link_libraries(${_target} PRIVATE "${_themis_vcpkg_lib_dir}/arrow.lib")
            set(_themis_arrow_core_linked TRUE)
        endif()
        if(_themis_arrow_core_linked)
            target_compile_definitions(${_target} PRIVATE THEMIS_HAS_ARROW=1)
        endif()

        set(_themis_arrow_flight_linked FALSE)
        if(TARGET ArrowFlight::arrow_flight)
            target_link_libraries(${_target} PRIVATE ArrowFlight::arrow_flight)
            set(_themis_arrow_flight_linked TRUE)
        elseif(TARGET ArrowFlight::arrow_flight_shared)
            target_link_libraries(${_target} PRIVATE ArrowFlight::arrow_flight_shared)
            set(_themis_arrow_flight_linked TRUE)
        elseif(TARGET ArrowFlight::arrow_flight_static)
            target_link_libraries(${_target} PRIVATE ArrowFlight::arrow_flight_static)
            set(_themis_arrow_flight_linked TRUE)
        elseif(TARGET Arrow::arrow_flight)
            target_link_libraries(${_target} PRIVATE Arrow::arrow_flight)
            set(_themis_arrow_flight_linked TRUE)
        elseif(TARGET Arrow::arrow_flight_shared)
            target_link_libraries(${_target} PRIVATE Arrow::arrow_flight_shared)
            set(_themis_arrow_flight_linked TRUE)
        elseif(TARGET Arrow::arrow_flight_static)
            target_link_libraries(${_target} PRIVATE Arrow::arrow_flight_static)
            set(_themis_arrow_flight_linked TRUE)
        elseif(TARGET arrow_flight)
            target_link_libraries(${_target} PRIVATE arrow_flight)
            set(_themis_arrow_flight_linked TRUE)
        elseif(TARGET arrow_flight_shared)
            target_link_libraries(${_target} PRIVATE arrow_flight_shared)
            set(_themis_arrow_flight_linked TRUE)
        elseif(TARGET arrow_flight_static)
            target_link_libraries(${_target} PRIVATE arrow_flight_static)
            set(_themis_arrow_flight_linked TRUE)
        elseif(EXISTS "${_themis_vcpkg_lib_dir}/arrow_flight.lib")
            target_link_libraries(${_target} PRIVATE "${_themis_vcpkg_lib_dir}/arrow_flight.lib")
            set(_themis_arrow_flight_linked TRUE)
        endif()
        if(_themis_arrow_flight_linked)
            if(NOT _themis_arrow_core_linked AND EXISTS "${_themis_vcpkg_lib_dir}/arrow.lib")
                target_link_libraries(${_target} PRIVATE "${_themis_vcpkg_lib_dir}/arrow.lib")
                set(_themis_arrow_core_linked TRUE)
                target_compile_definitions(${_target} PRIVATE THEMIS_HAS_ARROW=1)
            endif()
            target_compile_definitions(${_target} PRIVATE THEMIS_HAS_ARROW_FLIGHT=1)
        else()
            message(STATUS "  Arrow Flight focused test: Arrow Flight library not detected, build may fail if analytics/arrow_flight.cpp requires native symbols")
        endif()
    endif()
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE analytics
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
    )
endforeach()
