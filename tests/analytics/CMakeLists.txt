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
            ${THEMIS_ROOT_DIR}/src/analytics/knowledge_base.cpp
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
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE analytics
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
    )
endforeach()
