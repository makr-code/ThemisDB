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
    option(THEMIS_MODULE_CONTENT "Include content processors module (optional)" ON)
    option(THEMIS_MODULE_TIMESERIES "Include time-series module" ON)
    option(THEMIS_MODULE_SHARDING "Include distributed sharding module" ON)
    option(THEMIS_MODULE_INGESTION "Include ingestion module (all data-intake connectors)" ON)
endif()

# Helper function to create a modular library target
# Usage: themis_add_module(module_name SOURCES file1.cpp file2.cpp ... DEPENDENCIES dep1 dep2 ... [DISABLE_AUTO_EXPORT] [STATIC_MODULE])
function(themis_add_module MODULE_NAME)
    cmake_parse_arguments(ARG "DISABLE_AUTO_EXPORT;STATIC_MODULE" "" "SOURCES;DEPENDENCIES" ${ARGN})

    if(ARG_STATIC_MODULE)
        set(_themis_module_type STATIC)
    else()
        set(_themis_module_type SHARED)
    endif()

    # Create the library (SHARED by default for modular build)
    add_library(themis_${MODULE_NAME} ${_themis_module_type} ${ARG_SOURCES})
    
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
    
    # jemalloc compile definitions for all modules
    if(THEMIS_ENABLE_JEMALLOC)
        target_compile_definitions(themis_${MODULE_NAME} PRIVATE THEMIS_ENABLE_JEMALLOC)
    endif()
    
    # Windows: Export all symbols for DLL and disable /GL so __create_def can read symbols.
    # Large modules can disable this to avoid oversized import libraries.
    if(MSVC AND NOT ARG_STATIC_MODULE)
        if(ARG_DISABLE_AUTO_EXPORT)
            set_target_properties(themis_${MODULE_NAME} PROPERTIES
                WINDOWS_EXPORT_ALL_SYMBOLS OFF
                INTERPROCEDURAL_OPTIMIZATION FALSE
                VS_GLOBAL_WholeProgramOptimization "false"
            )
        else()
            set_target_properties(themis_${MODULE_NAME} PROPERTIES
                WINDOWS_EXPORT_ALL_SYMBOLS ON
                INTERPROCEDURAL_OPTIMIZATION FALSE
                VS_GLOBAL_WholeProgramOptimization "false"
            )
        endif()
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
    ../src/utils/cron_parser.cpp
    ../src/utils/bloom_filter.cpp
    ../src/utils/checksum_utils.cpp
    ../src/utils/compression_metrics.cpp
    ../src/utils/sampled_logger.cpp
    ../src/utils/self_awareness.cpp
    ../src/utils/timestamp_utils.cpp
    ../src/observability/metrics_collector.cpp
    ../src/config/config_path_resolver.cpp
    ../src/config/config_file_watcher.cpp
    ../src/config/config_metrics_exporter.cpp
    ../src/config/config_schema_validator.cpp
    ../src/config/config_audit_log.cpp
    ../src/config/config_encrypted_store.cpp
    ../src/themis/build_info.cpp
    ../src/themis/license_info.cpp
    ../src/utils/runtime_license_gate.cpp
    ../src/utils/error_registry.cpp
    ../src/utils/memory/pool_allocator.cpp
    ../src/utils/boost_throw_exception.cpp
    ../src/utils/file_utils.cpp
    ../src/utils/thread_pool_manager.cpp
    ../src/utils/consistent_hash.cpp
    ../src/utils/checksum_utils.cpp
    ../src/utils/sampled_logger.cpp
    ../src/utils/timestamp_utils.cpp
    ../src/utils/rate_limiter.cpp
    
    # Cross-cutting concerns abstraction layer
    ../src/core/concerns/i_logger.cpp
    ../src/core/concerns/concerns_context.cpp
    ../src/core/concerns/context_propagation.cpp
    ../src/core/concerns/redis_cache.cpp
    ../src/core/concerns/lockfree_metrics.cpp
    ../src/core/concerns/zero_copy_logger.cpp
    ../src/core/adapters/otel_tracer.cpp
    ../src/sharding/circuit_breaker.cpp
    
    # Hardware acceleration (core abstraction layer)
    ../src/acceleration/compute_backend.cpp
    ../src/acceleration/backend_registry.cpp
    ../src/acceleration/cpu_backend.cpp
    ../src/acceleration/multi_gpu_backend.cpp
    ../src/acceleration/tensor_core_matmul.cpp
    ../src/acceleration/plugin_loader.cpp
    ../src/acceleration/plugin_security.cpp
    ../src/acceleration/device_manager.cpp
    ../src/acceleration/vllm_resource_manager.cpp
    ../src/acceleration/shader_integrity.cpp
    # PERF-D3: Parallel batch insertion + SIMD distance pipeline
    ../src/acceleration/vec_knn.cpp
    # CPU multi-threaded backends (requires THEMIS_ENABLE_GPU)
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/acceleration/cpu_backend_mt.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/acceleration/cpu_backend_tbb.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/acceleration/graphics_backends.cpp>
    # GPU-specific backends
    $<$<AND:$<BOOL:${THEMIS_ENABLE_GPU}>,$<BOOL:${WIN32}>>:../src/acceleration/directx_backend_full.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HIP}>:../src/acceleration/hip_backend.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CUDA}>:../src/acceleration/cuda_backend.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CUDA}>:../src/acceleration/cuda/ann_kernels.cu>
    $<$<BOOL:${THEMIS_ENABLE_CUDA}>:../src/acceleration/cuda/vector_kernels.cu>
    $<$<BOOL:${THEMIS_ENABLE_CUDA}>:../src/acceleration/cuda/tensor_core_matmul.cu>
    $<$<BOOL:${THEMIS_ENABLE_CUDA}>:../src/acceleration/cuda/geo_kernels.cu>
    $<$<BOOL:${THEMIS_ENABLE_CUDA}>:../src/acceleration/cuda/graph_kernels.cu>
    $<$<OR:$<BOOL:${THEMIS_ENABLE_CUDA}>,$<BOOL:${THEMIS_ENABLE_HIP}>>:../src/acceleration/faiss_gpu_backend.cpp>
    $<$<BOOL:${THEMIS_ENABLE_ONEAPI}>:../src/acceleration/oneapi_backend.cpp>
    $<$<BOOL:${THEMIS_ENABLE_OPENCL}>:../src/acceleration/opencl_backend.cpp>
    $<$<BOOL:${THEMIS_ENABLE_VULKAN}>:../src/acceleration/vulkan_backend_full.cpp>
    $<$<BOOL:${THEMIS_ENABLE_ZLUDA}>:../src/acceleration/zluda_backend.cpp>
    # NCCL/RCCL vector backends (always compile for stub availability)
    ../src/acceleration/nccl_vector_backend.cpp
    ../src/acceleration/rccl_vector_backend.cpp
    $<$<BOOL:${THEMIS_ENABLE_VULKAN}>:../src/acceleration/vulkan_backend_full.cpp>
    ../src/gpu/device_discovery.cpp
    
    # Plugin manager (core plugin orchestration)
    ../src/plugins/plugin_manager.cpp
    ../src/plugins/plugin_hot_plug_monitor.cpp
    ../src/plugins/plugin_registry.cpp
    ../src/plugins/plugin_metrics.cpp
    ../src/plugins/plugin_health_monitor.cpp
    ../src/plugins/plugin_system_edition.cpp
    ../src/plugins/signed_plugin_repository.cpp
    ../src/plugins/oci_registry_client.cpp
    ../src/plugins/rpc_service_registry.cpp
    
    # Module loader (for security verification of modular DLLs)
    # Migrated to src/themis/ (v1.7.0): split into platform-independent core,
    # platform-specific helpers, and security verifier.
    ../src/themis/module_loader.cpp
    ../src/themis/module_loader_win32.cpp
    ../src/themis/module_loader_linux.cpp
    ../src/themis/module_security.cpp
    ../src/base/module_sandbox.cpp
    ../src/base/hot_reload_manager.cpp
    ../src/base/ab_test_manager.cpp
    ../src/base/remote_registry_client.cpp
    ../src/base/wasm_plugin_sandbox.cpp
    ../src/base/wasm_runtime_injector.cpp
    ../src/base/plugin_dependency_graph.cpp
    ../src/themis/module_hash_verifier.cpp
    ../src/themis/module_dependency_resolver.cpp
    ../src/themis/module_signature_verifier.cpp
    ../src/themis/edition_manager.cpp
    
    # Stubs for missing symbols
    ../src/stubs.cpp
    # Interface stubs: forces MSVC to emit ISecondaryIndex/IVectorIndex/IGraphIndex
    # constructor+destructor symbols into themis_base.dll (THEMIS_BASE_API = dllexport)
    ../src/core/index_interface_stubs.cpp
)

set(THEMIS_STORAGE_SOURCES
    # Core storage engine
    ../src/storage/rocksdb_wrapper.cpp
    ../src/storage/wom_tree.cpp
    ../src/storage/base_entity.cpp
    ../src/storage/key_schema.cpp
    ../src/storage/backup_manager.cpp
    ../src/storage/columnar_format.cpp
    ../src/storage/batch_write_optimizer.cpp
    # ../src/storage/pitr_manager.cpp  # Temporarily disabled - needs transaction module
    ../src/storage/blob_redundancy_manager.cpp
    ../src/storage/erasure_coding_backend.cpp
    ../src/storage/erasure_coder_factory.cpp
    ../src/storage/database_connection_manager.cpp
    ../src/storage/disk_space_monitor.cpp
    # WAL for durability and crash recovery
    ../src/storage/wal_storage.cpp
    ../src/storage/transaction_retry_manager.cpp
    # Compaction and GC management
    ../src/storage/compaction_manager.cpp
    # Adaptive compaction scheduler – ML-based scheduling (v1.7.0, Issue #209)
    ../src/storage/adaptive_compaction.cpp
    # RocksDB merge operators (counter/last-write-wins/append merge semantics)
    ../src/storage/merge_operators.cpp
    # Storage Audit Logger
    ../src/storage/storage_audit_logger.cpp
    # MVCC versioning and HLC timestamping
    ../src/storage/hlc.cpp
    ../src/storage/mvcc_store.cpp
    # Atomic history and conflict layer
    ../src/storage/history_manager.cpp
    ../src/storage/raft_mvcc_bridge.cpp
    # Tiered storage (hot/warm/cold) with age- and access-based migration
    ../src/storage/tiered_storage.cpp
    # Distributed transactions (2PC across multiple shards) – v1.7.0
    ../src/storage/distributed_transaction_manager.cpp
    # NVMe optimizations (io_uring, multi-queue, ZNS, Direct I/O) – v1.6.0
    ../src/storage/nvme_manager.cpp
    # Storage engine abstraction (DI-based)
    ../src/storage/storage_engine.cpp
    # Compression strategies (pluggable per-column-family)
    ../src/storage/compressed_storage.cpp
    ../src/storage/compression_strategy.cpp
    ../src/storage/gpu_compression.cpp
    # Index maintenance moved to THEMIS_SECURITY_SOURCES (depends on vector index internals)
    # Blob storage backends
    ../src/storage/blob_backend_filesystem.cpp
    $<$<AND:$<BOOL:${THEMIS_ENABLE_BLOB_S3}>,$<BOOL:${THEMIS_ENABLE_CLOUD_STORAGE}>,$<BOOL:${THEMIS_HAS_AWS_SDK}>>:../src/storage/blob_backend_s3.cpp>
    $<$<AND:$<BOOL:${THEMIS_ENABLE_BLOB_AZURE}>,$<BOOL:${THEMIS_ENABLE_CLOUD_STORAGE}>,$<BOOL:${THEMIS_HAS_AZURE_STORAGE}>>:../src/storage/blob_backend_azure.cpp>
    $<$<AND:$<BOOL:${THEMIS_ENABLE_BLOB_WEBDAV}>,$<BOOL:${THEMIS_ENABLE_CLOUD_STORAGE}>>:../src/storage/blob_backend_webdav.cpp>
    $<$<AND:$<BOOL:${THEMIS_ENABLE_BLOB_GCS}>,$<BOOL:${THEMIS_ENABLE_CLOUD_STORAGE}>,$<BOOL:${THEMIS_HAS_GCS_SDK}>>:../src/storage/blob_backend_gcs.cpp>
    # Merge operators (counter, list-append RocksDB custom operators)
    ../src/storage/merge_operators.cpp
    ../src/sharding/distributed_time_coordinator.cpp
    
    # Metadata management
    ../src/metadata/schema_manager.cpp
    ../src/metadata/statistics_collector.cpp
    ../src/metadata/information_schema.cpp
    ../src/metadata/schema_constraints.cpp
    ../src/metadata/schema_version_manager.cpp
    ../src/metadata/index_recommender.cpp
    ../src/metadata/column_lineage.cpp
    ../src/metadata/schema_audit_log.cpp
    ../src/metadata/schema_consistency_checker.cpp
    ../src/metadata/catalog_exporter.cpp
    # ../src/metadata/distributed_catalog.cpp
    # Temporarily excluded in modular build: depends on MetadataShardRouter
    # symbols from sharding module and introduces unresolved externals in
    # themis_storage when sharding is linked as a separate DLL.
    
    # Indexes
    ../src/index/secondary_index.cpp
    ../src/index/index_compression.cpp
    ../src/index/ann_index.cpp
    ../src/index/rotary_embeddings.cpp
    ../src/index/rotary_embeddings_gpu_cpu.cpp
    ../src/index/learnable_rope.cpp
    ../src/index/lora_rope.cpp
    ../src/index/hnsw_layer_optimizer.cpp
    ../src/index/hnsw_parameter_tuner.cpp
    ../src/index/hnsw_production_defaults.cpp
    ../src/index/cuda_hnsw_graph_traversal.cpp
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/index/gpu_vector_index.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/index/multi_gpu_vector_index.cpp>
    $<$<BOOL:${THEMIS_ENABLE_VULKAN}>:../src/index/gpu_vector_index_vulkan.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CUDA}>:../src/index/rotary_embeddings_cuda.cu>
    $<$<BOOL:${THEMIS_ENABLE_HIP}>:../src/index/rotary_embeddings_hip.cpp>
    ../src/index/advanced_vector_index.cpp
    ../src/index/product_quantizer.cpp
    ../src/index/binary_quantizer.cpp
    ../src/index/learned_quantizer.cpp
    ../src/index/residual_quantizer.cpp
    ../src/index/adaptive_index.cpp
    ../src/index/distributed_vector_index.cpp
    ../src/index/inverted_index.cpp
    ../src/index/workload_replay.cpp
    ../src/index/tiered_index_manager.cpp
    ../src/index/tiered_index_manager.cpp
    ../src/api/tracing_middleware.cpp
    ../src/api/otlp_exporter.cpp
    ../src/utils/geo/ewkb.cpp
    
    # Performance enhancements
    ../src/performance/phase2_feature_flags.cpp
    ../src/performance/phase3/feature_flags.cpp
    ../src/performance/numa_topology.cpp
    ../src/performance/prometheus_exporter.cpp
    ../src/performance/chimera_exporter.cpp
    ../src/performance/async_metrics_exporter.cpp
    ../src/performance/phase3/memory_pressure.cpp
    ../src/performance/phase3/adaptive_batch_tuner.cpp
    ../src/performance/phase4/feature_flags.cpp
    # pmu_counters.cpp is always compiled: it provides stub fallbacks when
    # perf_event_open is unavailable (containers, non-Linux).  The actual PMU
    # paths are gated by the THEMIS_ENABLE_PMU_COUNTERS compile definition.
    ../src/performance/phase4/pmu_counters.cpp
    
    # Storage enhancements
    ../src/cache/semantic_cache.cpp
    
    # Updates
    ../src/updates/release_manifest.cpp
    ../src/updates/manifest_database.cpp
    ../src/updates/hot_reload_engine.cpp
    ../src/updates/update_history_logger.cpp
    ../src/updates/updates_config.cpp
    ../src/updates/update_state_machine.cpp
    ../src/updates/canary_rollout.cpp
    ../src/updates/delta_update_engine.cpp
    ../src/updates/parallel_downloader.cpp
    ../src/updates/dependency_resolver.cpp
    ../src/updates/schema_migration_tester.cpp
    ../src/updates/in_place_schema_migrator.cpp
    ../src/updates/schema_migration.cpp
    ../src/updates/notification_webhook.cpp
    ../src/updates/blue_green_deployment.cpp
    ../src/updates/coordinated_update_manager.cpp
    ../src/updates/cluster_update_manager.cpp
    ../src/updates/preflight_health_check.cpp
    ../src/updates/tenant_update_scheduler.cpp

    # Storage security
    ../src/storage/security_signature.cpp
    ../src/storage/security_signature_manager.cpp

    # Change data capture (used by metadata/schema manager)
    ../src/cdc/changefeed.cpp
    ../src/cdc/changefeed_buffer.cpp
    ../src/cdc/consumer_group.cpp
    ../src/cdc/delivery_tracker.cpp
    ../src/cdc/dead_letter_queue.cpp
    ../src/cdc/outbox.cpp
    ../src/cdc/cdc_ws_handler.cpp
    ../src/cdc/cross_collection_stream.cpp
    ../src/cdc/cdc_materialized_view.cpp
    ../src/cdc/tenant_buffer_manager.cpp
    $<$<BOOL:${THEMIS_ENABLE_WEBSOCKET}>:../src/cdc/ws_transport.cpp>
    ../src/analytics/incremental_view.cpp
    $<$<BOOL:${THEMIS_ENABLE_KAFKA}>:../src/cdc/kafka_cdc_producer.cpp>
)

# Optional performance-optimization sources for the storage module
if(THEMIS_ENABLE_WISCKEY)
    list(APPEND THEMIS_STORAGE_SOURCES ../src/performance/wisckey.cpp)
endif()
if(THEMIS_ENABLE_DOSTOEVSKY)
    list(APPEND THEMIS_STORAGE_SOURCES ../src/performance/dostoevsky.cpp)
endif()
if(THEMIS_ENABLE_CICADA)
    list(APPEND THEMIS_STORAGE_SOURCES ../src/performance/cicada.cpp)
endif()
if(THEMIS_ENABLE_LIGRA)
    list(APPEND THEMIS_STORAGE_SOURCES ../src/performance/ligra.cpp)
endif()
if(THEMIS_ENABLE_RABITQ)
    list(APPEND THEMIS_STORAGE_SOURCES ../src/performance/rabitq.cpp)
endif()
if(THEMIS_ENABLE_DISKANN)
    list(APPEND THEMIS_STORAGE_SOURCES ../src/performance/phase3/diskann.cpp)
endif()
if(THEMIS_ENABLE_BWTREE)
    list(APPEND THEMIS_STORAGE_SOURCES ../src/performance/phase3/bwtree.cpp)
endif()
if(THEMIS_ENABLE_SPLINTERDB)
    list(APPEND THEMIS_STORAGE_SOURCES ../src/performance/phase3/splinterdb.cpp)
endif()
if(THEMIS_ENABLE_GUNROCK)
    list(APPEND THEMIS_STORAGE_SOURCES ../src/performance/phase3/gunrock.cpp)
endif()
if(THEMIS_ENABLE_BAO)
    list(APPEND THEMIS_STORAGE_SOURCES ../src/performance/phase3/bao.cpp)
endif()
if(THEMIS_ENABLE_PMEM)
    list(APPEND THEMIS_STORAGE_SOURCES ../src/performance/phase4/pmem_storage.cpp)
endif()
if(THEMIS_ENABLE_IO_URING)
    list(APPEND THEMIS_STORAGE_SOURCES ../src/performance/phase4/io_uring_zero_copy.cpp)
endif()

set(THEMIS_QUERY_SOURCES
    # Query engine
    ../src/query/query_engine.cpp
    ../src/query/query_optimizer.cpp
    ../src/query/adaptive_optimizer.cpp
    ../src/query/adaptive_join.cpp
    ../src/query/runtime_reoptimizer.cpp
    ../src/query/optimizer_cost_model.cpp
    ../src/query/aql_parser.cpp
    ../src/query/aql_parser_json.cpp
    ../src/query/sql_parser.cpp
    ../src/query/aql_translator.cpp
    ../src/query/aql_runner.cpp
    ../src/query/result_type_annotation.cpp
    ../src/query/query_plan_visualizer.cpp
    ../src/query/let_evaluator.cpp
    ../src/query/window_evaluator.cpp
    ../src/query/cte_subquery.cpp
    ../src/query/cte_cache.cpp
    ../src/query/materialized_cte.cpp
    ../src/query/result_stream.cpp
    ../src/query/query_cache.cpp
    ../src/query/workload_cache_strategy.cpp
    ../src/query/query_cache_manager.cpp
    ../src/query/cross_cluster_federation.cpp
    ../src/query/materialized_cte.cpp
    ../src/query/sparql_parser.cpp
    ../src/query/cypher_parser.cpp
    ../src/query/gremlin_parser.cpp
    ../src/query/parallel_executor.cpp
    ../src/query/query_canceller.cpp
    ../src/query/query_federation.cpp
    ../src/query/plan_cache.cpp
    ../src/query/query_compiler.cpp
    ../src/query/materialized_view.cpp
    # Vectorized Execution Engine – column-store style batch processing (Issue #2434)
    ../src/query/vectorized_execution.cpp
    ../src/query/sparql_parser.cpp
    ../src/performance/cycle_metrics.cpp
    ../src/performance/workload_predictor.cpp
    ../src/performance/async_metrics_exporter.cpp
    ../src/performance/chimera_exporter.cpp
    ../src/performance/prometheus_exporter.cpp
    ../src/performance/phase3/per_query_cost_model.cpp
    ../src/cache/cache_replication.cpp
    ../src/cache/distributed_cache_coordinator.cpp
    ../src/cache/adaptive_query_cache.cpp
    ../src/cache/warmup.cpp
    ../src/cache/cache_hit_rate_slo_monitor.cpp
    ../src/cache/predictive_prefetcher.cpp
    ../src/query/statistical_aggregator.cpp
    ../src/query/semantic_cache.cpp
    ../src/query/functions/function_registry.cpp
    ../src/api/graphql.cpp
    ../src/query/functions/ethics_functions.cpp
    ../src/query/functions/fulltext_functions.cpp
    ../src/query/functions/lora_functions.cpp
    ../src/query/functions/process_mining_functions.cpp
    ../src/query/functions/udf_registry.cpp
    
    # Analytics
    ../src/analytics/olap.cpp
    # Data export: JSON/CSV always available; Arrow/Parquet/Feather when THEMIS_HAS_ARROW
    ../src/analytics/analytics_export.cpp
    ../src/analytics/arrow_export.cpp
    ../src/analytics/process_mining.cpp
    ../src/analytics/process_pattern_matcher.cpp
    ../src/analytics/nlp_text_analyzer.cpp
    ../src/analytics/cep_engine.cpp
    ../src/analytics/streaming_window.cpp
    ../src/analytics/incremental_view.cpp
    ../src/analytics/columnar_execution.cpp
    # Process Modeling Module
    ../src/process/process_model_manager.cpp
    ../src/process/bpmn_serializer.cpp
    ../src/process/epk_serializer.cpp
    ../src/process/llm_process_descriptor.cpp
    ../src/process/vcc_vpb_importer.cpp
    ../src/process/process_linker.cpp
    ../src/process/process_graph_rag.cpp
    ../src/analytics/jit_aggregation.cpp
    ../src/analytics/anomaly_detection.cpp
    ../src/analytics/forecasting.cpp
    ../src/analytics/automl.cpp
    ../src/analytics/ml_serving.cpp

    # Model Serving and Online Inference Pipeline (Issue #1477)
    ../src/analytics/model_serving.cpp
    ../src/analytics/distributed_analytics.cpp

    # Arrow Flight RPC support for remote analytics (Issue #1472)
    ../src/analytics/arrow_flight.cpp
    
    # AQL handlers (non-LLM)
    ../src/aql/aql_query_builder.cpp
    ../src/aql/aql_query_validator.cpp
    ../src/aql/aql_optimizer_advisor.cpp
    ../src/aql/aql_query_template_library.cpp
    ../src/aql/aql_conversation_context.cpp
    ../src/aql/aql_schema_provider.cpp
    ../src/aql/aql_migration_assistant.cpp
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/aql/classify_bridge.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/aql/docs_assistant_functions.cpp>

    # Security: AQL injection detection (uses AQLParser)
    ../src/security/aql_injection_detector.cpp
    
    # NLP features (moved from storage)
    ../src/storage/nlp_metadata_extractor.cpp
    
    # Import/Export
    ../src/exporters/jsonl_llm_exporter.cpp
    ../src/exporters/format_template.cpp
    ../src/exporters/exporter_metrics.cpp
    ../src/exporters/pii_detector.cpp
    ../src/exporters/stream_writer.cpp
    ../src/exporters/parquet_exporter.cpp
    ../src/exporters/streaming_exporter.cpp
    ../src/exporters/aql_predicate_filter.cpp
    ../src/exporters/arrow_ipc_exporter.cpp
    ../src/exporters/incremental_exporter.cpp
    ../src/exporters/export_encryption.cpp
    ../src/exporters/data_augmentation.cpp
    ../src/exporters/huggingface_exporter.cpp
    ../src/exporters/huggingface_exporter.cpp
    ../src/exporters/data_augmentation.cpp
    ../src/exporters/export_format_registry.cpp
    ../src/exporters/join_exporter.cpp
    ../src/exporters/huggingface_hub_client.cpp
    ../src/importers/conflict_resolver.cpp
    $<$<BOOL:${THEMIS_ENABLE_POSTGRES_WIRE}>:../src/importers/postgres_importer.cpp>
    ../src/importers/mysql_importer.cpp
    ../src/importers/mongo_importer.cpp
    ../src/importers/sqlite_importer.cpp
    ../src/importers/flatfile_importer.cpp
    ../src/importers/schema_validator.cpp
    ../src/importers/kafka_importer.cpp
    ../src/importers/oracle_importer.cpp
    ../src/importers/gui_import_wizard.cpp
    $<$<BOOL:${THEMIS_ENABLE_S3}>:../src/importers/s3_importer.cpp>
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
    ../src/importers/deterministic_matcher.cpp
    ../src/importers/semantic_matcher.cpp
    ../src/importers/entity_linker.cpp
    ../src/importers/canonical_resolver.cpp
    ../src/importers/mdm_engine.cpp
    ../src/importers/mdm_audit_trail.cpp
    ../src/importers/mdm_metrics.cpp
    $<$<BOOL:${THEMIS_ENABLE_POSTGRES_WIRE}>:../src/importers/postgres_importer_mdm.cpp>

)

set(THEMIS_SECURITY_SOURCES
    # Encryption and key management
    ../src/security/mock_key_provider.cpp
    ../src/security/vault_key_provider.cpp
    ../src/security/key_cache.cpp
    ../src/security/keyprovider_signing.cpp
    ../src/security/vault_signing_provider.cpp
    ../src/security/malware_scanner.cpp
    ../src/security/usb_admin_authenticator.cpp
    ../src/security/usb_volume_hardening.cpp
    ../src/security/pki_key_provider.cpp
    ../src/security/cms_signing.cpp
    ../src/security/rbac.cpp
    ../src/security/access_control_manager.cpp
    ../src/security/row_level_security.cpp
    ../src/security/access_control.cpp
    ../src/security/zero_trust_policy_enforcer.cpp
    ../src/auth/auth_audit_logger.cpp
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
    ../src/utils/lek_manager.cpp
    ../src/utils/saga_logger.cpp
    
    # Authentication
    ../src/auth/jwt_validator.cpp
    ../src/auth/token_blacklist.cpp
    ../src/auth/redis_token_blacklist.cpp
    ../src/auth/rocksdb_token_blacklist.cpp
    ../src/auth/jwks_validator.cpp
    ../src/auth/gssapi_authenticator.cpp
    ../src/auth/mtls_authenticator.cpp
    ../src/auth/api_key_authenticator.cpp
    ../src/auth/session_manager.cpp
    ../src/auth/ldap_authenticator.cpp
    ../src/auth/ldap_connection_pool.cpp
    ../src/auth/mfa_authenticator.cpp
    ../src/auth/password_policy.cpp
    ../src/auth/oidc_provider.cpp
    ../src/auth/federated_identity_manager.cpp
    ../src/auth/oauth_device_flow.cpp
    ../src/auth/oauth_pkce_flow.cpp
    ../src/auth/saml_authenticator.cpp
    ../src/auth/zero_trust_auth_verifier.cpp
    ../src/auth/webauthn_authenticator.cpp
    ../src/auth/auth_metrics.cpp
    ../src/auth/auth_error.cpp
    ../src/auth/jwks_security.cpp
    ../src/auth/kerberos_security.cpp
    ../src/auth/totp_replay_cache.cpp
    ../src/auth/totp_secret_encryption.cpp
    ../src/auth/jwt_key_rotation_manager.cpp
    ../src/auth/principal_validator.cpp
    ../src/server/auth_middleware.cpp
    ../src/server/request_validation_middleware.cpp
    ../src/server/policy_engine.cpp
    
    # Governance
    ../src/governance/policy_engine.cpp
    ../src/governance/policy_manager.cpp
    ../src/governance/policy_version_history.cpp
    ../src/governance/opa_adapter.cpp
    ../src/governance/data_lineage.cpp
    ../src/governance/ccpa_rules.cpp
    ../src/governance/model_governance.cpp
    ../src/governance/pci_dss_rules.cpp
    ../src/governance/cross_tenant_policy_inheritance.cpp
    ../src/governance/data_masker.cpp
    ../src/governance/policy_template.cpp
    ../src/governance/policy_coordinator.cpp
    ../src/governance/compliance_reporting.cpp
    ../src/governance/policy_validation.cpp
    ../src/governance/policy_manager_versioned.cpp
    ../src/governance/compliance_reporter.cpp
    ../src/governance/policy_validator.cpp
    ../src/governance/review_scheduler.cpp
    ../src/governance/policy_review.cpp
    ../src/governance/policy_file_watcher.cpp
    ../src/governance/soc2_controls.cpp
    ../src/governance/iso27001_rules.cpp
    ../src/governance/hipaa_rules.cpp
    
    # PII detection
    ../src/utils/pii_detection_engine.cpp
    ../src/utils/regex_detection_engine.cpp
    ../src/utils/ner_detection_engine.cpp
    ../src/utils/pii_detector.cpp
    ../src/utils/pii_stream_scanner.cpp
    ../src/utils/utils_adapters.cpp
    ../src/utils/retention_manager.cpp
    ../src/utils/pki_client.cpp
    
    # Post-quantum cryptography (CRYSTALS-Kyber / Dilithium migration path)
    ../src/security/post_quantum_crypto.cpp

    # Advanced security features (Phase 4)
    ../src/security/binary_manifest.cpp
    ../src/security/manifest_signer.cpp
    ../src/security/confidential_computing.cpp
    ../src/security/fips_crypto_mode.cpp
    ../src/security/hsm_signing.cpp
    ../src/security/tsa_api.cpp
    ../src/security/secret_manager.cpp
    ../src/security/security_evidence_collector.cpp

    # Security initialization
    ../src/core/security_initialization.cpp
    
    # Encryption and vector/graph index helpers (use storage + security features)
    ../src/security/field_encryption.cpp
    ../src/security/encrypted_field.cpp
    ../src/utils/audit_logger.cpp
    ../src/storage/index_maintenance.cpp
    ../src/index/vector_index.cpp
    ../src/index/graph_index.cpp
    ../src/index/approximate_radius_search.cpp
    ../src/index/multi_vector_search.cpp
    ../src/index/index_manager.cpp
    ../src/index/vector_auto_buffer.cpp
    # Storage-backed PII and vector index helpers
    ../src/utils/pii_pseudonymizer.cpp
    # ../src/cache/embedding_cache.cpp  # Temporarily disabled - requires mimalloc
    ../src/search/hybrid_search.cpp
    ../src/search/llm_reranker.cpp
    ../src/search/llm_query_rewriter.cpp
    ../src/search/query_expander.cpp
    ../src/search/fuzzy_matcher.cpp
    ../src/search/faceted_search.cpp
    ../src/search/search_analytics.cpp
    ../src/search/autocomplete.cpp
    ../src/search/learning_to_rank.cpp
    ../src/search/multi_modal_search.cpp
    ../src/search/personalized_ranker.cpp
    ../src/search/multi_field_search.cpp
    ../src/search/neural_sparse_retrieval.cpp
    ../src/search/search_highlighter.cpp
    ../src/search/cross_lingual_search.cpp
    ../src/search/negative_keyword_filter.cpp
)

set(THEMIS_TRANSACTION_SOURCES
    # Transaction management
    ../src/transaction/transaction_manager.cpp
    ../src/transaction/lock_manager.cpp
    ../src/transaction/crash_recovery_manager.cpp
    ../src/transaction/saga.cpp
    ../src/transaction/distributed_saga.cpp
    ../src/transaction/saga_orchestrator.cpp
    ../src/transaction/snapshot_manager.cpp
    ../src/transaction/branch_manager.cpp
    ../src/transaction/merge_engine.cpp
    ../src/transaction/deadlock_predictor.cpp
    ../src/transaction/transaction_batcher.cpp
    ../src/transaction/transaction_auditor.cpp
    ../src/analytics/diff_engine.cpp
    
    # Temporal conflict resolution and production-readiness modules
    ../src/temporal/temporal_conflict_resolver.cpp
    ../src/temporal/system_versioned_table.cpp
    ../src/temporal/temporal_query_engine.cpp
    ../src/temporal/temporal_index.cpp
    ../src/temporal/retention_manager.cpp
    ../src/temporal/bi_temporal.cpp
    ../src/temporal/snapshot_manager.cpp
    ../src/temporal/temporal_aggregator.cpp
    ../src/temporal/bitemporal_join.cpp
    ../src/temporal/interval_tree_index.cpp
    ../src/temporal/temporal_compressor.cpp
    ../src/temporal/temporal_cdc.cpp
    
    # Replication
    ../src/replication/replication_manager.cpp
    ../src/replication/observability.cpp
    ../src/replication/conflict_resolution.cpp
    ../src/replication/event_stream.cpp
    ../src/replication/policy.cpp
    ../src/replication/replication_slot.cpp
    ../src/replication/raft_v2.cpp
    ../src/replication/schema_cdc.cpp
    ../src/replication/multi_tier_replication.cpp
    ../src/replication/logical_replication.cpp
    
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
    ../src/transaction/global_transaction_manager.cpp
    ../src/search/distributed_hybrid_search.cpp
    ../src/sharding/shard_resource_manager.cpp
    ../src/sharding/locality_aware_router.cpp
    ../src/sharding/adaptive_shard_router.cpp
    ../src/sharding/capability_matcher.cpp
    ../src/utils/capability_auto_generator.cpp
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
    ../src/sharding/two_phase_commit_participant.cpp
    ../src/sharding/two_phase_commit_coordinator.cpp
    ../src/sharding/consensus_factory.cpp
    ../src/sharding/raft_consensus_adapter.cpp
    ../src/sharding/gossip_consensus_adapter.cpp
    ../src/sharding/paxos_consensus.cpp
    ../src/sharding/paxos_wal.cpp
    ../src/sharding/paxos_snapshot.cpp
    ../src/sharding/paxos_state_persistence.cpp
    ../src/sharding/cross_shard_transaction.cpp
    ../src/sharding/transaction_wal.cpp
    ../src/sharding/transaction_snapshot.cpp

    # Redundancy and reliability
    ../src/sharding/hardware_migration_manager.cpp
    ../src/sharding/redundancy_strategy.cpp
    ../src/sharding/hot_spare_manager.cpp
    ../src/sharding/predictive_detector.cpp
    ../src/sharding/shard_rpc_server.cpp
    ../src/sharding/cloud_backup.cpp
    ../src/sharding/orphan_detector.cpp

    # GPU erasure coding (conditional)
    $<$<BOOL:${THEMIS_ENABLE_CUDA}>:../src/sharding/gpu_erasure_coder.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CUDA}>:../src/sharding/gpu_erasure_coder.cu>
    $<$<BOOL:${THEMIS_ENABLE_OPENCL}>:../src/sharding/gpu_erasure_coder_opencl.cpp>

    # Enhanced sharding features
    ../src/sharding/shard_durability.cpp
    ../src/sharding/operational_metrics.cpp
    ../src/sharding/admin_operations.cpp
    ../src/sharding/sharding_manager_edition.cpp
    ../src/sharding/slo_monitor.cpp
    ../src/sharding/cloud_backup.cpp
    ../src/sharding/hardware_migration_manager.cpp
    ../src/sharding/orphan_detector.cpp
    ../src/sharding/sharding_manager_edition.cpp
    ../src/sharding/two_phase_commit_coordinator.cpp
    ../src/sharding/two_phase_commit_participant.cpp

    # Phase 4.1 — Epoch-based fencing + lease management
    ../src/sharding/epoch_fencing.cpp
)

set(THEMIS_LLM_SOURCES
    # LLM core components
    ../src/llm/llm_interaction_store.cpp
    ../src/llm/llm_response_cache.cpp
    # Prompt Engineering Module (all components)
    ../src/prompt_engineering/prompt_manager.cpp
    ../src/prompt_engineering/prompt_engineering_metrics.cpp
    ../src/prompt_engineering/feedback_collector.cpp
    ../src/prompt_engineering/prompt_evaluator.cpp
    ../src/prompt_engineering/meta_prompt_generator.cpp
    ../src/prompt_engineering/prompt_optimizer.cpp
    ../src/prompt_engineering/prompt_performance_tracker.cpp
    ../src/prompt_engineering/prompt_version_control.cpp
    ../src/prompt_engineering/self_improvement_orchestrator.cpp
    ../src/prompt_engineering/prompt_engineering_integration.cpp
    ../src/prompt_engineering/prompt_injection_detector.cpp
    ../src/prompt_engineering/chain_of_thought.cpp
    ../src/prompt_engineering/rag_prompt_builder.cpp
    ../src/prompt_engineering/system_prompt_manager.cpp
    ../src/prompt_engineering/context_window_manager.cpp
    ../src/prompt_engineering/reflection_tuner.cpp
    ../src/prompt_engineering/llm_reflection_adapter.cpp
    ../src/prompt_engineering/cot_tracer.cpp
    ../src/prompt_engineering/prompt_regression_runner.cpp
    ../src/prompt_engineering/prompt_ab_experiment.cpp
    ../src/prompt_engineering/prompt_library_io.cpp
    ../src/prompt_engineering/tree_of_thoughts.cpp
    ../src/prompt_engineering/protegi_optimizer.cpp
    ../src/prompt_engineering/dspy_module.cpp
    ../src/llm/block_table.cpp
    ../src/llm/paged_block_manager.cpp
    ../src/llm/paged_kv_cache.cpp
    ../src/llm/paged_kv_cache_manager.cpp
    ../src/llm/llm_plugin_manager.cpp
    ../src/llm/model_loader.cpp
    ../src/llm/model_quantization_pipeline.cpp
    ../src/llm/model_downloader.cpp
    ../src/llm/aql_train_parser.cpp
    ../src/llm/llama_wrapper.cpp
    ../src/llm/llama_lora_adapter.cpp
    ../src/llm/llama_grammar_adapter.cpp
    ../src/llm/llamacpp_inference_engine.cpp
    ../src/llm/shared_worker_pool.cpp
    ../src/llm/async_inference_engine.cpp
    ../src/llm/prompt_policy.cpp
    ../src/llm/speculative_decoder.cpp
    ../src/llm/model_router.cpp
    ../src/llm/inference_engine_enhanced.cpp
    ../src/llm/streaming_handler.cpp
    ../src/llm/openai_compat_adapter.cpp
    ../src/llm/embedded_llm.cpp
    ../src/llm/ethical_guidelines_manager.cpp
    ../src/llm/constitutional_reasoning_engine.cpp
    ../src/llm/ethics_aware_confidence_detector.cpp
    ../src/llm/ai_decision_auditor.cpp
    ../src/llm/moral_analyzer.cpp
    ../src/llm/multi_perspective_generator.cpp
    ../src/llm/meta_prompt_generator.cpp
    ../src/llm/prompt_evaluator.cpp
    ../src/llm/prompt_optimizer.cpp
    ../src/llm/inference_handle.cpp
    ../src/llm/llm_security_utils.cpp
    ../src/llm/vision_resource_monitor.cpp
    ../src/llm/docs_assistant.cpp
    ../src/llm/applications/themis_help_lora.cpp
    ../src/llm/feedback_store.cpp
    ../src/llm/llm_model_storage.cpp
    ../src/llm/llm_model_audit_logger.cpp
    ../src/llm/lora_framework/lora_audit_logger.cpp
    ../src/llm/kv_cache_buffer.cpp
    ../src/llm/multi_lora_manager.cpp
    ../src/llm/gguf_loader.cpp
    ../src/llm/grammar.cpp
    ../src/llm/grammar_cache.cpp
    ../src/llm/json_schema_converter.cpp
    ../src/llm/llm_prefix_cache.cpp
    ../src/llm/continuous_batch_scheduler.cpp
    ../src/llm/mixed_precision_inference.cpp
    ../src/llm/adaptive_vram_allocator.cpp
    ../src/llm/active_vram_allocator.cpp
    ../src/llm/multi_gpu_memory_coordinator.cpp
    ../src/llm/gpu_safe_fail.cpp
    ../src/llm/token_quota_manager.cpp
    ../src/llm/grafana_metrics.cpp
    ../src/llm/distributed_training_coordinator.cpp
    # Inference & Sampling core
    ../src/llm/inference_handle.cpp
    ../src/llm/sampling_strategy.cpp
    # Ethics, AI Safety & Multi-Perspective
    ../src/llm/constitutional_reasoning_engine.cpp
    ../src/llm/ethics_aware_confidence_detector.cpp
    ../src/llm/moral_analyzer.cpp
    ../src/llm/multi_perspective_generator.cpp
    # Feedback & Security
    ../src/llm/feedback_plugin_basic.cpp
    ../src/llm/llm_security_utils.cpp
    # Vision resource monitoring
    ../src/llm/vision_resource_monitor.cpp
    # LoRA framework additions (unconditional)
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/llm/lora_framework/distributed_dataloader.cpp>
    ../src/llm/lora_framework/kernels/cpu_fused_kernels.cpp
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/llm/lora_framework/paged_optimizer.cpp>
        ../src/cache/embedding_cache.cpp
        ../src/llm/lora_framework/lora_layers.cpp
    
    # LoRA framework (core subset)
    ../src/llm/lora_framework/lora_orchestrator.cpp
    ../src/llm/lora_framework/lora_feedback_storage.cpp
    ../src/llm/lora_framework/lora_training_config.cpp
    ../src/llm/lora_framework/feedback_plugin.cpp
    ../src/llm/lora_framework/lora_provenance.cpp
    ../src/llm/lora_framework/lora_storage_service.cpp
    ../src/llm/lora_framework/lora_checkpoint_manager.cpp
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
    ../src/rag/llm_integration.cpp
    ../src/rag/llm_judge_client.cpp
    ../src/rag/llm_judge_integration.cpp
    ../src/rag/nli_faithfulness_verifier.cpp
    ../src/rag/quality_control_pipeline.cpp
    ../src/rag/prompt_templates.cpp
    ../src/rag/response_parser.cpp
    ../src/training/lora_data_selection.cpp
    ../src/rag/faithfulness_evaluator.cpp
    ../src/rag/relevance_evaluator.cpp
    ../src/rag/completeness_evaluator.cpp
    ../src/rag/coherence_evaluator.cpp
    ../src/rag/geval_evaluator.cpp
    ../src/rag/rag_judge.cpp
    ../src/rag/reranker.cpp
    ../src/rag/document_summarizer.cpp
    ../src/rag/document_splitter.cpp
    ../src/rag/hybrid_retriever.cpp
    ../src/rag/citation_highlighter.cpp
    # Phase 5: Distributed evaluation and security
    ../src/rag/distributed_rag_evaluator.cpp
    ../src/rag/prompt_injection_detector.cpp
    # Phase 5: Evaluation pipeline (report, cache, calibration, batch, bias)
    ../src/rag/evaluation_report_exporter.cpp
    ../src/rag/evaluation_cache.cpp
    ../src/rag/calibration_manager.cpp
    ../src/rag/batch_evaluator.cpp
    ../src/rag/bias_detector.cpp
    ../src/rag/adversarial_tester.cpp

    # LLM-owned AQL support files
    ../src/aql/llm_aql_handler.cpp
    ../src/aql/aql_query_validator.cpp
    ../src/aql/aql_query_builder.cpp
    ../src/aql/aql_schema_provider.cpp
    ../src/aql/aql_fewshot_example_library.cpp
    ../src/aql/aql_syntax_highlighter.cpp
    ../src/aql/aql_confidence_scorer.cpp
    ../src/aql/llm_metrics_collector.cpp
    # Phase 4: Multi-modal RAG (image + text retrieval)
    ../src/rag/multimodal_rag.cpp
    # Phase 1–4: Missing RAG evaluators and orchestrators
    ../src/rag/ab_testing_framework.cpp
    ../src/rag/agentic_rag.cpp
    ../src/rag/bayesian_optimizer.cpp
    ../src/rag/claim_extractor.cpp
    ../src/rag/coherence_evaluator.cpp
    ../src/rag/completeness_evaluator.cpp
    ../src/rag/continuous_learning_client.cpp
    ../src/rag/continuous_learning_orchestrator.cpp
    ../src/rag/cot_evaluator.cpp
    ../src/rag/faithfulness_evaluator.cpp
    ../src/rag/hallucination_dashboard.cpp
    ../src/rag/http_metrics_client.cpp
    ../src/rag/judge_config.cpp
    ../src/rag/judge_ensemble.cpp
    ../src/rag/knowledge_graph_retriever.cpp
    ../src/rag/learning_metrics.cpp
    ../src/rag/llm_judge_integration.cpp
    ../src/rag/llm_meta_analyzer.cpp
    ../src/rag/onnx_model_loader.cpp
    ../src/rag/pairwise_comparator.cpp
    ../src/rag/prompt_templates.cpp
    ../src/rag/quality_control_factory.cpp
    ../src/rag/rag_judge.cpp
    ../src/rag/relevance_evaluator.cpp
    ../src/rag/response_parser.cpp
    ../src/rag/rubric_evaluator.cpp
    ../src/rag/streaming_retriever.cpp
    
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

if(THEMIS_ENABLE_HIP)
    list(APPEND THEMIS_LLM_SOURCES
        ../src/llm/lora_framework/kernels/hip_fused_kernels.cpp
    )
endif()

if(THEMIS_ENABLE_VULKAN)
    list(APPEND THEMIS_LLM_SOURCES
        ../src/llm/lora_framework/vulkan_buffer.cpp
        ../src/llm/lora_framework/vulkan_context.cpp
        ../src/llm/lora_framework/vulkan_pipeline.cpp
    )
endif()

set(THEMIS_CONTENT_SOURCES
    # Content processing (conditional)
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/content_type.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/content_manager.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/html_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/text_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/markdown_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/content_validator.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/deduplication_checker.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/image_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/mock_clip_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/mime_detector.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/content_policy.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/content_metrics.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/content_errors.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/content_fs.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/version_manager.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/embedding_pipeline.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/language_detector.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/content_manager_embedding.cpp>
    $<$<AND:$<BOOL:${THEMIS_ENABLE_CONTENT}>,$<BOOL:${THEMIS_ENABLE_OFFICE}>>:../src/content/office_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/pdf_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/ocr_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/archive_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/async_ingestion_worker.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/ingestion_plugin.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/audio_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/video_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/geo_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/cad_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/content_logger.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/content_security.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/abuse_detector.cpp>
    $<$<AND:$<BOOL:${THEMIS_ENABLE_CONTENT}>,$<NOT:$<BOOL:${THEMIS_ENABLE_VOICE_ASSISTANT}>>>:../src/content/stt_processor.cpp>
    $<$<AND:$<BOOL:${THEMIS_ENABLE_CONTENT}>,$<NOT:$<BOOL:${THEMIS_ENABLE_VOICE_ASSISTANT}>>>:../src/content/tts_processor.cpp>
    $<$<AND:$<BOOL:${THEMIS_ENABLE_CONTENT}>,$<BOOL:${THEMIS_ENABLE_LLM}>>:../src/content/content_manager_llm.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/pipeline/zstd_compression.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/pipeline/content_chunker.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/pipeline/bulk_upload_interface.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/pipeline/async_bulk_uploader.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/pipeline/multimodal_chunker.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/plugins/huggingface_ingestion_plugin.cpp>
)

set(THEMIS_TIMESERIES_SOURCES
    # Time-series storage
    ../src/timeseries/timeseries.cpp
    ../src/timeseries/tsstore.cpp
    ../src/timeseries/adaptive_flush_controller.cpp
    ../src/timeseries/gorilla.cpp
    ../src/timeseries/gorilla_simd.cpp
    ../src/timeseries/retention.cpp
    ../src/timeseries/continuous_agg.cpp
    ../src/timeseries/aggregate_scheduler.cpp
    ../src/timeseries/aggregate_scheduler_helper.cpp
    ../src/timeseries/query_optimizer.cpp
    ../src/timeseries/timeseries_metrics.cpp
    ../src/timeseries/prometheus_remote_write.cpp
    ../src/timeseries/ts_auto_buffer.cpp
    ../src/timeseries/hypertable.cpp
    ../src/timeseries/aggregates.cpp
    ../src/timeseries/downsampling.cpp
    ../src/timeseries/ts_auto_buffer_adaptive.cpp
    ../src/timeseries/encrypted_chunk_store.cpp
    ../src/timeseries/ts_encrypted_key_rotation.cpp
)

set(THEMIS_INGESTION_SOURCES
    # Ingestion module – unified data intake layer
    ../src/ingestion/ingestion_manager.cpp
    ../src/ingestion/filesystem_ingester.cpp
    ../src/ingestion/api_connector.cpp
    ../src/ingestion/huggingface_connector.cpp
    ../src/ingestion/kafka_connector.cpp
    ../src/ingestion/s3_connector.cpp
    ../src/ingestion/object_storage_connector.cpp
    ../src/ingestion/database_connector.cpp
    ../src/ingestion/web_crawler_connector.cpp
    ../src/ingestion/ingestion_coordinator.cpp
    # cdc_connector.cpp uses #ifdef THEMIS_ENABLE_CDC_STREAM internally; always compile.
    ../src/ingestion/cdc_connector.cpp
    # Legal ingestion pipeline: deontic extraction, semantic validation, reference validation
    ../src/ingestion/deontic_extractor.cpp
    ../src/ingestion/semantic_validator.cpp
    ../src/ingestion/agentic_reference_validator.cpp
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/ingestion/llm_adapter.cpp>
)

set(THEMIS_NETWORK_SOURCES
    # HTTP Server (conditional)
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/http_server.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/transaction_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/distributed_txn_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/auth_middleware.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/api_auth_config.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/api_security_audit.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/session_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/saml_auth_provider.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/oauth2_provider.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/opa_adapter.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/request_validation_middleware.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/http_type_adapter.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/chunked_response_writer.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/health_error_service.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/error_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/serverless_function_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/udf_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/async_job_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/task_scheduler_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/scheduler/distributed_task_coordinator.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/scheduler/event_trigger.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/scheduler/external_scheduler_adapter.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/scheduler/hybrid_retention_manager.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/scheduler/task_anomaly_detector.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/scheduler/task_audit_event.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/scheduler/task_audit_manager.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/scheduler/task_result_store.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/scheduler/task_scheduler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/branch_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/merge_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/diff_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/rope_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/bpmn_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/geo_topology_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/replication_topology_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/cache_admin_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/api_key_mgmt_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/snapshot_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/pitr_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/storage/pitr_manager.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/mvcc_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/cdc/cdc_admin.cpp>
    $<$<AND:$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>,$<BOOL:${THEMIS_ENABLE_LLM}>>:../src/server/feedback_api_handler.cpp>
    # Maintenance Orchestrator (always compiled when HTTP server is on)
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/maintenance/database_maintenance_orchestrator.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/maintenance/maintenance_schedule_store.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/maintenance/maintenance_registry.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/maintenance_api_handler.cpp>

    # API handlers (always included)
    ../src/server/cache_api_handler.cpp
    ../src/server/admin_api_handler.cpp
    ../src/server/vector_api_handler.cpp
    ../src/server/spatial_api_handler.cpp
    ../src/server/openapi_route_registry.cpp
    ../src/server/monitoring_api_handler.cpp
    ../src/server/query_api_handler.cpp
    ../src/server/policy_api_handler.cpp
    ../src/server/prompt_api_handler.cpp
    ../src/server/graph_api_handler.cpp
    ../src/server/index_api_handler.cpp
    ../src/server/timeseries_api_handler.cpp
    ../src/server/entity_api_handler.cpp
    $<$<AND:$<BOOL:${THEMIS_ENABLE_CONTENT}>,$<BOOL:${THEMIS_MODULE_CONTENT}>>:../src/server/content_api_handler.cpp>
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
    ../src/security/query_masking_policy.cpp
    ../src/server/rate_limiter.cpp
    ../src/server/rate_limiter_v2.cpp
    ../src/server/adaptive_rate_limiter.cpp
    ../src/server/cost_based_rate_limiter.cpp
    ../src/server/rate_limiting_middleware.cpp
    ../src/server/load_shedder.cpp
    ../src/auth/auth_rate_limiter.cpp
    ../src/server/api_version.cpp
    ../src/server/response_transformer.cpp
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/api_gateway.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/distributed_gateway.cpp>
    ../src/server/request_coalescing.cpp
    ../src/server/smart_routing.cpp
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
    $<$<BOOL:${THEMIS_ENABLE_HTTP3}>:../src/server/http3_datagram.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP3}>:../src/server/http3_production_config.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/cdn_cache_middleware.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/import_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/import_wizard_builder.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/profiling_api_handler.cpp>
    $<$<AND:$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>,$<BOOL:${THEMIS_ENABLE_LLM}>>:../src/server/ethics_api_handler.cpp>
    $<$<AND:$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>,$<BOOL:${THEMIS_ENABLE_VOICE_ASSISTANT}>>:../src/server/voice_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GRPC}>:../src/server/prompt_engineering_grpc_service.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GRPC}>:../src/server/themis_core_grpc_service.cpp>
    $<$<BOOL:${THEMIS_ENABLE_WEBSOCKET}>:../src/server/websocket_session.cpp>
    $<$<BOOL:${THEMIS_ENABLE_MQTT}>:../src/server/mqtt_session.cpp>
    $<$<BOOL:${THEMIS_ENABLE_MQTT}>:../src/server/mqtt_client_service.cpp>
    $<$<BOOL:${THEMIS_ENABLE_POSTGRES_WIRE}>:../src/server/postgres_session.cpp>
    $<$<BOOL:${THEMIS_ENABLE_MCP}>:../src/server/mcp_server.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GRPC}>:../src/server/grpc_web_proxy_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_SERVICE_MESH}>:../src/server/service_mesh_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/wasm_handler_registry.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP3}>:../src/server/http3_datagram.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GRPC}>:../src/server/prompt_engineering_grpc_service.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GRPC}>:../src/server/themis_core_grpc_service.cpp>
    $<$<BOOL:${THEMIS_ENABLE_VOICE_ASSISTANT}>:../src/server/voice_api_handler.cpp>
    ../src/server/cdn_cache_middleware.cpp
    ../src/server/ethics_api_handler.cpp
    ../src/server/import_api_handler.cpp
    ../src/server/import_wizard_builder.cpp
    ../src/server/profiling_api_handler.cpp
    ../src/server/buffer_api_handler.cpp
    ../src/server/buffer_binary_protocol.cpp
    ../src/server/compliance_reporting_api_handler.cpp
    ../src/server/policy_manager_api_handler.cpp
    ../src/server/policy_template_api_handler.cpp
    ../src/server/policy_validation_api_handler.cpp
    ../src/server/policy_versioning_api_handler.cpp
    ../src/server/prompt_engineering_api_handler.cpp
    ../src/server/review_scheduling_api_handler.cpp
    
    # GraphQL API (conditional)
    $<$<BOOL:${THEMIS_ENABLE_GRAPHQL}>:../src/api/graphql.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GRAPHQL}>:../src/server/graphql_api_handler.cpp>

    # WebSocket change-stream handler (conditional)
    $<$<BOOL:${THEMIS_ENABLE_WEBSOCKET}>:../src/api/ws_handler.cpp>

    # gRPC API server alongside REST (conditional)
    $<$<BOOL:${THEMIS_ENABLE_GRPC}>:../src/api/grpc_server.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GRPC}>:../src/api/themisdb_grpc_service.cpp>
    
    # Network protocol server (themis::network – backward-compatible implementation)
    ../src/network/wire_protocol_server.cpp
    # Themis core module wire protocol (themis::wire – Phase-3 modular implementation)
    ../src/themis/wire_protocol_server.cpp
    ../src/network/qos_manager.cpp
    ../src/network/raft_load_balancer.cpp
    ../src/network/wire_protocol_helpers.cpp
    ../src/network/wire_protocol_connection_pool.cpp
    ../src/network/wire_protocol_v2.cpp
    ../src/network/wire_protocol_performance.cpp
    ../src/network/wire_protocol_zero_copy.cpp
    ../src/network/wire_protocol_batch.cpp
    ../src/network/connection_compression.cpp
    $<$<BOOL:${THEMIS_ENABLE_HTTP3}>:../src/network/quic_transport.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GRPC}>:../src/network/grpc_transport.cpp>
    ../src/network/geo_topology_router.cpp
    ../src/network/socket_timeout_manager.cpp
    ../src/network/adaptive_circuit_breaker.cpp
    ../src/network/udp_fast_path.cpp
    $<$<BOOL:${THEMIS_ENABLE_WEBSOCKET}>:../src/network/wire_protocol_server_ws.cpp>
    $<$<BOOL:${THEMIS_ENABLE_SERVICE_MESH}>:../src/network/service_mesh.cpp>
    $<$<BOOL:${THEMIS_ENABLE_SERVICE_MESH}>:../src/network/envoy_xds.cpp>

    # Modular globals shared across handlers
    ../src/server/hsm_provider_global.cpp
    
    # Observability (GAP-008: Alertmanager integration + full stack)
    ../src/observability/alertmanager.cpp
    # Observability: continuous profiling (pprof / async-profiler compatible)
    ../src/observability/continuous_profiler.cpp
    # Observability: eBPF-based kernel-level tracing (Issue #2055)
    ../src/observability/ebpf_tracer.cpp
    # Observability: distributed flame graph generation (Issue #2108)
    ../src/observability/distributed_flame_graph.cpp
    # Observability: SLO/SLA compliance reporting with burn-rate alerts (Issue #2148)
    ../src/observability/slo_reporter.cpp
    # Observability: ML-based anomaly detection on metric time-series (Issue #2097)
    ../src/observability/metric_anomaly_detector.cpp
    # Observability: ML anomaly detector (forecasting + change-point + outlier) (Issue #83)
    ../src/observability/ml_anomaly_detector.cpp
    # Observability: query, storage, and performance profiling
    ../src/observability/query_profiler.cpp
    ../src/observability/storage_profiler.cpp
    ../src/observability/performance_analyzer.cpp
    # Observability: standalone span management and structured log aggregation (OBS-MISSING-001)
    ../src/observability/tracer.cpp
    ../src/observability/log_aggregator.cpp
    # Observability: rule-based alerting engine with configurable notification channels
    ../src/observability/alerting_engine.cpp
    # Observability: Prometheus advanced — rate calculation, histogram aggregation, cardinality
    ../src/observability/metric_aggregator.cpp
    # Observability: real-time metric streaming via WebSocket / SSE (v1.6.0, Issue #82)
    ../src/observability/metrics_stream_server.cpp
    # Observability: custom metric types — summary, exponential histogram, cardinality, TWA, rate (v1.6.0)
    ../src/observability/advanced_metrics.cpp
    # Observability: OpenTelemetry Full Integration (v1.6.0)
    ../src/observability/opentelemetry_tracer.cpp
    ../src/observability/root_cause_analyzer.cpp
)

set(THEMIS_GEO_SOURCES
    # Geospatial processing
    ../src/acceleration/geo_acceleration_bridge.cpp
    ../src/index/spatial_index.cpp
    ../src/api/geo_index_hooks.cpp
    ../src/geo/cpu_backend.cpp
    ../src/geo/device_detector.cpp
    ../src/geo/gpu_backend_stub.cpp
    ../src/geo/boost_cpu_exact_backend.cpp
    ../src/geo/geo_rtree.cpp
    ../src/geo/geo_clustering.cpp
    ../src/geo/spatial_join.cpp
    ../src/geo/raster.cpp
    ../src/geo/temporal_spatial_query.cpp
    ../src/geo/tile_server.cpp
    ../src/geo/gpu_backend_production.cpp
    # GPU module sources unconditionally required by the geo backend
    ../src/gpu/device_discovery.cpp
    ../src/gpu/safe_fail.cpp
    ../src/gpu/metrics.cpp
    ../src/gpu/audit_log.cpp
)

# Full GPU module sources (gated on THEMIS_ENABLE_GPU)
if(THEMIS_ENABLE_GPU)
    list(APPEND THEMIS_GEO_SOURCES
        ../src/gpu/gpu_module.cpp
        ../src/gpu/config.cpp
        ../src/gpu/feature_flags.cpp
        ../src/gpu/gpu_memory_manager_edition.cpp
        ../src/gpu/memory_pool.cpp
        ../src/gpu/kernel_validator.cpp
        ../src/gpu/policy.cpp
        ../src/gpu/alerts.cpp
        ../src/gpu/launcher.cpp
        ../src/gpu/stream_manager.cpp
        ../src/gpu/load_balancer.cpp
        ../src/gpu/tensor_buffer.cpp
        ../src/gpu/query_accelerator.cpp
        ../src/gpu/graph_cache.cpp
        ../src/gpu/training_loop.cpp
        ../src/gpu/rocm_backend.cpp
        ../src/gpu/cluster_topology.cpp
        ../src/gpu/cluster_coordinator.cpp
        ../src/gpu/profiler.cpp
        ../src/gpu/unified_memory.cpp
        ../src/gpu/time_slice_scheduler.cpp
        ../src/gpu/wasm_kernel_sandbox.cpp
        ../src/gpu/mig_manager.cpp
        ../src/gpu/p2p_transfer.cpp
        ../src/gpu/vulkan_backend.cpp
        ../src/gpu/admin_api.cpp
    )
endif()

# CUDA kernel dispatch for geo GPU backend (THEMIS_GEO_CUDA=ON)
if(THEMIS_GEO_CUDA)
    list(APPEND THEMIS_GEO_SOURCES ../src/geo/gpu_backend_cuda.cu)
elseif(THEMIS_GEO_HIP)
    list(APPEND THEMIS_GEO_SOURCES ../src/geo/gpu_backend_hip.cpp)
else()
    list(APPEND THEMIS_GEO_SOURCES ../src/geo/gpu_kernel_dispatcher_cpu.cpp)
endif()

set(THEMIS_GRAPH_SOURCES
    # Graph indexes and analytics
    ../src/index/graph_auto_buffer.cpp
    ../src/index/spatial_index.cpp
    ../src/index/temporal_graph.cpp
    ../src/index/property_graph.cpp
    ../src/index/edge_types.cpp
    ../src/index/process_graph.cpp
    ../src/index/gnn_embeddings.cpp
    ../src/index/graph_analytics.cpp
    ../src/graph/graph_query_optimizer.cpp
    ../src/query/result_stream.cpp
    ../src/graph/path_constraints.cpp
    ../src/graph/distributed_graph.cpp
    ../src/graph/gpu_traversal.cpp
    ../src/graph/parallel_traversal.cpp
    ../src/graph/scheduled_edge_refresh.cpp
)

# Function to build modular architecture (post-v1.3.0)
function(themis_build_modular)
    message(STATUS "Building ThemisDB with modular architecture")
    if(THEMIS_ENABLE_CONTENT AND NOT THEMIS_MODULE_CONTENT)
        set(THEMIS_MODULE_CONTENT ON CACHE BOOL "Include content processors module (optional)" FORCE)
        message(STATUS "THEMIS_ENABLE_CONTENT is ON -> forcing THEMIS_MODULE_CONTENT=ON for modular consistency")
    endif()
    
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
    if(TARGET prometheus-cpp::core)
        list(APPEND _themis_base_deps prometheus-cpp::core)
    endif()
    if(TARGET prometheus-cpp::pull)
        list(APPEND _themis_base_deps prometheus-cpp::pull)
    endif()
    if(TARGET libzip::zip)
        list(APPEND _themis_base_deps libzip::zip)
        list(APPEND _themis_base_compile_defs THEMIS_HAVE_LIBZIP)
    elseif(TARGET libzip::libzip)
        list(APPEND _themis_base_deps libzip::libzip)
        list(APPEND _themis_base_compile_defs THEMIS_HAVE_LIBZIP)
    endif()

    themis_add_module(base
        SOURCES ${THEMIS_BASE_SOURCES}
        DEPENDENCIES ${_themis_base_deps}
    )

    if(_themis_base_compile_defs)
        target_compile_definitions(themis_base PRIVATE ${_themis_base_compile_defs})
    endif()

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
        # Ensure pugixml is found before checking for its targets
        # (this module may be included before find_package(pugixml) is called in CMakeLists.txt)
        if(NOT TARGET pugixml::shared AND NOT TARGET pugixml::pugixml)
            find_package(pugixml CONFIG QUIET)
        endif()
    if(TARGET TBB::tbb)
        list(APPEND _themis_security_deps TBB::tbb)
    endif()
    if(TARGET CURL::libcurl)
        list(APPEND _themis_security_deps CURL::libcurl)
    endif()
    if(THEMIS_ENABLE_MIMALLOC AND TARGET mimalloc)
        list(APPEND _themis_security_deps mimalloc)
    endif()
    if(TARGET prometheus-cpp::core)
        list(APPEND _themis_security_deps prometheus-cpp::core)
    endif()
    if(TARGET prometheus-cpp::pull)
        list(APPEND _themis_security_deps prometheus-cpp::pull)
    endif()
    if(TARGET prometheus-cpp::push)
        list(APPEND _themis_security_deps prometheus-cpp::push)
    endif()
    if(TARGET prometheus-cpp::util)
        list(APPEND _themis_security_deps prometheus-cpp::util)
    endif()
    if(THEMIS_ENABLE_JEMALLOC)
        if(TARGET jemalloc::jemalloc)
            list(APPEND _themis_security_deps jemalloc::jemalloc)
        elseif(jemalloc_LIBRARIES)
            list(APPEND _themis_security_deps ${jemalloc_LIBRARIES})
        endif()
    endif()
    if(WIN32)
        list(APPEND _themis_security_deps Secur32 Wldap32)
    endif()
    if(TARGET pugixml::shared)
        list(APPEND _themis_security_deps pugixml::shared)
    elseif(TARGET pugixml::pugixml)
        list(APPEND _themis_security_deps pugixml::pugixml)
    elseif(TARGET unofficial::pugixml::pugixml)
        list(APPEND _themis_security_deps unofficial::pugixml::pugixml)
    elseif(TARGET pugixml::static)
        list(APPEND _themis_security_deps pugixml::static)
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
        themis_security
    )

    # Arrow / Parquet / Arrow Flight targets for modular query build
    if(TARGET Arrow::arrow_shared)
        list(APPEND _themis_query_deps Arrow::arrow_shared)
    elseif(TARGET Arrow::arrow_static)
        list(APPEND _themis_query_deps Arrow::arrow_static)
    elseif(TARGET arrow_shared)
        list(APPEND _themis_query_deps arrow_shared)
    elseif(TARGET arrow_static)
        list(APPEND _themis_query_deps arrow_static)
    endif()

    if(TARGET Parquet::parquet_shared)
        list(APPEND _themis_query_deps Parquet::parquet_shared)
    elseif(TARGET Parquet::parquet_static)
        list(APPEND _themis_query_deps Parquet::parquet_static)
    elseif(TARGET Parquet::parquet)
        list(APPEND _themis_query_deps Parquet::parquet)
    elseif(TARGET parquet_shared)
        list(APPEND _themis_query_deps parquet_shared)
    elseif(TARGET parquet_static)
        list(APPEND _themis_query_deps parquet_static)
    elseif(TARGET parquet)
        list(APPEND _themis_query_deps parquet)
    endif()

    if(TARGET Arrow::arrow_flight_shared)
        list(APPEND _themis_query_deps Arrow::arrow_flight_shared)
    elseif(TARGET Arrow::arrow_flight_static)
        list(APPEND _themis_query_deps Arrow::arrow_flight_static)
    elseif(TARGET arrow_flight_shared)
        list(APPEND _themis_query_deps arrow_flight_shared)
    elseif(TARGET arrow_flight_static)
        list(APPEND _themis_query_deps arrow_flight_static)
    endif()
    if(THEMIS_MODULE_LLM)
        list(APPEND _themis_query_deps themis_llm)
    endif()
    if(THEMIS_MODULE_GEO)
        list(APPEND _themis_query_deps themis_geo)
    endif()
    if(onnxruntime_FOUND)
        list(APPEND _themis_query_deps onnxruntime::onnxruntime)
    endif()
    
    themis_add_module(query
        SOURCES ${THEMIS_QUERY_SOURCES}
        DEPENDENCIES ${_themis_query_deps}
        STATIC_MODULE
    )
    
    set(_themis_network_deps
        themis_base
        themis_storage
        themis_query
        themis_transaction
        themis_security
    )
    if(THEMIS_MODULE_SHARDING)
        list(APPEND _themis_network_deps themis_sharding)
    endif()
    if(THEMIS_MODULE_LLM)
        list(APPEND _themis_network_deps themis_llm)
    endif()
    if(THEMIS_MODULE_TIMESERIES)
        list(APPEND _themis_network_deps themis_timeseries)
    endif()
    if(THEMIS_MODULE_GRAPH)
        list(APPEND _themis_network_deps themis_graph)
    endif()
    if(THEMIS_MODULE_GEO)
        list(APPEND _themis_network_deps themis_geo)
    endif()
    if(THEMIS_MODULE_CONTENT)
        list(APPEND _themis_network_deps themis_content)
    endif()

    themis_add_module(network
        SOURCES ${THEMIS_NETWORK_SOURCES}
        DEPENDENCIES ${_themis_network_deps}
    )
    if(MSVC)
        set_source_files_properties(
            ${CMAKE_SOURCE_DIR}/src/server/monitoring_api_handler.cpp
            ${CMAKE_SOURCE_DIR}/src/server/index_api_handler.cpp
            PROPERTIES COMPILE_OPTIONS "/bigobj;/Od;/Zm200"
        )
    endif()
    
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
        if(THEMIS_MODULE_GRAPH)
            target_link_libraries(themis_llm PUBLIC themis_graph)
        endif()
        target_include_directories(themis_llm PRIVATE
            ${CMAKE_SOURCE_DIR}/llama.cpp/include
            ${CMAKE_SOURCE_DIR}/llama.cpp/ggml/include
        )
        if(THEMIS_ENABLE_MIMALLOC AND TARGET mimalloc)
            target_link_libraries(themis_llm PUBLIC mimalloc)
        endif()
        if(THEMIS_ENABLE_JEMALLOC)
            if(TARGET jemalloc::jemalloc)
                target_link_libraries(themis_llm PUBLIC jemalloc::jemalloc)
            elseif(jemalloc_LIBRARIES)
                target_link_libraries(themis_llm PUBLIC ${jemalloc_LIBRARIES})
            endif()
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
            themis_transaction
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

        # Link CUDA runtime when geo CUDA dispatch is enabled.
        if(THEMIS_GEO_CUDA AND TARGET CUDA::cudart)
            list(APPEND _themis_geo_deps CUDA::cudart)
        endif()

        themis_add_module(geo
            SOURCES ${THEMIS_GEO_SOURCES}
            DEPENDENCIES ${_themis_geo_deps}
        )

        # Propagate THEMIS_GEO_CUDA compile definition to the geo module.
        if(THEMIS_GEO_CUDA AND TARGET themis_geo)
            target_compile_definitions(themis_geo PUBLIC THEMIS_GEO_CUDA)
        endif()
    endif()
    
    if(THEMIS_MODULE_GRAPH)
        set(_themis_graph_deps
            themis_base
            themis_storage
            themis_transaction
        )
        if(THEMIS_MODULE_GEO)
            list(APPEND _themis_graph_deps themis_geo)
        endif()

        themis_add_module(graph
            SOURCES ${THEMIS_GRAPH_SOURCES}
            DEPENDENCIES ${_themis_graph_deps}
        )
    endif()
    
    if(THEMIS_MODULE_CONTENT)
        set(_themis_content_deps
            themis_base
            themis_storage
            themis_security
        )
        if(TARGET yaml-cpp::yaml-cpp)
            list(APPEND _themis_content_deps yaml-cpp::yaml-cpp)
        endif()
        if(THEMIS_MODULE_GRAPH)
            list(APPEND _themis_content_deps themis_graph)
        endif()
        if(THEMIS_MODULE_LLM)
            list(APPEND _themis_content_deps themis_llm)
        endif()
        if(TARGET libzip::zip)
            list(APPEND _themis_content_deps libzip::zip)
        elseif(TARGET libzip::libzip)
            list(APPEND _themis_content_deps libzip::libzip)
        endif()
        if(THEMIS_ENABLE_OFFICE)
            if(TARGET pugixml::pugixml)
                list(APPEND _themis_content_deps pugixml::pugixml)
            elseif(TARGET pugixml::static)
                list(APPEND _themis_content_deps pugixml::static)
            elseif(TARGET pugixml)
                list(APPEND _themis_content_deps pugixml)
            endif()
        endif()

        themis_add_module(content
            SOURCES ${THEMIS_CONTENT_SOURCES}
            DEPENDENCIES ${_themis_content_deps}
        )
    endif()

    # Ingestion module (always included – covers all connector types)
    set(_themis_ingestion_deps
        themis_base
        themis_storage
    )
    if(THEMIS_MODULE_LLM)
        list(APPEND _themis_ingestion_deps themis_llm)
    endif()
    themis_add_module(ingestion
        SOURCES ${THEMIS_INGESTION_SOURCES}
        DEPENDENCIES ${_themis_ingestion_deps}
    )

    # Cross-module fixups for modular build
    # Removed: storage -> security link to avoid cycle
    # if(TARGET themis_storage AND TARGET themis_security)
    #     target_link_libraries(themis_storage PUBLIC themis_security)
    # endif()
    if(THEMIS_ENABLE_MIMALLOC AND TARGET mimalloc)
        target_link_libraries(themis_storage PUBLIC mimalloc)
    endif()
    if(THEMIS_ENABLE_JEMALLOC)
        if(TARGET jemalloc::jemalloc)
            target_link_libraries(themis_storage PUBLIC jemalloc::jemalloc)
        elseif(jemalloc_LIBRARIES)
            target_link_libraries(themis_storage PUBLIC ${jemalloc_LIBRARIES})
        endif()
    endif()
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

    list(APPEND THEMIS_ALL_MODULES themis_ingestion)
    
    set(THEMIS_ALL_MODULES ${THEMIS_ALL_MODULES} PARENT_SCOPE)
endfunction()
