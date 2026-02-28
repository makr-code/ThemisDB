# ThemisDB Tests CMakeLists.txt
cmake_minimum_required(VERSION 3.20)

# Check if we're building tests
if(NOT THEMIS_BUILD_TESTS)
    message(STATUS "Tests disabled")
    return()
endif()

message(STATUS "Configuring ThemisDB test suite")

if(MSVC)
    message(STATUS "MSVC Runtime: ${CMAKE_MSVC_RUNTIME_LIBRARY}")
endif()

find_package(GTest QUIET CONFIG)
if(NOT GTest_FOUND)
    message(WARNING "GTest not found")
    return()
endif()

# Find yaml-cpp for test configuration
find_package(yaml-cpp CONFIG REQUIRED)

# Shared test libraries so individual test executables don't forget to link gtest
set(TEST_LIBS
    GTest::gtest
    GTest::gtest_main
)

message(STATUS "GTest found - building unified test binary")

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${GTest_INCLUDE_DIRS}
)

file(GLOB_RECURSE ALL_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

list(FILTER ALL_TEST_SOURCES EXCLUDE REGEX ".*test_minimal\\.cpp$")
list(FILTER ALL_TEST_SOURCES EXCLUDE REGEX ".*test_manual.*\\.cpp$")
list(FILTER ALL_TEST_SOURCES EXCLUDE REGEX ".*benchmark.*\\.cpp$")
list(FILTER ALL_TEST_SOURCES EXCLUDE REGEX ".*test_wire_protocol_manual\\.cpp$")
list(FILTER ALL_TEST_SOURCES EXCLUDE REGEX ".*test_env\\.cpp$")
list(FILTER ALL_TEST_SOURCES EXCLUDE REGEX ".*test_auto_failover_recovery\\.cpp$")
list(FILTER ALL_TEST_SOURCES EXCLUDE REGEX ".*test_feature_flags\\.cpp$")
# Exclude tests requiring unimplemented performance monitoring classes
list(FILTER ALL_TEST_SOURCES EXCLUDE REGEX ".*test_pmu_counters\\.cpp$")
list(FILTER ALL_TEST_SOURCES EXCLUDE REGEX ".*test_memory_pressure\\.cpp$")

# Skip feature-dependent tests when the corresponding modules are disabled
set(TEST_EXCLUDE_PATTERNS "")

if(NOT THEMIS_ENABLE_LLM)
    list(APPEND TEST_EXCLUDE_PATTERNS
        ".*test_llm.*\\.cpp$"
        ".*test_lora.*\\.cpp$"
        ".*test_llama.*\\.cpp$"
        ".*test_kv_cache.*\\.cpp$"
        ".*test_meta_prompt.*\\.cpp$"
        ".*test_mixed_precision.*\\.cpp$"
        ".*test_multi_gpu.*\\.cpp$"
        ".*test_multi_lora.*\\.cpp$"
        ".*test_paged_attention\\.cpp$"
        ".*test_production_validator\\.cpp$"
        ".*test_prompt_optimizer\\.cpp$"
        ".*test_qlora.*\\.cpp$"
        ".*test_quantization\\.cpp$"
        ".*test_rag_judge.*\\.cpp$"
        ".*test_rag_streaming_retriever\\.cpp$"
        ".*test_rag_reranker\\.cpp$"
        ".*test_rag_hallucination_dashboard\\.cpp$"
        ".*test_rag_citation_highlighter\\.cpp$"
        ".*test_rag_document_summarizer\\.cpp$"
        ".*test_rag_citation_highlighter\\.cpp$"
        ".*test_rag_knowledge_graph_retriever\\.cpp$"
        ".*test_rag_document_splitter\\.cpp$"
        ".*test_rag_hybrid_retriever\\.cpp$"
        ".*test_rag_agentic\\.cpp$"
        ".*test_raid_lora_integration\\.cpp$"
        ".*test_training_service_registry\\.cpp$"
        ".*test_signature_verifier\\.cpp$"
        ".*test_ethics_aware_confidence_detector\\.cpp$"
        ".*test_ethical_guidelines_manager\\.cpp$"
        ".*test_explanation_generator\\.cpp$"
        ".*test_fewshot_optimizer\\.cpp$"
        ".*test_flash_lora\\.cpp$"
        ".*test_fused_kernels\\.cpp$"
        ".*test_gguf_loader\\.cpp$"
        ".*test_gpu_lora_layers\\.cpp$"
        ".*test_gradient_checkpointing\\.cpp$"
        ".*test_inference_engine_enhanced\\.cpp$"
        ".*test_kernel_fusion\\.cpp$"
        ".*test_kernel_fusion_cpu_fallback\\.cpp$"
        ".*test_kernel_fusion_cuda\\.cpp$"
        ".*test_model_loader_async\\.cpp$"
        ".*test_phase1_flash_attention\\.cpp$"
        ".*test_phase1_kv_cache_reuse\\.cpp$"
        ".*test_ai_decision_auditor\\.cpp$"
        ".*test_ai_orchestrator\\.cpp$"
        ".*test_mcp_orchestrator_bridge\\.cpp$"
        ".*test_byzantine_detector\\.cpp$"
        ".*test_continuous_batch_scheduler\\.cpp$"
        ".*test_data_loader\\.cpp$"
        ".*test_directx_backend\\.cpp$"
        ".*test_docs_assistant_aql\\.cpp$"
        ".*test_aql_lora_finetuner\\.cpp$"
        ".*test_claim_extractor\\.cpp$"
        ".*test_learning_metrics\\.cpp$"
        ".*test_adapter_sync\\.cpp$"
        ".*test_cross_functional_voice_observability\\.cpp$"
        ".*test_process_mining_e2e\\.cpp$"
        ".*test_extended_context\\.cpp$"
        ".*test_model_loader_error_handling\\.cpp$"
        ".*test_model_loading_from_themisdb\\.cpp$"
        ".*test_real_embeddings\\.cpp$"
        ".*test_adaptive_query_cache\\.cpp$"
        ".*test_streaming_handler\\.cpp$"
        ".*test_shared_worker_pool\\.cpp$"
        ".*test_json_schema_binding\\.cpp$"
        ".*test_openai_compat_adapter\\.cpp$"
    )
endif()

if(NOT THEMIS_ENABLE_GPU)
    list(APPEND TEST_EXCLUDE_PATTERNS
        ".*test_collective_backends\\.cpp$"
        ".*test_gpu_vector_index\\.cpp$"
        ".*test_multi_gpu_vector_index\\.cpp$"
        ".*test_multi_gpu_training\\.cpp$"
        ".*test_vram_allocator_null_checks\\.cpp$"
        ".*test_gpu_lora_layers\\.cpp$"
        ".*test_cpu_gpu_parity\\.cpp$"
    )
endif()

if(NOT THEMIS_ENABLE_VULKAN)
    list(APPEND TEST_EXCLUDE_PATTERNS
        ".*test_vulkan_compute_equivalents\\.cpp$"
    )
endif()

if(NOT THEMIS_ENABLE_FLASH_ATTENTION)
    list(APPEND TEST_EXCLUDE_PATTERNS
        ".*test_flash_attention.*\\.cpp$"
    )
endif()

# Data selection pipeline tests belong to the legal training feature, not LLM.
# When THEMIS_ENABLE_LLM is OFF its broad ".*test_lora.*\.cpp$" exclusion would
# incorrectly exclude test_lora_data_selection.cpp, so we add it back explicitly
# when legal training is enabled.
if(NOT THEMIS_ENABLE_LLM AND THEMIS_ENABLE_LEGAL_TRAINING)
    # Re-include: remove the data-selection and training-pipeline e2e tests from
    # the LLM-based exclusion list by explicitly adding them back to ALL_TEST_SOURCES.
    # (list(FILTER … EXCLUDE) has already run; we add them back before the
    # add_executable call.)
    list(APPEND ALL_TEST_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/test_lora_data_selection.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_training_pipeline_e2e.cpp"
    )
    list(REMOVE_DUPLICATES ALL_TEST_SOURCES)
endif()

if(NOT THEMIS_ENABLE_LEGAL_TRAINING)
    list(APPEND TEST_EXCLUDE_PATTERNS
        ".*test_lora_data_selection\\.cpp$"
        ".*test_training_pipeline_e2e\\.cpp$"
    )
endif()

if(NOT THEMIS_ENABLE_WEBSOCKET)
    list(APPEND TEST_EXCLUDE_PATTERNS
        ".*test_wire_protocol_websocket\\.cpp$"
        ".*test_ws_handler\\.cpp$"
        ".*test_websocket_cdc\\.cpp$"
    )
endif()

if(NOT THEMIS_ENABLE_CONTENT)
    list(APPEND TEST_EXCLUDE_PATTERNS
        ".*test_office_processor\\.cpp$"
        ".*test_text_processor\\.cpp$"
        ".*test_mock_clip\\.cpp$"
        ".*test_http_content\\.cpp$"
        ".*test_content_fs\\.cpp$"
        ".*test_content_policy.*\\.cpp$"
        ".*test_pdf_processor\\.cpp$"
        ".*test_content_embedding_pipeline\\.cpp$"
        ".*test_content_html_processor\\.cpp$"
        ".*test_content_markdown_processor\\.cpp$"
    )
endif()

# Tests with own main() - these have custom logic or missing dependencies
list(APPEND TEST_EXCLUDE_PATTERNS
    ".*test_argument_store\\.cpp$"
    ".*test_adaptive_cache_fuzz\\.cpp$"
    ".*test_adaptive_cache_integration\\.cpp$"
    ".*test_adaptive_cache_phase1\\.cpp$"
    ".*test_continuous_learning_client\\.cpp$"
    ".*test_geval\\.cpp$"
    ".*test_metrics\\.cpp$"
    ".*test_onnx_model_loader\\.cpp$"
    ".*test_content_policy_manual\\.cpp$"
    ".*test_ethics_ai_types\\.cpp$"
    ".*test_ethics_evaluator\\.cpp$"
    ".*test_graph_advanced_features\\.cpp$"
    ".*test_approximate_radius_search_integration\\.cpp$"
    ".*test_philosophy_loader\\.cpp$"
    ".*test_metadata_wal_recovery\\.cpp$"
    ".*test_paxos_wal_recovery\\.cpp$"
    ".*test_quality_control_pipeline\\.cpp$"
    ".*test_slo_monitor\\.cpp$"
    ".*test_jsonl_llm_exporter\\.cpp$"
    ".*test_huggingface_exporter\\.cpp$"
)

if(NOT THEMIS_ENABLE_WISCKEY)
    list(APPEND TEST_EXCLUDE_PATTERNS
        ".*test_wisckey.*\\.cpp$"
    )
endif()

# Missing implementations and unfinished features
list(APPEND TEST_EXCLUDE_PATTERNS
    ".*test_merge_operator.*\\.cpp$"
    ".*test_shard_durability.*\\.cpp$"
    ".*test_gpu_erasure_coding.*\\.cpp$"
    ".*test_binary_integrity.*\\.cpp$"
    ".*test_cross_functional_voice_observability\\.cpp$"
    ".*test_adaptive_query_cache\\.cpp$"
    ".*test_byzantine_detector\\.cpp$"
    ".*test_directx_backend\\.cpp$"
    ".*test_ethics_aware_confidence_detector\\.cpp$"
    ".*test_gradient_checkpointing\\.cpp$"
    ".*test_raid_lora_integration\\.cpp$"
    ".*test_distributed_coordinator\\.cpp$"
    ".*test_multi_gpu_lora_advanced\\.cpp$"
    ".*test_enhanced_backup.*\\.cpp$"
    ".*test_gunrock.*\\.cpp$"
    ".*test_splinterdb.*\\.cpp$"
    ".*test_aql_shortestpath.*\\.cpp$"
    ".*test_phase3.*\\.cpp$"
    ".*test_consensus_module\\.cpp$"
    ".*test_content_policy_manual\\.cpp$"
    ".*test_bao\\.cpp$"
    ".*test_bwtree\\.cpp$"
    ".*test_diskann\\.cpp$"
    ".*test_pmem_storage\\.cpp$"
    ".*test_nli_verifier\\.cpp$"
    ".*test_continuous_learning_orchestrator\\.cpp$"
    ".*test_gpu_stream_manager\\.cpp$"
    ".*test_gpu_module\\.cpp$"
    ".*test_query_engine_di\\.cpp$"
    ".*test_raii_wrappers\\.cpp$"
    ".*test_cdc_admin\\.cpp$"
    ".*test_tenant_buffer_manager\\.cpp$"
    ".*test_real_embeddings\\.cpp$"
    ".*test_adapter_sync\\.cpp$"
    ".*test_fewshot_optimizer\\.cpp$"
    ".*test_flash_attention_correctness\\.cpp$"
    ".*test_geo_topology_api_handler\\.cpp$"
    ".*test_gpu_admin_api\\.cpp$"
    ".*test_gpu_.*\\.cpp$"
    ".*test_graphql.*\\.cpp$"
    ".*test_lora_router\\.cpp$"
    ".*test_lora_security\\.cpp$"
    ".*test_multi_gpu_management\\.cpp$"
    ".*test_mvcc_api_handler\\.cpp$"
    ".*test_phi3_integration\\.cpp$"
    ".*test_prompt_.*\\.cpp$"
    ".*test_rag_ethics\\.cpp$"
    ".*test_resource_limits_comprehensive\\.cpp$"
    ".*test_secret_manager_comprehensive\\.cpp$"
    ".*test_policy_change_auditing\\.cpp$"
    ".*test_workload_driven_cache\\.cpp$"
    ".*test_gpu_alerts\\.cpp$"
    ".*test_gpu_config\\.cpp$"
    ".*test_gpu_feature_flags\\.cpp$"
    ".*test_gpu_kernel_validator\\.cpp$"

    ".*test_ai_orchestrator\\.cpp$"
    ".*test_acceleration\\.cpp$"
    ".*test_acceleration_dispatch\\.cpp$"
    ".*test_context_propagation\\.cpp$"
    ".*test_graph_query_optimizer\\.cpp$"
    ".*test_graph_distributed\\.cpp$"
    ".*test_llm_timeout_cancellation\\.cpp$"
    ".*test_lora_provenance\\.cpp$"
    ".*test_shader_integrity\\.cpp$"
    ".*test_two_phase_commit\\.cpp$"
    ".*test_vulkan_health\\.cpp$"
    ".*test_vulkan_metrics\\.cpp$"
    ".*test_vulkan_compute_equivalents\.cpp$"
    ".*test_serverless_function_api_handler\\.cpp$"
    ".*test_mcp_orchestrator_bridge\\.cpp$"
    ".*test_shared_worker_pool\\.cpp$"
)
foreach(pattern IN LISTS TEST_EXCLUDE_PATTERNS)
    list(FILTER ALL_TEST_SOURCES EXCLUDE REGEX "${pattern}")
endforeach()

list(LENGTH ALL_TEST_SOURCES TEST_SOURCE_COUNT)
message(STATUS "Building ${TEST_SOURCE_COUNT} test files after feature gating")

add_executable(themis_tests ${ALL_TEST_SOURCES})

# Enforce feature-gated exclusions on the resolved target source list as a
# defensive step (handles late target_sources() additions in this file).
get_target_property(_themis_tests_sources themis_tests SOURCES)
if(_themis_tests_sources)
    list(FILTER _themis_tests_sources EXCLUDE REGEX ".*[\\/]test_feature_flags\\.cpp$")
    list(FILTER _themis_tests_sources EXCLUDE REGEX ".*[\\/]test_vulkan_compute_equivalents\\.cpp$")
    if(NOT THEMIS_ENABLE_GPU)
        list(FILTER _themis_tests_sources EXCLUDE REGEX ".*[\\/]test_cpu_gpu_parity\\.cpp$")
    endif()
    set_property(TARGET themis_tests PROPERTY SOURCES ${_themis_tests_sources})
endif()

# Re-include prompt_engineering production-readiness tests that were excluded
# by the broad ".*test_prompt_.*\\.cpp$" and ".*test_meta_prompt.*\\.cpp$" patterns.
# These tests are fully functional and cover the validation / scaling / LLM /
# statistical and metrics improvements added in the production-readiness pass.
target_sources(themis_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/test_prompt_manager_validation.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_prompt_evaluator_ttest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_prompt_evaluator_embedding.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_prompt_engineering_metrics_persistence.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_feedback_collector_scaling.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_meta_prompt_llm_provider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_self_improvement_auto_optimize.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_prompt_version_control_diff.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_prompt_injection_detector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_prompt_manager_multimodal.cpp
)

# Re-include the ROCm/HIP backend test excluded by the broad ".*test_gpu_.*\\.cpp$"
# pattern.  This test exercises the ROCm backend CPU fallback path and is fully
# functional without AMD GPU hardware.
if(THEMIS_ENABLE_GPU)
    target_sources(themis_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_rocm_backend.cpp
    )
endif()

# Re-include the Vulkan compute backend test excluded by the broad
# ".*test_gpu_.*\\.cpp$" pattern (see line 267 above).  This test exercises
# the Vulkan backend CPU fallback path and is fully functional without
# Vulkan hardware.
if(THEMIS_ENABLE_GPU)
    target_sources(themis_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_vulkan_backend.cpp
    )
endif()

# Re-include test_multi_gpu_backend.cpp which tests the acceleration module's
# multi-GPU sharding backend (MultiGPUVectorBackend + ShardDescriptor).
# This test does NOT depend on LLM, but is caught by the broad LLM exclusion
# pattern ".*test_multi_gpu.*\\.cpp$" that targets test_multi_gpu_training.cpp,
# test_multi_gpu_lora*.cpp, etc. Re-adding it unconditionally ensures it always
# compiles and runs; it uses only CPU sub-backends so no GPU hardware is required.
target_sources(themis_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/test_multi_gpu_backend.cpp
)

# Re-include the GPU memory pool test excluded by the broad ".*test_gpu_.*\\.cpp$"
# pattern.  This test validates the slab-based pre-allocator and the
# defragmentation routine (GPUMemoryPool::defragment()).  It uses only the CPU
# bookkeeping simulation path and requires no GPU hardware.
if(THEMIS_ENABLE_GPU)
    target_sources(themis_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_memory_pool.cpp
    )
endif()

# Re-include the GPU metrics test excluded by the broad ".*test_gpu_.*\\.cpp$"
# pattern.  This test validates the in-process Prometheus-compatible counter/gauge
# registry (GPUMetrics) and the CUDA Nsight-compatible JSON export (nsight_export).
# It exercises only CPU-side bookkeeping and requires no GPU hardware.
if(THEMIS_ENABLE_GPU)
    target_sources(themis_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_metrics.cpp
    )
endif()

# Re-include the GPU profiler test excluded by the broad ".*test_gpu_.*\\.cpp$"
# pattern.  This test validates GPUProfiler (NVTX/rocTX range markers, Chrome trace
# JSON export for ROCm profiler) and GPUMetrics::rocm_profiler_export().
# It exercises only the CPU-side fallback path and requires no GPU hardware.
if(THEMIS_ENABLE_GPU)
    target_sources(themis_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_profiler.cpp
    )
endif()

# Re-include the GPU cluster topology test excluded by the broad
# ".*test_gpu_.*\\.cpp$" pattern.  This test validates GPUClusterTopology
# (NVLink / InfiniBand topology detection) and GPUClusterCoordinator
# (multi-node cluster coordination).  It uses only the CPU-side bookkeeping
# simulation path and requires no GPU hardware.
if(THEMIS_ENABLE_GPU)
    target_sources(themis_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_cluster_topology.cpp
    )
endif()

# Re-include the GPU stream manager test excluded by the two overlapping patterns
# ".*test_gpu_stream_manager\.cpp$" and ".*test_gpu_.*\.cpp$".
# This test validates GPUStreamManager stream lifecycle, work submission,
# statistics, CPU fallback budget, and CUDA stream creation/destruction.
# It uses only the CPU bookkeeping simulation path and requires no GPU hardware.
if(THEMIS_ENABLE_GPU)
    target_sources(themis_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_stream_manager.cpp
    )
endif()

# Re-include the GPU cluster coordinator test excluded by the broad
# ".*test_gpu_.*\\.cpp$" pattern.  This test validates multi-node GPU cluster
# coordination (node registration, heartbeat tracking, stale-node expiry, and
# least-loaded node selection).  It uses only CPU-side bookkeeping and requires
# no GPU hardware.
if(THEMIS_ENABLE_GPU)
    target_sources(themis_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_cluster_coordinator.cpp
    )
endif()

# Re-include the GPU graph cache test excluded by the broad ".*test_gpu_.*\\.cpp$"
# pattern.  This test validates GPUGraphCache (QueryShape hashing, capture/replay,
# LRU eviction) and the GPUQueryAccelerator graph-cache integration.  It uses only
# CPU-side bookkeeping and requires no GPU hardware.
if(THEMIS_ENABLE_GPU)
    target_sources(themis_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_graph_cache.cpp
    )
endif()

# Re-include the GPU unified memory test excluded by the broad ".*test_gpu_.*\\.cpp$"
# pattern.  This test validates GPUUnifiedMemoryAllocator (allocate, free, prefetch,
# advise, stats, tenant isolation, and concurrent safety).  It uses only CPU-side
# bookkeeping (malloc/free fallback) and requires no GPU hardware.
if(THEMIS_ENABLE_GPU)
    target_sources(themis_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_unified_memory.cpp
    )
endif()

# Re-include the GPU time-slice scheduler test excluded by the broad
# ".*test_gpu_.*\\.cpp$" pattern.  This test validates GPUTimeSliceScheduler
# (tenant lifecycle, work submission, round-robin dispatch, time-slice
# preemption, drainAll, and statistics).  It uses only CPU-side bookkeeping
# and requires no GPU hardware.
if(THEMIS_ENABLE_GPU)
    target_sources(themis_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_time_slice_scheduler.cpp
    )
endif()

# Re-include the WASM kernel sandbox test excluded by the broad
# ".*test_gpu_.*\\.cpp$" pattern.  This test validates WASMKernelSandbox
# (feature-gate enforcement, whitelist/checksum integration, memory-limit
# rejection, timeout enforcement, statistics, and concurrent safety).
# It uses only CPU-side bookkeeping and requires no GPU or WASM hardware.
if(THEMIS_ENABLE_GPU)
    target_sources(themis_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_wasm_kernel_sandbox.cpp
    )
endif()

# Re-include the MIG manager test excluded by the broad ".*test_gpu_.*\\.cpp$"
# pattern.  This test validates MIGManager (feature-gate enforcement, device
# capability detection, profile validation, partition lifecycle, tenant
# assignment, statistics, and concurrent safety).  It uses only CPU-side
# bookkeeping and requires no GPU or NVML hardware.
if(THEMIS_ENABLE_GPU)
    target_sources(themis_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_mig_manager.cpp
    )
endif()

# Re-include the P2P transfer test excluded by the broad ".*test_gpu_.*\\.cpp$"
# pattern.  This test validates GPUP2PTransferManager (feature-gate enforcement,
# peer access lifecycle, CPU fallback transfer, error handling, statistics, and
# concurrent safety).  It uses only CPU-side simulation and requires no GPU hardware.
if(THEMIS_ENABLE_GPU)
    target_sources(themis_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_p2p_transfer.cpp
    )
endif()

# Re-include LlmQueryRewriter tests when LLM is disabled.
# The ".*test_llm.*\.cpp$" exclusion pattern above would exclude
# test_llm_query_rewriter.cpp in MINIMAL builds, but that test has zero actual
# LLM library dependency — it only uses a std::function mock backend.
# Add it back unconditionally so it runs in every build variant.
if(NOT THEMIS_ENABLE_LLM)
    target_sources(themis_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/test_llm_query_rewriter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test_llm_reranker.cpp
        # LLMProcessAnalyzer has no actual LLM library dependency (uses a
        # simulated callLLM stub) — include in every build variant.
        ${CMAKE_CURRENT_SOURCE_DIR}/analytics/test_llm_process_analyzer.cpp
    )
endif()

# Ensure VisionConfig implementation is linked for vision API tests.
target_sources(themis_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/llm/vision_config.cpp
)

# Ensure legal training tests link even when the feature is disabled in themis_core
if(NOT THEMIS_ENABLE_LEGAL_TRAINING)
    target_sources(themis_tests PRIVATE
        ../src/ingestion/ingestion_manager.cpp
        ../src/ingestion/huggingface_connector.cpp
        ../src/ingestion/filesystem_ingester.cpp
        ../src/ingestion/api_connector.cpp
        ../src/ingestion/kafka_connector.cpp
        ../src/ingestion/object_storage_connector.cpp
        ../src/ingestion/database_connector.cpp
        ../src/training/auto_labeler.cpp
        ../src/training/knowledge_graph_enricher.cpp
        ../src/training/incremental_lora_trainer.cpp
        ../src/training/training_pipeline.cpp
        ../src/training/lora_data_selection.cpp
    )
endif()

# Voice module production components for testing (compile standalone, not gated by THEMIS_ENABLE_VOICE_ASSISTANT)
if(NOT THEMIS_ENABLE_VOICE_ASSISTANT)
    target_sources(themis_tests PRIVATE
        ../src/content/stt_processor.cpp
        ../src/content/tts_processor.cpp
        ../src/voice/audio_preprocessing.cpp
        ../src/voice/wake_word_detector.cpp
        ../src/voice/voice_intent_detector.cpp
        ../src/voice/voice_session_manager.cpp
        ../src/voice/voice_security.cpp
        ../src/voice/voice_error_handler.cpp
        ../src/voice/voice_tts_customizer.cpp
        ../src/voice/voice_meeting_support.cpp
        ../src/voice/voice_audio_storage.cpp
        ../src/voice/voice_accessibility.cpp
        ../src/voice/voice_model_cache.cpp
        ../src/voice/voice_batch_processor.cpp
        ../src/voice/voice_authenticator.cpp
    )
endif()

# CHIMERA adapter sources (no external library dependencies – run in simulation mode)
target_sources(themis_tests PRIVATE
    ../src/chimera/adapter_factory.cpp
    ../src/chimera/mongodb_adapter.cpp
    ../src/chimera/postgresql_adapter.cpp
    ../src/chimera/themisdb_adapter.cpp
    ../src/chimera/weaviate_adapter.cpp
    ../src/chimera/qdrant_adapter.cpp
    ../src/chimera/pinecone_adapter.cpp
    ../src/chimera/elasticsearch_adapter.cpp
)

set(_themis_test_core_libs)
if(THEMIS_BUILD_MODULAR)
    if(DEFINED THEMIS_CORE_SOURCES AND THEMIS_CORE_SOURCES)
        message(STATUS "THEMIS_BUILD_MODULAR=ON: building themis_tests with THEMIS_CORE_SOURCES for full symbol coverage")
        set(_themis_test_core_sources ${THEMIS_CORE_SOURCES})
        if(TARGET themis_sharding)
            list(FILTER _themis_test_core_sources EXCLUDE REGEX ".*/src/sharding/.*\\.cpp$")
            list(FILTER _themis_test_core_sources EXCLUDE REGEX ".*/src/server/rpc/blob_transfer_handler\\.cpp$")
            list(FILTER _themis_test_core_sources EXCLUDE REGEX ".*/src/utils/tracing\\.cpp$")
            message(STATUS "themis_tests: excluding direct sharding sources (provided by themis_sharding module)")
        endif()
        list(FILTER _themis_test_core_sources EXCLUDE REGEX ".*/src/server/http_server\\.cpp$")
        list(FILTER _themis_test_core_sources EXCLUDE REGEX ".*[\\\\/]src[\\\\/]server[\\\\/]http_server\\.cpp$")
        list(FILTER _themis_test_core_sources EXCLUDE REGEX ".*/src/security/stubs\\.cpp$")
        list(FILTER _themis_test_core_sources EXCLUDE REGEX ".*/src/stubs\\.cpp$")
        list(FILTER _themis_test_core_sources EXCLUDE REGEX ".*/src/security/pii_redaction_policy\\.cpp$")
        if(TARGET themis_llm)
            list(FILTER _themis_test_core_sources EXCLUDE REGEX ".*/src/llm/.*\\.cpp$")
            list(FILTER _themis_test_core_sources EXCLUDE REGEX ".*/src/llm/lora_framework/(gpu_tensor|gpu_lora_layers|flash_lora|paged_optimizer|paged_memory_manager|custom_allreduce|multi_gpu|multi_gpu_trainer|multi_gpu_lora_layer)\\.cpp$")
            message(STATUS "themis_tests: excluding direct LLM sources (provided by themis_llm module)")
        endif()
        target_sources(themis_tests PRIVATE ${_themis_test_core_sources})
        target_compile_definitions(themis_tests PRIVATE
            THEMIS_BASE_EXPORTS
            THEMIS_STORAGE_EXPORTS
            THEMIS_SECURITY_EXPORTS
            THEMIS_TRANSACTION_EXPORTS
            THEMIS_SHARDING_EXPORTS
            THEMIS_LLM_EXPORTS
            THEMIS_QUERY_EXPORTS
            THEMIS_TIMESERIES_EXPORTS
            THEMIS_GRAPH_EXPORTS
            THEMIS_GEO_EXPORTS
            THEMIS_CONTENT_EXPORTS
            THEMIS_NETWORK_EXPORTS
        )

        set(_themis_test_extra_sources
            ${THEMIS_ROOT_DIR}/src/llm/adapter_registry.cpp
            ${THEMIS_ROOT_DIR}/src/llm/ai_decision_auditor.cpp
            ${THEMIS_ROOT_DIR}/src/llm/explanation_generator.cpp
            ${THEMIS_ROOT_DIR}/src/llm/kernel_fusion.cpp
            ${THEMIS_ROOT_DIR}/src/llm/model_metadata_cache.cpp
            ${THEMIS_ROOT_DIR}/src/llm/lora_metadata_cache.cpp
            ${THEMIS_ROOT_DIR}/src/llm/production_validator.cpp
            ${THEMIS_ROOT_DIR}/src/llm/llm_model_audit_logger.cpp
            ${THEMIS_ROOT_DIR}/src/llm/applications/themis_help_lora.cpp
            ${THEMIS_ROOT_DIR}/src/llm/lora_framework/lora_feedback_storage.cpp
            ${THEMIS_ROOT_DIR}/src/llm/lora_framework/lora_training_config.cpp
            ${THEMIS_ROOT_DIR}/src/llm/lora_framework/lora_audit_logger.cpp
            ${THEMIS_ROOT_DIR}/src/llm/lora_framework/feedback_plugin.cpp
            ${THEMIS_ROOT_DIR}/src/llm/security/signature_verifier.cpp
            ${THEMIS_ROOT_DIR}/src/server/mvcc_api_handler.cpp
            ${THEMIS_ROOT_DIR}/src/server/geo_topology_api_handler.cpp
            ${THEMIS_ROOT_DIR}/src/server/replication_topology_api_handler.cpp
            ${THEMIS_ROOT_DIR}/src/server/serverless_function_api_handler.cpp
            ${THEMIS_ROOT_DIR}/src/server/async_job_api_handler.cpp
            ${THEMIS_ROOT_DIR}/src/server/distributed_txn_api_handler.cpp
            ${THEMIS_ROOT_DIR}/src/server/health_error_service.cpp
            ${THEMIS_ROOT_DIR}/src/server/http_type_adapter.cpp
            ${THEMIS_ROOT_DIR}/src/server/chunked_response_writer.cpp
            ${THEMIS_ROOT_DIR}/src/server/branch_api_handler.cpp
            ${THEMIS_ROOT_DIR}/src/server/merge_api_handler.cpp
            ${THEMIS_ROOT_DIR}/src/security/query_masking_policy.cpp
            ${THEMIS_ROOT_DIR}/src/graph/graph_query_optimizer.cpp
            ${THEMIS_ROOT_DIR}/src/graph/distributed_graph.cpp
            ${THEMIS_ROOT_DIR}/src/graph/gpu_traversal.cpp
            ${THEMIS_ROOT_DIR}/src/timeseries/ts_auto_buffer.cpp
            ${THEMIS_ROOT_DIR}/src/timeseries/tsstore.cpp
            ${THEMIS_ROOT_DIR}/src/timeseries/timeseries_metrics.cpp
            ${THEMIS_ROOT_DIR}/src/index/vector_auto_buffer.cpp
            ${THEMIS_ROOT_DIR}/src/cdc/cdc_admin.cpp
            ${THEMIS_ROOT_DIR}/src/storage/pitr_manager.cpp
            ${THEMIS_ROOT_DIR}/src/auth/auth_error.cpp
            ${THEMIS_ROOT_DIR}/src/auth/mtls_authenticator.cpp
            ${THEMIS_ROOT_DIR}/tests/test_hsm_provider_stub.cpp
            ${THEMIS_ROOT_DIR}/src/cdc/dead_letter_queue.cpp
            ${THEMIS_ROOT_DIR}/src/cdc/consumer_group.cpp
            ${THEMIS_ROOT_DIR}/src/cdc/cross_collection_stream.cpp
            ${THEMIS_ROOT_DIR}/src/cdc/cdc_materialized_view.cpp
            $<$<BOOL:${THEMIS_ENABLE_KAFKA}>:${THEMIS_ROOT_DIR}/src/cdc/kafka_cdc_producer.cpp>
            ${THEMIS_ROOT_DIR}/src/cdc/outbox.cpp
        )

        if(THEMIS_ENABLE_GPU AND NOT TARGET themis_llm)
            list(APPEND _themis_test_extra_sources
                ${THEMIS_ROOT_DIR}/src/llm/lora_framework/gpu_tensor.cpp
                ${THEMIS_ROOT_DIR}/src/llm/lora_framework/vram_allocator.cpp
                ${THEMIS_ROOT_DIR}/src/llm/lora_framework/gpu_lora_layers.cpp
                ${THEMIS_ROOT_DIR}/src/llm/lora_framework/paged_memory_manager.cpp
                ${THEMIS_ROOT_DIR}/src/llm/lora_framework/custom_allreduce.cpp
                ${THEMIS_ROOT_DIR}/src/llm/lora_framework/multi_gpu.cpp
                ${THEMIS_ROOT_DIR}/src/llm/lora_framework/multi_gpu_trainer.cpp
                ${THEMIS_ROOT_DIR}/src/llm/lora_framework/multi_gpu_lora_layer.cpp
                ${THEMIS_ROOT_DIR}/src/index/gpu_vector_index.cpp
                ${THEMIS_ROOT_DIR}/src/index/multi_gpu_vector_index.cpp
            )
        elseif(THEMIS_ENABLE_GPU)
            message(STATUS "themis_tests: skipping extra GPU sources (linked via themis_llm module)")
        endif()
        foreach(_extra_src IN LISTS _themis_test_extra_sources)
            if(EXISTS "${_extra_src}")
                list(FIND THEMIS_CORE_SOURCES "${_extra_src}" _extra_src_idx)
                if(_extra_src_idx EQUAL -1)
                    target_sources(themis_tests PRIVATE "${_extra_src}")
                endif()
            endif()
        endforeach()

        foreach(_mod
            themis_base
            themis_storage
            themis_security
            themis_transaction
            themis_sharding
            themis_llm
            themis_query
            themis_timeseries
            themis_graph
            themis_geo
            themis_content
        )
            if(TARGET ${_mod})
                list(APPEND _themis_test_core_libs ${_mod})
            endif()
        endforeach()
    else()
        foreach(_mod
            themis_base
            themis_storage
            themis_security
            themis_transaction
            themis_sharding
            themis_llm
            themis_query
            themis_timeseries
            themis_graph
            themis_geo
            themis_content
        )
            if(TARGET ${_mod})
                list(APPEND _themis_test_core_libs ${_mod})
            endif()
        endforeach()
    endif()
else()
    list(APPEND _themis_test_core_libs themis_core)
endif()

# Final defensive pruning after all themis_tests target_sources() additions.
get_target_property(_themis_tests_sources_final themis_tests SOURCES)
if(_themis_tests_sources_final)
    list(FILTER _themis_tests_sources_final EXCLUDE REGEX ".*[\\/]test_feature_flags\\.cpp$")
    list(FILTER _themis_tests_sources_final EXCLUDE REGEX ".*[\\/]test_vulkan_compute_equivalents\\.cpp$")
    if(NOT THEMIS_ENABLE_GPU)
        list(FILTER _themis_tests_sources_final EXCLUDE REGEX ".*[\\/]test_cpu_gpu_parity\\.cpp$")
    endif()
    set_property(TARGET themis_tests PROPERTY SOURCES ${_themis_tests_sources_final})
endif()

target_link_libraries(themis_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    ${_themis_test_core_libs}
    spdlog::spdlog
    fmt::fmt
    nlohmann_json::nlohmann_json
    yaml-cpp
    RocksDB::rocksdb
    OpenSSL::SSL
    OpenSSL::Crypto
    CURL::libcurl
    protobuf::libprotobuf
    Threads::Threads
)

if(TARGET prometheus-cpp::core)
    target_link_libraries(themis_tests PRIVATE prometheus-cpp::core)
endif()
if(TARGET prometheus-cpp::pull)
    target_link_libraries(themis_tests PRIVATE prometheus-cpp::pull)
endif()
if(TARGET libzip::zip)
    target_link_libraries(themis_tests PRIVATE libzip::zip)
elseif(TARGET libzip::libzip)
    target_link_libraries(themis_tests PRIVATE libzip::libzip)
endif()
if(TARGET pugixml::pugixml)
    target_link_libraries(themis_tests PRIVATE pugixml::pugixml)
elseif(TARGET pugixml::static)
    target_link_libraries(themis_tests PRIVATE pugixml::static)
endif()

if(WIN32)
    set(_themis_vcpkg_tbb_lib "${THEMIS_ROOT_DIR}/vcpkg_installed/x64-windows/lib/tbb12.lib")
    if(EXISTS "${_themis_vcpkg_tbb_lib}")
        target_link_libraries(themis_tests PRIVATE "${_themis_vcpkg_tbb_lib}")
    endif()

    set(_themis_vcpkg_pugixml_lib "${THEMIS_ROOT_DIR}/vcpkg_installed/x64-windows/lib/pugixml.lib")
    if(EXISTS "${_themis_vcpkg_pugixml_lib}")
        target_link_libraries(themis_tests PRIVATE "${_themis_vcpkg_pugixml_lib}")
    endif()
endif()

if(THEMIS_ENABLE_GRPC)
    target_link_libraries(themis_tests PRIVATE gRPC::grpc++)
endif()

if(TARGET llama)
    target_link_libraries(themis_tests PRIVATE llama)
endif()

target_compile_definitions(themis_tests PRIVATE
    THEMIS_TEST_BUILD=1
    THEMIS_TEST_CERT_DIR="${THEMIS_ROOT_DIR}/certs/test/wire_protocol"
)

# On Windows, disable whole program optimization (/GL) to match DLL ABI.
# Modular DLLs use /GL- for symbol export, so themis_tests must match.
if(MSVC)
    target_compile_options(themis_tests PRIVATE
        $<$<CONFIG:Release>:/GL->
        $<$<CONFIG:RelWithDebInfo>:/GL->
        $<$<CONFIG:MinSizeRel>:/GL->
    )
    target_link_options(themis_tests PRIVATE
        $<$<CONFIG:Release>:/LTCG:OFF>
        $<$<CONFIG:RelWithDebInfo>:/LTCG:OFF>
        $<$<CONFIG:MinSizeRel>:/LTCG:OFF>
    )
endif()

target_include_directories(themis_tests PRIVATE
    ${CMAKE_BINARY_DIR}/proto_generated
    ${THEMIS_ROOT_DIR}/vcpkg_installed/x64-windows/include
)

if(TARGET themis_shard_proto)
    add_dependencies(themis_tests themis_shard_proto)
endif()

# Wire Protocol V1 protobuf: add dependency so the generated .pb.h is ready
# before the test that includes wire_protocol_server.hpp compiles.
if(THEMIS_WIRE_V1_PROTO_AVAILABLE)
    target_compile_definitions(themis_tests PRIVATE THEMIS_WIRE_V1_PROTO_AVAILABLE)
endif()

# Copy DLLs to test directory on Windows after build
if(WIN32)
    # Copy DLLs from themis_core directory
    add_custom_command(TARGET themis_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Copying DLLs to test directory..."
    )
    
    # Copy llama.cpp and runtime DLLs if they exist
    if(EXISTS "${CMAKE_BINARY_DIR}/bin")
        add_custom_command(TARGET themis_tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_BINARY_DIR}/bin"
                "$<TARGET_FILE_DIR:themis_tests>"
            COMMENT "Copying llama.cpp DLLs to test directory"
        )
    endif()
    
    # Copy vcpkg runtime DLLs
    if(EXISTS "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/bin")
        file(GLOB VCPKG_RUNTIME_DLLS "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/bin/*.dll")
        if(VCPKG_RUNTIME_DLLS)
            add_custom_command(TARGET themis_tests POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy ${VCPKG_RUNTIME_DLLS} "$<TARGET_FILE_DIR:themis_tests>"
                COMMENT "Copying vcpkg runtime DLLs to test directory"
            )
        endif()
    endif()
endif()

add_test(NAME AllTests COMMAND themis_tests)
set_tests_properties(AllTests PROPERTIES
    LABELS "all;unified"
    TIMEOUT 600
)

# Session management and revocation endpoint tests (subset of AllTests)
add_test(NAME SessionManagerTests
    COMMAND themis_tests --gtest_filter=SessionManagerTest*:SessionApiHandlerTest*)
set_tests_properties(SessionManagerTests PROPERTIES
    LABELS "auth;session;revocation;unit"
    TIMEOUT 60
)
# Disabled: Missing implementation
# if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_multi_gpu_lora_advanced.cpp")
if(FALSE)
    message(STATUS "Adding Multi-GPU LoRA Advanced tests")
    
    add_executable(test_multi_gpu_lora_advanced
        test_multi_gpu_lora_advanced.cpp
    )
    
    target_link_libraries(test_multi_gpu_lora_advanced PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )
    
    target_compile_definitions(test_multi_gpu_lora_advanced PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME MultiGPULoRAAdvancedTests COMMAND test_multi_gpu_lora_advanced)
    
    set_tests_properties(MultiGPULoRAAdvancedTests PROPERTIES
        LABELS "lora;llm;multi-gpu;advanced;resource-aware;migration;security"
        TIMEOUT 180
    )
    
    message(STATUS "  Multi-GPU LoRA Advanced: resource-aware eviction, migration, audit logging")
else()
    message(STATUS "Multi-GPU LoRA Advanced tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_multi_gpu_lora_advanced.cpp")
endif()

# ============================================================================
# Distributed Coordinator Tests
# ============================================================================

# Disabled: Missing implementation
# if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_distributed_coordinator.cpp")
if(FALSE)
    message(STATUS "Adding Distributed Coordinator tests")
    
    add_executable(test_distributed_coordinator
        test_distributed_coordinator.cpp
    )
    
    target_link_libraries(test_distributed_coordinator PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )
    
    target_compile_definitions(test_distributed_coordinator PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME DistributedCoordinatorTests COMMAND test_distributed_coordinator)
    
    set_tests_properties(DistributedCoordinatorTests PROPERTIES
        LABELS "sharding;coordinator;leader-election;unit"
        TIMEOUT 120
    )
    
    message(STATUS "  Distributed Coordinator: ephemeral leader election, task scheduling")
else()
    message(STATUS "Distributed Coordinator tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_distributed_coordinator.cpp")
endif()

# Consensus Module Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_consensus_module.cpp")
    message(STATUS "Adding Consensus Module tests")
    
    add_executable(test_consensus_module
        test_consensus_module.cpp
    )
    
    target_link_libraries(test_consensus_module PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )
    
    target_compile_definitions(test_consensus_module PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME ConsensusModuleTests COMMAND test_consensus_module)
    
    set_tests_properties(ConsensusModuleTests PROPERTIES
        LABELS "sharding;consensus;raft;gossip;paxos;unit"
        TIMEOUT 120
    )
    
    message(STATUS "  Consensus Module: Raft, Gossip, Paxos implementations")
else()
    message(STATUS "Consensus Module tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_consensus_module.cpp")
endif()

# Raft Consensus Adapter Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_raft_consensus_adapter.cpp")
    message(STATUS "Adding Raft Consensus Adapter tests")
    
    add_executable(test_raft_consensus_adapter
        test_raft_consensus_adapter.cpp
    )
    
    target_link_libraries(test_raft_consensus_adapter PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )
    
    target_compile_definitions(test_raft_consensus_adapter PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME RaftConsensusAdapterTests COMMAND test_raft_consensus_adapter)
    
    set_tests_properties(RaftConsensusAdapterTests PROPERTIES
        LABELS "sharding;consensus;raft;adapter;unit"
        TIMEOUT 120
    )
    
    message(STATUS "  Raft Consensus Adapter: State conversion, log operations, membership changes")
else()
    message(STATUS "Raft Consensus Adapter tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_raft_consensus_adapter.cpp")
endif()

# Raft Configuration (joint consensus) Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_raft_configuration.cpp")
    message(STATUS "Adding Raft Configuration tests")

    add_executable(test_raft_configuration
        test_raft_configuration.cpp
        ../src/sharding/raft_configuration.cpp
    )

    target_include_directories(test_raft_configuration PRIVATE
        ${CMAKE_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
    )

    target_link_libraries(test_raft_configuration PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_raft_configuration PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME RaftConfigurationTests COMMAND test_raft_configuration)

    set_tests_properties(RaftConfigurationTests PROPERTIES
        LABELS "sharding;consensus;raft;membership;unit"
        TIMEOUT 60
    )

    message(STATUS "  Raft Configuration: joint consensus, membership changes, quorum")
else()
    message(STATUS "Raft Configuration tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_raft_configuration.cpp")
endif()

# ============================================================================
# Property Graph Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_property_graph.cpp")
    message(STATUS "Adding Property Graph tests")
    
    add_executable(test_property_graph
        test_property_graph.cpp
    )
    
    target_link_libraries(test_property_graph PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )
    
    target_compile_definitions(test_property_graph PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME PropertyGraphTests COMMAND test_property_graph)
    
    set_tests_properties(PropertyGraphTests PROPERTIES
        LABELS "index;property-graph;unit"
        TIMEOUT 120
    )
    
    message(STATUS "  Property Graph: edge cascade deletion, node/edge management")
else()
    message(STATUS "Property Graph tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_property_graph.cpp")
endif()

# ============================================================================
# Graph Query Optimizer Tests (standalone)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_graph_query_optimizer.cpp")
    message(STATUS "Adding Graph Query Optimizer tests")

    add_executable(test_graph_query_optimizer
        test_graph_query_optimizer.cpp
        ${THEMIS_ROOT_DIR}/src/graph/graph_query_optimizer.cpp
        ${THEMIS_ROOT_DIR}/src/graph/path_constraints.cpp
        ${THEMIS_ROOT_DIR}/src/graph/parallel_traversal.cpp
    )

    target_link_libraries(test_graph_query_optimizer PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        fmt::fmt
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )

    target_compile_definitions(test_graph_query_optimizer PRIVATE
        THEMIS_TEST_BUILD=1
        THEMIS_GRAPH_EXPORTS
    )

    add_test(NAME GraphQueryOptimizerTests COMMAND test_graph_query_optimizer)

    set_tests_properties(GraphQueryOptimizerTests PROPERTIES
        LABELS "graph;optimizer;query;unit"
        TIMEOUT 120
    )

    message(STATUS "  Graph Query Optimizer: BFS/DFS/Dijkstra/A*/Bidirectional, pattern matching, subgraph isomorphism")
else()
    message(STATUS "Graph Query Optimizer tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_graph_query_optimizer.cpp")
endif()

# ============================================================================
# Distributed Graph Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_graph_distributed.cpp")
    message(STATUS "Adding Distributed Graph tests")

    add_executable(test_graph_distributed
        test_graph_distributed.cpp
        ${THEMIS_ROOT_DIR}/src/graph/graph_query_optimizer.cpp
        ${THEMIS_ROOT_DIR}/src/graph/path_constraints.cpp
        ${THEMIS_ROOT_DIR}/src/graph/distributed_graph.cpp
        ${THEMIS_ROOT_DIR}/src/graph/parallel_traversal.cpp
    )

    target_link_libraries(test_graph_distributed PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        fmt::fmt
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )

    target_compile_definitions(test_graph_distributed PRIVATE
        THEMIS_TEST_BUILD=1
        THEMIS_GRAPH_EXPORTS
    )

    add_test(NAME DistributedGraphTests COMMAND test_graph_distributed)

    set_tests_properties(DistributedGraphTests PROPERTIES
        LABELS "graph;distributed;shards;unit"
        TIMEOUT 120
    )

    message(STATUS "  Distributed Graph: shard-aware query execution, BFS/Dijkstra fan-out, plan generation")
else()
    message(STATUS "Distributed Graph tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_graph_distributed.cpp")
endif()

# ============================================================================
# HTTP Client Pool Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_http_client_pool.cpp")
    message(STATUS "Adding HTTP Client Pool tests")
    
    add_executable(test_http_client_pool
        test_http_client_pool.cpp
    )
    
    target_link_libraries(test_http_client_pool PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )
    
    target_compile_definitions(test_http_client_pool PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME HTTPClientPoolTests COMMAND test_http_client_pool)
    
    set_tests_properties(HTTPClientPoolTests PROPERTIES
        LABELS "utils;http;connection-pool;concurrency;unit"
        TIMEOUT 60
    )
    
    message(STATUS "  HTTP Client Pool: striped locking, stale connection removal, timeouts")
else()
    message(STATUS "HTTP Client Pool tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_http_client_pool.cpp")
endif()

# ============================================================================
# gRPC Channel Pool Tests
# ============================================================================

if(THEMIS_ENABLE_GRPC AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_grpc_channel_pool.cpp")
    message(STATUS "Adding gRPC Channel Pool tests")
    
    add_executable(test_grpc_channel_pool
        test_grpc_channel_pool.cpp
    )
    
    target_link_libraries(test_grpc_channel_pool PRIVATE
        ${TEST_LIBS}
        themis_core
        gRPC::grpc++
        Threads::Threads
    )
    
    target_compile_definitions(test_grpc_channel_pool PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME GrpcChannelPoolTests COMMAND test_grpc_channel_pool)
    
    set_tests_properties(GrpcChannelPoolTests PROPERTIES
        LABELS "utils;grpc;connection-pool;concurrency;unit"
        TIMEOUT 60
    )
    
    message(STATUS "  gRPC Channel Pool: channel reuse, keepalive, stale pruning")
elseif(NOT THEMIS_ENABLE_GRPC)
    message(STATUS "gRPC Channel Pool tests skipped (gRPC disabled)")
else()
    message(STATUS "gRPC Channel Pool tests not found or gRPC disabled")
endif()

# ============================================================================
# gRPC API Server Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_api_grpc_server.cpp")
    message(STATUS "Adding gRPC API Server tests")

    add_executable(test_api_grpc_server
        test_api_grpc_server.cpp
        ${CMAKE_SOURCE_DIR}/src/api/grpc_server.cpp
        ${CMAKE_SOURCE_DIR}/src/utils/logger.cpp
    )

    target_link_libraries(test_api_grpc_server PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    if(THEMIS_ENABLE_GRPC)
        target_link_libraries(test_api_grpc_server PRIVATE gRPC::grpc++)
        target_compile_definitions(test_api_grpc_server PRIVATE THEMIS_ENABLE_GRPC)
    endif()

    target_compile_definitions(test_api_grpc_server PRIVATE
        THEMIS_TEST_BUILD=1
    )

    target_include_directories(test_api_grpc_server PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    add_test(NAME GrpcApiServerTests COMMAND test_api_grpc_server)

    set_tests_properties(GrpcApiServerTests PROPERTIES
        LABELS "api;grpc;server;lifecycle;unit"
        TIMEOUT 60
    )

    message(STATUS "  gRPC API Server: lifecycle, TLS config, service registration")
endif()

# ============================================================================
# ThemisDB gRPC Service wrapper Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_themisdb_grpc_service.cpp")
    message(STATUS "Adding ThemisDB gRPC Service wrapper tests")

    add_executable(test_themisdb_grpc_service
        test_themisdb_grpc_service.cpp
        ${CMAKE_SOURCE_DIR}/src/api/themisdb_grpc_service.cpp
        ${CMAKE_SOURCE_DIR}/src/utils/logger.cpp
    )

    target_link_libraries(test_themisdb_grpc_service PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    if(THEMIS_ENABLE_GRPC)
        target_link_libraries(test_themisdb_grpc_service PRIVATE gRPC::grpc++)
        target_compile_definitions(test_themisdb_grpc_service PRIVATE THEMIS_ENABLE_GRPC)
    endif()

    target_compile_definitions(test_themisdb_grpc_service PRIVATE
        THEMIS_TEST_BUILD=1
    )

    target_include_directories(test_themisdb_grpc_service PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    add_test(NAME ThemisDBGrpcServiceTests COMMAND test_themisdb_grpc_service)

    set_tests_properties(ThemisDBGrpcServiceTests PROPERTIES
        LABELS "api;grpc;service;unit"
        TIMEOUT 60
    )

    message(STATUS "  ThemisDB gRPC Service: construction, service() accessor")
endif()

# ============================================================================
# Batch Operation Manager Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_batch_operation_manager.cpp")
    message(STATUS "Adding Batch Operation Manager tests")
    
    add_executable(test_batch_operation_manager
        test_batch_operation_manager.cpp
    )
    
    target_link_libraries(test_batch_operation_manager PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )
    
    target_compile_definitions(test_batch_operation_manager PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME BatchOperationManagerTests COMMAND test_batch_operation_manager)
    
    set_tests_properties(BatchOperationManagerTests PROPERTIES
        LABELS "utils;batch;performance;unit"
        TIMEOUT 60
    )
    
    message(STATUS "  Batch Operation Manager: adaptive sizing, queue management")
else()
    message(STATUS "Batch Operation Manager tests not found")
endif()

# ============================================================================
# HNSW Parameter Tuner Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_hnsw_parameter_tuner.cpp")
    message(STATUS "Adding HNSW Parameter Tuner tests")
    
    add_executable(test_hnsw_parameter_tuner
        test_hnsw_parameter_tuner.cpp
    )
    
    target_link_libraries(test_hnsw_parameter_tuner PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )
    
    target_compile_definitions(test_hnsw_parameter_tuner PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME HnswParameterTunerTests COMMAND test_hnsw_parameter_tuner)
    
    set_tests_properties(HnswParameterTunerTests PROPERTIES
        LABELS "index;hnsw;performance;unit"
        TIMEOUT 60
    )
    
    message(STATUS "  HNSW Parameter Tuner: adaptive efSearch, memory optimization")
else()
    message(STATUS "HNSW Parameter Tuner tests not found")
endif()

# ============================================================================
# HNSW Incremental Re-index Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_hnsw_incremental_reindex.cpp")
    message(STATUS "Adding HNSW Incremental Re-index tests")

    add_executable(test_hnsw_incremental_reindex
        test_hnsw_incremental_reindex.cpp
    )

    target_link_libraries(test_hnsw_incremental_reindex PRIVATE
        ${TEST_LIBS}
        themis_core
        RocksDB::rocksdb
    )

    target_compile_definitions(test_hnsw_incremental_reindex PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME HnswIncrementalReindexTests COMMAND test_hnsw_incremental_reindex)

    set_tests_properties(HnswIncrementalReindexTests PROPERTIES
        LABELS "index;hnsw;incremental;unit"
        TIMEOUT 120
    )

    message(STATUS "  HNSW Incremental Re-index: add/remove/update/unchanged, idempotency, full-rebuild trigger")
else()
    message(STATUS "HNSW Incremental Re-index tests not found")
endif()



if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_enhanced_query_cache.cpp")
    message(STATUS "Adding Enhanced Query Cache tests")
    
    add_executable(test_enhanced_query_cache
        test_enhanced_query_cache.cpp
    )
    
    target_link_libraries(test_enhanced_query_cache PRIVATE
        ${TEST_LIBS}
        themis_core
        TBB::tbb
        Threads::Threads
    )
    
    target_compile_definitions(test_enhanced_query_cache PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME EnhancedQueryCacheTests COMMAND test_enhanced_query_cache)
    
    set_tests_properties(EnhancedQueryCacheTests PROPERTIES
        LABELS "cache;performance;concurrency;unit"
        TIMEOUT 60
    )
    
    message(STATUS "  Enhanced Query Cache: TBB concurrent hash map, TTL, LRU eviction")
else()
    message(STATUS "Enhanced Query Cache tests not found")
endif()

# ============================================================================
# Cache Admin API Handler Tests (Phase 3 – health endpoint, stats, eviction)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_cache_admin_api_handler.cpp")
    message(STATUS "Adding Cache Admin API Handler tests")

    add_executable(test_cache_admin_api_handler
        test_cache_admin_api_handler.cpp
    )

    target_link_libraries(test_cache_admin_api_handler PRIVATE
        ${TEST_LIBS}
        themis_core
        RocksDB::rocksdb
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_cache_admin_api_handler PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME CacheAdminApiHandlerTests COMMAND test_cache_admin_api_handler)

    set_tests_properties(CacheAdminApiHandlerTests PROPERTIES
        LABELS "cache;admin;health;circuit-breaker;unit"
        TIMEOUT 60
    )

    message(STATUS "  Cache Admin API Handler: health endpoint, per-tier status, circuit breaker state")
else()
    message(STATUS "Cache Admin API Handler tests not found")
endif()

# ============================================================================
# Cache Warmup Tests (Phase 3 – bulk warmup from query log / snapshot)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_cache_warmup.cpp")
    message(STATUS "Adding Cache Warmup tests")

    add_executable(test_cache_warmup
        test_cache_warmup.cpp
    )

    target_link_libraries(test_cache_warmup PRIVATE
        ${TEST_LIBS}
        themis_core
        RocksDB::rocksdb
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_cache_warmup PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME CacheWarmupTests COMMAND test_cache_warmup)

    set_tests_properties(CacheWarmupTests PROPERTIES
        LABELS "cache;warmup;snapshot;unit"
        TIMEOUT 60
    )

    message(STATUS "  Cache Warmup: warmupFromLog, exportSnapshot, tenant isolation, metrics")
else()
    message(STATUS "Cache Warmup tests not found")
endif()

# ============================================================================
# Cache Replication Tests (Phase 4 – HA cache replication)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_cache_replication.cpp")
    message(STATUS "Adding Cache Replication tests")

    add_executable(test_cache_replication
        test_cache_replication.cpp
        ${CMAKE_SOURCE_DIR}/src/cache/cache_replication_coordinator.cpp
        ${CMAKE_SOURCE_DIR}/src/utils/logger.cpp
    )

    target_link_libraries(test_cache_replication PRIVATE
        ${TEST_LIBS}
        themis_core
        RocksDB::rocksdb
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_cache_replication PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME CacheReplicationTests COMMAND test_cache_replication)

    set_tests_properties(CacheReplicationTests PROPERTIES
        LABELS "cache;replication;ha;unit"
        TIMEOUT 60
    )

    message(STATUS "  Cache Replication: listener callbacks, snapshot, graceful degradation, replica health")
else()
    message(STATUS "Cache Replication tests not found")
endif()

# ============================================================================
# Distributed Cache Coordinator Tests (Phase 4 – Redis-compatible protocol)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_distributed_cache_coordinator.cpp")
    message(STATUS "Adding Distributed Cache Coordinator tests")

    add_executable(test_distributed_cache_coordinator
        test_distributed_cache_coordinator.cpp
    )

    target_link_libraries(test_distributed_cache_coordinator PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_distributed_cache_coordinator PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME DistributedCacheCoordinatorTests COMMAND test_distributed_cache_coordinator)

    set_tests_properties(DistributedCacheCoordinatorTests PROPERTIES
        LABELS "cache;distributed;coordinator;redis;unit"
        TIMEOUT 60
    )

    message(STATUS "  Distributed Cache Coordinator: RedisCacheCoordinator, graceful degradation, pub/sub, stats")
else()
    message(STATUS "Distributed Cache Coordinator tests not found")
endif()

# ============================================================================
# Multi-Region Active-Active Tests (Phase 4 – bounded staleness)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_multi_region_active_active.cpp")
    message(STATUS "Adding Multi-Region Active-Active tests")

    add_executable(test_multi_region_active_active
        test_multi_region_active_active.cpp
    )

    target_link_libraries(test_multi_region_active_active PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_multi_region_active_active PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME MultiRegionActiveActiveTests COMMAND test_multi_region_active_active)

    set_tests_properties(MultiRegionActiveActiveTests PROPERTIES
        LABELS "replication;multi-region;active-active;bounded-staleness;unit"
        TIMEOUT 60
    )

    message(STATUS "  Multi-Region Active-Active: bounded staleness, session tokens, consistency levels, metrics")
else()
    message(STATUS "Multi-Region Active-Active tests not found")
endif()

# RPC Geospatial Query Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_rpc_geo_query.cpp")
    message(STATUS "Adding RPC Geospatial Query tests")
    
    add_executable(test_rpc_geo_query
        test_rpc_geo_query.cpp
        ${CMAKE_SOURCE_DIR}/src/server/rpc/rpc_service_impl.cpp
    )
    
    target_link_libraries(test_rpc_geo_query PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )
    
    target_compile_definitions(test_rpc_geo_query PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME RPCGeoQueryTests COMMAND test_rpc_geo_query)
    
    set_tests_properties(RPCGeoQueryTests PROPERTIES
        LABELS "rpc;geospatial;query;unit"
        TIMEOUT 60
    )
    
    message(STATUS "  RPC Geo Query: intersects, within, near spatial queries")
else()
    message(STATUS "RPC Geo Query tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_rpc_geo_query.cpp")
endif()

# ============================================================================
# Index Maintenance Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_index_maintenance.cpp")
    message(STATUS "Adding Index Maintenance tests")
    
    add_executable(test_index_maintenance
        test_index_maintenance.cpp
    )
    
    target_link_libraries(test_index_maintenance PRIVATE
        ${TEST_LIBS}
        themis_core
        RocksDB::rocksdb
    )
    
    target_compile_definitions(test_index_maintenance PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME IndexMaintenanceTests COMMAND test_index_maintenance)
    
    set_tests_properties(IndexMaintenanceTests PROPERTIES
        LABELS "storage;index;maintenance;unit"
        TIMEOUT 180
    )
    
    message(STATUS "  Index Maintenance: fragmentation monitoring, rebuild, reorganization, statistics")
else()
    message(STATUS "Index Maintenance tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_index_maintenance.cpp")
endif()

# GraphQL Parser Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_graphql.cpp")
    message(STATUS "Adding GraphQL parser tests")
    
    add_executable(test_graphql
        test_graphql.cpp
    )
    
    target_link_libraries(test_graphql PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )
    
    target_compile_definitions(test_graphql PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME GraphQLParserTests COMMAND test_graphql)
    
    set_tests_properties(GraphQLParserTests PROPERTIES
        LABELS "query;graphql;parser;unit"
        TIMEOUT 60
    )
    
    message(STATUS "  GraphQL Parser: query parsing, arguments, validation")
else()
    message(STATUS "GraphQL Parser tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_graphql.cpp")
endif()

# ============================================================================
# GraphQL Performance Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_graphql_performance.cpp")
    message(STATUS "Adding GraphQL performance tests")

    add_executable(test_graphql_performance
        test_graphql_performance.cpp
    )

    target_link_libraries(test_graphql_performance PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )

    target_compile_definitions(test_graphql_performance PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME GraphQLPerformanceTests COMMAND test_graphql_performance)

    set_tests_properties(GraphQLPerformanceTests PROPERTIES
        LABELS "query;graphql;performance;cache;metrics;unit"
        TIMEOUT 60
    )

    message(STATUS "  GraphQL Performance: parse cache hits, O(1) LRU eviction, executor metrics")
else()
    message(STATUS "GraphQL Performance tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_graphql_performance.cpp")
endif()

# ============================================================================
# GraphQL Limits Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_graphql_limits.cpp")
    message(STATUS "Adding GraphQL limits tests")

    add_executable(test_graphql_limits
        test_graphql_limits.cpp
    )

    target_link_libraries(test_graphql_limits PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )

    target_compile_definitions(test_graphql_limits PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME GraphQLLimitsTests COMMAND test_graphql_limits)

    set_tests_properties(GraphQLLimitsTests PROPERTIES
        LABELS "query;graphql;limits;unit"
        TIMEOUT 60
    )

    message(STATUS "  GraphQL Limits: query size, depth, field count limits")
else()
    message(STATUS "GraphQL Limits tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_graphql_limits.cpp")
endif()

# ============================================================================
# GraphQL Error Masking Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_graphql_error_masking.cpp")
    message(STATUS "Adding GraphQL error masking tests")

    add_executable(test_graphql_error_masking
        test_graphql_error_masking.cpp
    )

    target_link_libraries(test_graphql_error_masking PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )

    target_compile_definitions(test_graphql_error_masking PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME GraphQLErrorMaskingTests COMMAND test_graphql_error_masking)

    set_tests_properties(GraphQLErrorMaskingTests PROPERTIES
        LABELS "query;graphql;error;security;unit"
        TIMEOUT 60
    )

    message(STATUS "  GraphQL Error Masking: production error masking, development mode")
else()
    message(STATUS "GraphQL Error Masking tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_graphql_error_masking.cpp")
endif()

# ============================================================================
# GraphQL Phase 1 Feature Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_graphql_p1_features.cpp")
    message(STATUS "Adding GraphQL Phase 1 feature tests")

    add_executable(test_graphql_p1_features
        test_graphql_p1_features.cpp
    )

    target_link_libraries(test_graphql_p1_features PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )

    target_compile_definitions(test_graphql_p1_features PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME GraphQLP1FeatureTests COMMAND test_graphql_p1_features)

    set_tests_properties(GraphQLP1FeatureTests PROPERTIES
        LABELS "query;graphql;schema;geo;unit"
        TIMEOUT 60
    )

    message(STATUS "  GraphQL P1 Features: geo scalars, document/graph/vector types, query/mutation")
else()
    message(STATUS "GraphQL P1 Feature tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_graphql_p1_features.cpp")
endif()

# ============================================================================
# GraphQL Multi-Model Query Schema Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_graphql_multimodel.cpp")
    message(STATUS "Adding GraphQL multi-model query schema tests")

    add_executable(test_graphql_multimodel
        test_graphql_multimodel.cpp
    )

    target_link_libraries(test_graphql_multimodel PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )

    target_compile_definitions(test_graphql_multimodel PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME GraphQLMultiModelTests COMMAND test_graphql_multimodel)

    set_tests_properties(GraphQLMultiModelTests PROPERTIES
        LABELS "query;graphql;schema;multimodel;timeseries;geo;subscription;unit"
        TIMEOUT 60
    )

    message(STATUS "  GraphQL Multi-Model: timeseries queries, geo search, subscription type")
else()
    message(STATUS "GraphQL Multi-Model tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_graphql_multimodel.cpp")
endif()

# ============================================================================
# GraphQL Cache Security Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_graphql_cache_security.cpp")
    message(STATUS "Adding GraphQL cache security tests")

    add_executable(test_graphql_cache_security
        test_graphql_cache_security.cpp
    )

    target_link_libraries(test_graphql_cache_security PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )

    target_compile_definitions(test_graphql_cache_security PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME GraphQLCacheSecurityTests COMMAND test_graphql_cache_security)

    set_tests_properties(GraphQLCacheSecurityTests PROPERTIES
        LABELS "query;graphql;cache;security;unit"
        TIMEOUT 60
    )

    message(STATUS "  GraphQL Cache Security: LRU cache, TTL expiration, XSS encoding, injection prevention")
else()
    message(STATUS "GraphQL Cache Security tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_graphql_cache_security.cpp")
endif()

# ============================================================================
# Serverless Function API Handler Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_serverless_function_api_handler.cpp")
    message(STATUS "Adding Serverless Function API Handler tests")

    add_executable(test_serverless_function_api_handler
        test_serverless_function_api_handler.cpp
        ${THEMIS_ROOT_DIR}/src/server/serverless_function_api_handler.cpp
    )

    target_include_directories(test_serverless_function_api_handler PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_serverless_function_api_handler PRIVATE
        ${TEST_LIBS}
        Threads::Threads
    )

    target_compile_definitions(test_serverless_function_api_handler PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ServerlessFunctionApiHandlerTests
             COMMAND test_serverless_function_api_handler)

    set_tests_properties(ServerlessFunctionApiHandlerTests PROPERTIES
        LABELS "server;serverless;functions;unit"
        TIMEOUT 60
    )

    message(STATUS "  Serverless Function Handler: CRUD, invocation DSL, version history")
else()
    message(STATUS "Serverless Function API Handler tests not found")
endif()

# ============================================================================
# UDF API Handler Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_udf_api_handler.cpp")
    message(STATUS "Adding UDF API Handler tests")

    add_executable(test_udf_api_handler
        test_udf_api_handler.cpp
        ${THEMIS_ROOT_DIR}/src/server/udf_api_handler.cpp
        ${THEMIS_ROOT_DIR}/src/query/functions/udf_registry.cpp
    )

    target_include_directories(test_udf_api_handler PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_udf_api_handler PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_udf_api_handler PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME UdfApiHandlerTests
             COMMAND test_udf_api_handler)

    set_tests_properties(UdfApiHandlerTests PROPERTIES
        LABELS "server;query;udf;unit"
        TIMEOUT 60
    )

    message(STATUS "  UDF API Handler: register, list, get, delete, expression DSL")
else()
    message(STATUS "UDF API Handler tests not found")
endif()



if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_inverted_index.cpp")
    message(STATUS "Adding InvertedIndex unit tests")

    add_executable(test_inverted_index
        test_inverted_index.cpp
        ${CMAKE_SOURCE_DIR}/src/index/inverted_index.cpp
        ${CMAKE_SOURCE_DIR}/src/query/functions/fulltext_functions.cpp
        ${CMAKE_SOURCE_DIR}/src/utils/logger.cpp
    )

    target_link_libraries(test_inverted_index PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )

    target_compile_definitions(test_inverted_index PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME InvertedIndexTests COMMAND test_inverted_index)

    set_tests_properties(InvertedIndexTests PROPERTIES
        LABELS "index;fulltext;inverted;unit"
        TIMEOUT 120
    )

    message(STATUS "  InvertedIndex: create/drop, index/deindex, BM25 search, phrase, fuzzy")
else()
    message(STATUS "InvertedIndex tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_inverted_index.cpp")
endif()

# ============================================================================
# Fulltext Phrase and Fuzzy Search Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_fulltext_phrase_fuzzy.cpp")
    message(STATUS "Adding Fulltext Phrase and Fuzzy Search tests")
    
    add_executable(test_fulltext_phrase_fuzzy
        test_fulltext_phrase_fuzzy.cpp
    )
    
    target_link_libraries(test_fulltext_phrase_fuzzy PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )
    
    target_compile_definitions(test_fulltext_phrase_fuzzy PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME FulltextPhraseFuzzyTests COMMAND test_fulltext_phrase_fuzzy)
    
    set_tests_properties(FulltextPhraseFuzzyTests PROPERTIES
        LABELS "index;fulltext;phrase;fuzzy;unit"
        TIMEOUT 120
    )
    
    message(STATUS "  Fulltext: phrase matching, fuzzy search with Levenshtein distance")
else()
    message(STATUS "Fulltext Phrase and Fuzzy Search tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_fulltext_phrase_fuzzy.cpp")
endif()

# ============================================================================
# Comprehensive Real Unit Tests (PR: Implement Real Unit Tests)
# ============================================================================

# RocksDBWrapper Comprehensive Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_rocksdb_wrapper_comprehensive.cpp")
    message(STATUS "Adding RocksDBWrapper comprehensive tests")
    
    add_executable(test_rocksdb_wrapper_comprehensive
        test_rocksdb_wrapper_comprehensive.cpp
    )
    
    target_link_libraries(test_rocksdb_wrapper_comprehensive PRIVATE
        ${TEST_LIBS}
        themis_core
        RocksDB::rocksdb
        spdlog::spdlog
        Threads::Threads
    )
    
    target_compile_definitions(test_rocksdb_wrapper_comprehensive PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME RocksDBWrapperComprehensiveTests COMMAND test_rocksdb_wrapper_comprehensive)
    
    set_tests_properties(RocksDBWrapperComprehensiveTests PROPERTIES
        LABELS "storage;rocksdb;comprehensive;unit"
        TIMEOUT 300
    )
    
    message(STATUS "  RocksDBWrapper Comprehensive: CRUD, transactions, MVCC, backup/restore, iterators, column families")
else()
    message(STATUS "RocksDBWrapper comprehensive tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_rocksdb_wrapper_comprehensive.cpp")
endif()

# PITR Manager Comprehensive Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_pitr_manager_comprehensive.cpp")
    message(STATUS "Adding PITR Manager comprehensive tests")
    
    add_executable(test_pitr_manager_comprehensive
        test_pitr_manager_comprehensive.cpp
    )
    
    target_link_libraries(test_pitr_manager_comprehensive PRIVATE
        ${TEST_LIBS}
        themis_core
        RocksDB::rocksdb
        spdlog::spdlog
        Threads::Threads
    )
    
    target_compile_definitions(test_pitr_manager_comprehensive PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME PITRManagerComprehensiveTests COMMAND test_pitr_manager_comprehensive)
    
    set_tests_properties(PITRManagerComprehensiveTests PROPERTIES
        LABELS "storage;pitr;recovery;comprehensive;unit"
        TIMEOUT 300
    )
    
    message(STATUS "  PITR Manager Comprehensive: point-in-time recovery, changefeed replay, snapshots, consistency")
else()
    message(STATUS "PITR Manager comprehensive tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_pitr_manager_comprehensive.cpp")
endif()

# Vector Index Comprehensive Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_vector_index_comprehensive.cpp")
    message(STATUS "Adding Vector Index comprehensive tests")
    
    add_executable(test_vector_index_comprehensive
        test_vector_index_comprehensive.cpp
    )
    
    target_link_libraries(test_vector_index_comprehensive PRIVATE
        ${TEST_LIBS}
        themis_core
        RocksDB::rocksdb
        spdlog::spdlog
        Threads::Threads
    )
    
    target_compile_definitions(test_vector_index_comprehensive PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME VectorIndexComprehensiveTests COMMAND test_vector_index_comprehensive)
    
    set_tests_properties(VectorIndexComprehensiveTests PROPERTIES
        LABELS "index;vector;hnsw;comprehensive;unit"
        TIMEOUT 300
    )
    
    message(STATUS "  Vector Index Comprehensive: HNSW, distance metrics, filtered search, persistence, concurrency")
else()
    message(STATUS "Vector Index comprehensive tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_vector_index_comprehensive.cpp")
endif()

# Lossless Vector Compression Tests (sparse encoding for index compression)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_vector_compression_lossless.cpp")
    message(STATUS "Adding Lossless Vector Compression tests")

    add_executable(test_vector_compression_lossless
        test_vector_compression_lossless.cpp
    )

    target_link_libraries(test_vector_compression_lossless PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_vector_compression_lossless PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME VectorCompressionLosslessTests COMMAND test_vector_compression_lossless)

    set_tests_properties(VectorCompressionLosslessTests PROPERTIES
        LABELS "index;compression;sparse;lossless;unit"
        TIMEOUT 120
    )

    message(STATUS "  Lossless Vector Compression: sparse CSR, delta+varint, dictionary encoding")
else()
    message(STATUS "Lossless vector compression tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_vector_compression_lossless.cpp")
endif()

# Transaction Savepoint Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_savepoints.cpp")
    message(STATUS "Adding Transaction Savepoint tests")

    add_executable(test_savepoints
        test_savepoints.cpp
    )

    target_link_libraries(test_savepoints PRIVATE
        ${TEST_LIBS}
        themis_core
        RocksDB::rocksdb
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_savepoints PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME TransactionSavepointTests COMMAND test_savepoints)

    set_tests_properties(TransactionSavepointTests PROPERTIES
        LABELS "transaction;savepoint;acid;unit"
        TIMEOUT 120
    )

    message(STATUS "  Transaction Savepoints: named partial rollback, SAGA trimming, error handling")
else()
    message(STATUS "Transaction Savepoint tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_savepoints.cpp")
endif()

# Transaction Manager Comprehensive Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_transaction_manager_comprehensive.cpp")
    message(STATUS "Adding Transaction Manager comprehensive tests")
    
    add_executable(test_transaction_manager_comprehensive
        test_transaction_manager_comprehensive.cpp
    )
    
    target_link_libraries(test_transaction_manager_comprehensive PRIVATE
        ${TEST_LIBS}
        themis_core
        RocksDB::rocksdb
        spdlog::spdlog
        Threads::Threads
    )
    
    target_compile_definitions(test_transaction_manager_comprehensive PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME TransactionManagerComprehensiveTests COMMAND test_transaction_manager_comprehensive)
    
    set_tests_properties(TransactionManagerComprehensiveTests PROPERTIES
        LABELS "transaction;mvcc;acid;distributed;comprehensive;unit"
        TIMEOUT 300
    )
    
    message(STATUS "  Transaction Manager Comprehensive: ACID, isolation levels, deadlock detection, 2PC, savepoints")
else()
    message(STATUS "Transaction Manager comprehensive tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_transaction_manager_comprehensive.cpp")
endif()

# Transaction OCC Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_transaction_occ.cpp")
    message(STATUS "Adding Transaction OCC tests")

    add_executable(test_transaction_occ
        test_transaction_occ.cpp
    )

    target_link_libraries(test_transaction_occ PRIVATE
        ${TEST_LIBS}
        themis_core
        RocksDB::rocksdb
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_transaction_occ PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME TransactionOccTests COMMAND test_transaction_occ)

    set_tests_properties(TransactionOccTests PROPERTIES
        LABELS "transaction;occ;optimistic;unit"
        TIMEOUT 120
    )

    message(STATUS "  Transaction OCC: getEntityVersion, optimisticPut, optimisticErase, retry pattern")
else()
    message(STATUS "Transaction OCC tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_transaction_occ.cpp")
endif()

# Utilities Comprehensive Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_utilities_comprehensive.cpp")
    message(STATUS "Adding Utilities comprehensive tests")
    
    add_executable(test_utilities_comprehensive
        test_utilities_comprehensive.cpp
    )
    
    target_link_libraries(test_utilities_comprehensive PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
    )
    
    target_compile_definitions(test_utilities_comprehensive PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME UtilitiesComprehensiveTests COMMAND test_utilities_comprehensive)
    
    set_tests_properties(UtilitiesComprehensiveTests PROPERTIES
        LABELS "utilities;error;pii;stemming;olap;comprehensive;unit"
        TIMEOUT 180
    )
    
    message(STATUS "  Utilities Comprehensive: error handling, PII detection, stemming, OLAP engine")
else()
    message(STATUS "Utilities comprehensive tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_utilities_comprehensive.cpp")
endif()

# ============================================================================
# Utils Production Readiness Tests (Phases 1–8)
# ============================================================================

# Phase 1 – Logger production tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_logger_production.cpp")
    message(STATUS "Adding Logger production tests (Phase 1)")
    add_executable(test_logger_production
        test_logger_production.cpp
        ${CMAKE_SOURCE_DIR}/src/utils/logger.cpp
    )
    target_link_libraries(test_logger_production PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(test_logger_production PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME LoggerProductionTests COMMAND test_logger_production)
    set_tests_properties(LoggerProductionTests PROPERTIES
        LABELS "utils;logging;production;phase1;unit"
        TIMEOUT 60
    )
    message(STATUS "  Logger Production: JSON logging, rotation, PII redaction, metrics")
endif()

# Phase 2 – Tracing production tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_tracing_production.cpp")
    message(STATUS "Adding Tracing production tests (Phase 2)")
    add_executable(test_tracing_production test_tracing_production.cpp)
    target_link_libraries(test_tracing_production PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )
    target_compile_definitions(test_tracing_production PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME TracingProductionTests COMMAND test_tracing_production)
    set_tests_properties(TracingProductionTests PROPERTIES
        LABELS "utils;tracing;baggage;sampling;phase2;unit"
        TIMEOUT 60
    )
    message(STATUS "  Tracing Production: Baggage, SamplingStrategy, W3C propagation")
endif()

# Phase 2 – OpenTelemetry tracer adapter unit tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_otel_tracer_adapter.cpp")
    message(STATUS "Adding OtelTracerAdapter unit tests (Phase 2)")
    add_executable(test_otel_tracer_adapter test_otel_tracer_adapter.cpp)
    target_link_libraries(test_otel_tracer_adapter PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )
    target_compile_definitions(test_otel_tracer_adapter PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME OtelTracerAdapterTests COMMAND test_otel_tracer_adapter)
    set_tests_properties(OtelTracerAdapterTests PROPERTIES
        LABELS "core;tracing;otel;circuit-breaker;phase2;unit"
        TIMEOUT 60
    )
    message(STATUS "  OtelTracerAdapter: adapter construction, span lifecycle, circuit-breaker, health")
endif()

# Phase 2 – Jaeger tracing backend adapter unit tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_jaeger_tracer_adapter.cpp")
    message(STATUS "Adding JaegerTracerAdapter unit tests (Phase 2)")
    add_executable(test_jaeger_tracer_adapter test_jaeger_tracer_adapter.cpp)
    target_link_libraries(test_jaeger_tracer_adapter PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )
    target_compile_definitions(test_jaeger_tracer_adapter PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME JaegerTracerAdapterTests COMMAND test_jaeger_tracer_adapter)
    set_tests_properties(JaegerTracerAdapterTests PROPERTIES
        LABELS "core;tracing;jaeger;circuit-breaker;phase2;unit"
        TIMEOUT 60
    )
    message(STATUS "  JaegerTracerAdapter: adapter construction, span lifecycle, uber-trace-id propagation, circuit-breaker, health")
endif()

# Phase 2 – Zipkin tracing backend adapter unit tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_zipkin_tracer_adapter.cpp")
    message(STATUS "Adding ZipkinTracerAdapter unit tests (Phase 2)")
    add_executable(test_zipkin_tracer_adapter test_zipkin_tracer_adapter.cpp)
    target_link_libraries(test_zipkin_tracer_adapter PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )
    target_compile_definitions(test_zipkin_tracer_adapter PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME ZipkinTracerAdapterTests COMMAND test_zipkin_tracer_adapter)
    set_tests_properties(ZipkinTracerAdapterTests PROPERTIES
        LABELS "core;tracing;zipkin;b3;circuit-breaker;phase2;unit"
        TIMEOUT 60
    )
    message(STATUS "  ZipkinTracerAdapter: adapter construction, span lifecycle, B3 propagation, circuit-breaker, health")
endif()

# Phase 2 – OpenTelemetry trace and span propagation tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_otel_propagation.cpp")
    message(STATUS "Adding OTel propagation tests (Phase 2)")
    add_executable(test_otel_propagation test_otel_propagation.cpp)
    target_link_libraries(test_otel_propagation PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )
    target_compile_definitions(test_otel_propagation PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME OtelPropagationTests COMMAND test_otel_propagation)
    set_tests_properties(OtelPropagationTests PROPERTIES
        LABELS "core;tracing;otel;propagation;w3c;phase2;unit"
        TIMEOUT 60
    )
    message(STATUS "  OtelPropagation: startSpanFromHeaders, injectContext, baggage round-trip")
endif()
    
# Phase 3 – Structured log correlation (trace ID injection into structured logs)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_structured_log_correlation.cpp")
    message(STATUS "Adding Structured Log Correlation tests (Phase 3)")
    add_executable(test_structured_log_correlation test_structured_log_correlation.cpp)
    target_link_libraries(test_structured_log_correlation PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(test_structured_log_correlation PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME StructuredLogCorrelationTests COMMAND test_structured_log_correlation)
    set_tests_properties(StructuredLogCorrelationTests PROPERTIES
        LABELS "core;logging;tracing;trace-id;structured-log;phase3;unit"
        TIMEOUT 60
    )
    message(STATUS "  Structured Log Correlation: TraceContext, logWithContext(), logWithTrace(), JSON and plain-text injection")
endif()

# Phase 3 – Distributed tracing unit tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_distributed_tracing.cpp")
    message(STATUS "Adding Distributed Tracing tests (Phase 3)")
    add_executable(test_distributed_tracing test_distributed_tracing.cpp)
    target_link_libraries(test_distributed_tracing PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )
    target_compile_definitions(test_distributed_tracing PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME DistributedTracingTests COMMAND test_distributed_tracing)
    set_tests_properties(DistributedTracingTests PROPERTIES
        LABELS "core;tracing;distributed;spans;phase3;unit"
        TIMEOUT 60
    )
    message(STATUS "  Distributed Tracing: span lifecycle, child spans, error recording, W3C propagation")
endif()

# Phase 2 – Sampling strategy unit tests (replaces stub)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_sampling_strategy.cpp")
    message(STATUS "Adding Sampling Strategy tests (Phase 2)")
    add_executable(test_sampling_strategy test_sampling_strategy.cpp)
    target_link_libraries(test_sampling_strategy PRIVATE
        ${TEST_LIBS}
        themis_core
    )
    target_compile_definitions(test_sampling_strategy PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME SamplingStrategyTests COMMAND test_sampling_strategy)
    set_tests_properties(SamplingStrategyTests PROPERTIES
        LABELS "utils;tracing;sampling;phase2;unit"
        TIMEOUT 30
    )
    message(STATUS "  Sampling Strategy: ALWAYS_ON/OFF, PROBABILITY, PARENT_BASED")
endif()

# Phase 3 – Audit Logger production tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_audit_logger_production.cpp")
    message(STATUS "Adding Audit Logger production tests (Phase 3)")
    add_executable(test_audit_logger_production test_audit_logger_production.cpp)
    target_link_libraries(test_audit_logger_production PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::Crypto
        Threads::Threads
    )
    target_compile_definitions(test_audit_logger_production PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME AuditLoggerProductionTests COMMAND test_audit_logger_production)
    set_tests_properties(AuditLoggerProductionTests PROPERTIES
        LABELS "utils;audit;search;compliance;phase3;unit"
        TIMEOUT 60
    )
    message(STATUS "  Audit Logger Production: search API, compliance reporting, retention")
endif()

# Phase 3 – Governance policy hot-reload tests (Issue: #1774)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_governance_policy_hot_reload.cpp")
    message(STATUS "Adding Governance Policy Hot-Reload tests (Phase 3 - Issue #1774)")
    add_executable(test_governance_policy_hot_reload test_governance_policy_hot_reload.cpp)
    target_link_libraries(test_governance_policy_hot_reload PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::Crypto
        Threads::Threads
    )
    target_compile_definitions(test_governance_policy_hot_reload PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME GovernancePolicyHotReloadTests COMMAND test_governance_policy_hot_reload)
    set_tests_properties(GovernancePolicyHotReloadTests PROPERTIES
        LABELS "governance;policy;hot-reload;file-watcher;phase3;unit"
        TIMEOUT 60
    )
    message(STATUS "  Governance Hot-Reload: PolicyFileWatcher start/stop, file-change detection, Prometheus counter, audit entry")
endif()

# Phase 5 – LEK Manager lifecycle tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_lek_manager_lifecycle.cpp")
    message(STATUS "Adding LEK Manager lifecycle tests (Phase 5)")
    add_executable(test_lek_manager_lifecycle test_lek_manager_lifecycle.cpp)
    target_link_libraries(test_lek_manager_lifecycle PRIVATE
        ${TEST_LIBS}
        themis_core
    )
    target_compile_definitions(test_lek_manager_lifecycle PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME LEKManagerLifecycleTests COMMAND test_lek_manager_lifecycle)
    set_tests_properties(LEKManagerLifecycleTests PROPERTIES
        LABELS "utils;lek;key-management;expiry;revocation;phase5;unit"
        TIMEOUT 30
    )
    message(STATUS "  LEK Manager Lifecycle: expiry, revocation, migration")
endif()

# Phase 7 – SelfAwareness production tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_self_awareness_production.cpp")
    message(STATUS "Adding SelfAwareness production tests (Phase 7)")
    add_executable(test_self_awareness_production
        test_self_awareness_production.cpp
        ${CMAKE_SOURCE_DIR}/src/utils/self_awareness.cpp
    )
    target_link_libraries(test_self_awareness_production PRIVATE
        ${TEST_LIBS}
        themis_core
        yaml-cpp::yaml-cpp
        Threads::Threads
    )
    target_compile_definitions(test_self_awareness_production PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME SelfAwarenessProductionTests COMMAND test_self_awareness_production)
    set_tests_properties(SelfAwarenessProductionTests PROPERTIES
        LABELS "utils;self-awareness;snapshot;anomaly;phase7;unit"
        TIMEOUT 60
    )
    message(STATUS "  SelfAwareness Production: snapshot persistence, anomaly detection")
endif()

# Graph Index Comprehensive Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_graph_index_comprehensive.cpp")
    message(STATUS "Adding Graph Index comprehensive tests")
    
    add_executable(test_graph_index_comprehensive
        test_graph_index_comprehensive.cpp
    )
    
    target_link_libraries(test_graph_index_comprehensive PRIVATE
        ${TEST_LIBS}
        themis_core
        RocksDB::rocksdb
        spdlog::spdlog
        Threads::Threads
    )
    
    target_compile_definitions(test_graph_index_comprehensive PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME GraphIndexComprehensiveTests COMMAND test_graph_index_comprehensive)
    
    set_tests_properties(GraphIndexComprehensiveTests PROPERTIES
        LABELS "index;graph;traversal;comprehensive;unit"
        TIMEOUT 300
    )
    
    message(STATUS "  Graph Index Comprehensive: nodes, edges, traversals, BFS, shortest path, weighted graphs")
else()
    message(STATUS "Graph Index comprehensive tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_graph_index_comprehensive.cpp")
endif()

# Learned Index Structures — ML-based B-tree replacement (Issue #1990)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/index/test_learned_index.cpp")
    message(STATUS "Adding Learned Index tests (Issue #1990)")

    add_executable(test_learned_index
        index/test_learned_index.cpp
    )

    target_include_directories(test_learned_index PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_learned_index PRIVATE
        ${TEST_LIBS}
        Threads::Threads
    )

    target_compile_definitions(test_learned_index PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME LearnedIndexTests COMMAND test_learned_index)

    set_tests_properties(LearnedIndexTests PROPERTIES
        LABELS "index;learned;ml;btree;unit"
        TIMEOUT 60
    )

    message(STATUS "  LearnedIndex: training, point lookup, range query, serialisation, security, edge cases")
else()
    message(STATUS "Learned Index tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/index/test_learned_index.cpp")
endif()

# Distributed Vector Index across Shards (Issue #1879)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/index/test_distributed_vector_index.cpp")
    message(STATUS "Adding Distributed Vector Index tests (Issue #1879)")

    add_executable(test_distributed_vector_index
        index/test_distributed_vector_index.cpp
        ${CMAKE_SOURCE_DIR}/src/index/distributed_vector_index.cpp
        ${CMAKE_SOURCE_DIR}/src/index/ann_index.cpp
    )

    target_include_directories(test_distributed_vector_index PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_distributed_vector_index PRIVATE
        ${TEST_LIBS}
        Threads::Threads
    )

    target_compile_definitions(test_distributed_vector_index PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME DistributedVectorIndexTests COMMAND test_distributed_vector_index)

    set_tests_properties(DistributedVectorIndexTests PROPERTIES
        LABELS "index;distributed;vector;shards;unit"
        TIMEOUT 120
    )

    message(STATUS "  DistributedVectorIndex: insert/remove, scatter-gather KNN, shard routing, stats")
else()
    message(STATUS "Distributed Vector Index tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/index/test_distributed_vector_index.cpp")
endif()

# Cold/Warm Tier Index Migration Tests (Issue #2407)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/index/test_tiered_index_migration.cpp")
    message(STATUS "Adding Cold/Warm Tier Index Migration tests (Issue #2407)")

    add_executable(test_tiered_index_migration
        index/test_tiered_index_migration.cpp
        ${CMAKE_SOURCE_DIR}/src/index/tiered_index_manager.cpp
    )

    target_include_directories(test_tiered_index_migration PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_tiered_index_migration PRIVATE
        ${TEST_LIBS}
        Threads::Threads
    )

    target_compile_definitions(test_tiered_index_migration PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME TieredIndexMigrationTests COMMAND test_tiered_index_migration)

    set_tests_properties(TieredIndexMigrationTests PROPERTIES
        LABELS "index;tiered;cold;warm;migration;unit"
        TIMEOUT 60
    )

    message(STATUS "  TieredIndexMigration: register, access tracking, demotion, promotion, auto-pass")
else()
    message(STATUS "TieredIndexMigration tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/index/test_tiered_index_migration.cpp")
endif()

# Cloud Storage Backup Comprehensive Tests (GAP-008)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_cloud_storage_backup_comprehensive.cpp")
    message(STATUS "Adding Cloud Storage Backup comprehensive tests (GAP-008)")
    
    add_executable(test_cloud_storage_backup_comprehensive
        test_cloud_storage_backup_comprehensive.cpp
    )
    
    target_link_libraries(test_cloud_storage_backup_comprehensive PRIVATE
        ${TEST_LIBS}
        themis_core
        RocksDB::rocksdb
        spdlog::spdlog
        Threads::Threads
    )
    
    # Add cloud storage SDK dependencies if available
    if(TARGET aws-cpp-sdk-s3)
        target_link_libraries(test_cloud_storage_backup_comprehensive PRIVATE
            aws-cpp-sdk-s3
            aws-cpp-sdk-transfer
        )
        target_compile_definitions(test_cloud_storage_backup_comprehensive PRIVATE
            THEMIS_HAS_AWS_SDK=1
        )
    endif()
    
    if(TARGET azure-storage-cpp)
        target_link_libraries(test_cloud_storage_backup_comprehensive PRIVATE
            azure-storage-cpp
        )
        target_compile_definitions(test_cloud_storage_backup_comprehensive PRIVATE
            THEMIS_HAS_AZURE_STORAGE=1
        )
    endif()
    
    if(TARGET google-cloud-cpp::storage)
        target_link_libraries(test_cloud_storage_backup_comprehensive PRIVATE
            google-cloud-cpp::storage
        )
        target_compile_definitions(test_cloud_storage_backup_comprehensive PRIVATE
            THEMIS_HAS_GCS_SDK=1
        )
    endif()
    
    target_compile_definitions(test_cloud_storage_backup_comprehensive PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME CloudStorageBackupComprehensiveTests COMMAND test_cloud_storage_backup_comprehensive)
    
    set_tests_properties(CloudStorageBackupComprehensiveTests PROPERTIES
        LABELS "backup;cloud;storage;s3;azure;gcs;comprehensive;unit"
        TIMEOUT 180
    )
    
    message(STATUS "  Cloud Storage Backup: AWS S3, Azure Blob, Google Cloud Storage interfaces")
else()
    message(STATUS "Cloud Storage Backup comprehensive tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_cloud_storage_backup_comprehensive.cpp")
endif()

# ============================================================================
# Custom Test Targets
# ============================================================================

# Add custom target to run all tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

message(STATUS "Tests configured successfully")
