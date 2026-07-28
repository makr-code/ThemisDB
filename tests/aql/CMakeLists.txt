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

    # Ensure mutation-focused tests have the necessary implementation units.
    # Always include the validator implementation if present (some modular
    # configurations omit query/ validation sources from themis_query), but
    # only append translator/executor units when themis_query is not built to
    # avoid duplicate-definition LNK2005 errors.
    if(_stem MATCHES "test_aql_mutations_.*")
        # Support both THEMIS_ROOT_DIR and CMAKE_SOURCE_DIR layouts to be robust
        if(EXISTS "${THEMIS_ROOT_DIR}/src/query/aql_mutation_validator.cpp")
            list(APPEND _extra_sources ${THEMIS_ROOT_DIR}/src/query/aql_mutation_validator.cpp)
        elseif(EXISTS "${CMAKE_SOURCE_DIR}/src/query/aql_mutation_validator.cpp")
            list(APPEND _extra_sources ${CMAKE_SOURCE_DIR}/src/query/aql_mutation_validator.cpp)
        endif()
        if(NOT TARGET themis_query)
            if(EXISTS "${THEMIS_ROOT_DIR}/src/query/aql_mutation_translator.cpp")
                list(APPEND _extra_sources ${THEMIS_ROOT_DIR}/src/query/aql_mutation_translator.cpp)
            endif()
            if(EXISTS "${THEMIS_ROOT_DIR}/src/query/aql_translator.cpp")
                list(APPEND _extra_sources ${THEMIS_ROOT_DIR}/src/query/aql_translator.cpp)
            endif()
            if(EXISTS "${THEMIS_ROOT_DIR}/src/query/mutation_executor.cpp")
                list(APPEND _extra_sources ${THEMIS_ROOT_DIR}/src/query/mutation_executor.cpp)
            endif()
        endif()
    endif()

    add_executable(${_target}
        "${_src}"
        ${_extra_sources}
    )
    # Some focused tests require additional implementation units that are
    # not covered by the conditional _extra_sources logic above (edge-cases
    # when modular targets are present or specific helper files live in
    # different directories). Explicitly add the commonly-missing AQL
    # implementation units per-test here to ensure the focused targets link
    # reliably without pulling the entire `themis_query` unity target.
    if(_stem STREQUAL "test_aql_agent")
        # Add the ReActAgent implementation. Only add the LLM handler
        # implementation directly when the themis_llm library target is
        # not available to avoid duplicate-definition LNK2005 errors.
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/aql/aql_agent.cpp
        )
        if(NOT TARGET themis_llm)
            target_sources(${_target} PRIVATE ${THEMIS_ROOT_DIR}/src/aql/llm_aql_handler.cpp)
        endif()
    elseif(_stem STREQUAL "test_aql_autocomplete")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/aql/aql_autocomplete.cpp
        )
    elseif(_stem STREQUAL "test_aql_lora_finetuner")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/aql/aql_lora_finetuner.cpp
        )
    elseif(_stem STREQUAL "test_aql_model_router")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/aql/aql_model_router.cpp
        )
    elseif(_stem STREQUAL "test_aql_query_diff_explainer")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/aql/aql_query_diff_explainer.cpp
        )
    elseif(_stem STREQUAL "test_aql_rollback_suggester")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/aql/aql_rollback_suggester.cpp
        )
    endif()
    # If the validator source exists but for some reason wasn't picked up
    # earlier (generator/ordering edge-case), explicitly add it to the
    # target so the linker has the symbol available in modular builds.
    if(_stem MATCHES "test_aql_mutations_.*")
        if(EXISTS "${THEMIS_ROOT_DIR}/src/query/aql_mutation_validator.cpp")
            target_sources(${_target} PRIVATE ${THEMIS_ROOT_DIR}/src/query/aql_mutation_validator.cpp)
        elseif(EXISTS "${CMAKE_SOURCE_DIR}/src/query/aql_mutation_validator.cpp")
            target_sources(${_target} PRIVATE ${CMAKE_SOURCE_DIR}/src/query/aql_mutation_validator.cpp)
        endif()
    endif()
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
