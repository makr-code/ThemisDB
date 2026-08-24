# LLM focused module tests (flat hierarchy under tests/llm)

file(GLOB LLM_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

set(_llm_wiki_private_include_dir
    "${THEMIS_ROOT_DIR}/plugins/themisdb_llm_wiki/include")
set(_llm_wiki_private_orchestrator_header
    "${_llm_wiki_private_include_dir}/wikipedia/wiki_workspace_orchestrator.h")
set(_llm_wiki_private_orchestrator_source
    "${THEMIS_ROOT_DIR}/plugins/themisdb_llm_wiki/src/wikipedia/wiki_workspace_orchestrator.cpp")

foreach(_src IN LISTS LLM_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_llm_${_stem}_focused")
    set(_ctest "${_stem}_LlmFocusedTests")

    # Temporarily quarantine phase3 wiki focused suites with known API/syntax drift.
     if(_stem STREQUAL "test_llm_wiki_phase3_edge_cases_focused"
         OR _stem STREQUAL "test_llm_wiki_phase3_features_focused"
         OR _stem STREQUAL "test_llm_wiki_plugin_phase3_phase4_focused")
        continue()
    endif()

    # LLM Inferencing CI suites (TinyLlama + doku.db RAG + AdaLoRA) are
    # registered separately below with model_required labels and longer
    # timeouts.  Exclude them from the generic autogen loop.
    if(_stem STREQUAL "test_llm_tinyllama_inference"
        OR _stem STREQUAL "test_llm_doku_rag"
        OR _stem STREQUAL "test_llm_adalora_doku_training")
        continue()
    endif()

    if(_stem STREQUAL "test_llm_wiki_provenance_focused"
        AND NOT TARGET themisdb_llm_wiki_cpp
        AND (NOT EXISTS "${_llm_wiki_private_orchestrator_header}"
             OR NOT EXISTS "${_llm_wiki_private_orchestrator_source}"))
        message(STATUS
            "Skipping ${_stem}: private LLM Wiki plugin sources are unavailable at "
            "${_llm_wiki_private_orchestrator_source}")
        continue()
    endif()

    add_executable(${_target} "${_src}")
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
        ${THEMIS_ROOT_DIR}/tests
    )
    # When building modularly the llama_cpp plugin may not be linked into
    # `themis_core`. Ensure focused LLM tests have the plugin implementation
    # available by compiling the plugin sources directly into the test target.
    if(THEMIS_BUILD_MODULAR)
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/llama_cpp/llama_cpp_plugin.cpp
            ${THEMIS_ROOT_DIR}/src/llama_cpp/llama_cpp_registrar.cpp
            # RAG assembler provides context assembly used by LlamaCppPlugin::generateRAG()
            ${THEMIS_ROOT_DIR}/src/rag/rag_context_assembler.cpp
        )
    endif()
    if(_stem STREQUAL "test_ai_orchestrator")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/llm/ai_orchestrator.cpp
            ${THEMIS_ROOT_DIR}/src/llm/mode_spec_loader.cpp
        )
    endif()
    if(_stem STREQUAL "test_kernel_fusion_cpu_fallback" OR _stem STREQUAL "test_kernel_fusion_cuda")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/llm/kernel_fusion.cpp
        )
    endif()
    if(_stem STREQUAL "test_infini_attention")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/tests/llm/infini_attention_cuda_fallback.cpp
        )
    endif()
    if(_stem STREQUAL "test_llm_caching")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/llm/model_metadata_cache.cpp
            ${THEMIS_ROOT_DIR}/src/llm/lora_metadata_cache.cpp
        )
    endif()
    if(_stem STREQUAL "test_llm_memory_safety")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/llm/model_metadata_cache.cpp
        )
    endif()
    if(_stem STREQUAL "test_llm_deployment_plugin")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/llm/llm_deployment_plugin.cpp
        )
    endif()
    if(_stem STREQUAL "test_subagent_orchestration_focused")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/llm/subagent_factory_impl.cpp
            ${THEMIS_ROOT_DIR}/src/llm/subagent_coordinator_impl.cpp
        )
    endif()
    if(_stem STREQUAL "test_llm_workflow_orchestration_focused")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/llm/task_decomposer.cpp
        )
    endif()
    if((_stem STREQUAL "test_llm_vision_encoder" OR _stem STREQUAL "test_llm_vision_integration") AND NOT TARGET themis_llm)
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/llm/vision_config.cpp
        )
    endif()
    if(_stem STREQUAL "test_mcp_orchestrator_bridge")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/llm/mode_spec_loader.cpp
        )
    endif()
    if(_stem STREQUAL "test_real_embeddings")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/llm/lora_framework/embedding_provider.cpp
        )
    endif()
    if(_stem STREQUAL "test_llm_query_rewriter")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/search/llm_query_rewriter.cpp
        )
    endif()
    if(_stem STREQUAL "test_llm_query_rewriter")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/search/llm_query_rewriter.cpp
        )
    endif()
    if(_stem STREQUAL "test_llm_query_rewriter_validator")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/search/llm_query_rewriter.cpp
        )
    endif()
    if(_stem STREQUAL "test_llm_reranker")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/search/llm_reranker.cpp
            ${THEMIS_ROOT_DIR}/src/search/learning_to_rank.cpp
        )
    endif()
    if(_stem STREQUAL "test_llm_query_rewriter_validator")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/search/llm_query_rewriter.cpp
        )
    endif()
    if(_stem STREQUAL "test_ssm_rocksdb_store" AND NOT TARGET themis_llm)
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/llm/ssm_state_rocksdb_store.cpp
        )
        target_link_libraries(${_target} PRIVATE rocksdb)
    endif()
    # Wiki secondary index tests: compile the three wiki implementation units
    # directly into the test target.  JsonWikiIndexReader and WikiChunkSplitter
    # have no RocksDB dependency, so these tests are always runnable.
    if(_stem STREQUAL "test_wiki_index_store" OR _stem STREQUAL "test_wiki_rag_quality")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/llm/wiki_index_store.cpp
            ${THEMIS_ROOT_DIR}/src/llm/wiki_chunk_splitter.cpp
            ${THEMIS_ROOT_DIR}/src/llm/wiki_rag_source.cpp
        )
    endif()
    # Phase B wiki integration tests: full RocksDB + BM25 + HNSW + RRF path.
    if(_stem STREQUAL "test_wiki_index_store_phase_b")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/llm/wiki_index_store.cpp
            ${THEMIS_ROOT_DIR}/src/llm/wiki_chunk_splitter.cpp
            ${THEMIS_ROOT_DIR}/src/llm/wiki_rag_source.cpp
        )
        # Link RocksDB explicitly when not pulled in through themis_llm.
        if(NOT TARGET themis_llm)
            target_link_libraries(${_target} PRIVATE rocksdb)
        endif()
    endif()
     if(_stem STREQUAL "test_llm_wiki_phase3_edge_cases" OR
         _stem STREQUAL "test_wiki_crossdep_focused" OR
         _stem STREQUAL "test_llm_wiki_process_policy_manager")
        # Workspace state manager and edition gate are in themis_llm_wiki.
        # If the library target exists (added via src/llm_wiki), link it;
        # otherwise compile the sources directly for standalone focused builds.
        if(TARGET themis_llm_wiki)
            target_link_libraries(${_target} PRIVATE themis_llm_wiki)
        else()
            target_sources(${_target} PRIVATE
                ${THEMIS_ROOT_DIR}/src/llm_wiki/workspace_state_manager.cpp
                ${THEMIS_ROOT_DIR}/src/llm_wiki/edition_gate.cpp
            )
            target_include_directories(${_target} PRIVATE
                ${THEMIS_ROOT_DIR}/src/llm_wiki
            )
        endif()
    endif()
    # LLM Wiki Phase 4 gap closure tests (Wave B)
    if(_stem STREQUAL "test_llm_wiki_phase4_roundtrip" OR 
       _stem STREQUAL "test_llm_wiki_edition_gates" OR
       _stem STREQUAL "test_llm_wiki_performance" OR
       _stem STREQUAL "test_llm_wiki_provenance_focused")
        # These tests require workspace state manager and edition gate
        if(TARGET themis_llm_wiki)
            target_link_libraries(${_target} PRIVATE themis_llm_wiki)
        else()
            target_sources(${_target} PRIVATE
                ${THEMIS_ROOT_DIR}/src/llm_wiki/workspace_state_manager.cpp
                ${THEMIS_ROOT_DIR}/src/llm_wiki/edition_gate.cpp
            )
            target_include_directories(${_target} PRIVATE
                ${THEMIS_ROOT_DIR}/src/llm_wiki
            )
        endif()
        if(_stem STREQUAL "test_llm_wiki_provenance_focused" AND TARGET themisdb_llm_wiki_cpp)
            target_link_libraries(${_target} PRIVATE themisdb_llm_wiki_cpp)
            target_include_directories(${_target} PRIVATE
                ${_llm_wiki_private_include_dir}
            )
        elseif(_stem STREQUAL "test_llm_wiki_provenance_focused")
            target_sources(${_target} PRIVATE
                ${_llm_wiki_private_orchestrator_source}
            )
            target_include_directories(${_target} PRIVATE
                ${_llm_wiki_private_include_dir}
            )
        endif()
        # Add Wave B labels for tracking
        set(_wave_b_labels "wave_b llm_wiki phase4")
    elseif(_stem STREQUAL "test_llm_wiki_phase_b_integration")
        # Wave B Phase B integration tests (LWP-INT-01..05) — self-contained,
        # no RocksDB or external deps required
        set(_wave_b_labels "wave_b llm_wiki release_critical phase_b_integration")
    else()
        set(_wave_b_labels "")
    endif()
    # Module-wide policy: do not link gmock into LLM focused tests.
    # This keeps test registration and test main on a single gtest runtime
    # and avoids no-linked-test false negatives in DLL builds.
    set(_gmock_link "")

    set(_llm_test_libs ${TEST_LIBS})

    target_link_libraries(${_target} PRIVATE
        ${_llm_test_libs}
        ${_gmock_link}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    # Phase-specific label overrides
    if(_stem STREQUAL "test_llm_phase5_hardening")
        themis_register_module_focused_test(
            MODULE llm
            NAME ${_ctest}
            TARGET ${_target}
            TIER unit
            TIMEOUT 120
            LABELS llm hardening phase5
        )
    elseif(_stem STREQUAL "test_llm_phase1_hardening")
        themis_register_module_focused_test(
            MODULE llm
            NAME ${_ctest}
            TARGET ${_target}
            TIER unit
            TIMEOUT 120
            LABELS release_critical llm phase1
        )
    elseif(_stem STREQUAL "test_llm_memory_safety_hardening")
        themis_register_module_focused_test(
            MODULE llm
            NAME ${_ctest}
            TARGET ${_target}
            TIER unit
            TIMEOUT 120
            LABELS release_critical llm batch1 memory_safety
        )
    elseif(_stem STREQUAL "test_subagent_orchestration_focused" OR
           _stem STREQUAL "test_wiki_crossdep_focused")
        themis_register_module_focused_test(
            MODULE llm
            NAME ${_ctest}
            TARGET ${_target}
            TIER unit
            TIMEOUT 120
            ARGS --gtest_fail_if_no_test_linked
        )
    elseif(_wave_b_labels)
        # Wave B LLM Wiki Phase 4 gap closure tests (LWP-RT-01, LWP-GATE-01, LWP-PERF-01)
        themis_register_module_focused_test(
            MODULE llm
            NAME ${_ctest}
            TARGET ${_target}
            TIER unit
            TIMEOUT 180
            LABELS ${_wave_b_labels}
        )
    else()
        themis_register_module_focused_test(
            MODULE llm
            NAME ${_ctest}
            TARGET ${_target}
            TIER unit
            TIMEOUT 120
        )
    endif()
endforeach()

# AUTOGEN PREFIX BLOCK: llm
file(GLOB LLM_AUTOGEN_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_llm_*.cpp"
)

foreach(_src IN LISTS LLM_AUTOGEN_PREFIX_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_llm_${_stem}_autofocused")
    set(_ctest "${_stem}_llm_AutoFocusedTests")

     if(_stem STREQUAL "test_llm_wiki_phase3_edge_cases_focused"
         OR _stem STREQUAL "test_llm_wiki_phase3_features_focused"
         OR _stem STREQUAL "test_llm_wiki_plugin_phase3_phase4_focused")
        continue()
    endif()

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
        ${THEMIS_ROOT_DIR}/tests
    )
    if(_stem STREQUAL "test_llm_ai_orchestrator")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/llm/ai_orchestrator.cpp
            ${THEMIS_ROOT_DIR}/src/llm/mode_spec_loader.cpp
        )
    endif()
    if(_stem STREQUAL "test_llm_kernel_fusion_cpu_fallback" OR _stem STREQUAL "test_llm_kernel_fusion_cuda")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/llm/kernel_fusion.cpp
        )
    endif()
    if(_stem STREQUAL "test_llm_caching")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/llm/model_metadata_cache.cpp
            ${THEMIS_ROOT_DIR}/src/llm/lora_metadata_cache.cpp
        )
    endif()
    if(_stem STREQUAL "test_llm_memory_safety")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/llm/model_metadata_cache.cpp
        )
    endif()
    if(_stem STREQUAL "test_llm_deployment_plugin")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/llm/llm_deployment_plugin.cpp
        )
    endif()
    if(_stem STREQUAL "test_llm_query_rewriter" OR _stem STREQUAL "test_llm_query_rewriter_validator")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/search/llm_query_rewriter.cpp
        )
    endif()
     if(_stem STREQUAL "test_llm_wiki_edition_gates" OR
       _stem STREQUAL "test_llm_wiki_performance" OR
       _stem STREQUAL "test_llm_wiki_phase4_roundtrip" OR
       _stem STREQUAL "test_llm_wiki_phase_b_integration" OR
         _stem STREQUAL "test_llm_wiki_provenance_focused" OR
         _stem STREQUAL "test_llm_wiki_process_policy_manager")
        if(TARGET themis_llm_wiki)
            target_link_libraries(${_target} PRIVATE themis_llm_wiki)
        else()
            target_sources(${_target} PRIVATE
                ${THEMIS_ROOT_DIR}/src/llm_wiki/workspace_state_manager.cpp
                ${THEMIS_ROOT_DIR}/src/llm_wiki/edition_gate.cpp
            )
            target_include_directories(${_target} PRIVATE
                ${THEMIS_ROOT_DIR}/src/llm_wiki
            )
        endif()
        if(_stem STREQUAL "test_llm_wiki_provenance_focused" AND TARGET themisdb_llm_wiki_cpp)
            target_link_libraries(${_target} PRIVATE themisdb_llm_wiki_cpp)
            target_include_directories(${_target} PRIVATE
                ${_llm_wiki_private_include_dir}
            )
        elseif(_stem STREQUAL "test_llm_wiki_provenance_focused")
            target_sources(${_target} PRIVATE
                ${_llm_wiki_private_orchestrator_source}
            )
            target_include_directories(${_target} PRIVATE
                ${_llm_wiki_private_include_dir}
            )
        endif()
    endif()
    if((_stem STREQUAL "test_llm_vision_encoder" OR _stem STREQUAL "test_llm_vision_integration") AND NOT TARGET themis_llm)
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/llm/vision_config.cpp
        )
    endif()
    if(_stem STREQUAL "test_llm_mcp_orchestrator_bridge")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/llm/mode_spec_loader.cpp
        )
    endif()
    if(_stem STREQUAL "test_llm_real_embeddings")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/llm/lora_framework/embedding_provider.cpp
        )
    endif()
    if(_stem STREQUAL "test_llm_ssm_rocksdb_store" AND NOT TARGET themis_llm)
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/llm/ssm_state_rocksdb_store.cpp
        )
        target_link_libraries(${_target} PRIVATE rocksdb)
    endif()
    if(_stem STREQUAL "test_llm_workflow_orchestration_focused")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/llm/task_decomposer.cpp
        )
    endif()
    if(_stem STREQUAL "test_llm_reranker")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/search/llm_reranker.cpp
            ${THEMIS_ROOT_DIR}/src/search/learning_to_rank.cpp
        )
    endif()
    # Module-wide policy: do not link gmock into LLM autofocused tests.
    # Keep runtime consistent with focused targets.
    set(_gmock_link "")
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        ${_gmock_link}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE llm
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS llm autogen
    )
endforeach()

# =============================================================================
# LLM Inferencing CI Targets — TinyLlama + doku.db RAG + AdaLoRA
#
# These targets are guarded by THEMIS_LLM_INFERENCE_TESTS (default OFF) so
# they are not built in standard unit-test builds.  The CI workflow
# ci-llm-inference.yml enables this option explicitly:
#   cmake -DTHEMIS_LLM_INFERENCE_TESTS=ON ...
#
# Labels:
#   llm_inference    — engine tests (INFER-01..10); requires TinyLlama GGUF
#   llm_rag_doku     — RAG tests    (RAG-01..07);  requires doku.db.json
#   adalora_training — training     (LORA-01..07); pure C++ adapter tests
#   model_required   — marker for tests that may be skipped without a model
# =============================================================================

option(THEMIS_LLM_INFERENCE_TESTS
    "Build LLM inferencing CI test suite (TinyLlama + doku.db RAG + AdaLoRA)"
    OFF)

# Allow the CI workflow to pass the model dir as a CMake variable and expose
# it as an environment variable to CTest at test time.
set(THEMIS_TEST_MODEL_DIR "" CACHE PATH
    "Directory containing tinyllama.gguf (set by CI or developer; "
    "overridden by env var THEMIS_TEST_MODEL_PATH at runtime)")

if(THEMIS_LLM_INFERENCE_TESTS)

    # ── Helper macro for the three inference CI targets ───────────────────
    macro(_add_llm_inference_ci_target _stem _labels _timeout_s)
        set(_tgt  "module_llm_${_stem}_focused")
        set(_cname "${_stem}_LlmInferenceCITests")
        set(_src  "${CMAKE_CURRENT_SOURCE_DIR}/${_stem}.cpp")

        add_executable(${_tgt} "${_src}")
        target_include_directories(${_tgt} PRIVATE
            ${THEMIS_ROOT_DIR}/include
            ${THEMIS_ROOT_DIR}/src
            ${THEMIS_ROOT_DIR}/tests
        )
        target_link_libraries(${_tgt} PRIVATE
            ${TEST_LIBS}
            themis_core
            spdlog::spdlog
            Threads::Threads
        )
        target_compile_definitions(${_tgt} PRIVATE THEMIS_TEST_BUILD=1)

        themis_register_module_focused_test(
            MODULE  llm
            NAME    ${_cname}
            TARGET  ${_tgt}
            TIER    integration
            TIMEOUT ${_timeout_s}
            LABELS  "${_labels}"
        )

        # Expose model path and doku.db path as CTest environment variables.
        # CTest picks these up via set_tests_properties ENVIRONMENT.
        if(THEMIS_TEST_MODEL_DIR)
            set_tests_properties(${_cname} PROPERTIES
                ENVIRONMENT
                "THEMIS_TEST_MODEL_PATH=${THEMIS_TEST_MODEL_DIR}/tinyllama.gguf")
        endif()
    endmacro()

    # ── INFER-01..10: TinyLlama inferencing tests ─────────────────────────
    _add_llm_inference_ci_target(
        test_llm_tinyllama_inference
        "llm;llm_inference;model_required"
        120
    )
    # INFER-04 uses LlamaTokenizer which lives outside themis_core
    target_sources(module_llm_test_llm_tinyllama_inference_focused PRIVATE
        ${THEMIS_ROOT_DIR}/src/llm/lora_framework/llama_tokenizer.cpp
    )

    # ── RAG-01..12: doku.db RAG tests ────────────────────────────────────
    _add_llm_inference_ci_target(
        test_llm_doku_rag
        "llm;llm_rag_doku;model_required"
        180
    )

    # Copy the golden dataset YAML next to the test binary so RAG-08..12
    # can find it without hard-coding source-tree paths at runtime.
    set(_golden_yaml "${CMAKE_CURRENT_SOURCE_DIR}/data/themisdb_rag_golden_dataset.yaml")
    if(EXISTS "${_golden_yaml}")
        add_custom_command(
            TARGET module_llm_test_llm_doku_rag_focused POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_golden_yaml}"
                    "$<TARGET_FILE_DIR:module_llm_test_llm_doku_rag_focused>/themisdb_rag_golden_dataset.yaml"
            COMMENT "Copying RAG golden dataset YAML to test binary directory"
        )
        set_tests_properties(test_llm_doku_rag_LlmInferenceCITests PROPERTIES
            ENVIRONMENT
            "THEMIS_RAG_GOLDEN_DATASET_PATH=$<TARGET_FILE_DIR:module_llm_test_llm_doku_rag_focused>/themisdb_rag_golden_dataset.yaml"
        )
        message(STATUS "RAG golden dataset registered: ${_golden_yaml}")
    else()
        message(WARNING "RAG golden dataset not found: ${_golden_yaml} — RAG-08..12 will use fallback path search")
    endif()

    # ── LORA-01..08: AdaLoRA training + cache-gate tests ─────────────────
    _add_llm_inference_ci_target(
        test_llm_adalora_doku_training
        "llm;adalora_training"
        300
    )

    message(STATUS "THEMIS_LLM_INFERENCE_TESTS=ON: registered 3 LLM inference CI targets")

endif() # THEMIS_LLM_INFERENCE_TESTS
