# LLM focused module tests (flat hierarchy under tests/llm)

file(GLOB LLM_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS LLM_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_llm_${_stem}_focused")
    set(_ctest "${_stem}_LlmFocusedTests")

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
    if(_stem STREQUAL "test_llm_wiki_phase3_edge_cases")
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
    # Tests using EXPECT_THAT/HasSubstr need gmock.
    set(_gmock_link "")
    if(TARGET GTest::gmock)
        set(_gmock_link GTest::gmock)
    elseif(TARGET gmock::gmock)
        set(_gmock_link gmock::gmock)
    endif()
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
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
    # Tests using EXPECT_THAT/HasSubstr need gmock.
    set(_gmock_link "")
    if(TARGET GTest::gmock)
        set(_gmock_link GTest::gmock)
    elseif(TARGET gmock::gmock)
        set(_gmock_link gmock::gmock)
    endif()
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
