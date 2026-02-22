# Modular Build Configuration for ThemisDB
# This feature is planned for post-v1.3.0 release
# See docs/architecture/MODULARIZATION_PLAN.md for details

cmake_minimum_required(VERSION 3.20)

# Build mode option - defaults to legacy monolithic build until v1.4.0 is released
option(THEMIS_BUILD_MODULAR "Build as modular libraries instead of monolithic core (v1.4.0+ feature)" OFF)

# Version check - only allow modular build after v1.4.0
if(THEMIS_BUILD_MODULAR)
    if(PROJECT_VERSION VERSION_LESS "1.4.0")
        message(WARNING 
            "THEMIS_BUILD_MODULAR requires v1.4.0 or later. Current version: ${PROJECT_VERSION}\n"
            "Modular build is disabled. See docs/architecture/MODULARIZATION_PLAN.md for details.\n"
            "Falling back to monolithic build.")
        set(THEMIS_BUILD_MODULAR OFF CACHE BOOL "Modular build disabled - version too old" FORCE)
    else()
        message(STATUS "Modular build enabled (v${PROJECT_VERSION} >= 1.4.0)")
    endif()
endif()

# Optional module configuration (only relevant when THEMIS_BUILD_MODULAR=ON)
if(THEMIS_BUILD_MODULAR)
    option(THEMIS_MODULE_TRANSACTION "Include transaction module (required)" ON)
    option(THEMIS_MODULE_LLM "Include LLM inference module (optional)" ON)
    option(THEMIS_MODULE_GEO "Include geospatial module (optional)" ON)
    option(THEMIS_MODULE_GRAPH "Include graph analytics module (optional)" ON)
    option(THEMIS_MODULE_CONTENT "Include content processors module (optional)" OFF)
    option(THEMIS_MODULE_TIMESERIES "Include time-series module" ON)
    option(THEMIS_MODULE_SHARDING "Include distributed sharding module" ON)
endif()

# Helper function to create a modular library target
# Usage: themis_add_module(module_name SOURCES file1.cpp file2.cpp ... DEPENDENCIES dep1 dep2 ...)
function(themis_add_module MODULE_NAME)
    cmake_parse_arguments(ARG "" "" "SOURCES;DEPENDENCIES" ${ARGN})
    
    # Create the library (SHARED for modular build)
    add_library(themis_${MODULE_NAME} SHARED ${ARG_SOURCES})
    
    # Set export macro
    string(TOUPPER ${MODULE_NAME} MODULE_NAME_UPPER)
    target_compile_definitions(themis_${MODULE_NAME} 
        PRIVATE THEMIS_${MODULE_NAME_UPPER}_EXPORTS
        PUBLIC THEMIS_${MODULE_NAME_UPPER}_ENABLED
    )
    
    # Include directories
    target_include_directories(themis_${MODULE_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src
    )
    
    # C++20 standard
    target_compile_features(themis_${MODULE_NAME} PUBLIC cxx_std_20)
    
    # Link dependencies
    if(ARG_DEPENDENCIES)
        target_link_libraries(themis_${MODULE_NAME} PUBLIC ${ARG_DEPENDENCIES})
    endif()
    
    # Define module-specific export macro
    string(TOUPPER ${MODULE_NAME} MODULE_NAME_UPPER)
    target_compile_definitions(themis_${MODULE_NAME} PRIVATE THEMIS_${MODULE_NAME_UPPER}_EXPORTS)
    
    # mimalloc import definitions for all modules
    if(THEMIS_ENABLE_MIMALLOC AND TARGET mimalloc)
        target_compile_definitions(themis_${MODULE_NAME} PRIVATE MI_SHARED_LIB=1 MI_SHARED_LIB_EXPORT=0)
    endif()
    
    # Windows: Export all symbols for DLL and disable /GL so __create_def can read symbols
    if(MSVC)
        set_target_properties(themis_${MODULE_NAME} PROPERTIES
            WINDOWS_EXPORT_ALL_SYMBOLS ON
            INTERPROCEDURAL_OPTIMIZATION FALSE
            VS_GLOBAL_WholeProgramOptimization "false"
        )
        target_compile_options(themis_${MODULE_NAME} PRIVATE
            $<$<CONFIG:Release>:/GL->
            $<$<CONFIG:RelWithDebInfo>:/GL->
            $<$<CONFIG:MinSizeRel>:/GL->
        )
        target_link_options(themis_${MODULE_NAME} PRIVATE
            $<$<CONFIG:Release>:/LTCG:OFF>
            $<$<CONFIG:RelWithDebInfo>:/LTCG:OFF>
            $<$<CONFIG:MinSizeRel>:/LTCG:OFF>
        )
    endif()
    
    # Set output directories
    set_target_properties(themis_${MODULE_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    )
    
    # Installation
    install(TARGETS themis_${MODULE_NAME}
        EXPORT ThemisTargets
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
    
    message(STATUS "Module configured: themis_${MODULE_NAME}")
endfunction()

# Module source file lists (to be populated during modularization)
# These will be extracted from the current THEMIS_CORE_SOURCES

set(THEMIS_BASE_SOURCES
    # Core utilities and cross-cutting concerns
    ../src/utils/serialization.cpp
    ../src/utils/logger.cpp
    ../src/utils/cursor.cpp
    ../src/utils/tracing.cpp
    ../src/utils/zstd_codec.cpp
    ../src/utils/input_validator.cpp
    ../src/utils/hkdf_helper.cpp
    ../src/utils/hkdf_cache.cpp
    ../src/utils/stemmer.cpp
    ../src/utils/stopwords.cpp
    ../src/utils/normalizer.cpp
    ../src/utils/simd_distance.cpp
    ../src/utils/update_checker.cpp
    ../src/utils/http_client_pool.cpp
    ../src/utils/grpc_channel_pool.cpp
    ../src/observability/metrics_collector.cpp
    ../src/config/config_path_resolver.cpp
    ../src/utils/build_info.cpp
    ../src/utils/license_info.cpp
    ../src/utils/runtime_license_gate.cpp
    ../src/utils/error_registry.cpp
    ../src/utils/memory/pool_allocator.cpp
    ../src/utils/boost_throw_exception.cpp
    ../src/utils/file_utils.cpp
    ../src/utils/thread_pool_manager.cpp
    
    # Cross-cutting concerns abstraction layer
    ../src/core/concerns/i_logger.cpp
    ../src/core/concerns/concerns_context.cpp
    ../src/sharding/circuit_breaker.cpp
    
    # Hardware acceleration (core abstraction layer)
    ../src/acceleration/backend_registry.cpp
    ../src/acceleration/cpu_backend.cpp
    ../src/acceleration/plugin_loader.cpp
    ../src/acceleration/plugin_security.cpp
    
    # Plugin manager (core plugin orchestration)
    ../src/plugins/plugin_manager.cpp
    ../src/plugins/plugin_hot_plug_monitor.cpp
    ../src/plugins/plugin_registry.cpp
    ../src/plugins/plugin_metrics.cpp
    
    # Module loader (for security verification of modular DLLs)
    ../src/base/module_loader.cpp
    ../src/base/module_sandbox.cpp
    ../src/base/hot_reload_manager.cpp
    
    # Stubs for missing symbols
    ../src/stubs.cpp
)

set(THEMIS_STORAGE_SOURCES
    # Core storage engine
    ../src/storage/rocksdb_wrapper.cpp
    ../src/storage/base_entity.cpp
    ../src/storage/key_schema.cpp
    ../src/storage/backup_manager.cpp
    ../src/storage/columnar_format.cpp
    # ../src/storage/pitr_manager.cpp  # Temporarily disabled - needs transaction module
    ../src/storage/blob_redundancy_manager.cpp
    # WAL for durability and crash recovery
    ../src/storage/wal_storage.cpp
    # Compaction and GC management
    ../src/storage/compaction_manager.cpp
    # Storage Audit Logger
    ../src/storage/storage_audit_logger.cpp
    # MVCC versioning and HLC timestamping
    ../src/storage/hlc.cpp
    ../src/storage/mvcc_store.cpp
    ../src/storage/raft_mvcc_bridge.cpp
    ../src/sharding/distributed_time_coordinator.cpp
    
    # Metadata management
    ../src/metadata/schema_manager.cpp
    ../src/metadata/statistics_collector.cpp
    ../src/metadata/information_schema.cpp
    ../src/metadata/schema_constraints.cpp
    ../src/metadata/schema_version_manager.cpp
    ../src/metadata/index_recommender.cpp
    ../src/metadata/schema_audit_log.cpp
    ../src/metadata/schema_consistency_checker.cpp
    
    # Indexes
    ../src/index/secondary_index.cpp
    ../src/index/rotary_embeddings.cpp
    ../src/index/learnable_rope.cpp
    ../src/index/hnsw_layer_optimizer.cpp
    ../src/index/hnsw_parameter_tuner.cpp
    ../src/index/hnsw_production_defaults.cpp
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/index/gpu_vector_index.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/index/multi_gpu_vector_index.cpp>
    $<$<BOOL:${THEMIS_ENABLE_VULKAN}>:../src/index/gpu_vector_index_vulkan.cpp>
    ../src/index/advanced_vector_index.cpp
    ../src/index/product_quantizer.cpp
    ../src/index/adaptive_index.cpp
    ../src/index/spatial_index.cpp
    ../src/api/geo_index_hooks.cpp
    ../src/utils/geo/ewkb.cpp
    
    # Performance enhancements
    ../src/performance/phase2_feature_flags.cpp
    ../src/performance/phase3/feature_flags.cpp
    
    # Storage enhancements
    ../src/cache/semantic_cache.cpp
    
    # Updates
    ../src/updates/release_manifest.cpp
    ../src/updates/manifest_database.cpp
    ../src/updates/hot_reload_engine.cpp
    ../src/updates/updates_config.cpp
    ../src/updates/update_state_machine.cpp

    # Storage security
    ../src/storage/security_signature.cpp
    ../src/storage/security_signature_manager.cpp
)

set(THEMIS_QUERY_SOURCES
    # Query engine
    ../src/query/query_engine.cpp
    ../src/query/query_optimizer.cpp
    ../src/query/adaptive_optimizer.cpp
    ../src/query/optimizer_cost_model.cpp
    ../src/query/aql_parser.cpp
    ../src/query/aql_parser_json.cpp
    ../src/query/aql_translator.cpp
    ../src/query/aql_runner.cpp
    ../src/query/let_evaluator.cpp
    ../src/query/window_evaluator.cpp
    ../src/query/cte_subquery.cpp
    ../src/query/cte_cache.cpp
    ../src/query/result_stream.cpp
    ../src/query/query_cache.cpp
    ../src/query/workload_cache_strategy.cpp
    ../src/query/query_cache_manager.cpp
    ../src/cache/adaptive_query_cache.cpp
    ../src/cache/warmup.cpp
    ../src/query/statistical_aggregator.cpp
    ../src/query/semantic_cache.cpp
    ../src/query/functions/function_registry.cpp
    ../src/query/functions/ethics_functions.cpp
    ../src/query/functions/lora_functions.cpp
    ../src/query/functions/process_mining_functions.cpp
    
    # Analytics
    ../src/analytics/olap.cpp
    ../src/analytics/process_mining.cpp
    ../src/analytics/process_pattern_matcher.cpp
    ../src/analytics/nlp_text_analyzer.cpp
    ../src/analytics/diff_engine.cpp
    ../src/analytics/cep_engine.cpp
    ../src/analytics/streaming_window.cpp
    ../src/analytics/incremental_view.cpp
    ../src/analytics/anomaly_detection.cpp
    
    # AQL handlers
    ../src/aql/llm_aql_handler.cpp
    ../src/aql/aql_syntax_highlighter.cpp
    ../src/aql/aql_confidence_scorer.cpp
    ../src/aql/aql_query_builder.cpp
    ../src/aql/aql_query_validator.cpp
    ../src/aql/aql_query_template_library.cpp
    ../src/aql/aql_conversation_context.cpp
    
    # Security: AQL injection detection (uses AQLParser)
    ../src/security/aql_injection_detector.cpp
    
    # NLP features (moved from storage)
    ../src/storage/nlp_metadata_extractor.cpp
    
    # Import/Export
    ../src/exporters/jsonl_llm_exporter.cpp
    ../src/exporters/exporter_metrics.cpp
    ../src/exporters/pii_detector.cpp
    ../src/exporters/stream_writer.cpp
    ../src/importers/postgres_importer.cpp

    # AQL metrics support
    ../src/aql/llm_metrics_collector.cpp
)

set(THEMIS_SECURITY_SOURCES
    # Encryption and key management
    ../src/security/mock_key_provider.cpp
    ../src/security/vault_key_provider.cpp
    ../src/security/key_cache.cpp
    ../src/security/keyprovider_signing.cpp
    ../src/security/vault_signing_provider.cpp
    ../src/security/field_encryption.cpp
    ../src/security/encrypted_field.cpp
    ../src/security/malware_scanner.cpp
    ../src/security/usb_admin_authenticator.cpp
    ../src/security/pki_key_provider.cpp
    ../src/security/cms_signing.cpp
    ../src/security/rbac.cpp
    ../src/security/access_control_manager.cpp
    ../src/security/access_control.cpp
    ../src/security/user_registration_plugin.cpp
    ../src/security/arrow_user_registration_plugin.cpp
    ../src/security/webdav_user_registration_plugin.cpp
    ../src/security/embedded_user_registration_plugin.cpp
    ../src/security/hsm_provider.cpp
    ../src/security/hsm_provider_pkcs11.cpp
    ../src/security/hsm_key_provider_adapter.cpp
    ../src/security/timestamp_authority.cpp
    ../src/security/timestamp_authority_openssl.cpp
    ../src/security/vcc_pki_client.cpp
    ../src/security/pii_redaction_policy.cpp
    ../src/utils/audit_logger.cpp
    ../src/utils/lek_manager.cpp
    ../src/utils/saga_logger.cpp
    
    # Authentication
    ../src/auth/jwt_validator.cpp
    ../src/auth/token_blacklist.cpp
    ../src/auth/jwks_validator.cpp
    ../src/auth/gssapi_authenticator.cpp
    ../src/auth/mfa_authenticator.cpp
    ../src/server/auth_middleware.cpp
    ../src/server/request_validation_middleware.cpp
    
    # Governance
    ../src/governance/policy_engine.cpp
    
    # PII detection
    ../src/utils/pii_detection_engine.cpp
    ../src/utils/regex_detection_engine.cpp
    ../src/utils/pii_detector.cpp
    ../src/utils/retention_manager.cpp
    ../src/utils/pki_client.cpp
    
    # Security initialization
    ../src/core/security_initialization.cpp
    
    # Storage-backed PII and vector index (require both storage and security)
    ../src/utils/pii_pseudonymizer.cpp
    ../src/index/vector_index.cpp
    # ../src/cache/embedding_cache.cpp  # Temporarily disabled - requires mimalloc
    ../src/search/hybrid_search.cpp
    ../src/search/query_expander.cpp
    ../src/search/fuzzy_matcher.cpp
    ../src/search/faceted_search.cpp
    ../src/search/search_analytics.cpp
    ../src/search/autocomplete.cpp
    ../src/search/learning_to_rank.cpp
    ../src/search/multi_modal_search.cpp
)

set(THEMIS_TRANSACTION_SOURCES
    # Transaction management
    ../src/transaction/transaction_manager.cpp
    ../src/transaction/lock_manager.cpp
    ../src/transaction/crash_recovery_manager.cpp
    ../src/transaction/saga.cpp
    ../src/transaction/snapshot_manager.cpp
    
    # Temporal conflict resolution and production-readiness modules
    ../src/temporal/temporal_conflict_resolver.cpp
    ../src/temporal/system_versioned_table.cpp
    ../src/temporal/temporal_query_engine.cpp
    ../src/temporal/temporal_index.cpp
    ../src/temporal/retention_manager.cpp
    ../src/temporal/bi_temporal.cpp
    ../src/temporal/snapshot_manager.cpp
    ../src/temporal/temporal_aggregator.cpp
    
    # Replication
    ../src/replication/replication_manager.cpp
    
    # Change data capture
    ../src/cdc/changefeed.cpp
    
    # Graph index (used by transactions)
    ../src/index/graph_index.cpp
)

set(THEMIS_SHARDING_SOURCES
    # Sharding core
    ../src/sharding/urn.cpp
    ../src/sharding/consistent_hash.cpp
    ../src/sharding/shard_topology.cpp
    ../src/sharding/urn_resolver.cpp
    ../src/sharding/pki_shard_certificate.cpp
    ../src/sharding/mtls_client.cpp
    ../src/sharding/mtls_connection_pool.cpp
    ../src/sharding/signed_request.cpp
    ../src/sharding/remote_executor.cpp
    ../src/sharding/shard_rpc_client.cpp
    ../src/sharding/shard_router.cpp
    ../src/sharding/rebalance_operation.cpp
    ../src/sharding/data_migrator.cpp
    ../src/sharding/shard_load_detector.cpp
    ../src/sharding/auto_rebalancer.cpp
    ../src/sharding/prometheus_metrics.cpp
    ../src/sharding/metrics_registry.cpp
    ../src/sharding/health_check.cpp
    ../src/sharding/admin_api.cpp
    ../src/sharding/shard_repair_engine.cpp
    ../src/sharding/cloud_agent.cpp
    ../src/sharding/circuit_breaker.cpp
    ../src/sharding/gossip_protocol.cpp
    ../src/sharding/gossip_config_manager.cpp
    ../src/sharding/distributed_coordinator.cpp
    ../src/sharding/shard_resource_manager.cpp
    ../src/sharding/locality_aware_router.cpp
    ../src/sharding/adaptive_shard_router.cpp
    ../src/sharding/capability_matcher.cpp
    ../src/sharding/metadata_shard.cpp
    ../src/sharding/metadata_wal.cpp
    ../src/sharding/metadata_snapshot.cpp
    ../src/cache/bounded_lru_cache.cpp
    ../src/server/rpc/blob_transfer_handler.cpp
    
    # Raft consensus
    ../src/sharding/raft_configuration.cpp
    ../src/sharding/raft_log.cpp
    ../src/sharding/raft_state.cpp
    ../src/sharding/raft_wal_integration.cpp
    ../src/sharding/raft_consensus.cpp
    ../src/sharding/raft_shard_manager.cpp
    ../src/sharding/quorum_manager.cpp
    ../src/sharding/partition_detector.cpp
    ../src/sharding/replica_consistency.cpp
    
    # WAL and replication
    ../src/sharding/stream_protocol.cpp
    ../src/sharding/wal_applier.cpp
    ../src/sharding/wal_manager.cpp
    ../src/sharding/wal_shipper.cpp
    ../src/sharding/secure_transport_client.cpp
    ../src/sharding/replication_coordinator.cpp
    ../src/sharding/replica_topology.cpp
    ../src/sharding/multi_primary_coordinator.cpp
    ../src/sharding/health_monitor.cpp
    ../src/sharding/truetime.cpp
    ../src/sharding/distributed_time_coordinator.cpp
    
    # Distributed transactions
    ../src/sharding/distributed_transaction.cpp
    ../src/sharding/consensus_factory.cpp
    ../src/sharding/raft_consensus_adapter.cpp
    ../src/sharding/gossip_consensus_adapter.cpp
    ../src/sharding/paxos_consensus.cpp
    ../src/sharding/paxos_wal.cpp
    ../src/sharding/paxos_snapshot.cpp
    ../src/sharding/cross_shard_transaction.cpp
    ../src/sharding/transaction_wal.cpp
    ../src/sharding/transaction_snapshot.cpp
    
    # Redundancy and reliability
    ../src/sharding/redundancy_strategy.cpp
    ../src/sharding/hot_spare_manager.cpp
    ../src/sharding/predictive_detector.cpp
    ../src/sharding/shard_rpc_server.cpp
    
    # GPU erasure coding (conditional)
    $<$<BOOL:${THEMIS_ENABLE_CUDA}>:../src/sharding/gpu_erasure_coder.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CUDA}>:../src/sharding/gpu_erasure_coder.cu>
    $<$<BOOL:${THEMIS_ENABLE_OPENCL}>:../src/sharding/gpu_erasure_coder_opencl.cpp>
    
    # Enhanced sharding features
    ../src/sharding/shard_durability.cpp
    ../src/sharding/operational_metrics.cpp
    ../src/sharding/admin_operations.cpp
    ../src/sharding/slo_monitor.cpp
)

set(THEMIS_LLM_SOURCES
    # LLM core components
    ../src/llm/llm_interaction_store.cpp
    ../src/llm/llm_response_cache.cpp
    ../src/prompt_engineering/prompt_manager.cpp
    ../src/llm/block_table.cpp
    ../src/llm/paged_block_manager.cpp
    ../src/llm/paged_kv_cache.cpp
    ../src/llm/llm_plugin_manager.cpp
    ../src/llm/model_loader.cpp
    ../src/llm/llama_wrapper.cpp
    ../src/llm/llama_lora_adapter.cpp
    ../src/llm/llama_grammar_adapter.cpp
    ../src/llm/llamacpp_inference_engine.cpp
    ../src/llm/async_inference_engine.cpp
    ../src/llm/inference_engine_enhanced.cpp
    ../src/llm/embedded_llm.cpp
    ../src/llm/ethical_guidelines_manager.cpp
    ../src/llm/docs_assistant.cpp
    ../src/llm/feedback_store.cpp
    ../src/llm/llm_model_storage.cpp
    ../src/llm/kv_cache_buffer.cpp
    ../src/llm/multi_lora_manager.cpp
    ../src/llm/gguf_loader.cpp
    ../src/llm/grammar.cpp
    ../src/llm/grammar_cache.cpp
    ../src/llm/llm_prefix_cache.cpp
    ../src/llm/continuous_batch_scheduler.cpp
    ../src/llm/token_quota_manager.cpp
    ../src/llm/grafana_metrics.cpp
    ../src/llm/distributed_training_coordinator.cpp
        ../src/cache/embedding_cache.cpp
        ../src/llm/lora_framework/lora_layers.cpp
    
    # LoRA framework (core subset)
    ../src/llm/lora_framework/lora_orchestrator.cpp
    ../src/llm/lora_framework/lora_provenance.cpp
    ../src/llm/lora_framework/lora_storage_service.cpp
    ../src/llm/lora_framework/lora_training_service.cpp
    ../src/llm/lora_framework/adapter_consistency_checker.cpp
    ../src/llm/lora_framework/gradient_utils.cpp
    ../src/llm/lora_framework/mixed_precision.cpp
    ../src/llm/lora_framework/lr_scheduler.cpp
    ../src/llm/lora_framework/data_loader.cpp
    ../src/llm/lora_framework/distributed_trainer.cpp
    ../src/llm/lora_framework/quantized_model.cpp
    ../src/llm/lora_framework/gguf_converter.cpp
    ../src/llm/lora_framework/llama_tokenizer.cpp
    ../src/llm/lora_framework/base_model_adapter.cpp
    ../src/llm/lora_framework/quantization.cpp
    ../src/llm/lora_framework/model_compatibility.cpp
    ../src/llm/lora_framework/resource_profiler.cpp
    ../src/llm/lora_framework/training_service_registry.cpp
    ../src/llm/byzantine_detector.cpp
    ../src/security/vram_secure_clear.cpp
    # GPU memory management and security
    ../src/llm/gpu_memory_manager.cpp
    # GPU-specific sources (conditional)
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/llm/lora_framework/vram_allocator.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/llm/lora_framework/gpu_memory.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/llm/lora_framework/gpu_tensor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/llm/lora_framework/gpu_lora_layers.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/llm/lora_framework/flash_lora.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/llm/lora_framework/gpu_data_loader.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/llm/lora_framework/gpu_embedding_layer.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/llm/lora_framework/multi_gpu_lora_layer.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/llm/lora_framework/multi_gpu.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/llm/lora_framework/multi_gpu_trainer.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/llm/lora_framework/nccl_backend.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/llm/lora_framework/rccl_backend.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/llm/lora_framework/custom_allreduce.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/llm/lora_framework/adaptive_batcher.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/llm/lora_framework/gpu_utilization_monitor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/llm/lora_framework/gradient_checkpointing.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/llm/lora_framework/gpu_training_loop.cpp>
    
    # RAG enhancement modules
    ../src/rag/knowledge_gap_detector.cpp
    ../src/rag/llm_judge_client.cpp
    ../src/rag/nli_faithfulness_verifier.cpp
    ../src/rag/quality_control_pipeline.cpp
    ../src/rag/geval_evaluator.cpp
    
    # LLM server API handlers (conditional)
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/server/llm_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/server/lora_api_handler.cpp>
)

if(THEMIS_ENABLE_GPU)
    list(APPEND THEMIS_LLM_SOURCES
        ../src/llm/lora_framework/vram_allocator.cpp
        ../src/llm/lora_framework/gpu_memory.cpp
        ../src/llm/lora_framework/gpu_tensor.cpp
        ../src/llm/lora_framework/gpu_lora_layers.cpp
        ../src/llm/lora_framework/flash_lora.cpp
        ../src/llm/lora_framework/gpu_data_loader.cpp
        ../src/llm/lora_framework/gpu_embedding_layer.cpp
        ../src/llm/lora_framework/multi_gpu_lora_layer.cpp
        ../src/llm/lora_framework/multi_gpu.cpp
        ../src/llm/lora_framework/multi_gpu_trainer.cpp
        ../src/llm/lora_framework/nccl_backend.cpp
        ../src/llm/lora_framework/rccl_backend.cpp
        ../src/llm/lora_framework/custom_allreduce.cpp
        ../src/llm/lora_framework/adaptive_batcher.cpp
        ../src/llm/lora_framework/gpu_utilization_monitor.cpp
        ../src/llm/lora_framework/gradient_checkpointing.cpp
        ../src/llm/lora_framework/gpu_training_loop.cpp
    )
endif()

set(THEMIS_CONTENT_SOURCES
    # Content processing (conditional)
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/content_type.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/content_manager.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/text_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/mock_clip_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/mime_detector.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/content_policy.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/content_fs.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/version_manager.cpp>
    $<$<AND:$<BOOL:${THEMIS_ENABLE_CONTENT}>,$<BOOL:${THEMIS_ENABLE_OFFICE}>>:../src/content/office_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/archive_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/async_ingestion_worker.cpp>
)

set(THEMIS_TIMESERIES_SOURCES
    # Time-series storage
    ../src/timeseries/timeseries.cpp
    ../src/timeseries/tsstore.cpp
    ../src/timeseries/gorilla.cpp
    ../src/timeseries/retention.cpp
    ../src/timeseries/continuous_agg.cpp
    ../src/timeseries/aggregate_scheduler.cpp
    ../src/timeseries/aggregate_scheduler_helper.cpp
    ../src/timeseries/query_optimizer.cpp
    ../src/timeseries/timeseries_metrics.cpp
)

set(THEMIS_NETWORK_SOURCES
    # HTTP Server (conditional)
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/http_server.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/transaction_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/auth_middleware.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/request_validation_middleware.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/diff_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/snapshot_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/pitr_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/mvcc_api_handler.cpp>
    $<$<AND:$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>,$<BOOL:${THEMIS_ENABLE_LLM}>>:../src/server/feedback_api_handler.cpp>
    
    # API handlers (always included)
    ../src/server/cache_api_handler.cpp
    ../src/server/admin_api_handler.cpp
    ../src/server/vector_api_handler.cpp
    ../src/server/spatial_api_handler.cpp
    ../src/server/monitoring_api_handler.cpp
    ../src/server/query_api_handler.cpp
    ../src/server/policy_api_handler.cpp
    ../src/server/prompt_api_handler.cpp
    ../src/server/graph_api_handler.cpp
    ../src/server/index_api_handler.cpp
    ../src/server/timeseries_api_handler.cpp
    ../src/server/entity_api_handler.cpp
    ../src/server/content_api_handler.cpp
    ../src/server/changefeed_api_handler.cpp
    ../src/server/wal_api_handler.cpp
    ../src/server/audit_api_handler.cpp
    ../src/server/pki_api_handler.cpp
    ../src/server/saga_api_handler.cpp
    ../src/server/pii_api_handler.cpp
    ../src/server/retention_api_handler.cpp
    ../src/server/keys_api_handler.cpp
    ../src/server/classification_api_handler.cpp
    ../src/server/reports_api_handler.cpp
    ../src/server/schema_api_handler.cpp
    ../src/server/ranger_adapter.cpp
    ../src/server/policy_engine.cpp
    ../src/server/rate_limiter.cpp
    ../src/server/rate_limiter_v2.cpp
    ../src/server/load_shedder.cpp
    ../src/server/api_version.cpp
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/api_gateway.cpp>
    ../src/server/update_api_handler.cpp
    ../src/server/hot_reload_api_handler.cpp
    ../src/server/export_api_handler.cpp
    ../src/server/tenant_manager.cpp
    ../src/server/sharding_metrics_handler.cpp
    
    # SSE support (conditional)
    $<$<BOOL:${THEMIS_ENABLE_SSE}>:../src/server/sse_connection_manager.cpp>
    
    # gRPC support (conditional)
    $<$<BOOL:${THEMIS_ENABLE_GRPC}>:../src/server/wal_grpc_service.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GRPC}>:../src/server/pitr_grpc_service.cpp>
    
    # Advanced protocols (conditional)
    $<$<BOOL:${THEMIS_ENABLE_HTTP2}>:../src/server/http2_session.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP3}>:../src/server/http3_session.cpp>
    $<$<BOOL:${THEMIS_ENABLE_WEBSOCKET}>:../src/server/websocket_session.cpp>
    $<$<BOOL:${THEMIS_ENABLE_MQTT}>:../src/server/mqtt_session.cpp>
    $<$<BOOL:${THEMIS_ENABLE_POSTGRES_WIRE}>:../src/server/postgres_session.cpp>
    $<$<BOOL:${THEMIS_ENABLE_MCP}>:../src/server/mcp_server.cpp>
    
    # GraphQL API (conditional)
    $<$<BOOL:${THEMIS_ENABLE_GRAPHQL}>:../src/api/graphql.cpp>
    
    # Network protocol server
    ../src/network/wire_protocol_server.cpp
    ../src/network/wire_protocol_connection_pool.cpp
    ../src/network/wire_protocol_v2.cpp
    ../src/network/wire_protocol_performance.cpp
    
    # Observability (GAP-008: Alertmanager integration)
    ../src/observability/alertmanager.cpp
)

set(THEMIS_GEO_SOURCES
    # Geospatial processing
    ../src/geo/cpu_backend.cpp
    ../src/geo/gpu_backend_stub.cpp
    ../src/geo/boost_cpu_exact_backend.cpp
    ../src/gpu/device_discovery.cpp
    ../src/gpu/safe_fail.cpp
    ../src/gpu/metrics.cpp
    ../src/gpu/audit_log.cpp
)

set(THEMIS_GRAPH_SOURCES
    # Graph indexes and analytics
    ../src/index/temporal_graph.cpp
    ../src/index/property_graph.cpp
    ../src/index/edge_types.cpp
    ../src/index/process_graph.cpp
    ../src/index/gnn_embeddings.cpp
    ../src/index/graph_analytics.cpp
    ../src/graph/graph_query_optimizer.cpp
)

# Function to build modular architecture (post-v1.3.0)
function(themis_build_modular)
    message(STATUS "Building ThemisDB with modular architecture")
    
    # Core modules (always required)
    set(_themis_base_deps
        OpenSSL::SSL
        OpenSSL::Crypto
        fmt::fmt
        spdlog::spdlog
        Boost::system
        nlohmann_json::nlohmann_json
        ${THEMIS_YAML_TARGET}
    )
    if(THEMIS_ENABLE_GRPC)
        find_package(gRPC CONFIG)
        find_package(Protobuf CONFIG)
        if(gRPC_FOUND AND Protobuf_FOUND)
            list(APPEND _themis_base_deps gRPC::grpc++ protobuf::libprotobuf)
        endif()
    endif()
    if(THEMIS_ENABLE_TRACING)
        find_package(opentelemetry-cpp CONFIG REQUIRED)
        if(NOT TARGET opentelemetry-cpp::trace)
            message(FATAL_ERROR "Required CMake target 'opentelemetry-cpp::trace' not found. Ensure opentelemetry-cpp was found successfully.")
        endif()
        if(NOT TARGET opentelemetry-cpp::otlp_http_exporter)
            message(FATAL_ERROR "Required CMake target 'opentelemetry-cpp::otlp_http_exporter' not found. Ensure opentelemetry-cpp was found with otlp-http feature.")
        endif()
    endif()
    if(TARGET CURL::libcurl)
        list(APPEND _themis_base_deps CURL::libcurl)
    endif()

    themis_add_module(base
        SOURCES ${THEMIS_BASE_SOURCES}
        DEPENDENCIES ${_themis_base_deps}
    )

    if(THEMIS_ENABLE_TRACING)
        target_link_libraries(themis_base PRIVATE
            opentelemetry-cpp::trace
            opentelemetry-cpp::otlp_http_exporter
        )
    endif()
    
    set(_themis_storage_deps
        themis_base
        ${THEMIS_ROCKSDB_TARGET}
    )
    if(TARGET simdjson::simdjson)
        list(APPEND _themis_storage_deps simdjson::simdjson)
    endif()
    if(TARGET TBB::tbb)
        list(APPEND _themis_storage_deps TBB::tbb)
    endif()
    if(THEMIS_ENABLE_GPU AND TARGET faiss)
        list(APPEND _themis_storage_deps faiss)
    endif()

    themis_add_module(storage
        SOURCES ${THEMIS_STORAGE_SOURCES}
        DEPENDENCIES ${_themis_storage_deps}
    )
    
    set(_themis_security_deps
        themis_base
        themis_storage
        OpenSSL::SSL
        OpenSSL::Crypto
    )
    if(TARGET TBB::tbb)
        list(APPEND _themis_security_deps TBB::tbb)
    endif()
    if(TARGET CURL::libcurl)
        list(APPEND _themis_security_deps CURL::libcurl)
    endif()
    if(THEMIS_ENABLE_MIMALLOC AND TARGET mimalloc)
        list(APPEND _themis_security_deps mimalloc)
    endif()
    if(WIN32)
        list(APPEND _themis_security_deps Secur32)
    endif()
    
    themis_add_module(security
        SOURCES ${THEMIS_SECURITY_SOURCES}
        DEPENDENCIES ${_themis_security_deps}
    )
    
    themis_add_module(transaction
        SOURCES ${THEMIS_TRANSACTION_SOURCES}
        DEPENDENCIES 
            themis_base 
            themis_storage
            themis_security
    )

    set(_themis_query_deps
        themis_base
        themis_storage
        themis_transaction
        ${THEMIS_ARROW_TARGET}
        ${THEMIS_PARQUET_TARGET}
    )
    if(THEMIS_MODULE_LLM)
        list(APPEND _themis_query_deps themis_llm)
    endif()
    
    themis_add_module(query
        SOURCES ${THEMIS_QUERY_SOURCES}
        DEPENDENCIES ${_themis_query_deps}
    )
    
    themis_add_module(network
        SOURCES ${THEMIS_NETWORK_SOURCES}
        DEPENDENCIES 
            themis_base 
            themis_storage 
            themis_query 
            themis_transaction
    )
    
    # Optional modules
    if(THEMIS_MODULE_SHARDING)
        themis_add_module(sharding
            SOURCES ${THEMIS_SHARDING_SOURCES}
            DEPENDENCIES 
                themis_base 
                themis_storage 
                themis_security
                themis_transaction
        )
        # Ensure proto files are generated before compiling sharding sources
        if(TARGET themis_shard_proto)
            add_dependencies(themis_sharding themis_shard_proto)
            target_link_libraries(themis_sharding PRIVATE themis_shard_proto)
            # Add include directory for generated proto headers
            target_include_directories(themis_sharding PRIVATE ${CMAKE_BINARY_DIR}/proto_generated)
            message(STATUS "themis_sharding linked to themis_shard_proto for gRPC inter-shard communication")
        endif()
        # Additional gRPC dependencies for sharding module
        if(TARGET gRPC::grpc++)
            target_link_libraries(themis_sharding PUBLIC gRPC::grpc++)
        endif()
        if(TARGET protobuf::libprotobuf)
            target_link_libraries(themis_sharding PUBLIC protobuf::libprotobuf)
        endif()
    endif()
    
    if(THEMIS_MODULE_TIMESERIES)
        themis_add_module(timeseries
            SOURCES ${THEMIS_TIMESERIES_SOURCES}
            DEPENDENCIES 
                themis_base 
                themis_storage
        )
    endif()
    
    if(THEMIS_MODULE_LLM)
        themis_add_module(llm
            SOURCES ${THEMIS_LLM_SOURCES}
            DEPENDENCIES 
                themis_base 
                themis_storage
                themis_security
                themis_sharding
        )
        target_include_directories(themis_llm PRIVATE
            ${CMAKE_SOURCE_DIR}/llama.cpp/include
            ${CMAKE_SOURCE_DIR}/llama.cpp/ggml/include
        )
        if(THEMIS_ENABLE_MIMALLOC AND TARGET mimalloc)
            target_link_libraries(themis_llm PUBLIC mimalloc)
        endif()
        if(TARGET llama)
            target_link_libraries(themis_llm PUBLIC llama)
        elseif(llama_LIBRARIES)
            target_link_libraries(themis_llm PUBLIC ${llama_LIBRARIES})
        endif()
    endif()
    
    if(THEMIS_MODULE_GEO)
        set(_themis_geo_deps
            themis_base
            themis_storage
        )
        if(TARGET Boost::geometry)
            list(APPEND _themis_geo_deps Boost::geometry)
        elseif(TARGET Boost::boost)
            list(APPEND _themis_geo_deps Boost::boost)
        elseif(TARGET Boost::headers)
            list(APPEND _themis_geo_deps Boost::headers)
        elseif(Boost_FOUND)
            if(NOT TARGET themis_boost_headers)
                add_library(themis_boost_headers INTERFACE)
                target_include_directories(themis_boost_headers INTERFACE ${Boost_INCLUDE_DIRS})
            endif()
            list(APPEND _themis_geo_deps themis_boost_headers)
        endif()

        themis_add_module(geo
            SOURCES ${THEMIS_GEO_SOURCES}
            DEPENDENCIES ${_themis_geo_deps}
        )
    endif()
    
    if(THEMIS_MODULE_GRAPH)
        themis_add_module(graph
            SOURCES ${THEMIS_GRAPH_SOURCES}
            DEPENDENCIES 
                themis_base 
                themis_storage
                themis_transaction
        )
    endif()
    
    if(THEMIS_MODULE_CONTENT)
        themis_add_module(content
            SOURCES ${THEMIS_CONTENT_SOURCES}
            DEPENDENCIES 
                themis_base 
                themis_storage
        )
    endif()

    # Cross-module fixups for modular build
    # Removed: storage -> security link to avoid cycle
    # if(TARGET themis_storage AND TARGET themis_security)
    #     target_link_libraries(themis_storage PUBLIC themis_security)
    # endif()
    if(THEMIS_ENABLE_MIMALLOC AND TARGET mimalloc)
        target_link_libraries(themis_storage PUBLIC mimalloc)
    endif()
    
    # Create convenience variable for all modules to link against
    set(THEMIS_ALL_MODULES
        themis_base
        themis_storage
        themis_query
        themis_security
        themis_transaction
        themis_network
        PARENT_SCOPE
    )
    
    if(THEMIS_MODULE_SHARDING)
        list(APPEND THEMIS_ALL_MODULES themis_sharding)
    endif()
    
    if(THEMIS_MODULE_TIMESERIES)
        list(APPEND THEMIS_ALL_MODULES themis_timeseries)
    endif()
    
    if(THEMIS_MODULE_LLM)
        list(APPEND THEMIS_ALL_MODULES themis_llm)
    endif()
    
    if(THEMIS_MODULE_GEO)
        list(APPEND THEMIS_ALL_MODULES themis_geo)
    endif()
    
    if(THEMIS_MODULE_GRAPH)
        list(APPEND THEMIS_ALL_MODULES themis_graph)
    endif()
    
    if(THEMIS_MODULE_CONTENT)
        list(APPEND THEMIS_ALL_MODULES themis_content)
    endif()
    
    set(THEMIS_ALL_MODULES ${THEMIS_ALL_MODULES} PARENT_SCOPE)
endfunction()

