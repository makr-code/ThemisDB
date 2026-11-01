# RAG focused module tests (flat hierarchy under tests/rag)

file(GLOB RAG_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS RAG_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_rag_${_stem}_focused")
    set(_ctest "${_stem}_RagFocusedTests")

    add_executable(${_target} "${_src}")
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    set(_extra_sources)
    if("${_stem}" STREQUAL "test_rag_adaptive_retrieval")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/adaptive_retrieval.cpp
        )
    elseif("${_stem}" STREQUAL "test_rag_rlaif_trainer")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/rlaif_trainer.cpp
        )
    elseif("${_stem}" STREQUAL "test_rag_multi_hop_reasoner")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/multi_hop_reasoner.cpp
        )
    elseif("${_stem}" STREQUAL "test_rag_replug_retriever")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/replug_retriever.cpp
        )
    elseif("${_stem}" STREQUAL "test_rag_context_assembler")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/rag_context_assembler.cpp
        )
    elseif("${_stem}" STREQUAL "test_rag_context_engine")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/ethics_ai/argument_store.cpp
            ${THEMIS_ROOT_DIR}/src/ethics_ai/rag_context_engine.cpp
            ${THEMIS_ROOT_DIR}/src/ethics_ai/ethics_ai_types.cpp
        )
    elseif("${_stem}" STREQUAL "test_metric_computation")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/metric_computation.cpp
        )
    elseif("${_stem}" STREQUAL "test_cost_attribution_tracker")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/cost_attribution_tracker.cpp
        )
    elseif("${_stem}" STREQUAL "test_realtime_slo_tracker")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/realtime_slo_tracker.cpp
        )
    elseif("${_stem}" STREQUAL "test_recommendation_engine")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/recommendation_engine.cpp
        )
    elseif("${_stem}" STREQUAL "test_reranker_budget_gate")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/reranker_budget_gate.cpp
        )
    elseif("${_stem}" STREQUAL "test_reranker_cost_analyzer")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/reranker_cost_analyzer.cpp
        )
    elseif("${_stem}" STREQUAL "test_router_policy_store")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/router_policy_store.cpp
        )
    elseif("${_stem}" STREQUAL "test_cost_model_builder")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/cost_model_builder.cpp
        )
    elseif("${_stem}" STREQUAL "test_wave5_rag_hardening")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/rlaif_trainer.cpp
            ${THEMIS_ROOT_DIR}/src/rag/distributed_rag_evaluator.cpp
        )
        if(NOT THEMIS_ENABLE_LLM)
            list(APPEND _extra_sources
                ${THEMIS_ROOT_DIR}/src/rag/wiki_index_store.cpp
            )
        endif()
    elseif("${_stem}" STREQUAL "test_wave7_rag_costmodel_guardrail")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/tensor_rag_cost_model.cpp
            ${THEMIS_ROOT_DIR}/src/rag/retrieval_guardrail.cpp
            ${THEMIS_ROOT_DIR}/src/rag/rag_quality_monitor.cpp
        )
    elseif("${_stem}" STREQUAL "test_wave7_bm25_positional_fts")
        if(NOT THEMIS_ENABLE_LLM)
            list(APPEND _extra_sources
                ${THEMIS_ROOT_DIR}/src/rag/wiki_index_store.cpp
            )
        endif()
    elseif("${_stem}" STREQUAL "test_adaptive_hybrid_router")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/adaptive_hybrid_router.cpp
            ${THEMIS_ROOT_DIR}/src/rag/router_policy_store.cpp
            ${THEMIS_ROOT_DIR}/src/rag/otel_span_emitter.cpp
        )
    elseif("${_stem}" STREQUAL "test_benchmark_suite")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/benchmark_suite.cpp
        )
    elseif("${_stem}" STREQUAL "test_evaluation_result_store")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/evaluation_result_store.cpp
        )
    elseif("${_stem}" STREQUAL "test_cross_encoder_orchestrator")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/cross_encoder_orchestrator.cpp
        )
    elseif("${_stem}" STREQUAL "test_ingestion_latency_monitor")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/ingestion_latency_monitor.cpp
        )
    elseif("${_stem}" STREQUAL "test_otel_span_emitter")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/otel_span_emitter.cpp
        )
    elseif("${_stem}" STREQUAL "test_query_intent_classifier")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/query_intent_classifier.cpp
        )
    elseif("${_stem}" STREQUAL "test_freshness_sla_enforcer")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/freshness_sla_enforcer.cpp
            ${THEMIS_ROOT_DIR}/src/rag/index_refresh_scheduler.cpp
            ${THEMIS_ROOT_DIR}/src/rag/ingestion_latency_monitor.cpp
        )
    elseif("${_stem}" STREQUAL "test_index_refresh_scheduler")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/index_refresh_scheduler.cpp
        )
    elseif("${_stem}" STREQUAL "test_gradient_descent_optimizer")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/gradient_descent_optimizer.cpp
        )
    elseif("${_stem}" STREQUAL "test_staleness_aware_router")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/staleness_aware_router.cpp
            ${THEMIS_ROOT_DIR}/src/rag/ingestion_latency_monitor.cpp
        )
    endif()
    if(_extra_sources)
        target_sources(${_target} PRIVATE ${_extra_sources})
    endif()
    if("${_stem}" STREQUAL "test_tensor_rag_provenance")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/query/tensor_aware_query_optimizer.cpp
            ${THEMIS_ROOT_DIR}/src/rag/targ_retrieval.cpp
            ${THEMIS_ROOT_DIR}/src/rag/flare_retrieval.cpp
            ${THEMIS_ROOT_DIR}/src/rag/tensor_rag_pipeline.cpp
        )
    endif()
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE rag
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
    )
endforeach()

# AUTOGEN PREFIX BLOCK: rag
file(GLOB RAG_AUTOGEN_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_rag_*.cpp"
)

foreach(_src IN LISTS RAG_AUTOGEN_PREFIX_TEST_SOURCES)
    get_filename_component(_src_real "${_src}" REALPATH)
    set(_registered_targets "")
    get_property(_rag_dir_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    foreach(_rag_tgt IN LISTS _rag_dir_targets)
        if(_rag_tgt STREQUAL "themis_tests")
            continue()
        endif()
        get_target_property(_rag_tgt_type ${_rag_tgt} TYPE)
        if(NOT _rag_tgt_type STREQUAL "EXECUTABLE")
            continue()
        endif()
        get_target_property(_rag_tgt_sources ${_rag_tgt} SOURCES)
        foreach(_rag_tgt_source IN LISTS _rag_tgt_sources)
            if(_rag_tgt_source MATCHES "\\$<")
                continue()
            endif()
            if(NOT IS_ABSOLUTE "${_rag_tgt_source}")
                get_filename_component(
                    _rag_tgt_source
                    "${_rag_tgt_source}"
                    ABSOLUTE
                    BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
                )
            endif()
            get_filename_component(_rag_tgt_source_real "${_rag_tgt_source}" REALPATH)
            if(_rag_tgt_source_real STREQUAL _src_real)
                list(APPEND _registered_targets "${_rag_tgt}")
                break()
            endif()
        endforeach()
    endforeach()

    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_rag_${_stem}_autofocused")
    set(_ctest "${_stem}_rag_AutoFocusedTests")

    list(LENGTH _registered_targets _registered_target_count)
    if(_registered_target_count GREATER 1)
        message(FATAL_ERROR
            "RAG test source ${_src} is already compiled by multiple executable targets: "
            "${_registered_targets}"
        )
    elseif(_registered_target_count EQUAL 1)
        list(GET _registered_targets 0 _registered_target)
        themis_register_module_focused_test(
            MODULE rag
            NAME ${_ctest}
            TARGET ${_registered_target}
            TIER unit
            TIMEOUT 120
            LABELS rag autogen
        )
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
    set(_extra_sources)
    if("${_stem}" STREQUAL "test_rag_adaptive_retrieval")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/adaptive_retrieval.cpp
        )
    elseif("${_stem}" STREQUAL "test_rag_rlaif_trainer")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/rlaif_trainer.cpp
        )
    elseif("${_stem}" STREQUAL "test_rag_multi_hop_reasoner")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/multi_hop_reasoner.cpp
        )
    elseif("${_stem}" STREQUAL "test_rag_replug_retriever")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/replug_retriever.cpp
        )
    elseif("${_stem}" STREQUAL "test_rag_context_assembler")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/rag_context_assembler.cpp
        )
    elseif("${_stem}" STREQUAL "test_rag_context_engine")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/ethics_ai/argument_store.cpp
            ${THEMIS_ROOT_DIR}/src/ethics_ai/rag_context_engine.cpp
            ${THEMIS_ROOT_DIR}/src/ethics_ai/ethics_ai_types.cpp
        )
    elseif("${_stem}" STREQUAL "test_metric_computation")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/metric_computation.cpp
        )
    elseif("${_stem}" STREQUAL "test_adaptive_hybrid_router")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/adaptive_hybrid_router.cpp
            ${THEMIS_ROOT_DIR}/src/rag/router_policy_store.cpp
            ${THEMIS_ROOT_DIR}/src/rag/otel_span_emitter.cpp
        )
    elseif("${_stem}" STREQUAL "test_benchmark_suite")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/benchmark_suite.cpp
        )
    elseif("${_stem}" STREQUAL "test_evaluation_result_store")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/evaluation_result_store.cpp
        )
    elseif("${_stem}" STREQUAL "test_cross_encoder_orchestrator")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/cross_encoder_orchestrator.cpp
        )
    elseif("${_stem}" STREQUAL "test_otel_span_emitter")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/otel_span_emitter.cpp
        )
    elseif("${_stem}" STREQUAL "test_query_intent_classifier")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/rag/query_intent_classifier.cpp
        )
    endif()
    if(_extra_sources)
        target_sources(${_target} PRIVATE ${_extra_sources})
    endif()
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE rag
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS rag autogen
    )
endforeach()

# ─────────────────────────────────────────────────────────────────────────────
# Wave 5 Phase 2 RAG hardening test (explicit entry — not caught by GLOB when
# file is added after initial configure)
# ─────────────────────────────────────────────────────────────────────────────
set(_wave5_src "${CMAKE_CURRENT_SOURCE_DIR}/test_wave5_rag_hardening.cpp")
if(EXISTS "${_wave5_src}")
    if(NOT TARGET module_rag_test_wave5_rag_hardening_focused)
        add_executable(module_rag_test_wave5_rag_hardening_focused "${_wave5_src}")
    endif()
    target_include_directories(module_rag_test_wave5_rag_hardening_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    # The wave-5 hardening checks intentionally exercise trainer/evaluator logic
    # regardless of the LLM feature gate; keep the wiki index fallback only for
    # the no-LLM build path.
    target_sources(module_rag_test_wave5_rag_hardening_focused PRIVATE
        ${THEMIS_ROOT_DIR}/src/rag/rlaif_trainer.cpp
        ${THEMIS_ROOT_DIR}/src/rag/distributed_rag_evaluator.cpp
    )
    if(NOT THEMIS_ENABLE_LLM)
        target_sources(module_rag_test_wave5_rag_hardening_focused PRIVATE
            ${THEMIS_ROOT_DIR}/src/rag/wiki_index_store.cpp
        )
    endif()
    target_link_libraries(module_rag_test_wave5_rag_hardening_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(module_rag_test_wave5_rag_hardening_focused PRIVATE
        THEMIS_TEST_BUILD=1)
endif()

# ─────────────────────────────────────────────────────────────────────────────
# Wave 7 BM25+ Positional + FTS phrase/proximity test
# Labels: wave_b, release_critical
# ─────────────────────────────────────────────────────────────────────────────
set(_wave7_src "${CMAKE_CURRENT_SOURCE_DIR}/test_wave7_bm25_positional_fts.cpp")
if(EXISTS "${_wave7_src}")
    if(NOT TARGET module_rag_test_wave7_bm25_positional_fts_focused)
        add_executable(module_rag_test_wave7_bm25_positional_fts_focused "${_wave7_src}")
    endif()
    target_include_directories(module_rag_test_wave7_bm25_positional_fts_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    if(NOT THEMIS_ENABLE_LLM)
        target_sources(module_rag_test_wave7_bm25_positional_fts_focused PRIVATE
            ${THEMIS_ROOT_DIR}/src/rag/wiki_index_store.cpp
        )
    endif()
    target_link_libraries(module_rag_test_wave7_bm25_positional_fts_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(module_rag_test_wave7_bm25_positional_fts_focused PRIVATE
        THEMIS_TEST_BUILD=1)
endif()

# ─────────────────────────────────────────────────────────────────────────────
# Wave B X1a/X1b/X1c: TensorRagCostModel, RetrievalGuardrail, RagQualityMonitor
# ─────────────────────────────────────────────────────────────────────────────
set(_wave7_costmodel_src
    "${CMAKE_CURRENT_SOURCE_DIR}/test_wave7_rag_costmodel_guardrail.cpp")
if(EXISTS "${_wave7_costmodel_src}")
    if(NOT TARGET module_rag_test_wave7_rag_costmodel_guardrail_focused)
        add_executable(module_rag_test_wave7_rag_costmodel_guardrail_focused
            "${_wave7_costmodel_src}")
    endif()
    target_include_directories(module_rag_test_wave7_rag_costmodel_guardrail_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    target_sources(module_rag_test_wave7_rag_costmodel_guardrail_focused PRIVATE
        ${THEMIS_ROOT_DIR}/src/rag/tensor_rag_cost_model.cpp
        ${THEMIS_ROOT_DIR}/src/rag/retrieval_guardrail.cpp
        ${THEMIS_ROOT_DIR}/src/rag/rag_quality_monitor.cpp
    )
    target_link_libraries(module_rag_test_wave7_rag_costmodel_guardrail_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(module_rag_test_wave7_rag_costmodel_guardrail_focused PRIVATE
        THEMIS_TEST_BUILD=1)
endif()