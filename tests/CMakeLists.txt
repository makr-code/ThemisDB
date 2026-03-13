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
        ".*test_rag_adversarial_tester\\.cpp$"
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
        # test_voice_assistant.cpp includes voice/voice_assistant.h which includes llm/llama_wrapper.h
        ".*test_voice_assistant\\.cpp$"
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
        # LLM feedback store – depends on llm/feedback_store.h / llm/i_feedback_plugin.h
        ".*test_feedback_store\\.cpp$"
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
        ".*test_cuda_ann_search\\.cpp$"
        ".*test_acceleration_coverage\\.cpp$"
    )
endif()

if(NOT THEMIS_ENABLE_CICADA)
    list(APPEND TEST_EXCLUDE_PATTERNS
        ".*test_cicada\\.cpp$"
    )
endif()

if(NOT THEMIS_ENABLE_RABITQ)
    list(APPEND TEST_EXCLUDE_PATTERNS
        ".*test_rabitq_product_quantizer\\.cpp$"
    )
endif()

if(NOT THEMIS_ENABLE_OPENGL)
    list(APPEND TEST_EXCLUDE_PATTERNS
        ".*test_opengl_backend\\.cpp$"
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
        ".*test_http3_production_readiness\\.cpp$"
    )
endif()

if(NOT THEMIS_ENABLE_POSTGRES_WIRE)
    list(APPEND TEST_EXCLUDE_PATTERNS
        ".*test_postgres_importer_.*\\.cpp$"
        ".*test_postgres_import_integration_v2\\.cpp$"
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
        ".*test_service_mesh_api_handler\\.cpp$"
        ".*test_envoy_xds\\.cpp$"
    )
endif()

if(NOT THEMIS_ENABLE_HTTP_SERVER)
    list(APPEND TEST_EXCLUDE_PATTERNS
        ".*test_task_scheduler\\.cpp$"
        ".*test_task_scheduler_api_handler\\.cpp$"
        ".*test_task_scheduler_dynamic_scaling\\.cpp$"
        ".*test_task_scheduler_siem_integration\\.cpp$"
        ".*test_task_scheduler_triggers\\.cpp$"
        ".*test_scheduler_integration\\.cpp$"
        ".*test_distributed_task_coordinator\\.cpp$"
        ".*test_task_audit\\.cpp$"
        ".*test_task_result_store\\.cpp$"
        ".*test_external_scheduler_adapter\\.cpp$"
        ".*test_chaos_scheduler\\.cpp$"
        ".*test_export_api_handler\\.cpp$"
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
        # test_ingestion_manager_adapter uses content/async_ingestion_worker.h
        ".*test_ingestion_manager_adapter\\.cpp$"
        # test_async_ingestion_backpressure uses AsyncIngestionWorker + ContentManager (CON-005)
        ".*test_async_ingestion_backpressure\\.cpp$"
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
    # Standalone test executables that define their own main() — incompatible with themis_tests
    ".*test_oauth2_provider\\.cpp$"
    ".*test_postgres_importer_v2\\.cpp$"
    ".*test_saml_auth_provider\\.cpp$"
    # Built as standalone focused target with explicit maintenance sources.
    ".*test_database_maintenance_orchestrator\\.cpp$"
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
    ".*test_cdc_retention\\.cpp$"
    ".*test_tenant_buffer_manager\\.cpp$"
    ".*test_real_embeddings\\.cpp$"
    ".*test_adapter_sync\\.cpp$"
    ".*test_fewshot_optimizer\\.cpp$"
    ".*test_flash_attention_correctness\\.cpp$"
    ".*test_geo_topology_api_handler\\.cpp$"
    ".*test_office_processor\\.cpp$"
    ".*test_ocr_processor\\.cpp$"
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
    ".*test_wasm_handler_registry\\.cpp$"
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
    list(FILTER _themis_tests_sources EXCLUDE REGEX ".*[\\/]src[\\/]importers[\\/]postgres_importer\\.cpp$")
    list(FILTER _themis_tests_sources EXCLUDE REGEX ".*[\\/]src[\\/]importers[\\/]postgres_importer_mdm\\.cpp$")
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

# Re-include additional prompt_engineering tests that have no LLM runtime
# dependency and are fully functional without THEMIS_ENABLE_LLM.  These files
# are caught by the broad ".*test_prompt_.*\\.cpp$" or ".*test_meta_prompt.*\\.cpp$"
# exclusion patterns added in the NOT THEMIS_ENABLE_LLM block, but they only
# use the prompt_engineering/ headers which link cleanly against themis_core.
target_sources(themis_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/test_prompt_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_prompt_version_control.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_prompt_optimizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_meta_prompt_generator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_prompt_performance_tracker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_prompt_engineering_metrics.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_self_improvement_orchestrator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_prompt_engineering_integration.cpp
)

# Re-include prompt_engineering tests that depend on the LLM subsystem (llm/
# headers) and are therefore only valid when THEMIS_ENABLE_LLM is ON.
if(THEMIS_ENABLE_LLM)
    target_sources(themis_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/test_prompt_evaluator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test_prompt_policy.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test_rag_prompt_injection.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test_feedback_store.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test_prompt_engineering_api_handler.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test_prompt_engineering_grpc_service.cpp
    )
endif()

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

# Provide CUDA backend symbols for test_acceleration.cpp / test_acceleration_dispatch.cpp
# when CUDA is not enabled.  The source compiles cleanly without the CUDA toolkit
# because all CUDA API calls are guarded by #ifdef THEMIS_ENABLE_CUDA;
# the non-CUDA paths provide CPU-fallback symbols with correct signatures.
if(NOT THEMIS_ENABLE_CUDA)
    if(EXISTS "${THEMIS_ROOT_DIR}/src/acceleration/cuda_backend.cpp")
        list(FIND THEMIS_CORE_SOURCES "${THEMIS_ROOT_DIR}/src/acceleration/cuda_backend.cpp" _cuda_src_idx)
        if(_cuda_src_idx EQUAL -1)
            target_sources(themis_tests PRIVATE
                "${THEMIS_ROOT_DIR}/src/acceleration/cuda_backend.cpp"
            )
        endif()
    endif()
endif()

# Provide graphics backend symbols (OpenGL, DirectX) for tests that reference them.
# When graphics APIs are disabled, graphics_backends.cpp still compiles with CPU-only
# stub implementations guarded by #ifdef THEMIS_ENABLE_OPENGL / THEMIS_ENABLE_DIRECTX.
if(NOT (THEMIS_ENABLE_OPENGL OR THEMIS_ENABLE_DIRECTX OR THEMIS_ENABLE_VULKAN))
    if(EXISTS "${THEMIS_ROOT_DIR}/src/acceleration/graphics_backends.cpp")
        list(FIND THEMIS_CORE_SOURCES "${THEMIS_ROOT_DIR}/src/acceleration/graphics_backends.cpp" _gfx_src_idx)
        if(_gfx_src_idx EQUAL -1)
            target_sources(themis_tests PRIVATE
                "${THEMIS_ROOT_DIR}/src/acceleration/graphics_backends.cpp"
            )
        endif()
    endif()
endif()

# Re-include the OpenGL backend test only when OpenGL is enabled.
# OpenGLVectorBackend symbols require THEMIS_ENABLE_OPENGL (graphics_backends.cpp).
if(THEMIS_ENABLE_OPENGL)
target_sources(themis_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/test_opengl_backend.cpp
)
endif()

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
# NOTE: Many of these files are now registered in cmake/CMakeLists.txt and cmake/ModularBuild.cmake.
# These target_sources entries serve as fallback for builds where the relevant feature flags are
# OFF (e.g., THEMIS_ENABLE_HTTP_SERVER=OFF) so tests still compile and link correctly.
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

# Re-include test_huggingface_exporter.cpp which was excluded as a missing-implementation.
# Now that huggingface_exporter.cpp is registered in cmake/CMakeLists.txt and
# cmake/ModularBuild.cmake the test can be compiled and linked normally.
target_sources(themis_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/exporters/test_huggingface_exporter.cpp
    ${CMAKE_SOURCE_DIR}/src/exporters/huggingface_exporter.cpp
)

# Ensure legal training tests link even when the feature is disabled in themis_core
if(NOT THEMIS_ENABLE_LEGAL_TRAINING)
    target_sources(themis_tests PRIVATE
        # NOTE: All ingestion sources are now unconditionally in THEMIS_CORE_SOURCES
        # (cmake/CMakeLists.txt). They are NO LONGER added here to avoid duplicate
        # symbols when linking against themis_core.
        ../src/training/auto_labeler.cpp
        ../src/training/knowledge_graph_enricher.cpp
        ../src/training/incremental_lora_trainer.cpp
        ../src/training/training_pipeline.cpp
        ../src/training/lora_data_selection.cpp
        ../src/training/lora_checkpoint_manager.cpp
        ../src/training/provenance_tracker.cpp
        ../src/training/modality_parser.cpp
        ../src/training/lora_adapter.cpp
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
        ../src/voice/voice_telephony.cpp
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
        if(NOT THEMIS_ENABLE_POSTGRES_WIRE)
            list(REMOVE_ITEM _themis_test_core_sources
                ../src/server/import_api_handler.cpp
                ${THEMIS_ROOT_DIR}/src/server/import_api_handler.cpp
                ${CMAKE_SOURCE_DIR}/src/server/import_api_handler.cpp
            )
        endif()
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
            ${THEMIS_ROOT_DIR}/src/index/approximate_radius_search.cpp
            ${THEMIS_ROOT_DIR}/src/index/graph_auto_buffer.cpp
            ${THEMIS_ROOT_DIR}/src/index/index_manager.cpp
            ${THEMIS_ROOT_DIR}/src/index/rotary_embeddings_gpu_cpu.cpp
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

        # Scheduler sources are compiled into themis_core only when THEMIS_ENABLE_HTTP_SERVER=ON.
        # Add them directly when HTTP_SERVER is OFF to avoid missing-symbol errors.
        if(NOT THEMIS_ENABLE_HTTP_SERVER)
            list(APPEND _themis_test_extra_sources
                ${THEMIS_ROOT_DIR}/src/server/task_scheduler_api_handler.cpp
                ${THEMIS_ROOT_DIR}/src/scheduler/task_scheduler.cpp
                ${THEMIS_ROOT_DIR}/src/scheduler/task_audit_event.cpp
                ${THEMIS_ROOT_DIR}/src/scheduler/task_audit_manager.cpp
                ${THEMIS_ROOT_DIR}/src/scheduler/task_result_store.cpp
                ${THEMIS_ROOT_DIR}/src/scheduler/task_anomaly_detector.cpp
                ${THEMIS_ROOT_DIR}/src/scheduler/hybrid_retention_manager.cpp
                ${THEMIS_ROOT_DIR}/src/scheduler/event_trigger.cpp
                ${THEMIS_ROOT_DIR}/src/scheduler/external_scheduler_adapter.cpp
                ${THEMIS_ROOT_DIR}/src/scheduler/distributed_task_coordinator.cpp
            )
        endif()

        # HttpServer routes maintenance endpoints through dedicated handlers.
        # Add these sources explicitly for the unified test binary.
        if(THEMIS_ENABLE_HTTP_SERVER)
            list(APPEND _themis_test_extra_sources
                ${THEMIS_ROOT_DIR}/src/maintenance/database_maintenance_orchestrator.cpp
                ${THEMIS_ROOT_DIR}/src/maintenance/maintenance_schedule_store.cpp
                ${THEMIS_ROOT_DIR}/src/maintenance/maintenance_registry.cpp
                ${THEMIS_ROOT_DIR}/src/server/maintenance_api_handler.cpp
            )
        endif()

        # Import API currently references PostgreSQL importer symbols directly.
        # Exclude the API source from unified tests when PostgreSQL wire support is OFF.
        if(NOT THEMIS_ENABLE_POSTGRES_WIRE)
            list(REMOVE_ITEM _themis_test_extra_sources
                ${THEMIS_ROOT_DIR}/src/server/import_api_handler.cpp
            )
        endif()

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
#         WireProtocolConstants, WireProtocolV1ThemisAuth/Get/Put/Delete/
#         Query/VectorSearch/GeoQuery/Timeseries/Bpmn/ErrorCodes
#         (from test_themis_wire_protocol_server.cpp)
add_test(NAME ThemisWireProtocolV1Tests
    COMMAND themis_tests --gtest_filter=WireV1Constants*:WireFrameHeader*:OpCode*:MessageFlags*:MessageDispatcher*:WireProtocolSession*:WireProtocolServer*:WireProtocolConstants*:WireProtocolV1Themis*)
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

# Module hash verifier tests (subset of AllTests)
# Covers: SHA-256 computation (empty/known/large files, non-existent),
#         verifyHash helper (match/mismatch/missing), manifest round-trip,
#         addExpectedHash / manifestSize / clearManifest / getExpectedHash,
#         verifyModule (success/mismatch/missing)
#         (from test_module_hash_verifier.cpp, Issue #2471)
add_test(NAME ModuleHashVerifierFocusedTests
    COMMAND themis_tests --gtest_filter=ModuleHashVerifier.*)
set_tests_properties(ModuleHashVerifierFocusedTests PROPERTIES
    LABELS "module;hash;verifier;security;themis;unit"
    TIMEOUT 60
)

# Module signature verifier tests (subset of AllTests)
# Covers: non-existent file, platform field, error on failure,
#         GPG (no sig file, explicit non-existent sig, bad content, auto-detect .asc),
#         Authenticode unsigned/non-existent, Windows unsigned module
#         (from test_module_signature_verifier.cpp, Issue #2473)
add_test(NAME ModuleSignatureVerifierFocusedTests
    COMMAND themis_tests --gtest_filter=ModuleSignatureVerifier.*)
set_tests_properties(ModuleSignatureVerifierFocusedTests PROPERTIES
    LABELS "module;signature;verifier;security;themis;unit"
    TIMEOUT 60
)

# Module dependency resolver tests (subset of AllTests)
# Covers: ModuleDependency metadata, DependencyResolutionResult, ModuleErrorCode,
#         ModuleMetadata, ModuleDependencyResolver (topological sort, version compat,
#         cycle detection, registerModule/resolve/resolveFor/clear)
#         (from test_module_dependency_resolver.cpp, Issue #2474)
add_test(NAME ModuleDependencyResolverFocusedTests
    COMMAND themis_tests --gtest_filter=ModuleDependency.*:DependencyResolutionResult.*:ModuleErrorCode.*:ModuleMetadata.*:ModuleDependencyResolver.*)
set_tests_properties(ModuleDependencyResolverFocusedTests PROPERTIES
    LABELS "module;dependency;resolver;themis;unit"
    TIMEOUT 60
)

# Module Loader core tests (subset of AllTests)
# Covers: ModuleErrorCode enum values/ranges, ErrorCategory values,
#         ModuleMetadata defaults/isValid, ModuleVerificationResult defaults,
#         LoadedModule defaults, ModuleRegistry singleton/register/unregister,
#         error message strings, error categorization, hash manifest
#         (from test_module_loader.cpp)
add_test(NAME ModuleLoaderFocusedTests
    COMMAND themis_tests --gtest_filter=ModuleLoader.*:ModuleRegistry.*:ModuleMetadata.*:ModuleVerificationResult.*:LoadedModule.*)
set_tests_properties(ModuleLoaderFocusedTests PROPERTIES
    LABELS "module;loader;security;themis;unit"
    TIMEOUT 60
)

# Plugin Watchdog tests (subset of AllTests)
# Covers: WatchdogConfig defaults/custom values/zero-max-restarts,
#         WatchdogModuleStats defaults/field completeness, lifecycle
#         (start/stop/isRunning), idempotent start/stop, configureWatchdog,
#         getWatchdogStats/getAllWatchdogStats, resetWatchdogStats (incl.
#         while running), disabled watchdog, destructor cleanup
#         (from test_module_loader.cpp, Issue #2373)
add_test(NAME PluginWatchdogFocusedTests
    COMMAND themis_tests --gtest_filter=WatchdogConfig.*:WatchdogModuleStats.*:PluginWatchdog.*)
set_tests_properties(PluginWatchdogFocusedTests PROPERTIES
    LABELS "module;watchdog;health;restart;themis;unit"
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

# Replication HA Focused Tests (Phase 1-3 production-readiness)
# Covers: config validation, WAL checksum, LWW/CRDT resolvers, quorum-based
# leader election, thread-safe replica list, error handling, witness nodes.
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_replication_ha.cpp")
    message(STATUS "Adding Replication HA focused tests")

    add_executable(test_replication_ha_focused
        test_replication_ha.cpp
    )

    target_include_directories(test_replication_ha_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_replication_ha_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_replication_ha_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ReplicationHAFocusedTests COMMAND test_replication_ha_focused)

    set_tests_properties(ReplicationHAFocusedTests PROPERTIES
        LABELS "replication;ha;witness-node;conflict-resolution;focused;unit"
        TIMEOUT 120
    )

    message(STATUS "  ReplicationHA: config validation, WAL checksum, LWW/CRDT, quorum leader election, witness nodes")
else()
    message(STATUS "Replication HA focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_replication_ha.cpp")
endif()

# Replication New Features Focused Tests (v1.7.0 modules)
# Covers: ReplicationObserver, ThreeWayMergeResolver, FieldLevelMergeResolver,
# ReplicationEventStream, ReplicationPolicy, ReplicationSlot/Manager.
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_replication_new_features.cpp")
    message(STATUS "Adding Replication New Features focused tests")

    add_executable(test_replication_new_features_focused
        test_replication_new_features.cpp
    )

    target_include_directories(test_replication_new_features_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_replication_new_features_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_replication_new_features_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ReplicationNewFeaturesFocusedTests
             COMMAND test_replication_new_features_focused)

    set_tests_properties(ReplicationNewFeaturesFocusedTests PROPERTIES
        LABELS "replication;v1.7.0;observer;conflict;event-stream;policy;slot;focused;unit"
        TIMEOUT 120
    )

    message(STATUS "  ReplicationNewFeatures: observer, 3-way merge, field merge, event stream, policy, slot manager")
else()
    message(STATUS "Replication New Features focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_replication_new_features.cpp")
endif()

# ============================================================================
# Replication HA Tests (Phase 1-3 – leader election, witness nodes, WAL)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_replication_ha.cpp")
    message(STATUS "Adding Replication HA tests")

    add_executable(test_replication_ha
        test_replication_ha.cpp
    )

    target_link_libraries(test_replication_ha PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
        zstd::zstd
    )

    target_compile_definitions(test_replication_ha PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ReplicationHATests COMMAND test_replication_ha)

    set_tests_properties(ReplicationHATests PROPERTIES
        LABELS "replication;ha;leader-election;witness;wal;unit"
        TIMEOUT 120
    )

    message(STATUS "  Replication HA: config validation, WAL checksum, LWW/CRDT resolvers, quorum, witness nodes")
else()
    message(STATUS "Replication HA tests not found")
endif()

# ============================================================================
# Replication New Features Tests (v1.7.0 – observer, slots, policy, events)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_replication_new_features.cpp")
    message(STATUS "Adding Replication New Features tests")

    add_executable(test_replication_new_features
        test_replication_new_features.cpp
    )

    target_link_libraries(test_replication_new_features PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_replication_new_features PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ReplicationNewFeaturesTests COMMAND test_replication_new_features)

    set_tests_properties(ReplicationNewFeaturesTests PROPERTIES
        LABELS "replication;observer;slots;policy;event-stream;unit"
        TIMEOUT 60
    )

    message(STATUS "  Replication New Features: observer lag/topology, ThreeWayMerge, FieldLevelMerge, EventStream, slots")
else()
    message(STATUS "Replication New Features tests not found")
endif()

# ============================================================================
# Replication Topology API Handler Tests (web UI / REST endpoint)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_replication_topology_api_handler.cpp" AND
   EXISTS "${CMAKE_SOURCE_DIR}/src/server/replication_topology_api_handler.cpp")
    message(STATUS "Adding Replication Topology API Handler tests")

    add_executable(test_replication_topology_api_handler
        test_replication_topology_api_handler.cpp
        ${CMAKE_SOURCE_DIR}/src/server/replication_topology_api_handler.cpp
    )

    target_link_libraries(test_replication_topology_api_handler PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_replication_topology_api_handler PRIVATE
        THEMIS_TEST_BUILD=1
        THEMIS_ENABLE_HTTP_SERVER=1
    )

    add_test(NAME ReplicationTopologyAPITests COMMAND test_replication_topology_api_handler)

    set_tests_properties(ReplicationTopologyAPITests PROPERTIES
        LABELS "replication;topology;rest;web-ui;unit"
        TIMEOUT 30
    )

    message(STATUS "  Replication Topology API: 503 on no coordinator, JSON topology, health summary, HTML visualizer")
else()
    message(STATUS "Replication Topology API Handler tests not found")
endif()

# ============================================================================
# Replication Raft v2 Tests (Phase 4 – joint consensus membership changes)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_replication_raft_v2.cpp")
    message(STATUS "Adding Replication Raft v2 tests")

    add_executable(test_replication_raft_v2
        test_replication_raft_v2.cpp
    )

    target_link_libraries(test_replication_raft_v2 PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_replication_raft_v2 PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ReplicationRaftV2Tests COMMAND test_replication_raft_v2)

    set_tests_properties(ReplicationRaftV2Tests PROPERTIES
        LABELS "replication;raft;v2;joint-consensus;membership;unit"
        TIMEOUT 30
    )

    message(STATUS "  Replication Raft v2: joint consensus, quorum, add/remove member, MCManager lifecycle")
else()
    message(STATUS "Replication Raft v2 tests not found")
endif()

# ============================================================================
# Replication CRDT Types Tests (Phase 4 – expanded CRDT library)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_replication_crdt_types.cpp")
    message(STATUS "Adding Replication CRDT Types tests")

    add_executable(test_replication_crdt_types
        test_replication_crdt_types.cpp
    )

    target_link_libraries(test_replication_crdt_types PRIVATE
        ${TEST_LIBS}
        Threads::Threads
    )

    target_compile_definitions(test_replication_crdt_types PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ReplicationCRDTTypesTests COMMAND test_replication_crdt_types)

    set_tests_properties(ReplicationCRDTTypesTests PROPERTIES
        LABELS "replication;crdt;counter;register;set;map;sequence;flag;unit"
        TIMEOUT 30
    )

    message(STATUS "  Replication CRDT Types: GCounter, PNCounter, LWWRegister, MVRegister, GSet, 2PSet, ORSet, LWWMap, RGA, EW/DW-Flag")
else()
    message(STATUS "Replication CRDT Types tests not found")
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
# WebAssembly Handler Registry Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_wasm_handler_registry.cpp")
    message(STATUS "Adding WebAssembly Handler Registry tests")

    add_executable(test_wasm_handler_registry
        test_wasm_handler_registry.cpp
        ${THEMIS_ROOT_DIR}/src/server/wasm_handler_registry.cpp
        ${THEMIS_ROOT_DIR}/src/base/wasm_plugin_sandbox.cpp
        ${THEMIS_ROOT_DIR}/src/base/module_sandbox.cpp
    )

    target_include_directories(test_wasm_handler_registry PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_wasm_handler_registry PRIVATE
        ${TEST_LIBS}
        Threads::Threads
    )

    target_compile_definitions(test_wasm_handler_registry PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME WasmHandlerRegistryTests
             COMMAND test_wasm_handler_registry)

    set_tests_properties(WasmHandlerRegistryTests PROPERTIES
        LABELS "server;wasm;edge;unit"
        TIMEOUT 60
    )

    message(STATUS "  WASM Handler Registry: upload, list, get, delete, invoke, error handling")
else()
    message(STATUS "WebAssembly Handler Registry tests not found")
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

# Transaction Manager Focused Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_transaction_manager.cpp")
    message(STATUS "Adding Transaction Manager focused tests")

    add_executable(test_transaction_manager_focused
        test_transaction_manager.cpp
    )

    target_link_libraries(test_transaction_manager_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        RocksDB::rocksdb
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_transaction_manager_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME TransactionManagerFocusedTests COMMAND test_transaction_manager_focused)

    set_tests_properties(TransactionManagerFocusedTests PROPERTIES
        LABELS "transaction;mvcc;acid;occ;explain;unit"
        TIMEOUT 120
    )

    message(STATUS "  TransactionManager: ACID commit/rollback, graph/vector ops, isolation, statistics, explain")
else()
    message(STATUS "Transaction Manager focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_transaction_manager.cpp")
endif()

# Transaction Isolation Levels Focused Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_transaction_isolation_levels.cpp")
    message(STATUS "Adding Transaction Isolation Levels focused tests")

    add_executable(test_transaction_isolation_levels_focused
        test_transaction_isolation_levels.cpp
    )

    target_link_libraries(test_transaction_isolation_levels_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_transaction_isolation_levels_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME TransactionIsolationLevelsFocusedTests COMMAND test_transaction_isolation_levels_focused)

    set_tests_properties(TransactionIsolationLevelsFocusedTests PROPERTIES
        LABELS "transaction;isolation;lock;2pl;intent-lock;unit"
        TIMEOUT 60
    )

    message(STATUS "  TransactionIsolationLevels: enum correctness, LockManager RC/RR/SZ compliance, 2PL, intent locks")
else()
    message(STATUS "Transaction Isolation Levels focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_transaction_isolation_levels.cpp")
endif()

# SAGA Logger Focused Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_saga_logger.cpp")
    message(STATUS "Adding SAGA Logger focused tests")

    add_executable(test_saga_logger_focused
        test_saga_logger.cpp
    )

    target_link_libraries(test_saga_logger_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_saga_logger_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME SAGALoggerFocusedTests COMMAND test_saga_logger_focused)

    set_tests_properties(SAGALoggerFocusedTests PROPERTIES
        LABELS "utils;saga;logger;encryption;signing;unit"
        TIMEOUT 60
    )

    message(STATUS "  SAGALogger: signed batch flush, batch verification, encrypted step load, empty flush")
else()
    message(STATUS "SAGA Logger focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_saga_logger.cpp")
endif()

# SAGA Compactor Focused Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_saga_compactor.cpp")
    message(STATUS "Adding SAGA Compactor focused tests")

    add_executable(test_saga_compactor_focused
        test_saga_compactor.cpp
    )

    target_link_libraries(test_saga_compactor_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_saga_compactor_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME SAGACompactorFocusedTests COMMAND test_saga_compactor_focused)

    set_tests_properties(SAGACompactorFocusedTests PROPERTIES
        LABELS "utils;saga;compactor;wal;replay;unit"
        TIMEOUT 60
    )

    message(STATUS "  SAGACompactor: compact WAL, archive completed entries, replay pending compensations")
else()
    message(STATUS "SAGA Compactor focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_saga_compactor.cpp")
endif()

# HashChainAudit Focused Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_hash_chain_audit.cpp")
    message(STATUS "Adding HashChainAudit focused tests")

    add_executable(test_hash_chain_audit_focused
        test_hash_chain_audit.cpp
    )

    target_link_libraries(test_hash_chain_audit_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::Crypto
        Threads::Threads
    )

    target_compile_definitions(test_hash_chain_audit_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME HashChainAuditFocusedTests COMMAND test_hash_chain_audit_focused)

    set_tests_properties(HashChainAuditFocusedTests PROPERTIES
        LABELS "utils;audit;hash-chain;tamper-evidence;unit"
        TIMEOUT 60
    )

    message(STATUS "  HashChainAudit: write, sequence increment, head persistence, tamper detection")
else()
    message(STATUS "HashChainAudit focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_hash_chain_audit.cpp")
endif()

# PIIStreamScanner Focused Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_pii_stream_scanner.cpp")
    message(STATUS "Adding PIIStreamScanner focused tests")

    add_executable(test_pii_stream_scanner_focused
        test_pii_stream_scanner.cpp
    )

    target_link_libraries(test_pii_stream_scanner_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_pii_stream_scanner_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME PIIStreamScannerFocusedTests COMMAND test_pii_stream_scanner_focused)

    set_tests_properties(PIIStreamScannerFocusedTests PROPERTIES
        LABELS "utils;pii;streaming;redaction;unit"
        TIMEOUT 60
    )

    message(STATUS "  PIIStreamScanner: single-chunk scan, cross-boundary detection, reset, bytes_processed")
else()
    message(STATUS "PIIStreamScanner focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_pii_stream_scanner.cpp")
endif()

# SampledLogger Focused Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_sampled_logger.cpp")
    message(STATUS "Adding SampledLogger focused tests")

    add_executable(test_sampled_logger_focused
        test_sampled_logger.cpp
    )

    target_link_libraries(test_sampled_logger_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_sampled_logger_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME SampledLoggerFocusedTests COMMAND test_sampled_logger_focused)

    set_tests_properties(SampledLoggerFocusedTests PROPERTIES
        LABELS "utils;logging;sampling;rate-limiting;unit"
        TIMEOUT 60
    )

    message(STATUS "  SampledLogger: construction, suppression counter, token-bucket sampling")
else()
    message(STATUS "SampledLogger focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_sampled_logger.cpp")
endif()

# TimestampUtils Focused Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_timestamp_utils.cpp")
    message(STATUS "Adding TimestampUtils focused tests")

    add_executable(test_timestamp_utils_focused
        test_timestamp_utils.cpp
    )

    target_link_libraries(test_timestamp_utils_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_timestamp_utils_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME TimestampUtilsFocusedTests COMMAND test_timestamp_utils_focused)

    set_tests_properties(TimestampUtilsFocusedTests PROPERTIES
        LABELS "utils;timestamp;iso8601;rfc3339;unit"
        TIMEOUT 30
    )

    message(STATUS "  TimestampUtils: ISO 8601 format/parse round-trip, milliseconds, duration formatting")
else()
    message(STATUS "TimestampUtils focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_timestamp_utils.cpp")
endif()

# UtilsRateLimiter Focused Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_utils_rate_limiter.cpp")
    message(STATUS "Adding UtilsRateLimiter focused tests")

    add_executable(test_utils_rate_limiter_focused
        test_utils_rate_limiter.cpp
    )

    target_link_libraries(test_utils_rate_limiter_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_utils_rate_limiter_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME UtilsRateLimiterFocusedTests COMMAND test_utils_rate_limiter_focused)

    set_tests_properties(UtilsRateLimiterFocusedTests PROPERTIES
        LABELS "utils;rate-limiter;token-bucket;concurrency;unit"
        TIMEOUT 30
    )

    message(STATUS "  UtilsRateLimiter: try_acquire, acquire blocking, set_rate, available")
else()
    message(STATUS "UtilsRateLimiter focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_utils_rate_limiter.cpp")
endif()

# RateLimiterV2 Focused Tests (TokenBucketRateLimiter + PerClientRateLimiter + Redis fallback)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_rate_limiter_v2.cpp")
    message(STATUS "Adding RateLimiterV2 focused tests")

    add_executable(test_rate_limiter_v2_focused
        test_rate_limiter_v2.cpp
    )

    target_link_libraries(test_rate_limiter_v2_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_rate_limiter_v2_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    if(THEMIS_ENABLE_REDIS AND hiredis_FOUND)
        if(TARGET hiredis::hiredis)
            target_link_libraries(test_rate_limiter_v2_focused PRIVATE hiredis::hiredis)
        elseif(TARGET hiredis)
            target_link_libraries(test_rate_limiter_v2_focused PRIVATE hiredis)
        endif()
        target_compile_definitions(test_rate_limiter_v2_focused PRIVATE THEMIS_ENABLE_REDIS=1)
    endif()

    add_test(NAME RateLimiterV2FocusedTests COMMAND test_rate_limiter_v2_focused)

    set_tests_properties(RateLimiterV2FocusedTests PROPERTIES
        LABELS "server;rate-limiter;token-bucket;redis;fallback;unit"
        TIMEOUT 30
    )

    message(STATUS "  RateLimiterV2: local bucket, Redis fallback, per-client, priority lanes")
else()
    message(STATUS "RateLimiterV2 focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_rate_limiter_v2.cpp")
endif()

# Rate Limiting Improvements Focused Tests (adaptive + cost-based)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_rate_limiting_improvements.cpp")
    message(STATUS "Adding RateLimitingImprovements focused tests")

    add_executable(test_rate_limiting_improvements_focused
        test_rate_limiting_improvements.cpp
    )

    target_link_libraries(test_rate_limiting_improvements_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_rate_limiting_improvements_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME RateLimitingImprovementsFocusedTests
             COMMAND test_rate_limiting_improvements_focused)

    set_tests_properties(RateLimitingImprovementsFocusedTests PROPERTIES
        LABELS "server;rate-limiter;adaptive;cost-based;unit"
        TIMEOUT 30
    )

    message(STATUS "  RateLimitingImprovements: adaptive limiter, cost-based limiter")
else()
    message(STATUS "RateLimitingImprovements focused tests not found")
endif()

# UtilsStandalone Focused Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_utils_standalone.cpp")
    message(STATUS "Adding UtilsStandalone focused tests")

    add_executable(test_utils_standalone_focused
        test_utils_standalone.cpp
    )

    target_link_libraries(test_utils_standalone_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::Crypto
        Threads::Threads
    )

    target_compile_definitions(test_utils_standalone_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME UtilsStandaloneFocusedTests COMMAND test_utils_standalone_focused)

    set_tests_properties(UtilsStandaloneFocusedTests PROPERTIES
        LABELS "utils;checksum;file;normalizer;feature-flags;unit"
        TIMEOUT 30
    )

    message(STATUS "  UtilsStandalone: calculateSHA256, calculateMD5, readFileContents, Normalizer, Phase2FeatureFlags")
else()
    message(STATUS "UtilsStandalone focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_utils_standalone.cpp")
endif()

# Sharding Transaction WAL Focused Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_sharding_transaction_wal.cpp")
    message(STATUS "Adding Sharding Transaction WAL focused tests")

    add_executable(test_sharding_transaction_wal_focused
        test_sharding_transaction_wal.cpp
        ${CMAKE_SOURCE_DIR}/src/sharding/transaction_wal.cpp
    )

    target_link_libraries(test_sharding_transaction_wal_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_sharding_transaction_wal_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ShardingTransactionWALFocusedTests COMMAND test_sharding_transaction_wal_focused)

    set_tests_properties(ShardingTransactionWALFocusedTests PROPERTIES
        LABELS "transaction;sharding;wal;2pc;saga;unit"
        TIMEOUT 60
    )

    message(STATUS "  ShardingTransactionWAL: initialize, logBegin/Prepare/Commit/Abort/Compensate LSNs, config defaults, protocol values")
else()
    message(STATUS "Sharding Transaction WAL focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_sharding_transaction_wal.cpp")
endif()

# Multi-Shard Transaction Focused Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_multi_shard_transactions.cpp")
    message(STATUS "Adding Multi-Shard Transaction focused tests")

    add_executable(test_multi_shard_transactions_focused
        test_multi_shard_transactions.cpp
    )

    target_link_libraries(test_multi_shard_transactions_focused PRIVATE
        ${TEST_LIBS}
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_multi_shard_transactions_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME MultiShardTransactionFocusedTests COMMAND test_multi_shard_transactions_focused)

    set_tests_properties(MultiShardTransactionFocusedTests PROPERTIES
        LABELS "transaction;sharding;2pc;distributed;unit"
        TIMEOUT 120
    )

    message(STATUS "  MultiShardTransaction: 2PC, rollback on prepare failure, concurrent transactions, cross-shard consistency, deadlock detection")
else()
    message(STATUS "Multi-Shard Transaction focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_multi_shard_transactions.cpp")
endif()

# Distributed Transactions Focused Tests (disabled placeholder)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_distributed_transactions.cpp")
    message(STATUS "Adding Distributed Transactions focused tests")

    add_executable(test_distributed_transactions_focused
        test_distributed_transactions.cpp
    )

    target_link_libraries(test_distributed_transactions_focused PRIVATE
        ${TEST_LIBS}
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_distributed_transactions_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME DistributedTransactionsFocusedTests COMMAND test_distributed_transactions_focused)

    set_tests_properties(DistributedTransactionsFocusedTests PROPERTIES
        LABELS "transaction;distributed;2pc;unit"
        TIMEOUT 30
    )

    message(STATUS "  DistributedTransactions: placeholder (full suite gated behind gRPC shard transport)")
else()
    message(STATUS "Distributed Transactions focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_distributed_transactions.cpp")
endif()

# Postgres Transaction Focused Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_postgres_transactions.cpp")
    message(STATUS "Adding Postgres Transaction focused tests")

    add_executable(test_postgres_transactions_focused
        test_postgres_transactions.cpp
    )

    target_link_libraries(test_postgres_transactions_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_postgres_transactions_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME PostgresTransactionFocusedTests COMMAND test_postgres_transactions_focused)

    set_tests_properties(PostgresTransactionFocusedTests PROPERTIES
        LABELS "transaction;postgres;wire;2pc;savepoint;unit"
        TIMEOUT 60
    )

    message(STATUS "  PostgresTransaction: transaction state, BEGIN/COMMIT/ROLLBACK, savepoints, 2PC prepare/commit/rollback (POSTGRES_WIRE)")
else()
    message(STATUS "Postgres Transaction focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_postgres_transactions.cpp")
endif()

# AQL Multi-Statement Transaction Focused Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_aql_multi_statement_transaction.cpp")
    message(STATUS "Adding AQL Multi-Statement Transaction focused tests")

    add_executable(test_aql_multi_statement_transaction_focused
        test_aql_multi_statement_transaction.cpp
    )

    target_link_libraries(test_aql_multi_statement_transaction_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        RocksDB::rocksdb
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_aql_multi_statement_transaction_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME AQLMultiStatementTransactionFocusedTests COMMAND test_aql_multi_statement_transaction_focused)

    set_tests_properties(AQLMultiStatementTransactionFocusedTests PROPERTIES
        LABELS "transaction;aql;multi-statement;begin;commit;rollback;unit"
        TIMEOUT 120
    )

    message(STATUS "  AQLMultiStatementTransaction: BEGIN/COMMIT/ROLLBACK parse, multi-statement batching, query engine integration")
else()
    message(STATUS "AQL Multi-Statement Transaction focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_aql_multi_statement_transaction.cpp")
endif()

# DB Transaction Isolation Focused Tests (tests/db/ subdirectory)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/db/test_transaction_isolation.cpp")
    message(STATUS "Adding DB Transaction Isolation focused tests")

    add_executable(test_db_transaction_isolation_focused
        db/test_transaction_isolation.cpp
    )

    target_link_libraries(test_db_transaction_isolation_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        RocksDB::rocksdb
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_db_transaction_isolation_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME DbTransactionIsolationFocusedTests COMMAND test_db_transaction_isolation_focused)

    set_tests_properties(DbTransactionIsolationFocusedTests PROPERTIES
        LABELS "transaction;isolation;mvcc;snapshot;deadlock;unit"
        TIMEOUT 180
    )

    message(STATUS "  DbTransactionIsolation: ReadCommitted/Snapshot isolation, concurrent transactions, deadlock detection, rollback consistency")
else()
    message(STATUS "DB Transaction Isolation focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/db/test_transaction_isolation.cpp")
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

# OpenTelemetry API-path tracing tests (all major API paths)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_otel_api_tracing.cpp")
    message(STATUS "Adding OpenTelemetry API-path tracing tests")
    add_executable(test_otel_api_tracing test_otel_api_tracing.cpp)
    target_link_libraries(test_otel_api_tracing PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
        CURL::libcurl
    )
    target_compile_definitions(test_otel_api_tracing PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME OtelApiTracingTests COMMAND test_otel_api_tracing)
    set_tests_properties(OtelApiTracingTests PROPERTIES
        LABELS "api;tracing;otel;distributed;spans;integration;unit"
        TIMEOUT 60
    )
    message(STATUS "  OTel API Tracing: span names, attributes, W3C propagation, OtlpExporter, TracingMiddleware")
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

# Export API Handler — PolicyEngine authorization enforcement unit tests (EXP-001)
if(THEMIS_ENABLE_HTTP_SERVER AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_export_api_handler.cpp")
    message(STATUS "Adding ExportApiHandler focused tests (EXP-001)")
    add_executable(test_export_api_handler_focused
        test_export_api_handler.cpp
        ${CMAKE_SOURCE_DIR}/src/server/export_api_handler.cpp
        ${CMAKE_SOURCE_DIR}/src/exporters/jsonl_llm_exporter.cpp
        ${CMAKE_SOURCE_DIR}/src/exporters/format_template.cpp
        ${CMAKE_SOURCE_DIR}/src/exporters/exporter_metrics.cpp
        ${CMAKE_SOURCE_DIR}/src/exporters/pii_detector.cpp
        ${CMAKE_SOURCE_DIR}/src/exporters/stream_writer.cpp
        ${CMAKE_SOURCE_DIR}/src/exporters/aql_predicate_filter.cpp
    )
    target_include_directories(test_export_api_handler_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )
    set(_export_api_test_libs ${TEST_LIBS} spdlog::spdlog nlohmann_json::nlohmann_json Threads::Threads)
    if(TARGET themis_core)
        list(APPEND _export_api_test_libs themis_core)
    endif()
    if(TARGET OpenSSL::Crypto)
        list(APPEND _export_api_test_libs OpenSSL::Crypto)
    endif()
    target_link_libraries(test_export_api_handler_focused PRIVATE ${_export_api_test_libs})
    target_compile_definitions(test_export_api_handler_focused PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME ExportApiHandlerFocusedTests COMMAND test_export_api_handler_focused)
    set_tests_properties(ExportApiHandlerFocusedTests PROPERTIES
        LABELS "exporters;server;policy-engine;authorization;403;focused;unit"
        TIMEOUT 60
    )
    message(STATUS "  ExportApiHandler: 403 Forbidden on ERR_EXPORT_POLICY_DENIED, setPolicyEngine/setAuditLogger setters")
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
# Scheduled Graph Edge Refresh focused tests
# (Issue: #FEATURE/ScheduledGraphEdgeRefresh)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/graph/test_scheduled_edge_refresh.cpp")
    message(STATUS "Adding focused Scheduled Graph Edge Refresh tests")

    add_executable(test_scheduled_edge_refresh_focused
        graph/test_scheduled_edge_refresh.cpp
        ${CMAKE_SOURCE_DIR}/src/graph/scheduled_edge_refresh.cpp
        ${CMAKE_SOURCE_DIR}/src/graph/graph_query_optimizer.cpp
        ${CMAKE_SOURCE_DIR}/src/graph/path_constraints.cpp
        ${CMAKE_SOURCE_DIR}/src/graph/parallel_traversal.cpp
        ${CMAKE_SOURCE_DIR}/src/graph/distributed_graph.cpp
    )

    target_include_directories(test_scheduled_edge_refresh_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_scheduled_edge_refresh_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        fmt::fmt
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )

    target_compile_definitions(test_scheduled_edge_refresh_focused PRIVATE
        THEMIS_TEST_BUILD=1
        THEMIS_GRAPH_EXPORTS
    )

    add_test(NAME ScheduledEdgeRefreshFocusedTests
             COMMAND test_scheduled_edge_refresh_focused)

    set_tests_properties(ScheduledEdgeRefreshFocusedTests PROPERTIES
        LABELS "graph;scheduled;edge_refresh;similarity;policy;audit;anomaly;changefeed;integration;regression;focused"
        TIMEOUT 180
    )

    message(STATUS "  ScheduledEdgeRefresh: policy validation, similarity, temporal decay, safety gates, audit trail, anomaly detection, ChangeFeed integration, large-graph integration, regression")
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
        test_postgres_importer_fk.cpp
        test_postgres_importer_integration.cpp
        test_postgres_importer_live.cpp
        test_postgres_importer_robustness.cpp
        test_postgres_importer_security_audit.cpp
        test_postgres_importer_streaming.cpp
        test_postgres_importer_v2.cpp
        test_postgres_import_integration_v2.cpp)
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

    message(STATUS "  PostgresImporter: advanced mapping, chaos/retry, complex DDL, data types, FK preservation, integration, robustness, security audit, streaming")
else()
    message(STATUS "PostgresImporter focused test sources not found")
endif()

# ============================================================================
# PostgreSQL Importer v2.1+ Focused Tests (standalone binary)
# Covers: schema_inference, column_importance, crdt_importer, postgres_cdc,
#         data_quality, audit_trail, adaptive_import, polyglot_mapper,
#         temporal_support, blockchain_integrity, federated_learning,
#         graphql_federation
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_postgres_importer_v2.cpp")
    message(STATUS "Adding PostgresImporterV2 focused tests (standalone binary)")

    add_executable(test_postgres_importer_v2_focused
        test_postgres_importer_v2.cpp
        ../src/importers/schema_inference.cpp
        ../src/importers/column_importance.cpp
        ../src/importers/crdt_importer.cpp
        ../src/importers/postgres_cdc.cpp
        ../src/importers/data_quality.cpp
        ../src/importers/audit_trail.cpp
        ../src/importers/adaptive_import.cpp
        ../src/importers/polyglot_mapper.cpp
        ../src/importers/temporal_support.cpp
        ../src/importers/blockchain_integrity.cpp
        ../src/importers/federated_learning.cpp
        ../src/importers/graphql_federation.cpp
    )

    target_include_directories(test_postgres_importer_v2_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_postgres_importer_v2_focused PRIVATE
        ${TEST_LIBS}
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_postgres_importer_v2_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME PostgresImporterV2FocusedTests
             COMMAND test_postgres_importer_v2_focused)

    set_tests_properties(PostgresImporterV2FocusedTests PROPERTIES
        LABELS "importers;postgres;v2;ml;cdc;crdt;quality;audit;temporal;blockchain;federated;graphql;unit"
        TIMEOUT 300
    )

    message(STATUS "  PostgresImporterV2: schema_inference, column_importance, crdt, cdc, data_quality, audit_trail, adaptive_import, polyglot_mapper, temporal_support, blockchain_integrity, federated_learning, graphql_federation")
else()
    message(STATUS "PostgresImporterV2 focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_postgres_importer_v2.cpp")
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
# MDM Entity Matching Focused Tests (standalone binary)
# ============================================================================

set(_mdm_matching_test_sources "")
foreach(_mdm_test
        test_entity_matching.cpp
        test_entity_linking.cpp
        test_canonical_resolver.cpp)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_mdm_test}")
        list(APPEND _mdm_matching_test_sources "${_mdm_test}")
    endif()
endforeach()

if(_mdm_matching_test_sources)
    message(STATUS "Adding MDM EntityMatching focused tests (${_mdm_matching_test_sources})")

    add_executable(test_mdm_entity_matching_focused
        ${_mdm_matching_test_sources}
        ${CMAKE_SOURCE_DIR}/src/importers/deterministic_matcher.cpp
        ${CMAKE_SOURCE_DIR}/src/importers/entity_linker.cpp
        ${CMAKE_SOURCE_DIR}/src/importers/canonical_resolver.cpp
    )

    target_include_directories(test_mdm_entity_matching_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_mdm_entity_matching_focused PRIVATE
        ${TEST_LIBS}
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_mdm_entity_matching_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME MDMEntityMatchingFocusedTests COMMAND test_mdm_entity_matching_focused)

    set_tests_properties(MDMEntityMatchingFocusedTests PROPERTIES
        LABELS "importers;mdm;entity-matching;deduplication;unit"
        TIMEOUT 120
    )

    message(STATUS "  MDMEntityMatching: DeterministicMatcher, SemanticMatcher, HybridMatcher, EntityLinker, CanonicalResolver")
else()
    message(STATUS "MDM entity matching focused test sources not found")
endif()

# ============================================================================
# MDM Engine & Integration Focused Tests (standalone binary)
# ============================================================================

set(_mdm_engine_test_sources "")
foreach(_mdm_test
        test_mdm_engine.cpp
        test_mdm_integration.cpp)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_mdm_test}")
        list(APPEND _mdm_engine_test_sources "${_mdm_test}")
    endif()
endforeach()

if(_mdm_engine_test_sources)
    message(STATUS "Adding MDM Engine focused tests (${_mdm_engine_test_sources})")

    add_executable(test_mdm_engine_focused
        ${_mdm_engine_test_sources}
        ${CMAKE_SOURCE_DIR}/src/importers/deterministic_matcher.cpp
        ${CMAKE_SOURCE_DIR}/src/importers/entity_linker.cpp
        ${CMAKE_SOURCE_DIR}/src/importers/canonical_resolver.cpp
        ${CMAKE_SOURCE_DIR}/src/importers/mdm_engine.cpp
        ${CMAKE_SOURCE_DIR}/src/importers/mdm_audit_trail.cpp
        ${CMAKE_SOURCE_DIR}/src/importers/mdm_metrics.cpp
    )

    target_include_directories(test_mdm_engine_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_mdm_engine_focused PRIVATE
        ${TEST_LIBS}
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_mdm_engine_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME MDMEngineFocusedTests COMMAND test_mdm_engine_focused)

    set_tests_properties(MDMEngineFocusedTests PROPERTIES
        LABELS "importers;mdm;engine;audit-trail;metrics;unit;integration"
        TIMEOUT 180
    )

    message(STATUS "  MDMEngine: workflow execution, audit trail, metrics, integration tests")
else()
    message(STATUS "MDM engine focused test sources not found")
endif()

# ============================================================================
# PostgreSQLImporterWithMDM Focused Tests (PART D integration point)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_postgres_importer_mdm.cpp")
    message(STATUS "Adding PostgreSQLImporterWithMDM focused tests (MDM PART D)")

    add_executable(test_postgres_importer_mdm_focused
        test_postgres_importer_mdm.cpp
        ${CMAKE_SOURCE_DIR}/src/importers/deterministic_matcher.cpp
        ${CMAKE_SOURCE_DIR}/src/importers/entity_linker.cpp
        ${CMAKE_SOURCE_DIR}/src/importers/canonical_resolver.cpp
        ${CMAKE_SOURCE_DIR}/src/importers/mdm_engine.cpp
        ${CMAKE_SOURCE_DIR}/src/importers/mdm_audit_trail.cpp
        ${CMAKE_SOURCE_DIR}/src/importers/mdm_metrics.cpp
        ${CMAKE_SOURCE_DIR}/src/importers/postgres_importer_mdm.cpp
        ${CMAKE_SOURCE_DIR}/src/importers/conflict_resolver.cpp
        ${CMAKE_SOURCE_DIR}/src/importers/postgres_importer.cpp
        ${CMAKE_SOURCE_DIR}/src/utils/logger.cpp
    )

    target_include_directories(test_postgres_importer_mdm_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_postgres_importer_mdm_focused PRIVATE
        ${TEST_LIBS}
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    if(TARGET themis_base)
        target_link_libraries(test_postgres_importer_mdm_focused PRIVATE
            themis_base
        )
    endif()

    target_compile_definitions(test_postgres_importer_mdm_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME PostgresImporterMDMFocusedTests COMMAND test_postgres_importer_mdm_focused)

    set_tests_properties(PostgresImporterMDMFocusedTests PROPERTIES
        LABELS "importers;mdm;postgres;integration"
        TIMEOUT 120
    )

    message(STATUS "  PostgresImporterMDM: EntityLinkingConfig, ImportStats MDM fields, PostgreSQLImporterWithMDM")
else()
    message(STATUS "PostgresImporterMDM focused test sources not found")
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

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_per_operation_circuit_breakers.cpp")
    message(STATUS "Adding Per-Operation Circuit Breakers tests (Issue #33)")

    add_executable(test_per_operation_circuit_breakers
        test_per_operation_circuit_breakers.cpp
    )

    target_include_directories(test_per_operation_circuit_breakers PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    if(TARGET themis_llm)
        target_link_libraries(test_per_operation_circuit_breakers PRIVATE
            ${TEST_LIBS}
            themis_llm
            spdlog::spdlog
            nlohmann_json::nlohmann_json
            OpenSSL::SSL
            OpenSSL::Crypto
            Threads::Threads
        )
    else()
        target_link_libraries(test_per_operation_circuit_breakers PRIVATE
            ${TEST_LIBS}
            themis_core
            spdlog::spdlog
            nlohmann_json::nlohmann_json
            OpenSSL::SSL
            OpenSSL::Crypto
            Threads::Threads
        )
    endif()

    target_compile_definitions(test_per_operation_circuit_breakers PRIVATE
        THEMIS_TEST_BUILD=1
        THEMIS_LLM_EXPORTS
    )

    add_test(NAME PerOperationCircuitBreakersFocusedTests COMMAND test_per_operation_circuit_breakers)

    set_tests_properties(PerOperationCircuitBreakersFocusedTests PROPERTIES
        LABELS "llm;aql;circuit-breaker;resilience;per-operation;unit"
        TIMEOUT 60
    )

    message(STATUS "  PerOperationCircuitBreakers: per-op isolation, config injection, getCircuitBreakerStates(), LLM STATS output")
else()
    message(STATUS "Per-Operation Circuit Breakers tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_per_operation_circuit_breakers.cpp")
endif()


# =============================================================================
# Accurate Token-Count Estimation Tests (Issue #145, v1.6.0)
# =============================================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_accurate_token_count_estimation.cpp")
    message(STATUS "Adding Accurate Token-Count Estimation tests (Issue #145)")

    add_executable(test_accurate_token_count_estimation
        test_accurate_token_count_estimation.cpp
    )

    target_include_directories(test_accurate_token_count_estimation PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    if(TARGET themis_llm)
        target_link_libraries(test_accurate_token_count_estimation PRIVATE
            ${TEST_LIBS}
            themis_llm
            spdlog::spdlog
            nlohmann_json::nlohmann_json
            OpenSSL::SSL
            OpenSSL::Crypto
            Threads::Threads
        )
    else()
        target_link_libraries(test_accurate_token_count_estimation PRIVATE
            ${TEST_LIBS}
            themis_core
            spdlog::spdlog
            nlohmann_json::nlohmann_json
            OpenSSL::SSL
            OpenSSL::Crypto
            Threads::Threads
        )
    endif()

    target_compile_definitions(test_accurate_token_count_estimation PRIVATE
        THEMIS_TEST_BUILD=1
        THEMIS_LLM_EXPORTS
    )

    add_test(NAME AccurateTokenCountEstimationTests
             COMMAND test_accurate_token_count_estimation)

    set_tests_properties(AccurateTokenCountEstimationTests PROPERTIES
        LABELS "llm;aql;token-estimation;benchmark;unit"
        TIMEOUT 60
    )

    message(STATUS "  AccurateTokenCountEstimation: CharDivisionEstimator, TiktokenEstimator, injection, p95 benchmark")
else()
    message(STATUS "Accurate Token-Count Estimation tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_accurate_token_count_estimation.cpp")
endif()



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

    # -------------------------------------------------------------------------
    # ContentPolicy OCR routing focused tests
    # -------------------------------------------------------------------------
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_content_policy.cpp")
        message(STATUS "Adding ContentPolicy OCR routing focused tests")

        add_executable(test_content_policy_ocr_focused
            test_content_policy.cpp
        )

        target_include_directories(test_content_policy_ocr_focused PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
        )

        target_link_libraries(test_content_policy_ocr_focused PRIVATE
            ${TEST_LIBS}
            themis_content
            Threads::Threads
        )

        target_compile_definitions(test_content_policy_ocr_focused PRIVATE
            THEMIS_TEST_BUILD=1
        )

        add_test(NAME ContentPolicyOcrFocusedTests COMMAND test_content_policy_ocr_focused)

        set_tests_properties(ContentPolicyOcrFocusedTests PROPERTIES
            LABELS "content;policy;ocr;mime;routing;focused;unit"
            TIMEOUT 60
        )

        message(STATUS "  ContentPolicy: ocrEnabled(), shouldTriggerOcr() for PNG/JPEG/TIFF, ValidationResult::ocr_recommended")
    else()
        message(STATUS "Content Policy OCR focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_content_policy.cpp")
    endif()

    # -------------------------------------------------------------------------
    # OcrProcessor DPI pre-processing focused tests (CON-003)
    # -------------------------------------------------------------------------
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_ocr_processor.cpp")
        message(STATUS "Adding OcrProcessor DPI pre-processing focused tests (CON-003)")

        add_executable(test_ocr_dpi_preprocessing_focused
            test_ocr_processor.cpp
        )

        target_include_directories(test_ocr_dpi_preprocessing_focused PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
        )

        target_link_libraries(test_ocr_dpi_preprocessing_focused PRIVATE
            ${TEST_LIBS}
            themis_content
            themis_core
            Threads::Threads
        )

        target_compile_definitions(test_ocr_dpi_preprocessing_focused PRIVATE
            THEMIS_TEST_BUILD=1
        )

        add_test(NAME OcrDpiPreprocessingFocusedTests
            COMMAND test_ocr_dpi_preprocessing_focused
            --gtest_filter=OcrProcessorConfigTest.*Dpi*:OcrProcessorConfigTest.*Binariz*:OcrProcessorTest.*Preprocess*:OcrProcessorTest.*Suppressed*:OcrProcessorTest.LowResSample*)

        set_tests_properties(OcrDpiPreprocessingFocusedTests PROPERTIES
            LABELS "content;ocr;dpi;binarization;leptonica;preprocessing;focused;unit"
            TIMEOUT 60
        )

        message(STATUS "  OcrProcessor: 300-DPI rescaling (pixScale), adaptive Sauvola binarization (pixSauvolaBinarize), ocr_input_dpi/ocr_rescaled/ocr_binarized metadata (CON-003)")
    else()
        message(STATUS "OcrProcessor DPI pre-processing focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_ocr_processor.cpp")
    endif()

    # -------------------------------------------------------------------------
    # OcrProcessor default data-dir path resolution focused tests (CON-004)
    # -------------------------------------------------------------------------
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_ocr_processor.cpp")
        message(STATUS "Adding OcrProcessor default data-dir focused tests (CON-004)")

        add_executable(test_ocr_default_data_dir_focused
            test_ocr_processor.cpp
        )

        target_include_directories(test_ocr_default_data_dir_focused PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
        )

        target_link_libraries(test_ocr_default_data_dir_focused PRIVATE
            ${TEST_LIBS}
            themis_content
            themis_core
            Threads::Threads
        )

        target_compile_definitions(test_ocr_default_data_dir_focused PRIVATE
            THEMIS_TEST_BUILD=1
        )

        add_test(NAME OcrDefaultDataDirFocusedTests
            COMMAND test_ocr_default_data_dir_focused
            --gtest_filter=OcrProcessorDefaultDataDirTest.*)

        set_tests_properties(OcrDefaultDataDirFocusedTests PROPERTIES
            LABELS "content;ocr;tesseract;tessdata;config;path;focused;unit"
            TIMEOUT 60
        )

        message(STATUS "  OcrProcessor: PATH_MAPPING entry, tryResolve, Config defaults, per-collection override (CON-004)")
    else()
        message(STATUS "OcrProcessor focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_ocr_processor.cpp")
    endif()

    # -------------------------------------------------------------------------
    # AsyncIngestionWorker back-pressure focused tests (CON-005)
    # -------------------------------------------------------------------------
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_async_ingestion_backpressure.cpp")
        message(STATUS "Adding AsyncIngestionWorker back-pressure focused tests (CON-005)")

        add_executable(test_async_ingestion_backpressure_focused
            test_async_ingestion_backpressure.cpp
        )

        target_include_directories(test_async_ingestion_backpressure_focused PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
        )

        target_link_libraries(test_async_ingestion_backpressure_focused PRIVATE
            ${TEST_LIBS}
            themis_content
            themis_core
            nlohmann_json::nlohmann_json
            Threads::Threads
        )

        target_compile_definitions(test_async_ingestion_backpressure_focused PRIVATE
            THEMIS_TEST_BUILD=1
        )

        add_test(NAME AsyncIngestionBackpressureFocusedTests
            COMMAND test_async_ingestion_backpressure_focused)

        set_tests_properties(AsyncIngestionBackpressureFocusedTests PROPERTIES
            LABELS "content;ingestion;backpressure;async;focused;unit"
            TIMEOUT 120
        )

        message(STATUS "  AsyncIngestionWorker: submitStream() blocking, ingestStream() future, shutdown unblocking")
    else()
        message(STATUS "AsyncIngestionWorker back-pressure focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_async_ingestion_backpressure.cpp")
    endif()

    # -------------------------------------------------------------------------
    # LegacyOfficeExtraction focused tests (CON-001)
    # Covers: OLE header validation, DOC/XLS/PPT metadata routing,
    #         graceful failure when soffice is absent, timeout enforcement,
    #         and metrics counters (extract_errors_total).
    # -------------------------------------------------------------------------
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_office_processor.cpp")
        message(STATUS "Adding LegacyOfficeExtraction focused tests (CON-001)")

        add_executable(test_legacy_office_extraction_focused
            test_office_processor.cpp
        )

        target_include_directories(test_legacy_office_extraction_focused PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
        )

        target_link_libraries(test_legacy_office_extraction_focused PRIVATE
            ${TEST_LIBS}
            themis_content
            themis_core
            Threads::Threads
        )

        target_compile_definitions(test_legacy_office_extraction_focused PRIVATE
            THEMIS_TEST_BUILD=1
        )

        add_test(NAME LegacyOfficeExtractionFocusedTests
            COMMAND test_legacy_office_extraction_focused
            --gtest_filter=LegacyOfficeExtractionTest.*:LegacyOfficeMetricsTest.*)

        set_tests_properties(LegacyOfficeExtractionFocusedTests PROPERTIES
            LABELS "content;office;legacy;doc;xls;ppt;libreoffice;posix-spawn;focused;unit"
            TIMEOUT 120
        )

        message(STATUS "  LegacyOffice: OLE header check, DOC/XLS/PPT type routing, graceful soffice-absent failure, timeout, metrics (CON-001)")
    else()
        message(STATUS "LegacyOfficeExtraction focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_office_processor.cpp")
    endif()

    # -------------------------------------------------------------------------
    # LibreOffice subprocess security focused tests (CON-007)
    # Covers: relative-path rejection, dot-slash rejection, path-traversal
    #         rejection, shell-metacharacter injection resistance, malformed
    #         OLE blob handling, large-blob handling, and RAII temp-dir cleanup.
    # -------------------------------------------------------------------------
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_office_processor.cpp")
        message(STATUS "Adding LibreOffice subprocess security focused tests (CON-007)")

        add_executable(test_libreoffice_security_focused
            test_office_processor.cpp
        )

        target_include_directories(test_libreoffice_security_focused PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
        )

        target_link_libraries(test_libreoffice_security_focused PRIVATE
            ${TEST_LIBS}
            themis_content
            themis_core
            Threads::Threads
        )

        target_compile_definitions(test_libreoffice_security_focused PRIVATE
            THEMIS_TEST_BUILD=1
        )

        add_test(NAME LibreOfficeSecurityFocusedTests
            COMMAND test_libreoffice_security_focused
            --gtest_filter=LibreOfficeSecurityTest.*)

        set_tests_properties(LibreOfficeSecurityFocusedTests PROPERTIES
            LABELS "content;office;security;posix-spawn;sandbox;path-hijacking;focused;unit"
            TIMEOUT 120
        )

        message(STATUS "  LibreOfficeSecurity: relative-path rejection, shell-metachar injection, malformed OLE, RAII cleanup (CON-007)")
    else()
        message(STATUS "LibreOffice security focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_office_processor.cpp")
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
endif()

# ============================================================================
# CDC Retention Tests (standalone – exercises runtime-configurable retention
# policies, background cleanup thread lifecycle, and RetentionStatus reporting)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_cdc_retention.cpp")
    message(STATUS "Adding CDC Retention focused tests (runtime TTL + size-based retention)")

    add_executable(test_cdc_retention
        test_cdc_retention.cpp
    )

    # cdc_admin.cpp is compiled into themis_core only when THEMIS_ENABLE_HTTP_SERVER=ON.
    # When HTTP_SERVER is OFF, add it as a direct source to avoid missing-symbol errors.
    if(NOT THEMIS_ENABLE_HTTP_SERVER)
        target_sources(test_cdc_retention PRIVATE
            ${THEMIS_ROOT_DIR}/src/cdc/cdc_admin.cpp
        )
    endif()

    target_include_directories(test_cdc_retention PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_cdc_retention PRIVATE
        ${TEST_LIBS}
        themis_core
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
        Threads::Threads
    )

    target_compile_definitions(test_cdc_retention PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME CDCRetentionFocusedTests COMMAND test_cdc_retention)

    set_tests_properties(CDCRetentionFocusedTests PROPERTIES
        LABELS "cdc;retention;ttl;background-thread;unit"
        TIMEOUT 120
    )

    message(STATUS "  CDCRetention: runtime policy updates, background thread lifecycle, TTL/size/count enforcement, RetentionStatus")
else()
    message(STATUS "CDC Retention tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_cdc_retention.cpp")
endif()

# Config Module – Focused Standalone Test Targets
# ============================================================================

# Shared config source list used by all four focused config test executables.
set(CONFIG_MODULE_SOURCES
    ${THEMIS_ROOT_DIR}/src/config/config_path_resolver.cpp
    ${THEMIS_ROOT_DIR}/src/config/config_metrics_exporter.cpp
    ${THEMIS_ROOT_DIR}/src/config/config_schema_validator.cpp
    ${THEMIS_ROOT_DIR}/src/config/config_audit_log.cpp
    ${THEMIS_ROOT_DIR}/src/config/config_encrypted_store.cpp
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

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_config_metrics_scrape.cpp")
    message(STATUS "Adding Config metrics scrape latency focused tests")

    add_executable(test_config_metrics_scrape_focused
        test_config_metrics_scrape.cpp
        ${CONFIG_MODULE_SOURCES}
    )

    target_include_directories(test_config_metrics_scrape_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_config_metrics_scrape_focused PRIVATE
        ${TEST_LIBS}
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_config_metrics_scrape_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME MetricsScrapeFocusedTests COMMAND test_config_metrics_scrape_focused)

    set_tests_properties(MetricsScrapeFocusedTests PROPERTIES
        LABELS "config;metrics;scrape;latency;prometheus;unit;focused"
        TIMEOUT 60
    )

    message(STATUS "  Config metrics scrape: Prometheus text format, scrape latency < 1 ms, counter accuracy")
else()
    message(STATUS "Config metrics scrape tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_config_metrics_scrape.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_config_encrypted_store.cpp")
    message(STATUS "Adding ConfigEncryptedStore focused tests")

    add_executable(test_config_encrypted_store_focused
        test_config_encrypted_store.cpp
        ${THEMIS_ROOT_DIR}/src/config/config_encrypted_store.cpp
    )

    target_include_directories(test_config_encrypted_store_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_config_encrypted_store_focused PRIVATE
        ${TEST_LIBS}
        nlohmann_json::nlohmann_json
        OpenSSL::Crypto
        Threads::Threads
    )

    target_compile_definitions(test_config_encrypted_store_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ConfigEncryptedStoreFocusedTests COMMAND test_config_encrypted_store_focused)

    set_tests_properties(ConfigEncryptedStoreFocusedTests PROPERTIES
        LABELS "config;encryption;key-rotation;aes-gcm;security;unit;focused"
        TIMEOUT 120
    )

    message(STATUS "  ConfigEncryptedStore: AES-256-GCM encryption, key rotation, serialisation, thread safety")
else()
    message(STATUS "ConfigEncryptedStore focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_config_encrypted_store.cpp")
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

# Auth Rate Limiter Distributed Tests (IRateLimiterBackend + shared InMemory backend)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_auth_rate_limiter_distributed.cpp")
    message(STATUS "Adding Auth Rate Limiter Distributed tests")

    add_executable(test_auth_rate_limiter_distributed
        test_auth_rate_limiter_distributed.cpp
    )

    target_link_libraries(test_auth_rate_limiter_distributed PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_auth_rate_limiter_distributed PRIVATE
        THEMIS_TEST_BUILD=1
    )

    if(THEMIS_ENABLE_REDIS AND hiredis_FOUND)
        if(TARGET hiredis::hiredis)
            target_link_libraries(test_auth_rate_limiter_distributed PRIVATE hiredis::hiredis)
        elseif(TARGET hiredis)
            target_link_libraries(test_auth_rate_limiter_distributed PRIVATE hiredis)
        endif()
        target_compile_definitions(test_auth_rate_limiter_distributed PRIVATE THEMIS_ENABLE_REDIS=1)
    endif()

    add_test(NAME AuthRateLimiterDistributedTests COMMAND test_auth_rate_limiter_distributed)

    set_tests_properties(AuthRateLimiterDistributedTests PROPERTIES
        LABELS "auth;rate-limiter;distributed;backend;integration"
        TIMEOUT 60
    )

    message(STATUS "  Auth Rate Limiter Distributed: IRateLimiterBackend, InMemory, Redis, shared state")
else()
    message(STATUS "Auth Rate Limiter Distributed tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_auth_rate_limiter_distributed.cpp")
endif()


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

# LDAP Connection Pool Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_ldap_connection_pool.cpp")
    message(STATUS "Adding LDAP Connection Pool tests")

    add_executable(test_ldap_connection_pool
        test_ldap_connection_pool.cpp
    )

    target_link_libraries(test_ldap_connection_pool PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        Threads::Threads
    )

    if(THEMIS_LDAP_LIBRARIES)
        target_link_libraries(test_ldap_connection_pool PRIVATE ${THEMIS_LDAP_LIBRARIES})
    endif()

    target_compile_definitions(test_ldap_connection_pool PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME LDAPConnectionPoolTests COMMAND test_ldap_connection_pool)

    set_tests_properties(LDAPConnectionPoolTests PROPERTIES
        LABELS "auth;ldap;pool;unit"
        TIMEOUT 60
    )

    message(STATUS "  LDAP Connection Pool: pool lifecycle, checkout/return, health check, metrics")
else()
    message(STATUS "LDAP Connection Pool tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_ldap_connection_pool.cpp")
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

# SAML Auth Provider Tests (server-layer, focused binary)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_saml_auth_provider.cpp")
    message(STATUS "Adding SAML Auth Provider tests (server-layer)")

    add_executable(test_saml_auth_provider
        test_saml_auth_provider.cpp
    )

    target_link_libraries(test_saml_auth_provider PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )

    if(TARGET pugixml::pugixml)
        target_link_libraries(test_saml_auth_provider PRIVATE pugixml::pugixml)
    elseif(TARGET pugixml::static)
        target_link_libraries(test_saml_auth_provider PRIVATE pugixml::static)
    endif()

    target_compile_definitions(test_saml_auth_provider PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME SAMLAuthProviderTests COMMAND test_saml_auth_provider)

    set_tests_properties(SAMLAuthProviderTests PROPERTIES
        LABELS "server;auth;saml;sso;sp;unit"
        TIMEOUT 120
    )

    message(STATUS "  SAML Auth Provider: SP login redirect, ACS validation, SLO, metadata, replay detection")
else()
    message(STATUS "SAML Auth Provider tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_saml_auth_provider.cpp")
endif()

# OAuth2/OIDC Provider Tests (server-layer, focused binary)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_oauth2_provider.cpp")
    message(STATUS "Adding OAuth2/OIDC Provider tests (server-layer)")

    add_executable(test_oauth2_provider
        test_oauth2_provider.cpp
    )

    target_link_libraries(test_oauth2_provider PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )

    if(TARGET CURL::libcurl)
        target_link_libraries(test_oauth2_provider PRIVATE CURL::libcurl)
    elseif(TARGET curl)
        target_link_libraries(test_oauth2_provider PRIVATE curl)
    endif()

    target_compile_definitions(test_oauth2_provider PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME OAuth2ProviderTests COMMAND test_oauth2_provider)

    set_tests_properties(OAuth2ProviderTests PROPERTIES
        LABELS "server;auth;oauth2;oidc;pkce;unit"
        TIMEOUT 120
    )

    message(STATUS "  OAuth2/OIDC Provider: authorize, callback, token exchange, refresh, introspect, logout")
else()
    message(STATUS "OAuth2/OIDC Provider tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_oauth2_provider.cpp")
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
# Core Module – Lock-Free Metrics Focused Tests (Issue #66)
# Validates all three acceptance criteria:
#   AC-1  std::atomic counters
#   AC-2  Lock-free per-thread ring buffer for histograms
#   AC-3  Thread-local aggregation with periodic background flush
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_lockfree_metrics.cpp")
    message(STATUS "Adding LockFree Metrics focused tests (core module, Issue #66)")

    add_executable(test_lockfree_metrics_focused
        test_lockfree_metrics.cpp
    )

    target_include_directories(test_lockfree_metrics_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_lockfree_metrics_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_lockfree_metrics_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME LockFreeMetricsFocusedTests COMMAND test_lockfree_metrics_focused)

    set_tests_properties(LockFreeMetricsFocusedTests PROPERTIES
        LABELS "core;metrics;lock-free;atomic;ring-buffer;thread-local;v1.6.0;unit"
        TIMEOUT 60
    )

    message(STATUS "  LockFreeMetrics: atomic counters, SPSC ring buffer, thread-local aggregation, periodic flush")
else()
    message(STATUS "LockFree Metrics focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_lockfree_metrics.cpp")
endif()

# ============================================================================
# Zero-Copy Logging focused tests (core module, Issue #65, v1.6.0)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_zero_copy_logging.cpp")
    message(STATUS "Adding Zero-Copy Logging focused tests (core module, Issue #65)")

    add_executable(test_zero_copy_logging_focused
        test_zero_copy_logging.cpp
    )

    target_include_directories(test_zero_copy_logging_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_zero_copy_logging_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_zero_copy_logging_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ZeroCopyLoggingFocusedTests COMMAND test_zero_copy_logging_focused)

    set_tests_properties(ZeroCopyLoggingFocusedTests PROPERTIES
        LABELS "core;logging;zero-copy;string-view;thread-local;buffer;v1.6.0;unit"
        TIMEOUT 60
    )

    message(STATUS "  ZeroCopyLogger: string_view hot-path, pre-allocated thread-local buffer, early level-check, PII redaction")
else()
    message(STATUS "Zero-Copy Logging focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_zero_copy_logging.cpp")
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

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/security/test_security_negative_integration.cpp")
    message(STATUS "Adding Security Negative Integration focused tests")

    add_executable(test_security_negative_integration_focused
        security/test_security_negative_integration.cpp
    )

    target_include_directories(test_security_negative_integration_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_security_negative_integration_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::Crypto
        Threads::Threads
    )

    target_compile_definitions(test_security_negative_integration_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME SecurityNegativeIntegrationFocusedTests
             COMMAND test_security_negative_integration_focused)

    set_tests_properties(SecurityNegativeIntegrationFocusedTests PROPERTIES
        LABELS "security;negative;jwt;rbac;vault;integration;focused;unit"
        TIMEOUT 120
    )

    message(STATUS "  SecurityNegativeIntegration: JWT (expired/tampered/alg-confusion/revoked-kid), RBAC (deny/cycle/wildcard), Vault (HTTP 403/404/malformed)")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/security/test_input_validation_security.cpp")
    message(STATUS "Adding Input Validation Security focused tests")

    add_executable(test_input_validation_security_focused
        security/test_input_validation_security.cpp
    )

    target_include_directories(test_input_validation_security_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_input_validation_security_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_input_validation_security_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME InputValidationSecurityFocusedTests
             COMMAND test_input_validation_security_focused)

    set_tests_properties(InputValidationSecurityFocusedTests PROPERTIES
        LABELS "security;input-validation;injection;xss;focused;unit"
        TIMEOUT 60
    )

    message(STATUS "  InputValidationSecurity: AQL/path-traversal/XSS/command-injection prevention (legacy API disabled; DISABLED placeholder)")
endif()

# ============================================================================
# Time Series Module – Focused Standalone Test Targets
# Each test is a standalone executable covering a distinct Phase 2 / Phase 3
# feature of the time series subsystem.  All tests are CPU-only; no TSStore
# on-disk state is required for unit-level tests.
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_downsampling.cpp")
    message(STATUS "Adding Downsampling Pipeline focused tests")

    add_executable(test_downsampling_focused
        test_downsampling.cpp
    )

    target_include_directories(test_downsampling_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_downsampling_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_downsampling_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME DownsamplingFocusedTests COMMAND test_downsampling_focused)

    set_tests_properties(DownsamplingFocusedTests PROPERTIES
        LABELS "timeseries;downsampling;pipeline;tier-selector;focused;unit"
        TIMEOUT 60
    )

    message(STATUS "  Downsampling: DownsamplingTier factories, policy, TierSelector routing, DownsamplingPipeline lifecycle")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_ts_adaptive_flush.cpp")
    message(STATUS "Adding TSAutoBuffer adaptive flush focused tests")

    add_executable(test_ts_adaptive_flush_focused
        test_ts_adaptive_flush.cpp
    )

    target_include_directories(test_ts_adaptive_flush_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_ts_adaptive_flush_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_ts_adaptive_flush_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME TSAdaptiveFlushFocusedTests COMMAND test_ts_adaptive_flush_focused)

    set_tests_properties(TSAdaptiveFlushFocusedTests PROPERTIES
        LABELS "timeseries;ts-auto-buffer;adaptive-flush;backpressure;focused;unit"
        TIMEOUT 60
    )

    message(STATUS "  TSAdaptiveFlush: FlushController EWMA, backpressure, TSAutoBufferConfig, metrics integration")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_prometheus_remote_write.cpp")
    message(STATUS "Adding Prometheus Remote Write focused tests")

    add_executable(test_prometheus_remote_write_focused
        test_prometheus_remote_write.cpp
    )

    target_include_directories(test_prometheus_remote_write_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_prometheus_remote_write_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_prometheus_remote_write_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME PrometheusRemoteWriteFocusedTests
             COMMAND test_prometheus_remote_write_focused)

    set_tests_properties(PrometheusRemoteWriteFocusedTests PROPERTIES
        LABELS "timeseries;prometheus;remote-write;protobuf;snappy;focused;unit"
        TIMEOUT 60
    )

    message(STATUS "  PrometheusRemoteWrite: snappy decode, protobuf label mapping, HTTP 204/400/501 paths")
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

add_chimera_focused_test(
    test_chimera_retry_policy
    chimera/test_retry_policy.cpp
    ChimeraRetryPolicyTests
    "chimera;retry;circuit-breaker;error-recovery;unit"
    "RetryPolicy/CircuitBreaker/ConnectionWithRetry: exponential backoff, transient classification, circuit states, health check"
)

add_chimera_focused_test(
    test_chimera_batch_operations
    chimera/test_chimera_batch_operations.cpp
    ChimeraBatchOperationsTests
    "chimera;adapter;batch;unit"
    "Batch Operation Enhancements: BatchOptions, BatchResult, batch_insert_advanced, progress/batch callbacks, stop_on_error"
)

add_chimera_focused_test(
    test_chimera_adapter_config_validation
    chimera/test_adapter_config_validation.cpp
    ChimeraAdapterConfigValidationTests
    "chimera;adapter;config;validation;unit"
    "AdapterConfig: validate(), get_validation_errors(), parse_connection_string(), type/range checks"
    test_chimera_async_api
    chimera/test_chimera_async_api.cpp
    ChimeraAsyncAPITests
    "chimera;adapter;async;future;unit"
    "Async/Promise-Based API: execute_query_async, batch_insert_async, search_vectors_async, cancel_async, concurrent fan-out"
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
# LLM Module Focused Tests (tests/llm/)
# ============================================================================

function(add_llm_focused_test target_name test_src test_name labels description)
    set(full_path "${CMAKE_CURRENT_SOURCE_DIR}/${test_src}")
    if(NOT EXISTS "${full_path}")
        message(STATUS "LLM focused test not found: ${full_path}")
        return()
    endif()

    message(STATUS "Adding LLM focused test: ${target_name}")

    add_executable(${target_name} ${test_src})

    target_include_directories(${target_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${THEMIS_ROOT_DIR}/include
    )

    if(TARGET themis_llm)
        target_link_libraries(${target_name} PRIVATE
            ${TEST_LIBS}
            themis_llm
            spdlog::spdlog
            nlohmann_json::nlohmann_json
            OpenSSL::SSL
            OpenSSL::Crypto
            Threads::Threads
        )
        # Also link themis_core when available so LLM tests can use cross-module
        # types such as PolicyEngine (governance) without separate link steps.
        if(TARGET themis_core)
            target_link_libraries(${target_name} PRIVATE themis_core)
        endif()
    else()
        target_link_libraries(${target_name} PRIVATE
            ${TEST_LIBS}
            themis_core
            spdlog::spdlog
            nlohmann_json::nlohmann_json
            OpenSSL::SSL
            OpenSSL::Crypto
            Threads::Threads
        )
    endif()

    target_compile_definitions(${target_name} PRIVATE
        THEMIS_TEST_BUILD=1
        THEMIS_LLM_EXPORTS
    )

    add_test(NAME ${test_name} COMMAND ${target_name})

    set_tests_properties(${test_name} PROPERTIES
        LABELS "${labels}"
        TIMEOUT 120
    )

    message(STATUS "  ${description}")
endfunction()

if(THEMIS_ENABLE_LLM)

    add_llm_focused_test(
        test_llm_ai_orchestrator
        llm/test_ai_orchestrator.cpp
        LlmAiOrchestratorFocusedTests
        "llm;ai;orchestrator;pipeline;unit"
        "AiOrchestrator: YAML pipeline modes (ask/edit/rag/agentic/ethics/multi_agent), ToolRegistry, mode dispatch"
    )

    add_llm_focused_test(
        test_llm_streaming_handler
        llm/test_streaming_handler.cpp
        LlmStreamingHandlerFocusedTests
        "llm;streaming;sse;handler;unit"
        "StreamingHandler: SSE token delivery, cancel mid-stream, is_final sentinel, backpressure"
    )

    add_llm_focused_test(
        test_llm_openai_compat_adapter
        llm/test_openai_compat_adapter.cpp
        LlmOpenAICompatAdapterFocusedTests
        "llm;openai;compat;adapter;unit"
        "OpenAICompatAdapter: /v1/chat/completions mapping, stream=true SSE, tool_calls, 401 on denied"
    )

    add_llm_focused_test(
        test_llm_lora_hot_loading
        llm/test_lora_hot_loading.cpp
        LlmLoraHotLoadingFocusedTests
        "llm;lora;hot-loading;adapter;unit"
        "LoRA hot-loading: loadLoRAAdapter, unloadLoRAAdapter, zero dropped requests during swap"
    )

    add_llm_focused_test(
        test_llm_model_loader_async
        llm/test_model_loader_async.cpp
        LlmModelLoaderAsyncFocusedTests
        "llm;model;loader;async;unit"
        "ModelLoader async: lazy loading, concurrent requests, cancellation, timeout"
    )

    add_llm_focused_test(
        test_llm_audit_logger
        llm/test_llm_audit_logger.cpp
        LlmAuditLoggerFocusedTests
        "llm;audit;logger;unit"
        "LlmModelAuditLogger: structured audit events, hash-chain integrity, PII redaction"
    )

    add_llm_focused_test(
        test_llm_json_schema_binding
        llm/test_json_schema_binding.cpp
        LlmJsonSchemaBindingFocusedTests
        "llm;json;schema;binding;tool-calls;unit"
        "JSON schema binding: schema→GBNF conversion, tool-call output validation, nested schemas"
    )

    add_llm_focused_test(
        test_llm_grammar_integration
        llm/test_grammar_integration.cpp
        LlmGrammarIntegrationFocusedTests
        "llm;grammar;constrained-generation;unit"
        "Grammar-constrained generation: EBNF compilation, runtime API detection, graceful degradation"
    )

    add_llm_focused_test(
        test_llm_validator
        llm/test_llm_validator.cpp
        LlmValidatorFocusedTests
        "llm;validator;production;unit"
        "ProductionValidator: model health checks, benchmark thresholds, validation report"
    )

    add_llm_focused_test(
        test_llm_deployment_plugin
        llm/test_llm_deployment_plugin.cpp
        LlmDeploymentPluginFocusedTests
        "llm;deployment;plugin;unit"
        "LlmDeploymentPlugin: model lifecycle, caching, audit, hot-swap"
    )

    add_llm_focused_test(
        test_llm_lora_adapters
        llm/test_lora_adapters.cpp
        LlmLoraAdaptersFocusedTests
        "llm;lora;adapters;unit"
        "LoRA adapters: load/unload, metadata, versioning, VRAM footprint"
    )

    add_llm_focused_test(
        test_llm_lora_auto_binding
        llm/test_lora_auto_binding.cpp
        LlmLoraAutoBindingFocusedTests
        "llm;lora;auto-binding;unit"
        "LoRA auto-binding: rule matching, priority ordering, fallback to base model"
    )

    add_llm_focused_test(
        test_llm_lora_adapter_application
        llm/test_lora_adapter_application.cpp
        LlmLoraAdapterApplicationFocusedTests
        "llm;lora;adapter;application;unit"
        "LoRA adapter application: weight injection, rank/alpha params, output delta"
    )

    add_llm_focused_test(
        test_llm_mcp_orchestrator_bridge
        llm/test_mcp_orchestrator_bridge.cpp
        LlmMcpOrchestratorBridgeFocusedTests
        "llm;mcp;orchestrator;bridge;unit"
        "MCP↔Orchestrator bridge: tool registration, call routing, result marshalling"
    )

    add_llm_focused_test(
        test_llm_extended_context
        llm/test_extended_context.cpp
        LlmExtendedContextFocusedTests
        "llm;context;extended;rope;unit"
        "Extended context: RoPE scaling, context window extension, cache coherency"
    )

    add_llm_focused_test(
        test_llm_inference_performance
        llm/test_inference_performance.cpp
        LlmInferencePerformanceFocusedTests
        "llm;inference;performance;benchmark;unit"
        "Inference performance: tokens/sec baseline, latency p99, batching throughput"
    )

    add_llm_focused_test(
        test_llm_inference_quality
        llm/test_inference_quality.cpp
        LlmInferenceQualityFocusedTests
        "llm;inference;quality;evaluation;unit"
        "Inference quality: output determinism, grammar compliance, tool-call correctness"
    )

    add_llm_focused_test(
        test_llm_kernel_fusion_cpu_fallback
        llm/test_kernel_fusion_cpu_fallback.cpp
        LlmKernelFusionCpuFallbackFocusedTests
        "llm;kernel-fusion;cpu;fallback;unit"
        "Kernel fusion CPU fallback: correctness vs reference, no CUDA dependency"
    )

    add_llm_focused_test(
        test_llm_kernel_fusion_cuda
        llm/test_kernel_fusion_cuda.cpp
        LlmKernelFusionCudaFocusedTests
        "llm;kernel-fusion;cuda;unit"
        "Kernel fusion CUDA: Flash Attention parity, FP16/BF16 kernels, skip when no CUDA"
    )

    add_llm_focused_test(
        test_llm_llama_cpp_tokenizer
        llm/test_llama_cpp_tokenizer.cpp
        LlmLlamaCppTokenizerFocusedTests
        "llm;llama;tokenizer;unit"
        "llama.cpp tokenizer: encode/decode round-trip, special tokens, BPE merges"
    )

    add_llm_focused_test(
        test_llm_llama_wrapper_state
        llm/test_llama_wrapper_state.cpp
        LlmLlamaWrapperStateFocusedTests
        "llm;llama;wrapper;state;unit"
        "LlamaWrapper state: model load/unload, context lifecycle, thread safety"
    )

    add_llm_focused_test(
        test_llm_gpu_lora_integration
        llm/test_gpu_lora_integration.cpp
        LlmGpuLoraIntegrationFocusedTests
        "llm;gpu;lora;integration;unit"
        "GPU LoRA integration: multi-LoRA on GPU, VRAM tracking, adapter switch latency"
    )

    add_llm_focused_test(
        test_llm_real_embeddings
        llm/test_real_embeddings.cpp
        LlmRealEmbeddingsFocusedTests
        "llm;embeddings;real;unit"
        "Real embeddings: embedding generation via llamacpp, similarity search, dimension check"
    )

    add_llm_focused_test(
        test_llm_model_loader_error_handling
        llm/test_model_loader_error_handling.cpp
        LlmModelLoaderErrorHandlingFocusedTests
        "llm;model;loader;error-handling;unit"
        "ModelLoader error handling: missing file, corrupted GGUF, OOM, permission denied"
    )

    add_llm_focused_test(
        test_llm_model_loading_best_practices
        llm/test_model_loading_best_practices.cpp
        LlmModelLoadingBestPracticesFocusedTests
        "llm;model;loading;best-practices;unit"
        "Model loading best practices: lazy init, memory budget enforcement, eviction policies"
    )

    add_llm_focused_test(
        test_llm_model_loading_from_themisdb
        llm/test_model_loading_from_themisdb.cpp
        LlmModelLoadingFromThemisDbFocusedTests
        "llm;model;loading;themisdb;integration"
        "Model loading from ThemisDB: store, retrieve, checksum verify, hot-reload"
    )

    # GTest-based micro-benchmarks (registered as tests with longer timeout)
    add_llm_focused_test(
        bench_llm_continuous_batch_scheduler
        llm/bench_continuous_batch_scheduler.cpp
        LlmBenchContinuousBatchScheduler
        "llm;benchmark;batch-scheduler;performance"
        "Continuous batch scheduler benchmark: throughput at varying concurrency levels"
    )

    add_llm_focused_test(
        test_llm_active_vram_allocator
        llm/test_active_vram_allocator.cpp
        LlmActiveVRAMAllocatorFocusedTests
        "llm;vram;gpu;oom;recovery;unit"
        "ActiveVRAMAllocator: allocation/free, OOM recovery (eviction/defrag/spill), thread safety, AdaptiveVRAMAllocator delegation"
    )

    # Standalone focused test target for InferenceEngineEnhanced.
    # Exercises KV-cache prewarming, embedding-based cache lookup, hot-swap,
    # speculative decoding, load-balancing, and per-model quotas.
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_inference_engine_enhanced.cpp")
        add_executable(test_inference_engine_enhanced_focused
            test_inference_engine_enhanced.cpp
        )
        target_include_directories(test_inference_engine_enhanced_focused PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${THEMIS_ROOT_DIR}/include
        )
        if(TARGET themis_llm)
            target_link_libraries(test_inference_engine_enhanced_focused PRIVATE
                ${TEST_LIBS}
                themis_llm
                spdlog::spdlog
                nlohmann_json::nlohmann_json
                OpenSSL::SSL
                OpenSSL::Crypto
                Threads::Threads
            )
        else()
            target_link_libraries(test_inference_engine_enhanced_focused PRIVATE
                ${TEST_LIBS}
                themis_core
                spdlog::spdlog
                nlohmann_json::nlohmann_json
                OpenSSL::SSL
                OpenSSL::Crypto
                Threads::Threads
            )
        endif()
        target_compile_definitions(test_inference_engine_enhanced_focused PRIVATE
            THEMIS_TEST_BUILD=1
            THEMIS_LLM_EXPORTS
        )
        add_test(NAME InferenceEngineEnhancedFocusedTests
            COMMAND test_inference_engine_enhanced_focused)
        set_tests_properties(InferenceEngineEnhancedFocusedTests PROPERTIES
            LABELS "llm;inference;cache;prewarming;embedding;unit"
            TIMEOUT 120
        )
        message(STATUS "InferenceEngineEnhanced focused tests: KV-cache prewarming, embedding-based lookup, hot-swap, load-balancing")
    endif()
    # NOTE: tests/llm/bench_model_loading_from_themisdb.cpp uses Google Benchmark
    # (benchmark::benchmark_main / BENCHMARK() macro), NOT GTest.
    # It is registered as a proper benchmark target in benchmarks/CMakeLists.txt.

else()
    message(STATUS "LLM focused tests (tests/llm/) skipped (THEMIS_ENABLE_LLM=OFF)")
endif()

# Ingestion Module – Focused Standalone Test Targets
# Each target links against themis_core (which now always includes all 10
# ingestion source files) and the target's single test source.
# ============================================================================

# Shared source list used by ingestion focused test executables.
set(INGESTION_CORE_SOURCES
    ${THEMIS_ROOT_DIR}/src/ingestion/ingestion_manager.cpp
    ${THEMIS_ROOT_DIR}/src/ingestion/filesystem_ingester.cpp
    ${THEMIS_ROOT_DIR}/src/ingestion/api_connector.cpp
    ${THEMIS_ROOT_DIR}/src/ingestion/huggingface_connector.cpp
    ${THEMIS_ROOT_DIR}/src/ingestion/kafka_connector.cpp
    ${THEMIS_ROOT_DIR}/src/ingestion/object_storage_connector.cpp
    ${THEMIS_ROOT_DIR}/src/ingestion/database_connector.cpp
    ${THEMIS_ROOT_DIR}/src/ingestion/web_crawler_connector.cpp
    ${THEMIS_ROOT_DIR}/src/ingestion/ingestion_coordinator.cpp
    ${THEMIS_ROOT_DIR}/src/ingestion/cdc_connector.cpp
    ${THEMIS_ROOT_DIR}/src/ingestion/deontic_extractor.cpp
    ${THEMIS_ROOT_DIR}/src/ingestion/semantic_validator.cpp
    ${THEMIS_ROOT_DIR}/src/ingestion/agentic_reference_validator.cpp
    ${THEMIS_ROOT_DIR}/src/ingestion/llm_adapter.cpp
)

# Helper macro for ingestion focused tests
macro(add_ingestion_focused_test TARGET_NAME TEST_SOURCE TEST_NAME LABELS DESCRIPTION)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SOURCE}")
        message(STATUS "Adding ingestion focused test: ${TARGET_NAME}")

        add_executable(${TARGET_NAME}
            ${TEST_SOURCE}
        )

        # Ingestion sources are in themis_core; if building without themis_core
        # (standalone), add them explicitly to avoid missing-symbol errors.
        if(NOT TARGET themis_core)
            target_sources(${TARGET_NAME} PRIVATE ${INGESTION_CORE_SOURCES})
        endif()

        target_include_directories(${TARGET_NAME} PRIVATE
            ${THEMIS_ROOT_DIR}/include
            ${THEMIS_ROOT_DIR}/src
        )

        target_link_libraries(${TARGET_NAME} PRIVATE
            ${TEST_LIBS}
            $<$<TARGET_EXISTS:themis_core>:themis_core>
            Threads::Threads
        )

        target_compile_definitions(${TARGET_NAME} PRIVATE
            THEMIS_TEST_BUILD=1
        )

        add_test(NAME ${TEST_NAME} COMMAND ${TARGET_NAME})

        set_tests_properties(${TEST_NAME} PROPERTIES
            LABELS "${LABELS}"
            TIMEOUT 120
        )

        message(STATUS "  ${DESCRIPTION}")
    else()
        message(STATUS "Ingestion test not found: ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SOURCE}")
    endif()
endmacro()

add_ingestion_focused_test(
    test_ingestion_builder_focused
    test_ingestion_builder.cpp
    IngestionBuilderFocusedTests
    "ingestion;builder;ratelimit;metrics;focused;unit"
    "IngestionBuilder: fluent API, RateLimitConfig, correlation_id, IngestionMetricsExporter"
)

add_ingestion_focused_test(
    test_ingestion_features_focused
    test_ingestion_features.cpp
    IngestionFeaturesFocusedTests
    "ingestion;features;filesystem;checkpoint;focused;unit"
    "IngestionFeatures: filesystem ingestion, checkpointing, parallel sources, dry-run"
)

add_ingestion_focused_test(
    test_ingestion_errors_focused
    test_ingestion_errors.cpp
    IngestionErrorsFocusedTests
    "ingestion;errors;retry;quarantine;focused;unit"
    "IngestionErrors: error codes, retry logic, quarantine queue, error propagation"
)

add_ingestion_focused_test(
    test_ingestion_checkpoint_focused
    test_ingestion_checkpoint.cpp
    IngestionCheckpointFocusedTests
    "ingestion;checkpoint;incremental;focused;unit"
    "IngestionCheckpoint: incremental progress, resume-from-checkpoint, cursor/offset pagination"
)

add_ingestion_focused_test(
    test_ingestion_resilience_focused
    test_ingestion_resilience.cpp
    IngestionResilienceFocusedTests
    "ingestion;resilience;retry;backoff;focused;unit"
    "IngestionResilience: exponential back-off, transient failures, partial-batch recovery"
)

add_ingestion_focused_test(
    test_ingestion_reconfig_focused
    test_ingestion_reconfig.cpp
    IngestionReconfigFocusedTests
    "ingestion;reconfig;dynamic;focused;unit"
    "IngestionReconfig: dynamic source reconfiguration without restart"
)

add_ingestion_focused_test(
    test_ingestion_plugin_api_focused
    test_ingestion_plugin_api.cpp
    IngestionPluginApiFocusedTests
    "ingestion;plugin;registry;focused;unit"
    "IngestionPluginApi: ConnectorPluginRegistry CRUD, concurrent isolation, IngestionBuilder plugin sources"
)

add_ingestion_focused_test(
    test_ingestion_schema_validation_focused
    test_ingestion_schema_validation.cpp
    IngestionSchemaValidationFocusedTests
    "ingestion;schema;validation;focused;unit"
    "IngestionSchemaValidation: field-existence/type/pattern rules, reject_invalid flag, schema_violations metric"
)

add_ingestion_focused_test(
    test_ingestion_security_focused
    test_ingestion_security.cpp
    IngestionSecurityFocusedTests
    "ingestion;security;pathtraversal;credentials;focused;unit"
    "IngestionSecurity: path-traversal guard, credential masking, SSRF prevention"
)

add_ingestion_focused_test(
    test_ingestion_oauth_focused
    test_ingestion_oauth.cpp
    IngestionOauthFocusedTests
    "ingestion;oauth;token-refresh;focused;unit"
    "IngestionOauth: OAuth 2.0 token refresh, expired token handling, connector retry"
)

add_ingestion_focused_test(
    test_ingestion_kafka_focused
    test_ingestion_kafka.cpp
    IngestionKafkaFocusedTests
    "ingestion;kafka;consumer;offset;focused;unit"
    "IngestionKafka: KafkaConnector consumer group, offset commit, topic filtering, async ingestion"
)

add_ingestion_focused_test(
    test_ingestion_object_storage_focused
    test_ingestion_object_storage.cpp
    IngestionObjectStorageFocusedTests
    "ingestion;objectstorage;s3;gcs;azure;focused;unit"
    "IngestionObjectStorage: S3/GCS/Azure mock-injection, prefix filter, path-traversal rejection, 28 tests"
)

add_ingestion_focused_test(
    test_ingestion_database_focused
    test_ingestion_database.cpp
    IngestionDatabaseFocusedTests
    "ingestion;database;odbc;jdbc;focused;unit"
    "IngestionDatabase: ODBC mock-injection, JDBC URL parsing, text_columns extraction, credential masking"
)

add_ingestion_focused_test(
    test_ingestion_web_crawler_focused
    test_ingestion_web_crawler.cpp
    IngestionWebCrawlerFocusedTests
    "ingestion;webcrawler;sitemap;ssrf;focused;unit"
    "IngestionWebCrawler: BFS crawl, sitemap parsing, robots.txt, SSRF prevention"
)

add_ingestion_focused_test(
    test_ingestion_coordinator_focused
    test_ingestion_coordinator.cpp
    IngestionCoordinatorFocusedTests
    "ingestion;coordinator;distributed;workstealing;focused;unit"
    "IngestionCoordinator: distributed work-stealing thread pool, progress aggregation, node coordination"
)

add_ingestion_focused_test(
    test_ingestion_cdc_focused
    test_ingestion_cdc.cpp
    IngestionCdcFocusedTests
    "ingestion;cdc;replication;lsn;focused;unit"
    "IngestionCdc: CdcConnector mock-injection, LSN tracking, WAL poll loop, table_filter"
)

add_ingestion_focused_test(
    test_ingestion_integration_focused
    test_ingestion_integration.cpp
    IngestionIntegrationFocusedTests
    "ingestion;integration;filesystem;api;huggingface;focused;integration"
    "IngestionIntegration: end-to-end filesystem/API/HuggingFace integration tests"
)

add_ingestion_focused_test(
    test_ingestion_pipeline_focused
    test_ingestion_pipeline.cpp
    IngestionPipelineFocusedTests
    "ingestion;pipeline;kv;rocksdb;focused;unit"
    "IngestionPipeline: key-value ingestion pipeline, RocksDB write path, batch throughput"
)

add_ingestion_focused_test(
    test_ingestion_lineage_focused
    test_ingestion_lineage.cpp
    IngestionLineageFocusedTests
    "ingestion;lineage;provenance;tracking;focused;unit"
    "IngestionLineage: IngestionLineageRecord struct, IngestionLineageStore, IngestionManager lineage API (Issue #1901)"
)

add_ingestion_focused_test(
    test_legal_extraction_focused
    test_legal_extraction.cpp
    LegalExtractionFocusedTests
    "ingestion;legal;deontic;semantic;extraction;focused;unit"
    "LegalExtraction: DeonticExtractor, SemanticValidator, AgenticReferenceValidator, LegalIngestionConfig, IngestionManager legal pipeline"
)
# Metadata Module – Focused Test Targets
# ============================================================================

# Metadata – SchemaManager focused tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_schema_manager.cpp")
    message(STATUS "Adding Metadata SchemaManager focused tests")
    add_executable(test_schema_manager_focused test_schema_manager.cpp)
    target_include_directories(test_schema_manager_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_schema_manager_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::Crypto
        Threads::Threads
    )
    target_compile_definitions(test_schema_manager_focused PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME SchemaManagerFocusedTests COMMAND test_schema_manager_focused)
    set_tests_properties(SchemaManagerFocusedTests PROPERTIES
        LABELS "metadata;schema;discovery;ttl;changefeed;adaptive-ttl;unit"
        TIMEOUT 120
    )
    message(STATUS "  SchemaManager: automatic table discovery, property type detection, adaptive TTL, changefeeds")
endif()

# Metadata – StatisticsCollector focused tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_statistics_collector.cpp")
    message(STATUS "Adding Metadata StatisticsCollector focused tests")
    add_executable(test_statistics_collector_focused test_statistics_collector.cpp)
    target_include_directories(test_statistics_collector_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_statistics_collector_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::Crypto
        Threads::Threads
    )
    target_compile_definitions(test_statistics_collector_focused PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME StatisticsCollectorFocusedTests COMMAND test_statistics_collector_focused)
    set_tests_properties(StatisticsCollectorFocusedTests PROPERTIES
        LABELS "metadata;statistics;histograms;cardinality;selectivity;unit"
        TIMEOUT 120
    )
    message(STATUS "  StatisticsCollector: equi-height histograms, cardinality estimation, index stats, auto-refresh, metrics hook")
endif()

# Metadata – StatisticsCollector auto-refresh focused tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_statistics_auto_refresh.cpp")
    message(STATUS "Adding Metadata StatisticsCollector auto-refresh focused tests")
    add_executable(test_statistics_auto_refresh_focused test_statistics_auto_refresh.cpp)
    target_include_directories(test_statistics_auto_refresh_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_statistics_auto_refresh_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::Crypto
        Threads::Threads
    )
    target_compile_definitions(test_statistics_auto_refresh_focused PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME StatisticsAutoRefreshFocusedTests COMMAND test_statistics_auto_refresh_focused)
    set_tests_properties(StatisticsAutoRefreshFocusedTests PROPERTIES
        LABELS "metadata;statistics;auto-refresh;unit"
        TIMEOUT 120
    )
    message(STATUS "  StatisticsCollector auto-refresh: background thread, metrics hook, interval control")
endif()

# Metadata – ColumnLineageTracker focused tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_column_lineage.cpp")
    message(STATUS "Adding Metadata ColumnLineage focused tests")
    add_executable(test_column_lineage_focused test_column_lineage.cpp)
    target_include_directories(test_column_lineage_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_column_lineage_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::Crypto
        Threads::Threads
    )
    target_compile_definitions(test_column_lineage_focused PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME ColumnLineageFocusedTests COMMAND test_column_lineage_focused)
    set_tests_properties(ColumnLineageFocusedTests PROPERTIES
        LABELS "metadata;lineage;provenance;dag;column-derivation;unit"
        TIMEOUT 60
    )
    message(STATUS "  ColumnLineage: derivation DAG, transitive upstream/downstream traversal, provenance JSON export")
endif()

# Metadata – CatalogExporter focused tests (Atlas + DataHub)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_catalog_exporter.cpp")
    message(STATUS "Adding Metadata CatalogExporter focused tests")
    add_executable(test_catalog_exporter_focused test_catalog_exporter.cpp)
    target_include_directories(test_catalog_exporter_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_catalog_exporter_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::Crypto
        Threads::Threads
    )
    target_compile_definitions(test_catalog_exporter_focused PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME CatalogExporterFocusedTests COMMAND test_catalog_exporter_focused)
    set_tests_properties(CatalogExporterFocusedTests PROPERTIES
        LABELS "metadata;catalog;atlas;datahub;external-catalog;unit"
        TIMEOUT 60
    )
    message(STATUS "  CatalogExporter: Apache Atlas entity bulk API, DataHub MCP emission, injected HTTP (no network)")
endif()

# Metadata – SchemaAuditLog focused tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_schema_audit_log.cpp")
    message(STATUS "Adding Metadata SchemaAuditLog focused tests")
    add_executable(test_schema_audit_log_focused test_schema_audit_log.cpp)
    target_include_directories(test_schema_audit_log_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_schema_audit_log_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::Crypto
        Threads::Threads
    )
    target_compile_definitions(test_schema_audit_log_focused PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME SchemaAuditLogFocusedTests COMMAND test_schema_audit_log_focused)
    set_tests_properties(SchemaAuditLogFocusedTests PROPERTIES
        LABELS "metadata;audit;schema-changes;ddl;rocksdb;unit"
        TIMEOUT 60
    )
    message(STATUS "  SchemaAuditLog: durable per-table audit trail, JSON export, round-trip persistence")
endif()

# Metadata – SchemaConsistencyChecker focused tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_schema_consistency_checker.cpp")
    message(STATUS "Adding Metadata SchemaConsistencyChecker focused tests")
    add_executable(test_schema_consistency_checker_focused test_schema_consistency_checker.cpp)
    target_include_directories(test_schema_consistency_checker_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_schema_consistency_checker_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::Crypto
        Threads::Threads
    )
    target_compile_definitions(test_schema_consistency_checker_focused PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME SchemaConsistencyCheckerFocusedTests COMMAND test_schema_consistency_checker_focused)
    set_tests_properties(SchemaConsistencyCheckerFocusedTests PROPERTIES
        LABELS "metadata;consistency;background-scan;schema-health;unit"
        TIMEOUT 60
    )
    message(STATUS "  SchemaConsistencyChecker: background health scan, inconsistency detection and reporting")
endif()

# Metadata – IndexRecommender focused tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_index_recommender.cpp")
    message(STATUS "Adding Metadata IndexRecommender focused tests")
    add_executable(test_index_recommender_focused test_index_recommender.cpp)
    target_include_directories(test_index_recommender_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_index_recommender_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::Crypto
        Threads::Threads
    )
    target_compile_definitions(test_index_recommender_focused PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME IndexRecommenderFocusedTests COMMAND test_index_recommender_focused)
    set_tests_properties(IndexRecommenderFocusedTests PROPERTIES
        LABELS "metadata;index;recommendation;query-patterns;advisor;unit"
        TIMEOUT 60
    )
    message(STATUS "  IndexRecommender: query-pattern analysis, usage tracking, missing index detection, cost-benefit")
endif()

# Metadata – DistributedMetadataCatalog focused tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_distributed_catalog.cpp")
    message(STATUS "Adding Metadata DistributedMetadataCatalog focused tests")
    add_executable(test_distributed_catalog_focused test_distributed_catalog.cpp)
    target_include_directories(test_distributed_catalog_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_distributed_catalog_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::Crypto
        Threads::Threads
    )
    target_compile_definitions(test_distributed_catalog_focused PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME DistributedMetadataCatalogFocusedTests COMMAND test_distributed_catalog_focused)
    set_tests_properties(DistributedMetadataCatalogFocusedTests PROPERTIES
        LABELS "metadata;distributed;catalog;shards;replication;unit"
        TIMEOUT 90
    )
    message(STATUS "  DistributedMetadataCatalog: cross-shard catalog synchronisation, shard metadata lookup")
endif()

# ============================================================================
# Network Module – Focused Standalone Test Targets
# Standalone executables so each network component can be tested in isolation
# without linking the full themis_core library.
# ============================================================================

# Helper macro to create a standalone network focused test executable.
# Usage:
#   add_network_focused_test(<target> <src> <test_name> "<labels>" "<desc>" [LIBS ...])
macro(add_network_focused_test target src test_name labels desc)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${src}")
        message(STATUS "Adding Network focused test: ${target}")

        add_executable(${target} ${src})

        target_include_directories(${target} PRIVATE
            ${THEMIS_ROOT_DIR}/include
            ${CMAKE_SOURCE_DIR}/include
        )

        target_link_libraries(${target} PRIVATE
            ${TEST_LIBS}
            themis_core
            Threads::Threads
        )

        target_compile_definitions(${target} PRIVATE
            THEMIS_TEST_BUILD=1
        )

        add_test(NAME ${test_name} COMMAND ${target})

        set_tests_properties(${test_name} PROPERTIES
            LABELS "${labels}"
            TIMEOUT 60
        )

        message(STATUS "  ${desc}")
    else()
        message(STATUS "Network focused test not found: ${CMAKE_CURRENT_SOURCE_DIR}/${src}")
    endif()
endmacro()

# --- Wire Protocol V1 Opcode Handlers ---
add_network_focused_test(
    test_wire_protocol_v1_handlers_focused
    test_wire_protocol_v1_handlers.cpp
    WireProtocolV1HandlersFocusedTests
    "network;wire-protocol;v1;handlers;auth;get;put;delete;vector-search;focused;unit"
    "WireProtocolV1Handlers: Config::auth_token defaults, auth decision logic (3 modes), HELLO caps, GET/PUT/DELETE key format, VECTOR_SEARCH shape, QUERY_AQL/GEO_QUERY structured-error contract"
)

# --- Wire Protocol Backpressure / TCP Backlog Management ---
add_network_focused_test(
    test_wire_protocol_backpressure_focused
    test_wire_protocol_backpressure.cpp
    WireProtocolBackpressureFocusedTests
    "network;wire-protocol;backpressure;tcp-backlog;connection-limit;overload;focused;unit"
    "WireProtocolBackpressure: tcp_backlog default/custom, max_connections enforcement, overloaded_ state transitions, recovery logging, rejected_connections stats"
)

# --- Wire Protocol IPv6 Support ---
add_network_focused_test(
    test_wire_protocol_ipv6_focused
    test_wire_protocol_ipv6.cpp
    WireProtocolIPv6FocusedTests
    "network;wire-protocol;ipv6;dual-stack;bind-address;focused;unit"
    "WireProtocolIPv6: Config defaults (enable_ipv6, ipv6_dual_stack), address promotion logic, IPv6 address parsing/string, dual-stack semantics, connection-tracking keys"
)

# --- QoS Manager ---
add_network_focused_test(
    test_qos_manager_focused
    test_qos_manager.cpp
    QoSManagerFocusedTests
    "network;qos;token-bucket;bandwidth;priority;focused;unit"
    "QoSManager: TokenBucket construction/tryConsume/timeout, per-tenant quota, backpressure, stats, callback"
)

# --- Network Timeout (SocketTimeoutManager) ---
add_network_focused_test(
    test_network_timeout_focused
    test_network_timeout.cpp
    NetworkTimeoutFocusedTests
    "network;timeout;socket;circuit-breaker;focused;unit"
    "SocketTimeoutManager: construction/destruction, timeout recording, circuit breaker, health state transitions"
)

# --- Adaptive Circuit Breaker (network resilience) ---
add_network_focused_test(
    test_network_circuit_breaker_focused
    test_network_circuit_breaker.cpp
    NetworkCircuitBreakerFocusedTests
    "network;circuit-breaker;resilience;adaptive;recovery;focused;unit"
    "AdaptiveCircuitBreaker: CLOSED/OPEN/HALF_OPEN state machine, error threshold trigger, recovery scenarios, adaptive threshold, stats, callback, thread-safety"
)

# --- Wire Protocol Connection Pool ---
add_network_focused_test(
    test_wire_protocol_connection_pool_focused
    test_wire_protocol_connection_pool.cpp
    WireProtocolConnectionPoolFocusedTests
    "network;connection-pool;wire-protocol;lifecycle;focused;unit"
    "WireProtocolConnectionPool: config defaults, acquire/release, health tracking, RAII handles"
)

# --- Wire Protocol Performance Monitor ---
add_network_focused_test(
    test_wire_protocol_performance_focused
    test_wire_protocol_performance.cpp
    WireProtocolPerformanceFocusedTests
    "network;performance;metrics;wire-protocol;focused;unit"
    "WireProtocolPerformanceMonitor: latency tracking, throughput counters, percentile computation"
)

# --- UDP Fast Path ---
add_network_focused_test(
    test_udp_fast_path_focused
    test_udp_fast_path.cpp
    UDPFastPathFocusedTests
    "network;udp;fast-path;packet-validation;opcode-filter;focused;unit"
    "UDPFastPath: config defaults, packet format validation, opcode filter, response builder"
)

# --- Geo Topology Router ---
# Needs sharding sources for ShardTopology / ShardInfo types.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_geo_topology_router.cpp")
    message(STATUS "Adding Network focused test: test_geo_topology_router_focused")

    add_executable(test_geo_topology_router_focused
        test_geo_topology_router.cpp
    )

    target_include_directories(test_geo_topology_router_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_geo_topology_router_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_geo_topology_router_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME GeoTopologyRouterFocusedTests COMMAND test_geo_topology_router_focused)

    set_tests_properties(GeoTopologyRouterFocusedTests PROPERTIES
        LABELS "network;geo;topology;routing;sharding;focused;unit"
        TIMEOUT 60
    )

    message(STATUS "  GeoTopologyRouter: PREFER_LOCAL/LOWEST_LATENCY/ROUND_ROBIN strategies, zone/datacenter affinity, fallback, stats")
else()
    message(STATUS "Network focused test not found: ${CMAKE_CURRENT_SOURCE_DIR}/test_geo_topology_router.cpp")
endif()

# --- Wire Protocol V2 ---
# Uses themis/network/wire_protocol_v2.hpp (header-only protocol types).
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_wire_protocol_v2.cpp")
    message(STATUS "Adding Network focused test: test_wire_protocol_v2_focused")

    add_executable(test_wire_protocol_v2_focused
        test_wire_protocol_v2.cpp
    )

    target_include_directories(test_wire_protocol_v2_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(test_wire_protocol_v2_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_wire_protocol_v2_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME WireProtocolV2FocusedTests COMMAND test_wire_protocol_v2_focused)

    set_tests_properties(WireProtocolV2FocusedTests PROPERTIES
        LABELS "network;wire-protocol;v2;multiplexing;flow-control;server-push;focused;unit"
        TIMEOUT 60
    )

    message(STATUS "  WireProtocolV2: frame types, stream state machine, flow control, server push, session cleanup")
else()
    message(STATUS "Network focused test not found: ${CMAKE_CURRENT_SOURCE_DIR}/test_wire_protocol_v2.cpp")
endif()

# --- WebSocket Upgrade ---
if(THEMIS_ENABLE_WEBSOCKET)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_wire_protocol_websocket.cpp")
        message(STATUS "Adding Network focused test: test_wire_protocol_websocket_focused")

        add_executable(test_wire_protocol_websocket_focused
            test_wire_protocol_websocket.cpp
        )

        target_include_directories(test_wire_protocol_websocket_focused PRIVATE
            ${THEMIS_ROOT_DIR}/include
            ${CMAKE_SOURCE_DIR}/include
        )

        target_link_libraries(test_wire_protocol_websocket_focused PRIVATE
            ${TEST_LIBS}
            themis_core
            Threads::Threads
        )

        target_compile_definitions(test_wire_protocol_websocket_focused PRIVATE
            THEMIS_TEST_BUILD=1
            THEMIS_ENABLE_WEBSOCKET=1
        )

        add_test(NAME WireProtocolWebSocketFocusedTests COMMAND test_wire_protocol_websocket_focused)

        set_tests_properties(WireProtocolWebSocketFocusedTests PROPERTIES
            LABELS "network;websocket;upgrade;protocol-detection;focused;unit"
            TIMEOUT 60
        )

        message(STATUS "  WireProtocolWebSocket: protocol detection, upgrade handshake, connection accounting, text-frame dispatch")
    else()
        message(STATUS "Network focused test not found: ${CMAKE_CURRENT_SOURCE_DIR}/test_wire_protocol_websocket.cpp")
    endif()
endif()

# --- QUIC Transport ---
if(THEMIS_ENABLE_HTTP3)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_quic_transport.cpp")
        message(STATUS "Adding Network focused test: test_quic_transport_focused")

        add_executable(test_quic_transport_focused
            test_quic_transport.cpp
        )

        target_include_directories(test_quic_transport_focused PRIVATE
            ${THEMIS_ROOT_DIR}/include
            ${CMAKE_SOURCE_DIR}/include
        )

        target_link_libraries(test_quic_transport_focused PRIVATE
            ${TEST_LIBS}
            themis_core
            Threads::Threads
        )

        target_compile_definitions(test_quic_transport_focused PRIVATE
            THEMIS_TEST_BUILD=1
            THEMIS_ENABLE_HTTP3=1
        )

        add_test(NAME QuicTransportFocusedTests COMMAND test_quic_transport_focused)

        set_tests_properties(QuicTransportFocusedTests PROPERTIES
            LABELS "network;quic;http3;transport;config;focused;unit"
            TIMEOUT 60
        )

        message(STATUS "  QuicTransport: config defaults, port validation, stats initialisation, protocol constants, isRunning state")
    else()
        message(STATUS "Network focused test not found: ${CMAKE_CURRENT_SOURCE_DIR}/test_quic_transport.cpp")
    endif()
endif()

# --- HTTP/3 Production Readiness ---
if(THEMIS_ENABLE_HTTP3)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_http3_production_readiness.cpp")
        message(STATUS "Adding HTTP/3 focused test: test_http3_production_readiness_focused")

        add_executable(test_http3_production_readiness_focused
            test_http3_production_readiness.cpp
        )

        target_include_directories(test_http3_production_readiness_focused PRIVATE
            ${CMAKE_SOURCE_DIR}/../include
            ${CMAKE_SOURCE_DIR}/../src
        )

        target_link_libraries(test_http3_production_readiness_focused PRIVATE
            GTest::gtest_main
            GTest::gtest
            themis_core
        )

        target_compile_definitions(test_http3_production_readiness_focused PRIVATE
            THEMIS_ENABLE_HTTP3=1
        )

        add_test(NAME Http3ProductionReadinessFocusedTests
                 COMMAND test_http3_production_readiness_focused)

        set_tests_properties(Http3ProductionReadinessFocusedTests PROPERTIES
            LABELS "server;http3;quic;production;fallback;congestion;migration;focused;unit"
            TIMEOUT 60
        )

        message(STATUS "  HTTP/3 Production Readiness: congestion control, connection migration, 0-RTT, fallback, metrics")
    else()
        message(STATUS "HTTP/3 production readiness focused test not found: ${CMAKE_CURRENT_SOURCE_DIR}/test_http3_production_readiness.cpp")
    endif()
endif()

# --- gRPC Transport ---
if(THEMIS_ENABLE_GRPC)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_grpc_transport.cpp")
        message(STATUS "Adding Network focused test: test_grpc_transport_focused")

        add_executable(test_grpc_transport_focused
            test_grpc_transport.cpp
        )

        target_include_directories(test_grpc_transport_focused PRIVATE
            ${THEMIS_ROOT_DIR}/include
            ${CMAKE_SOURCE_DIR}/include
        )

        target_link_libraries(test_grpc_transport_focused PRIVATE
            ${TEST_LIBS}
            themis_core
            Threads::Threads
        )

        target_compile_definitions(test_grpc_transport_focused PRIVATE
            THEMIS_TEST_BUILD=1
            THEMIS_ENABLE_GRPC=1
        )

        add_test(NAME GrpcTransportFocusedTests COMMAND test_grpc_transport_focused)

        set_tests_properties(GrpcTransportFocusedTests PROPERTIES
            LABELS "network;grpc;transport;config;tls;focused;unit"
            TIMEOUT 60
        )

        message(STATUS "  GrpcTransport: config defaults, TLS flags, port validation, stats, address format, isRunning state")
    else()
        message(STATUS "Network focused test not found: ${CMAKE_CURRENT_SOURCE_DIR}/test_grpc_transport.cpp")
    endif()
endif()
# Prompt Engineering Module – Standalone Focused Test Targets
# Each target is an independent executable so individual components can be
# validated in isolation without linking the entire themis_tests binary.
# All targets compile against themis_core only; no live database is required.
# ============================================================================

# Helper macro shared by all prompt_engineering focused targets.
# Usage: add_prompt_focused_test(<target> <src> <test_name> <labels> <desc>)
macro(add_prompt_focused_test _target _src _test_name _labels _desc)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_src}")
        message(STATUS "Adding Prompt Engineering focused test: ${_target}")

        add_executable(${_target} ${_src})

        target_include_directories(${_target} PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
        )

        target_link_libraries(${_target} PRIVATE
            ${TEST_LIBS}
            themis_core
            nlohmann_json::nlohmann_json
            spdlog::spdlog
            Threads::Threads
        )

        target_compile_definitions(${_target} PRIVATE
            THEMIS_TEST_BUILD=1
        )

        add_test(NAME ${_test_name} COMMAND ${_target})

        set_tests_properties(${_test_name} PROPERTIES
            LABELS "${_labels}"
            TIMEOUT 120
        )

        message(STATUS "  ${_test_name}: ${_desc}")
    else()
        message(STATUS "Prompt Engineering focused test not found: ${CMAKE_CURRENT_SOURCE_DIR}/${_src}")
    endif()
endmacro()

add_prompt_focused_test(
    test_prompt_manager_focused
    test_prompt_manager.cpp
    PromptManagerFocusedTests
    "prompt_engineering;prompt_manager;crud;validation;yaml;unit"
    "CRUD with RocksDB persistence, template validation, context injection, YAML bulk-load"
)

add_prompt_focused_test(
    test_prompt_version_control_focused
    test_prompt_version_control.cpp
    PromptVersionControlFocusedTests
    "prompt_engineering;version_control;branches;commits;diffs;unit"
    "Git-like version control: branches, commits, diffs, A/B testing, parent tracking"
)

add_prompt_focused_test(
    test_feedback_collector_focused
    test_feedback_collector.cpp
    FeedbackCollectorFocusedTests
    "prompt_engineering;feedback;anomaly_detection;audit_checksum;unit"
    "FeedbackCollector: 10 feedback types, aggregate stats, Z-score anomaly detection, FNV-1a checksums"
)

add_prompt_focused_test(
    test_prompt_optimizer_focused
    test_prompt_optimizer.cpp
    PromptOptimizerFocusedTests
    "prompt_engineering;optimizer;iterative;eval;unit"
    "PromptOptimizer: iterative improvement loop, pluggable eval/improvement functions"
)

add_prompt_focused_test(
    test_meta_prompt_generator_focused
    test_meta_prompt_generator.cpp
    MetaPromptGeneratorFocusedTests
    "prompt_engineering;meta_prompt;llm_interface;fallback;unit"
    "MetaPromptGenerator: ILLMProvider interface, graceful fallback, strategy selection"
)

# ============================================================================
# Index Module Focused Tests (Issue: #1882 — unit test coverage >80%)
# Each test is a standalone executable covering a specific index subsystem.
# ============================================================================

function(add_index_focused_test target_name test_src test_name labels description)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${test_src}")
        message(STATUS "Adding Index focused test: ${target_name}")

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

        if(TARGET themis_storage)
            target_link_libraries(${target_name} PRIVATE themis_storage)
        endif()

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
        message(STATUS "Index focused test not found: ${CMAKE_CURRENT_SOURCE_DIR}/${test_src}")
    endif()
endfunction()

add_index_focused_test(
    test_ann_index_focused
    index/test_ann_index.cpp
    AnnIndexFocusedTests
    "index;ann;diskann;scann;focused;unit"
    "AnnIndex: DiskANN and ScaNN alternative ANN algorithms, on-disk vector search"
)

add_index_focused_test(
    test_distributed_vector_index_focused
    index/test_distributed_vector_index.cpp
    DistributedVectorIndexFocusedTests
    "index;distributed;vector;shards;focused;unit"
    "DistributedVectorIndex: vector search across shards, fan-out/merge, partition correctness"
)

# ============================================================================
# Cache Interfaces Tests (Issue: #1596 — unit test coverage >80%)
# ============================================================================
add_prompt_focused_test(
    test_prompt_performance_tracker_focused
    test_prompt_performance_tracker.cpp
    PromptPerformanceTrackerFocusedTests
    "prompt_engineering;performance_tracker;metrics;thresholds;unit"
    "PromptPerformanceTracker: execution stats, threshold alerts, percentile metrics"
)

add_prompt_focused_test(
    test_prompt_engineering_metrics_focused
    test_prompt_engineering_metrics.cpp
    PromptEngineeringMetricsFocusedTests
    "prompt_engineering;metrics;prometheus;persistence;crash_recovery;unit"
    "PromptEngineeringMetrics: Prometheus counters, snapshotToJson/restoreFromJson crash recovery"
)

add_prompt_focused_test(
    test_self_improvement_orchestrator_focused
    test_self_improvement_orchestrator.cpp
    SelfImprovementOrchestratorFocusedTests
    "prompt_engineering;self_improvement;orchestrator;background_worker;unit"
    "SelfImprovementOrchestrator: background worker, trigger thresholds, auto-optimization cycle"
)

add_prompt_focused_test(
    test_prompt_injection_detector_focused
    test_prompt_injection_detector.cpp
    PromptInjectionDetectorFocusedTests
    "prompt_engineering;injection_detector;security;sanitize;unit"
    "PromptInjectionDetector: 10 built-in patterns, detect/sanitize, custom plugin support"
)

add_prompt_focused_test(
    test_chain_of_thought_focused
    test_chain_of_thought.cpp
    ChainOfThoughtFocusedTests
    "prompt_engineering;chain_of_thought;cot;few_shot;zero_shot;unit"
    "ChainOfThoughtBuilder: step delimiters, auto-numbering, zero-shot/few-shot/wrap helpers"
)

add_prompt_focused_test(
    test_rag_prompt_builder_focused
    test_rag_prompt_builder.cpp
    RAGPromptBuilderFocusedTests
    "prompt_engineering;rag;budget;chunk_selection;citations;unit"
    "RAGPromptBuilder: budget-aware chunk selection, source citations, template injection"
)

add_prompt_focused_test(
    test_system_prompt_manager_focused
    test_system_prompt_manager.cpp
    SystemPromptManagerFocusedTests
    "prompt_engineering;system_prompt;roles;context_variables;json;unit"
    "SystemPromptManager: built-in and custom roles, context-variable rendering, JSON serialisation"
)

add_prompt_focused_test(
    test_prompt_evaluator_focused
    test_prompt_evaluator_ttest.cpp
    PromptEvaluatorFocusedTests
    "prompt_engineering;evaluator;welch_ttest;statistical_significance;unit"
    "PromptEvaluator: Welch's two-sample t-test, semantic similarity, IEmbeddingProvider interface"
)

add_prompt_focused_test(
    test_prompt_engineering_integration_focused
    test_prompt_engineering_integration.cpp
    PromptEngineeringIntegrationFocusedTests
    "prompt_engineering;integration;facade;lifecycle;unit"
    "PromptEngineeringIntegration: facade combining all subsystems, before/after execution hooks"
)

# ============================================================================
# Custom Test Targets
# ============================================================================

# ============================================================================
# Ingestion Module Focused Tests (Issue: #1910 — Integration tests)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_ingestion_integration.cpp" AND NOT TARGET test_ingestion_integration_focused)
    message(STATUS "Adding Ingestion Integration focused tests (Issue #1910)")

    add_executable(test_ingestion_integration_focused
        test_ingestion_integration.cpp
        ${THEMIS_ROOT_DIR}/src/ingestion/ingestion_manager.cpp
        ${THEMIS_ROOT_DIR}/src/ingestion/filesystem_ingester.cpp
        ${THEMIS_ROOT_DIR}/src/ingestion/api_connector.cpp
        ${THEMIS_ROOT_DIR}/src/ingestion/huggingface_connector.cpp
        ${THEMIS_ROOT_DIR}/src/ingestion/kafka_connector.cpp
        ${THEMIS_ROOT_DIR}/src/ingestion/object_storage_connector.cpp
    )

    target_include_directories(test_ingestion_integration_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_ingestion_integration_focused PRIVATE
        ${TEST_LIBS}
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_ingestion_integration_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME IngestionIntegrationFocusedTests COMMAND test_ingestion_integration_focused)

    set_tests_properties(IngestionIntegrationFocusedTests PROPERTIES
        LABELS "ingestion;integration;filesystem;huggingface;api;focused"
        TIMEOUT 120
    )

    message(STATUS "  IngestionIntegration: FileSystemIngester, HuggingFaceConnector, GenericApiConnector end-to-end flows")
else()
    if(TARGET test_ingestion_integration_focused)
        message(STATUS "Ingestion Integration focused tests already registered: test_ingestion_integration_focused")
    else()
        message(STATUS "Ingestion Integration focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_ingestion_integration.cpp")
    endif()
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_ingestion_coordinator.cpp" AND NOT TARGET test_ingestion_coordinator_focused)
    message(STATUS "Adding Ingestion Coordinator focused tests (Issue #1910)")

    add_executable(test_ingestion_coordinator_focused
        test_ingestion_coordinator.cpp
        ${THEMIS_ROOT_DIR}/src/ingestion/ingestion_coordinator.cpp
        ${THEMIS_ROOT_DIR}/src/ingestion/ingestion_manager.cpp
        ${THEMIS_ROOT_DIR}/src/ingestion/filesystem_ingester.cpp
        ${THEMIS_ROOT_DIR}/src/ingestion/api_connector.cpp
        ${THEMIS_ROOT_DIR}/src/ingestion/kafka_connector.cpp
        ${THEMIS_ROOT_DIR}/src/ingestion/object_storage_connector.cpp
        ${THEMIS_ROOT_DIR}/src/ingestion/huggingface_connector.cpp
    )

    target_include_directories(test_ingestion_coordinator_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_ingestion_coordinator_focused PRIVATE
        ${TEST_LIBS}
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_ingestion_coordinator_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME IngestionCoordinatorFocusedTests COMMAND test_ingestion_coordinator_focused)

    set_tests_properties(IngestionCoordinatorFocusedTests PROPERTIES
        LABELS "ingestion;coordinator;distributed;work-stealing;focused"
        TIMEOUT 120
    )

    message(STATUS "  IngestionCoordinator: distributed coordinator, consistent-hash ring, work-stealing pool, leader election")
else()
    if(TARGET test_ingestion_coordinator_focused)
        message(STATUS "Ingestion Coordinator focused tests already registered: test_ingestion_coordinator_focused")
    else()
        message(STATUS "Ingestion Coordinator focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_ingestion_coordinator.cpp")
    endif()
endif()

# ============================================================================
# Observability Focused Test Targets
# Each target links against themis_core so the full observability stack is
# available.  Tests can be run in isolation without building the whole suite.
# ============================================================================

# MetricsCollector tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_metrics_collector.cpp")
    message(STATUS "Adding Observability: MetricsCollector focused tests")

    add_executable(test_metrics_collector_focused
        test_metrics_collector.cpp
    )

    target_include_directories(test_metrics_collector_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_metrics_collector_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_metrics_collector_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME MetricsCollectorFocusedTests COMMAND test_metrics_collector_focused)

    set_tests_properties(MetricsCollectorFocusedTests PROPERTIES
        LABELS "observability;metrics;prometheus;focused;unit"
        TIMEOUT 60
    )

    message(STATUS "  MetricsCollector: counters, gauges, histograms, Prometheus text export")
else()
    message(STATUS "MetricsCollector focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_metrics_collector.cpp")
endif()

# MetricsExemplar tests (Issue #1995)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_metrics_exemplar.cpp")
    message(STATUS "Adding Observability: MetricsExemplar focused tests (Issue #1995)")

    add_executable(test_metrics_exemplar_focused
        test_metrics_exemplar.cpp
    )

    target_include_directories(test_metrics_exemplar_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_metrics_exemplar_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_metrics_exemplar_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME MetricsExemplarFocusedTests COMMAND test_metrics_exemplar_focused)

    set_tests_properties(MetricsExemplarFocusedTests PROPERTIES
        LABELS "observability;metrics;exemplar;prometheus;histogram;focused;unit"
        TIMEOUT 60
    )

    message(STATUS "  MetricsExemplar: Exemplar struct, observeHistogramWithExemplar, inline p99 exemplar export")
else()
    message(STATUS "MetricsExemplar focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_metrics_exemplar.cpp")
endif()

# MetricsAggregation focused tests (Prometheus advanced features: rate, histogram, cardinality)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_metrics_aggregation.cpp")
    message(STATUS "Adding Observability: MetricsAggregation focused tests (Prometheus advanced features)")

    add_executable(test_metrics_aggregation_focused
        test_metrics_aggregation.cpp
    )

    target_include_directories(test_metrics_aggregation_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_metrics_aggregation_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_metrics_aggregation_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME MetricsAggregationFocusedTests COMMAND test_metrics_aggregation_focused)

    set_tests_properties(MetricsAggregationFocusedTests PROPERTIES
        LABELS "observability;metrics;aggregation;rate;histogram;cardinality;prometheus;focused;unit"
        TIMEOUT 120
    )

    message(STATUS "  MetricsAggregation: rate calculation, histogram aggregation, rule-based aggregation, cardinality limits")
else()
    message(STATUS "MetricsAggregation focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_metrics_aggregation.cpp")
endif()

# AlertRules tests (Issue #2025)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_alert_rules.cpp")
    message(STATUS "Adding Observability: AlertRules focused tests (Issue #2025)")

    add_executable(test_alert_rules_focused
        test_alert_rules.cpp
    )

    target_include_directories(test_alert_rules_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_alert_rules_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_alert_rules_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME AlertRulesFocusedTests COMMAND test_alert_rules_focused)

    set_tests_properties(AlertRulesFocusedTests PROPERTIES
        LABELS "observability;alertmanager;alert-rules;focused;unit"
        TIMEOUT 60
    )

    message(STATUS "  AlertRules: AlertRule, AlertRuleOperator, AlertRuleManager CRUD, evaluateRules")
else()
    message(STATUS "AlertRules focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_alert_rules.cpp")
endif()

# AlertingEngine tests (rule-based alerting engine with notification channels)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_alerting_engine.cpp")
    message(STATUS "Adding Observability: AlertingEngine focused tests")

    add_executable(test_alerting_engine_focused
        test_alerting_engine.cpp
    )

    target_include_directories(test_alerting_engine_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_alerting_engine_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_alerting_engine_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME AlertingEngineFocusedTests COMMAND test_alerting_engine_focused)

    set_tests_properties(AlertingEngineFocusedTests PROPERTIES
        LABELS "observability;alerting-engine;notification-channels;focused;unit"
        TIMEOUT 60
    )

    message(STATUS "  AlertingEngine: INotificationChannel, LogChannel, WebhookChannel, SlackChannel, AlertingEngine CRUD, evaluateAndNotify, default rules")
else()
    message(STATUS "AlertingEngine focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_alerting_engine.cpp")
endif()

# MetricsStreamServer focused tests (v1.6.0 — real-time metric streaming, Issue #82)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_metrics_stream_server.cpp")
    message(STATUS "Adding Observability: MetricsStreamServer focused tests (v1.6.0 streaming metrics)")

    add_executable(test_metrics_stream_server_focused
        test_metrics_stream_server.cpp
    )

    target_include_directories(test_metrics_stream_server_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_metrics_stream_server_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_metrics_stream_server_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME MetricsStreamServerFocusedTests COMMAND test_metrics_stream_server_focused)

    set_tests_properties(MetricsStreamServerFocusedTests PROPERTIES
        LABELS "observability;streaming;websocket;sse;metrics;focused;unit"
        TIMEOUT 120
    )

    message(STATUS "  MetricsStreamServer: subscribe/unsubscribe, pushMetrics, label filters, rate-limiting, serialisation, thread-safety")
else()
    message(STATUS "MetricsStreamServer focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_metrics_stream_server.cpp")
endif()


if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_continuous_profiler.cpp")
    message(STATUS "Adding Observability: ContinuousProfiler focused tests (Issue #2418)")

    add_executable(test_continuous_profiler_focused
        test_continuous_profiler.cpp
    )

    target_include_directories(test_continuous_profiler_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_continuous_profiler_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_continuous_profiler_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ContinuousProfilerFocusedTests COMMAND test_continuous_profiler_focused)

    set_tests_properties(ContinuousProfilerFocusedTests PROPERTIES
        LABELS "observability;profiling;pprof;adaptive-sampling;focused;unit"
        TIMEOUT 60
    )

    message(STATUS "  ContinuousProfiler: lifecycle, pprof export, adaptive sampling, thread-safety")
else()
    message(STATUS "ContinuousProfiler focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_continuous_profiler.cpp")
endif()

# ObservabilityTracer tests (OBS-MISSING-001)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_tracer.cpp")
    message(STATUS "Adding Observability: ObservabilityTracer focused tests (OBS-MISSING-001)")

    add_executable(test_tracer_focused
        test_tracer.cpp
    )

    target_include_directories(test_tracer_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_tracer_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_tracer_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ObservabilityTracerFocusedTests COMMAND test_tracer_focused)

    set_tests_properties(ObservabilityTracerFocusedTests PROPERTIES
        LABELS "observability;tracing;w3c-trace-context;spans;sampling;focused;unit"
        TIMEOUT 60
    )

    message(STATUS "  ObservabilityTracer: lifecycle, span creation, W3C traceparent, child spans, sampling, ring buffer, thread-safety")
else()
    message(STATUS "ObservabilityTracer focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_tracer.cpp")
endif()

# LogAggregator tests (OBS-MISSING-001)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_log_aggregator.cpp")
    message(STATUS "Adding Observability: LogAggregator focused tests (OBS-MISSING-001)")

    add_executable(test_log_aggregator_focused
        test_log_aggregator.cpp
    )

    target_include_directories(test_log_aggregator_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_log_aggregator_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_log_aggregator_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME LogAggregatorFocusedTests COMMAND test_log_aggregator_focused)

    set_tests_properties(LogAggregatorFocusedTests PROPERTIES
        LABELS "observability;logging;structured-log;trace-context;ring-buffer;focused;unit"
        TIMEOUT 60
    )

    message(STATUS "  LogAggregator: level filtering, structured fields, trace context, ring buffer, callback, shutdown, thread-safety")
else()
    message(STATUS "LogAggregator focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_log_aggregator.cpp")
endif()

# ============================================================================
# Training Module Focused Tests
# Standalone targets that link only the training sources they exercise,
# enabling fast iteration without recompiling the full themis_core library.
# ============================================================================

# ModalityParser focused tests – covers ModalityDetector, all four extractors,
# and ConfidenceCalibrator from the same test file.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_training_convergence.cpp")
    message(STATUS "Adding ModalityParser focused tests (Phase 3)")

    add_executable(test_modality_parser_focused
        test_training_convergence.cpp
        ${THEMIS_ROOT_DIR}/src/training/modality_parser.cpp
        ${THEMIS_ROOT_DIR}/src/training/training_pipeline.cpp
        ${THEMIS_ROOT_DIR}/src/training/auto_labeler.cpp
        ${THEMIS_ROOT_DIR}/src/training/knowledge_graph_enricher.cpp
        ${THEMIS_ROOT_DIR}/src/training/incremental_lora_trainer.cpp
        ${THEMIS_ROOT_DIR}/src/training/lora_data_selection.cpp
        ${THEMIS_ROOT_DIR}/src/training/lora_checkpoint_manager.cpp
        ${THEMIS_ROOT_DIR}/src/training/provenance_tracker.cpp
    )

    target_include_directories(test_modality_parser_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_modality_parser_focused PRIVATE
        ${TEST_LIBS}
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_modality_parser_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ModalityParserFocusedTests COMMAND test_modality_parser_focused)

    set_tests_properties(ModalityParserFocusedTests PROPERTIES
        LABELS "training;modality;legal;calibration;ConfidenceCalibrator;PAV;focused;unit"
        TIMEOUT 120
    )

    message(STATUS "  ModalityParser: ModalityDetector heuristics, TextClause/Table/Citation/OCR extractors, ConfidenceCalibrator PAV, batch parse")
else()
    message(STATUS "Modality parser focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_training_convergence.cpp")
endif()

# TrainingConvergence alias – same binary, separate CTest entry for labelling
if(TARGET test_modality_parser_focused)
    add_test(NAME TrainingConvergenceFocusedTests COMMAND test_modality_parser_focused)

    set_tests_properties(TrainingConvergenceFocusedTests PROPERTIES
        LABELS "training;convergence;calibration;isotonic;legal;focused;unit"
        TIMEOUT 120
    )

    message(STATUS "  TrainingConvergence: ConfidenceCalibrator PAV isotonic regression, F1 threshold selection, multi-category")
endif()

# AutoLabelerDbFetch focused tests – covers DB fetch paths in labelAll() and
# labelQuery() by wiring a real QueryEngine (RocksDB + SecondaryIndexManager).
# Runs as a gtest_filter slice of themis_tests so it inherits all core libs.
if(TARGET themis_tests)
    add_test(NAME AutoLabelerDbFetchFocusedTests
        COMMAND themis_tests
            --gtest_filter=AutoLabelerDbFetchTest.*:AutoLabelerOfflineTest.*)

    set_tests_properties(AutoLabelerDbFetchFocusedTests PROPERTIES
        LABELS "training;auto-labeler;db-fetch;aql;legal;focused;integration"
        TIMEOUT 60
    )

    message(STATUS "Adding AutoLabelerDbFetch focused tests")
    message(STATUS "  AutoLabelerDbFetch: labelAll/labelQuery DB fetch via AQL executor, offline fallback, callback, stats")
endif()

# KgeVectorSearch focused tests – covers findSimilarDocuments() wired to a
# real VectorIndexManager (FINDING-T-002 resolution, v1.6.0).
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_kge_vector_search.cpp")
    message(STATUS "Adding KgeVectorSearch focused tests (knowledge_graph_enricher + VectorIndexManager)")

    add_executable(test_kge_vector_search_focused
        test_kge_vector_search.cpp
        ${THEMIS_ROOT_DIR}/src/training/knowledge_graph_enricher.cpp
    )

    target_include_directories(test_kge_vector_search_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_kge_vector_search_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        RocksDB::rocksdb
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_kge_vector_search_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME KgeVectorSearchFocusedTests COMMAND test_kge_vector_search_focused)

    set_tests_properties(KgeVectorSearchFocusedTests PROPERTIES
        LABELS "training;knowledge-graph;vector-search;cosine;similarity;focused;integration"
        TIMEOUT 60
    )

    message(STATUS "  KgeVectorSearch: offline stub, setVectorIndex wiring, self-exclusion, max_results, score range, nearest-neighbor ranking, nullptr reset")
else()
    message(STATUS "KgeVectorSearch focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_kge_vector_search.cpp")
endif()

# LoRAAdapter focused tests – real weight manipulation (no simulation)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_training_lora_adapter.cpp")
    message(STATUS "Adding LoRAAdapter focused tests (lora_adapter.cpp)")

    add_executable(test_lora_adapter_focused
        test_training_lora_adapter.cpp
        ${THEMIS_ROOT_DIR}/src/training/lora_adapter.cpp
    )

    target_include_directories(test_lora_adapter_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_lora_adapter_focused PRIVATE
        ${TEST_LIBS}
        Threads::Threads
    )

    target_compile_definitions(test_lora_adapter_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME LoRAAdapterFocusedTests COMMAND test_lora_adapter_focused)

    set_tests_properties(LoRAAdapterFocusedTests PROPERTIES
        LABELS "training;lora;weight-manipulation;real;focused;unit"
        TIMEOUT 60
    )

    message(STATUS "  LoRAAdapter: addLayer/removeLayer, Kaiming init, setWeights, applyUpdate, applyBatchUpdate, forward pass, export/import")
else()
    message(STATUS "LoRAAdapter focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_training_lora_adapter.cpp")
endif()

# AdvancedTrainingFeatures focused tests – quantization config, multi-GPU config,
# training metrics tracking, checkpoint/resume enhancements (feat/advanced-training-features)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_advanced_training_features.cpp")
    message(STATUS "Adding AdvancedTrainingFeatures focused tests")

    add_executable(test_advanced_training_features
        test_advanced_training_features.cpp
        ${THEMIS_ROOT_DIR}/src/training/incremental_lora_trainer.cpp
        ${THEMIS_ROOT_DIR}/src/training/lora_checkpoint_manager.cpp
        ${THEMIS_ROOT_DIR}/src/training/provenance_tracker.cpp
        ${THEMIS_ROOT_DIR}/src/utils/checksum_utils.cpp
    )

    target_include_directories(test_advanced_training_features PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_advanced_training_features PRIVATE
        ${TEST_LIBS}
        spdlog::spdlog
        OpenSSL::Crypto
        Threads::Threads
    )

    target_compile_definitions(test_advanced_training_features PRIVATE
        THEMIS_TEST_BUILD=1
        THEMIS_NO_SPDLOG=1
    )

    add_test(NAME AdvancedTrainingFeaturesFocusedTests
             COMMAND test_advanced_training_features)

    set_tests_properties(AdvancedTrainingFeaturesFocusedTests PROPERTIES
        LABELS "training;quantization;multi-gpu;metrics;checkpoint;advanced;focused;unit"
        TIMEOUT 120
    )

    message(STATUS "  AdvancedTrainingFeatures: QuantizationConfig, multi-GPU config, TrainingMetrics, EpochMetrics, getMetrics(), checkpoint dir")
else()
    message(STATUS "AdvancedTrainingFeatures focused tests not found")
endif()

# ──────────────────────────────────────────────────────────────────────────────
# Voice Module Focused Tests
# ──────────────────────────────────────────────────────────────────────────────

# VoiceProduction focused tests – covers all 10 production-readiness phases
# (AudioPreprocessing, WakeWordDetector, VoiceIntentDetector, VoiceSessionManager,
#  VoiceSecurityManager, VoiceErrorHandler, VoiceTTSCustomizer, VoiceMeetingSupport,
#  VoiceAudioStorage, VoiceAccessibility, VoiceModelCache, VoiceBatchProcessor,
#  VoiceBiometricAuthenticator, EmotionAnalyzer)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_voice_production.cpp")
    message(STATUS "Adding Voice Production focused tests")

    add_executable(test_voice_production_focused
        test_voice_production.cpp
        ${THEMIS_ROOT_DIR}/src/voice/audio_preprocessing.cpp
        ${THEMIS_ROOT_DIR}/src/voice/wake_word_detector.cpp
        ${THEMIS_ROOT_DIR}/src/voice/voice_intent_detector.cpp
        ${THEMIS_ROOT_DIR}/src/voice/voice_session_manager.cpp
        ${THEMIS_ROOT_DIR}/src/voice/voice_security.cpp
        ${THEMIS_ROOT_DIR}/src/voice/voice_error_handler.cpp
        ${THEMIS_ROOT_DIR}/src/voice/voice_tts_customizer.cpp
        ${THEMIS_ROOT_DIR}/src/voice/voice_meeting_support.cpp
        ${THEMIS_ROOT_DIR}/src/voice/voice_audio_storage.cpp
        ${THEMIS_ROOT_DIR}/src/voice/voice_accessibility.cpp
        ${THEMIS_ROOT_DIR}/src/voice/voice_model_cache.cpp
        ${THEMIS_ROOT_DIR}/src/voice/voice_batch_processor.cpp
        ${THEMIS_ROOT_DIR}/src/voice/voice_authenticator.cpp
        ${THEMIS_ROOT_DIR}/src/voice/voice_macro_manager.cpp
        ${THEMIS_ROOT_DIR}/src/voice/emotion_analyzer.cpp
    )

    target_include_directories(test_voice_production_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_voice_production_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_voice_production_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME VoiceProductionFocusedTests COMMAND test_voice_production_focused)

    set_tests_properties(VoiceProductionFocusedTests PROPERTIES
        LABELS "voice;audio;wake-word;intent;session;security;tts;meeting;emotion;production;focused;unit"
        TIMEOUT 120
    )

    message(STATUS "  VoiceProduction: AudioPreprocessing, WakeWordDetector, VoiceIntentDetector, VoiceSessionManager, EmotionAnalyzer, VoiceSecurityManager")
else()
    message(STATUS "Voice Production focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_voice_production.cpp")
endif()

# BlueGreenDeployment – focused tests for blue/green deployment support
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_blue_green_deployment.cpp" AND NOT TARGET test_blue_green_deployment_focused)
    message(STATUS "Adding Blue/Green Deployment focused tests")

    add_executable(test_blue_green_deployment_focused
        test_blue_green_deployment.cpp
    )

    target_include_directories(test_blue_green_deployment_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_blue_green_deployment_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_blue_green_deployment_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME BlueGreenDeploymentFocusedTests COMMAND test_blue_green_deployment_focused)

    set_tests_properties(BlueGreenDeploymentFocusedTests PROPERTIES
        LABELS "updates;blue-green;deployment;zero-downtime;focused;unit"
        TIMEOUT 120
    )

    message(STATUS "  BlueGreenDeployment: Blue/Green deployment support")
else()
    message(STATUS "Blue/Green Deployment focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_blue_green_deployment.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/exporters/test_huggingface_exporter.cpp" AND NOT TARGET test_huggingface_exporter_focused)
    message(STATUS "Adding HuggingFace Exporter focused tests (Issue #1729)")

    add_executable(test_huggingface_exporter_focused
        exporters/test_huggingface_exporter.cpp
        ${THEMIS_ROOT_DIR}/src/exporters/huggingface_exporter.cpp
        ${THEMIS_ROOT_DIR}/src/exporters/exporter_metrics.cpp
        ${THEMIS_ROOT_DIR}/src/exporters/stream_writer.cpp
        ${THEMIS_ROOT_DIR}/src/exporters/pii_detector.cpp
    )

    target_include_directories(test_huggingface_exporter_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_huggingface_exporter_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_huggingface_exporter_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME HuggingFaceExporterFocusedTests COMMAND test_huggingface_exporter_focused)

    set_tests_properties(HuggingFaceExporterFocusedTests PROPERTIES
        LABELS "exporter;huggingface;ml;focused;unit"
        TIMEOUT 120
    )

    message(STATUS "  HuggingFaceExporter: Model export to HuggingFace Hub")
else()
    message(STATUS "HuggingFace Exporter focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/exporters/test_huggingface_exporter.cpp")
endif()

# VoiceCoverage focused tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_voice_coverage.cpp" AND NOT TARGET test_voice_coverage_focused)
    message(STATUS "Adding Voice Coverage focused tests")

    add_executable(test_voice_coverage_focused
        test_voice_coverage.cpp
        ${THEMIS_ROOT_DIR}/src/voice/voice_session_manager.cpp
        ${THEMIS_ROOT_DIR}/src/voice/voice_audio_storage.cpp
        ${THEMIS_ROOT_DIR}/src/voice/voice_meeting_support.cpp
        ${THEMIS_ROOT_DIR}/src/voice/voice_error_handler.cpp
        ${THEMIS_ROOT_DIR}/src/voice/voice_security.cpp
        ${THEMIS_ROOT_DIR}/src/voice/voice_tts_customizer.cpp
        ${THEMIS_ROOT_DIR}/src/voice/voice_intent_detector.cpp
        ${THEMIS_ROOT_DIR}/src/voice/voice_accessibility.cpp
    )

    target_include_directories(test_voice_coverage_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_voice_coverage_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_voice_coverage_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME VoiceCoverageFocusedTests COMMAND test_voice_coverage_focused)
endif()

# CanaryRollout – focused tests for canary rollout mode
# CanaryRollout – focused tests for canary rollout mode
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_canary_rollout.cpp" AND NOT TARGET test_canary_rollout_focused)
    message(STATUS "Adding Canary Rollout focused tests")

    add_executable(test_canary_rollout_focused
        test_canary_rollout.cpp
    )

    target_include_directories(test_canary_rollout_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_canary_rollout_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_canary_rollout_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME CanaryRolloutFocusedTests COMMAND test_canary_rollout_focused)

    set_tests_properties(CanaryRolloutFocusedTests PROPERTIES
        LABELS "deployment;canary;rollout;focused;unit"
        TIMEOUT 90
    )

    message(STATUS "  CanaryRollout: canary deployment, traffic shifting, metrics validation, automatic rollback")
else()
    message(STATUS "Canary Rollout focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_canary_rollout.cpp")
endif()

# BinaryDeltaPatches – focused tests for DeltaUpdateEngine (v1.6.0, Issue #126)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_binary_delta_patches.cpp" AND NOT TARGET test_binary_delta_patches_focused)
    message(STATUS "Adding Binary Delta Patches focused tests (Issue #126)")

    add_executable(test_binary_delta_patches_focused
        test_binary_delta_patches.cpp
    )

    target_include_directories(test_binary_delta_patches_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_binary_delta_patches_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_binary_delta_patches_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME BinaryDeltaPatchesFocusedTests COMMAND test_binary_delta_patches_focused)

    set_tests_properties(BinaryDeltaPatchesFocusedTests PROPERTIES
        LABELS "updates;delta;binary-diff;patches;zstd;vcdiff;checksums;fallback;focused;unit;integration"
        TIMEOUT 120
    )

    message(STATUS "  BinaryDeltaPatches: diff generation (ZSTD_DICT/VCDIFF), checksum verification, fallback, CI/CD patch API, path traversal security")
else()
    message(STATUS "Binary Delta Patches focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_binary_delta_patches.cpp")
endif()

# VoiceAssistant focused tests – covers VoiceBiometricAuthenticator, WakeWordDetector,
# VoiceBatchProcessor, VoiceMacroManager, and full VoiceAssistant pipeline.
# Requires THEMIS_ENABLE_LLM because voice_assistant.h includes llm/llama_wrapper.h.
if(THEMIS_ENABLE_LLM)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_voice_assistant.cpp")
        message(STATUS "Adding Voice Assistant focused tests (requires THEMIS_ENABLE_LLM)")

        add_executable(test_voice_assistant_focused
            test_voice_assistant.cpp
            ${THEMIS_ROOT_DIR}/src/voice/voice_authenticator.cpp
            ${THEMIS_ROOT_DIR}/src/voice/wake_word_detector.cpp
            ${THEMIS_ROOT_DIR}/src/voice/voice_batch_processor.cpp
            ${THEMIS_ROOT_DIR}/src/voice/voice_macro_manager.cpp
            ${THEMIS_ROOT_DIR}/src/voice/audio_preprocessing.cpp
            ${THEMIS_ROOT_DIR}/src/voice/voice_intent_detector.cpp
            ${THEMIS_ROOT_DIR}/src/voice/voice_session_manager.cpp
            ${THEMIS_ROOT_DIR}/src/voice/voice_security.cpp
            ${THEMIS_ROOT_DIR}/src/voice/voice_error_handler.cpp
            ${THEMIS_ROOT_DIR}/src/voice/voice_tts_customizer.cpp
            ${THEMIS_ROOT_DIR}/src/voice/voice_meeting_support.cpp
            ${THEMIS_ROOT_DIR}/src/voice/voice_audio_storage.cpp
            ${THEMIS_ROOT_DIR}/src/voice/voice_accessibility.cpp
            ${THEMIS_ROOT_DIR}/src/voice/voice_model_cache.cpp
            ${THEMIS_ROOT_DIR}/src/voice/emotion_analyzer.cpp
            ${THEMIS_ROOT_DIR}/src/voice/voice_assistant.cpp
            ${THEMIS_ROOT_DIR}/src/voice/voice_assistant_llm.cpp
            ${THEMIS_ROOT_DIR}/src/content/stt_processor.cpp
            ${THEMIS_ROOT_DIR}/src/content/tts_processor.cpp
        )

        target_include_directories(test_voice_assistant_focused PRIVATE
            ${THEMIS_ROOT_DIR}/include
            ${THEMIS_ROOT_DIR}/src
        )

        target_link_libraries(test_voice_assistant_focused PRIVATE
            ${TEST_LIBS}
            nlohmann_json::nlohmann_json
            spdlog::spdlog
            Threads::Threads
        )

        target_compile_definitions(test_voice_assistant_focused PRIVATE
            THEMIS_TEST_BUILD=1
        )

        add_test(NAME VoiceAssistantFocusedTests COMMAND test_voice_assistant_focused)

        set_tests_properties(VoiceAssistantFocusedTests PROPERTIES
            LABELS "voice;assistant;biometric;authentication;speaker-verification;wake-word;macros;focused;unit"
            TIMEOUT 180
        )

        message(STATUS "  VoiceAssistant: VoiceBiometricAuthenticator enroll/verify/identify, liveness detection, WakeWordDetector, VoiceBatchProcessor, VoiceMacroManager, VoiceAssistant pipeline")
    else()
        message(STATUS "Voice Assistant focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_voice_assistant.cpp")
    endif()
endif()

# VoiceBrowserStreaming focused tests – covers VoiceStreamingSession lifecycle,
# frame processing, streaming stats, partial-result emission, and error handling.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_voice_browser_streaming.cpp")
    message(STATUS "Adding Voice Browser Streaming focused tests")

    add_executable(test_voice_browser_streaming_focused
        test_voice_browser_streaming.cpp
        ${THEMIS_ROOT_DIR}/src/voice/voice_browser_streaming.cpp
    )

    target_include_directories(test_voice_browser_streaming_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_voice_browser_streaming_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_voice_browser_streaming_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME VoiceBrowserStreamingFocusedTests COMMAND test_voice_browser_streaming_focused)

    set_tests_properties(VoiceBrowserStreamingFocusedTests PROPERTIES
        LABELS "voice;streaming;websocket;browser;focused;unit"
        TIMEOUT 120
    )

    message(STATUS "  VoiceBrowserStreaming: VoiceStreamingSession lifecycle, frame processing, streaming stats, error handling")
else()
    message(STATUS "Voice Browser Streaming focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_voice_browser_streaming.cpp")
endif()

# VoiceBrowserStreaming focused tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_voice_browser_streaming.cpp" AND NOT TARGET test_voice_browser_streaming_focused)
    message(STATUS "Adding Voice Browser Streaming focused tests")

    add_executable(test_voice_browser_streaming_focused
        test_voice_browser_streaming.cpp
        ${THEMIS_ROOT_DIR}/src/voice/voice_browser_streaming.cpp
    )

    target_link_libraries(test_voice_browser_streaming_focused PRIVATE
        ${TEST_LIBS}
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_voice_browser_streaming_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME VoiceBrowserStreamingFocusedTests COMMAND test_voice_browser_streaming_focused)
endif()

# HuggingFace exporter focused tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/exporters/test_huggingface_exporter.cpp" AND NOT TARGET test_huggingface_exporter_focused)
    message(STATUS "Adding HuggingFace Exporter focused tests (Issue #1729)")

    add_executable(test_huggingface_exporter_focused
        exporters/test_huggingface_exporter.cpp
        ${THEMIS_ROOT_DIR}/src/exporters/huggingface_exporter.cpp
        ${THEMIS_ROOT_DIR}/src/exporters/exporter_metrics.cpp
        ${THEMIS_ROOT_DIR}/src/exporters/stream_writer.cpp
        ${THEMIS_ROOT_DIR}/src/exporters/pii_detector.cpp
    )

    target_include_directories(test_huggingface_exporter_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_huggingface_exporter_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_huggingface_exporter_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME HuggingFaceExporterFocusedTests COMMAND test_huggingface_exporter_focused)
endif()

# Data augmentation focused tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/exporters/test_data_augmentation.cpp" AND NOT TARGET test_data_augmentation_focused)
    message(STATUS "Adding Data Augmentation focused tests (Issue #1729)")

    add_executable(test_data_augmentation_focused
        exporters/test_data_augmentation.cpp
        ${THEMIS_ROOT_DIR}/src/exporters/data_augmentation.cpp
    )

    target_include_directories(test_data_augmentation_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_data_augmentation_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_link_libraries(test_in_place_schema_migrator_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_data_augmentation_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME DataAugmentationFocusedTests COMMAND test_data_augmentation_focused)

    set_tests_properties(DataAugmentationFocusedTests PROPERTIES
        LABELS "exporters;data-augmentation;focused"
        TIMEOUT 60
    )
else()
    message(STATUS "Data Augmentation focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/exporters/test_data_augmentation.cpp")
endif()

# VoiceTelephony focused tests – covers SipCallSession, WebRtcCallSession,
# IVREngine, and TelephonyBridge coordinator.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_voice_telephony.cpp" AND NOT TARGET test_voice_telephony_focused)
    message(STATUS "Adding Voice Telephony focused tests")

    add_executable(test_voice_telephony_focused
        test_voice_telephony.cpp
        ${THEMIS_ROOT_DIR}/src/voice/voice_telephony.cpp
    )

    target_include_directories(test_voice_telephony_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_voice_telephony_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_voice_telephony_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME VoiceTelephonyFocusedTests COMMAND test_voice_telephony_focused)

    set_tests_properties(VoiceTelephonyFocusedTests PROPERTIES
        LABELS "voice;telephony;sip;webrtc;ivr;bridge;focused;unit"
        TIMEOUT 120
    )
endif()

# NotificationWebhook – focused tests for Slack/PagerDuty update notifications
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_notification_webhook.cpp" AND NOT TARGET test_notification_webhook_focused)
    message(STATUS "Adding Notification Webhook focused tests")

    add_executable(test_notification_webhook_focused
        test_notification_webhook.cpp
    )

    target_include_directories(test_notification_webhook_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_notification_webhook_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_notification_webhook_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME NotificationWebhookFocusedTests COMMAND test_notification_webhook_focused)

    set_tests_properties(NotificationWebhookFocusedTests PROPERTIES
        LABELS "updates;notification;webhook;slack;pagerduty;focused;unit"
        TIMEOUT 120
    )
endif()

# PreflightHealthCheck – focused tests for pre-update health checks
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_preflight_health_check.cpp")
    message(STATUS "Adding Preflight Health Check focused tests")

    add_executable(test_preflight_health_check_focused
        test_preflight_health_check.cpp
    )

    target_include_directories(test_preflight_health_check_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_preflight_health_check_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_preflight_health_check_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME PreflightHealthCheckFocusedTests COMMAND test_preflight_health_check_focused)

    set_tests_properties(PreflightHealthCheckFocusedTests PROPERTIES
        LABELS "updates;preflight;health-check;disk-space;memory;dependency;focused;unit"
        TIMEOUT 60
    )

    message(STATUS "  PreflightHealthCheck: DiskSpaceChecker, MemoryHeadroomChecker, DependencyVersionChecker, injectable providers, <=2s completion")
else()
    if(TARGET test_data_augmentation_focused)
        message(STATUS "Data Augmentation focused tests already registered: test_data_augmentation_focused")
    else()
        message(STATUS "Data Augmentation focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/exporters/test_data_augmentation.cpp")
    endif()
    message(STATUS "Preflight Health Check focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_preflight_health_check.cpp")
endif()

# SchemaMigrationTester – focused tests for schema migration testing framework
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_schema_migration_tester.cpp")
    message(STATUS "Adding Schema Migration Tester focused tests")

    add_executable(test_schema_migration_tester_focused
        test_schema_migration_tester.cpp
    )

    target_include_directories(test_schema_migration_tester_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_schema_migration_tester_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        RocksDB::rocksdb
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_schema_migration_tester_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME SchemaMigrationTesterFocusedTests COMMAND test_schema_migration_tester_focused)

    set_tests_properties(SchemaMigrationTesterFocusedTests PROPERTIES
        LABELS "updates;schema-migration;testing-framework;staging;focused;unit"
        TIMEOUT 120
    )

    message(STATUS "  SchemaMigrationTester: apply to staging before production, rollback on failure, version compatibility checks")
else()
    message(STATUS "Schema Migration Tester focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_schema_migration_tester.cpp")
endif()

# ParallelFileDownloads – focused tests for parallel downloader (Issue #128 / v1.6.0)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_parallel_file_downloads.cpp")
    message(STATUS "Adding Parallel File Downloads focused tests")

    add_executable(test_parallel_file_downloads_focused
        test_parallel_file_downloads.cpp
    )

    target_include_directories(test_parallel_file_downloads_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_parallel_file_downloads_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_parallel_file_downloads_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME ParallelFileDownloadsFocusedTests COMMAND test_parallel_file_downloads_focused)

    set_tests_properties(ParallelFileDownloadsFocusedTests PROPERTIES
        LABELS "updates;parallel-downloads;concurrency;bandwidth;priority;resume;focused;unit"
        TIMEOUT 120
    )

    message(STATUS "  ParallelFileDownloads: configurable concurrency, bandwidth throttling, priority queue, resume support")
else()
    message(STATUS "Parallel File Downloads focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_parallel_file_downloads.cpp")
endif()

# DependencyResolutionEngine – focused tests for update dependency resolver (Issue #216 / v1.6.0)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_dependency_resolution_engine.cpp")
    message(STATUS "Adding Dependency Resolution Engine focused tests")

    add_executable(test_dependency_resolution_engine_focused
        test_dependency_resolution_engine.cpp
    )

    target_include_directories(test_dependency_resolution_engine_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_dependency_resolution_engine_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_dependency_resolution_engine_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME DependencyResolutionEngineFocusedTests
        COMMAND test_dependency_resolution_engine_focused)

    set_tests_properties(DependencyResolutionEngineFocusedTests PROPERTIES
        LABELS "updates;dependency-resolution;topological-sort;cycle-detection;version-constraints;conflict-resolution;backfill;focused;unit;v1.6.0"
        TIMEOUT 60
    )

    message(STATUS "  DependencyResolutionEngine: dependency graph, topological sort, cycle detection, version constraints, conflict resolution, backfill")
else()
    message(STATUS "Dependency Resolution Engine focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_dependency_resolution_engine.cpp")
endif()

# DistributedGateway – focused tests for multi-node API gateway
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_distributed_gateway.cpp")
    message(STATUS "Adding Distributed Gateway focused tests")

    add_executable(test_distributed_gateway_focused
        test_distributed_gateway.cpp
    )

    target_include_directories(test_distributed_gateway_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_distributed_gateway_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_distributed_gateway_focused PRIVATE
        THEMIS_TEST_BUILD=1
        THEMIS_ENABLE_HTTP_SERVER=1
    )

    add_test(NAME DistributedGatewayFocusedTests COMMAND test_distributed_gateway_focused)

    set_tests_properties(DistributedGatewayFocusedTests PROPERTIES
        LABELS "server;gateway;distributed;raft;failover;affinity;routing;focused;unit"
        TIMEOUT 120
    )

    message(STATUS "  DistributedGateway: Raft config sync, consistent-hash ring, session affinity, failover, quorum handling")
else()
    message(STATUS "Distributed Gateway focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_distributed_gateway.cpp")
endif()

# DatabaseMaintenanceOrchestrator – focused tests for CRUD, jobs, and health
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_database_maintenance_orchestrator.cpp")
    message(STATUS "Adding Database Maintenance Orchestrator focused tests")

    add_executable(test_database_maintenance_orchestrator_focused
        test_database_maintenance_orchestrator.cpp
        ${THEMIS_ROOT_DIR}/src/maintenance/database_maintenance_orchestrator.cpp
        ${THEMIS_ROOT_DIR}/src/maintenance/maintenance_schedule_store.cpp
        ${THEMIS_ROOT_DIR}/src/maintenance/maintenance_registry.cpp
        ${THEMIS_ROOT_DIR}/src/server/maintenance_api_handler.cpp
    )

    target_include_directories(test_database_maintenance_orchestrator_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_database_maintenance_orchestrator_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_database_maintenance_orchestrator_focused PRIVATE
        THEMIS_TEST_BUILD=1
        THEMIS_ENABLE_HTTP_SERVER=1
    )

    add_test(NAME DatabaseMaintenanceOrchestratorFocusedTests
             COMMAND test_database_maintenance_orchestrator_focused)

    set_tests_properties(DatabaseMaintenanceOrchestratorFocusedTests PROPERTIES
        LABELS "maintenance;orchestrator;schedule;crud;health;window;metrics;focused;unit"
        TIMEOUT 120
    )

    message(STATUS "  DatabaseMaintenanceOrchestrator: schedule CRUD, job management, health reporting")
else()
    message(STATUS "Database Maintenance Orchestrator focused tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_database_maintenance_orchestrator.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_query_engine.cpp")
    message(STATUS "Adding Query: QueryEngine focused tests")
    add_executable(test_query_engine_focused test_query_engine.cpp)
    target_include_directories(test_query_engine_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_query_engine_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_query_engine_focused PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME QueryEngineFocusedTests COMMAND test_query_engine_focused)
    set_tests_properties(QueryEngineFocusedTests PROPERTIES LABELS "query;engine;aql;focused;unit" TIMEOUT 90)
    message(STATUS "  QueryEngine: AQL execution, optimizer, multi-model queries")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_query_plan_visualizer.cpp")
    message(STATUS "Adding Query: QueryPlanVisualizer focused tests")
    add_executable(test_query_plan_visualizer_focused test_query_plan_visualizer.cpp)
    target_include_directories(test_query_plan_visualizer_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_query_plan_visualizer_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_query_plan_visualizer_focused PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME QueryPlanVisualizerFocusedTests COMMAND test_query_plan_visualizer_focused)
    set_tests_properties(QueryPlanVisualizerFocusedTests PROPERTIES LABELS "query;visualizer;explain;focused;unit" TIMEOUT 60)
    message(STATUS "  QueryPlanVisualizer: EXPLAIN, EXPLAIN ANALYZE, JSON/text output")
endif()

# ============================================================================
# Storage Module Focused Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_storage_audit_logger.cpp")
    message(STATUS "Adding Storage: StorageAuditLogger focused tests")
    add_executable(test_storage_audit_logger_focused test_storage_audit_logger.cpp)
    target_include_directories(test_storage_audit_logger_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_storage_audit_logger_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_storage_audit_logger_focused PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME StorageAuditLoggerFocusedTests COMMAND test_storage_audit_logger_focused)
    set_tests_properties(StorageAuditLoggerFocusedTests PROPERTIES LABELS "storage;audit;focused;unit" TIMEOUT 60)
    message(STATUS "  StorageAuditLogger: write, read, rotate, tamper detection")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_storage_engine_di.cpp")
    message(STATUS "Adding Storage: StorageEngineDI focused tests")
    add_executable(test_storage_engine_di_focused test_storage_engine_di.cpp)
    target_include_directories(test_storage_engine_di_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_storage_engine_di_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_storage_engine_di_focused PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME StorageEngineDIFocusedTests COMMAND test_storage_engine_di_focused)
    set_tests_properties(StorageEngineDIFocusedTests PROPERTIES LABELS "storage;engine;di;focused;unit" TIMEOUT 60)
    message(STATUS "  StorageEngine DI: constructor injection, encryption, evaluator")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_storage_engine_prod.cpp")
    message(STATUS "Adding Storage: StorageEngineProd focused tests")
    add_executable(test_storage_engine_prod_focused test_storage_engine_prod.cpp)
    target_include_directories(test_storage_engine_prod_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_storage_engine_prod_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_storage_engine_prod_focused PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME StorageEngineProdFocusedTests COMMAND test_storage_engine_prod_focused)
    set_tests_properties(StorageEngineProdFocusedTests PROPERTIES LABELS "storage;engine;production;focused;unit" TIMEOUT 60)
    message(STATUS "  StorageEngine prod: PRODUCTION_MODE safety flag, encryption defaults")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_nvme_manager.cpp")
    message(STATUS "Adding Storage: NVMeFocusedTests")
    add_executable(test_nvme_manager_focused test_nvme_manager.cpp)
    target_include_directories(test_nvme_manager_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_nvme_manager_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        RocksDB::rocksdb
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(test_nvme_manager_focused PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME NVMeFocusedTests COMMAND test_nvme_manager_focused)
    set_tests_properties(NVMeFocusedTests PROPERTIES
        LABELS "storage;nvme;io_uring;zns;direct-io;focused;unit"
        TIMEOUT 60
    )
    # POSIX-specific tests (::open/::close/pread/pwrite) are guarded in the
    # source with #ifndef _WIN32 and will SKIP gracefully on Windows.
    message(STATUS "  NVMe: capability detection, io_uring lifecycle, ZNS no-ops, Direct I/O flags, RocksDB integration")
endif()

# ============================================================================
# Network Module Focused Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_network_timeout.cpp" AND NOT TARGET test_network_timeout_focused)
    message(STATUS "Adding Network: NetworkTimeout focused tests")
    add_executable(test_network_timeout_focused test_network_timeout.cpp)
    target_include_directories(test_network_timeout_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_network_timeout_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_network_timeout_focused PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME NetworkTimeoutFocusedTests COMMAND test_network_timeout_focused)
    set_tests_properties(NetworkTimeoutFocusedTests PROPERTIES LABELS "network;timeout;circuit-breaker;focused;unit" TIMEOUT 60)
    message(STATUS "  NetworkTimeout: socket timeout manager, circuit breaker")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_wire_protocol_v2.cpp" AND NOT TARGET test_wire_protocol_v2_focused)
    message(STATUS "Adding Network: WireProtocolV2 focused tests")
    add_executable(test_wire_protocol_v2_focused test_wire_protocol_v2.cpp)
    target_include_directories(test_wire_protocol_v2_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_wire_protocol_v2_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_wire_protocol_v2_focused PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME WireProtocolV2FocusedTests COMMAND test_wire_protocol_v2_focused)
    set_tests_properties(WireProtocolV2FocusedTests PROPERTIES LABELS "network;wire-protocol;v2;multiplexing;focused;unit" TIMEOUT 60)
    message(STATUS "  WireProtocolV2: multiplexed streams, flow control, GOAWAY, push-promise")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_wire_protocol_websocket.cpp")
    message(STATUS "Adding Network: WireProtocolWebSocket focused tests")
    add_executable(test_wire_protocol_websocket_focused test_wire_protocol_websocket.cpp)
    target_include_directories(test_wire_protocol_websocket_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_wire_protocol_websocket_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_wire_protocol_websocket_focused PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME WireProtocolWebSocketFocusedTests COMMAND test_wire_protocol_websocket_focused)
    set_tests_properties(WireProtocolWebSocketFocusedTests PROPERTIES LABELS "network;websocket;upgrade;focused;unit" TIMEOUT 60)
    message(STATUS "  WireProtocolWebSocket: HTTP upgrade, frame handling, connection accounting")
endif()

# ============================================================================
# Utils Module Focused Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_bloom_filter.cpp")
    message(STATUS "Adding Utils: BloomFilter focused tests")
    add_executable(test_bloom_filter_focused test_bloom_filter.cpp)
    target_include_directories(test_bloom_filter_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_bloom_filter_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_bloom_filter_focused PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME BloomFilterFocusedTests COMMAND test_bloom_filter_focused)
    set_tests_properties(BloomFilterFocusedTests PROPERTIES LABELS "utils;bloom-filter;focused;unit" TIMEOUT 60)
    message(STATUS "  BloomFilter: insert, lookup, false-positive rate, merge")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_pii_stream_scanner.cpp" AND NOT TARGET test_pii_stream_scanner_focused)
    message(STATUS "Adding Utils: PIIStreamScanner focused tests")
    add_executable(test_pii_stream_scanner_focused test_pii_stream_scanner.cpp)
    target_include_directories(test_pii_stream_scanner_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_pii_stream_scanner_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_pii_stream_scanner_focused PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME PIIStreamScannerFocusedTests COMMAND test_pii_stream_scanner_focused)
    set_tests_properties(PIIStreamScannerFocusedTests PROPERTIES LABELS "utils;pii;scanner;privacy;focused;unit" TIMEOUT 60)
    message(STATUS "  PIIStreamScanner: streaming redaction, pattern matching, callback")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_sampled_logger.cpp" AND NOT TARGET test_sampled_logger_focused)
    message(STATUS "Adding Utils: SampledLogger focused tests")
    add_executable(test_sampled_logger_focused test_sampled_logger.cpp)
    target_include_directories(test_sampled_logger_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_sampled_logger_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_sampled_logger_focused PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME SampledLoggerFocusedTests COMMAND test_sampled_logger_focused)
    set_tests_properties(SampledLoggerFocusedTests PROPERTIES LABELS "utils;logging;sampling;focused;unit" TIMEOUT 60)
    message(STATUS "  SampledLogger: rate-limited log emission, token-bucket sampling")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_timestamp_utils.cpp" AND NOT TARGET test_timestamp_utils_focused)
    message(STATUS "Adding Utils: TimestampUtils focused tests")
    add_executable(test_timestamp_utils_focused test_timestamp_utils.cpp)
    target_include_directories(test_timestamp_utils_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_timestamp_utils_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_timestamp_utils_focused PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME TimestampUtilsFocusedTests COMMAND test_timestamp_utils_focused)
    set_tests_properties(TimestampUtilsFocusedTests PROPERTIES LABELS "utils;timestamp;hlc;focused;unit" TIMEOUT 60)
    message(STATUS "  TimestampUtils: parse, format, HLC ordering, epoch conversion")
endif()

# ============================================================================
# Server Module Focused Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_cdn_cache_middleware.cpp")
    message(STATUS "Adding Server: CDNCacheMiddleware focused tests")
    add_executable(test_cdn_cache_middleware_focused test_cdn_cache_middleware.cpp)
    target_include_directories(test_cdn_cache_middleware_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_cdn_cache_middleware_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_cdn_cache_middleware_focused PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME CDNCacheMiddlewareFocusedTests COMMAND test_cdn_cache_middleware_focused)
    set_tests_properties(CDNCacheMiddlewareFocusedTests PROPERTIES LABELS "server;cdn;cache;middleware;focused;unit" TIMEOUT 60)
    message(STATUS "  CDNCacheMiddleware: Cache-Control headers, ETag, surrogate keys")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_import_wizard.cpp")
    message(STATUS "Adding Server: ImportWizardBuilder focused tests")
    add_executable(test_import_wizard_builder_focused test_import_wizard.cpp)
    target_include_directories(test_import_wizard_builder_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_import_wizard_builder_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_import_wizard_builder_focused PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME ImportWizardBuilderFocusedTests COMMAND test_import_wizard_builder_focused)
    set_tests_properties(ImportWizardBuilderFocusedTests PROPERTIES LABELS "server;import;wizard;focused;unit" TIMEOUT 60)
    message(STATUS "  ImportWizardBuilder: multi-format import builder, schema inference")
endif()

# ============================================================================
# Sharding Module Focused Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_cloud_backup.cpp")
    message(STATUS "Adding Sharding: CloudBackup focused tests")
    add_executable(test_cloud_backup_focused test_cloud_backup.cpp)
    target_include_directories(test_cloud_backup_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_cloud_backup_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_cloud_backup_focused PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME CloudBackupFocusedTests COMMAND test_cloud_backup_focused)
    set_tests_properties(CloudBackupFocusedTests PROPERTIES LABELS "sharding;backup;cloud;focused;unit" TIMEOUT 60)
    message(STATUS "  CloudBackup: multi-cloud backup, incremental, restore verification")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_two_phase_commit.cpp")
    message(STATUS "Adding Sharding: TwoPhaseCommit focused tests")
    add_executable(test_two_phase_commit_focused test_two_phase_commit.cpp)
    target_include_directories(test_two_phase_commit_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_two_phase_commit_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_two_phase_commit_focused PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME TwoPhaseCommitFocusedTests COMMAND test_two_phase_commit_focused)
    set_tests_properties(TwoPhaseCommitFocusedTests PROPERTIES LABELS "sharding;2pc;distributed;transaction;focused;unit" TIMEOUT 90)
    message(STATUS "  TwoPhaseCommit: prepare/commit/abort, coordinator crash recovery")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_sharding_core.cpp")
    message(STATUS "Adding Sharding: ShardingCore focused tests")
    add_executable(test_sharding_core_focused test_sharding_core.cpp)
    target_include_directories(test_sharding_core_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_sharding_core_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_sharding_core_focused PRIVATE THEMIS_TEST_BUILD=1)
    add_test(NAME ShardingCoreFocusedTests COMMAND test_sharding_core_focused)
    set_tests_properties(ShardingCoreFocusedTests PROPERTIES LABELS "sharding;core;raft;consensus;focused;unit" TIMEOUT 90)
    message(STATUS "  ShardingCore: Raft consensus, shard routing, rebalancing")
endif()

# ── RS Repair Engine Parallelisation focused tests (v1.6.0 Issue #204) ──────
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_sharding_repair.cpp")
    message(STATUS "Adding Sharding: RS Repair Engine Parallelisation focused tests")

    add_executable(test_rs_repair_parallelisation_focused
        test_sharding_repair.cpp
    )

    target_include_directories(test_rs_repair_parallelisation_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_rs_repair_parallelisation_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_rs_repair_parallelisation_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME RSRepairParallelisationFocusedTests
             COMMAND test_rs_repair_parallelisation_focused)

    set_tests_properties(RSRepairParallelisationFocusedTests PROPERTIES
        LABELS "sharding;repair;reed-solomon;parallelisation;slo;iops;gpu;focused;unit"
        TIMEOUT 120
    )

    message(STATUS "  RSRepairParallelisation: parallel scan bands, SLO progress, IOPS throttle, GPU flag")
endif()

# Process Modeling Module focused tests
# Covers: BpmnSerializer, EpkSerializer, VccVpbImporter, LlmProcessDescriptor,
#         ProcessModelManager, ProcessLinker, ProcessGraphRag
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_process_module.cpp")
    message(STATUS "Adding Process Module focused tests")
    add_executable(test_process_module_focused
        test_process_module.cpp
        ${CMAKE_SOURCE_DIR}/src/process/process_model_manager.cpp
        ${CMAKE_SOURCE_DIR}/src/process/bpmn_serializer.cpp
        ${CMAKE_SOURCE_DIR}/src/process/epk_serializer.cpp
        ${CMAKE_SOURCE_DIR}/src/process/llm_process_descriptor.cpp
        ${CMAKE_SOURCE_DIR}/src/process/vcc_vpb_importer.cpp
        ${CMAKE_SOURCE_DIR}/src/process/process_linker.cpp
        ${CMAKE_SOURCE_DIR}/src/process/process_graph_rag.cpp
        ${CMAKE_SOURCE_DIR}/src/index/process_graph.cpp
        ${CMAKE_SOURCE_DIR}/src/index/edge_types.cpp
    )
    target_include_directories(test_process_module_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    target_link_libraries(test_process_module_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(test_process_module_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )
    add_test(NAME ProcessModuleFocusedTests COMMAND test_process_module_focused)
    set_tests_properties(ProcessModuleFocusedTests PROPERTIES
        LABELS "process;bpmn;epk;vcc-vpb;graph-rag;linker;focused;unit"
        TIMEOUT 120
    )
    message(STATUS "  ProcessModule: BPMN/EPK/VCC-VPB import-export, ProcessModelManager CRUD,")
    message(STATUS "                 ProcessLinker attach/detach/required-docs, ProcessGraphRag")
endif()

# ============================================================================
# Temporal Module Focused Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/temporal/test_temporal_conflict_resolver.cpp")
    message(STATUS "Adding Temporal: TemporalConflictResolver + TemporalConflictDetector focused tests")

    add_executable(test_temporal_conflict_resolver_focused
        temporal/test_temporal_conflict_resolver.cpp
        ${CMAKE_SOURCE_DIR}/src/temporal/temporal_conflict_resolver.cpp
        ${CMAKE_SOURCE_DIR}/src/replication/replication_manager.cpp
    )

    target_include_directories(test_temporal_conflict_resolver_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_temporal_conflict_resolver_focused PRIVATE
        ${TEST_LIBS}
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    if(TARGET themis_core)
        target_link_libraries(test_temporal_conflict_resolver_focused PRIVATE themis_core)
    endif()

    target_compile_definitions(test_temporal_conflict_resolver_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME TemporalConflictResolverFocusedTests COMMAND test_temporal_conflict_resolver_focused)

    set_tests_properties(TemporalConflictResolverFocusedTests PROPERTIES
        LABELS "temporal;conflict;resolver;detector;mvcc;unit"
        TIMEOUT 60
    )

    message(STATUS "  TemporalConflictResolver: LWW, FWW, NODE_PRIORITY, MANUAL, CRDT_MERGE policies, audit log")
    message(STATUS "  TemporalConflictDetector: CONCURRENT_UPDATE, OVERLAPPING_PERIODS, REFERENTIAL_INTEGRITY, UNIQUENESS_VIOLATION")
else()
    message(STATUS "Temporal conflict resolver tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/temporal/test_temporal_conflict_resolver.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/temporal/test_retention_manager.cpp")
    message(STATUS "Adding Temporal: RetentionManager focused tests (time-based, storage-based, compliance, incremental, retry)")

    add_executable(test_temporal_retention_manager_focused
        temporal/test_retention_manager.cpp
        ${CMAKE_SOURCE_DIR}/src/temporal/retention_manager.cpp
        ${CMAKE_SOURCE_DIR}/src/temporal/system_versioned_table.cpp
    )

    target_include_directories(test_temporal_retention_manager_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_temporal_retention_manager_focused PRIVATE
        ${TEST_LIBS}
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_temporal_retention_manager_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME TemporalRetentionManagerFocusedTests COMMAND test_temporal_retention_manager_focused)

    set_tests_properties(TemporalRetentionManagerFocusedTests PROPERTIES
        LABELS "temporal;retention;time-based;storage-based;compliance;incremental;retry;unit"
        TIMEOUT 60
    )

    message(STATUS "  RetentionManager: TIME_BASED, VERSION_COUNT_BASED, STORAGE_BASED, CUSTOM policies, archiving, compliance, incremental batch, retry")
else()
    message(STATUS "Temporal RetentionManager tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/temporal/test_retention_manager.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/temporal/test_bi_temporal.cpp")
    message(STATUS "Adding Temporal: BiTemporalTable focused tests (bi-temporal, gap detection, FK)")

    add_executable(test_bi_temporal_focused
        temporal/test_bi_temporal.cpp
        ${CMAKE_SOURCE_DIR}/src/temporal/bi_temporal.cpp
    )

    target_include_directories(test_bi_temporal_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_bi_temporal_focused PRIVATE
        ${TEST_LIBS}
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_bi_temporal_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME BiTemporalFocusedTests COMMAND test_bi_temporal_focused)

    set_tests_properties(BiTemporalFocusedTests PROPERTIES
        LABELS "temporal;bitemporal;gap;uniqueness;foreign-key;unit"
        TIMEOUT 60
    )

    message(STATUS "  BiTemporalTable: insert/update/delete, bi-temporal queries, gap detection, uniqueness constraints, temporal foreign keys")
else()
    message(STATUS "BiTemporalTable tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/temporal/test_bi_temporal.cpp")
endif()

# ============================================================================
# Core Module – Distributed Cache Integration Focused Tests (Issue #64)
# Validates all seven acceptance criteria:
#   AC-1  Cluster-wide cache invalidation
#   AC-2  Consistent hashing for distributed keys
#   AC-3  TTL support
#   AC-4  Pub/sub for cache invalidation messages
#   AC-5  Query result caching across nodes
#   AC-6  Session state management
#   AC-7  Distributed rate limiting state
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_distributed_cache_integration.cpp")
    message(STATUS "Adding Distributed Cache Integration focused tests (core module, Issue #64)")

    add_executable(test_distributed_cache_integration_focused
        test_distributed_cache_integration.cpp
    )

    target_include_directories(test_distributed_cache_integration_focused PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_distributed_cache_integration_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_distributed_cache_integration_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME DistributedCacheIntegrationFocusedTests
        COMMAND test_distributed_cache_integration_focused)

    set_tests_properties(DistributedCacheIntegrationFocusedTests PROPERTIES
        LABELS "core;cache;redis;distributed;consistent-hashing;pubsub;ttl;v1.6.0;unit"
        TIMEOUT 60
    )

    message(STATUS "  RedisCache: URL parsing, consistent hashing, TTL, pub/sub invalidation, graceful degradation, statistics, ConcernsContext integration")
else()
    message(STATUS "Distributed Cache Integration tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_distributed_cache_integration.cpp")
endif()

# ============================================================================
# Custom Test Targets
# ============================================================================
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

if(TARGET themis_tests)
    get_target_property(_themis_tests_sources_final themis_tests SOURCES)
    if(_themis_tests_sources_final)
        list(FILTER _themis_tests_sources_final EXCLUDE REGEX ".*[\\/]src[\\/]importers[\\/]postgres_importer\\.cpp$")
        list(FILTER _themis_tests_sources_final EXCLUDE REGEX ".*[\\/]src[\\/]importers[\\/]postgres_importer_mdm\\.cpp$")
        set_property(TARGET themis_tests PROPERTY SOURCES ${_themis_tests_sources_final})
    endif()
endif()

message(STATUS "Tests configured successfully")
