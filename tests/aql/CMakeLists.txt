# AQL focused module tests (flat hierarchy under tests/aql)

file(GLOB AQL_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_aql_*.cpp"
)

# Some LLM-oriented AQL implementations are not part of themis_query in modular
# builds. Keep these focused tests self-contained to avoid unresolved externals.
set(AQL_FORCE_LOCAL_IMPL_TESTS
    test_aql_autocomplete
    test_aql_agent
    test_aql_lora_finetuner
    test_aql_model_router
    test_aql_query_diff_explainer
    test_aql_rollback_suggester
)

foreach(_src IN LISTS AQL_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_aql_${_stem}_focused")
    set(_ctest "${_stem}_AqlFocusedTests")
    set(_extra_sources)
    string(REGEX REPLACE "^test_" "" _impl_stem "${_stem}")
    set(_impl_src "${THEMIS_ROOT_DIR}/src/aql/${_impl_stem}.cpp")
    # Only include the implementation source directly if themis_query does NOT
    # exist as a build target. When themis_query is available it already
    # contains every src/aql/*.cpp translation unit (via Unity build), so
    # adding the same file again produces LNK2005 duplicate-symbol errors.
    if(EXISTS "${_impl_src}" AND NOT _stem STREQUAL "test_aql_query_validator"
       AND (NOT TARGET themis_query OR _stem IN_LIST AQL_FORCE_LOCAL_IMPL_TESTS))
        list(APPEND _extra_sources "${_impl_src}")
    endif()

    if(_stem STREQUAL "test_aql_query_validator")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/aql/aql_lora_finetuner.cpp
        )
    endif()
    
    if(_stem STREQUAL "test_episodic_compaction")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/aql/llm_extractive_compressor.cpp
        )
    endif()

    if(_stem STREQUAL "test_p2_d06_benchmarks")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/aql/llm_extractive_compressor.cpp
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
        $<$<TARGET_EXISTS:themis_query>:themis_query>
        $<$<TARGET_EXISTS:themis_llm>:themis_llm>
        $<$<TARGET_EXISTS:themis_analytics>:themis_analytics>
        $<$<TARGET_EXISTS:themis_security>:themis_security>
        $<$<TARGET_EXISTS:themis_sharding>:themis_sharding>
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE aql
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS aql
    )
endforeach()
