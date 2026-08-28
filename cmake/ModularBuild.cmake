# Modular Build Configuration for ThemisDB
# This feature is planned for post-v1.3.0 release
# See docs/architecture/MODULARIZATION_PLAN.md for details

cmake_minimum_required(VERSION 3.20)

# Build mode option - default to modular build for current branch
# Note: keep version guard below to prevent enabling on older releases
option(THEMIS_BUILD_MODULAR "Build as modular libraries instead of monolithic core (v1.4.0+ feature)" ON)

# ── Wave-2 optional dependency guards (Q4 2026, default OFF = opt-in) ────────
# Security: RFC 3161 TSA via OpenSSL TS_* + libcurl (real impl in timestamp_authority_openssl.cpp)
option(THEMIS_USE_OPENSSL_TSA   "Enable RFC 3161 timestamping via OpenSSL TS_* API + libcurl" OFF)
# Security: Post-quantum crypto via liboqs (Kyber/MLKEM + Dilithium/MLDSA + SPHINCS+)
option(THEMIS_HAS_OQS           "Enable post-quantum crypto via liboqs (Kyber/Dilithium/SPHINCS+)" OFF)
# Security: Real PKCS#11 HSM backend (hsm_provider_pkcs11.cpp); software stub when OFF
option(THEMIS_ENABLE_HSM_REAL   "Enable real PKCS#11 HSM backend instead of software-only stub" OFF)
# Security: LoRA-adapter intent classifier calling LLM plugin classify endpoint
option(THEMIS_HAS_LORA_CLASSIFIER "Enable LoRA-adapted intent classifier via LLM plugin endpoint" OFF)
# Cache: Direct hiredis operations (SET/GET/DEL/EXPIRE) beyond pub/sub
option(THEMIS_HAS_HIREDIS       "Enable direct hiredis cache operations (SET/GET/DEL/EXPIRE)" OFF)
# Tensor: Auto-register RocksDBTensorBackend as default factory in TensorCoreStorageBridge
option(THEMIS_HAS_ROCKSDB_TENSOR "Auto-register RocksDBTensorBackend for TensorCoreStorageBridge" OFF)

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

    function(_themis_modular_mark_public_optional_source result_var relative_src_path)
        if(EXISTS "${CMAKE_SOURCE_DIR}/${relative_src_path}")
            set(${result_var} ON PARENT_SCOPE)
        else()
            message(STATUS "Modular optional public source missing (treated as private/externalized): ${relative_src_path}")
            set(${result_var} OFF PARENT_SCOPE)
        endif()
    endfunction()

    _themis_modular_mark_public_optional_source(THEMIS_HAS_PUBLIC_MYSQL_IMPORTER "src/importers/mysql_importer.cpp")
    _themis_modular_mark_public_optional_source(THEMIS_HAS_PUBLIC_MONGO_IMPORTER "src/importers/mongo_importer.cpp")
    _themis_modular_mark_public_optional_source(THEMIS_HAS_PUBLIC_KAFKA_IMPORTER "src/importers/kafka_importer.cpp")
    _themis_modular_mark_public_optional_source(THEMIS_HAS_PUBLIC_S3_IMPORTER "src/importers/s3_importer.cpp")
    _themis_modular_mark_public_optional_source(THEMIS_HAS_PUBLIC_BLOB_BACKEND_S3 "src/storage/blob_backend_s3.cpp")
    _themis_modular_mark_public_optional_source(THEMIS_HAS_PUBLIC_BLOB_BACKEND_AZURE "src/storage/blob_backend_azure.cpp")
endif()

# Optional module configuration (only relevant when THEMIS_BUILD_MODULAR=ON)
if(THEMIS_BUILD_MODULAR)
    option(THEMIS_MODULE_TRANSACTION "Include transaction module (required)" ON)
    option(THEMIS_MODULE_LLM "Include LLM inference module (optional)" ON)
    option(THEMIS_MODULE_LLM_SPLIT "Split LLM module into core and extension libraries" OFF)
    option(THEMIS_MODULE_GEO "Include geospatial module (optional)" ON)
    option(THEMIS_MODULE_GRAPH "Include graph analytics module (optional)" ON)
    option(THEMIS_MODULE_CONTENT "Include content processors module (optional)" ON)
    option(THEMIS_MODULE_TIMESERIES "Include time-series module" ON)
    option(THEMIS_MODULE_SHARDING "Include distributed sharding module" ON)
    option(THEMIS_MODULE_INGESTION "Include ingestion module (all data-intake connectors)" ON)
    option(THEMIS_MODULES_ENABLE_UNITY "Enable Unity Build for modular libraries on MSVC" OFF)
    option(THEMIS_MODULES_DISABLE_IPO_FOR_DLL_EXPORTS "Disable IPO/LTO for modular shared libraries that need Windows export-symbol generation" ON)
    set(THEMIS_MODULES_UNITY_BATCH_SIZE "20" CACHE STRING "Unity batch size for modular libraries")
    set(THEMIS_MODULES_UNITY_ALLOWLIST "network;query;sharding;geo;content;timeseries;security;transaction;ingestion;llm;llm_ext" CACHE STRING
        "Semicolon-separated module names for Unity Build (or ALL)")

    # llm_ext existed as a linker-symbol-count mitigation. With the current
    # Unity strategy enabled, keep the LLM module monolithic to avoid
    # split-related symbol ownership/link regressions.
    if(THEMIS_MODULES_ENABLE_UNITY AND THEMIS_MODULE_LLM_SPLIT)
        message(STATUS "THEMIS_MODULES_ENABLE_UNITY=ON -> disabling THEMIS_MODULE_LLM_SPLIT (llm_ext)")
        set(THEMIS_MODULE_LLM_SPLIT OFF CACHE BOOL
            "Split LLM module into core and extension libraries" FORCE)
    endif()
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
    
    # Set export macro (only for shared DLL modules)
    string(TOUPPER ${MODULE_NAME} MODULE_NAME_UPPER)
    if(NOT ARG_STATIC_MODULE)
        target_compile_definitions(themis_${MODULE_NAME} 
            PRIVATE THEMIS_${MODULE_NAME_UPPER}_EXPORTS
            PUBLIC THEMIS_${MODULE_NAME_UPPER}_ENABLED
        )
    else()
        # Static modules do not export symbols via __declspec; only mark as enabled
        target_compile_definitions(themis_${MODULE_NAME} 
            PUBLIC THEMIS_${MODULE_NAME_UPPER}_ENABLED
                   THEMIS_${MODULE_NAME_UPPER}_STATIC
        )
    endif()
    
    # Include directories
    target_include_directories(themis_${MODULE_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src
    )

    if(THEMIS_GENERATED_INCLUDE_DIR)
        target_include_directories(themis_${MODULE_NAME} PRIVATE "${THEMIS_GENERATED_INCLUDE_DIR}")
    endif()

    if(THEMIS_VCPKG_INCLUDE_FALLBACK_DIR)
        target_include_directories(themis_${MODULE_NAME} SYSTEM PRIVATE "${THEMIS_VCPKG_INCLUDE_FALLBACK_DIR}")
    endif()

    if(THEMIS_GLOBAL_COMPILE_DEFINITIONS)
        target_compile_definitions(themis_${MODULE_NAME} PUBLIC ${THEMIS_GLOBAL_COMPILE_DEFINITIONS})
    endif()
    
    # C++20 standard
    target_compile_features(themis_${MODULE_NAME} PUBLIC cxx_std_20)

    # Unity Build rollout for modular targets on MSVC.
    # Guarded by allowlist to prevent broad ODR/macro collisions.
    if(MSVC AND THEMIS_MODULES_ENABLE_UNITY)
        set(_themis_enable_unity_for_module OFF)
        if(THEMIS_MODULES_UNITY_ALLOWLIST STREQUAL "ALL")
            set(_themis_enable_unity_for_module ON)
        else()
            set(_themis_unity_modules ${THEMIS_MODULES_UNITY_ALLOWLIST})
            list(FIND _themis_unity_modules "${MODULE_NAME}" _themis_unity_module_idx)
            if(NOT _themis_unity_module_idx EQUAL -1)
                set(_themis_enable_unity_for_module ON)
            endif()
        endif()

        if(_themis_enable_unity_for_module)
            set_target_properties(themis_${MODULE_NAME} PROPERTIES
                UNITY_BUILD ON
                UNITY_BUILD_BATCH_SIZE ${THEMIS_MODULES_UNITY_BATCH_SIZE}
            )
        else()
            set_target_properties(themis_${MODULE_NAME} PROPERTIES
                UNITY_BUILD OFF
            )
        endif()
    endif()
    
    # Link dependencies
    if(ARG_DEPENDENCIES)
        target_link_libraries(themis_${MODULE_NAME} PUBLIC ${ARG_DEPENDENCIES})
    endif()
    
    # Define module-specific export macro for shared modules only
    if(NOT ARG_STATIC_MODULE)
        string(TOUPPER ${MODULE_NAME} MODULE_NAME_UPPER)
        target_compile_definitions(themis_${MODULE_NAME} PRIVATE THEMIS_${MODULE_NAME_UPPER}_EXPORTS)
    endif()
    
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
                VS_GLOBAL_WholeProgramOptimization "false"
            )
        else()
            set_target_properties(themis_${MODULE_NAME} PROPERTIES
                WINDOWS_EXPORT_ALL_SYMBOLS ON
                VS_GLOBAL_WholeProgramOptimization "false"
            )
        endif()
        if(THEMIS_MODULES_DISABLE_IPO_FOR_DLL_EXPORTS)
            set_target_properties(themis_${MODULE_NAME} PROPERTIES
                INTERPROCEDURAL_OPTIMIZATION FALSE
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
            COMPONENT runtime
        LIBRARY DESTINATION lib
            COMPONENT runtime
        ARCHIVE DESTINATION lib
            COMPONENT development
    )
    
    message(STATUS "Module configured: themis_${MODULE_NAME}")
endfunction()

# Module source file lists (to be populated during modularization)
# These will be extracted from the current THEMIS_CORE_SOURCES

set(THEMIS_BASE_SOURCES
    # Core utilities and cross-cutting concerns
    ../src/utils/serialization.cpp
    ../src/utils/logger.cpp
    ../src/utils/error_contracts.cpp
    ../src/utils/cursor.cpp
    ../src/utils/tracing.cpp
    ../src/utils/zstd_codec.cpp
    ../src/utils/lz4_codec.cpp
    ../src/utils/input_validator.cpp
    ../src/utils/hkdf_helper.cpp
    ../src/utils/hkdf_cache.cpp
    ../src/utils/stemmer.cpp
    ../src/utils/stopwords.cpp
    ../src/utils/normalizer.cpp
    ../src/utils/simd_distance.cpp
    ../src/utils/update_checker.cpp
    ../src/utils/http_client_pool.cpp
    $<$<BOOL:${THEMIS_ENABLE_GRPC}>:../src/utils/grpc_channel_pool.cpp>
    ../src/utils/cron_parser.cpp
    ../src/utils/bloom_filter.cpp
    ../src/utils/checksum_utils.cpp
    ../src/utils/compression_metrics.cpp
    ../src/utils/sampled_logger.cpp
    ../src/utils/self_awareness.cpp
    ../src/utils/timestamp_utils.cpp
    ../src/observability/metrics_collector.cpp
    ../src/observability/field_diagnostics_collector.cpp
    ../src/security/pii_redaction_policy.cpp
    ../src/utils/pii_detection_engine.cpp
    ../src/utils/regex_detection_engine.cpp
    ../src/utils/ner_detection_engine.cpp
    ../src/utils/pii_detector.cpp
    ../src/utils/pki_client.cpp
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
    ../src/core/concerns/adapter_registry.cpp
    ../src/core/concerns/adapter_signing.cpp
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
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/gpu/gpu_memory_manager_edition.cpp>
    # GPU-specific backends
    $<$<AND:$<BOOL:${THEMIS_ENABLE_GPU}>,$<BOOL:${WIN32}>>:../src/acceleration/directx_backend_full.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HIP}>:../src/acceleration/hip_backend.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CUDA}>:../src/acceleration/cuda_backend.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CUDA}>:../src/acceleration/cuda/ann_kernels.cu>
    $<$<BOOL:${THEMIS_ENABLE_CUDA}>:../src/acceleration/cuda/vector_kernels.cu>
    $<$<BOOL:${THEMIS_ENABLE_CUDA}>:../src/acceleration/cuda/tensor_core_matmul.cu>
    $<$<BOOL:${THEMIS_ENABLE_CUDA}>:../src/acceleration/cuda/tensor_compression_routing_kernels.cu>
    $<$<BOOL:${THEMIS_ENABLE_CUDA}>:../src/acceleration/cuda/geo_kernels.cu>
    $<$<BOOL:${THEMIS_ENABLE_CUDA}>:../src/acceleration/cuda/graph_kernels.cu>
    $<$<OR:$<BOOL:${THEMIS_ENABLE_CUDA}>,$<BOOL:${THEMIS_ENABLE_HIP}>>:../src/acceleration/faiss_gpu_backend.cpp>
    # Always compile oneapi_backend.cpp to provide the stub path when OneAPI
    # is not enabled. The file itself switches between real and stub
    # implementations with #ifdef THEMIS_ENABLE_ONEAPI, so it is safe to
    # compile unconditionally and avoids unresolved symbols in tests.
    ../src/acceleration/oneapi_backend.cpp
    # Always compile opencl_backend.cpp to provide the stub path when OpenCL
    # is not enabled. The file itself switches between real and stub
    # implementations with #ifdef THEMIS_ENABLE_OPENCL, so it is safe to
    # compile unconditionally and avoids unresolved symbols in tests.
    ../src/acceleration/opencl_backend.cpp
    $<$<BOOL:${THEMIS_ENABLE_VULKAN}>:../src/acceleration/vulkan_backend_full.cpp>
    $<$<BOOL:${THEMIS_ENABLE_ZLUDA}>:../src/acceleration/zluda_backend.cpp>
    # NCCL/RCCL vector backends (always compile for stub availability)
    ../src/acceleration/nccl_vector_backend.cpp
    ../src/acceleration/rccl_vector_backend.cpp
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
    
    # Interface stubs: forces MSVC to emit ISecondaryIndex/IVectorIndex/IGraphIndex
    # constructor+destructor symbols into themis_base.dll (THEMIS_BASE_API = dllexport)
    ../src/core/index_interface_stubs.cpp
)

set(THEMIS_STORAGE_SOURCES
    # Core storage engine
    ../src/storage/rocksdb_wrapper.cpp
    ../src/storage/base_entity.cpp
    ../src/storage/key_schema.cpp
    ../src/storage/backup_manager.cpp
    ../src/storage/columnar_format.cpp
    ../src/storage/simd_filter.cpp
    ../src/storage/storage_parquet_exporter.cpp
    ../src/storage/tensor_train_decomposer.cpp
    ../src/storage/tensor_network_storage_engine.cpp
    ../src/storage/tt_quantizer.cpp
    ../src/storage/tensor_router.cpp
    ../src/storage/hierarchical_tucker_decomposer.cpp
    ../src/tensor/hyper_index_builder.cpp
    ../src/tensor/tensor_mmap_bridge.cpp
    ../src/tensor/hiss_structural_search.cpp
    ../src/tensor/tensor_index_manager.cpp
    ../src/tensor/tensor_ingestion_bridge.cpp
    ../src/tensor/tensor_core_bridge.cpp
    ../src/tensor/tensor_fingerprint_graph.cpp
    ../src/tensor/tensor_error_handling.cpp
    ../src/tensor/tensor_summary_types.cpp
    ../src/tensor/compression_strategy.cpp
    ../src/tensor/adapter_repository.cpp
    ../src/tensor/tensor_mid_layer.cpp
    ../src/tensor/utr_converter.cpp
    ../src/tensor/tnsr_task.cpp
    ../src/tensor/hnsw_tt_bridge.cpp
    ../src/storage/batch_write_optimizer.cpp
    # ../src/storage/pitr_manager.cpp  # Temporarily disabled - needs transaction module
    ../src/storage/blob_redundancy_manager.cpp
    ../src/storage/erasure_coding_backend.cpp
    ../src/storage/erasure_coder_factory.cpp
    ../src/storage/hamming_coder.cpp
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
    # Index Analyzer – per-index analyze with tier thresholds, cron scheduling, AI/ML hook – v1.9.0
    ../src/storage/index_analyzer.cpp
    # Distributed transactions (2PC across multiple shards) – v1.7.0
    ../src/storage/distributed_transaction_manager.cpp
    # NVMe optimizations (io_uring, multi-queue, ZNS, Direct I/O) – v1.6.0
    ../src/storage/nvme_manager.cpp
    # Storage engine abstraction (DI-based)
    ../src/storage/storage_engine.cpp
    # Compression strategies (pluggable per-column-family)
    ../src/storage/compressed_storage.cpp
    ../src/storage/zero_copy_blob_transfer.cpp
    ../src/storage/gguf_metadata.cpp
    ../src/storage/blob_backend_gcs.cpp
    ../src/storage/compression_strategy.cpp
    ../src/storage/gpu_compression.cpp
    # Index maintenance moved to THEMIS_SECURITY_SOURCES (depends on vector index internals)
    # Blob storage backends
    ../src/storage/blob_backend_filesystem.cpp
    $<$<AND:$<BOOL:${THEMIS_ENABLE_BLOB_S3}>,$<BOOL:${THEMIS_ENABLE_CLOUD_STORAGE}>,$<BOOL:${THEMIS_HAS_AWS_SDK}>,$<BOOL:${THEMIS_HAS_PUBLIC_BLOB_BACKEND_S3}>>:../src/storage/blob_backend_s3.cpp>
    $<$<AND:$<BOOL:${THEMIS_ENABLE_BLOB_AZURE}>,$<BOOL:${THEMIS_ENABLE_CLOUD_STORAGE}>,$<BOOL:${THEMIS_HAS_AZURE_STORAGE}>,$<BOOL:${THEMIS_HAS_PUBLIC_BLOB_BACKEND_AZURE}>>:../src/storage/blob_backend_azure.cpp>
    $<$<AND:$<BOOL:${THEMIS_ENABLE_BLOB_WEBDAV}>,$<BOOL:${THEMIS_ENABLE_CLOUD_STORAGE}>>:../src/storage/blob_backend_webdav.cpp>
    $<$<AND:$<BOOL:${THEMIS_ENABLE_BLOB_GCS}>,$<BOOL:${THEMIS_ENABLE_CLOUD_STORAGE}>,$<BOOL:${THEMIS_HAS_GCS_SDK}>>:../src/storage/blob_backend_gcs.cpp>
    # Merge operators (counter, list-append RocksDB custom operators)
    ../src/storage/merge_operators.cpp
    ../src/sharding/distributed_time_coordinator.cpp
    
    ../src/storage/encrypted_blob_backend.cpp
    ../src/storage/mvcc_chain_pruner.cpp
    ../src/storage/streaming_ingest_manager.cpp
    ../src/storage/vector_index_backend.cpp
    ../src/storage/wom_tree.cpp
    ../src/utils/geo/ewkb.cpp
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
    ../src/metadata/er_diagram_exporter.cpp
    # ../src/metadata/distributed_catalog.cpp
    # Temporarily excluded in modular build: depends on MetadataShardRouter
    # symbols from sharding module and introduces unresolved externals in
    # themis_storage when sharding is linked as a separate DLL.
    
    # Indexes
    ../src/index/secondary_index.cpp
    ../src/index/index_compression.cpp
    ../src/index/ann_index.cpp
        ../src/index/ann_frontdoor.cpp
    ../src/index/rotary_embeddings.cpp
    ../src/index/rotary_embeddings_gpu_cpu.cpp
    ../src/index/learnable_rope.cpp
    ../src/index/lora_rope.cpp
    ../src/index/property_graph.cpp
    ../src/index/process_graph.cpp
    ../src/index/hnsw_layer_optimizer.cpp
    ../src/index/hnsw_parameter_tuner.cpp
    ../src/index/hnsw_production_defaults.cpp
    ../src/index/cuda_hnsw_graph_traversal.cpp
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/index/gpu_vector_index.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/index/multi_gpu_vector_index.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/index/gpu_memory_oversubscription.cpp>
    $<$<BOOL:${THEMIS_ENABLE_VULKAN}>:../src/index/gpu_vector_index_vulkan.cpp>
    $<$<BOOL:${THEMIS_ENABLE_VULKAN}>:../src/llm/lora_framework/vulkan_context.cpp>
    $<$<BOOL:${THEMIS_ENABLE_VULKAN}>:../src/llm/lora_framework/vulkan_buffer.cpp>
    $<$<BOOL:${THEMIS_ENABLE_VULKAN}>:../src/llm/lora_framework/vulkan_pipeline.cpp>
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

    
    # Performance enhancements
    ../src/performance/phase2_feature_flags.cpp
    ../src/performance/phase3/feature_flags.cpp
    ../src/performance/numa_topology.cpp
    ../src/performance/prometheus_exporter.cpp
        $<$<BOOL:${THEMIS_BUILD_CHIMERA}>:../src/performance/chimera_exporter.cpp>
    ../src/performance/async_metrics_exporter.cpp
    ../src/performance/phase3/memory_pressure.cpp
    ../src/performance/phase3/adaptive_batch_tuner.cpp
    ../src/performance/phase4/feature_flags.cpp
    # pmu_counters.cpp is always compiled: it provides stub fallbacks when
    # perf_event_open is unavailable (containers, non-Linux).  The actual PMU
    # paths are gated by the THEMIS_ENABLE_PMU_COUNTERS compile definition.
    ../src/performance/phase4/pmu_counters.cpp
    ../src/performance/numa_memory_manager.cpp
    ../src/performance/advanced_cache_manager.cpp
    ../src/performance/workload_adaptive_optimizer.cpp
    
    # Storage enhancements
    ../src/cache/cache_eviction_policy.cpp
    ../src/cache/semantic_cache.cpp
    ../src/cache/cache_manager.cpp
    
    # Updates
    ../src/updates/build_verifier.cpp
    ../src/updates/hardware_telemetry.cpp
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
    ../src/temporal/temporal_cold_store.cpp
    ../src/temporal/temporal_tier_manager.cpp
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
# Keep Bao implementation available in all builds because network/tests may
# reference BaoOptimizer symbols even when feature flags disable active usage.
list(APPEND THEMIS_STORAGE_SOURCES ../src/performance/phase3/bao.cpp)
if(THEMIS_ENABLE_PMEM)
    list(APPEND THEMIS_STORAGE_SOURCES ../src/performance/phase4/pmem_storage.cpp)
endif()
if(THEMIS_ENABLE_IO_URING)
    list(APPEND THEMIS_STORAGE_SOURCES ../src/performance/phase4/io_uring_zero_copy.cpp)
endif()

# Vector and graph indexing (storage module - basic utilities only)
list(APPEND THEMIS_STORAGE_SOURCES)

set(THEMIS_QUERY_SOURCES
    # Query engine
    ../src/query/query_engine.cpp
    ../src/search/hybrid_search.cpp
    ../src/query/query_optimizer.cpp
    ../src/query/adaptive_optimizer.cpp
    ../src/query/adaptive_join.cpp
    ../src/query/runtime_reoptimizer.cpp
    ../src/query/optimizer_cost_model.cpp
    ../src/query/aql_parser.cpp
    ../src/query/aql_parser_json.cpp
    ../src/query/aql_parser_service.cpp
    ../src/query/sql_parser.cpp
    ../src/query/aql_translator.cpp
    ../src/query/aql_mutation_translator.cpp
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
    ../src/query/synopsis_store.cpp
    ../src/query/incremental_agg.cpp
    ../src/query/cq_watermark.cpp
    ../src/query/continuous_query_engine.cpp
    ../src/query/continuous_query_planner.cpp
    ../src/query/query_rewrite_rule.cpp
    ../src/query/query_profiler.cpp
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
    ../src/distributed_knowledge/federated_rag_merger.cpp
    ../src/query/plan_cache.cpp
    ../src/query/query_compiler.cpp
    ../src/query/materialized_view.cpp
    ../src/query/mutation_executor.cpp
    # Vectorized Execution Engine – column-store style batch processing (Issue #2434)
    ../src/query/vectorized_execution.cpp
    ../src/query/sparql_parser.cpp
    ../src/performance/cycle_metrics.cpp
    ../src/performance/workload_predictor.cpp
    ../src/performance/async_metrics_exporter.cpp
        $<$<BOOL:${THEMIS_BUILD_CHIMERA}>:../src/performance/chimera_exporter.cpp>
    ../src/performance/prometheus_exporter.cpp
    ../src/performance/phase3/per_query_cost_model.cpp
    ../src/cache/cache_replication.cpp
    ../src/cache/cache_replication_coordinator.cpp
    $<$<BOOL:${THEMIS_ENABLE_GRPC}>:../src/cache/grpc_remote_cache_peer.cpp>
    # Legacy hiredis implementation conflicts with distributed_cache_coordinator
    # (duplicate symbol definitions). Keep only the distributed implementation.
    ../src/cache/distributed_cache_coordinator.cpp
    ../src/cache/adaptive_query_cache.cpp
    ../src/cache/warmup.cpp
    ../src/cache/cache_hit_rate_slo_monitor.cpp
    ../src/cache/predictive_prefetcher.cpp
    ../src/query/approximate_aggregator.cpp
    ../src/query/aql_safety_validator.cpp
    ../src/query/statistical_aggregator.cpp
    ../src/query/semantic_cache.cpp
    ../src/query/functions/function_registry.cpp
    $<$<BOOL:${THEMIS_ENABLE_GRAPHQL}>:../src/api/graphql.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GRAPHQL}>:../src/api/graphql_aql_resolver.cpp>
    ../src/query/functions/ethics_functions.cpp
    ../src/query/functions/fulltext_functions.cpp
    ../src/query/functions/lora_functions.cpp
    ../src/query/llm_query_shims.cpp
    ../src/query/functions/tensor_functions.cpp
    ../src/query/tensor_contraction_engine.cpp
    ../src/rag/ontology_aware_retriever.cpp
    ../src/rag/graph_truth_validator.cpp
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
    ../src/process/process_common.cpp
    ../src/process/epk_aris_xml_importer.cpp
    ../src/process/bpmn_serializer.cpp
    ../src/process/serializer_hardening.cpp
    ../src/process/epk_serializer.cpp
    ../src/process/llm_process_descriptor.cpp
    ../src/process/vcc_vpb_importer.cpp
    ../src/process/process_linker.cpp
    ../src/process/process_diagnostics.cpp
    ../src/process/process_graph_rag.cpp
    ../src/process/cmmn_serializer.cpp
    ../src/process/fim_importer.cpp
    ../src/process/object_centric_tracer.cpp
    ../src/process/process_community_detector.cpp
    ../src/process/process_light_retriever.cpp
    ../src/process/federation_replica_manager.cpp
    ../src/process/process_conflict_resolver.cpp
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
    $<$<NOT:$<BOOL:${THEMIS_MODULE_LLM}>>:../src/aql/aql_query_builder.cpp>
    $<$<NOT:$<BOOL:${THEMIS_MODULE_LLM}>>:../src/aql/aql_query_validator.cpp>
    ../src/aql/aql_optimizer_advisor.cpp
    ../src/aql/aql_query_template_library.cpp
    ../src/aql/aql_conversation_context.cpp
    $<$<NOT:$<BOOL:${THEMIS_MODULE_LLM}>>:../src/aql/aql_schema_provider.cpp>
    ../src/aql/aql_migration_assistant.cpp
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/aql/classify_bridge.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/aql/docs_assistant_functions.cpp>
    ../src/query/scope_enforcer.cpp

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
    $<$<BOOL:${THEMIS_HAS_PUBLIC_MYSQL_IMPORTER}>:../src/importers/mysql_importer.cpp>
    $<$<BOOL:${THEMIS_HAS_PUBLIC_MONGO_IMPORTER}>:../src/importers/mongo_importer.cpp>
    ../src/importers/elasticsearch_importer.cpp
    ../src/importers/redis_importer.cpp
    ../src/importers/debezium_cdc_importer.cpp
    ../src/importers/sqlite_importer.cpp
    ../src/importers/flatfile_importer.cpp
    ../src/importers/huggingface_ingest_plugin.cpp
    ../src/importers/schema_validator.cpp
    $<$<BOOL:${THEMIS_HAS_PUBLIC_KAFKA_IMPORTER}>:../src/importers/kafka_importer.cpp>
    ../src/importers/oracle_importer.cpp
    ../src/importers/gui_import_wizard.cpp
    $<$<AND:$<BOOL:${THEMIS_ENABLE_S3}>,$<BOOL:${THEMIS_HAS_PUBLIC_S3_IMPORTER}>>:../src/importers/s3_importer.cpp>
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
    # Input validation and sanitization
    ../src/security/input_validator.cpp
    # Encryption and key management
    ../src/security/ai_operation_guard.cpp
    ../src/security/ai_snapshot_cleanup.cpp
    ../src/security/intent_classifier.cpp
    ../src/governance/cross_border_transfer.cpp
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
    ../src/security/prompt_injection_pattern_registry.cpp
    ../src/auth/auth_audit_logger.cpp
    ../src/auth/rate_limiter_backend.cpp
    ../src/security/user_registration_plugin.cpp
    ../src/security/arrow_user_registration_plugin.cpp
    ../src/security/webdav_user_registration_plugin.cpp
    ../src/security/embedded_user_registration_plugin.cpp
    ../src/security/hsm_provider.cpp
    ../src/security/hsm_provider_pkcs11.cpp
    ../src/security/hsm_key_provider_adapter.cpp
    $<$<BOOL:${THEMIS_USE_OPENSSL_TSA}>:../src/security/timestamp_authority_openssl.cpp>
    $<$<NOT:$<BOOL:${THEMIS_USE_OPENSSL_TSA}>>:../src/security/timestamp_authority.cpp>
    ../src/security/vcc_pki_client.cpp
    ../src/security/xxe_safe_xml_parser.cpp
    
    # Authentication
    ../src/auth/jwt_validator.cpp
    ../src/auth/token_blacklist.cpp
    ../src/auth/redis_token_blacklist.cpp
    ../src/auth/rocksdb_token_blacklist.cpp
    ../src/auth/distributed_token_blacklist.cpp
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
    ../src/auth/passkey_authenticator.cpp
    ../src/auth/auth_metrics.cpp
    ../src/auth/auth_error.cpp
    ../src/auth/jwks_security.cpp
    ../src/auth/kerberos_security.cpp
    ../src/auth/totp_replay_cache.cpp
    ../src/auth/totp_secret_encryption.cpp
    ../src/auth/jwt_key_rotation_manager.cpp
    ../src/auth/principal_validator.cpp
    ../src/server/auth_middleware.cpp
    ../src/server/policy_engine.cpp
    
    # Governance
    ../src/governance/policy_engine.cpp
    ../src/governance/policy_manager.cpp
    ../src/governance/policy_version_history.cpp
    ../src/governance/opa_adapter.cpp
    ../src/governance/data_lineage.cpp
    ../src/governance/governance_diagnostics.cpp
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

    # Utility modules that depend on security/storage internals
    ../src/utils/pii_stream_scanner.cpp
    ../src/utils/utils_adapters.cpp
    ../src/utils/lek_manager.cpp
    ../src/utils/saga_logger.cpp
    ../src/utils/audit_logger.cpp
    ../src/utils/retention_manager.cpp
    
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
    
    # Encryption and field helpers (use storage + security features)
    ../src/security/field_encryption.cpp
    ../src/security/encrypted_field.cpp
    ../src/index/graph_index.cpp
    ../src/storage/index_maintenance.cpp
    ../src/storage/index_analyzer.cpp
    ../src/index/vector_index.cpp
    ../src/index/index_manager.cpp
    ../src/index/vector_auto_buffer.cpp
    ../src/index/gnn_embeddings.cpp
    ../src/index/approximate_radius_search.cpp
    ../src/index/multi_vector_search.cpp
    # Storage-backed PII and vector index helpers
    ../src/utils/pii_pseudonymizer.cpp
    # ../src/cache/embedding_cache.cpp  # Temporarily disabled - requires mimalloc
)

set(THEMIS_TRANSACTION_SOURCES
    # Transaction management
    ../src/transaction/transaction_manager.cpp
    ../src/transaction/lock_manager.cpp
    ../src/transaction/crash_recovery_manager.cpp
    ../src/transaction/saga.cpp
    ../src/transaction/distributed_saga.cpp
    ../src/transaction/saga_orchestrator.cpp
    ../src/transaction/saga_plugin_bridge.cpp
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
    ../src/replication/async_wal_shipper.cpp
    ../src/replication/geo_placement.cpp
    
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
    ../src/sharding/raid_paxos_consensus.cpp
    ../src/sharding/paxos_wal.cpp
    ../src/sharding/paxos_snapshot.cpp
    ../src/sharding/paxos_state_persistence.cpp
    ../src/sharding/dual_consensus_orchestrator.cpp
    ../src/sharding/cross_shard_transaction.cpp
    ../src/sharding/cross_shard_fk_validator.cpp
    ../src/sharding/cross_shard_ssi_manager.cpp
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
    ../src/sharding/gpu_erasure_coder.cpp
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

    # Phase 5 — LoRA Artifact Distribution
    ../src/sharding/lora_artifact_distribution.cpp
)

# Do not gate this on TARGET existence. The proto library is created later in the
# same configure pass and the target may not exist yet when this source list is
# assembled. Only omit the protobuf-dependent sharding sources when protobuf is
# genuinely unavailable, otherwise the build will lose the concrete implementations
# that satisfy the link-time references from other sharding modules.
if(NOT Protobuf_FOUND)
    list(REMOVE_ITEM THEMIS_SHARDING_SOURCES
        ../src/sharding/shard_rpc_server.cpp
        ../src/server/rpc/blob_transfer_handler.cpp
        ../src/sharding/gossip_config_manager.cpp
        ../src/sharding/gossip_consensus_adapter.cpp
        ../src/sharding/shard_resource_manager.cpp
        ../src/sharding/distributed_coordinator.cpp
    )
    message(WARNING "Protobuf not found: excluding protobuf-dependent sharding RPC/gossip sources")
endif()

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
    ../src/prompt_engineering/rag_context_budget_manager.cpp
    ../src/prompt_engineering/prompt_quality_evaluator.cpp
    ../src/prompt_engineering/tree_of_thoughts.cpp
    ../src/prompt_engineering/protegi_optimizer.cpp
    ../src/prompt_engineering/dspy_module.cpp
    ../src/prompt_engineering/structured_output.cpp
    # SSM RocksDB-backed store: ensure implementation is part of modular LLM build
    ../src/llm/ssm_state_rocksdb_store.cpp
    ../src/prompt_engineering/markdown_utils.cpp
    ../src/prompt_engineering/prompt_compressor.cpp
    ../src/prompt_engineering/adversarial_prompt_tester.cpp
    ../src/prompt_engineering/prompt_template_validator.cpp
    ../src/prompt_engineering/prompt_template_compiler.cpp
    ../src/distributed_knowledge/cross_shard_feedback_sync.cpp
    ../src/distributed_knowledge/adapter_capability_announcement.cpp
    ../src/distributed_knowledge/lora_federation_coordinator.cpp
    ../src/api/federation_admin_handler.cpp
    ../src/llm/block_table.cpp
    ../src/llm/paged_block_manager.cpp
    ../src/llm/paged_kv_cache.cpp
    ../src/llm/paged_kv_cache_manager.cpp
    ../src/llm/llm_plugin_manager.cpp
    ../src/llm/model_loader.cpp
    ../src/llm/model_quantization_pipeline.cpp
    ../src/llm/model_downloader.cpp
    ../src/llm/aql_train_parser.cpp
    ../src/aql/llm_aql_embedding_bridge.cpp
    ../src/llm/llama_wrapper.cpp
    ../src/llm/llama_lora_adapter.cpp
    ../src/llm/llama_grammar_adapter.cpp
    ../src/llm/llamacpp_inference_engine.cpp
    ../src/llm/shared_worker_pool.cpp
    ../src/llm/async_inference_engine.cpp
    ../src/llm/prompt_policy.cpp
    ../src/llm/speculative_decoder.cpp
    ../src/llm/kv_prefix_transfer_manager.cpp
    ../src/llm/lookup_decoder.cpp
    ../src/llm/model_router.cpp
    ../src/llm/adapter_registry.cpp
    ../src/llm/final_layer_orchestrator.cpp
    ../src/llm/inference_engine_enhanced.cpp
    ../src/llm/streaming_handler.cpp
    ../src/llm/openai_compat_adapter.cpp
    ../src/llm/embedded_llm.cpp
    ../src/llm/ethical_guidelines_manager.cpp
    ../src/llm/constitutional_reasoning_engine.cpp
    ../src/ai/cai_ethics_integration.cpp
    ../src/ethics_ai/ethics_evaluator.cpp
    ../src/llm/ethics_aware_confidence_detector.cpp
    ../src/llm/ai_decision_auditor.cpp
    ../src/llm/decision_record_yaml_processor.cpp
    ../src/storage/schema_dead_weight_detector.cpp
    ../src/storage/storage_layout_advisor.cpp
    ../src/transaction/transaction_semantic_advisor.cpp
    ../src/temporal/temporal_migrator.cpp
    ../src/llm/moral_analyzer.cpp
    ../src/llm/multi_perspective_generator.cpp
    ../src/llm/meta_prompt_generator.cpp
    ../src/llm/prompt_evaluator.cpp
    ../src/llm/prompt_optimizer.cpp
    ../src/llm/inference_handle.cpp
    ../src/llm/llm_security_utils.cpp
    ../src/llm/safety/classifier.cpp
    ../src/llm/safety/guardian.cpp
    ../src/llm/safety/monitoring.cpp
    ../src/llm/vision_config.cpp
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
    ../src/ai/cai_ethics_integration.cpp
    ../src/ethics_ai/ethics_evaluator.cpp
    ../src/llm/ethics_aware_confidence_detector.cpp
    ../src/llm/moral_analyzer.cpp
    ../src/llm/multi_perspective_generator.cpp
    # Decision Record YAML Processor (async, independent of llama.cpp / RocksDB)
    ../src/llm/decision_record_yaml_processor.cpp
    # Feedback & Security
    ../src/llm/feedback_plugin_basic.cpp
    ../src/llm/llm_security_utils.cpp
    ../src/llm/lora_security_validator.cpp
    ../src/llm/lora_certificate_store.cpp
    ../src/llm/security/signature_verifier.cpp
    # Vision resource monitoring
    ../src/llm/vision_config.cpp
    ../src/llm/vision_resource_monitor.cpp
    # LoRA framework additions (unconditional)
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/llm/lora_framework/distributed_dataloader.cpp>
    ../src/llm/lora_framework/kernels/cpu_fused_kernels.cpp
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/llm/lora_framework/paged_memory_manager.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/llm/lora_framework/paged_optimizer.cpp>
        ../src/cache/embedding_cache.cpp
        ../src/llm/lora_framework/lora_layers.cpp
    
    # LoRA framework (core subset)
    ../src/llm/lora_framework/lora_orchestrator.cpp
    ../src/llm/lora_framework/lora_feedback_storage.cpp
    ../src/llm/lora_framework/lora_training_config.cpp
    ../src/llm/lora_framework/feedback_plugin.cpp
    ../src/llm/lora_framework/lora_provenance.cpp
    # Phase 4: HashChain & Provenance Layer (issue #5417)
    ../src/llm/lora_framework/lora_package_provenance.cpp
    # Use the ThemisDB-integrated storage service (BaseEntity + RocksDB + BlobStorage + encryption).
    # lora_storage_service.cpp (filesystem-only) is intentionally excluded here to avoid duplicate
    # symbol errors.  Search for target "lora_storage_service_themisdb" in cmake/CMakeLists.txt
    # and src/llm/lora_framework/ for details on the two implementations.
    ../src/llm/lora_framework/lora_storage_service_themisdb.cpp
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
    ../src/rag/self_rag.cpp
    ../src/rag/knowledge_gap_detector.cpp
    ../src/rag/llm_integration.cpp
    ../src/rag/llm_judge_client.cpp
    ../src/rag/llm_judge_integration.cpp
    ../src/rag/nli_faithfulness_verifier.cpp
    ../src/rag/quality_control_pipeline.cpp
    ../src/rag/prompt_templates.cpp
    ../src/rag/response_parser.cpp
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
    ../src/rag/lora_enhanced_retriever.cpp
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
    # DPR vectorizer and fairness detector (RAG retrieval and bias evaluation)
    ../src/rag/dpr_vectorizer.cpp
    ../src/rag/fairness_detector.cpp

    # LLM-owned AQL support files
    ../src/aql/llm_aql_handler.cpp
    ../src/aql/llm_validation_pipeline.cpp
    ../src/aql/aql_query_validator.cpp
    ../src/aql/aql_query_builder.cpp
    ../src/aql/aql_schema_provider.cpp
    ../src/aql/aql_fewshot_example_library.cpp
    ../src/aql/aql_syntax_highlighter.cpp
    ../src/aql/aql_confidence_scorer.cpp
    ../src/aql/llm_metrics_collector.cpp
    ../src/llm/llm_client_default.cpp
    # Phase 4: Multi-modal RAG (image + text retrieval)
    ../src/rag/multimodal_rag.cpp
    # Phase 1–4: Missing RAG evaluators and orchestrators
    ../src/rag/ab_testing_framework.cpp
    ../src/rag/agentic_rag.cpp
    ../src/rag/delegate_evaluator.cpp
    ../src/rag/bayesian_optimizer.cpp
    ../src/rag/claim_extractor.cpp
    ../src/rag/coherence_evaluator.cpp
    ../src/rag/completeness_evaluator.cpp
    ../src/rag/continuous_learning_client.cpp
    ../src/rag/cot_evaluator.cpp
    ../src/rag/faithfulness_evaluator.cpp
    ../src/rag/hallucination_dashboard.cpp
    ../src/rag/http_metrics_client.cpp
    ../src/rag/judge_config.cpp
    ../src/rag/judge_ensemble.cpp
    ../src/rag/knowledge_graph_retriever.cpp
    ../src/rag/kg/knowledge_graph_adapter.cpp
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
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/server/model_integrity_verifier.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/server/lora_api_handler.cpp>
    # Voice assistant implementation (always required when tests link VoiceAssistant)
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_assistant.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_assistant_llm.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/audio_preprocessing.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/emotion_analyzer.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_accessibility.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_audio_storage.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_authenticator.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_batch_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_browser_streaming.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_error_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_intent_detector.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_macro_manager.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_meeting_support.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_model_cache.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_security.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_session_manager.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_telephony.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_tts_customizer.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/wake_word_detector.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/content/stt_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/content/tts_processor.cpp>
)

set(THEMIS_TRAINING_SOURCES
    ../src/training/auto_labeler.cpp
    ../src/training/ada_lora_adapter.cpp
    ../src/training/adalora_tt_bridge.cpp
    ../src/training/adapter_serving.cpp
    ../src/training/database_domain_auto_labeler.cpp
    ../src/training/incremental_lora_trainer.cpp
    ../src/training/knowledge_graph_enricher.cpp
    ../src/training/lora_adapter.cpp
    ../src/training/lora_adapter_merger.cpp
    ../src/training/lora_checkpoint_manager.cpp
    ../src/training/lora_data_selection.cpp
    ../src/training/modality_parser.cpp
    ../src/training/multi_task_lora.cpp
    ../src/training/provenance_tracker.cpp
    ../src/training/training_pipeline.cpp
    ../src/rag/continuous_learning_orchestrator.cpp
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
        ../src/llm/lora_framework/kernels/vulkan_kernels.cpp
    )
endif()

# Optional split for very large LLM builds (notably MSVC toolchains).
if(THEMIS_BUILD_MODULAR AND THEMIS_MODULE_LLM AND THEMIS_MODULE_LLM_SPLIT)
    list(LENGTH THEMIS_LLM_SOURCES _themis_llm_source_count)
    if(_themis_llm_source_count GREATER 1)
        math(EXPR _themis_llm_split_index "${_themis_llm_source_count} / 2")
        math(EXPR _themis_llm_ext_count "${_themis_llm_source_count} - ${_themis_llm_split_index}")
        list(SUBLIST THEMIS_LLM_SOURCES 0 ${_themis_llm_split_index} THEMIS_LLM_CORE_SOURCES)
        list(SUBLIST THEMIS_LLM_SOURCES ${_themis_llm_split_index} ${_themis_llm_ext_count} THEMIS_LLM_EXT_SOURCES)
    else()
        set(THEMIS_LLM_CORE_SOURCES ${THEMIS_LLM_SOURCES})
        set(THEMIS_LLM_EXT_SOURCES)
    endif()
else()
    set(THEMIS_LLM_CORE_SOURCES ${THEMIS_LLM_SOURCES})
    set(THEMIS_LLM_EXT_SOURCES)
endif()

# Create a small LLM API module that contains a minimal set of
# runtime-facing implementations (DocsAssistant, EmbeddedLLM, ThemisHelpLoRA)
# These are required by query components at link time but should not
# drag in the full LLM implementation which depends on sharding.
set(THEMIS_LLM_API_SOURCES
    ../src/llm/llm_factory_stub.cpp
    ../src/llm/api/docs_assistant_adapter.cpp
    ../src/llm/api/themis_help_lora_adapter.cpp
    ../src/llm/api/embedded_llm_adapter.cpp
)

# Ensure the heavyweight implementations remain in the core LLM
# sources so the full `themis_llm` library continues to provide
# real functionality when linked at final link time.
list(APPEND THEMIS_LLM_CORE_SOURCES
    ../src/llm/docs_assistant.cpp
    ../src/llm/embedded_llm.cpp
    ../src/llm/applications/themis_help_lora.cpp
)

# Remove API files from the core sources if they are present
list(REMOVE_ITEM THEMIS_LLM_CORE_SOURCES ${THEMIS_LLM_API_SOURCES})

# Keep AQL LLM integration files together in llm_ext when split is enabled.
# This avoids accidental core/ext separation by index-based splitting and
# prevents link-time resolution from pulling query objects into unrelated DLLs.
if(THEMIS_BUILD_MODULAR AND THEMIS_MODULE_LLM AND THEMIS_MODULE_LLM_SPLIT)
    set(_themis_llm_ext_aql_sources
        ../src/aql/llm_aql_handler.cpp
        ../src/aql/aql_query_validator.cpp
        ../src/aql/aql_query_builder.cpp
        ../src/aql/aql_schema_provider.cpp
    )
    list(REMOVE_ITEM THEMIS_LLM_CORE_SOURCES ${_themis_llm_ext_aql_sources})
    list(REMOVE_ITEM THEMIS_LLM_EXT_SOURCES ${_themis_llm_ext_aql_sources})
    list(APPEND THEMIS_LLM_EXT_SOURCES ${_themis_llm_ext_aql_sources})
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
    $<$<AND:$<BOOL:${THEMIS_ENABLE_CONTENT}>,$<BOOL:${THEMIS_ENABLE_LLM}>>:../src/content/content_manager_llm.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/pipeline/zstd_compression.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/pipeline/content_chunker.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/pipeline/bulk_upload_interface.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/pipeline/async_bulk_uploader.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/pipeline/multimodal_chunker.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/adapters/archive_extractor_adapter.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/adapters/audio_extractor_adapter.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/adapters/format_extractor_factory.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/adapters/image_extractor_adapter.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/adapters/office_extractor_adapter.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/adapters/pdf_extractor_adapter.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/adapters/text_extractor_adapter.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/aql/aql_ingestion_bridge.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/rag/rag_ingestion_bridge.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/toolbox/content_toolbox_bridge.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/toolbox/toolbox_builder.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/toolbox/toolbox_registry.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/importers/huggingface_ingestion_plugin.cpp>
    # Project collaboration and audit (always required by project tests)
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/projects/collaboration_manager.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/projects/in_memory_project_audit_log.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/projects/project_diff.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/projects/project_lifecycle.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/projects/project_metrics.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/projects/project_template.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/projects/project_versioning.cpp>
)

## Avoid Unity aggregation for a small list of very large/complex source files
# which historically produced oversized Unity objects on MSVC (LNK1248).
# Add heavy sources here to compile them outside of Unity batches.
set(THEMIS_UNITY_SKIP_SOURCES
    ../src/server/http_server.cpp
    ../src/query/query_engine.cpp
    ../src/query/functions/function_registry.cpp
    ../src/index/inverted_index.cpp
    ../src/index/graph_index.cpp
    ../src/index/product_quantizer.cpp
)

foreach(_themis_skip_src IN LISTS THEMIS_UNITY_SKIP_SOURCES)
    # Resolve to absolute path relative to this CMake file
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_themis_skip_src}")
        get_filename_component(_themis_skip_abs "${CMAKE_CURRENT_SOURCE_DIR}/${_themis_skip_src}" ABSOLUTE)
        set_source_files_properties(${_themis_skip_abs} PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)
        message(STATUS "Marked to skip Unity inclusion: ${_themis_skip_abs}")
    endif()
endforeach()

set(THEMIS_TIMESERIES_SOURCES
    # Time-series storage
    ../src/timeseries/timeseries.cpp
    ../src/timeseries/tsstore.cpp
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
    ../src/timeseries/adaptive_flush_controller.cpp
    ../src/timeseries/ts_stream_cursor.cpp
    ../src/timeseries/encrypted_chunk_store.cpp
    ../src/timeseries/ts_encrypted_key_rotation.cpp
    ../src/timeseries/compression_selector.cpp
    ../src/timeseries/anomaly_detection.cpp
    ../src/timeseries/gap_fill.cpp
)

set(THEMIS_INGESTION_SOURCES
    # Ingestion module – unified data intake layer
    ../src/ingestion/ingestion_manager.cpp
    ../src/ingestion/filesystem_ingester.cpp
    ../src/ingestion/api_connector.cpp
    ../src/ingestion/entity_assembler.cpp
    ../src/ingestion/ingestion_sinks.cpp
    ../src/ingestion/legal_domain.cpp
    ../src/ingestion/workflow_engine.cpp
    ../src/ingestion/steps/base_entity_assembler_step.cpp
    ../src/ingestion/steps/chunk_embed_step.cpp
    ../src/ingestion/steps/chunk_text_step.cpp
    ../src/ingestion/steps/decompress_step.cpp
    ../src/ingestion/steps/deontic_step.cpp
    ../src/ingestion/steps/format_parse_step.cpp
    ../src/ingestion/steps/legal_metadata_step.cpp
    ../src/ingestion/steps/legal_reference_step.cpp
    ../src/ingestion/steps/llm_extract_step.cpp
    ../src/ingestion/steps/ner_step.cpp
    ../src/ingestion/steps/parse_text_step.cpp
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
    ../src/ingestion/ingestion_quality_judge.cpp
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/ingestion/llm_adapter.cpp>
    ../src/ingestion/steps/chunk_tt_decompose_step.cpp
    ../src/ingestion/steps/tensor_core_bridge_step.cpp
    ../src/toolbox/ingestion_toolbox.cpp
)

set(THEMIS_NETWORK_SOURCES
    # HTTP Server (conditional)
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/http_server.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/http_shutdown_manager.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/transaction_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/distributed_txn_api_handler.cpp>
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
    ../src/scheduler/task_execution_result.cpp
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
    ../src/server/continuous_query_api_handler.cpp
    ../src/server/shard_repair_api_handler.cpp
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
    ../src/server/grpc_web_proxy_handler.cpp
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
    $<$<BOOL:${THEMIS_ENABLE_GRAPHQL}>:../src/server/graphql_api_handler.cpp>

    # WebSocket change-stream handler (conditional)
    $<$<BOOL:${THEMIS_ENABLE_WEBSOCKET}>:../src/api/ws_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_WEBSOCKET}>:../src/api/graphql_ws_handler.cpp>

    # gRPC API server alongside REST (conditional)
    $<$<BOOL:${THEMIS_ENABLE_GRPC}>:../src/api/grpc_server.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GRPC}>:../src/api/themisdb_grpc_service.cpp>
    
    # Network protocol server (themis::network – backward-compatible implementation)
    ../src/network/wire_protocol_server.cpp
        # New wire protocol implementation (themis::wire namespace, protobuf-aware)
        ../src/themis/wire_protocol_server.cpp
    ../src/network/qos_manager.cpp
    ../src/network/raft_load_balancer.cpp
    ../src/network/wire_protocol_helpers.cpp
    ../src/network/wire_protocol_connection_pool.cpp
    ../src/network/wire_protocol_v2.cpp
    ../src/network/wire_protocol_performance.cpp
    ../src/network/wire_protocol_zero_copy.cpp
    ../src/network/wire_protocol_batch.cpp
    ../src/network/io_uring_batcher.cpp
    ../src/network/connection_compression.cpp
    $<$<BOOL:${THEMIS_ENABLE_HTTP3}>:../src/network/quic_transport.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GRPC}>:../src/network/grpc_transport.cpp>
    ../src/network/geo_topology_router.cpp
    ../src/network/socket_timeout_manager.cpp
    ../src/network/adaptive_circuit_breaker.cpp
    ../src/network/udp_fast_path.cpp
    ../src/network/retry_policy.cpp
    ../src/network/wire_retry_policy.cpp
    ../src/network/multipath_tcp.cpp
    ../src/network/bbr_congestion_control.cpp
    ../src/network/network_observability.cpp
    $<$<BOOL:${THEMIS_ENABLE_WEBSOCKET}>:../src/network/wire_protocol_server_ws.cpp>
    $<$<BOOL:${THEMIS_ENABLE_SERVICE_MESH}>:../src/network/service_mesh.cpp>
    $<$<BOOL:${THEMIS_ENABLE_SERVICE_MESH}>:../src/network/envoy_xds.cpp>
    ../src/network/kernel_bypass.cpp
    $<$<BOOL:${THEMIS_ENABLE_HTTP3}>:../src/network/quic_server.cpp>

    # Modular globals shared across handlers
    ../src/server/hsm_provider_global.cpp
    ../src/server/workload_fingerprint_engine.cpp
    
    # Observability (GAP-008: Alertmanager integration + full stack)
    ../src/observability/alertmanager.cpp
    # Observability: persistent provenance storage / export support (GAP-4.1)
    ../src/observability/provenance_store.cpp
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
    ../src/observability/log_search_engine.cpp
    ../src/observability/tenant_metrics_namespace.cpp
    # RPC service implementation (handleGet/handlePut/handleQuery/handleVectorSearch)
    ../src/server/rpc/rpc_service_impl.cpp
)

if(NOT MessagePack_FOUND)
    list(REMOVE_ITEM THEMIS_NETWORK_SOURCES
        ../src/server/buffer_binary_protocol.cpp
    )
    message(STATUS "MessagePack not found: excluding buffer_binary_protocol.cpp from themis_network")
endif()

if(NOT Protobuf_FOUND)
    list(REMOVE_ITEM THEMIS_NETWORK_SOURCES
        ../src/themis/wire_protocol_server.cpp
    )
    message(WARNING "Protobuf not found: excluding themis::wire protocol server source from themis_network")
endif()

set(THEMIS_GEO_SOURCES
    # Geospatial processing
    ../src/geo/geo_json_geometry.cpp
    ../src/geo/raster_query_interface.cpp
    ../src/geo/rtree_cursor.cpp
    ../src/geo/spatial_join_filter.cpp
    ../src/geo/temporal_spatial_query_builder.cpp
    ../src/acceleration/geo_acceleration_bridge.cpp
    ../src/index/spatial_index.cpp
    ../src/api/geo_index_hooks.cpp
    ../src/api/tracing_middleware.cpp
    ../src/api/otlp_exporter.cpp
    ../src/core/concerns/prometheus_metrics.cpp
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
        ../src/gpu/gpu_backend_dispatch_diagnostics.cpp
        ../src/gpu/config.cpp
        ../src/gpu/feature_flags.cpp
        ../src/gpu/memory_pool.cpp
        ../src/gpu/kernel_validator.cpp
        $<$<BOOL:${THEMIS_ENABLE_CUDA}>:../src/gpu/cuda_operations.cpp>
        ../src/gpu/policy.cpp
        ../src/gpu/alerts.cpp
        ../src/gpu/launcher.cpp
        ../src/gpu/stream_manager.cpp
        ../src/gpu/gpu_safe_raii.cpp
        ../src/gpu/kernel_timeout_enforcer.cpp
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
    ../src/graph/graph_query_optimizer.cpp
    ../src/graph/explain_plan.cpp
    ../src/index/graph_analytics.cpp
    ../src/query/result_stream.cpp
    ../src/graph/ontology_manager.cpp
    ../src/graph/knowledge_graph_reasoner.cpp
    ../src/graph/rotate_completion.cpp
    ../src/graph/path_constraints.cpp
    ../src/graph/distributed_graph.cpp
    ../src/graph/gpu_traversal.cpp
    ../src/graph/parallel_traversal.cpp
    ../src/graph/scheduled_edge_refresh.cpp
    ../src/graph/graph_query_rewriter.cpp
    ../src/graph/graph_query_cache.cpp
)

# Function to build modular architecture (post-v1.3.0)
function(themis_build_modular)
    message(STATUS "Building ThemisDB with modular architecture")
    if(THEMIS_ENABLE_CONTENT AND NOT THEMIS_MODULE_CONTENT)
        set(THEMIS_MODULE_CONTENT ON CACHE BOOL "Include content processors module (optional)" FORCE)
        message(STATUS "THEMIS_ENABLE_CONTENT is ON -> forcing THEMIS_MODULE_CONTENT=ON for modular consistency")
    endif()

    # Optional externalization path: strip integrated module sources so optional
    # public submodules can inject their own sources into the modular targets.
    if(THEMIS_USE_EXTERNAL_TIMESERIES_PLUGIN)
        list(FILTER THEMIS_TIMESERIES_SOURCES EXCLUDE REGEX "^\.\./src/timeseries/")
        message(STATUS "Modular externalization active: integrated TimeSeries sources removed from THEMIS_TIMESERIES_SOURCES")
    endif()

    if(THEMIS_USE_EXTERNAL_GEO_PLUGIN)
        list(FILTER THEMIS_GEO_SOURCES EXCLUDE REGEX "^\.\./src/geo/")
        message(STATUS "Modular externalization active: integrated Geo sources removed from THEMIS_GEO_SOURCES")
    endif()
    
    # Core modules (always required)
    set(_themis_base_deps
        OpenSSL::SSL
        OpenSSL::Crypto
        fmt::fmt
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        ${THEMIS_YAML_TARGET}
    )
    if(TARGET Boost::system)
        list(APPEND _themis_base_deps Boost::system)
    endif()
    if(THEMIS_ENABLE_GRPC)
        # Dependencies.cmake already ran find_package(gRPC) with CONFIG+pkg-config fallback
        # and created the gRPC::grpc++ imported target when found. Re-running CONFIG here
        # would reset gRPC_FOUND to FALSE when only pkg-config is available, so guard
        # on target existence instead of re-discovering.
        if(NOT TARGET gRPC::grpc++)
            find_package(gRPC QUIET CONFIG)
        endif()
        # Link gRPC independently: grpc_channel_pool.cpp and related sources are always
        # compiled when THEMIS_ENABLE_GRPC=ON, so the library must be linked regardless
        # of whether protobuf is also available as a CMake target.
        if(TARGET gRPC::grpc++)
            list(APPEND _themis_base_deps gRPC::grpc++)
        endif()
        if(NOT TARGET protobuf::libprotobuf)
            find_package(Protobuf QUIET CONFIG)
        endif()
        if(TARGET protobuf::libprotobuf)
            list(APPEND _themis_base_deps protobuf::libprotobuf)
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
    if(TARGET TBB::tbb)
        list(APPEND _themis_base_deps TBB::tbb)
    endif()
    if(THEMIS_ENABLE_VULKAN AND TARGET Vulkan::Vulkan)
        list(APPEND _themis_base_deps Vulkan::Vulkan)
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
    if(THEMIS_ENABLE_VULKAN)
        target_compile_definitions(themis_base PUBLIC THEMIS_ENABLE_VULKAN)
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
    if(THEMIS_ENABLE_VULKAN AND TARGET Vulkan::Vulkan)
        list(APPEND _themis_storage_deps Vulkan::Vulkan)
    endif()
    if(DEFINED THEMIS_LZ4_TARGET AND NOT "${THEMIS_LZ4_TARGET}" STREQUAL "")
        list(APPEND _themis_storage_deps ${THEMIS_LZ4_TARGET})
    endif()
    if(DEFINED THEMIS_SNAPPY_TARGET AND NOT "${THEMIS_SNAPPY_TARGET}" STREQUAL "")
        list(APPEND _themis_storage_deps ${THEMIS_SNAPPY_TARGET})
    endif()

    themis_add_module(storage
        SOURCES ${THEMIS_STORAGE_SOURCES}
        DEPENDENCIES ${_themis_storage_deps}
    )

    if(DEFINED THEMIS_LZ4_TARGET AND NOT "${THEMIS_LZ4_TARGET}" STREQUAL "")
        target_compile_definitions(themis_storage PUBLIC THEMIS_HAS_LZ4)
    endif()
    if(DEFINED THEMIS_SNAPPY_TARGET AND NOT "${THEMIS_SNAPPY_TARGET}" STREQUAL "")
        target_compile_definitions(themis_storage PUBLIC THEMIS_HAS_SNAPPY)
    endif()

    if(THEMIS_HAS_ROCKSDB_TENSOR)
        target_compile_definitions(themis_storage PUBLIC THEMIS_HAS_ROCKSDB_TENSOR)
    endif()

    if(THEMIS_HAS_IO_URING AND THEMIS_IO_URING_LIB)
        target_link_libraries(themis_storage PRIVATE ${THEMIS_IO_URING_LIB})
        if(THEMIS_IO_URING_INCLUDE)
            target_include_directories(themis_storage PRIVATE ${THEMIS_IO_URING_INCLUDE})
        endif()
    endif()

    if(THEMIS_ENABLE_VULKAN)
        target_compile_definitions(themis_storage PUBLIC THEMIS_ENABLE_VULKAN)
    endif()
    
    set(_themis_security_deps
        themis_base
        themis_storage
        OpenSSL::SSL
        OpenSSL::Crypto
    )
        # Ensure pugixml is found before checking for its targets
        # (this module may be included before find_package(pugixml) is called in CMakeLists.txt)
        if(NOT TARGET pugixml::shared AND NOT TARGET pugixml::pugixml AND NOT TARGET pugixml::static AND NOT TARGET pugixml)
            find_package(pugixml CONFIG QUIET)
        endif()
        if(pugixml_FOUND AND NOT TARGET pugixml::pugixml AND NOT TARGET pugixml::static AND NOT TARGET pugixml)
            find_path(PUGIXML_INCLUDE_DIR NAMES pugixml.hpp PATH_SUFFIXES include)
            find_library(PUGIXML_LIB NAMES pugixml libpugixml)
            if(PUGIXML_INCLUDE_DIR AND PUGIXML_LIB)
                add_library(pugixml UNKNOWN IMPORTED)
                set_target_properties(pugixml PROPERTIES
                    IMPORTED_LOCATION "${PUGIXML_LIB}"
                    INTERFACE_INCLUDE_DIRECTORIES "${PUGIXML_INCLUDE_DIR}")
                add_library(pugixml::pugixml ALIAS pugixml)
            endif()
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
    
    # ── liboqs (post-quantum crypto) ─────────────────────────────────────────
    if(THEMIS_HAS_OQS)
        find_package(liboqs CONFIG QUIET)
        if(liboqs_FOUND OR TARGET OQS::oqs)
            list(APPEND _themis_security_deps OQS::oqs)
            message(STATUS "Wave-2: liboqs found – THEMIS_HAS_OQS enabled")
        else()
            find_library(_OQS_LIB NAMES oqs liboqs)
            find_path(_OQS_INC NAMES oqs/oqs.h)
            if(_OQS_LIB AND _OQS_INC)
                add_library(OQS::oqs UNKNOWN IMPORTED)
                set_target_properties(OQS::oqs PROPERTIES
                    IMPORTED_LOCATION "${_OQS_LIB}"
                    INTERFACE_INCLUDE_DIRECTORIES "${_OQS_INC}")
                list(APPEND _themis_security_deps OQS::oqs)
                message(STATUS "Wave-2: liboqs found (manual) – THEMIS_HAS_OQS enabled")
            else()
                message(WARNING "THEMIS_HAS_OQS=ON but liboqs not found; disabling")
                set(THEMIS_HAS_OQS OFF)
            endif()
        endif()
    endif()

    themis_add_module(security
        SOURCES ${THEMIS_SECURITY_SOURCES}
        DEPENDENCIES ${_themis_security_deps}
    )

    # ── Wave-2 compile definitions for security module ────────────────────────
    if(THEMIS_USE_OPENSSL_TSA)
        target_compile_definitions(themis_security PUBLIC THEMIS_USE_OPENSSL_TSA)
        message(STATUS "Wave-2: THEMIS_USE_OPENSSL_TSA enabled (RFC 3161 via OpenSSL)")
    endif()
    if(THEMIS_HAS_OQS)
        target_compile_definitions(themis_security PUBLIC THEMIS_HAS_OQS)
    endif()
    if(THEMIS_ENABLE_HSM_REAL)
        target_compile_definitions(themis_security PUBLIC THEMIS_ENABLE_HSM_REAL)
        message(STATUS "Wave-2: THEMIS_ENABLE_HSM_REAL enabled (PKCS#11 HSM backend)")
    endif()
    if(THEMIS_HAS_LORA_CLASSIFIER)
        target_compile_definitions(themis_security PUBLIC THEMIS_HAS_LORA_CLASSIFIER)
        message(STATUS "Wave-2: THEMIS_HAS_LORA_CLASSIFIER enabled (LoRA intent classifier)")
    endif()
    if(THEMIS_HAS_HIREDIS)
        target_compile_definitions(themis_security PUBLIC THEMIS_HAS_HIREDIS)
    endif()

    # These two translation units include different governance headers that both
    # declare a class named ComplianceReporter. In Unity mode they can end up in
    # one TU and trigger class redefinition errors (C2011/C2027 cascade).
    set_source_files_properties(
        ${CMAKE_SOURCE_DIR}/src/governance/compliance_reporter.cpp
        ${CMAKE_SOURCE_DIR}/src/governance/compliance_reporting.cpp
        ${CMAKE_SOURCE_DIR}/src/governance/policy_validator.cpp
        ${CMAKE_SOURCE_DIR}/src/governance/policy_validation.cpp
        ${CMAKE_SOURCE_DIR}/src/governance/policy_review.cpp
        ${CMAKE_SOURCE_DIR}/src/governance/review_scheduler.cpp
        PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON
    )
    
    themis_add_module(transaction
        SOURCES ${THEMIS_TRANSACTION_SOURCES}
        DEPENDENCIES 
            themis_base 
            themis_storage
            themis_security
    )

    if(DEFINED THEMIS_ZSTD_TARGET AND NOT "${THEMIS_ZSTD_TARGET}" STREQUAL "")
        target_link_libraries(themis_transaction PUBLIC ${THEMIS_ZSTD_TARGET})
        target_compile_definitions(themis_transaction PUBLIC THEMIS_HAS_ZSTD)
    endif()

    set(_themis_query_deps
        themis_base
        themis_storage
        themis_transaction
        themis_security
    )
    if(TARGET themis_process)
        list(APPEND _themis_query_deps themis_process)
    endif()

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
    # NOTE: themis_llm only includes query/adaptive_optimizer.h (header-only)
    # for runtime registration. No compile-time link dependency needed.
    # Excluding themis_llm from themis_query deps avoids circular dependency:
    # themis_query → themis_llm → themis_graph would cycle back to themis_query.
    if(THEMIS_MODULE_GEO)
        list(APPEND _themis_query_deps themis_geo)
    endif()
    # Note: themis_llm is intentionally NOT added here to avoid creating
    # cyclic inter-target dependencies with themis_sharding. When LLM symbols
    # are required at link time, higher-level shared targets (e.g. themis_sharding)
    # should link both themis_query and themis_llm so the final link resolves
    # transitive references without introducing cycles among static libs.
    if(onnxruntime_FOUND)
        list(APPEND _themis_query_deps onnxruntime::onnxruntime)
    endif()
    if(TARGET httplib::httplib)
        list(APPEND _themis_query_deps httplib::httplib)
    endif()
    
    themis_add_module(query
        SOURCES ${THEMIS_QUERY_SOURCES}
        DEPENDENCIES ${_themis_query_deps}
        STATIC_MODULE
    )
    # ── Wave-2 compile definitions for query/cache module ─────────────────────
    if(THEMIS_HAS_HIREDIS)
        target_compile_definitions(themis_query PUBLIC THEMIS_HAS_HIREDIS)
        message(STATUS "Wave-2: THEMIS_HAS_HIREDIS enabled (direct Redis SET/GET/DEL/EXPIRE)")
    endif()
    if(MSVC)
        # Keep XML parser TUs separate: both files define helper types in
        # anonymous namespaces and can conflict when merged into one Unity TU.
        set_source_files_properties(
            ${CMAKE_SOURCE_DIR}/src/process/bpmn_serializer.cpp
            ${CMAKE_SOURCE_DIR}/src/process/epk_aris_xml_importer.cpp
            ${CMAKE_SOURCE_DIR}/src/process/cmmn_serializer.cpp
            ${CMAKE_SOURCE_DIR}/src/process/fim_importer.cpp
            ${CMAKE_SOURCE_DIR}/src/cache/distributed_cache_coordinator.cpp
            # Keep AQL translator-related TUs outside Unity batches to avoid
            # namespace bleed-through from concatenated query units.
            ${CMAKE_SOURCE_DIR}/src/query/aql_translator.cpp
            ${CMAKE_SOURCE_DIR}/src/aql/aql_optimizer_advisor.cpp
            PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON
        )
    endif()

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
        if(THEMIS_MODULE_LLM_SPLIT)
            list(APPEND _themis_network_deps themis_llm_ext)
        endif()
    endif()
    if(TARGET themis_training)
        list(APPEND _themis_network_deps themis_training)
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
    if(TARGET themis_api_proto)
        list(APPEND _themis_network_deps themis_api_proto)
    endif()
    if(WIN32)
        list(APPEND _themis_network_deps Dbghelp)
    endif()

    # LNK1189 fix: >65535 exported symbols (WINDOWS_EXPORT_ALL_SYMBOLS ON) exceed
    # the MSVC import-library limit. STATIC_MODULE avoids auto-export entirely;
    # the server exe links it transitively through themis_core INTERFACE.
    themis_add_module(network
        SOURCES ${THEMIS_NETWORK_SOURCES}
        DEPENDENCIES ${_themis_network_deps}
        STATIC_MODULE
    )
    if(MSVC)
        set_target_properties(themis_network PROPERTIES
            UNITY_BUILD OFF
        )
        # Files that must NOT enter a unity batch:
        # - monitoring_api_handler / index_api_handler: need per-file /bigobj;/Od
        # - distributed_flame_graph: defines 'parseFolded' in anonymous namespace
        #   which causes C2375 (redefinition / different linkage) when merged with
        #   another TU that also defines an anonymous-namespace symbol of the same name
        set_source_files_properties(
            ${CMAKE_SOURCE_DIR}/src/server/monitoring_api_handler.cpp
            ${CMAKE_SOURCE_DIR}/src/server/index_api_handler.cpp
            PROPERTIES
                SKIP_UNITY_BUILD_INCLUSION ON
                COMPILE_OPTIONS "/bigobj;/Zm200;$<$<NOT:$<CONFIG:Release>>:/Od>"
        )
        set_source_files_properties(
            ${CMAKE_SOURCE_DIR}/src/observability/distributed_flame_graph.cpp
            PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON
        )
        # task_scheduler_api_handler.cpp defines an anonymous-namespace
        # timePointToIso that conflicts with the namespace-level version in
        # async_job_api_handler.cpp when both are merged into the same Unity TU.
        set_source_files_properties(
            ${CMAKE_SOURCE_DIR}/src/server/task_scheduler_api_handler.cpp
            PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON
        )
        # cdc_admin.cpp pulls in cdc::TenantConfig which is ambiguous with
        # themis::TenantConfig from server/tenant_manager.h in the same Unity TU.
        set_source_files_properties(
            ${CMAKE_SOURCE_DIR}/src/cdc/cdc_admin.cpp
            PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON
        )
        # WinSock include-order and namespace-heavy handlers are sensitive to
        # unity TU concatenation; compile them standalone to avoid conflicts.
        set_source_files_properties(
            ${CMAKE_SOURCE_DIR}/src/utils/tracing.cpp
            ${CMAKE_SOURCE_DIR}/src/server/graphql_api_handler.cpp
            ${CMAKE_SOURCE_DIR}/src/server/ethics_api_handler.cpp
            ${CMAKE_SOURCE_DIR}/src/network/wire_protocol_server.cpp
            PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON
        )
            # api_gateway.cpp defines an anonymous-namespace 'class Error' that
            # becomes ambiguous with themis::Error when merged with other server files.
            set_source_files_properties(
                ${CMAKE_SOURCE_DIR}/src/server/api_gateway.cpp
                PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON
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
                themis_query
                themis_transaction
        )
        # Link the lightweight LLM API module (if built) to provide
        # runtime-facing LLM symbols used by query/sharding without
        # pulling in the full LLM implementation that depends on sharding.
        # Prefer linking the full themis_llm target if available; it no
        # longer depends on themis_sharding (avoid cycles). Fall back to
        # themis_llm_api if a split API module is used.
        if(TARGET themis_llm)
            target_link_libraries(themis_sharding PRIVATE themis_llm)
            if(THEMIS_MODULE_LLM_SPLIT AND TARGET themis_llm_ext)
                target_link_libraries(themis_sharding PRIVATE themis_llm_ext)
            endif()
        elseif(TARGET themis_llm_api)
            target_link_libraries(themis_sharding PRIVATE themis_llm_api)
        endif()
        # Ensure proto files are generated before compiling sharding sources
        if(TARGET themis_shard_proto)
            add_dependencies(themis_sharding themis_shard_proto)
            target_link_libraries(themis_sharding PRIVATE themis_shard_proto protobuf::libprotobuf)
            if(THEMIS_ENABLE_GRPC AND TARGET gRPC::grpc++)
                # Ensure Abseil package is found before referencing component targets.
                if(NOT TARGET absl::abseil_dll)
                    find_package(absl CONFIG REQUIRED)
                endif()

                # Link gRPC directly, and only add the Abseil DLL target when the
                # platform/package layout actually provides it.
                target_link_libraries(themis_sharding PRIVATE gRPC::grpc++)
                if(TARGET absl::abseil_dll)
                    target_link_libraries(themis_sharding PRIVATE absl::abseil_dll)
                endif()
                if(MSVC)
                    if(TARGET absl::absl_log)
                        set(_sharding_absl_links
                            absl::absl_log
                            absl::hash
                            absl::raw_logging_internal
                            absl::strings
                        )
                        if(TARGET protobuf::libupb)
                            list(APPEND _sharding_absl_links protobuf::libupb)
                        endif()
                        list(APPEND _sharding_absl_links "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows/lib/abseil_dll.lib")
                        target_link_libraries(themis_sharding PRIVATE ${_sharding_absl_links})
                    endif()
                endif()
            endif()
            # Add include directory for generated proto headers
            target_include_directories(themis_sharding PRIVATE ${CMAKE_BINARY_DIR}/proto_generated)
            if(MSVC)
                # Generated proto headers can surface size_t->int narrowing warnings in consumers.
                target_compile_options(themis_sharding PRIVATE /wd4267)
            endif()
            message(STATUS "themis_sharding linked to themis_shard_proto (protobuf messages always, gRPC optional)")
        endif()

        if(THEMIS_ENABLE_CUDA)
            target_compile_definitions(themis_sharding PUBLIC THEMIS_ENABLE_CUDA)
        endif()
        if(THEMIS_ENABLE_OPENCL)
            target_compile_definitions(themis_sharding PUBLIC THEMIS_ENABLE_OPENCL)
            if(TARGET OpenCL::OpenCL)
                target_link_libraries(themis_sharding PUBLIC OpenCL::OpenCL)
            endif()
        endif()
        
        # Cloud SDK integration (optional)
        if(THEMIS_WITH_S3_SDK)
            find_package(AWSSDK CONFIG QUIET COMPONENTS s3)
            if(AWSSDK_FOUND)
                target_compile_definitions(themis_sharding PRIVATE THEMIS_WITH_S3_SDK)
                target_link_libraries(themis_sharding PRIVATE AWS::s3)
                message(STATUS "S3 cloud provider enabled (AWS SDK)")
            else()
                message(WARNING "THEMIS_WITH_S3_SDK requested but AWS SDK not found - S3 provider will be unavailable")
            endif()
        endif()
        
        if(THEMIS_WITH_AZURE_SDK)
            find_package(azure-storage-blobs-cpp CONFIG QUIET)
            if(azure-storage-blobs-cpp_FOUND)
                target_compile_definitions(themis_sharding PRIVATE THEMIS_WITH_AZURE_SDK)
                target_link_libraries(themis_sharding PRIVATE Azure::Storage::Blobs)
                message(STATUS "Azure Blob Storage provider enabled (Azure SDK)")
            else()
                message(WARNING "THEMIS_WITH_AZURE_SDK requested but Azure SDK not found - Azure provider will be unavailable")
            endif()
        endif()
        
        if(THEMIS_WITH_GCS_SDK)
            find_package(google-cloud-cpp CONFIG QUIET COMPONENTS storage)
            if(google-cloud-cpp_FOUND)
                target_compile_definitions(themis_sharding PRIVATE THEMIS_WITH_GCS_SDK)
                target_link_libraries(themis_sharding PRIVATE google-cloud-cpp::storage)
                message(STATUS "Google Cloud Storage provider enabled (GCS SDK)")
            else()
                message(WARNING "THEMIS_WITH_GCS_SDK requested but Google Cloud SDK not found - GCS provider will be unavailable")
            endif()
        endif()
        
        # Add cloud SDK integration source if any provider is enabled
        if(THEMIS_WITH_S3_SDK OR THEMIS_WITH_AZURE_SDK OR THEMIS_WITH_GCS_SDK)
            target_sources(themis_sharding PRIVATE ../src/sharding/cloud_sdk_integration.cpp)
        endif()
    endif()
    
    if(THEMIS_MODULE_TIMESERIES)
        themis_add_module(timeseries
            SOURCES ${THEMIS_TIMESERIES_SOURCES}
            DEPENDENCIES 
                themis_base 
                themis_storage
                themis_security
        )
    endif()
    
    if(THEMIS_MODULE_LLM)
        # Small API module providing DocsAssistant/EmbeddedLLM/ThemisHelpLoRA
        themis_add_module(llm_api
            STATIC_MODULE
            DISABLE_AUTO_EXPORT
            SOURCES ${THEMIS_LLM_API_SOURCES}
            DEPENDENCIES
                themis_base
                themis_storage
                themis_security
        )

        themis_add_module(llm
            STATIC_MODULE
            DISABLE_AUTO_EXPORT
            SOURCES ${THEMIS_LLM_CORE_SOURCES}
            DEPENDENCIES 
                themis_base 
                themis_storage
                themis_security
                themis_query
        )
        if(THEMIS_MODULE_LLM_SPLIT AND THEMIS_LLM_EXT_SOURCES)
            themis_add_module(llm_ext
                STATIC_MODULE
                DISABLE_AUTO_EXPORT
                SOURCES ${THEMIS_LLM_EXT_SOURCES}
                DEPENDENCIES
                    themis_base
                    themis_storage
                    themis_security
                    themis_llm
                    themis_query
            )
        endif()
        if(THEMIS_MODULE_GRAPH)
            target_link_libraries(themis_llm PUBLIC themis_graph)
        endif()
        if(onnxruntime_FOUND)
            target_link_libraries(themis_llm PUBLIC onnxruntime::onnxruntime)
            if(THEMIS_MODULE_LLM_SPLIT AND TARGET themis_llm_ext)
                target_link_libraries(themis_llm_ext PUBLIC onnxruntime::onnxruntime)
            endif()
        endif()
        if(THEMIS_ENABLE_MIMALLOC AND TARGET mimalloc)
            target_link_libraries(themis_llm PUBLIC mimalloc)
            if(THEMIS_MODULE_LLM_SPLIT AND TARGET themis_llm_ext)
                target_link_libraries(themis_llm_ext PUBLIC mimalloc)
            endif()
        endif()
        if(THEMIS_ENABLE_JEMALLOC)
            if(TARGET jemalloc::jemalloc)
                target_link_libraries(themis_llm PUBLIC jemalloc::jemalloc)
                if(THEMIS_MODULE_LLM_SPLIT AND TARGET themis_llm_ext)
                    target_link_libraries(themis_llm_ext PUBLIC jemalloc::jemalloc)
                endif()
            elseif(jemalloc_LIBRARIES)
                target_link_libraries(themis_llm PUBLIC ${jemalloc_LIBRARIES})
                if(THEMIS_MODULE_LLM_SPLIT AND TARGET themis_llm_ext)
                    target_link_libraries(themis_llm_ext PUBLIC ${jemalloc_LIBRARIES})
                endif()
            endif()
        endif()
        if(TARGET llama)
            target_link_libraries(themis_llm PUBLIC llama)
            if(THEMIS_MODULE_LLM_SPLIT AND TARGET themis_llm_ext)
                target_link_libraries(themis_llm_ext PUBLIC llama)
            endif()
        elseif(llama_LIBRARIES)
            target_link_libraries(themis_llm PUBLIC ${llama_LIBRARIES})
            if(THEMIS_MODULE_LLM_SPLIT AND TARGET themis_llm_ext)
                target_link_libraries(themis_llm_ext PUBLIC ${llama_LIBRARIES})
            endif()
        endif()
        if(THEMIS_ENABLE_VULKAN)
            target_compile_definitions(themis_llm PUBLIC THEMIS_ENABLE_VULKAN)
            if(THEMIS_MODULE_LLM_SPLIT AND TARGET themis_llm_ext)
                target_compile_definitions(themis_llm_ext PUBLIC THEMIS_ENABLE_VULKAN)
            endif()
        endif()
        if(THEMIS_ENABLE_VULKAN AND TARGET Vulkan::Vulkan)
            target_link_libraries(themis_llm PUBLIC Vulkan::Vulkan)
            if(THEMIS_MODULE_LLM_SPLIT AND TARGET themis_llm_ext)
                target_link_libraries(themis_llm_ext PUBLIC Vulkan::Vulkan)
            endif()
        endif()
        if(DEFINED THEMIS_ROCKSDB_TARGET AND NOT "${THEMIS_ROCKSDB_TARGET}" STREQUAL "")
            target_link_libraries(themis_llm PUBLIC ${THEMIS_ROCKSDB_TARGET})
            if(THEMIS_MODULE_LLM_SPLIT AND TARGET themis_llm_ext)
                target_link_libraries(themis_llm_ext PUBLIC ${THEMIS_ROCKSDB_TARGET})
            endif()
        endif()
    endif()

    if(THEMIS_TRAINING_SOURCES)
        set(_themis_training_deps
            themis_base
            themis_storage
            themis_security
            themis_query
        )
        if(THEMIS_MODULE_GRAPH)
            list(APPEND _themis_training_deps themis_graph)
        endif()
        if(THEMIS_MODULE_LLM)
            list(APPEND _themis_training_deps themis_llm)
            if(THEMIS_MODULE_LLM_SPLIT AND TARGET themis_llm_ext)
                list(APPEND _themis_training_deps themis_llm_ext)
            endif()
        endif()

        themis_add_module(training
            STATIC_MODULE
            SOURCES ${THEMIS_TRAINING_SOURCES}
            DEPENDENCIES ${_themis_training_deps}
        )

        if(onnxruntime_FOUND)
            target_link_libraries(themis_training PUBLIC onnxruntime::onnxruntime)
        endif()
        if(THEMIS_ENABLE_MIMALLOC AND TARGET mimalloc)
            target_link_libraries(themis_training PUBLIC mimalloc)
        endif()
        if(THEMIS_ENABLE_JEMALLOC)
            if(TARGET jemalloc::jemalloc)
                target_link_libraries(themis_training PUBLIC jemalloc::jemalloc)
            elseif(jemalloc_LIBRARIES)
                target_link_libraries(themis_training PUBLIC ${jemalloc_LIBRARIES})
            endif()
        endif()
        if(THEMIS_ENABLE_VULKAN)
            target_compile_definitions(themis_training PUBLIC THEMIS_ENABLE_VULKAN)
        endif()
        if(THEMIS_ENABLE_VULKAN AND TARGET Vulkan::Vulkan)
            target_link_libraries(themis_training PUBLIC Vulkan::Vulkan)
        endif()
        if(DEFINED THEMIS_ROCKSDB_TARGET AND NOT "${THEMIS_ROCKSDB_TARGET}" STREQUAL "")
            target_link_libraries(themis_training PUBLIC ${THEMIS_ROCKSDB_TARGET})
        endif()
    endif()
    
    if(THEMIS_MODULE_GEO)
        set(_themis_geo_deps
            themis_base
            themis_storage
            themis_transaction
            themis_timeseries
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
            themis_security
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
            themis_ingestion
        )
        if(TARGET yaml-cpp::yaml-cpp)
            list(APPEND _themis_content_deps yaml-cpp::yaml-cpp)
        endif()
        if(THEMIS_MODULE_GRAPH)
            list(APPEND _themis_content_deps themis_graph)
        endif()
        if(THEMIS_MODULE_LLM)
            list(APPEND _themis_content_deps themis_llm)
            if(THEMIS_MODULE_LLM_SPLIT)
                list(APPEND _themis_content_deps themis_llm_ext)
            endif()
        endif()
        if(TARGET themis_query)
            list(APPEND _themis_content_deps themis_query)
        endif()
        if(THEMIS_MODULE_SHARDING)
            list(APPEND _themis_content_deps themis_sharding)
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
        if(THEMIS_MODULE_LLM_SPLIT)
            list(APPEND _themis_ingestion_deps themis_llm_ext)
        endif()
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

    # Ensure sharding links query then the lightweight llm api (link order matters)
    if(TARGET themis_sharding)
        if(TARGET themis_query)
            target_link_libraries(themis_sharding PRIVATE themis_query)
        endif()
        # Provide process serialization helpers used by query/sharding at link time
        if(TARGET themis_process)
            target_link_libraries(themis_sharding PRIVATE themis_process)
        endif()
        # Prefer linking the full themis_llm implementation when available
        # to avoid duplicate adapter symbols from the lightweight API module.
        if(TARGET themis_llm)
            target_link_libraries(themis_sharding PRIVATE themis_llm)
            if(THEMIS_MODULE_LLM_SPLIT AND TARGET themis_llm_ext)
                target_link_libraries(themis_sharding PRIVATE themis_llm_ext)
            endif()
        elseif(TARGET themis_llm_api)
            target_link_libraries(themis_sharding PRIVATE themis_llm_api)
        endif()
        # Graph/ontology helpers may be referenced by sharding (e.g. validation,
        # ontology lookups). Link graph module when available so those symbols
        # are provided at final link time.
        if(TARGET themis_graph)
            target_link_libraries(themis_sharding PRIVATE themis_graph)
        endif()
    endif()
    
    if(THEMIS_MODULE_SHARDING)
        list(APPEND THEMIS_ALL_MODULES themis_sharding)
    endif()
    
    if(THEMIS_MODULE_TIMESERIES)
        list(APPEND THEMIS_ALL_MODULES themis_timeseries)
    endif()
    
    if(THEMIS_MODULE_LLM)
        list(APPEND THEMIS_ALL_MODULES themis_llm)
        if(THEMIS_MODULE_LLM_SPLIT)
            list(APPEND THEMIS_ALL_MODULES themis_llm_ext)
        endif()
    endif()

    if(TARGET themis_training)
        list(APPEND THEMIS_ALL_MODULES themis_training)
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
