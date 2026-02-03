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
    
    # Windows: Export all symbols for DLL
    if(MSVC)
        set_target_properties(themis_${MODULE_NAME} PROPERTIES
            WINDOWS_EXPORT_ALL_SYMBOLS ON
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
    ../src/utils/audit_logger.cpp
    ../src/utils/hkdf_helper.cpp
    ../src/utils/hkdf_cache.cpp
    ../src/utils/saga_logger.cpp
    ../src/utils/lek_manager.cpp
    ../src/utils/stemmer.cpp
    ../src/utils/stopwords.cpp
    ../src/utils/pii_pseudonymizer.cpp
    ../src/utils/normalizer.cpp
    ../src/utils/simd_distance.cpp
    ../src/utils/update_checker.cpp
    ../src/utils/http_client_pool.cpp
    ../src/utils/grpc_channel_pool.cpp
    ../src/utils/build_info.cpp
    ../src/utils/license_info.cpp
    ../src/utils/error_registry.cpp
    ../src/utils/memory/pool_allocator.cpp
    ../src/utils/boost_throw_exception.cpp
    ../src/utils/file_utils.cpp
    
    # Cross-cutting concerns abstraction layer
    ../src/core/concerns/i_logger.cpp
    ../src/core/concerns/concerns_context.cpp
    
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
    
    # Stubs for missing symbols
    ../src/stubs.cpp
)

set(THEMIS_STORAGE_SOURCES
    # Core storage engine
    ../src/storage/rocksdb_wrapper.cpp
    ../src/storage/base_entity.cpp
    ../src/storage/key_schema.cpp
    ../src/storage/backup_manager.cpp
    ../src/storage/nlp_metadata_extractor.cpp
    ../src/storage/columnar_format.cpp
    ../src/storage/pitr_manager.cpp
    ../src/storage/blob_redundancy_manager.cpp
    
    # Metadata management
    ../src/metadata/schema_manager.cpp
    
    # Indexes
    ../src/index/secondary_index.cpp
    ../src/index/vector_index.cpp
    ../src/index/rotary_embeddings.cpp
    ../src/index/learnable_rope.cpp
    ../src/index/hnsw_layer_optimizer.cpp
    ../src/index/hnsw_parameter_tuner.cpp
    ../src/index/hnsw_production_defaults.cpp
    ../src/index/product_quantizer.cpp
    ../src/index/adaptive_index.cpp
    ../src/index/spatial_index.cpp
    
    # Performance enhancements
    ../src/performance/phase2_feature_flags.cpp
    ../src/performance/phase3/feature_flags.cpp
    
    # Hybrid search
    ../src/search/hybrid_search.cpp
    
    # Storage enhancements
    ../src/cache/semantic_cache.cpp
    ../src/cache/embedding_cache.cpp
    
    # Updates
    ../src/updates/release_manifest.cpp
    ../src/updates/manifest_database.cpp
    ../src/updates/hot_reload_engine.cpp
    ../src/updates/updates_config.cpp
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
    ../src/query/statistical_aggregator.cpp
    ../src/query/semantic_cache.cpp
    ../src/query/functions/function_registry.cpp
    
    # Analytics
    ../src/analytics/olap.cpp
    ../src/analytics/process_mining.cpp
    ../src/analytics/nlp_text_analyzer.cpp
    ../src/analytics/diff_engine.cpp
    
    # AQL handlers
    ../src/aql/llm_aql_handler.cpp
    
    # Import/Export
    ../src/exporters/jsonl_llm_exporter.cpp
    ../src/importers/postgres_importer.cpp
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
    
    # Storage security
    ../src/storage/security_signature.cpp
    ../src/storage/security_signature_manager.cpp
    
    # Authentication
    ../src/auth/jwt_validator.cpp
    ../src/auth/gssapi_authenticator.cpp
    
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
)

set(THEMIS_TRANSACTION_SOURCES
    # Transaction management
    ../src/transaction/transaction_manager.cpp
    ../src/transaction/saga.cpp
    ../src/transaction/snapshot_manager.cpp
    
    # Temporal conflict resolution
    ../src/temporal/temporal_conflict_resolver.cpp
    
    # Replication
    ../src/replication/replication_manager.cpp
    
    # Change data capture
    ../src/cdc/changefeed.cpp
)

set(THEMIS_SHARDING_SOURCES
    # Sharding core
    ../src/sharding/urn.cpp
    ../src/sharding/consistent_hash.cpp
    ../src/sharding/shard_topology.cpp
    ../src/sharding/urn_resolver.cpp
    ../src/sharding/pki_shard_certificate.cpp
    ../src/sharding/mtls_client.cpp
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
    ../src/sharding/cloud_agent.cpp
    ../src/sharding/circuit_breaker.cpp
    ../src/sharding/gossip_protocol.cpp
    ../src/sharding/gossip_config_manager.cpp
    ../src/sharding/distributed_coordinator.cpp
    ../src/sharding/shard_resource_manager.cpp
    ../src/sharding/locality_aware_router.cpp
    
    # Raft consensus
    ../src/sharding/raft_configuration.cpp
    ../src/sharding/raft_log.cpp
    ../src/sharding/raft_state.cpp
    ../src/sharding/raft_wal_integration.cpp
    ../src/sharding/raft_consensus.cpp
    ../src/sharding/quorum_manager.cpp
    ../src/sharding/partition_detector.cpp
    ../src/sharding/replica_consistency.cpp
    
    # WAL and replication
    ../src/sharding/stream_protocol.cpp
    ../src/sharding/wal_applier.cpp
    ../src/sharding/wal_manager.cpp
    ../src/sharding/wal_shipper.cpp
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
    ../src/sharding/cross_shard_transaction.cpp
    
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
)

set(THEMIS_LLM_SOURCES
    # LLM core components
    ../src/llm/llm_interaction_store.cpp
    ../src/llm/prompt_manager.cpp
    ../src/llm/block_table.cpp
    ../src/llm/paged_kv_cache.cpp
    
    # RAG enhancement modules
    ../src/rag/knowledge_gap_detector.cpp
    
    # LLM server API handlers (conditional)
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/server/llm_api_handler.cpp>
)

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
)

set(THEMIS_NETWORK_SOURCES
    # HTTP Server (conditional)
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/http_server.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/transaction_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/auth_middleware.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/diff_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/snapshot_api_handler.cpp>
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
    ../src/server/update_api_handler.cpp
    ../src/server/hot_reload_api_handler.cpp
    ../src/server/export_api_handler.cpp
    ../src/server/tenant_manager.cpp
    ../src/server/sharding_metrics_handler.cpp
    
    # SSE support (conditional)
    $<$<BOOL:${THEMIS_ENABLE_SSE}>:../src/server/sse_connection_manager.cpp>
    
    # gRPC support (conditional)
    $<$<BOOL:${THEMIS_ENABLE_GRPC}>:../src/server/wal_grpc_service.cpp>
    
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
    
    # Observability
    ../src/observability/metrics_collector.cpp
)

set(THEMIS_GEO_SOURCES
    # Geospatial processing
    ../src/geo/cpu_backend.cpp
    ../src/geo/gpu_backend_stub.cpp
    ../src/geo/boost_cpu_exact_backend.cpp
    ../src/api/geo_index_hooks.cpp
    ../src/utils/geo/ewkb.cpp
)

set(THEMIS_GRAPH_SOURCES
    # Graph indexes and analytics
    ../src/index/graph_index.cpp
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
    themis_add_module(base
        SOURCES ${THEMIS_BASE_SOURCES}
        DEPENDENCIES 
            OpenSSL::SSL
            OpenSSL::Crypto
            fmt::fmt
            spdlog::spdlog
            Boost::system
            nlohmann_json::nlohmann_json
            ${THEMIS_YAML_TARGET}
    )
    
    themis_add_module(storage
        SOURCES ${THEMIS_STORAGE_SOURCES}
        DEPENDENCIES 
            themis_base 
            ${THEMIS_ROCKSDB_TARGET}
            simdjson::simdjson
            TBB::tbb
    )
    
    themis_add_module(query
        SOURCES ${THEMIS_QUERY_SOURCES}
        DEPENDENCIES 
            themis_base 
            themis_storage
            ${THEMIS_ARROW_TARGET}
            ${THEMIS_PARQUET_TARGET}
    )
    
    themis_add_module(security
        SOURCES ${THEMIS_SECURITY_SOURCES}
        DEPENDENCIES 
            themis_base 
            OpenSSL::SSL 
            OpenSSL::Crypto
    )
    
    themis_add_module(transaction
        SOURCES ${THEMIS_TRANSACTION_SOURCES}
        DEPENDENCIES 
            themis_base 
            themis_storage
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
        )
    endif()
    
    if(THEMIS_MODULE_GEO)
        themis_add_module(geo
            SOURCES ${THEMIS_GEO_SOURCES}
            DEPENDENCIES 
                themis_base 
                themis_storage
                Boost::geometry
        )
    endif()
    
    if(THEMIS_MODULE_GRAPH)
        themis_add_module(graph
            SOURCES ${THEMIS_GRAPH_SOURCES}
            DEPENDENCIES 
                themis_base 
                themis_storage
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
