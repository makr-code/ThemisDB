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
list(FILTER ALL_TEST_SOURCES EXCLUDE REGEX ".*test_io_uring_zero_copy\\.cpp$")

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
        ".*test_rag_document_summarizer\\.cpp$"
        ".*test_rag_knowledge_graph_retriever\\.cpp$"
        ".*test_rag_document_splitter\\.cpp$"
        ".*test_rag_hybrid_retriever\\.cpp$"
        ".*test_rag_agentic\\.cpp$"
        ".*test_rag_evaluation_report_exporter\\.cpp$"
        ".*test_rag_multimodal\\.cpp$"
        ".*test_rag_distributed_evaluator\\.cpp$"
        ".*test_rag_prompt_injection\\.cpp$"
        ".*test_rag_evaluation_cache\\.cpp$"
        ".*test_rag_calibration_manager\\.cpp$"
        ".*test_rag_batch_evaluator\\.cpp$"
        ".*test_rag_pipeline_integration\\.cpp$"
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
        ".*test_speculative_decoder\\.cpp$"
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
        ".*test_aql_agent\\.cpp$"
        ".*test_nl_to_aql_translation\\.cpp$"
        ".*test_batch_nl_to_aql_translation\\.cpp$"
        ".*test_rag_aql_integration\\.cpp$"
        ".*test_aql_schema_aware\\.cpp$"
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
        ".*test_model_quantization_pipeline\\.cpp$"
        ".*test_model_router\\.cpp$"
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

if(NOT THEMIS_GEO_CUDA)
    list(APPEND TEST_EXCLUDE_PATTERNS
        ".*test_cuda_geo_kernels\\.cpp$"
    )
endif()

if(NOT THEMIS_GEO_HIP)
    list(APPEND TEST_EXCLUDE_PATTERNS
        ".*test_hip_geo_kernels\\.cpp$"
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
        # Phase 3 training tests – not LLM-dependent
        "${CMAKE_CURRENT_SOURCE_DIR}/test_lora_checkpoint_manager.cpp"
    )
    list(REMOVE_DUPLICATES ALL_TEST_SOURCES)
endif()

if(NOT THEMIS_ENABLE_LEGAL_TRAINING)
    list(APPEND TEST_EXCLUDE_PATTERNS
        ".*test_lora_data_selection\\.cpp$"
        ".*test_training_pipeline_e2e\\.cpp$"
        ".*test_lora_checkpoint_manager\\.cpp$"
        ".*test_provenance_tracker\\.cpp$"
    )
endif()

if(NOT THEMIS_ENABLE_WEBSOCKET)
    list(APPEND TEST_EXCLUDE_PATTERNS
        ".*test_wire_protocol_websocket\\.cpp$"
        ".*test_ws_handler\\.cpp$"
        ".*test_websocket_cdc\\.cpp$"
        ".*test_graphql_ws_handler\\.cpp$"
    )
endif()

if(NOT THEMIS_ENABLE_HTTP3)
    list(APPEND TEST_EXCLUDE_PATTERNS
        ".*test_http3_protocol\\.cpp$"
        ".*test_quic_transport\\.cpp$"
    )
endif()

if(NOT THEMIS_ENABLE_GRPC)
    list(APPEND TEST_EXCLUDE_PATTERNS
        ".*test_grpc_transport\\.cpp$"
    )
endif()

if(NOT THEMIS_ENABLE_SERVICE_MESH)
    list(APPEND TEST_EXCLUDE_PATTERNS
        ".*test_service_mesh\\.cpp$"
        ".*test_envoy_xds\\.cpp$"
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
        ".*test_ocr_processor\\.cpp$"
        ".*test_content_embedding_pipeline\\.cpp$"
        ".*test_content_html_processor\\.cpp$"
        ".*test_content_markdown_processor\\.cpp$"
        ".*test_content_version_manager\\.cpp$"
        ".*test_content_streaming_ingestion\\.cpp$"
        ".*test_content_audio_processor\\.cpp$"
        ".*test_content_deduplication\\.cpp$"
        ".*test_content_errors\\.cpp$"
        ".*test_content_features\\.cpp$"
        ".*test_content_fulltext_index\\.cpp$"
        ".*test_content_language_detector\\.cpp$"
        ".*test_content_logger\\.cpp$"
        ".*test_content_metrics\\.cpp$"
        ".*test_content_pipeline\\.cpp$"
        ".*test_content_pipeline_hardening\\.cpp$"
        ".*test_content_processor_chain\\.cpp$"
        ".*test_content_security\\.cpp$"
    )
endif()

# Tests with own main() - these have custom logic or missing dependencies
list(APPEND TEST_EXCLUDE_PATTERNS
    ".*test_argument_store\\.cpp$"
    ".*test_schema_validator\\.cpp$"
    ".*test_flatfile_importer\\.cpp$"
    ".*test_adaptive_cache_fuzz\\.cpp$"
    ".*test_adaptive_cache_integration\\.cpp$"
    ".*test_adaptive_cache_phase1\\.cpp$"
    ".*test_continuous_learning_client\\.cpp$"
    ".*test_geval\\.cpp$"
    ".*test_metrics\\.cpp$"
    ".*test_onnx_model_loader\\.cpp$"
    ".*test_content_policy_manual\\.cpp$"
    ".*test_llm_deployment_plugin\\.cpp$"
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

if(NOT THEMIS_ENABLE_PMEM)
    list(APPEND TEST_EXCLUDE_PATTERNS
        ".*test_pmem_storage\\.cpp$"
    )
endif()

if(NOT THEMIS_ENABLE_PMU_COUNTERS)
    list(APPEND TEST_EXCLUDE_PATTERNS
        ".*test_pmu_counters\\.cpp$"
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
    ".*test_cdn_cache_middleware\\.cpp$"
    ".*test_mcp_orchestrator_bridge\\.cpp$"
    ".*test_shared_worker_pool\\.cpp$"
    # test_importer_plugin_api uses THEMIS_IMPORTER_PLUGIN_IMPL which generates
    # C-linkage createPlugin/destroyPlugin symbols.  postgres_importer.cpp also
    # exports these same symbols in its own extern "C" block (see the bottom of
    # src/importers/postgres_importer.cpp).  Linking both into the same static
    # binary causes a duplicate-symbol linker error.  Build it as a dedicated
    # standalone target below instead.
    ".*test_importer_plugin_api\\.cpp$"
    # test_importer_interfaces is built as a dedicated standalone target below
    # to allow running the abstract-interface tests in isolation.
    ".*test_importer_interfaces\\.cpp$"
    # test_plugin_capability_negotiation is built as a dedicated standalone
    # target below (PluginCapabilityNegotiationTests) to allow running the
    # capability-negotiation tests in isolation.
    ".*test_plugin_capability_negotiation\\.cpp$"
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

# Re-include test_context_propagation.cpp excluded by the catch-all pattern above.
# This test validates ContextPropagation (thread-local current context, ContextScope
# RAII install/restore, propagate() across async tasks, nested scopes, exception
# safety, and multi-thread isolation).  It has no GPU or LLM dependencies; the
# previous exclusion was due to context_propagation.cpp being absent from the build
# graph (the thread_local definition was not compiled in).  Now that the source file
# is registered in cmake/CMakeLists.txt and cmake/ModularBuild.cmake it is safe to
# re-include unconditionally.
target_sources(themis_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/test_context_propagation.cpp
)

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
    ${CMAKE_CURRENT_SOURCE_DIR}/test_chain_of_thought.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_rag_prompt_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_system_prompt_manager.cpp
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

# Re-include acceleration module tests excluded by the explicit patterns
# ".*test_acceleration\\.cpp$" and ".*test_acceleration_dispatch\\.cpp$".
# Both tests exercise only CPU backends and gracefully handle missing CUDA devices:
#  - test_acceleration.cpp: BackendRegistry, CPUVectorBackend, CPUGeoBackend,
#    CPUGraphBackend, capability negotiation, input validation, tie-breaking.
#  - test_acceleration_dispatch.cpp: ANNKernelDispatch / GeoKernelDispatch CPU
#    function-pointer table; CUDA dispatch table (graceful no-op when unavailable).
# No GPU hardware is required to run these tests.
target_sources(themis_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/test_acceleration.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_acceleration_dispatch.cpp
)

# Re-include shader integrity test excluded by ".*test_shader_integrity\\.cpp$".
# shader_integrity.cpp uses only OpenSSL SHA-256 (no GPU dependency) and is
# now compiled unconditionally as a core acceleration source.  The test covers:
#  - SHA-256 hash computation (13 test cases)
#  - Hash registration, strict/non-strict verification modes
#  - Manifest file parsing
# No GPU hardware is required.
target_sources(themis_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/test_shader_integrity.cpp
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

# Re-include the GPU graph traversal test excluded by the broad ".*test_gpu_.*\\.cpp$"
# pattern.  This test validates GPUGraphTraversal (CSR load, BFS, DFS) using the
# CPU fallback path that is always active when no CUDA device is present.
# It exercises the complete traversal API (load, bfs, dfs, getStats) and does
# not require GPU hardware.
target_sources(themis_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_graph_traversal.cpp
)

# Re-include the GPU query accelerator test excluded by the broad
# ".*test_gpu_.*\\.cpp$" pattern.  This test validates GPUQueryAccelerator
# (scan, sort, aggregate, hashJoin, dotProduct FP16/BF16, and annSearch with
# CPU brute-force exact k-NN fallback).  It uses only CPU-side bookkeeping
# (force_cpu = true) and requires no GPU or cuVS/RAFT hardware.
if(THEMIS_ENABLE_GPU)
    target_sources(themis_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_query_accelerator.cpp
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

# Re-include the GPU load balancer test excluded by the broad ".*test_gpu_.*\\.cpp$"
# pattern.  This test validates GPULoadBalancer (ROUND_ROBIN, LEAST_LOADED,
# FIRST_HEALTHY, and TOPOLOGY_AWARE strategies), NVLink topology-aware scheduling
# via setTopology() / selectTopologyAware(), device failure handling, and
# concurrent safety.  It uses only CPU-side bookkeeping and requires no GPU hardware.
if(THEMIS_ENABLE_GPU)
    target_sources(themis_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_load_balancer.cpp
    )
endif()

# Re-include GPU module tests that were excluded by the broad ".*test_gpu_.*\\.cpp$"
# and related specific-pattern exclusions.  All of these tests exercise only
# CPU-side bookkeeping paths and require no GPU hardware.
if(THEMIS_ENABLE_GPU)
    target_sources(themis_tests PRIVATE
        # GPUAdminAPI: JSON stats serialisation, tenant breakdown, dry-run simulation
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_admin_api.cpp
        # GPUAlertManager: threshold evaluation, callback firing, hysteresis, stats
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_alerts.cpp
        # GPUAuditLog: ring-buffer event recording, overflow, structured fields
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_audit_log.cpp
        # GPUConfig: validation, dry-run simulation, VRAM cap enforcement
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_config.cpp
        # DeviceDiscovery: CPU-fallback sentinel, healthy-device filter, best-device
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_device_discovery.cpp
        # GPUFeatureFlags: per-edition gates, runtime overrides, serialisation
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_feature_flags.cpp
        # GPUKernelValidator: FNV-1a whitelist, validate-before-launch enforcement
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_kernel_validator.cpp
        # GPULauncher: typed work-item submission, timeout enforcement, batch launch
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_launcher.cpp
        # GPUMemoryManagerEdition: tenant quotas, VRAM cap, pre-allocation hints
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_memory_management.cpp
        # GPUModule facade: policy → circuit-breaker → alloc → launch pipeline
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_module.cpp
        # GPUOLAPAccelerator: parallel aggregation, sort, hash-join CPU paths
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_olap_accelerator.cpp
        # GPUPolicy: default-deny gate, per-edition capability overrides
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_policy.cpp
        # GPUSafeFail (circuit-breaker): trip/reset, fallback counter, callbacks
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_safe_fail.cpp
        # GPUSafeFail integration through GPUModule facade
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_safe_fail_module.cpp
        # Comprehensive stub coverage for GPU module public API surface
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_stubs_comprehensive.cpp
        # GPUTensorBuffer: shape/dtype, views, checkpointing, CPU bookkeeping
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_tensor.cpp
        # GPUTrainingLoop: batch iteration, loss tracking, early stopping
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_training_loop.cpp
        # GPUMemoryManagerEdition VRAM allocation: tenant quotas, eviction
        ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_vram_allocation.cpp
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
        # CitationHighlighter has no LLM dependency (pure Jaccard term-overlap);
        # re-include so non-LLM builds exercise the citation highlighting feature.
        ${CMAKE_CURRENT_SOURCE_DIR}/test_rag_citation_highlighter.cpp
        # EvaluationReportExporter has no LLM dependency (pure serialisation);
        # re-include so non-LLM builds exercise the JSON/HTML export feature.
        ${CMAKE_CURRENT_SOURCE_DIR}/test_rag_evaluation_report_exporter.cpp
        # EvaluationCache, CalibrationManager, BatchEvaluator have no LLM
        # dependency; re-include so non-LLM builds exercise these features.
        ${CMAKE_CURRENT_SOURCE_DIR}/test_rag_evaluation_cache.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test_rag_calibration_manager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test_rag_batch_evaluator.cpp
        # Pipeline integration test (heuristic/FAST mode, no LLM dependency).
        ${CMAKE_CURRENT_SOURCE_DIR}/test_rag_pipeline_integration.cpp
    )
endif()

# Ensure VisionConfig implementation is linked for vision API tests.
target_sources(themis_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/llm/vision_config.cpp
)

# Ensure implementations required by unit tests are always linked.
target_sources(themis_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/exporters/data_augmentation.cpp
    ${CMAKE_SOURCE_DIR}/src/exporters/export_format_registry.cpp
    ${CMAKE_SOURCE_DIR}/src/exporters/huggingface_hub_client.cpp
    ${CMAKE_SOURCE_DIR}/src/performance/phase3/adaptive_batch_tuner.cpp
    ${CMAKE_SOURCE_DIR}/src/server/import_wizard_builder.cpp
    ${CMAKE_SOURCE_DIR}/src/llm/llm_security_utils.cpp
    ${CMAKE_SOURCE_DIR}/src/search/multi_field_search.cpp
    ${CMAKE_SOURCE_DIR}/src/search/search_highlighter.cpp
    ${CMAKE_SOURCE_DIR}/src/search/negative_keyword_filter.cpp
    ${CMAKE_SOURCE_DIR}/src/sharding/cloud_backup.cpp
    ${CMAKE_SOURCE_DIR}/src/transaction/distributed_saga.cpp
    ${CMAKE_SOURCE_DIR}/src/server/grpc_web_proxy_handler.cpp
    ${CMAKE_SOURCE_DIR}/src/rag/evaluation_report_exporter.cpp
    ${CMAKE_SOURCE_DIR}/src/rag/evaluation_cache.cpp
    ${CMAKE_SOURCE_DIR}/src/rag/calibration_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/rag/batch_evaluator.cpp
    ${CMAKE_SOURCE_DIR}/src/server/cdn_cache_middleware.cpp
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
        ../src/ingestion/web_crawler_connector.cpp
        ../src/ingestion/ingestion_coordinator.cpp
        ../src/ingestion/cdc_connector.cpp
        ../src/training/auto_labeler.cpp
        ../src/training/knowledge_graph_enricher.cpp
        ../src/training/incremental_lora_trainer.cpp
        ../src/training/training_pipeline.cpp
        ../src/training/lora_data_selection.cpp
        ../src/training/lora_checkpoint_manager.cpp
        ../src/training/provenance_tracker.cpp
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
        ../src/voice/voice_macro_manager.cpp
        ../src/voice/emotion_analyzer.cpp
        ../src/voice/voice_browser_streaming.cpp
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
    ../src/chimera/neo4j_adapter.cpp
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
            ${THEMIS_ROOT_DIR}/src/graph/path_constraints.cpp
            ${THEMIS_ROOT_DIR}/src/graph/parallel_traversal.cpp
            ${THEMIS_ROOT_DIR}/src/timeseries/ts_auto_buffer.cpp
            ${THEMIS_ROOT_DIR}/src/timeseries/tsstore.cpp
            ${THEMIS_ROOT_DIR}/src/timeseries/timeseries_metrics.cpp
            ${THEMIS_ROOT_DIR}/src/timeseries/prometheus_remote_write.cpp
            ${THEMIS_ROOT_DIR}/src/timeseries/downsampling.cpp
            ${THEMIS_ROOT_DIR}/src/index/vector_auto_buffer.cpp
            ${THEMIS_ROOT_DIR}/src/cdc/cdc_admin.cpp
            ${THEMIS_ROOT_DIR}/src/cdc/changefeed_buffer.cpp
            ${THEMIS_ROOT_DIR}/src/cdc/tenant_buffer_manager.cpp
            ${THEMIS_ROOT_DIR}/src/storage/pitr_manager.cpp
            ${THEMIS_ROOT_DIR}/src/auth/auth_error.cpp
            ${THEMIS_ROOT_DIR}/src/auth/mtls_authenticator.cpp
            ${THEMIS_ROOT_DIR}/tests/test_hsm_provider_stub.cpp
            ${THEMIS_ROOT_DIR}/src/cdc/dead_letter_queue.cpp
            ${THEMIS_ROOT_DIR}/src/cdc/consumer_group.cpp
            ${THEMIS_ROOT_DIR}/src/cdc/delivery_tracker.cpp
            ${THEMIS_ROOT_DIR}/src/cdc/cross_collection_stream.cpp
            ${THEMIS_ROOT_DIR}/src/cdc/cdc_materialized_view.cpp
            $<$<BOOL:${THEMIS_ENABLE_KAFKA}>:${THEMIS_ROOT_DIR}/src/cdc/kafka_cdc_producer.cpp>
            ${THEMIS_ROOT_DIR}/src/cdc/outbox.cpp
            $<$<BOOL:${THEMIS_ENABLE_WEBSOCKET}>:${THEMIS_ROOT_DIR}/src/cdc/ws_transport.cpp>
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
if(TARGET Snappy::snappy)
    target_link_libraries(themis_tests PRIVATE Snappy::snappy)
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

# Arrow / Parquet dependencies for analytics_export.cpp (compiled into themis_tests)
if(DEFINED THEMIS_ARROW_TARGET AND TARGET ${THEMIS_ARROW_TARGET})
    target_link_libraries(themis_tests PRIVATE ${THEMIS_ARROW_TARGET})
elseif(TARGET Arrow::arrow_shared)
    target_link_libraries(themis_tests PRIVATE Arrow::arrow_shared)
elseif(TARGET Arrow::arrow_static)
    target_link_libraries(themis_tests PRIVATE Arrow::arrow_static)
endif()

if(DEFINED THEMIS_PARQUET_TARGET AND TARGET ${THEMIS_PARQUET_TARGET})
    target_link_libraries(themis_tests PRIVATE ${THEMIS_PARQUET_TARGET})
elseif(TARGET Parquet::parquet_shared)
    target_link_libraries(themis_tests PRIVATE Parquet::parquet_shared)
elseif(TARGET Parquet::parquet_static)
    target_link_libraries(themis_tests PRIVATE Parquet::parquet_static)
elseif(TARGET Parquet::parquet)
    target_link_libraries(themis_tests PRIVATE Parquet::parquet)
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

# Themis Wire Protocol V1 unit tests (subset of AllTests)
# Covers: WireV1Constants, WireFrameHeader, OpCode, MessageFlags,
#         MessageDispatcher, WireProtocolSession, WireProtocolServer,
#         WireProtocolConstants (from test_themis_wire_protocol_server.cpp)
add_test(NAME ThemisWireProtocolV1Tests
    COMMAND themis_tests --gtest_filter=WireV1Constants*:WireFrameHeader*:OpCode*:MessageFlags*:MessageDispatcher*:WireProtocolSession*:WireProtocolServer*:WireProtocolConstants*)
set_tests_properties(ThemisWireProtocolV1Tests PROPERTIES
    LABELS "wire;protocol;themis;unit"
    TIMEOUT 60
)

# Build info and module availability tests (subset of AllTests)
# Covers: getBuildConfiguration(), isModuleCompiledIn(), getCompiledModules(),
#         getDisabledModules(), getVersionSummary(), formatBuildInfo(),
#         getReproducibilityInfo(), exportBuildManifest(), verifyBuildManifest()
#         (from test_build_info.cpp, Issue #2470)
add_test(NAME BuildInfoTests
    COMMAND themis_tests --gtest_filter=BuildConfiguration.*:BuildInfo.*:ModuleQuery.*:ReproducibilityInfo.*:BuildManifest.*)
set_tests_properties(BuildInfoTests PROPERTIES
    LABELS "build;info;module;availability;themis;unit"
    TIMEOUT 60
)

# Edition manager tests (subset of AllTests)
# Covers: singleton identity, edition information (type/name/limits),
#         feature availability (compile-time gate + runtime license gate),
#         resource-limit checks (node count, VRAM), feature enumeration,
#         upgrade messages, RuntimeLicenseGate integration
#         (from test_edition_manager.cpp, Issue #2469)
add_test(NAME EditionManagerTests
    COMMAND themis_tests --gtest_filter=EditionManager.*)
set_tests_properties(EditionManagerTests PROPERTIES
    LABELS "edition;feature;gating;themis;unit"
    TIMEOUT 60
)

# Dynamic feature-flag override tests (subset of AllTests)
# Covers: setFeatureOverride / clearFeatureOverride / clearAllFeatureOverrides /
#         hasFeatureOverride / getFeatureOverride, interaction with compile-time
#         edition gate and RuntimeLicenseGate, feature enumeration, thread-safety
#         (from test_dynamic_feature_flags.cpp, Issue #2469)
add_test(NAME DynamicFeatureFlagTests
    COMMAND themis_tests --gtest_filter=DynamicFeatureFlagTest.*)
set_tests_properties(DynamicFeatureFlagTests PROPERTIES
    LABELS "edition;feature;override;themis;unit"
    TIMEOUT 60
)

# Runtime license gate tests (subset of AllTests)
# Covers: uninitialized gate, active/expired/grace/invalid/offline license states,
#         compile-time OFF features, unknown features, initialize()/update()
#         idempotency, diagnostics (licenseStatus, graceDaysRemaining, currentLicense)
#         (from test_runtime_license_gate.cpp)
add_test(NAME RuntimeLicenseGateTests
    COMMAND themis_tests --gtest_filter=RuntimeLicenseGateInit.*:RuntimeLicenseGate.*)
set_tests_properties(RuntimeLicenseGateTests PROPERTIES
    LABELS "license;runtime;gate;themis;unit"
    TIMEOUT 60
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
# Graph Advanced Features Tests (standalone — excluded from unified binary
# because helper functions use bare assert() calls that abort the process)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_graph_advanced_features.cpp")
    message(STATUS "Adding Graph Advanced Features tests")

    add_executable(test_graph_advanced_features
        test_graph_advanced_features.cpp
        ${THEMIS_ROOT_DIR}/src/graph/graph_query_optimizer.cpp
        ${THEMIS_ROOT_DIR}/src/graph/path_constraints.cpp
        ${THEMIS_ROOT_DIR}/src/graph/parallel_traversal.cpp
    )

    target_link_libraries(test_graph_advanced_features PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        fmt::fmt
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )

    target_compile_definitions(test_graph_advanced_features PRIVATE
        THEMIS_TEST_BUILD=1
        THEMIS_GRAPH_EXPORTS
    )

    add_test(NAME GraphAdvancedFeaturesTests COMMAND test_graph_advanced_features)

    set_tests_properties(GraphAdvancedFeaturesTests PROPERTIES
        LABELS "graph;path-constraints;advanced;unit"
        TIMEOUT 120
    )

    message(STATUS "  Graph Advanced Features: PathConstraints validation, constrained path finding, optimizer integration")
else()
    message(STATUS "Graph Advanced Features tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_graph_advanced_features.cpp")
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
# Parallel Graph Traversal Tests (standalone)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_graph_parallel_traversal.cpp")
    message(STATUS "Adding Parallel Graph Traversal tests")

    add_executable(test_graph_parallel_traversal
        test_graph_parallel_traversal.cpp
        ${THEMIS_ROOT_DIR}/src/graph/parallel_traversal.cpp
    )

    target_link_libraries(test_graph_parallel_traversal PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        fmt::fmt
        RocksDB::rocksdb
        Threads::Threads
    )

    target_compile_definitions(test_graph_parallel_traversal PRIVATE
        THEMIS_TEST_BUILD=1
        THEMIS_GRAPH_EXPORTS
    )

    add_test(NAME ParallelGraphTraversalTests COMMAND test_graph_parallel_traversal)

    set_tests_properties(ParallelGraphTraversalTests PROPERTIES
        LABELS "graph;parallel;bfs;dfs;unit"
        TIMEOUT 120
    )

    message(STATUS "  Parallel Graph Traversal: multi-source BFS/DFS, fan-out threshold, depth/result limits")
else()
    message(STATUS "Parallel Graph Traversal tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_graph_parallel_traversal.cpp")
endif()

# ============================================================================
# GPU Graph Traversal Tests (standalone — CPU fallback, no GPU required)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_graph_traversal.cpp")
    message(STATUS "Adding GPU Graph Traversal tests (CPU fallback)")

    add_executable(test_gpu_graph_traversal
        test_gpu_graph_traversal.cpp
        ${THEMIS_ROOT_DIR}/src/graph/gpu_traversal.cpp
        ${THEMIS_ROOT_DIR}/src/graph/graph_query_optimizer.cpp
        ${THEMIS_ROOT_DIR}/src/graph/path_constraints.cpp
        ${THEMIS_ROOT_DIR}/src/graph/parallel_traversal.cpp
    )

    target_link_libraries(test_gpu_graph_traversal PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        fmt::fmt
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )

    target_compile_definitions(test_gpu_graph_traversal PRIVATE
        THEMIS_TEST_BUILD=1
        THEMIS_GRAPH_EXPORTS
    )

    add_test(NAME GPUGraphTraversalTests COMMAND test_gpu_graph_traversal)

    set_tests_properties(GPUGraphTraversalTests PROPERTIES
        LABELS "graph;gpu;bfs;dfs;cpu-fallback;unit"
        TIMEOUT 120
    )

    message(STATUS "  GPU Graph Traversal: CSR load, BFS/DFS correctness, CPU fallback, stats")
else()
    message(STATUS "GPU Graph Traversal tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_graph_traversal.cpp")
endif()

# ============================================================================
# Graph Analytics Tests (standalone)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_graph_analytics.cpp")
    message(STATUS "Adding Graph Analytics tests (standalone)")

    add_executable(test_graph_analytics
        test_graph_analytics.cpp
    )

    target_link_libraries(test_graph_analytics PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        fmt::fmt
        RocksDB::rocksdb
        Threads::Threads
    )

    target_compile_definitions(test_graph_analytics PRIVATE
        THEMIS_TEST_BUILD=1
        THEMIS_GRAPH_EXPORTS
    )

    add_test(NAME GraphAnalyticsTests COMMAND test_graph_analytics)

    set_tests_properties(GraphAnalyticsTests PROPERTIES
        LABELS "graph;analytics;centrality;pagerank;community;unit"
        TIMEOUT 120
    )

    message(STATUS "  GraphAnalytics: degree/betweenness/closeness centrality, PageRank, Louvain community detection")
else()
    message(STATUS "Graph Analytics tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_graph_analytics.cpp")
endif()

# ============================================================================
# Graph Type Filtering Tests (standalone)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_graph_type_filtering.cpp")
    message(STATUS "Adding Graph Type Filtering tests (standalone)")

    add_executable(test_graph_type_filtering
        test_graph_type_filtering.cpp
    )

    target_link_libraries(test_graph_type_filtering PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        fmt::fmt
        RocksDB::rocksdb
        Threads::Threads
    )

    target_compile_definitions(test_graph_type_filtering PRIVATE
        THEMIS_TEST_BUILD=1
        THEMIS_GRAPH_EXPORTS
    )

    add_test(NAME GraphTypeFilteringTests COMMAND test_graph_type_filtering)

    set_tests_properties(GraphTypeFilteringTests PROPERTIES
        LABELS "graph;type-filtering;bfs;dijkstra;property-graph;unit"
        TIMEOUT 120
    )

    message(STATUS "  GraphTypeFiltering: BFS/Dijkstra with edge-type filters, property graph traversal")
else()
    message(STATUS "Graph Type Filtering tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_graph_type_filtering.cpp")
endif()

# ============================================================================
# Graph Edge Encryption Tests (standalone)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_graph_edge_encryption.cpp")
    message(STATUS "Adding Graph Edge Encryption tests (standalone)")

    add_executable(test_graph_edge_encryption
        test_graph_edge_encryption.cpp
    )

    target_link_libraries(test_graph_edge_encryption PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        fmt::fmt
        RocksDB::rocksdb
        Threads::Threads
    )

    target_compile_definitions(test_graph_edge_encryption PRIVATE
        THEMIS_TEST_BUILD=1
        THEMIS_GRAPH_EXPORTS
    )

    add_test(NAME GraphEdgeEncryptionTests COMMAND test_graph_edge_encryption)

    set_tests_properties(GraphEdgeEncryptionTests PROPERTIES
        LABELS "graph;edge;encryption;aes-gcm;field-encryption;unit"
        TIMEOUT 120
    )

    message(STATUS "  GraphEdgeEncryption: field encryption round-trip, multi-user access, BFS traversal with encrypted edges")
else()
    message(STATUS "Graph Edge Encryption tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_graph_edge_encryption.cpp")
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
# RPC Batch Operations Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_rpc_batch_operations.cpp")
    message(STATUS "Adding RPC Batch Operations tests")

    add_executable(test_rpc_batch_operations
        test_rpc_batch_operations.cpp
        ${CMAKE_SOURCE_DIR}/src/server/rpc/rpc_service_impl.cpp
    )

    target_link_libraries(test_rpc_batch_operations PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_rpc_batch_operations PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME RPCBatchOperationsTests COMMAND test_rpc_batch_operations)

    set_tests_properties(RPCBatchOperationsTests PROPERTIES
        LABELS "rpc;batch;integration;performance;unit"
        TIMEOUT 120
    )

    message(STATUS "  RPC Batch Operations: batch_get, batch_put, batch_delete, batch_update + perf benchmarks")
else()
    message(STATUS "RPC Batch Operations tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_rpc_batch_operations.cpp")
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
# OTLP Exporter Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_otlp_exporter.cpp")
    message(STATUS "Adding OTLP exporter tests")

    add_executable(test_otlp_exporter
        test_otlp_exporter.cpp
    )

    target_link_libraries(test_otlp_exporter PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
        CURL::libcurl
    )

    target_compile_definitions(test_otlp_exporter PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME OtlpExporterTests COMMAND test_otlp_exporter)

    set_tests_properties(OtlpExporterTests PROPERTIES
        LABELS "api;tracing;otlp;unit"
        TIMEOUT 60
    )

    message(STATUS "  OTLP Exporter: queue, span serialisation, middleware integration")
else()
    message(STATUS "OTLP exporter tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_otlp_exporter.cpp")
endif()

# ============================================================================
# API Interfaces Tests (header-only types: IHttpHandler, CorrelationId, etc.)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_api_interfaces.cpp")
    message(STATUS "Adding API interface header tests")

    add_executable(test_api_interfaces
        test_api_interfaces.cpp
    )

    target_link_libraries(test_api_interfaces PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_api_interfaces PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ApiInterfacesTests COMMAND test_api_interfaces)

    set_tests_properties(ApiInterfacesTests PROPERTIES
        LABELS "api;interfaces;unit"
        TIMEOUT 60
    )

    message(STATUS "  API Interfaces: IHttpHandler, CorrelationId, WebSocketCloseCode, VersionDescriptor, GRPCMetadata")
else()
    message(STATUS "API interface tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_api_interfaces.cpp")
endif()

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
# GraphQL WebSocket Handler Tests (graphql-transport-ws protocol)
# ============================================================================

if(THEMIS_ENABLE_WEBSOCKET AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_graphql_ws_handler.cpp")
    message(STATUS "Adding GraphQL WebSocket handler tests")

    add_executable(test_graphql_ws_handler
        test_graphql_ws_handler.cpp
    )

    target_link_libraries(test_graphql_ws_handler PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )

    target_compile_definitions(test_graphql_ws_handler PRIVATE
        THEMIS_TEST_BUILD=1
        THEMIS_ENABLE_WEBSOCKET=1
    )

    add_test(NAME GraphQLWsHandlerTests COMMAND test_graphql_ws_handler)

    set_tests_properties(GraphQLWsHandlerTests PROPERTIES
        LABELS "api;graphql;websocket;subscription;unit"
        TIMEOUT 60
    )

    message(STATUS "  GraphQL WS Handler: graphql-transport-ws protocol, subscription management")
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
# gRPC-Web Proxy Handler Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_grpc_web_proxy_handler.cpp")
    message(STATUS "Adding gRPC-Web Proxy Handler tests")

    add_executable(test_grpc_web_proxy_handler
        test_grpc_web_proxy_handler.cpp
        ${THEMIS_ROOT_DIR}/src/server/grpc_web_proxy_handler.cpp
    )

    target_include_directories(test_grpc_web_proxy_handler PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_grpc_web_proxy_handler PRIVATE
        ${TEST_LIBS}
        Threads::Threads
    )

    target_compile_definitions(test_grpc_web_proxy_handler PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME GrpcWebProxyHandlerTests
             COMMAND test_grpc_web_proxy_handler)

    set_tests_properties(GrpcWebProxyHandlerTests PROPERTIES
        LABELS "server;grpc;grpc-web;proxy;unit"
        TIMEOUT 60
    )

    message(STATUS "  gRPC-Web Proxy Handler: frame decode/encode, CORS, status, request validation")
else()
    message(STATUS "gRPC-Web Proxy Handler tests not found")
endif()

# ============================================================================
# Service Mesh API Handler Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_service_mesh_api_handler.cpp")
    message(STATUS "Adding Service Mesh API Handler tests")

    add_executable(test_service_mesh_api_handler
        test_service_mesh_api_handler.cpp
        ${THEMIS_ROOT_DIR}/src/server/service_mesh_api_handler.cpp
    )

    if(THEMIS_ENABLE_SERVICE_MESH)
        target_sources(test_service_mesh_api_handler PRIVATE
            ${THEMIS_ROOT_DIR}/src/network/service_mesh.cpp
        )
        target_compile_definitions(test_service_mesh_api_handler PRIVATE
            THEMIS_ENABLE_SERVICE_MESH=1
        )
    endif()

    target_include_directories(test_service_mesh_api_handler PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_service_mesh_api_handler PRIVATE
        ${TEST_LIBS}
        Threads::Threads
    )

    target_compile_definitions(test_service_mesh_api_handler PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ServiceMeshApiHandlerTests
             COMMAND test_service_mesh_api_handler)

    set_tests_properties(ServiceMeshApiHandlerTests PROPERTIES
        LABELS "server;service-mesh;network;unit"
        TIMEOUT 60
    )

    message(STATUS "  Service Mesh API Handler: status, config, annotations endpoints")
else()
    message(STATUS "Service Mesh API Handler tests not found")
endif()

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

# Transaction Bulk API Tests
# APIs under test : Transaction::bulkPutEntities, Transaction::bulkEraseEntities
# Test count      : 12 tests (unit + integration)
# Test types      : unit (empty-vector no-op, single entity, invalid PK rejection,
#                         finished-transaction guard, large-batch commit)
#                   integration (multi-entity commit with secondary-index verification,
#                                rollback atomicity, mixed put+erase in one transaction,
#                                bulk erase of previously inserted rows)
# Acceptance      : All bulk operations commit/rollback atomically; secondary indexes
#                   stay consistent; empty or invalid inputs are rejected with a
#                   descriptive error message.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_transaction_bulk.cpp")
    message(STATUS "Adding Transaction Bulk API tests")

    add_executable(test_transaction_bulk
        test_transaction_bulk.cpp
    )

    target_link_libraries(test_transaction_bulk PRIVATE
        ${TEST_LIBS}
        themis_core
        RocksDB::rocksdb
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_transaction_bulk PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME TransactionBulkTests COMMAND test_transaction_bulk)

    set_tests_properties(TransactionBulkTests PROPERTIES
        LABELS "transaction;bulk;batch;unit"
        TIMEOUT 120
    )

    message(STATUS "  Transaction Bulk: bulkPutEntities, bulkEraseEntities, batch insert/update/delete")
else()
    message(STATUS "Transaction Bulk API tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_transaction_bulk.cpp")
endif()

# Transaction Retry Manager Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_transaction_retry.cpp")
    message(STATUS "Adding Transaction Retry Manager tests")

    add_executable(test_transaction_retry
        test_transaction_retry.cpp
    )

    target_link_libraries(test_transaction_retry PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_transaction_retry PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME TransactionRetryTests COMMAND test_transaction_retry)

    set_tests_properties(TransactionRetryTests PROPERTIES
        LABELS "transaction;retry;backoff;circuit-breaker;unit"
        TIMEOUT 60
    )

    message(STATUS "  Transaction Retry: executeWithRetry, backoff, circuit breaker, error classification")
else()
    message(STATUS "Transaction Retry tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_transaction_retry.cpp")
endif()

# Per-tenant Transaction Namespace Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_tenant_transaction_namespace.cpp")
    message(STATUS "Adding Per-tenant Transaction Namespace tests")

    add_executable(test_tenant_transaction_namespace
        test_tenant_transaction_namespace.cpp
    )

    target_link_libraries(test_tenant_transaction_namespace PRIVATE
        ${TEST_LIBS}
        themis_core
        RocksDB::rocksdb
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_tenant_transaction_namespace PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME TenantTransactionNamespaceTests COMMAND test_tenant_transaction_namespace)

    set_tests_properties(TenantTransactionNamespaceTests PROPERTIES
        LABELS "transaction;tenant;namespace;unit"
        TIMEOUT 60
    )

    message(STATUS "  Tenant Transaction Namespace: per-tenant isolation, namespace resolution")
else()
    message(STATUS "Tenant Transaction Namespace tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_tenant_transaction_namespace.cpp")
endif()

# Transaction Timeout Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_transaction_timeout.cpp")
    message(STATUS "Adding Transaction Timeout tests")

    add_executable(test_transaction_timeout
        test_transaction_timeout.cpp
    )

    target_link_libraries(test_transaction_timeout PRIVATE
        ${TEST_LIBS}
        themis_core
        RocksDB::rocksdb
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_transaction_timeout PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME TransactionTimeoutTests COMMAND test_transaction_timeout)

    set_tests_properties(TransactionTimeoutTests PROPERTIES
        LABELS "transaction;timeout;auto-rollback;unit"
        TIMEOUT 120
    )

    message(STATUS "  Transaction Timeout: setTimeout, isTimedOut, auto-rollback, setDefaultTransactionTimeout")
else()
    message(STATUS "Transaction Timeout tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_transaction_timeout.cpp")
endif()

# Transaction Isolation Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_transaction_isolation.cpp")
    message(STATUS "Adding Transaction Isolation tests")

    add_executable(test_transaction_isolation
        test_transaction_isolation.cpp
    )

    target_link_libraries(test_transaction_isolation PRIVATE
        ${TEST_LIBS}
        themis_core
        RocksDB::rocksdb
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_transaction_isolation PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME TransactionIsolationTests COMMAND test_transaction_isolation)

    set_tests_properties(TransactionIsolationTests PROPERTIES
        LABELS "transaction;isolation;mvcc;unit"
        TIMEOUT 120
    )

    message(STATUS "  Transaction Isolation: ReadCommitted, Snapshot, Serializable isolation levels")
else()
    message(STATUS "Transaction Isolation tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_transaction_isolation.cpp")
endif()

# SAGA Concurrent Execution Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_saga_concurrent_execution.cpp")
    message(STATUS "Adding SAGA Concurrent Execution tests")

    add_executable(test_saga_concurrent_execution
        test_saga_concurrent_execution.cpp
        ${CMAKE_SOURCE_DIR}/src/transaction/saga.cpp
    )

    target_link_libraries(test_saga_concurrent_execution PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_saga_concurrent_execution PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME SagaConcurrentExecutionTests COMMAND test_saga_concurrent_execution)

    set_tests_properties(SagaConcurrentExecutionTests PROPERTIES
        LABELS "transaction;saga;concurrent;compensation;unit"
        TIMEOUT 120
    )

    message(STATUS "  SAGA Concurrent Execution: parallel compensations, retry, metrics")
else()
    message(STATUS "SAGA Concurrent Execution tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_saga_concurrent_execution.cpp")
endif()

# Distributed SAGA Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_distributed_saga.cpp")
    message(STATUS "Adding Distributed SAGA tests")

    add_executable(test_distributed_saga
        test_distributed_saga.cpp
        ${CMAKE_SOURCE_DIR}/src/transaction/distributed_saga.cpp
    )

    target_link_libraries(test_distributed_saga PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_distributed_saga PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME DistributedSagaTests COMMAND test_distributed_saga)

    set_tests_properties(DistributedSagaTests PROPERTIES
        LABELS "transaction;saga;distributed;compensation;unit"
        TIMEOUT 120
    )

    message(STATUS "  Distributed SAGA: DAG execution, compensation ordering, retry, step timeouts, metrics")
else()
    message(STATUS "Distributed SAGA tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_distributed_saga.cpp")
endif()

# SagaOperation Tests (indexPutWithCompensation, graphAddWithCompensation, etc.)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_saga_operation.cpp")
    message(STATUS "Adding SagaOperation tests")

    add_executable(test_saga_operation
        test_saga_operation.cpp
        ${CMAKE_SOURCE_DIR}/src/transaction/saga.cpp
    )

    target_link_libraries(test_saga_operation PRIVATE
        ${TEST_LIBS}
        themis_core
        RocksDB::rocksdb
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_saga_operation PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME SagaOperationTests COMMAND test_saga_operation)

    set_tests_properties(SagaOperationTests PROPERTIES
        LABELS "transaction;saga;compensation;secondary-index;graph;vector;unit"
        TIMEOUT 120
    )

    message(STATUS "  SagaOperation: indexPutWithCompensation, graphAddWithCompensation, putEntityWithCompensation, deleteEntityWithCompensation, vectorAddWithCompensation")
else()
    message(STATUS "SagaOperation tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_saga_operation.cpp")
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

# Governance – OPA adapter focused tests (Issue: #1768 / #1778)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_governance_opa_adapter.cpp")
    message(STATUS "Adding Governance OPA Adapter focused tests (Issue #1768 / #1778)")
    add_executable(test_governance_opa_adapter test_governance_opa_adapter.cpp)
    target_link_libraries(test_governance_opa_adapter PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::Crypto
        Threads::Threads
    )
    target_compile_definitions(test_governance_opa_adapter PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME GovernanceOpaAdapterFocusedTests COMMAND test_governance_opa_adapter)
    set_tests_properties(GovernanceOpaAdapterFocusedTests PROPERTIES
        LABELS "governance;opa;policy-evaluator;phase3;unit"
        TIMEOUT 60
    )
    message(STATUS "  Governance OPA Adapter: construction, URL building, response parsing, fallback counter, CCPA enforcement on top of OPA")
endif()

# Governance – policy simulation / dry-run focused tests (Issue: #1766 / #1778)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_governance_policy_simulation.cpp")
    message(STATUS "Adding Governance Policy Simulation focused tests (Issue #1766 / #1778)")
    add_executable(test_governance_policy_simulation test_governance_policy_simulation.cpp)
    target_link_libraries(test_governance_policy_simulation PRIVATE
        ${TEST_LIBS}
        themis_core
        yaml-cpp::yaml-cpp
        OpenSSL::Crypto
        Threads::Threads
    )
    target_compile_definitions(test_governance_policy_simulation PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME GovernancePolicySimulationFocusedTests COMMAND test_governance_policy_simulation)
    set_tests_properties(GovernancePolicySimulationFocusedTests PROPERTIES
        LABELS "governance;policy;simulation;dry-run;phase3;unit"
        TIMEOUT 60
    )
    message(STATUS "  Governance Policy Simulation: simulateDecision, YAML-loaded profiles, heuristic fallback, header overrides")
endif()

# Governance – compliance time window focused tests (Issue: #1781 / #1778)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_governance_compliance_time_window.cpp")
    message(STATUS "Adding Governance Compliance Time-Window focused tests (Issue #1781 / #1778)")
    add_executable(test_governance_compliance_time_window test_governance_compliance_time_window.cpp)
    target_link_libraries(test_governance_compliance_time_window PRIVATE
        ${TEST_LIBS}
        themis_core
        nlohmann_json::nlohmann_json
        Threads::Threads
    )
    target_compile_definitions(test_governance_compliance_time_window PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME GovernanceComplianceTimeWindowFocusedTests COMMAND test_governance_compliance_time_window)
    set_tests_properties(GovernanceComplianceTimeWindowFocusedTests PROPERTIES
        LABELS "governance;compliance;reporter;time-window;phase2;unit"
        TIMEOUT 60
    )
    message(STATUS "  Governance Compliance Time-Window: generateReport, CCPA report, time-range filtering")
endif()

# Governance – review scheduler focused tests (Issue: #1778)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_governance_review_scheduler.cpp")
    message(STATUS "Adding Governance Review Scheduler focused tests (Issue #1778)")
    add_executable(test_governance_review_scheduler test_governance_review_scheduler.cpp)
    target_link_libraries(test_governance_review_scheduler PRIVATE
        ${TEST_LIBS}
        themis_core
        nlohmann_json::nlohmann_json
        Threads::Threads
    )
    target_compile_definitions(test_governance_review_scheduler PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME GovernanceReviewSchedulerFocusedTests COMMAND test_governance_review_scheduler)
    set_tests_properties(GovernanceReviewSchedulerFocusedTests PROPERTIES
        LABELS "governance;review-scheduler;policy-review;phase4;unit"
        TIMEOUT 60
    )
    message(STATUS "  Governance Review Scheduler: schedule, create/approve/reject review, overdue detection, history, export/import")
endif()

# Governance – AI/ML model governance focused tests (Issue: #1771 / #1778)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_model_governance.cpp")
    message(STATUS "Adding Model Governance focused tests (Issue #1771 / #1778)")
    add_executable(test_model_governance_focused test_model_governance.cpp)
    target_link_libraries(test_model_governance_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        nlohmann_json::nlohmann_json
        OpenSSL::Crypto
        Threads::Threads
    )
    target_compile_definitions(test_model_governance_focused PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME ModelGovernanceFocusedTests COMMAND test_model_governance_focused)
    set_tests_properties(ModelGovernanceFocusedTests PROPERTIES
        LABELS "governance;model-governance;training-lineage;bias-audit;phase4;unit"
        TIMEOUT 60
    )
    message(STATUS "  Model Governance: training export request, bias audit, data lineage, checkExportAllowed")
endif()

# Governance – compliance + security integration focused tests (Issue: #1778)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_compliance_security_governance.cpp")
    message(STATUS "Adding Compliance & Security Governance integration tests (Issue #1778)")
    add_executable(test_compliance_security_governance_focused test_compliance_security_governance.cpp)
    target_link_libraries(test_compliance_security_governance_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::Crypto
        Threads::Threads
    )
    target_compile_definitions(test_compliance_security_governance_focused PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME ComplianceSecurityGovernanceFocusedTests COMMAND test_compliance_security_governance_focused)
    set_tests_properties(ComplianceSecurityGovernanceFocusedTests PROPERTIES
        LABELS "governance;compliance;security;gdpr;hipaa;soc2;pci-dss;ccpa;unit"
        TIMEOUT 90
    )
    message(STATUS "  Compliance & Security Governance: GDPR, HIPAA, SOC 2, PCI-DSS, CCPA/CPRA cross-compliance scenarios")
endif()

# Governance – HTTP server governance endpoint focused tests (Issue: #1778)
if(THEMIS_ENABLE_HTTP_SERVER AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_http_governance.cpp")
    message(STATUS "Adding HTTP Governance endpoint focused tests (Issue #1778)")
    add_executable(test_http_governance_focused test_http_governance.cpp)
    target_link_libraries(test_http_governance_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )
    target_compile_definitions(test_http_governance_focused PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME HttpGovernanceFocusedTests COMMAND test_http_governance_focused)
    set_tests_properties(HttpGovernanceFocusedTests PROPERTIES
        LABELS "governance;http;server;policy-engine;integration"
        TIMEOUT 120
    )
    message(STATUS "  HTTP Governance: policy evaluation via HTTP, governance endpoints, access-control enforcement")
endif()

# Governance – HTTP changefeed + governance integration focused tests (Issue: #1778)
if(THEMIS_ENABLE_HTTP_SERVER AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_http_changefeed_governance.cpp")
    message(STATUS "Adding HTTP Changefeed Governance focused tests (Issue #1778)")
    add_executable(test_http_changefeed_governance_focused test_http_changefeed_governance.cpp)
    target_link_libraries(test_http_changefeed_governance_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )
    target_compile_definitions(test_http_changefeed_governance_focused PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME HttpChangefeedGovernanceFocusedTests COMMAND test_http_changefeed_governance_focused)
    set_tests_properties(HttpChangefeedGovernanceFocusedTests PROPERTIES
        LABELS "governance;http;changefeed;cdc;integration"
        TIMEOUT 120
    )
    message(STATUS "  HTTP Changefeed Governance: changefeed governance enforcement, policy gating on CDC streams")
endif()

# Governance – compliance reporting focused tests (Issue: #1781 / #1778)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_compliance_reporting.cpp")
    message(STATUS "Adding Compliance Reporting focused tests (Issue #1781 / #1778)")
    add_executable(test_compliance_reporting_focused test_compliance_reporting.cpp)
    target_link_libraries(test_compliance_reporting_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        nlohmann_json::nlohmann_json
        Threads::Threads
    )
    target_compile_definitions(test_compliance_reporting_focused PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME ComplianceReportingFocusedTests COMMAND test_compliance_reporting_focused)
    set_tests_properties(ComplianceReportingFocusedTests PROPERTIES
        LABELS "governance;compliance;reporting;coverage;gap-detection;pdf;json;html;unit"
        TIMEOUT 90
    )
    message(STATUS "  Compliance Reporting: CoverageAnalyzer, ComplianceGapDetector, ComplianceReporter (JSON/CSV/HTML/PDF)")
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
# Note: test_distributed_vector_index is already defined as a standalone target
# in cmake/CMakeLists.txt; defining it here would create a duplicate-target error.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/index/test_distributed_vector_index.cpp")
    message(STATUS "  DistributedVectorIndex tests are built via cmake/CMakeLists.txt")
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

# HNSW recall@10 integration tests (Production Readiness Checklist)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/index/test_hnsw_recall_integration.cpp")
    message(STATUS "Adding HNSW recall@10 integration tests")

    add_executable(test_hnsw_recall_integration
        index/test_hnsw_recall_integration.cpp
    )

    target_include_directories(test_hnsw_recall_integration PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_hnsw_recall_integration PRIVATE
        ${TEST_LIBS}
        Threads::Threads
    )

    if(TARGET hnswlib::hnswlib)
        target_link_libraries(test_hnsw_recall_integration PRIVATE hnswlib::hnswlib)
    endif()

    target_compile_definitions(test_hnsw_recall_integration PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME HnswRecallIntegrationTests COMMAND test_hnsw_recall_integration)

    set_tests_properties(HnswRecallIntegrationTests PROPERTIES
        LABELS "index;hnsw;recall;integration"
        TIMEOUT 120
    )

    message(STATUS "  HnswRecall: recall@10 >= 0.90 for L2, Cosine, Dot Product; ef sensitivity; brute-force baseline")
else()
    message(STATUS "HNSW recall@10 integration tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/index/test_hnsw_recall_integration.cpp")
endif()

# Spatial index correctness integration tests (Production Readiness Checklist)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/index/test_spatial_correctness_integration.cpp")
    message(STATUS "Adding Spatial correctness integration tests")

    add_executable(test_spatial_correctness_integration
        index/test_spatial_correctness_integration.cpp
        ${CMAKE_SOURCE_DIR}/src/index/spatial_index.cpp
    )

    target_include_directories(test_spatial_correctness_integration PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_spatial_correctness_integration PRIVATE
        ${TEST_LIBS}
        themis_core
        RocksDB::rocksdb
        Threads::Threads
    )

    target_compile_definitions(test_spatial_correctness_integration PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME SpatialCorrectnessIntegrationTests COMMAND test_spatial_correctness_integration)

    set_tests_properties(SpatialCorrectnessIntegrationTests PROPERTIES
        LABELS "index;spatial;correctness;integration"
        TIMEOUT 120
    )

    message(STATUS "  SpatialCorrectness: intersects, within, contains, nearby, KNN, Z-range, precision, recall")
else()
    message(STATUS "Spatial correctness integration tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/index/test_spatial_correctness_integration.cpp")
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

# Focused Streaming Window Tests (fast debug target)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analytics/test_streaming_window.cpp")
    message(STATUS "Adding focused streaming window tests")

    add_executable(test_streaming_window_focused
        analytics/test_streaming_window.cpp
        ${CMAKE_SOURCE_DIR}/src/analytics/streaming_window.cpp
    )

    target_include_directories(test_streaming_window_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_streaming_window_focused PRIVATE
        ${TEST_LIBS}
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_streaming_window_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME StreamingWindowFocusedTests COMMAND test_streaming_window_focused)

    set_tests_properties(StreamingWindowFocusedTests PROPERTIES
        LABELS "analytics;streaming;window;focused;unit"
        TIMEOUT 120
    )
endif()

# ============================================================================
# Analytics Focused Test Targets
# Each target links only the analytics source(s) it exercises, enabling fast
# iteration without recompiling the full themis_core library.
# ============================================================================

# Anomaly Detection focused tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analytics/test_anomaly_detection.cpp")
    message(STATUS "Adding focused anomaly detection tests")

    add_executable(test_anomaly_detection_focused
        analytics/test_anomaly_detection.cpp
        ${CMAKE_SOURCE_DIR}/src/analytics/anomaly_detection.cpp
    )

    target_include_directories(test_anomaly_detection_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_anomaly_detection_focused PRIVATE
        ${TEST_LIBS}
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_anomaly_detection_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME AnomalyDetectionFocusedTests COMMAND test_anomaly_detection_focused)

    set_tests_properties(AnomalyDetectionFocusedTests PROPERTIES
        LABELS "analytics;anomaly;focused;unit"
        TIMEOUT 120
    )

    message(STATUS "  AnomalyDetection: Z-Score, MAD, IQR, IsolationForest, LOF, Ensemble, streaming, serialize")
endif()

# AutoML focused tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analytics/test_automl.cpp")
    message(STATUS "Adding focused AutoML tests")

    add_executable(test_automl_focused
        analytics/test_automl.cpp
        ${CMAKE_SOURCE_DIR}/src/analytics/automl.cpp
        ${CMAKE_SOURCE_DIR}/src/analytics/anomaly_detection.cpp
    )

    target_include_directories(test_automl_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_automl_focused PRIVATE
        ${TEST_LIBS}
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_automl_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME AutoMlFocusedTests COMMAND test_automl_focused)

    set_tests_properties(AutoMlFocusedTests PROPERTIES
        LABELS "analytics;automl;ml;focused;unit"
        TIMEOUT 180
    )

    message(STATUS "  AutoML: classification, regression, feature engineering, ensemble, SHAP, serialize")
endif()

# Distributed Analytics focused tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analytics/test_distributed_analytics.cpp")
    message(STATUS "Adding focused distributed analytics tests")

    add_executable(test_distributed_analytics_focused
        analytics/test_distributed_analytics.cpp
        ${CMAKE_SOURCE_DIR}/src/analytics/distributed_analytics.cpp
        ${CMAKE_SOURCE_DIR}/src/analytics/olap.cpp
    )

    target_include_directories(test_distributed_analytics_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_distributed_analytics_focused PRIVATE
        ${TEST_LIBS}
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_distributed_analytics_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME DistributedAnalyticsFocusedTests COMMAND test_distributed_analytics_focused)

    set_tests_properties(DistributedAnalyticsFocusedTests PROPERTIES
        LABELS "analytics;distributed;olap;focused;unit"
        TIMEOUT 120
    )

    message(STATUS "  DistributedAnalytics: shard management, scatter-gather, merge, partial failure")
endif()

# Process Pattern Matcher focused tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analytics/test_process_pattern_matcher.cpp")
    message(STATUS "Adding focused process pattern matcher tests")

    add_executable(test_process_pattern_matcher_focused
        analytics/test_process_pattern_matcher.cpp
        ${CMAKE_SOURCE_DIR}/src/analytics/process_pattern_matcher.cpp
        ${CMAKE_SOURCE_DIR}/src/analytics/process_mining.cpp
    )

    target_include_directories(test_process_pattern_matcher_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_process_pattern_matcher_focused PRIVATE
        ${TEST_LIBS}
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_process_pattern_matcher_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ProcessPatternMatcherFocusedTests COMMAND test_process_pattern_matcher_focused)

    set_tests_properties(ProcessPatternMatcherFocusedTests PROPERTIES
        LABELS "analytics;process-mining;pattern-matcher;focused;unit"
        TIMEOUT 120
    )

    message(STATUS "  ProcessPatternMatcher: graph, vector, behavioral, hybrid similarity, conformance")
endif()

# Forecasting focused tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analytics/test_forecasting.cpp")
    message(STATUS "Adding focused forecasting tests")

    add_executable(test_forecasting_focused
        analytics/test_forecasting.cpp
        ${CMAKE_SOURCE_DIR}/src/analytics/forecasting.cpp
    )

    target_include_directories(test_forecasting_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_forecasting_focused PRIVATE
        ${TEST_LIBS}
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_forecasting_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ForecastingFocusedTests COMMAND test_forecasting_focused)

    set_tests_properties(ForecastingFocusedTests PROPERTIES
        LABELS "analytics;forecasting;timeseries;focused;unit"
        TIMEOUT 180
    )

    message(STATUS "  Forecasting: LINEAR_REGRESSION, EXP_SMOOTHING, Holt-Winters, ARIMA, ENSEMBLE, serialize")
endif()

# Incremental View focused tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analytics/test_incremental_view.cpp")
    message(STATUS "Adding focused incremental view tests")

    add_executable(test_incremental_view_focused
        analytics/test_incremental_view.cpp
        ${CMAKE_SOURCE_DIR}/src/analytics/incremental_view.cpp
        ${CMAKE_SOURCE_DIR}/src/analytics/olap.cpp
    )

    target_include_directories(test_incremental_view_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_incremental_view_focused PRIVATE
        ${TEST_LIBS}
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_incremental_view_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME IncrementalViewFocusedTests COMMAND test_incremental_view_focused)

    set_tests_properties(IncrementalViewFocusedTests PROPERTIES
        LABELS "analytics;incremental-view;ivm;focused;unit"
        TIMEOUT 120
    )

    message(STATUS "  IncrementalView: delta maintenance, 10 aggregation functions, Welford STDDEV, COUNT_DISTINCT")
endif()

# Columnar Execution focused tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analytics/test_columnar_execution.cpp")
    message(STATUS "Adding focused columnar execution tests")

    add_executable(test_columnar_execution_focused
        analytics/test_columnar_execution.cpp
        ${CMAKE_SOURCE_DIR}/src/analytics/columnar_execution.cpp
    )

    target_include_directories(test_columnar_execution_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_columnar_execution_focused PRIVATE
        ${TEST_LIBS}
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_columnar_execution_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ColumnarExecutionFocusedTests COMMAND test_columnar_execution_focused)

    set_tests_properties(ColumnarExecutionFocusedTests PROPERTIES
        LABELS "analytics;columnar;vectorized;focused;unit"
        TIMEOUT 120
    )

    message(STATUS "  ColumnarExecution: vectorized pipeline, SIMD aggregations, filter+project operators")
endif()

# JIT Aggregation focused tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analytics/test_jit_aggregation.cpp")
    message(STATUS "Adding focused JIT aggregation tests")

    add_executable(test_jit_aggregation_focused
        analytics/test_jit_aggregation.cpp
        ${CMAKE_SOURCE_DIR}/src/analytics/jit_aggregation.cpp
    )

    target_include_directories(test_jit_aggregation_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_jit_aggregation_focused PRIVATE
        ${TEST_LIBS}
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_jit_aggregation_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME JitAggregationFocusedTests COMMAND test_jit_aggregation_focused)

    set_tests_properties(JitAggregationFocusedTests PROPERTIES
        LABELS "analytics;jit;aggregation;focused;unit"
        TIMEOUT 120
    )

    message(STATUS "  JITAggregation: hot-path detection, template dispatch, LLVM-MCJIT backend guard")
endif()

# ML Serving focused tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analytics/test_ml_serving.cpp")
    message(STATUS "Adding focused ML serving tests")

    add_executable(test_ml_serving_focused
        analytics/test_ml_serving.cpp
        ${CMAKE_SOURCE_DIR}/src/analytics/ml_serving.cpp
        ${CMAKE_SOURCE_DIR}/src/analytics/anomaly_detection.cpp
    )

    target_include_directories(test_ml_serving_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_ml_serving_focused PRIVATE
        ${TEST_LIBS}
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_ml_serving_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME MlServingFocusedTests COMMAND test_ml_serving_focused)

    set_tests_properties(MlServingFocusedTests PROPERTIES
        LABELS "analytics;ml;serving;onnx;focused;unit"
        TIMEOUT 120
    )

    message(STATUS "  MLServing: ONNX/TF-Serving unified client, DataPoint integration, graceful degradation")
endif()

# Model Serving focused tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analytics/test_model_serving.cpp")
    message(STATUS "Adding focused model serving tests")

    add_executable(test_model_serving_focused
        analytics/test_model_serving.cpp
        ${CMAKE_SOURCE_DIR}/src/analytics/model_serving.cpp
        ${CMAKE_SOURCE_DIR}/src/analytics/anomaly_detection.cpp
    )

    target_include_directories(test_model_serving_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_model_serving_focused PRIVATE
        ${TEST_LIBS}
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_model_serving_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ModelServingFocusedTests COMMAND test_model_serving_focused)

    set_tests_properties(ModelServingFocusedTests PROPERTIES
        LABELS "analytics;model;serving;inference;focused;unit"
        TIMEOUT 120
    )

    message(STATUS "  ModelServing: named+versioned registry, online/batch inference, health metrics, serialize")
endif()

# Arrow Flight focused tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analytics/test_arrow_flight.cpp")
    message(STATUS "Adding focused Arrow Flight tests")

    add_executable(test_arrow_flight_focused
        analytics/test_arrow_flight.cpp
        ${CMAKE_SOURCE_DIR}/src/analytics/arrow_flight.cpp
    )

    target_include_directories(test_arrow_flight_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_arrow_flight_focused PRIVATE
        ${TEST_LIBS}
        spdlog::spdlog
        Threads::Threads
    )

    if(THEMIS_HAS_ARROW)
        target_link_libraries(test_arrow_flight_focused PRIVATE ${THEMIS_ARROW_TARGET})
        target_compile_definitions(test_arrow_flight_focused PRIVATE THEMIS_HAS_ARROW=1)
    endif()

    if(THEMIS_HAS_ARROW_FLIGHT)
        target_compile_definitions(test_arrow_flight_focused PRIVATE THEMIS_HAS_ARROW_FLIGHT=1)
    endif()

    target_compile_definitions(test_arrow_flight_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ArrowFlightFocusedTests COMMAND test_arrow_flight_focused)

    set_tests_properties(ArrowFlightFocusedTests PROPERTIES
        LABELS "analytics;arrow;flight;rpc;focused;unit"
        TIMEOUT 120
    )

    message(STATUS "  ArrowFlight: in-process registry, FlightDescriptor, put/get, client round-trip")
endif()

# CEP Engine focused tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analytics/test_cep_engine.cpp")
    message(STATUS "Adding focused CEP engine tests")

    add_executable(test_cep_engine_focused
        analytics/test_cep_engine.cpp
        ${CMAKE_SOURCE_DIR}/src/analytics/cep_engine.cpp
    )

    target_include_directories(test_cep_engine_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_cep_engine_focused PRIVATE
        ${TEST_LIBS}
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_cep_engine_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME CepEngineFocusedTests COMMAND test_cep_engine_focused)

    set_tests_properties(CepEngineFocusedTests PROPERTIES
        LABELS "analytics;cep;event-processing;focused;unit"
        TIMEOUT 180
    )

    message(STATUS "  CEPEngine: NFA patterns, EPL parser, window+aggregation, alerts, checkpoint I/O")
endif()

# Arrow Export focused tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analytics/test_arrow_export.cpp")
    message(STATUS "Adding focused Arrow export tests")

    add_executable(test_arrow_export_focused
        analytics/test_arrow_export.cpp
        ${CMAKE_SOURCE_DIR}/src/analytics/arrow_export.cpp
        ${CMAKE_SOURCE_DIR}/src/analytics/analytics_export.cpp
    )

    target_include_directories(test_arrow_export_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_arrow_export_focused PRIVATE
        ${TEST_LIBS}
        spdlog::spdlog
        Threads::Threads
    )

    if(THEMIS_HAS_ARROW)
        target_link_libraries(test_arrow_export_focused PRIVATE ${THEMIS_ARROW_TARGET})
        target_compile_definitions(test_arrow_export_focused PRIVATE THEMIS_HAS_ARROW=1)
    endif()

    if(THEMIS_HAS_PARQUET)
        target_compile_definitions(test_arrow_export_focused PRIVATE THEMIS_HAS_PARQUET=1)
    endif()

    target_compile_definitions(test_arrow_export_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ArrowExportFocusedTests COMMAND test_arrow_export_focused)

    set_tests_properties(ArrowExportFocusedTests PROPERTIES
        LABELS "analytics;arrow;export;parquet;focused;unit"
        TIMEOUT 120
    )

    message(STATUS "  ArrowExport: RecordBatch, JSON/CSV export, optional Parquet/Feather/IPC, sanitization")
endif()

# Process Mining pattern-focused tests (file is test_process_mining_llm.cpp but exercises
# process_pattern_matcher + process_mining only; has no actual LLM library dependency)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analytics/test_process_mining_llm.cpp")
    message(STATUS "Adding focused process mining pattern tests")

    add_executable(test_process_mining_pattern_focused
        analytics/test_process_mining_llm.cpp
        ${CMAKE_SOURCE_DIR}/src/analytics/process_pattern_matcher.cpp
        ${CMAKE_SOURCE_DIR}/src/analytics/process_mining.cpp
    )

    target_include_directories(test_process_mining_pattern_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_process_mining_pattern_focused PRIVATE
        ${TEST_LIBS}
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_process_mining_pattern_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ProcessMiningPatternFocusedTests COMMAND test_process_mining_pattern_focused)

    set_tests_properties(ProcessMiningPatternFocusedTests PROPERTIES
        LABELS "analytics;process-mining;pattern;conformance;focused;unit"
        TIMEOUT 120
    )

    message(STATUS "  ProcessMiningPattern: conformance, compliance, fraud detection, activity prediction")
endif()

# ============================================================================
# Graph Module Focused Test Targets
# Each target links only the graph source(s) it exercises, enabling fast
# iteration without recompiling the full themis_core library.
# ============================================================================

# Graph Query EXPLAIN focused tests (Issue #1816)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/graph/test_query_explain.cpp")
    message(STATUS "Adding focused graph query EXPLAIN tests (Issue #1816)")

    add_executable(test_graph_query_explain_focused
        graph/test_query_explain.cpp
        ${CMAKE_SOURCE_DIR}/src/graph/graph_query_optimizer.cpp
        ${CMAKE_SOURCE_DIR}/src/graph/path_constraints.cpp
        ${CMAKE_SOURCE_DIR}/src/graph/parallel_traversal.cpp
        ${CMAKE_SOURCE_DIR}/src/graph/distributed_graph.cpp
    )

    target_include_directories(test_graph_query_explain_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_graph_query_explain_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        fmt::fmt
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )

    target_compile_definitions(test_graph_query_explain_focused PRIVATE
        THEMIS_TEST_BUILD=1
        THEMIS_GRAPH_EXPORTS
    )

    add_test(NAME GraphQueryExplainFocusedTests COMMAND test_graph_query_explain_focused)

    set_tests_properties(GraphQueryExplainFocusedTests PROPERTIES
        LABELS "graph;query;explain;optimizer;plan;focused;unit"
        TIMEOUT 120
    )

    message(STATUS "  GraphQueryExplain: explainPlan, explainConstrainedPath, dry-run, structural reuse, all query types")
endif()

# ============================================================================
# Importer Plugin API Tests (standalone – avoids createPlugin/destroyPlugin
# symbol collision with postgres_importer.cpp in themis_core)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_importer_plugin_api.cpp")
    message(STATUS "Adding Importer Plugin API tests (Issue #1854, standalone binary)")

    add_executable(test_importer_plugin_api
        test_importer_plugin_api.cpp
    )

    target_include_directories(test_importer_plugin_api PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_importer_plugin_api PRIVATE
        ${TEST_LIBS}
        nlohmann_json::nlohmann_json
        Threads::Threads
        ${CMAKE_DL_LIBS}
    )

    target_compile_definitions(test_importer_plugin_api PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ImporterPluginApiTests COMMAND test_importer_plugin_api)

    set_tests_properties(ImporterPluginApiTests PROPERTIES
        LABELS "importers;plugin;plugin-api;unit"
        TIMEOUT 60
    )

    message(STATUS "  ImporterPluginApi: registry, loader, macro, descriptor, thread safety")
else()
    message(STATUS "Importer Plugin API tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_importer_plugin_api.cpp")
endif()

# ============================================================================
# Importer Abstract Interfaces Tests (standalone target)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_importer_interfaces.cpp")
    message(STATUS "Adding Importer Interfaces tests (standalone binary)")

    add_executable(test_importer_interfaces
        test_importer_interfaces.cpp
    )

    target_include_directories(test_importer_interfaces PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_importer_interfaces PRIVATE
        ${TEST_LIBS}
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_importer_interfaces PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ImporterInterfacesTests COMMAND test_importer_interfaces)

    set_tests_properties(ImporterInterfacesTests PROPERTIES
        LABELS "importers;interfaces;unit"
        TIMEOUT 60
    )

    message(STATUS "  ImporterInterfaces: IImportConflictResolver, IFlatFileSchemaDetector, IKafkaConsumerSource, IIncrementalImportCursor, IImporterPlugin, IImporterPluginRegistry, REGISTER_IMPORTER_PLUGIN")
else()
    message(STATUS "Importer Interfaces tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_importer_interfaces.cpp")
endif()

# ============================================================================
# Flatfile Importer Focused Tests (standalone – has own main(), excluded from
# main build via TEST_EXCLUDE_PATTERNS)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_flatfile_importer.cpp")
    message(STATUS "Adding FlatfileImporter focused tests (standalone binary)")

    add_executable(test_flatfile_importer_focused
        test_flatfile_importer.cpp
    )

    target_include_directories(test_flatfile_importer_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_flatfile_importer_focused PRIVATE
        ${TEST_LIBS}
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_flatfile_importer_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME FlatfileImporterFocusedTests COMMAND test_flatfile_importer_focused)

    set_tests_properties(FlatfileImporterFocusedTests PROPERTIES
        LABELS "importers;flatfile;csv;tsv;parquet;unit"
        TIMEOUT 120
    )

    message(STATUS "  FlatfileImporter: CSV/TSV/JSONL row parsing, schema detection, dry-run, streaming, UTF-8, metrics")
else()
    message(STATUS "FlatfileImporter focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_flatfile_importer.cpp")
endif()

# ============================================================================
# Schema Validator Importer Focused Tests (standalone – has own main(), excluded
# from main build via TEST_EXCLUDE_PATTERNS)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_schema_validator.cpp")
    message(STATUS "Adding SchemaValidatorImporter focused tests (standalone binary)")

    add_executable(test_schema_validator_importer_focused
        test_schema_validator.cpp
    )

    target_include_directories(test_schema_validator_importer_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_schema_validator_importer_focused PRIVATE
        ${TEST_LIBS}
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_schema_validator_importer_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME SchemaValidatorImporterFocusedTests COMMAND test_schema_validator_importer_focused)

    set_tests_properties(SchemaValidatorImporterFocusedTests PROPERTIES
        LABELS "importers;schema;validation;unit"
        TIMEOUT 60
    )

    message(STATUS "  SchemaValidatorImporter: field-type validation, required fields, nested schema, conflict detection")
else()
    message(STATUS "SchemaValidatorImporter focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_schema_validator.cpp")
endif()

# ============================================================================
# Importer Conflict Resolver Focused Tests (standalone binary)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_importer_conflict_resolver.cpp")
    message(STATUS "Adding ImporterConflictResolver focused tests (standalone binary)")

    add_executable(test_importer_conflict_resolver_focused
        test_importer_conflict_resolver.cpp
    )

    target_include_directories(test_importer_conflict_resolver_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_importer_conflict_resolver_focused PRIVATE
        ${TEST_LIBS}
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_importer_conflict_resolver_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ImporterConflictResolverFocusedTests COMMAND test_importer_conflict_resolver_focused)

    set_tests_properties(ImporterConflictResolverFocusedTests PROPERTIES
        LABELS "importers;conflict;resolver;unit"
        TIMEOUT 60
    )

    message(STATUS "  ImporterConflictResolver: SKIP, OVERWRITE, MERGE (depth-1 and deep), ERROR strategies")
else()
    message(STATUS "ImporterConflictResolver focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_importer_conflict_resolver.cpp")
endif()

# ============================================================================
# Importer Async API Focused Tests (standalone binary)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_importer_async_api.cpp")
    message(STATUS "Adding ImporterAsyncApi focused tests (standalone binary)")

    add_executable(test_importer_async_api_focused
        test_importer_async_api.cpp
    )

    target_include_directories(test_importer_async_api_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_importer_async_api_focused PRIVATE
        ${TEST_LIBS}
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_importer_async_api_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ImporterAsyncApiFocusedTests COMMAND test_importer_async_api_focused)

    set_tests_properties(ImporterAsyncApiFocusedTests PROPERTIES
        LABELS "importers;async;streaming;unit"
        TIMEOUT 60
    )

    message(STATUS "  ImporterAsyncApi: async import pipeline, streaming callbacks, progress reporting, cancellation")
else()
    message(STATUS "ImporterAsyncApi focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_importer_async_api.cpp")
endif()

# ============================================================================
# MySQL Importer Focused Tests (standalone binary)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_mysql_importer.cpp")
    message(STATUS "Adding MySQLImporter focused tests (standalone binary)")

    add_executable(test_mysql_importer_focused
        test_mysql_importer.cpp
    )

    target_include_directories(test_mysql_importer_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_mysql_importer_focused PRIVATE
        ${TEST_LIBS}
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_mysql_importer_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME MySQLImporterFocusedTests COMMAND test_mysql_importer_focused)

    set_tests_properties(MySQLImporterFocusedTests PROPERTIES
        LABELS "importers;mysql;mariadb;unit"
        TIMEOUT 120
    )

    message(STATUS "  MySQLImporter: type mapping, batch import, incremental, JDBC-compatible config, streaming callbacks")
else()
    message(STATUS "MySQLImporter focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_mysql_importer.cpp")
endif()

# ============================================================================
# MongoDB Importer Focused Tests (standalone binary)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_mongo_importer.cpp")
    message(STATUS "Adding MongoImporter focused tests (standalone binary)")

    add_executable(test_mongo_importer_focused
        test_mongo_importer.cpp
    )

    target_include_directories(test_mongo_importer_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_mongo_importer_focused PRIVATE
        ${TEST_LIBS}
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_mongo_importer_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME MongoImporterFocusedTests COMMAND test_mongo_importer_focused)

    set_tests_properties(MongoImporterFocusedTests PROPERTIES
        LABELS "importers;mongodb;document;unit"
        TIMEOUT 120
    )

    message(STATUS "  MongoImporter: document collection import, nested docs, BSON type mapping, collection filtering")
else()
    message(STATUS "MongoImporter focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_mongo_importer.cpp")
endif()

# ============================================================================
# SQLite Importer Focused Tests (standalone binary)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_sqlite_importer.cpp")
    message(STATUS "Adding SQLiteImporter focused tests (standalone binary)")

    add_executable(test_sqlite_importer_focused
        test_sqlite_importer.cpp
    )

    target_include_directories(test_sqlite_importer_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_sqlite_importer_focused PRIVATE
        ${TEST_LIBS}
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_sqlite_importer_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME SQLiteImporterFocusedTests COMMAND test_sqlite_importer_focused)

    set_tests_properties(SQLiteImporterFocusedTests PROPERTIES
        LABELS "importers;sqlite;unit"
        TIMEOUT 120
    )

    message(STATUS "  SQLiteImporter: table discovery, type mapping, incremental import, attached databases, streaming")
else()
    message(STATUS "SQLiteImporter focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_sqlite_importer.cpp")
endif()

# ============================================================================
# Kafka Importer Focused Tests (standalone binary)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_kafka_importer.cpp")
    message(STATUS "Adding KafkaImporter focused tests (standalone binary)")

    add_executable(test_kafka_importer_focused
        test_kafka_importer.cpp
    )

    target_include_directories(test_kafka_importer_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_kafka_importer_focused PRIVATE
        ${TEST_LIBS}
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_kafka_importer_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME KafkaImporterFocusedTests COMMAND test_kafka_importer_focused)

    set_tests_properties(KafkaImporterFocusedTests PROPERTIES
        LABELS "importers;kafka;streaming;unit"
        TIMEOUT 120
    )

    message(STATUS "  KafkaImporter: consumer configuration, offset commit, topic filtering, async ingestion, back-pressure")
else()
    message(STATUS "KafkaImporter focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_kafka_importer.cpp")
endif()

# ============================================================================
# Oracle Importer Focused Tests (standalone binary)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_oracle_importer.cpp")
    message(STATUS "Adding OracleImporter focused tests (standalone binary)")

    add_executable(test_oracle_importer_focused
        test_oracle_importer.cpp
    )

    target_include_directories(test_oracle_importer_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_oracle_importer_focused PRIVATE
        ${TEST_LIBS}
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_oracle_importer_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME OracleImporterFocusedTests COMMAND test_oracle_importer_focused)

    set_tests_properties(OracleImporterFocusedTests PROPERTIES
        LABELS "importers;oracle;unit"
        TIMEOUT 120
    )

    message(STATUS "  OracleImporter: OCI config, type mapping (NUMBER/DATE/CLOB/BLOB), schema filtering, incremental import")
else()
    message(STATUS "OracleImporter focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_oracle_importer.cpp")
endif()

# ============================================================================
# S3 Importer Focused Tests (standalone binary)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_s3_importer.cpp")
    message(STATUS "Adding S3Importer focused tests (standalone binary)")

    add_executable(test_s3_importer_focused
        test_s3_importer.cpp
    )

    target_include_directories(test_s3_importer_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_s3_importer_focused PRIVATE
        ${TEST_LIBS}
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_s3_importer_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME S3ImporterFocusedTests COMMAND test_s3_importer_focused)

    set_tests_properties(S3ImporterFocusedTests PROPERTIES
        LABELS "importers;s3;object-storage;unit"
        TIMEOUT 120
    )

    message(STATUS "  S3Importer: bucket/prefix listing, object download, multipart, credential config, format auto-detect")
else()
    message(STATUS "S3Importer focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_s3_importer.cpp")
endif()

# ============================================================================
# PostgreSQL Importer Focused Tests (standalone binary – combines all
# postgres_importer test variants: advanced, chaos, complex_ddl, datatypes,
# integration, robustness, security_audit, streaming)
# ============================================================================

set(_postgres_importer_test_sources "")
foreach(_pg_test
        test_postgres_importer_advanced.cpp
        test_postgres_importer_chaos.cpp
        test_postgres_importer_complex_ddl.cpp
        test_postgres_importer_datatypes.cpp
        test_postgres_importer_integration.cpp
        test_postgres_importer_live.cpp
        test_postgres_importer_robustness.cpp
        test_postgres_importer_security_audit.cpp
        test_postgres_importer_streaming.cpp)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_pg_test}")
        list(APPEND _postgres_importer_test_sources "${_pg_test}")
    endif()
endforeach()

if(_postgres_importer_test_sources)
    message(STATUS "Adding PostgresImporter focused tests (${_postgres_importer_test_sources})")

    add_executable(test_postgres_importer_focused
        ${_postgres_importer_test_sources}
    )

    target_include_directories(test_postgres_importer_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_postgres_importer_focused PRIVATE
        ${TEST_LIBS}
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_postgres_importer_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME PostgresImporterFocusedTests COMMAND test_postgres_importer_focused)

    set_tests_properties(PostgresImporterFocusedTests PROPERTIES
        LABELS "importers;postgres;postgresql;unit"
        TIMEOUT 300
    )

    message(STATUS "  PostgresImporter: advanced mapping, chaos/retry, complex DDL, data types, integration, robustness, security audit, streaming")
else()
    message(STATUS "PostgresImporter focused test sources not found")
endif()

# ============================================================================
# Import Wizard Focused Tests (standalone binary)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_import_wizard.cpp")
    message(STATUS "Adding ImportWizard focused tests (standalone binary)")

    add_executable(test_import_wizard_focused
        test_import_wizard.cpp
    )

    target_include_directories(test_import_wizard_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_import_wizard_focused PRIVATE
        ${TEST_LIBS}
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_import_wizard_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ImportWizardFocusedTests COMMAND test_import_wizard_focused)

    set_tests_properties(ImportWizardFocusedTests PROPERTIES
        LABELS "importers;wizard;gui;unit"
        TIMEOUT 60
    )

    message(STATUS "  ImportWizard: step-by-step configuration, source type selection, schema mapping, dry-run preview")
else()
    message(STATUS "ImportWizard focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_import_wizard.cpp")
endif()

# ============================================================================
# Plugin Capability Negotiation Tests (standalone target for Issue #1984)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_capability_negotiation.cpp")
    message(STATUS "Adding Plugin Capability Negotiation tests (Issue #1984, standalone binary)")

    add_executable(test_plugin_capability_negotiation
        test_plugin_capability_negotiation.cpp
    )

    target_include_directories(test_plugin_capability_negotiation PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_plugin_capability_negotiation PRIVATE
        ${TEST_LIBS}
        themis_core
        nlohmann_json::nlohmann_json
        Threads::Threads
        ${CMAKE_DL_LIBS}
    )

    target_compile_definitions(test_plugin_capability_negotiation PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME PluginCapabilityNegotiationTests COMMAND test_plugin_capability_negotiation)

    set_tests_properties(PluginCapabilityNegotiationTests PROPERTIES
        LABELS "plugins;capability;negotiation;version-range;unit"
        TIMEOUT 60
    )

    message(STATUS "  PluginCapabilityNegotiation: PluginVersionRange, PluginCapabilityNegotiator, PluginManager::negotiateCapabilities")
else()
    message(STATUS "Plugin Capability Negotiation tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_capability_negotiation.cpp")
endif()

# ============================================================================
# Plugin Module Focused Test Targets
# Standalone binaries enabling fast, isolated iteration over each plugin
# subsystem without requiring a full themis_tests build.
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_manager.cpp")
    message(STATUS "Adding Plugin Manager focused tests")

    add_executable(test_plugin_manager_focused
        test_plugin_manager.cpp
    )

    target_include_directories(test_plugin_manager_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_plugin_manager_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        nlohmann_json::nlohmann_json
        Threads::Threads
        ${CMAKE_DL_LIBS}
    )

    target_compile_definitions(test_plugin_manager_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME PluginManagerFocusedTests COMMAND test_plugin_manager_focused)

    set_tests_properties(PluginManagerFocusedTests PROPERTIES
        LABELS "plugins;manager;lifecycle;singleton;focused;unit"
        TIMEOUT 60
    )

    message(STATUS "  PluginManager: singleton identity, scanPluginDirectory, autoLoad, hot-plug enable/disable, negotiateCapabilities")
else()
    message(STATUS "Plugin Manager focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_manager.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_lifecycle.cpp")
    message(STATUS "Adding Plugin Lifecycle focused tests")

    add_executable(test_plugin_lifecycle_focused
        test_plugin_lifecycle.cpp
    )

    target_include_directories(test_plugin_lifecycle_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_plugin_lifecycle_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        nlohmann_json::nlohmann_json
        Threads::Threads
        ${CMAKE_DL_LIBS}
    )

    target_compile_definitions(test_plugin_lifecycle_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME PluginLifecycleFocusedTests COMMAND test_plugin_lifecycle_focused)

    set_tests_properties(PluginLifecycleFocusedTests PROPERTIES
        LABELS "plugins;lifecycle;load;unload;hot-reload;focused;unit"
        TIMEOUT 60
    )

    message(STATUS "  PluginLifecycle: load/initialize/unload, hot-reload, dependency-ordered loading, RAII guards")
else()
    message(STATUS "Plugin Lifecycle focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_lifecycle.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_health_monitor.cpp")
    message(STATUS "Adding Plugin Health Monitor focused tests")

    add_executable(test_plugin_health_monitor_focused
        test_plugin_health_monitor.cpp
    )

    target_include_directories(test_plugin_health_monitor_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_plugin_health_monitor_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_plugin_health_monitor_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME PluginHealthMonitorFocusedTests COMMAND test_plugin_health_monitor_focused)

    set_tests_properties(PluginHealthMonitorFocusedTests PROPERTIES
        LABELS "plugins;health;liveness;auto-restart;focused;unit"
        TIMEOUT 60
    )

    message(STATUS "  PluginHealthMonitor: liveness probes, consecutive-failure threshold, automatic restart")
else()
    message(STATUS "Plugin Health Monitor focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_health_monitor.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_hot_plug.cpp")
    message(STATUS "Adding Plugin Hot-Plug Monitor focused tests")

    add_executable(test_plugin_hot_plug_focused
        test_plugin_hot_plug.cpp
    )

    target_include_directories(test_plugin_hot_plug_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_plugin_hot_plug_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
        ${CMAKE_DL_LIBS}
    )

    target_compile_definitions(test_plugin_hot_plug_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME PluginHotPlugFocusedTests COMMAND test_plugin_hot_plug_focused)

    set_tests_properties(PluginHotPlugFocusedTests PROPERTIES
        LABELS "plugins;hot-plug;directory-watcher;reload;focused;unit"
        TIMEOUT 60
    )

    message(STATUS "  PluginHotPlug: directory watcher, new/updated plugin detection, reload trigger")
else()
    message(STATUS "Plugin Hot-Plug focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_hot_plug.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_hot_reload_enhanced.cpp")
    message(STATUS "Adding Plugin Hot-Reload Enhanced focused tests")

    add_executable(test_plugin_hot_reload_enhanced_focused
        test_plugin_hot_reload_enhanced.cpp
    )

    target_include_directories(test_plugin_hot_reload_enhanced_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_plugin_hot_reload_enhanced_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
        ${CMAKE_DL_LIBS}
    )

    target_compile_definitions(test_plugin_hot_reload_enhanced_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME PluginHotReloadEnhancedFocusedTests COMMAND test_plugin_hot_reload_enhanced_focused)

    set_tests_properties(PluginHotReloadEnhancedFocusedTests PROPERTIES
        LABELS "plugins;hot-reload;toctou;state-preserve;rollback;focused;unit"
        TIMEOUT 90
    )

    message(STATUS "  PluginHotReloadEnhanced: TOCTOU-safe 3-phase reload, state preservation, atomic swap, rollback")
else()
    message(STATUS "Plugin Hot-Reload Enhanced focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_hot_reload_enhanced.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_dependency_graph.cpp")
    message(STATUS "Adding Plugin Dependency Graph focused tests")

    add_executable(test_plugin_dependency_graph_focused
        test_plugin_dependency_graph.cpp
    )

    target_include_directories(test_plugin_dependency_graph_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_plugin_dependency_graph_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_plugin_dependency_graph_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME PluginDependencyGraphFocusedTests COMMAND test_plugin_dependency_graph_focused)

    set_tests_properties(PluginDependencyGraphFocusedTests PROPERTIES
        LABELS "plugins;dependency;graph;cycle-detection;kahn;focused;unit"
        TIMEOUT 60
    )

    message(STATUS "  PluginDependencyGraph: Kahn topological sort, cycle detection, load-order validation")
else()
    message(STATUS "Plugin Dependency Graph focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_dependency_graph.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_dependency_resolver.cpp")
    message(STATUS "Adding Plugin Dependency Resolver focused tests")

    add_executable(test_plugin_dependency_resolver_focused
        test_plugin_dependency_resolver.cpp
    )

    target_include_directories(test_plugin_dependency_resolver_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_plugin_dependency_resolver_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_plugin_dependency_resolver_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME PluginDependencyResolverFocusedTests COMMAND test_plugin_dependency_resolver_focused)

    set_tests_properties(PluginDependencyResolverFocusedTests PROPERTIES
        LABELS "plugins;dependency;resolver;version-check;focused;unit"
        TIMEOUT 60
    )

    message(STATUS "  PluginDependencyResolver: inter-plugin dependency resolution, API version satisfaction, ordered loading")
else()
    message(STATUS "Plugin Dependency Resolver focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_dependency_resolver.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_metrics.cpp")
    message(STATUS "Adding Plugin Metrics focused tests")

    add_executable(test_plugin_metrics_focused
        test_plugin_metrics.cpp
    )

    target_include_directories(test_plugin_metrics_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_plugin_metrics_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_plugin_metrics_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME PluginMetricsFocusedTests COMMAND test_plugin_metrics_focused)

    set_tests_properties(PluginMetricsFocusedTests PROPERTIES
        LABELS "plugins;metrics;prometheus;call-count;latency;error-rate;focused;unit"
        TIMEOUT 60
    )

    message(STATUS "  PluginMetrics: call-count, latency P99, error-rate, Prometheus export, PluginMetricsCollector")
else()
    message(STATUS "Plugin Metrics focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_metrics.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_metrics_integration.cpp")
    message(STATUS "Adding Plugin Metrics Integration focused tests")

    add_executable(test_plugin_metrics_integration_focused
        test_plugin_metrics_integration.cpp
    )

    target_include_directories(test_plugin_metrics_integration_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_plugin_metrics_integration_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_plugin_metrics_integration_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME PluginMetricsIntegrationFocusedTests COMMAND test_plugin_metrics_integration_focused)

    set_tests_properties(PluginMetricsIntegrationFocusedTests PROPERTIES
        LABELS "plugins;metrics;integration;grafana;health-score;focused;integration"
        TIMEOUT 90
    )

    message(STATUS "  PluginMetricsIntegration: end-to-end metrics pipeline, Grafana dashboard integration, plugin_health_score gauge")
else()
    message(STATUS "Plugin Metrics Integration focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_metrics_integration.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_security_audit.cpp")
    message(STATUS "Adding Plugin Security Audit focused tests")

    add_executable(test_plugin_security_audit_focused
        test_plugin_security_audit.cpp
    )

    target_include_directories(test_plugin_security_audit_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_plugin_security_audit_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
        ${CMAKE_DL_LIBS}
    )

    target_compile_definitions(test_plugin_security_audit_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME PluginSecurityAuditFocusedTests COMMAND test_plugin_security_audit_focused)

    set_tests_properties(PluginSecurityAuditFocusedTests PROPERTIES
        LABELS "plugins;security;audit;ed25519;signature;focused;unit"
        TIMEOUT 60
    )

    message(STATUS "  PluginSecurityAudit: Ed25519 signature enforcement, unsigned plugin rejection, audit event emission")
else()
    message(STATUS "Plugin Security Audit focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_security_audit.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_security_implementation.cpp")
    message(STATUS "Adding Plugin Security Implementation focused tests")

    add_executable(test_plugin_security_impl_focused
        test_plugin_security_implementation.cpp
    )

    target_include_directories(test_plugin_security_impl_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_plugin_security_impl_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
        ${CMAKE_DL_LIBS}
    )

    target_compile_definitions(test_plugin_security_impl_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME PluginSecurityImplementationFocusedTests COMMAND test_plugin_security_impl_focused)

    set_tests_properties(PluginSecurityImplementationFocusedTests PROPERTIES
        LABELS "plugins;security;sandbox;capability;isolation;focused;unit"
        TIMEOUT 60
    )

    message(STATUS "  PluginSecurityImplementation: capability sandbox, in-process isolation, edition-gated enforcement")
else()
    message(STATUS "Plugin Security Implementation focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_security_implementation.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_marketplace_manifest.cpp")
    message(STATUS "Adding Plugin Marketplace Manifest focused tests")

    add_executable(test_plugin_marketplace_manifest_focused
        test_plugin_marketplace_manifest.cpp
    )

    target_include_directories(test_plugin_marketplace_manifest_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_plugin_marketplace_manifest_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_plugin_marketplace_manifest_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME PluginMarketplaceManifestFocusedTests COMMAND test_plugin_marketplace_manifest_focused)

    set_tests_properties(PluginMarketplaceManifestFocusedTests PROPERTIES
        LABELS "plugins;marketplace;manifest;json-schema;validation;focused;unit"
        TIMEOUT 60
    )

    message(STATUS "  PluginMarketplaceManifest: JSON Schema v2 validation, manifest field enforcement, capability declarations")
else()
    message(STATUS "Plugin Marketplace Manifest focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_marketplace_manifest.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_manager_comprehensive.cpp")
    message(STATUS "Adding Plugin Manager Comprehensive focused tests")

    add_executable(test_plugin_manager_comprehensive_focused
        test_plugin_manager_comprehensive.cpp
    )

    target_include_directories(test_plugin_manager_comprehensive_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_plugin_manager_comprehensive_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        nlohmann_json::nlohmann_json
        Threads::Threads
        ${CMAKE_DL_LIBS}
    )

    target_compile_definitions(test_plugin_manager_comprehensive_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME PluginManagerComprehensiveFocusedTests COMMAND test_plugin_manager_comprehensive_focused)

    set_tests_properties(PluginManagerComprehensiveFocusedTests PROPERTIES
        LABELS "plugins;manager;comprehensive;integration;focused;unit"
        TIMEOUT 120
    )

    message(STATUS "  PluginManagerComprehensive: full lifecycle integration, multi-plugin scenarios, edition gating")
else()
    message(STATUS "Plugin Manager Comprehensive focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_manager_comprehensive.cpp")
endif()

# ============================================================================
# LLM Timeout and Cancellation Tests (standalone – requires themis_llm / themis_core)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_llm_timeout_cancellation.cpp" AND THEMIS_ENABLE_LLM)
    message(STATUS "Adding LLM Timeout and Cancellation tests (Issue #1931, standalone binary)")

    add_executable(test_llm_timeout_cancellation
        test_llm_timeout_cancellation.cpp
    )

    target_include_directories(test_llm_timeout_cancellation PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    if(TARGET themis_llm)
        target_link_libraries(test_llm_timeout_cancellation PRIVATE
            ${TEST_LIBS}
            themis_llm
            spdlog::spdlog
            nlohmann_json::nlohmann_json
            OpenSSL::SSL
            OpenSSL::Crypto
            Threads::Threads
        )
    else()
        target_link_libraries(test_llm_timeout_cancellation PRIVATE
            ${TEST_LIBS}
            themis_core
            spdlog::spdlog
            nlohmann_json::nlohmann_json
            OpenSSL::SSL
            OpenSSL::Crypto
            Threads::Threads
        )
    endif()

    target_compile_definitions(test_llm_timeout_cancellation PRIVATE
        THEMIS_TEST_BUILD=1
        THEMIS_LLM_EXPORTS
    )

    add_test(NAME LLMTimeoutCancellationTests COMMAND test_llm_timeout_cancellation)

    set_tests_properties(LLMTimeoutCancellationTests PROPERTIES
        LABELS "llm;timeout;cancellation;async;inference;unit"
        TIMEOUT 120
    )

    message(STATUS "  LLMTimeoutCancellation: per-request timeout propagation, cancel(), streaming stop, DROP_OLDEST, dedup cache")
else()
    if(NOT THEMIS_ENABLE_LLM)
        message(STATUS "LLM Timeout Cancellation tests skipped (THEMIS_ENABLE_LLM=OFF)")
    else()
        message(STATUS "LLM Timeout Cancellation tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_llm_timeout_cancellation.cpp")
    endif()
endif()

# ============================================================================
# Global Transaction Manager Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_global_transaction_manager.cpp")
    message(STATUS "Adding Global Transaction Manager tests")

    add_executable(test_global_transaction_manager
        test_global_transaction_manager.cpp
    )

    target_link_libraries(test_global_transaction_manager PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_global_transaction_manager PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME GlobalTransactionManagerTests COMMAND test_global_transaction_manager)

    set_tests_properties(GlobalTransactionManagerTests PROPERTIES
        LABELS "transaction;distributed;multi-region;acid;unit"
        TIMEOUT 120
    )

    message(STATUS "  GlobalTransactionManager: 2PC across regions, TrueTime commit timestamp, recovery, statistics")
else()
    message(STATUS "Global Transaction Manager tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_global_transaction_manager.cpp")
endif()

# ============================================================================
# Parallel Full-Table Scan Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_parallel_scan.cpp")
    message(STATUS "Adding Parallel Full-Table Scan tests")

    add_executable(test_parallel_scan
        test_parallel_scan.cpp
    )

    target_link_libraries(test_parallel_scan PRIVATE
        ${TEST_LIBS}
        themis_core
        TBB::tbb
        RocksDB::rocksdb
        Threads::Threads
    )

    target_compile_definitions(test_parallel_scan PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ParallelScanTests COMMAND test_parallel_scan)

    set_tests_properties(ParallelScanTests PROPERTIES
        LABELS "query;parallel;scan;fullscan;unit"
        TIMEOUT 300
    )

    message(STATUS "  Parallel Scan: sequential path, morsel-driven parallel path, predicate correctness")
else()
    message(STATUS "Parallel Full-Table Scan tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_parallel_scan.cpp")
endif()

# ============================================================================
# CDN Cache Middleware Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_cdn_cache_middleware.cpp")
    message(STATUS "Adding CDN Cache Middleware tests")

    add_executable(test_cdn_cache_middleware
        test_cdn_cache_middleware.cpp
        ${THEMIS_ROOT_DIR}/src/server/cdn_cache_middleware.cpp
    )

    target_include_directories(test_cdn_cache_middleware PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_cdn_cache_middleware PRIVATE
        ${TEST_LIBS}
        Threads::Threads
    )

    target_compile_definitions(test_cdn_cache_middleware PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME CdnCacheMiddlewareTests
             COMMAND test_cdn_cache_middleware)

    set_tests_properties(CdnCacheMiddlewareTests PROPERTIES
        LABELS "server;cdn;cache;edge;http;unit"
        TIMEOUT 60
    )

    message(STATUS "  CdnCacheMiddleware: cache-control headers, ETag, conditional requests, route policies")
else()
    message(STATUS "CDN Cache Middleware tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_cdn_cache_middleware.cpp")
endif()

# ============================================================================
# Content Module — Focused Standalone Test Targets (Phase 4)
# These targets build directly against themis_content (or source files) and
# can be run quickly without the full unified test binary.
# ============================================================================

if(THEMIS_ENABLE_CONTENT)

    # -------------------------------------------------------------------------
    # ContentMetrics focused tests
    # -------------------------------------------------------------------------
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_content_metrics.cpp")
        message(STATUS "Adding Content Metrics focused tests (Phase 4, Issue #1698)")

        add_executable(test_content_metrics_focused
            test_content_metrics.cpp
        )

        target_include_directories(test_content_metrics_focused PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
        )

        target_link_libraries(test_content_metrics_focused PRIVATE
            ${TEST_LIBS}
            themis_content
            nlohmann_json::nlohmann_json
            Threads::Threads
        )

        target_compile_definitions(test_content_metrics_focused PRIVATE
            THEMIS_TEST_BUILD=1
        )

        add_test(NAME ContentMetricsFocusedTests COMMAND test_content_metrics_focused)

        set_tests_properties(ContentMetricsFocusedTests PROPERTIES
            LABELS "content;metrics;prometheus;focused;unit"
            TIMEOUT 60
        )

        message(STATUS "  ContentMetrics: ingestion counters, error rates, embedding failures, Prometheus export")
    else()
        message(STATUS "Content Metrics focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_content_metrics.cpp")
    endif()

    # -------------------------------------------------------------------------
    # ContentSecurity focused tests
    # -------------------------------------------------------------------------
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_content_security.cpp")
        message(STATUS "Adding Content Security focused tests (Phase 4, Issue #1698)")

        add_executable(test_content_security_focused
            test_content_security.cpp
        )

        target_include_directories(test_content_security_focused PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
        )

        target_link_libraries(test_content_security_focused PRIVATE
            ${TEST_LIBS}
            themis_content
            nlohmann_json::nlohmann_json
            Threads::Threads
        )

        target_compile_definitions(test_content_security_focused PRIVATE
            THEMIS_TEST_BUILD=1
        )

        add_test(NAME ContentSecurityFocusedTests COMMAND test_content_security_focused)

        set_tests_properties(ContentSecurityFocusedTests PROPERTIES
            LABELS "content;security;archive-bomb;validation;focused;unit"
            TIMEOUT 60
        )

        message(STATUS "  ContentSecurity: malware signature scan, archive-bomb detection, path traversal rejection")
    else()
        message(STATUS "Content Security focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_content_security.cpp")
    endif()

    # -------------------------------------------------------------------------
    # LanguageDetector focused tests
    # -------------------------------------------------------------------------
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_content_language_detector.cpp")
        message(STATUS "Adding Content LanguageDetector focused tests (Phase 4, Issue #1698)")

        add_executable(test_content_language_detector_focused
            test_content_language_detector.cpp
        )

        target_include_directories(test_content_language_detector_focused PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
        )

        target_link_libraries(test_content_language_detector_focused PRIVATE
            ${TEST_LIBS}
            themis_content
            Threads::Threads
        )

        target_compile_definitions(test_content_language_detector_focused PRIVATE
            THEMIS_TEST_BUILD=1
        )

        add_test(NAME ContentLanguageDetectorFocusedTests COMMAND test_content_language_detector_focused)

        set_tests_properties(ContentLanguageDetectorFocusedTests PROPERTIES
            LABELS "content;language-detector;nlp;focused;unit"
            TIMEOUT 60
        )

        message(STATUS "  LanguageDetector: per-script detection, confidence scores, unknown-language fallback")
    else()
        message(STATUS "Content LanguageDetector focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_content_language_detector.cpp")
    endif()

    # -------------------------------------------------------------------------
    # AudioProcessor focused tests
    # -------------------------------------------------------------------------
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_content_audio_processor.cpp")
        message(STATUS "Adding Content AudioProcessor focused tests (Phase 4, Issue #1698)")

        add_executable(test_content_audio_processor_focused
            test_content_audio_processor.cpp
        )

        target_include_directories(test_content_audio_processor_focused PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
        )

        target_link_libraries(test_content_audio_processor_focused PRIVATE
            ${TEST_LIBS}
            themis_content
            Threads::Threads
        )

        target_compile_definitions(test_content_audio_processor_focused PRIVATE
            THEMIS_TEST_BUILD=1
        )

        add_test(NAME ContentAudioProcessorFocusedTests COMMAND test_content_audio_processor_focused)

        set_tests_properties(ContentAudioProcessorFocusedTests PROPERTIES
            LABELS "content;audio;metadata;stt;focused;unit"
            TIMEOUT 60
        )

        message(STATUS "  AudioProcessor: WAV/FLAC/MP3/Ogg parsing, ID3v2 tags, STT transcription init, error handling")
    else()
        message(STATUS "Content AudioProcessor focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_content_audio_processor.cpp")
    endif()

    # -------------------------------------------------------------------------
    # ProcessorChainConfig focused tests
    # -------------------------------------------------------------------------
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_content_processor_chain.cpp")
        message(STATUS "Adding Content ProcessorChain focused tests (Phase 4, Issue #1698)")

        add_executable(test_content_processor_chain_focused
            test_content_processor_chain.cpp
        )

        target_include_directories(test_content_processor_chain_focused PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
        )

        target_link_libraries(test_content_processor_chain_focused PRIVATE
            ${TEST_LIBS}
            themis_content
            Threads::Threads
        )

        target_compile_definitions(test_content_processor_chain_focused PRIVATE
            THEMIS_TEST_BUILD=1
        )

        add_test(NAME ContentProcessorChainFocusedTests COMMAND test_content_processor_chain_focused)

        set_tests_properties(ContentProcessorChainFocusedTests PROPERTIES
            LABELS "content;processor-chain;pipeline;focused;unit"
            TIMEOUT 60
        )

        message(STATUS "  ProcessorChainConfig: per-ContentCategory chain, runtime enable/disable, fallback policy")
    else()
        message(STATUS "Content ProcessorChain focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_content_processor_chain.cpp")
    endif()

    # -------------------------------------------------------------------------
    # DeduplicationChecker focused tests
    # -------------------------------------------------------------------------
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_content_deduplication.cpp")
        message(STATUS "Adding Content DeduplicationChecker focused tests (Phase 4, Issue #1698)")

        add_executable(test_content_deduplication_focused
            test_content_deduplication.cpp
        )

        target_include_directories(test_content_deduplication_focused PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
        )

        target_link_libraries(test_content_deduplication_focused PRIVATE
            ${TEST_LIBS}
            themis_content
            themis_core
            nlohmann_json::nlohmann_json
            OpenSSL::SSL
            OpenSSL::Crypto
            Threads::Threads
        )

        target_compile_definitions(test_content_deduplication_focused PRIVATE
            THEMIS_TEST_BUILD=1
        )

        add_test(NAME ContentDeduplicationFocusedTests COMMAND test_content_deduplication_focused)

        set_tests_properties(ContentDeduplicationFocusedTests PROPERTIES
            LABELS "content;deduplication;phash;minhash;focused;unit"
            TIMEOUT 120
        )

        message(STATUS "  DeduplicationChecker: pHash Hamming distance, MinHash band-LSH, SHA-256 exact dedup, ContentMetrics counters")
    else()
        message(STATUS "Content DeduplicationChecker focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_content_deduplication.cpp")
    endif()

endif() # THEMIS_ENABLE_CONTENT
# CDC Admin Tests (standalone – cdc_admin.cpp is in themis_core only when
# THEMIS_ENABLE_HTTP_SERVER is ON; add it explicitly when HTTP_SERVER is OFF)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_cdc_admin.cpp")
    message(STATUS "Adding CDC Admin tests (Issue #1623, standalone binary)")

    add_executable(test_cdc_admin
        test_cdc_admin.cpp
    )

    # cdc_admin.cpp is compiled into themis_core only when THEMIS_ENABLE_HTTP_SERVER=ON.
    # When HTTP_SERVER is OFF, add it as a direct source to avoid missing-symbol errors.
    if(NOT THEMIS_ENABLE_HTTP_SERVER)
        target_sources(test_cdc_admin PRIVATE
            ${THEMIS_ROOT_DIR}/src/cdc/cdc_admin.cpp
        )
    endif()

    target_include_directories(test_cdc_admin PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_cdc_admin PRIVATE
        ${TEST_LIBS}
        themis_core
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )

    target_compile_definitions(test_cdc_admin PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME CDCAdminFocusedTests COMMAND test_cdc_admin)

    set_tests_properties(CDCAdminFocusedTests PROPERTIES
        LABELS "cdc;admin;retention;compaction;unit"
        TIMEOUT 120
    )

    message(STATUS "  CDCAdmin: CDCAdmin, purgeOlderThan, compactLog, retentionPolicy, subscriptionManagement")
else()
    message(STATUS "CDC Admin tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_cdc_admin.cpp")
endif()

# ============================================================================
# Tenant Buffer Manager Tests (standalone – changefeed_buffer.cpp and
# tenant_buffer_manager.cpp are in themis_core; link directly)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_tenant_buffer_manager.cpp")
    message(STATUS "Adding Tenant Buffer Manager tests (Issue #1623, standalone binary)")

    add_executable(test_tenant_buffer_manager
        test_tenant_buffer_manager.cpp
    )

    target_include_directories(test_tenant_buffer_manager PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_tenant_buffer_manager PRIVATE
        ${TEST_LIBS}
        themis_core
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )

    target_compile_definitions(test_tenant_buffer_manager PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME TenantBufferManagerFocusedTests COMMAND test_tenant_buffer_manager)

    set_tests_properties(TenantBufferManagerFocusedTests PROPERTIES
        LABELS "cdc;tenant;buffer;multi-tenant;isolation;unit"
        TIMEOUT 120
    )

    message(STATUS "  TenantBufferManager: per-tenant CDC buffers, quota enforcement, metrics isolation, flush lifecycle")
else()
    message(STATUS "Tenant Buffer Manager tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_tenant_buffer_manager.cpp")
# Config Module – Focused Standalone Test Targets
# ============================================================================

# Shared config source list used by all four focused config test executables.
set(CONFIG_MODULE_SOURCES
    ${THEMIS_ROOT_DIR}/src/config/config_path_resolver.cpp
    ${THEMIS_ROOT_DIR}/src/config/config_metrics_exporter.cpp
    ${THEMIS_ROOT_DIR}/src/config/config_schema_validator.cpp
    ${THEMIS_ROOT_DIR}/src/config/config_audit_log.cpp
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_config_path_resolver.cpp")
    message(STATUS "Adding ConfigPathResolver focused tests")

    add_executable(test_config_path_resolver_focused
        test_config_path_resolver.cpp
        ${CONFIG_MODULE_SOURCES}
    )

    target_include_directories(test_config_path_resolver_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_config_path_resolver_focused PRIVATE
        ${TEST_LIBS}
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_config_path_resolver_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ConfigPathResolverFocusedTests COMMAND test_config_path_resolver_focused)

    set_tests_properties(ConfigPathResolverFocusedTests PROPERTIES
        LABELS "config;path;resolver;lru;cache;unit;focused"
        TIMEOUT 120
    )

    message(STATUS "  ConfigPathResolver: path mapping, LRU cache, legacy fallback, metrics, audit trail")
else()
    message(STATUS "ConfigPathResolver focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_config_path_resolver.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_config_schema_validator.cpp")
    message(STATUS "Adding ConfigSchemaValidator focused tests")

    add_executable(test_config_schema_validator_focused
        test_config_schema_validator.cpp
        ${CONFIG_MODULE_SOURCES}
    )

    target_include_directories(test_config_schema_validator_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_config_schema_validator_focused PRIVATE
        ${TEST_LIBS}
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        yaml-cpp::yaml-cpp
        Threads::Threads
    )

    target_compile_definitions(test_config_schema_validator_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ConfigSchemaValidatorFocusedTests COMMAND test_config_schema_validator_focused)

    set_tests_properties(ConfigSchemaValidatorFocusedTests PROPERTIES
        LABELS "config;schema;validator;yaml;json;unit;focused"
        TIMEOUT 120
    )

    message(STATUS "  ConfigSchemaValidator: JSON Schema Draft-7 subset, YAML/JSON loading, error reporting")
else()
    message(STATUS "ConfigSchemaValidator focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_config_schema_validator.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_config_migration_scanner.cpp")
    message(STATUS "Adding ConfigMigrationScanner focused tests")

    add_executable(test_config_migration_scanner_focused
        test_config_migration_scanner.cpp
        ${CONFIG_MODULE_SOURCES}
    )

    target_include_directories(test_config_migration_scanner_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_config_migration_scanner_focused PRIVATE
        ${TEST_LIBS}
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_config_migration_scanner_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ConfigMigrationScannerFocusedTests COMMAND test_config_migration_scanner_focused)

    set_tests_properties(ConfigMigrationScannerFocusedTests PROPERTIES
        LABELS "config;migration;scanner;cli;unit;focused"
        TIMEOUT 120
    )

    message(STATUS "  ConfigMigrationScanner: file scanning, dry-run, fix mode, output formats (text/json/csv)")
else()
    message(STATUS "ConfigMigrationScanner focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_config_migration_scanner.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_config_coverage.cpp")
    message(STATUS "Adding Config coverage-gap focused tests")

    add_executable(test_config_coverage_focused
        test_config_coverage.cpp
        ${CONFIG_MODULE_SOURCES}
    )

    target_include_directories(test_config_coverage_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_config_coverage_focused PRIVATE
        ${TEST_LIBS}
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        yaml-cpp::yaml-cpp
        Threads::Threads
    )

    target_compile_definitions(test_config_coverage_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ConfigCoverageFocusedTests COMMAND test_config_coverage_focused)

    set_tests_properties(ConfigCoverageFocusedTests PROPERTIES
        LABELS "config;coverage;lru;audit;exceptions;unit;focused"
        TIMEOUT 120
    )

    message(STATUS "  Config coverage: LRUCacheWithTTL, ConfigAuditLog, exception hierarchy, schema edge-cases")
else()
    message(STATUS "Config coverage focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_config_coverage.cpp")
endif()

# ============================================================================
# Auth Module Tests
# ============================================================================

# JWT Validator Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_jwt_validator.cpp")
    message(STATUS "Adding JWT Validator tests")

    add_executable(test_jwt_validator
        test_jwt_validator.cpp
    )

    target_link_libraries(test_jwt_validator PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        CURL::libcurl
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_jwt_validator PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME JWTValidatorTests COMMAND test_jwt_validator)

    set_tests_properties(JWTValidatorTests PROPERTIES
        LABELS "auth;jwt;validation;rs256;unit"
        TIMEOUT 120
    )

    message(STATUS "  JWT Validator: RS256 signature, JWKS caching, claim extraction, expiry, kid revocation")
else()
    message(STATUS "JWT Validator tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_jwt_validator.cpp")
endif()

# JWT EdDSA Comprehensive Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_jwt_eddsa_comprehensive.cpp")
    message(STATUS "Adding JWT EdDSA Comprehensive tests")

    add_executable(test_jwt_eddsa_comprehensive
        test_jwt_eddsa_comprehensive.cpp
    )

    target_link_libraries(test_jwt_eddsa_comprehensive PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        CURL::libcurl
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_jwt_eddsa_comprehensive PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME JWTEdDSAComprehensiveTests COMMAND test_jwt_eddsa_comprehensive)

    set_tests_properties(JWTEdDSAComprehensiveTests PROPERTIES
        LABELS "auth;jwt;eddsa;ed25519;comprehensive;unit"
        TIMEOUT 120
    )

    message(STATUS "  JWT EdDSA: Ed25519/Ed448 signatures, positive/negative cases, algorithm agility")
else()
    message(STATUS "JWT EdDSA Comprehensive tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_jwt_eddsa_comprehensive.cpp")
endif()

# JWT ES256 Comprehensive Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_jwt_es256_comprehensive.cpp")
    message(STATUS "Adding JWT ES256 Comprehensive tests")

    add_executable(test_jwt_es256_comprehensive
        test_jwt_es256_comprehensive.cpp
    )

    target_link_libraries(test_jwt_es256_comprehensive PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        CURL::libcurl
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_jwt_es256_comprehensive PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME JWTES256ComprehensiveTests COMMAND test_jwt_es256_comprehensive)

    set_tests_properties(JWTES256ComprehensiveTests PROPERTIES
        LABELS "auth;jwt;es256;ecdsa;comprehensive;unit"
        TIMEOUT 120
    )

    message(STATUS "  JWT ES256: ECDSA P-256 signatures, JWKS with EC keys, algorithm coverage")
else()
    message(STATUS "JWT ES256 Comprehensive tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_jwt_es256_comprehensive.cpp")
endif()

# JWT Integration Tests (live mini-JWKS HTTP server via Boost.Asio)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_jwt_integration.cpp")
    message(STATUS "Adding JWT Integration tests")

    add_executable(test_jwt_integration
        test_jwt_integration.cpp
    )

    target_link_libraries(test_jwt_integration PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        CURL::libcurl
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )

    if(Boost_FOUND AND TARGET Boost::system)
        target_link_libraries(test_jwt_integration PRIVATE Boost::system)
    endif()

    target_compile_definitions(test_jwt_integration PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME JWTIntegrationTests COMMAND test_jwt_integration)

    set_tests_properties(JWTIntegrationTests PROPERTIES
        LABELS "auth;jwt;integration;jwks;live-server;unit"
        TIMEOUT 180
    )

    message(STATUS "  JWT Integration: live JWKS mini-server, end-to-end token validation, cache refresh")
else()
    message(STATUS "JWT Integration tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_jwt_integration.cpp")
endif()

# JWT Key Rotation Comprehensive Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_jwt_key_rotation_comprehensive.cpp")
    message(STATUS "Adding JWT Key Rotation Comprehensive tests")

    add_executable(test_jwt_key_rotation_comprehensive
        test_jwt_key_rotation_comprehensive.cpp
    )

    target_link_libraries(test_jwt_key_rotation_comprehensive PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        CURL::libcurl
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_jwt_key_rotation_comprehensive PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME JWTKeyRotationComprehensiveTests COMMAND test_jwt_key_rotation_comprehensive)

    set_tests_properties(JWTKeyRotationComprehensiveTests PROPERTIES
        LABELS "auth;jwt;key-rotation;comprehensive;unit"
        TIMEOUT 120
    )

    message(STATUS "  JWT Key Rotation: rotation manager, kid tracking, graceful overlap, revocation")
else()
    message(STATUS "JWT Key Rotation Comprehensive tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_jwt_key_rotation_comprehensive.cpp")
endif()

# JWT Management Comprehensive Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_jwt_management_comprehensive.cpp")
    message(STATUS "Adding JWT Management Comprehensive tests")

    add_executable(test_jwt_management_comprehensive
        test_jwt_management_comprehensive.cpp
    )

    target_link_libraries(test_jwt_management_comprehensive PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        CURL::libcurl
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_jwt_management_comprehensive PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME JWTManagementComprehensiveTests COMMAND test_jwt_management_comprehensive)

    set_tests_properties(JWTManagementComprehensiveTests PROPERTIES
        LABELS "auth;jwt;management;comprehensive;unit"
        TIMEOUT 120
    )

    message(STATUS "  JWT Management: issuer/audience checks, clock skew, multi-audience, edge cases")
else()
    message(STATUS "JWT Management Comprehensive tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_jwt_management_comprehensive.cpp")
endif()

# JWT Rotation Unit Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_jwt_rotation_unit.cpp")
    message(STATUS "Adding JWT Rotation Unit tests")

    add_executable(test_jwt_rotation_unit
        test_jwt_rotation_unit.cpp
    )

    target_link_libraries(test_jwt_rotation_unit PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        CURL::libcurl
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_jwt_rotation_unit PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME JWTRotationUnitTests COMMAND test_jwt_rotation_unit)

    set_tests_properties(JWTRotationUnitTests PROPERTIES
        LABELS "auth;jwt;rotation;unit"
        TIMEOUT 120
    )

    message(STATUS "  JWT Rotation Unit: key-id tracking, revalidation after rotation, determinism")
else()
    message(STATUS "JWT Rotation Unit tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_jwt_rotation_unit.cpp")
endif()

# JWT Token Revocation Integration Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_jwt_token_revocation_integration.cpp")
    message(STATUS "Adding JWT Token Revocation Integration tests")

    add_executable(test_jwt_token_revocation_integration
        test_jwt_token_revocation_integration.cpp
    )

    target_link_libraries(test_jwt_token_revocation_integration PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_jwt_token_revocation_integration PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME JWTTokenRevocationIntegrationTests COMMAND test_jwt_token_revocation_integration)

    set_tests_properties(JWTTokenRevocationIntegrationTests PROPERTIES
        LABELS "auth;jwt;revocation;blacklist;integration;unit"
        TIMEOUT 120
    )

    message(STATUS "  JWT Token Revocation: jti blacklist, per-token and per-kid revocation, persistence")
else()
    message(STATUS "JWT Token Revocation Integration tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_jwt_token_revocation_integration.cpp")
endif()

# JWT Validation Hardening Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_jwt_validation_hardening.cpp")
    message(STATUS "Adding JWT Validation Hardening tests")

    add_executable(test_jwt_validation_hardening
        test_jwt_validation_hardening.cpp
    )

    target_link_libraries(test_jwt_validation_hardening PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        CURL::libcurl
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_jwt_validation_hardening PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME JWTValidationHardeningTests COMMAND test_jwt_validation_hardening)

    set_tests_properties(JWTValidationHardeningTests PROPERTIES
        LABELS "auth;jwt;hardening;security;injection;unit"
        TIMEOUT 120
    )

    message(STATUS "  JWT Validation Hardening: malformed tokens, alg:none attack, oversized inputs, injection")
else()
    message(STATUS "JWT Validation Hardening tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_jwt_validation_hardening.cpp")
endif()

# API Key Authenticator Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_api_key_authenticator.cpp")
    message(STATUS "Adding API Key Authenticator tests")

    add_executable(test_api_key_authenticator
        test_api_key_authenticator.cpp
    )

    target_link_libraries(test_api_key_authenticator PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_api_key_authenticator PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ApiKeyAuthenticatorTests COMMAND test_api_key_authenticator)

    set_tests_properties(ApiKeyAuthenticatorTests PROPERTIES
        LABELS "auth;api-key;static-token;unit"
        TIMEOUT 60
    )

    message(STATUS "  API Key Authenticator: key creation, HMAC validation, expiry, revocation, scopes")
else()
    message(STATUS "API Key Authenticator tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_api_key_authenticator.cpp")
endif()

# Auth Anomaly Detection Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_auth_anomaly_detection.cpp")
    message(STATUS "Adding Auth Anomaly Detection tests")

    add_executable(test_auth_anomaly_detection
        test_auth_anomaly_detection.cpp
    )

    target_link_libraries(test_auth_anomaly_detection PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_auth_anomaly_detection PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME AuthAnomalyDetectionTests COMMAND test_auth_anomaly_detection)

    set_tests_properties(AuthAnomalyDetectionTests PROPERTIES
        LABELS "auth;anomaly;brute-force;rate-limiting;unit"
        TIMEOUT 120
    )

    message(STATUS "  Auth Anomaly Detection: brute-force detection, IP ban, concurrent attack simulation")
else()
    message(STATUS "Auth Anomaly Detection tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_auth_anomaly_detection.cpp")
endif()

# Auth Audit Logger Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_auth_audit_logger.cpp")
    message(STATUS "Adding Auth Audit Logger tests")

    add_executable(test_auth_audit_logger
        test_auth_audit_logger.cpp
    )

    target_link_libraries(test_auth_audit_logger PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        CURL::libcurl
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_auth_audit_logger PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME AuthAuditLoggerTests COMMAND test_auth_audit_logger)

    set_tests_properties(AuthAuditLoggerTests PROPERTIES
        LABELS "auth;audit;logging;compliance;unit"
        TIMEOUT 120
    )

    message(STATUS "  Auth Audit Logger: success/failure events, JWT/Kerberos/SAML/MFA audit trails")
else()
    message(STATUS "Auth Audit Logger tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_auth_audit_logger.cpp")
endif()

# Auth Error Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_auth_error.cpp")
    message(STATUS "Adding Auth Error tests")

    add_executable(test_auth_error
        test_auth_error.cpp
    )

    target_link_libraries(test_auth_error PRIVATE
        ${TEST_LIBS}
        themis_core
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_auth_error PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME AuthErrorTests COMMAND test_auth_error)

    set_tests_properties(AuthErrorTests PROPERTIES
        LABELS "auth;error;exception;unit"
        TIMEOUT 60
    )

    message(STATUS "  Auth Error: error codes, messages, JSON serialization, exception hierarchy")
else()
    message(STATUS "Auth Error tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_auth_error.cpp")
endif()

# Auth Input Validation Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_auth_input_validation.cpp")
    message(STATUS "Adding Auth Input Validation tests")

    add_executable(test_auth_input_validation
        test_auth_input_validation.cpp
    )

    target_link_libraries(test_auth_input_validation PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        CURL::libcurl
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_auth_input_validation PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME AuthInputValidationTests COMMAND test_auth_input_validation)

    set_tests_properties(AuthInputValidationTests PROPERTIES
        LABELS "auth;input-validation;security;injection;unit"
        TIMEOUT 60
    )

    message(STATUS "  Auth Input Validation: oversized tokens, null bytes, injection strings, boundary checks")
else()
    message(STATUS "Auth Input Validation tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_auth_input_validation.cpp")
endif()

# Auth Metrics Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_auth_metrics.cpp")
    message(STATUS "Adding Auth Metrics tests")

    add_executable(test_auth_metrics
        test_auth_metrics.cpp
    )

    target_link_libraries(test_auth_metrics PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_auth_metrics PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME AuthMetricsTests COMMAND test_auth_metrics)

    set_tests_properties(AuthMetricsTests PROPERTIES
        LABELS "auth;metrics;counters;latency;unit"
        TIMEOUT 60
    )

    message(STATUS "  Auth Metrics: success/failure counters, latency histograms, Prometheus export")
else()
    message(STATUS "Auth Metrics tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_auth_metrics.cpp")
endif()

# Auth Rate Limiter Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_auth_rate_limiter.cpp")
    message(STATUS "Adding Auth Rate Limiter tests")

    add_executable(test_auth_rate_limiter
        test_auth_rate_limiter.cpp
    )

    target_link_libraries(test_auth_rate_limiter PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_auth_rate_limiter PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME AuthRateLimiterTests COMMAND test_auth_rate_limiter)

    set_tests_properties(AuthRateLimiterTests PROPERTIES
        LABELS "auth;rate-limiter;brute-force;sliding-window;unit"
        TIMEOUT 120
    )

    message(STATUS "  Auth Rate Limiter: sliding window, per-IP, per-user, concurrent access, decay")
else()
    message(STATUS "Auth Rate Limiter tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_auth_rate_limiter.cpp")
endif()

# Auth Middleware Focused Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_auth_middleware.cpp")
    message(STATUS "Adding Auth Middleware focused tests")

    add_executable(test_auth_middleware
        test_auth_middleware.cpp
    )

    target_link_libraries(test_auth_middleware PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        CURL::libcurl
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_auth_middleware PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME AuthMiddlewareFocusedTests COMMAND test_auth_middleware)

    set_tests_properties(AuthMiddlewareFocusedTests PROPERTIES
        LABELS "auth;middleware;jwt;api-key;scope;unit"
        TIMEOUT 120
    )

    message(STATUS "  Auth Middleware: JWT + API-key pipeline, scope enforcement, tenant isolation")
else()
    message(STATUS "Auth Middleware focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_auth_middleware.cpp")
endif()

# GSSAPI / Kerberos Authenticator Tests (requires THEMIS_ENABLE_KERBEROS)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_gssapi_authenticator.cpp")
    if(THEMIS_ENABLE_KERBEROS)
        message(STATUS "Adding GSSAPI/Kerberos Authenticator tests")

        add_executable(test_gssapi_authenticator
            test_gssapi_authenticator.cpp
        )

        target_link_libraries(test_gssapi_authenticator PRIVATE
            ${TEST_LIBS}
            themis_core
            OpenSSL::SSL
            OpenSSL::Crypto
            spdlog::spdlog
            Threads::Threads
        )

        if(TARGET KRB5::krb5)
            target_link_libraries(test_gssapi_authenticator PRIVATE KRB5::krb5)
        endif()
        if(TARGET KRB5::gssapi)
            target_link_libraries(test_gssapi_authenticator PRIVATE KRB5::gssapi)
        endif()

        target_compile_definitions(test_gssapi_authenticator PRIVATE
            THEMIS_TEST_BUILD=1
        )

        add_test(NAME GSSAPIAuthenticatorTests COMMAND test_gssapi_authenticator)

        set_tests_properties(GSSAPIAuthenticatorTests PROPERTIES
            LABELS "auth;gssapi;kerberos;enterprise-sso;unit"
            TIMEOUT 120
        )

        message(STATUS "  GSSAPI Authenticator: Kerberos ticket validation, principal extraction, AD SSO")
    else()
        message(STATUS "GSSAPI Authenticator tests skipped (THEMIS_ENABLE_KERBEROS not set)")
    endif()
else()
    message(STATUS "GSSAPI Authenticator tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_gssapi_authenticator.cpp")
endif()

# LDAP Authenticator Tests (requires THEMIS_LDAP_FOUND)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_ldap_authenticator.cpp")
    if(THEMIS_LDAP_FOUND)
        message(STATUS "Adding LDAP Authenticator tests")

        add_executable(test_ldap_authenticator
            test_ldap_authenticator.cpp
        )

        target_link_libraries(test_ldap_authenticator PRIVATE
            ${TEST_LIBS}
            themis_core
            OpenSSL::SSL
            OpenSSL::Crypto
            spdlog::spdlog
            Threads::Threads
        )

        if(THEMIS_LDAP_LIBRARIES)
            target_link_libraries(test_ldap_authenticator PRIVATE ${THEMIS_LDAP_LIBRARIES})
        endif()

        target_compile_definitions(test_ldap_authenticator PRIVATE
            THEMIS_TEST_BUILD=1
        )

        add_test(NAME LDAPAuthenticatorTests COMMAND test_ldap_authenticator)

        set_tests_properties(LDAPAuthenticatorTests PROPERTIES
            LABELS "auth;ldap;active-directory;enterprise;unit"
            TIMEOUT 120
        )

        message(STATUS "  LDAP Authenticator: bind authentication, group membership, DN template")
    else()
        message(STATUS "LDAP Authenticator tests skipped (THEMIS_LDAP_FOUND not set)")
    endif()
else()
    message(STATUS "LDAP Authenticator tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_ldap_authenticator.cpp")
endif()

# MFA Authenticator Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_mfa_authenticator.cpp")
    message(STATUS "Adding MFA Authenticator tests")

    add_executable(test_mfa_authenticator
        test_mfa_authenticator.cpp
    )

    target_link_libraries(test_mfa_authenticator PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_mfa_authenticator PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME MFAAuthenticatorTests COMMAND test_mfa_authenticator)

    set_tests_properties(MFAAuthenticatorTests PROPERTIES
        LABELS "auth;mfa;totp;recovery-codes;unit"
        TIMEOUT 120
    )

    message(STATUS "  MFA Authenticator: TOTP generation/verification, recovery codes, replay prevention")
else()
    message(STATUS "MFA Authenticator tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_mfa_authenticator.cpp")
endif()

# mTLS Authenticator Focused Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_mtls_authenticator.cpp")
    message(STATUS "Adding mTLS Authenticator focused tests")

    add_executable(test_mtls_authenticator_focused
        test_mtls_authenticator.cpp
    )

    target_link_libraries(test_mtls_authenticator_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_mtls_authenticator_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME MtlsAuthenticatorFocusedTests COMMAND test_mtls_authenticator_focused)

    set_tests_properties(MtlsAuthenticatorFocusedTests PROPERTIES
        LABELS "auth;mtls;certificate;mutual-tls;unit"
        TIMEOUT 120
    )

    message(STATUS "  mTLS Authenticator: client certificate chain validation, identity mapping, SPIFFE")
else()
    message(STATUS "mTLS Authenticator focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_mtls_authenticator.cpp")
endif()

# OAuth 2.0 Device Flow Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_oauth_device_flow.cpp")
    message(STATUS "Adding OAuth 2.0 Device Flow tests")

    add_executable(test_oauth_device_flow
        test_oauth_device_flow.cpp
    )

    target_link_libraries(test_oauth_device_flow PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_oauth_device_flow PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME OAuthDeviceFlowTests COMMAND test_oauth_device_flow)

    set_tests_properties(OAuthDeviceFlowTests PROPERTIES
        LABELS "auth;oauth;device-flow;rfc8628;unit"
        TIMEOUT 120
    )

    message(STATUS "  OAuth Device Flow: device code request, polling, approval, token exchange (RFC 8628)")
else()
    message(STATUS "OAuth Device Flow tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_oauth_device_flow.cpp")
endif()

# OAuth 2.0 PKCE Flow Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_oauth_pkce_flow.cpp")
    message(STATUS "Adding OAuth 2.0 PKCE Flow tests")

    add_executable(test_oauth_pkce_flow
        test_oauth_pkce_flow.cpp
    )

    target_link_libraries(test_oauth_pkce_flow PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_oauth_pkce_flow PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME OAuthPKCEFlowTests COMMAND test_oauth_pkce_flow)

    set_tests_properties(OAuthPKCEFlowTests PROPERTIES
        LABELS "auth;oauth;pkce;s256;rfc7636;unit"
        TIMEOUT 60
    )

    message(STATUS "  OAuth PKCE Flow: code verifier/challenge, S256 method, public client flow (RFC 7636)")
else()
    message(STATUS "OAuth PKCE Flow tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_oauth_pkce_flow.cpp")
endif()

# SAML Authenticator Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_saml_authenticator.cpp")
    message(STATUS "Adding SAML Authenticator tests")

    add_executable(test_saml_authenticator
        test_saml_authenticator.cpp
    )

    target_link_libraries(test_saml_authenticator PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )

    if(TARGET pugixml::pugixml)
        target_link_libraries(test_saml_authenticator PRIVATE pugixml::pugixml)
    elseif(TARGET pugixml::static)
        target_link_libraries(test_saml_authenticator PRIVATE pugixml::static)
    endif()

    target_compile_definitions(test_saml_authenticator PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME SAMLAuthenticatorTests COMMAND test_saml_authenticator)

    set_tests_properties(SAMLAuthenticatorTests PROPERTIES
        LABELS "auth;saml;sso;idp;sp;unit"
        TIMEOUT 120
    )

    message(STATUS "  SAML Authenticator: SP/IdP-initiated SSO, assertion validation, attribute mapping")
else()
    message(STATUS "SAML Authenticator tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_saml_authenticator.cpp")
endif()

# Session Manager Focused Tests (standalone binary)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_session_manager.cpp")
    message(STATUS "Adding Session Manager focused tests (standalone binary)")

    add_executable(test_session_manager_focused
        test_session_manager.cpp
    )

    target_link_libraries(test_session_manager_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        CURL::libcurl
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_session_manager_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME SessionManagerFocusedTests COMMAND test_session_manager_focused)

    set_tests_properties(SessionManagerFocusedTests PROPERTIES
        LABELS "auth;session;revocation;lifecycle;unit"
        TIMEOUT 120
    )

    message(STATUS "  Session Manager: creation, validation, idle/absolute timeout, concurrent session limits, revocation")
else()
    message(STATUS "Session Manager focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_session_manager.cpp")
endif()

# TOTP Replay Cache Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_totp_replay_cache.cpp")
    message(STATUS "Adding TOTP Replay Cache tests")

    add_executable(test_totp_replay_cache
        test_totp_replay_cache.cpp
    )

    target_link_libraries(test_totp_replay_cache PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_totp_replay_cache PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME TOTPReplayCacheTests COMMAND test_totp_replay_cache)

    set_tests_properties(TOTPReplayCacheTests PROPERTIES
        LABELS "auth;totp;replay;cache;unit"
        TIMEOUT 60
    )

    message(STATUS "  TOTP Replay Cache: one-time-use enforcement, window expiry, concurrent access")
else()
    message(STATUS "TOTP Replay Cache tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_totp_replay_cache.cpp")
endif()

# TOTP Secret Encryption Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_totp_secret_encryption.cpp")
    message(STATUS "Adding TOTP Secret Encryption tests")

    add_executable(test_totp_secret_encryption
        test_totp_secret_encryption.cpp
    )

    target_link_libraries(test_totp_secret_encryption PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_totp_secret_encryption PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME TOTPSecretEncryptionTests COMMAND test_totp_secret_encryption)

    set_tests_properties(TOTPSecretEncryptionTests PROPERTIES
        LABELS "auth;totp;secret;encryption;aes-gcm;unit"
        TIMEOUT 60
    )

    message(STATUS "  TOTP Secret Encryption: AES-GCM wrapping, key derivation, round-trip, wrong-key rejection")
else()
    message(STATUS "TOTP Secret Encryption tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_totp_secret_encryption.cpp")
endif()

# WebAuthn / FIDO2 Authenticator Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_webauthn_authenticator.cpp")
    message(STATUS "Adding WebAuthn/FIDO2 Authenticator tests")

    add_executable(test_webauthn_authenticator
        test_webauthn_authenticator.cpp
    )

    target_link_libraries(test_webauthn_authenticator PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_webauthn_authenticator PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME WebAuthnAuthenticatorTests COMMAND test_webauthn_authenticator)

    set_tests_properties(WebAuthnAuthenticatorTests PROPERTIES
        LABELS "auth;webauthn;fido2;hardware-token;unit"
        TIMEOUT 120
    )

    message(STATUS "  WebAuthn/FIDO2: credential registration, assertion verification, CBOR, COSE keys")
else()
    message(STATUS "WebAuthn/FIDO2 Authenticator tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_webauthn_authenticator.cpp")
endif()

# Zero-Trust Auth Verifier Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_zero_trust_auth_verifier.cpp")
    message(STATUS "Adding Zero-Trust Auth Verifier tests")

    add_executable(test_zero_trust_auth_verifier
        test_zero_trust_auth_verifier.cpp
    )

    target_link_libraries(test_zero_trust_auth_verifier PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_zero_trust_auth_verifier PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ZeroTrustAuthVerifierTests COMMAND test_zero_trust_auth_verifier)

    set_tests_properties(ZeroTrustAuthVerifierTests PROPERTIES
        LABELS "auth;zero-trust;continuous-verification;unit"
        TIMEOUT 120
    )

    message(STATUS "  Zero-Trust Auth Verifier: continuous verification, device posture, risk scoring")
else()
    message(STATUS "Zero-Trust Auth Verifier tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_zero_trust_auth_verifier.cpp")
endif()

# Zero-Trust Policy Enforcer Focused Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_zero_trust_policy_enforcer.cpp")
    message(STATUS "Adding Zero-Trust Policy Enforcer focused tests")

    add_executable(test_zero_trust_policy_enforcer_focused
        test_zero_trust_policy_enforcer.cpp
    )

    target_link_libraries(test_zero_trust_policy_enforcer_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_zero_trust_policy_enforcer_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ZeroTrustPolicyEnforcerFocusedTests COMMAND test_zero_trust_policy_enforcer_focused)

    set_tests_properties(ZeroTrustPolicyEnforcerFocusedTests PROPERTIES
        LABELS "auth;zero-trust;policy;rbac;abac;unit"
        TIMEOUT 120
    )

    message(STATUS "  Zero-Trust Policy Enforcer: RBAC/ABAC policy evaluation, principal mapping, deny-by-default")
else()
    message(STATUS "Zero-Trust Policy Enforcer focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_zero_trust_policy_enforcer.cpp")
endif()

# ============================================================================
# Core Module – ConcernsContext Focused Tests (Issue #1419)
# Standalone target so the DI/observability layer can be validated in isolation
# without requiring the full themis_tests binary.
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_concerns_context.cpp")
    message(STATUS "Adding ConcernsContext focused tests (core module, Issue #1419)")

    add_executable(test_concerns_context_focused
        test_concerns_context.cpp
    )

    target_include_directories(test_concerns_context_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_concerns_context_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_concerns_context_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ConcernsContextFocusedTests COMMAND test_concerns_context_focused)

    set_tests_properties(ConcernsContextFocusedTests PROPERTIES
        LABELS "core;di;concerns;logger;tracer;metrics;cache;health;phase1;phase3;unit"
        TIMEOUT 120
    )

    message(STATUS "  ConcernsContextFocused: DI hub, ILogger/ITracer/IMetrics/ICache, factory methods, health checks, config-driven adapter selection")
else()
    message(STATUS "ConcernsContext focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_concerns_context.cpp")
endif()

# ============================================================================
# Core Module – Fuzz / Robustness Tests (Issue #1419)
# Exercises wire-protocol frame parser, CompressionAdvisor, PayloadBufferPool,
# WireProtocolMetrics, AbiChecker, LicenseData, and BuildInfo with adversarial
# and boundary-value inputs.
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_fuzz_core.cpp")
    message(STATUS "Adding Core Fuzz/Robustness tests (core module, Issue #1419)")

    add_executable(test_fuzz_core_focused
        test_fuzz_core.cpp
    )

    target_include_directories(test_fuzz_core_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_fuzz_core_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_fuzz_core_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME FuzzCoreFocusedTests COMMAND test_fuzz_core_focused)

    set_tests_properties(FuzzCoreFocusedTests PROPERTIES
        LABELS "core;fuzz;robustness;wire-protocol;license;build-info;security;unit"
        TIMEOUT 120
    )

    message(STATUS "  FuzzCoreFocused: WireV2 frame header, CompressionAdvisor, PayloadBufferPool, WireProtocolMetrics, AbiChecker, LicenseData, BuildInfo")
else()
    message(STATUS "Core Fuzz tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_fuzz_core.cpp")
endif()

# ============================================================================
# Security Sub-directory Focused Test Targets
# These provide dedicated standalone binaries for security module tests in
# tests/security/ and tests/security/attack-vectors/, enabling fast focused
# iteration without requiring a full themis_tests build.
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/security/test_security_evidence_collector.cpp")
    message(STATUS "Adding Security Evidence Collector focused tests")

    add_executable(test_security_evidence_collector_focused
        security/test_security_evidence_collector.cpp
    )

    target_include_directories(test_security_evidence_collector_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_security_evidence_collector_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_security_evidence_collector_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME SecurityEvidenceCollectorFocusedTests
             COMMAND test_security_evidence_collector_focused)

    set_tests_properties(SecurityEvidenceCollectorFocusedTests PROPERTIES
        LABELS "security;compliance;soc2;audit;focused;unit"
        TIMEOUT 120
    )

    message(STATUS "  SecurityEvidenceCollector: audit-log export, key-rotation records, RBAC report, retention enforcement")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/security/test_fips_crypto_mode.cpp")
    message(STATUS "Adding FIPS Crypto Mode focused tests")

    add_executable(test_fips_crypto_mode_focused
        security/test_fips_crypto_mode.cpp
    )

    target_include_directories(test_fips_crypto_mode_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_fips_crypto_mode_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_fips_crypto_mode_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME FipsCryptoModeFocusedTests
             COMMAND test_fips_crypto_mode_focused)

    set_tests_properties(FipsCryptoModeFocusedTests PROPERTIES
        LABELS "security;fips;crypto;focused;unit"
        TIMEOUT 60
    )

    message(STATUS "  FipsCryptoMode: FIPS activation, approved cipher suites, non-approved rejection")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/security/test_access_control_manager.cpp")
    message(STATUS "Adding Access Control Manager focused tests")

    add_executable(test_access_control_manager_focused
        security/test_access_control_manager.cpp
    )

    target_include_directories(test_access_control_manager_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_access_control_manager_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_access_control_manager_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME AccessControlManagerFocusedTests
             COMMAND test_access_control_manager_focused)

    set_tests_properties(AccessControlManagerFocusedTests PROPERTIES
        LABELS "security;access-control;rbac;focused;unit"
        TIMEOUT 120
    )

    message(STATUS "  AccessControlManager: RBAC enforcement, middleware integration, role hierarchy")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/security/test_row_level_security.cpp")
    message(STATUS "Adding Row Level Security focused tests")

    add_executable(test_row_level_security_focused
        security/test_row_level_security.cpp
    )

    target_include_directories(test_row_level_security_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_row_level_security_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_row_level_security_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME RowLevelSecurityFocusedTests
             COMMAND test_row_level_security_focused)

    set_tests_properties(RowLevelSecurityFocusedTests PROPERTIES
        LABELS "security;rls;row-level;policy;focused;unit"
        TIMEOUT 120
    )

    message(STATUS "  RowLevelSecurity: policy evaluation, predicate injection, AQL filter integration")
endif()

# Attack Vector Tests (security/attack-vectors/ sub-directory)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/security/attack-vectors/crypto/test_crypto_attack_vectors.cpp")
    message(STATUS "Adding Crypto Attack Vector tests")

    add_executable(test_crypto_attack_vectors
        security/attack-vectors/crypto/test_crypto_attack_vectors.cpp
    )

    target_include_directories(test_crypto_attack_vectors PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_crypto_attack_vectors PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_crypto_attack_vectors PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME CryptoAttackVectorTests COMMAND test_crypto_attack_vectors)

    set_tests_properties(CryptoAttackVectorTests PROPERTIES
        LABELS "security;attack-vector;crypto;iv-reuse;tag-tamper;unit"
        TIMEOUT 120
    )

    message(STATUS "  CryptoAttackVectors: IV/nonce reuse, tag tampering, bit-flip, key confusion, PQ key confusion, signature forgery")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/security/attack-vectors/injection/test_injection_attack_vectors.cpp")
    message(STATUS "Adding Injection Attack Vector tests")

    add_executable(test_injection_attack_vectors
        security/attack-vectors/injection/test_injection_attack_vectors.cpp
    )

    target_include_directories(test_injection_attack_vectors PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_injection_attack_vectors PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_injection_attack_vectors PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME InjectionAttackVectorTests COMMAND test_injection_attack_vectors)

    set_tests_properties(InjectionAttackVectorTests PROPERTIES
        LABELS "security;attack-vector;injection;aql;unit"
        TIMEOUT 120
    )

    message(STATUS "  InjectionAttackVectors: AQL injection (comment, dangerous ops, boolean-blind, union, stacked, case bypass, oversized)")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/security/attack-vectors/authentication/test_authentication_attack_vectors.cpp")
    message(STATUS "Adding Authentication Attack Vector tests")

    add_executable(test_authentication_attack_vectors
        security/attack-vectors/authentication/test_authentication_attack_vectors.cpp
    )

    target_include_directories(test_authentication_attack_vectors PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_authentication_attack_vectors PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_authentication_attack_vectors PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME AuthenticationAttackVectorTests COMMAND test_authentication_attack_vectors)

    set_tests_properties(AuthenticationAttackVectorTests PROPERTIES
        LABELS "security;attack-vector;authentication;rbac;privilege-escalation;unit"
        TIMEOUT 120
    )

    message(STATUS "  AuthenticationAttackVectors: privilege escalation, permission boundary, lateral movement, role injection")
endif()

# ============================================================================
# Geo Module – Focused Standalone Test Targets
# Each geo test is a standalone executable so it can be run independently
# without requiring a full themis_tests build.  All tests run in CPU-only
# simulation mode by default; CUDA/HIP variants are gated on THEMIS_GEO_CUDA
# and THEMIS_GEO_HIP respectively.
# ============================================================================

function(add_geo_focused_test target_name test_src test_name labels description)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${test_src}")
        message(STATUS "Adding Geo focused test: ${target_name}")

        add_executable(${target_name}
            ${test_src}
        )

        target_include_directories(${target_name} PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
        )

        target_link_libraries(${target_name} PRIVATE
            ${TEST_LIBS}
            themis_core
            Threads::Threads
        )

        if(TARGET themis_geo)
            target_link_libraries(${target_name} PRIVATE themis_geo)
        endif()

        target_compile_definitions(${target_name} PRIVATE
            THEMIS_TEST_BUILD=1
        )

        if(THEMIS_GEO_CUDA)
            target_compile_definitions(${target_name} PRIVATE THEMIS_GEO_CUDA)
        endif()

        if(THEMIS_GEO_HIP)
            target_compile_definitions(${target_name} PRIVATE THEMIS_GEO_HIP)
        endif()

        add_test(NAME ${test_name} COMMAND ${target_name})

        set_tests_properties(${test_name} PROPERTIES
            LABELS "${labels}"
            TIMEOUT 120
        )

        message(STATUS "  ${description}")
    else()
        message(STATUS "Geo focused test not found: ${CMAKE_CURRENT_SOURCE_DIR}/${test_src}")
    endif()
endfunction()

add_geo_focused_test(
    test_geo_rtree_focused
    geo/test_geo_rtree.cpp
    GeoRtreeFocusedTests
    "geo;rtree;spatial-index;focused;unit"
    "GeoRTree: insert, nearest-neighbor, range query, bulk insert, memory footprint"
)

add_geo_focused_test(
    test_geo_clustering_focused
    geo/test_geo_clustering.cpp
    GeoClusteringFocusedTests
    "geo;clustering;dbscan;kmeans;focused;unit"
    "GeoClustering: DBSCAN cluster detection, k-means convergence, noise point handling"
)

add_geo_focused_test(
    test_geo_raster_focused
    geo/test_geo_raster.cpp
    GeoRasterFocusedTests
    "geo;raster;elevation;heatmap;focused;unit"
    "GeoRaster: elevation sampling, bbox extraction, Gaussian KDE heatmap generation"
)

add_geo_focused_test(
    test_temporal_spatial_query_focused
    geo/test_temporal_spatial_query.cpp
    TemporalSpatialQueryFocusedTests
    "geo;temporal;spatial;query;focused;unit"
    "TemporalSpatialQuery: location at time T, entities within distance at time T"
)

add_geo_focused_test(
    test_geo_tile_server_focused
    geo/test_geo_tile_server.cpp
    GeoTileServerFocusedTests
    "geo;tile-server;mvt;focused;unit"
    "GeoTileServer: tile request handling, MVT encoding, zoom level routing"
)

add_geo_focused_test(
    test_geo_spatial_join_focused
    geo/test_geo_spatial_join.cpp
    GeoSpatialJoinFocusedTests
    "geo;spatial-join;distance;focused;unit"
    "GeoSpatialJoin: point-pair distance threshold, join result correctness"
)

add_geo_focused_test(
    test_geo_device_detector_focused
    geo/test_geo_device_detector.cpp
    GeoDeviceDetectorFocusedTests
    "geo;device-detector;gpu;capability;focused;unit"
    "GeoDeviceDetector: GPU discovery, compute-capability check, VRAM threshold"
)

add_geo_focused_test(
    test_geo_ewkb_focused
    geo/test_geo_ewkb.cpp
    GeoEwkbFocusedTests
    "geo;ewkb;serialization;focused;unit"
    "GeoEWKB: WKB/EWKB encode and decode round-trips for all geometry types"
)

add_geo_focused_test(
    test_geo_precision_mode_focused
    geo/test_geo_precision_mode.cpp
    GeoPrecisionModeFocusedTests
    "geo;precision;mode;backend;focused;unit"
    "GeoPrecisionMode: GeoPrecisionMode enum, getBackendForPrecision factory"
)

add_geo_focused_test(
    test_geo_st_buffer_focused
    geo/test_geo_st_buffer.cpp
    GeoStBufferFocusedTests
    "geo;st-buffer;geometry;focused;unit"
    "GeoSTBuffer: ST_BUFFER expand by fixed distance on CPU-exact and Boost backends"
)

add_geo_focused_test(
    test_geo_st_union_difference_focused
    geo/test_geo_st_union_difference.cpp
    GeoStUnionDifferenceFocusedTests
    "geo;st-union;st-difference;geometry;focused;unit"
    "GeoSTUnionDifference: ST_UNION and ST_DIFFERENCE operations, degenerate inputs"
)

add_geo_focused_test(
    test_rtree_cpu_integration_focused
    geo/test_rtree_cpu_integration.cpp
    RtreeCpuIntegrationFocusedTests
    "geo;rtree;cpu;integration;focused;unit"
    "RtreeCpuIntegration: R-tree backed CPU backend end-to-end spatial queries"
)

add_geo_focused_test(
    test_spatial_index_focused
    geo/test_spatial_index.cpp
    SpatialIndexFocusedTests
    "geo;spatial-index;s2;h3;focused;unit"
    "SpatialIndex: S2 cell lookup, H3 hexagonal grid binning, index consistency"
)

add_geo_focused_test(
    test_aql_st_functions_focused
    geo/test_aql_st_functions.cpp
    AqlStFunctionsFocusedTests
    "geo;aql;st-functions;focused;unit"
    "AqlStFunctions: ST_DISTANCE, ST_CONTAINS, ST_INTERSECTS, ST_BUFFER AQL integration"
)

add_geo_focused_test(
    test_aql_st_queryengine_focused
    geo/test_aql_st_queryengine.cpp
    AqlStQueryengineFocusedTests
    "geo;aql;query-engine;st-functions;focused;unit"
    "AqlStQueryengine: full AQL query execution with spatial predicates and geo index"
)

add_geo_focused_test(
    test_geo_3d_functions_focused
    geo/test_geo_3d_functions.cpp
    Geo3dFunctionsFocusedTests
    "geo;3d;spatial;focused;unit"
    "Geo3dFunctions: 3D containment, distance, intersection for point/polygon/polyhedron"
)

add_geo_focused_test(
    test_geo_wgs84_spherical_focused
    geo/test_geo_wgs84_spherical.cpp
    GeoWgs84SphericalFocusedTests
    "geo;wgs84;spherical;ellipsoid;focused;unit"
    "GeoWgs84Spherical: spherical WGS-84 distance calculations, ellipsoid approximation"
)

add_geo_focused_test(
    test_gpu_backend_production_focused
    geo/test_gpu_backend_production.cpp
    GpuBackendProductionFocusedTests
    "geo;gpu-backend;production;focused;unit"
    "GpuBackendProduction: production GPU backend dispatch, CPU fallback, circuit breaker"
)

add_geo_focused_test(
    test_gpu_kernel_dispatcher_focused
    geo/test_gpu_kernel_dispatcher.cpp
    GpuKernelDispatcherFocusedTests
    "geo;gpu-kernel;dispatcher;focused;unit"
    "GpuKernelDispatcher: kernel dispatch table, CPU fallback path, dispatch correctness"
)

if(THEMIS_GEO_CUDA)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/geo/test_cuda_geo_kernels.cpp")
        message(STATUS "Adding CUDA Geo Kernels focused tests (THEMIS_GEO_CUDA=ON)")

        add_executable(test_cuda_geo_kernels_focused
            geo/test_cuda_geo_kernels.cpp
        )

        target_include_directories(test_cuda_geo_kernels_focused PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
        )

        target_link_libraries(test_cuda_geo_kernels_focused PRIVATE
            ${TEST_LIBS}
            themis_core
            Threads::Threads
        )

        if(TARGET themis_geo)
            target_link_libraries(test_cuda_geo_kernels_focused PRIVATE themis_geo)
        endif()

        if(TARGET CUDA::cudart)
            target_link_libraries(test_cuda_geo_kernels_focused PRIVATE CUDA::cudart)
        endif()

        target_compile_definitions(test_cuda_geo_kernels_focused PRIVATE
            THEMIS_TEST_BUILD=1
            THEMIS_GEO_CUDA
        )

        add_test(NAME CudaGeoKernelsFocusedTests COMMAND test_cuda_geo_kernels_focused)

        set_tests_properties(CudaGeoKernelsFocusedTests PROPERTIES
            LABELS "geo;cuda;kernels;focused;unit"
            TIMEOUT 180  # GPU kernel init + device warm-up requires extra time
        )

        message(STATUS "  CudaGeoKernels: CUDA distance and containment kernel correctness vs CPU baseline")
    endif()
endif()

if(THEMIS_GEO_HIP)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/geo/test_hip_geo_kernels.cpp")
        message(STATUS "Adding HIP Geo Kernels focused tests (THEMIS_GEO_HIP=ON)")

        add_executable(test_hip_geo_kernels_focused
            geo/test_hip_geo_kernels.cpp
        )

        target_include_directories(test_hip_geo_kernels_focused PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
        )

        target_link_libraries(test_hip_geo_kernels_focused PRIVATE
            ${TEST_LIBS}
            themis_core
            Threads::Threads
        )

        if(TARGET themis_geo)
            target_link_libraries(test_hip_geo_kernels_focused PRIVATE themis_geo)
        endif()

        target_compile_definitions(test_hip_geo_kernels_focused PRIVATE
            THEMIS_TEST_BUILD=1
            THEMIS_GEO_HIP
        )

        add_test(NAME HipGeoKernelsFocusedTests COMMAND test_hip_geo_kernels_focused)

        set_tests_properties(HipGeoKernelsFocusedTests PROPERTIES
            LABELS "geo;hip;rocm;kernels;focused;unit"
            TIMEOUT 180  # GPU kernel init + device warm-up requires extra time
        )

        message(STATUS "  HipGeoKernels: ROCm/HIP distance and containment kernel correctness vs CPU baseline")
    endif()
endif()

# ============================================================================
# CHIMERA Adapter Focused Tests
# Each adapter test is a standalone executable so it can be run independently
# without needing the full themis_core link.  All adapters run in simulation
# mode – no live database server required.
# ============================================================================

# Common list of all CHIMERA adapter source files shared by every focused target.
set(CHIMERA_ADAPTER_SOURCES
    ${CMAKE_SOURCE_DIR}/src/chimera/adapter_factory.cpp
    ${CMAKE_SOURCE_DIR}/src/chimera/themisdb_adapter.cpp
    ${CMAKE_SOURCE_DIR}/src/chimera/mongodb_adapter.cpp
    ${CMAKE_SOURCE_DIR}/src/chimera/postgresql_adapter.cpp
    ${CMAKE_SOURCE_DIR}/src/chimera/elasticsearch_adapter.cpp
    ${CMAKE_SOURCE_DIR}/src/chimera/pinecone_adapter.cpp
    ${CMAKE_SOURCE_DIR}/src/chimera/qdrant_adapter.cpp
    ${CMAKE_SOURCE_DIR}/src/chimera/weaviate_adapter.cpp
    ${CMAKE_SOURCE_DIR}/src/chimera/neo4j_adapter.cpp
)

# Helper function to create a focused chimera test executable.
function(add_chimera_focused_test target_name test_src test_name labels description)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${test_src}")
        message(STATUS "Adding CHIMERA focused test: ${target_name}")

        add_executable(${target_name}
            ${test_src}
            ${CHIMERA_ADAPTER_SOURCES}
        )

        target_include_directories(${target_name} PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/tests/chimera
        )

        target_link_libraries(${target_name} PRIVATE
            ${TEST_LIBS}
            Threads::Threads
        )

        target_compile_definitions(${target_name} PRIVATE
            THEMIS_TEST_BUILD=1
        )

        add_test(NAME ${test_name} COMMAND ${target_name})

        set_tests_properties(${test_name} PROPERTIES
            LABELS "${labels}"
            TIMEOUT 60
        )

        message(STATUS "  ${description}")
    else()
        message(STATUS "CHIMERA focused test not found: ${CMAKE_CURRENT_SOURCE_DIR}/${test_src}")
    endif()
endfunction()

add_chimera_focused_test(
    test_chimera_adapter_factory
    chimera/test_adapter_factory.cpp
    ChimeraAdapterFactoryTests
    "chimera;adapter;factory;unit"
    "AdapterFactory: registration, create, list_supported, thread-safety"
)

add_chimera_focused_test(
    test_chimera_themisdb_adapter
    chimera/test_themisdb_adapter.cpp
    ChimeraThemisDBAdapterTests
    "chimera;adapter;themisdb;relational;vector;graph;document;unit"
    "ThemisDBAdapter: connect/disconnect, relational/vector/graph/document ops, transaction lifecycle"
)

add_chimera_focused_test(
    test_chimera_mongodb_adapter
    chimera/test_mongodb_adapter.cpp
    ChimeraMongoDBAdapterTests
    "chimera;adapter;mongodb;document;vector;unit"
    "MongoDBAdapter: document CRUD, Atlas Vector Search, transaction lifecycle, credential masking"
)

add_chimera_focused_test(
    test_chimera_postgresql_adapter
    chimera/test_postgresql_adapter.cpp
    ChimeraPostgreSQLAdapterTests
    "chimera;adapter;postgresql;relational;vector;document;unit"
    "PostgreSQLAdapter: relational CRUD, pgvector similarity, JSONB document store, transaction lifecycle"
)

add_chimera_focused_test(
    test_chimera_elasticsearch_adapter
    chimera/test_elasticsearch_adapter.cpp
    ChimeraElasticsearchAdapterTests
    "chimera;adapter;elasticsearch;fulltext;vector;unit"
    "ElasticsearchAdapter: full-text search, k-NN vector search, index management"
)

add_chimera_focused_test(
    test_chimera_pinecone_adapter
    chimera/test_pinecone_adapter.cpp
    ChimeraPineconeAdapterTests
    "chimera;adapter;pinecone;vector;unit"
    "PineconeAdapter: managed vector search, upsert, query, delete"
)

add_chimera_focused_test(
    test_chimera_qdrant_adapter
    chimera/test_qdrant_adapter.cpp
    ChimeraQdrantAdapterTests
    "chimera;adapter;qdrant;vector;unit"
    "QdrantAdapter: collections, vector search, payload filtering"
)

add_chimera_focused_test(
    test_chimera_weaviate_adapter
    chimera/test_weaviate_adapter.cpp
    ChimeraWeaviateAdapterTests
    "chimera;adapter;weaviate;vector;unit"
    "WeaviateAdapter: objects, semantic search, schema management"
)

add_chimera_focused_test(
    test_chimera_neo4j_adapter
    chimera/test_neo4j_adapter.cpp
    ChimeraNeo4jAdapterTests
    "chimera;adapter;neo4j;graph;cypher;unit"
    "Neo4jAdapter: Cypher queries, graph traversal, shortest path"
)

add_chimera_focused_test(
    test_chimera_capability_matrix
    chimera/test_capability_matrix.cpp
    ChimeraCapabilityMatrixTests
    "chimera;adapter;capability;matrix;unit"
    "AdapterCapabilityMatrix: cross-system capability comparison for all 9 adapters"
)

# ============================================================================
# Exporters Focused Test Targets
# Each target compiles the full exporter source set and links themis_core
# (when available) to resolve transitive dependencies (AQL parser, utils,
# OpenSSL, zlib/zstd, Arrow/Parquet).
# ============================================================================

set(EXPORTER_CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/exporters/jsonl_llm_exporter.cpp
    ${CMAKE_SOURCE_DIR}/src/exporters/format_template.cpp
    ${CMAKE_SOURCE_DIR}/src/exporters/exporter_metrics.cpp
    ${CMAKE_SOURCE_DIR}/src/exporters/pii_detector.cpp
    ${CMAKE_SOURCE_DIR}/src/exporters/stream_writer.cpp
    ${CMAKE_SOURCE_DIR}/src/exporters/parquet_exporter.cpp
    ${CMAKE_SOURCE_DIR}/src/exporters/streaming_exporter.cpp
    ${CMAKE_SOURCE_DIR}/src/exporters/aql_predicate_filter.cpp
    ${CMAKE_SOURCE_DIR}/src/exporters/arrow_ipc_exporter.cpp
    ${CMAKE_SOURCE_DIR}/src/exporters/incremental_exporter.cpp
    ${CMAKE_SOURCE_DIR}/src/exporters/export_encryption.cpp
    ${CMAKE_SOURCE_DIR}/src/exporters/huggingface_exporter.cpp
    ${CMAKE_SOURCE_DIR}/src/exporters/data_augmentation.cpp
    ${CMAKE_SOURCE_DIR}/src/exporters/export_format_registry.cpp
    ${CMAKE_SOURCE_DIR}/src/exporters/huggingface_hub_client.cpp
)

# Helper function to create an exporters focused test executable.
function(add_exporter_focused_test target_name test_src test_name labels description)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${test_src}")
        message(STATUS "Adding exporters focused test: ${target_name}")

        add_executable(${target_name}
            ${test_src}
            ${EXPORTER_CORE_SOURCES}
        )

        target_include_directories(${target_name} PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
        )

        # Link themis_core when available to satisfy transitive deps
        # (AQL parser/evaluator, utils, compression libraries, Arrow/Parquet).
        set(_exporter_test_libs ${TEST_LIBS} spdlog::spdlog nlohmann_json::nlohmann_json Threads::Threads)
        if(TARGET themis_core)
            list(APPEND _exporter_test_libs themis_core)
        endif()
        if(TARGET OpenSSL::Crypto)
            list(APPEND _exporter_test_libs OpenSSL::Crypto)
        endif()

        target_link_libraries(${target_name} PRIVATE ${_exporter_test_libs})

        target_compile_definitions(${target_name} PRIVATE
            THEMIS_TEST_BUILD=1
        )

        add_test(NAME ${test_name} COMMAND ${target_name})

        set_tests_properties(${test_name} PROPERTIES
            LABELS "${labels}"
            TIMEOUT 120
        )

        message(STATUS "  ${description}")
    else()
        message(STATUS "Exporters focused test not found: ${CMAKE_CURRENT_SOURCE_DIR}/${test_src}")
    endif()
endfunction()

add_exporter_focused_test(
    test_jsonl_llm_exporter_focused
    exporters/test_jsonl_llm_exporter.cpp
    JsonlLlmExporterFocusedTests
    "exporters;jsonl;llm;instruction-tuning;focused;unit"
    "JsonlLlmExporter: JSONL export, instruction-tuning formats, PII redaction, toxicity filtering, deduplication"
)

add_exporter_focused_test(
    test_huggingface_exporter_focused
    exporters/test_huggingface_exporter.cpp
    HuggingFaceExporterFocusedTests
    "exporters;huggingface;datasets;focused;unit"
    "HuggingFaceExporter: JSONL shards, dataset_card.md, dataset_info.json, field selection"
)

add_exporter_focused_test(
    test_parquet_exporter_focused
    exporters/test_parquet_exporter.cpp
    ParquetExporterFocusedTests
    "exporters;parquet;arrow;columnar;focused;unit"
    "ParquetExporter: columnar export, configurable Arrow schema, fallback writer"
)

add_exporter_focused_test(
    test_arrow_ipc_exporter_focused
    exporters/test_arrow_ipc_exporter.cpp
    ArrowIpcExporterFocusedTests
    "exporters;arrow;ipc;zero-copy;focused;unit"
    "ArrowIpcExporter: Arrow IPC file and stream format export"
)

add_exporter_focused_test(
    test_streaming_exporter_focused
    exporters/test_streaming_exporter.cpp
    StreamingExporterFocusedTests
    "exporters;streaming;progress;resumable;focused;unit"
    "StreamingExporter: large-collection streaming, progress callbacks, resumable export"
)

add_exporter_focused_test(
    test_incremental_exporter_focused
    exporters/test_incremental_exporter.cpp
    IncrementalExporterFocusedTests
    "exporters;incremental;delta;watermark;focused;unit"
    "IncrementalExporter: watermark-based delta export, checkpoint tracking"
)

add_exporter_focused_test(
    test_aql_predicate_filter_focused
    exporters/test_aql_predicate_filter.cpp
    AqlPredicateFilterFocusedTests
    "exporters;aql;predicate;filter;focused;unit"
    "AqlPredicateFilter: AQL-based record filtering before export"
)

add_exporter_focused_test(
    test_format_template_focused
    exporters/test_format_template.cpp
    FormatTemplateFocusedTests
    "exporters;format;template;alpaca;sharegpt;chatml;focused;unit"
    "FormatTemplate: Alpaca, ShareGPT, ChatML, OpenAI instruction-tuning transforms"
)

add_exporter_focused_test(
    test_export_encryption_focused
    exporters/test_export_encryption.cpp
    ExportEncryptionFocusedTests
    "exporters;encryption;aes-gcm;security;focused;unit"
    "ExportEncryption: AES-256-GCM encryption and decryption of export files"
)

add_exporter_focused_test(
    test_data_augmentation_focused
    exporters/test_data_augmentation.cpp
    DataAugmentationFocusedTests
    "exporters;augmentation;synthetic;diversity;focused;unit"
    "DataAugmentation: synthetic training data augmentation pipeline"
)

add_exporter_focused_test(
    test_export_format_registry_focused
    exporters/test_export_format_registry.cpp
    ExportFormatRegistryFocusedTests
    "exporters;registry;format;pluggable;focused;unit"
    "ExportFormatRegistry: singleton registry, registerBuiltins, createExporter, hasFormat, registeredFormats"
)

add_exporter_focused_test(
    test_huggingface_hub_client_focused
    exporters/test_huggingface_hub_client.cpp
    HuggingFaceHubClientFocusedTests
    "exporters;huggingface;hub;upload;libcurl;focused;unit"
    "HuggingFaceHubClient: token resolution, repo create, retry logic, error surfacing"
)
# Cache Interfaces Tests (Issue: #1596 — unit test coverage >80%)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_cache_interfaces.cpp")
    message(STATUS "Adding Cache Interfaces tests (Issue #1596)")

    add_executable(test_cache_interfaces
        test_cache_interfaces.cpp
    )

    target_include_directories(test_cache_interfaces PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_cache_interfaces PRIVATE
        ${TEST_LIBS}
        Threads::Threads
    )

    target_compile_definitions(test_cache_interfaces PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME CacheInterfacesTests COMMAND test_cache_interfaces)

    set_tests_properties(CacheInterfacesTests PROPERTIES
        LABELS "cache;interfaces;eviction;gdpr;ttl;warmup;unit"
        TIMEOUT 60
    )

    message(STATUS "  CacheInterfaces: IEvictionPolicy, ICacheAdminOps, ICacheWarmup, IGDPRPurgeHook, ITTLAdapter")
else()
    message(STATUS "Cache Interfaces tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_cache_interfaces.cpp")
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
