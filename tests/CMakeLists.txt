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
        ".*test_claim_extractor\\.cpp$"
        ".*test_adapter_sync\\.cpp$"
        ".*test_adapter_factory\\.cpp$"
        ".*test_cross_functional_voice_observability\\.cpp$"
        ".*test_process_mining_e2e\\.cpp$"
        ".*test_extended_context\\.cpp$"
        ".*test_model_loader_error_handling\\.cpp$"
        ".*test_model_loading_from_themisdb\\.cpp$"
        ".*test_real_embeddings\\.cpp$"
        ".*test_adaptive_query_cache\\.cpp$"
        ".*test_streaming_handler\\.cpp$"
        ".*test_shared_worker_pool\\.cpp$"
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
    ".*test_auth_error\\.cpp$"
    ".*test_auth_metrics\\.cpp$"
    ".*test_auth_rate_limiter\\.cpp$"
    ".*test_jwks_security\\.cpp$"
    ".*test_kerberos_security\\.cpp$"
    ".*test_principal_validator\\.cpp$"
    ".*test_totp_replay_cache\\.cpp$"
    ".*test_totp_secret_encryption\\.cpp$"
    ".*test_mfa_authenticator.*\\.cpp$"
    ".*test_binary_integrity.*\\.cpp$"
    ".*test_adapter_factory\\.cpp$"
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
    ".*test_replication_ha\\.cpp$"
    ".*test_real_embeddings\\.cpp$"
    ".*test_adapter_sync\\.cpp$"
    ".*test_fewshot_optimizer\\.cpp$"
    ".*test_flash_attention_correctness\\.cpp$"
    ".*test_geo_topology_api_handler\\.cpp$"
    ".*test_gpu_admin_api\\.cpp$"
    ".*test_gpu_.*\\.cpp$"
    ".*test_graphql.*\\.cpp$"
    ".*test_jwt_key_rotation_comprehensive\\.cpp$"
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
)
foreach(pattern IN LISTS TEST_EXCLUDE_PATTERNS)
    list(FILTER ALL_TEST_SOURCES EXCLUDE REGEX "${pattern}")
endforeach()

list(LENGTH ALL_TEST_SOURCES TEST_SOURCE_COUNT)
message(STATUS "Building ${TEST_SOURCE_COUNT} test files after feature gating")

add_executable(themis_tests ${ALL_TEST_SOURCES})

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

# Re-include LlmQueryRewriter tests when LLM is disabled.
# The ".*test_llm.*\.cpp$" exclusion pattern above would exclude
# test_llm_query_rewriter.cpp in MINIMAL builds, but that test has zero actual
# LLM library dependency — it only uses a std::function mock backend.
# Add it back unconditionally so it runs in every build variant.
if(NOT THEMIS_ENABLE_LLM)
    target_sources(themis_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/test_llm_query_rewriter.cpp
    )
endif()

# Ensure legal training tests link even when the feature is disabled in themis_core
if(NOT THEMIS_ENABLE_LEGAL_TRAINING)
    target_sources(themis_tests PRIVATE
        ../src/ingestion/ingestion_manager.cpp
        ../src/ingestion/huggingface_connector.cpp
        ../src/ingestion/filesystem_ingester.cpp
        ../src/ingestion/api_connector.cpp
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
            ${THEMIS_ROOT_DIR}/src/timeseries/ts_auto_buffer.cpp
            ${THEMIS_ROOT_DIR}/src/index/vector_auto_buffer.cpp
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
endif()

if(THEMIS_ENABLE_GRPC)
    target_link_libraries(themis_tests PRIVATE gRPC::grpc++)
endif()

if(TARGET llama)
    target_link_libraries(themis_tests PRIVATE llama)
endif()

target_compile_definitions(themis_tests PRIVATE
    THEMIS_TEST_BUILD=1
)

target_include_directories(themis_tests PRIVATE
    ${CMAKE_BINARY_DIR}/proto_generated
    ${THEMIS_ROOT_DIR}/vcpkg_installed/x64-windows/include
)

if(TARGET themis_shard_proto)
    add_dependencies(themis_tests themis_shard_proto)
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
# Enhanced Query Cache Tests
# ============================================================================

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

# RPC Geospatial Query Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_rpc_geo_query.cpp")
    message(STATUS "Adding RPC Geospatial Query tests")
    
    add_executable(test_rpc_geo_query
        test_rpc_geo_query.cpp
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
# Inverted Index (standalone full-text search module) Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_inverted_index.cpp")
    message(STATUS "Adding InvertedIndex unit tests")

    add_executable(test_inverted_index
        test_inverted_index.cpp
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
    add_executable(test_logger_production test_logger_production.cpp)
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
    add_executable(test_self_awareness_production test_self_awareness_production.cpp)
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

# Cloud Storage Backup Comprehensive Tests (GAP-008)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_cloud_storage_backup_comprehensive.cpp")
    message(STATUS "Adding Cloud Storage Backup comprehensive tests (GAP-008)")
    
    add_executable(test_cloud_storage_backup_comprehensive
        test_cloud_storage_backup_comprehensive.cpp
    )
    
    target_link_libraries(test_cloud_storage_backup_comprehensive PRIVATE
        GTest::gtest
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
