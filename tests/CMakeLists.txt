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
list(FILTER ALL_TEST_SOURCES EXCLUDE REGEX ".*test_multi_shard_transactions\\.cpp$")

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
        ".*test_model_loader_async\\.cpp$"
        ".*test_phase1_flash_attention\\.cpp$"
        ".*test_phase1_kv_cache_reuse\\.cpp$"
        ".*test_ai_decision_auditor\\.cpp$"
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
    )
endif()

# Missing implementations and unfinished features
list(APPEND TEST_EXCLUDE_PATTERNS
    ".*test_merge_operator.*\\.cpp$"
    ".*test_shard_durability.*\\.cpp$"
    ".*test_gpu_erasure_coding.*\\.cpp$"
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
    ".*test_api_version\\.cpp$"
)
foreach(pattern IN LISTS TEST_EXCLUDE_PATTERNS)
    list(FILTER ALL_TEST_SOURCES EXCLUDE REGEX "${pattern}")
endforeach()

list(LENGTH ALL_TEST_SOURCES TEST_SOURCE_COUNT)
message(STATUS "Building ${TEST_SOURCE_COUNT} test files after feature gating")

add_executable(themis_tests ${ALL_TEST_SOURCES})

target_link_libraries(themis_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    themis_core
    spdlog::spdlog
    fmt::fmt
    nlohmann_json::nlohmann_json
    RocksDB::rocksdb
    OpenSSL::SSL
    OpenSSL::Crypto
    CURL::libcurl
    protobuf::libprotobuf
    Threads::Threads
)

if(THEMIS_ENABLE_GRPC)
    target_link_libraries(themis_tests PRIVATE gRPC::grpc++)
endif()

if(TARGET llama)
    target_link_libraries(themis_tests PRIVATE llama)
endif()

target_compile_definitions(themis_tests PRIVATE
    THEMIS_TEST_BUILD=1
)

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

# ============================================================================
# Custom Test Targets
# ============================================================================

# Add custom target to run all tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

message(STATUS "Tests configured successfully")
