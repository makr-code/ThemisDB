# ThemisDB Tests CMakeLists.txt
cmake_minimum_required(VERSION 3.20)

# Check if we're building tests
if(NOT THEMIS_BUILD_TESTS)
    message(STATUS "Tests disabled (THEMIS_BUILD_TESTS=OFF)")
    return()
endif()

message(STATUS "Configuring ThemisDB test suite")

# Ensure consistent MSVC Runtime with vcpkg triplet
# NOTE: x64-windows uses /MD (dynamic runtime), x64-windows-static uses /MT (static runtime)
# Don't override CMAKE_MSVC_RUNTIME_LIBRARY here - let it match the vcpkg triplet
if(MSVC)
    message(STATUS "MSVC Runtime: ${CMAKE_MSVC_RUNTIME_LIBRARY} (from vcpkg triplet)")
endif()

# Optional toggles for unstable/legacy suites
option(THEMIS_ENABLE_LORA_TESTS "Build experimental LoRA framework tests" OFF)

# Find GTest
find_package(GTest QUIET CONFIG)
if(NOT GTest_FOUND)
    message(WARNING "GTest not found - tests will not be built")
    message(WARNING "Install GTest or use vcpkg: vcpkg install gtest")
    return()
endif()

message(STATUS "GTest found - building tests")

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${GTest_INCLUDE_DIRS}
)

# Common test libraries
set(TEST_LIBS
    GTest::gtest
    GTest::gtest_main
)

# ============================================================================
# Core Utilities Tests
# ============================================================================

# Error Handling Tests (tl::expected wrapper)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_expected.cpp")
    message(STATUS "Adding error handling tests (tl::expected)")
    
    add_executable(test_expected
        test_expected.cpp
    )
    
    target_link_libraries(test_expected PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        fmt::fmt
        nlohmann_json::nlohmann_json
    )
    
    target_compile_definitions(test_expected PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME ErrorHandlingTests COMMAND test_expected)
    
    set_tests_properties(ErrorHandlingTests PROPERTIES
        LABELS "core;utilities;error_handling"
        TIMEOUT 60
    )
else()
    message(STATUS "Error handling tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_expected.cpp")
endif()

# ============================================================================
# LoRA Framework Tests (NEW)
# ============================================================================

if(THEMIS_ENABLE_LORA_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_lora_framework.cpp")
        message(STATUS "Adding LoRA framework tests (THEMIS_ENABLE_LORA_TESTS=ON)")
        
        add_executable(test_lora_framework
            test_lora_framework.cpp
        )
        
        target_link_libraries(test_lora_framework PRIVATE
            ${TEST_LIBS}
            themis_core
            OpenSSL::SSL
            OpenSSL::Crypto
            spdlog::spdlog
            nlohmann_json::nlohmann_json
            RocksDB::rocksdb
        )
        
        target_compile_definitions(test_lora_framework PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        # Add to CTest
        add_test(NAME LoRAFrameworkTests COMMAND test_lora_framework)
        
        # Set test properties
        set_tests_properties(LoRAFrameworkTests PROPERTIES
            LABELS "lora;llm;framework"
            TIMEOUT 300
        )
    else()
        message(STATUS "LoRA framework tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_lora_framework.cpp")
    endif()
    
    # LoRA Adapter Application Tests
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_lora_adapter_application.cpp")
        message(STATUS "Adding LoRA adapter application tests (THEMIS_ENABLE_LORA_TESTS=ON)")
        
        add_executable(test_lora_adapter_application
            test_lora_adapter_application.cpp
        )
        
        target_link_libraries(test_lora_adapter_application PRIVATE
            ${TEST_LIBS}
            themis_core
            OpenSSL::SSL
            OpenSSL::Crypto
            spdlog::spdlog
            nlohmann_json::nlohmann_json
            RocksDB::rocksdb
        )
        
        target_compile_definitions(test_lora_adapter_application PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        add_test(NAME LoRAAdapterApplicationTests COMMAND test_lora_adapter_application)
        set_tests_properties(LoRAAdapterApplicationTests PROPERTIES
            LABELS "lora;llm;adapter;inference"
            TIMEOUT 300
        )
    else()
        message(STATUS "LoRA adapter application tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_lora_adapter_application.cpp")
    endif()
    
    # Multi-LoRA Fusion Tests (v1.5.0)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_multi_lora_fusion.cpp")
        message(STATUS "Adding Multi-LoRA fusion tests (THEMIS_ENABLE_LORA_TESTS=ON)")
        
        add_executable(test_multi_lora_fusion
            test_multi_lora_fusion.cpp
        )
        
        target_link_libraries(test_multi_lora_fusion PRIVATE
            ${TEST_LIBS}
            themis_core
            OpenSSL::SSL
            OpenSSL::Crypto
            spdlog::spdlog
            nlohmann_json::nlohmann_json
            RocksDB::rocksdb
        )
        
        target_compile_definitions(test_multi_lora_fusion PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        add_test(NAME MultiLoRAFusionTests COMMAND test_multi_lora_fusion)
        set_tests_properties(MultiLoRAFusionTests PROPERTIES
            LABELS "lora;llm;fusion;adapter;inference"
            TIMEOUT 300
        )
    else()
        message(STATUS "Multi-LoRA fusion tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_multi_lora_fusion.cpp")
    endif()
    
    # LoRA Auto-Binding and Lifecycle Tests
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/llm/test_lora_auto_binding.cpp")
        message(STATUS "Adding LoRA auto-binding and lifecycle tests")
        
        add_executable(test_lora_auto_binding
            llm/test_lora_auto_binding.cpp
        )
        
        target_link_libraries(test_lora_auto_binding PRIVATE
            ${TEST_LIBS}
            themis_core
            OpenSSL::SSL
            OpenSSL::Crypto
            spdlog::spdlog
            nlohmann_json::nlohmann_json
            RocksDB::rocksdb
        )
        
        target_compile_definitions(test_lora_auto_binding PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        add_test(NAME LoRAAutoBindingTests COMMAND test_lora_auto_binding)
        set_tests_properties(LoRAAutoBindingTests PROPERTIES
            LABELS "lora;llm;auto-binding;lifecycle;eviction"
            TIMEOUT 60
        )
    else()
        message(STATUS "LoRA auto-binding tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/llm/test_lora_auto_binding.cpp")
    endif()
    
    # Model Loader Error Handling Tests (Phase 4)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/llm/test_model_loader_error_handling.cpp")
        message(STATUS "Adding Model Loader Error Handling tests (Phase 4)")
        
        add_executable(test_model_loader_error_handling
            llm/test_model_loader_error_handling.cpp
        )
        
        target_link_libraries(test_model_loader_error_handling PRIVATE
            ${TEST_LIBS}
            themis_core
            spdlog::spdlog
            fmt::fmt
            nlohmann_json::nlohmann_json
        )
        
        target_compile_definitions(test_model_loader_error_handling PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        add_test(NAME ModelLoaderErrorHandlingTests COMMAND test_model_loader_error_handling)
        set_tests_properties(ModelLoaderErrorHandlingTests PROPERTIES
            LABELS "llm;error_handling;phase4;model_loader"
            TIMEOUT 60
        )
    else()
        message(STATUS "Model Loader Error Handling tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/llm/test_model_loader_error_handling.cpp")
    endif()
    
    # Distributed Training Coordinator Tests
    if(THEMIS_ENABLE_DISTRIBUTED_TRAINING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_distributed_training_coordinator.cpp")
        message(STATUS "Adding Distributed Training Coordinator tests (THEMIS_ENABLE_DISTRIBUTED_TRAINING=ON)")
        
        add_executable(test_distributed_training_coordinator
            test_distributed_training_coordinator.cpp
        )
        
        target_link_libraries(test_distributed_training_coordinator PRIVATE
            ${TEST_LIBS}
            themis_core
            OpenSSL::SSL
            OpenSSL::Crypto
            spdlog::spdlog
            nlohmann_json::nlohmann_json
            RocksDB::rocksdb
        )
        
        target_compile_definitions(test_distributed_training_coordinator PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        add_test(NAME DistributedTrainingCoordinatorTests COMMAND test_distributed_training_coordinator)
        set_tests_properties(DistributedTrainingCoordinatorTests PROPERTIES
            LABELS "lora;llm;distributed;training;fault-tolerance"
            TIMEOUT 300
        )
    else()
        if(THEMIS_ENABLE_DISTRIBUTED_TRAINING)
            message(STATUS "Distributed training coordinator tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_distributed_training_coordinator.cpp")
        else()
            message(STATUS "Distributed training coordinator tests skipped (THEMIS_ENABLE_DISTRIBUTED_TRAINING=OFF)")
        endif()
    endif()
    
    # Byzantine Detector Tests
    if(THEMIS_ENABLE_DISTRIBUTED_TRAINING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_byzantine_detector.cpp")
        message(STATUS "Adding Byzantine Detector tests (THEMIS_ENABLE_DISTRIBUTED_TRAINING=ON)")
        
        add_executable(test_byzantine_detector
            test_byzantine_detector.cpp
        )
        
        target_link_libraries(test_byzantine_detector PRIVATE
            ${TEST_LIBS}
            themis_core
            OpenSSL::SSL
            OpenSSL::Crypto
            spdlog::spdlog
            fmt::fmt
            nlohmann_json::nlohmann_json
            RocksDB::rocksdb
        )
        
        target_compile_definitions(test_byzantine_detector PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        add_test(NAME ByzantineDetectorTests COMMAND test_byzantine_detector)
        set_tests_properties(ByzantineDetectorTests PROPERTIES
            LABELS "byzantine;security;distributed;training;unit"
            TIMEOUT 120
        )
    else()
        if(THEMIS_ENABLE_DISTRIBUTED_TRAINING)
            message(STATUS "Byzantine detector tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_byzantine_detector.cpp")
        else()
            message(STATUS "Byzantine detector tests skipped (THEMIS_ENABLE_DISTRIBUTED_TRAINING=OFF)")
        endif()
    endif()
    
    # Training Service Registry Tests
    if(THEMIS_ENABLE_DISTRIBUTED_TRAINING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_training_service_registry.cpp")
        message(STATUS "Adding Training Service Registry tests (THEMIS_ENABLE_DISTRIBUTED_TRAINING=ON)")
        
        add_executable(test_training_service_registry
            test_training_service_registry.cpp
        )
        
        target_link_libraries(test_training_service_registry PRIVATE
            ${TEST_LIBS}
            themis_core
            OpenSSL::SSL
            OpenSSL::Crypto
            spdlog::spdlog
            nlohmann_json::nlohmann_json
            RocksDB::rocksdb
        )
        
        target_compile_definitions(test_training_service_registry PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        add_test(NAME TrainingServiceRegistryTests COMMAND test_training_service_registry)
        set_tests_properties(TrainingServiceRegistryTests PROPERTIES
            LABELS "lora;llm;distributed;training;registry;dependency-injection"
            TIMEOUT 60
        )
    else()
        if(THEMIS_ENABLE_DISTRIBUTED_TRAINING)
            message(STATUS "Training service registry tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_training_service_registry.cpp")
        else()
            message(STATUS "Training service registry tests skipped (THEMIS_ENABLE_DISTRIBUTED_TRAINING=OFF)")
        endif()
    endif()
    
    # LoRA Router Tests
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_lora_router.cpp")
        message(STATUS "Adding LoRA Router tests (routing automation)")
        
        add_executable(test_lora_router
            test_lora_router.cpp
        )
        
        target_link_libraries(test_lora_router PRIVATE
            ${TEST_LIBS}
            themis_core
            OpenSSL::SSL
            OpenSSL::Crypto
            spdlog::spdlog
            nlohmann_json::nlohmann_json
            RocksDB::rocksdb
        )
        
        target_compile_definitions(test_lora_router PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        add_test(NAME LoRARouterTests COMMAND test_lora_router)
        set_tests_properties(LoRARouterTests PROPERTIES
            LABELS "lora;llm;router;routing;semantic;ab-test;rollout"
            TIMEOUT 120
        )
    else()
        message(STATUS "LoRA Router tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_lora_router.cpp")
    endif()
    
    # LoRA Storage Key Provider Tests (Refactoring)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_lora_storage_key_provider.cpp")
        message(STATUS "Adding LoRA storage key provider tests")
        
        add_executable(test_lora_storage_key_provider
            test_lora_storage_key_provider.cpp
        )
        
        target_link_libraries(test_lora_storage_key_provider PRIVATE
            ${TEST_LIBS}
            themis_core
            OpenSSL::SSL
            OpenSSL::Crypto
            spdlog::spdlog
            nlohmann_json::nlohmann_json
            RocksDB::rocksdb
        )
        
        target_compile_definitions(test_lora_storage_key_provider PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        add_test(NAME LoRAStorageKeyProviderTests COMMAND test_lora_storage_key_provider)
        set_tests_properties(LoRAStorageKeyProviderTests PROPERTIES
            LABELS "lora;llm;storage;keys"
            TIMEOUT 300
        )
    else()
        message(STATUS "LoRA storage key provider tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_lora_storage_key_provider.cpp")
    endif()
    
    # Fused Kernels Tests (Phase 10.3)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_fused_kernels.cpp" AND (THEMIS_ENABLE_CUDA OR THEMIS_ENABLE_HIP))
        message(STATUS "Adding fused kernels tests (CUDA/HIP enabled)")
        
        add_executable(test_fused_kernels
            test_fused_kernels.cpp
        )
        
        target_link_libraries(test_fused_kernels PRIVATE
            ${TEST_LIBS}
            themis_core
            spdlog::spdlog
        )
        
        target_compile_definitions(test_fused_kernels PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        # Add to CTest
        add_test(NAME FusedKernelsTests COMMAND test_fused_kernels)
        
        # Set test properties
        set_tests_properties(FusedKernelsTests PROPERTIES
            LABELS "lora;gpu;cuda;hip;kernel_fusion"
            TIMEOUT 300
        )
    else()
        if(NOT (THEMIS_ENABLE_CUDA OR THEMIS_ENABLE_HIP))
            message(STATUS "Fused kernels tests skipped (CUDA/HIP not enabled)")
        else()
            message(STATUS "Fused kernels tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_fused_kernels.cpp")
        endif()
    endif()
    
    # Adaptive Batching Tests (Issue #39 - Dynamic Batch Size Adaptation)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_adaptive_batching.cpp")
        message(STATUS "Adding adaptive batching tests")
        
        add_executable(test_adaptive_batching
            test_adaptive_batching.cpp
        )
        
        target_link_libraries(test_adaptive_batching PRIVATE
            ${TEST_LIBS}
            themis_core
            spdlog::spdlog
        )
        
        target_compile_definitions(test_adaptive_batching PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        # Add to CTest
        add_test(NAME AdaptiveBatchingTests COMMAND test_adaptive_batching)
        
        # Set test properties
        set_tests_properties(AdaptiveBatchingTests PROPERTIES
            LABELS "lora;gpu;adaptive;batching"
            TIMEOUT 300
        )
    else()
        message(STATUS "Adaptive batching tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_adaptive_batching.cpp")
    endif()
    
    # GPU LoRA Layers Tests (Phase 10)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_lora_layers.cpp" AND (THEMIS_ENABLE_CUDA OR THEMIS_ENABLE_HIP))
        message(STATUS "Adding GPU LoRA layers tests (CUDA/HIP enabled)")
        
        add_executable(test_gpu_lora_layers
            test_gpu_lora_layers.cpp
        )
        
        target_link_libraries(test_gpu_lora_layers PRIVATE
            ${TEST_LIBS}
            themis_core
            spdlog::spdlog
        )
        
        target_compile_definitions(test_gpu_lora_layers PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        # Add to CTest
        add_test(NAME GPULoRALayersTests COMMAND test_gpu_lora_layers)
        
        # Set test properties
        set_tests_properties(GPULoRALayersTests PROPERTIES
            LABELS "lora;gpu;cuda;hip"
            TIMEOUT 300
        )
    else()
        if(NOT (THEMIS_ENABLE_CUDA OR THEMIS_ENABLE_HIP))
            message(STATUS "GPU LoRA layers tests skipped (CUDA/HIP not enabled)")
        else()
            message(STATUS "GPU LoRA layers tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_lora_layers.cpp")
        endif()
    endif()
    
    # Common timeout for LoRA integration tests
    set(LORA_TEST_TIMEOUT 600)
    
    # LoRA Storage Integration Tests (Issue #5)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_lora_storage_integration.cpp")
        message(STATUS "Adding LoRA storage integration tests")
        
        add_executable(test_lora_storage_integration
            test_lora_storage_integration.cpp
        )
        
        target_link_libraries(test_lora_storage_integration PRIVATE
            ${TEST_LIBS}
            themis_core
            OpenSSL::SSL
            OpenSSL::Crypto
            spdlog::spdlog
            nlohmann_json::nlohmann_json
            RocksDB::rocksdb
        )
        
        target_compile_definitions(test_lora_storage_integration PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        add_test(NAME LoRAStorageIntegrationTests COMMAND test_lora_storage_integration)
        set_tests_properties(LoRAStorageIntegrationTests PROPERTIES
            LABELS "lora;storage;integration"
            TIMEOUT ${LORA_TEST_TIMEOUT}
        )
    else()
        message(STATUS "LoRA storage integration tests not found")
    endif()
    
    # Cross-Shard LoRA Distribution Tests (Issue #5)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_cross_shard_distribution.cpp")
        message(STATUS "Adding cross-shard LoRA distribution tests")
        
        add_executable(test_cross_shard_distribution
            test_cross_shard_distribution.cpp
            fixtures/mock_shard_cluster.cpp
        )
        
        target_link_libraries(test_cross_shard_distribution PRIVATE
            ${TEST_LIBS}
            themis_core
            OpenSSL::SSL
            OpenSSL::Crypto
            spdlog::spdlog
            nlohmann_json::nlohmann_json
            RocksDB::rocksdb
        )
        
        target_compile_definitions(test_cross_shard_distribution PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        add_test(NAME CrossShardDistributionTests COMMAND test_cross_shard_distribution)
        set_tests_properties(CrossShardDistributionTests PROPERTIES
            LABELS "lora;shard;distribution;network"
            TIMEOUT ${LORA_TEST_TIMEOUT}
        )
    else()
        message(STATUS "Cross-shard distribution tests not found")
    endif()
    
    # RAID LoRA Integration Tests (Issue #5)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_raid_lora_integration.cpp")
        message(STATUS "Adding RAID LoRA integration tests")
        
        add_executable(test_raid_lora_integration
            test_raid_lora_integration.cpp
            fixtures/mock_shard_cluster.cpp
            utils/raid_simulator.cpp
            utils/shard_failure_injector.cpp
        )
        
        target_link_libraries(test_raid_lora_integration PRIVATE
            ${TEST_LIBS}
            themis_core
            OpenSSL::SSL
            OpenSSL::Crypto
            spdlog::spdlog
            nlohmann_json::nlohmann_json
            RocksDB::rocksdb
        )
        
        target_compile_definitions(test_raid_lora_integration PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        add_test(NAME RAIDLoRAIntegrationTests COMMAND test_raid_lora_integration)
        set_tests_properties(RAIDLoRAIntegrationTests PROPERTIES
            LABELS "lora;raid;redundancy;failover"
            TIMEOUT ${LORA_TEST_TIMEOUT}
        )
    else()
        message(STATUS "RAID LoRA integration tests not found")
    endif()
    
    # LoRA Encryption Integration Tests (Issue #5)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_lora_encryption_integration.cpp")
        message(STATUS "Adding LoRA encryption integration tests")
        
        add_executable(test_lora_encryption_integration
            test_lora_encryption_integration.cpp
        )
        
        target_link_libraries(test_lora_encryption_integration PRIVATE
            ${TEST_LIBS}
            themis_core
            OpenSSL::SSL
            OpenSSL::Crypto
            spdlog::spdlog
            nlohmann_json::nlohmann_json
            RocksDB::rocksdb
        )
        
        target_compile_definitions(test_lora_encryption_integration PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        add_test(NAME LoRAEncryptionIntegrationTests COMMAND test_lora_encryption_integration)
        set_tests_properties(LoRAEncryptionIntegrationTests PROPERTIES
            LABELS "lora;encryption;security;hsm;vault;pki"
            TIMEOUT ${LORA_TEST_TIMEOUT}
        )
    else()
        message(STATUS "LoRA encryption integration tests not found")
    endif()
    
    # LoRA Failure Scenarios Tests (Issue #5)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_lora_failure_scenarios.cpp")
        message(STATUS "Adding LoRA failure scenarios tests")
        
        add_executable(test_lora_failure_scenarios
            test_lora_failure_scenarios.cpp
            fixtures/mock_shard_cluster.cpp
            utils/shard_failure_injector.cpp
        )
        
        target_link_libraries(test_lora_failure_scenarios PRIVATE
            ${TEST_LIBS}
            themis_core
            OpenSSL::SSL
            OpenSSL::Crypto
            spdlog::spdlog
            nlohmann_json::nlohmann_json
            RocksDB::rocksdb
        )
        
        target_compile_definitions(test_lora_failure_scenarios PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        add_test(NAME LoRAFailureScenariosTests COMMAND test_lora_failure_scenarios)
        set_tests_properties(LoRAFailureScenariosTests PROPERTIES
            LABELS "lora;failure;chaos;resilience"
            TIMEOUT ${LORA_TEST_TIMEOUT}
        )
    else()
        message(STATUS "LoRA failure scenarios tests not found")
    endif()
    
    # LoRA Versioning Tests (Issue #5)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_lora_versioning.cpp")
        message(STATUS "Adding LoRA versioning tests")
        
        add_executable(test_lora_versioning
            test_lora_versioning.cpp
        )
        
        target_link_libraries(test_lora_versioning PRIVATE
            ${TEST_LIBS}
            themis_core
            OpenSSL::SSL
            OpenSSL::Crypto
            spdlog::spdlog
            nlohmann_json::nlohmann_json
            RocksDB::rocksdb
        )
        
        target_compile_definitions(test_lora_versioning PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        add_test(NAME LoRAVersioningTests COMMAND test_lora_versioning)
        set_tests_properties(LoRAVersioningTests PROPERTIES
            LABELS "lora;versioning;rollback"
            TIMEOUT ${LORA_TEST_TIMEOUT}
        )
    else()
        message(STATUS "LoRA versioning tests not found")
    endif()
    
    # Fused LoRA Kernels Tests (Issue #36)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_fused_lora_kernels.cpp" AND (THEMIS_ENABLE_CUDA OR THEMIS_ENABLE_HIP))
        message(STATUS "Adding fused LoRA kernels tests (CUDA/HIP enabled)")
        
        add_executable(test_fused_lora_kernels
            test_fused_lora_kernels.cpp
        )
        
        target_link_libraries(test_fused_lora_kernels PRIVATE
            ${TEST_LIBS}
            themis_core
            spdlog::spdlog
        )
        
        target_compile_definitions(test_fused_lora_kernels PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        # Add to CTest
        add_test(NAME FusedLoRAKernelsTests COMMAND test_fused_lora_kernels)
        
        # Set test properties
        set_tests_properties(FusedLoRAKernelsTests PROPERTIES
            LABELS "lora;gpu;cuda;hip;fused;performance"
            TIMEOUT 600
        )
    else()
        if(NOT (THEMIS_ENABLE_CUDA OR THEMIS_ENABLE_HIP))
            message(STATUS "Fused LoRA kernels tests skipped (CUDA/HIP not enabled)")
        else()
            message(STATUS "Fused LoRA kernels tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_fused_lora_kernels.cpp")
        endif()
    endif()
    
    # Mixed Precision GPU Tests (Phase 10.4)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_mixed_precision_gpu.cpp")
        message(STATUS "Adding mixed precision GPU tests")
        
        add_executable(test_mixed_precision_gpu
            test_mixed_precision_gpu.cpp
        )
        
        target_link_libraries(test_mixed_precision_gpu PRIVATE
            ${TEST_LIBS}
            themis_core
            spdlog::spdlog
        )
        
        target_compile_definitions(test_mixed_precision_gpu PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        # Add to CTest
        add_test(NAME MixedPrecisionGPUTests COMMAND test_mixed_precision_gpu)
        
        # Set test properties
        set_tests_properties(MixedPrecisionGPUTests PROPERTIES
            LABELS "lora;gpu;mixed_precision"
            TIMEOUT 300
        )
    endif()
    
    # Multi-GPU Training Tests (Phase 10.5)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_multi_gpu_training.cpp")
        message(STATUS "Adding multi-GPU training tests")
        
        add_executable(test_multi_gpu_training
            test_multi_gpu_training.cpp
        )
        
        target_link_libraries(test_multi_gpu_training PRIVATE
            ${TEST_LIBS}
            themis_core
            spdlog::spdlog
        )
        
        target_compile_definitions(test_multi_gpu_training PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        # Add to CTest
        add_test(NAME MultiGPUTrainingTests COMMAND test_multi_gpu_training)
        
        # Set test properties
        set_tests_properties(MultiGPUTrainingTests PROPERTIES
            LABELS "lora;gpu;multi_gpu;distributed"
            TIMEOUT 600
        )
    endif()
    
    # GPU Training Loop Tests (Phase 10.6)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_training_loop.cpp")
        message(STATUS "Adding GPU training loop tests")
        
        add_executable(test_gpu_training_loop
            test_gpu_training_loop.cpp
        )
        
        target_link_libraries(test_gpu_training_loop PRIVATE
            ${TEST_LIBS}
            themis_core
            spdlog::spdlog
        )
        
        target_compile_definitions(test_gpu_training_loop PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        # Add to CTest
        add_test(NAME GPUTrainingLoopTests COMMAND test_gpu_training_loop)
        
        # Set test properties
        set_tests_properties(GPUTrainingLoopTests PROPERTIES
            LABELS "lora;gpu;training;integration"
            TIMEOUT 600
        )
    endif()
    
    # GPU Memory Management Tests (PR 1 - Complete Test Coverage)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_memory_management.cpp")
        message(STATUS "Adding GPU memory management tests")
        
        add_executable(test_gpu_memory_management
            test_gpu_memory_management.cpp
        )
        
        target_link_libraries(test_gpu_memory_management PRIVATE
            ${TEST_LIBS}
            themis_core
            spdlog::spdlog
        )
        
        target_compile_definitions(test_gpu_memory_management PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        # Add to CTest
        add_test(NAME GPUMemoryManagementTests COMMAND test_gpu_memory_management)
        
        # Set test properties
        set_tests_properties(GPUMemoryManagementTests PROPERTIES
            LABELS "lora;gpu;memory;vram;oom"
            TIMEOUT 300
        )
    endif()
    
    # Training Convergence Tests (PR 1 - Complete Test Coverage)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_training_convergence.cpp")
        message(STATUS "Adding training convergence tests")
        
        add_executable(test_training_convergence
            test_training_convergence.cpp
        )
        
        target_link_libraries(test_training_convergence PRIVATE
            ${TEST_LIBS}
            themis_core
            spdlog::spdlog
        )
        
        target_compile_definitions(test_training_convergence PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        # Add to CTest
        add_test(NAME TrainingConvergenceTests COMMAND test_training_convergence)
        
        # Set test properties
        set_tests_properties(TrainingConvergenceTests PROPERTIES
            LABELS "lora;gpu;training;convergence"
            TIMEOUT 600
        )
    endif()
else()
    message(STATUS "LoRA framework tests disabled (THEMIS_ENABLE_LORA_TESTS=OFF)")
endif()

# ============================================================================
# LoRA Framework Comprehensive Tests (Issue: Unit Tests für LoRA Framework)
# ============================================================================

if(THEMIS_ENABLE_LORA_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_lora_framework_comprehensive.cpp")
        message(STATUS "Adding LoRA framework comprehensive tests (THEMIS_ENABLE_LORA_TESTS=ON)")
        
        add_executable(test_lora_framework_comprehensive
            test_lora_framework_comprehensive.cpp
        )
        
        target_link_libraries(test_lora_framework_comprehensive PRIVATE
            ${TEST_LIBS}
            themis_core
            OpenSSL::SSL
            OpenSSL::Crypto
            spdlog::spdlog
            nlohmann_json::nlohmann_json
            RocksDB::rocksdb
        )
        
        target_compile_definitions(test_lora_framework_comprehensive PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        # Add to CTest
        add_test(NAME LoRAFrameworkComprehensiveTests COMMAND test_lora_framework_comprehensive)
        
        # Set test properties
        set_tests_properties(LoRAFrameworkComprehensiveTests PROPERTIES
            LABELS "lora;llm;framework;comprehensive"
            TIMEOUT 600
        )
        
        message(STATUS "  - Comprehensive LoRA tests: 50+ test cases covering:")
        message(STATUS "    * Storage operations (save, load, delete, versioning)")
        message(STATUS "    * Adapter lifecycle (load, unload, hot-swap, caching)")
        message(STATUS "    * Training service (on-the-fly, batch, callbacks)")
        message(STATUS "    * Multi-adapter management (quantization, multi-GPU)")
        message(STATUS "    * Thread-safety (concurrent operations)")
        message(STATUS "    * Error handling and edge cases")
        message(STATUS "    * Memory management and leak detection")
        message(STATUS "    * Performance benchmarks")
    else()
        message(STATUS "LoRA framework comprehensive tests not found")
    endif()
else()
    message(STATUS "LoRA comprehensive tests disabled (THEMIS_ENABLE_LORA_TESTS=OFF)")
endif()

# ============================================================================
# Adapter Sync and Cross-Shard Consistency Tests (NEW)
# ============================================================================

if(THEMIS_ENABLE_LORA_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_adapter_sync.cpp")
        message(STATUS "Adding adapter sync and consistency tests (THEMIS_ENABLE_LORA_TESTS=ON)")
        
        add_executable(test_adapter_sync
            test_adapter_sync.cpp
            ${CMAKE_SOURCE_DIR}/src/llm/lora_framework/adapter_consistency_checker.cpp
            ${CMAKE_SOURCE_DIR}/src/llm/lora_framework/adapter_sync_manager.cpp
        )
        
        target_link_libraries(test_adapter_sync PRIVATE
            ${TEST_LIBS}
            themis_core
            OpenSSL::SSL
            OpenSSL::Crypto
            spdlog::spdlog
            nlohmann_json::nlohmann_json
            RocksDB::rocksdb
        )
        
        target_compile_definitions(test_adapter_sync PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        # Add to CTest
        add_test(NAME AdapterSyncTests COMMAND test_adapter_sync)
        
        # Set test properties
        set_tests_properties(AdapterSyncTests PROPERTIES
            LABELS "lora;llm;sync;consistency;sharding;raid"
            TIMEOUT 300
        )
        
        message(STATUS "  - Adapter sync tests: automatic cross-shard synchronization")
        message(STATUS "    * Consistency checking (checksum, signature)")
        message(STATUS "    * Version comparison and conflict resolution")
        message(STATUS "    * Multi-shard replication")
        message(STATUS "    * Failure scenarios and recovery")
        message(STATUS "    * RAID integration tests")
    else()
        message(STATUS "Adapter sync tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_adapter_sync.cpp")
    endif()
else()
    message(STATUS "Adapter sync tests disabled (THEMIS_ENABLE_LORA_TESTS=OFF)")
endif()

# ============================================================================
# QLoRA Quantization Tests
# ============================================================================

if(THEMIS_ENABLE_LORA_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_quantization.cpp")
        message(STATUS "Adding QLoRA quantization tests (THEMIS_ENABLE_LORA_TESTS=ON)")
        
        add_executable(test_quantization
            test_quantization.cpp
            ${CMAKE_SOURCE_DIR}/src/llm/lora_framework/quantization.cpp
        )
        
        target_link_libraries(test_quantization PRIVATE
            ${TEST_LIBS}
            spdlog::spdlog
        )
        
        target_compile_definitions(test_quantization PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        # Add to CTest
        add_test(NAME QuantizationTests COMMAND test_quantization)
        
        # Set test properties
        set_tests_properties(QuantizationTests PROPERTIES
            LABELS "lora;llm;quantization;qlora"
            TIMEOUT 300
        )
        
        message(STATUS "  - Quantization tests: NF4, INT8, double quantization")
        message(STATUS "    * Accuracy tests (reconstruction error)")
        message(STATUS "    * Memory reduction tests (60-80% savings)")
        message(STATUS "    * Block-wise quantization")
        message(STATUS "    * Performance benchmarks")
    else()
        message(STATUS "Quantization tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_quantization.cpp")
    endif()
else()
    message(STATUS "Quantization tests disabled (THEMIS_ENABLE_LORA_TESTS=OFF)")
endif()

# ============================================================================
# QLoRA Training Tests
# ============================================================================

if(THEMIS_ENABLE_LORA_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_qlora.cpp")
        message(STATUS "Adding QLoRA training tests (THEMIS_ENABLE_LORA_TESTS=ON)")
        
        add_executable(test_qlora
            test_qlora.cpp
            ${CMAKE_SOURCE_DIR}/src/llm/lora_framework/quantization.cpp
            ${CMAKE_SOURCE_DIR}/src/llm/lora_framework/quantized_model.cpp
            ${CMAKE_SOURCE_DIR}/src/llm/lora_framework/lora_layers.cpp
        )
        
        target_link_libraries(test_qlora PRIVATE
            ${TEST_LIBS}
            spdlog::spdlog
        )
        
        target_compile_definitions(test_qlora PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        # Add to CTest
        add_test(NAME QLoRATests COMMAND test_qlora)
        
        # Set test properties
        set_tests_properties(QLoRATests PROPERTIES
            LABELS "lora;llm;qlora;training"
            TIMEOUT 300
        )
        
        message(STATUS "  - QLoRA tests: Quantized LoRA training")
        message(STATUS "    * Quantized model management")
        message(STATUS "    * QLoRA layer forward/backward")
        message(STATUS "    * Memory reduction validation")
        message(STATUS "    * Training convergence")
    else()
        message(STATUS "QLoRA tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_qlora.cpp")
    endif()
else()
    message(STATUS "QLoRA tests disabled (THEMIS_ENABLE_LORA_TESTS=OFF)")
endif()

# ============================================================================
# QLoRA Training Integration Tests
# ============================================================================

if(THEMIS_ENABLE_LORA_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_qlora_training_integration.cpp")
        message(STATUS "Adding QLoRA training integration tests (THEMIS_ENABLE_LORA_TESTS=ON)")
        
        add_executable(test_qlora_training_integration
            test_qlora_training_integration.cpp
            ${CMAKE_SOURCE_DIR}/src/llm/lora_framework/quantization.cpp
            ${CMAKE_SOURCE_DIR}/src/llm/lora_framework/quantized_model.cpp
            ${CMAKE_SOURCE_DIR}/src/llm/lora_framework/lora_layers.cpp
            ${CMAKE_SOURCE_DIR}/src/llm/lora_framework/lora_training_service.cpp
            ${CMAKE_SOURCE_DIR}/src/llm/lora_framework/lora_storage_service.cpp
            ${CMAKE_SOURCE_DIR}/src/llm/lora_framework/data_loader.cpp
            ${CMAKE_SOURCE_DIR}/src/llm/lora_framework/base_model_adapter.cpp
            ${CMAKE_SOURCE_DIR}/src/llm/lora_framework/mixed_precision.cpp
            ${CMAKE_SOURCE_DIR}/src/llm/lora_framework/lr_scheduler.cpp
            ${CMAKE_SOURCE_DIR}/src/llm/lora_framework/gradient_utils.cpp
        )
        
        target_link_libraries(test_qlora_training_integration PRIVATE
            ${TEST_LIBS}
            spdlog::spdlog
            nlohmann_json::nlohmann_json
            OpenSSL::SSL
            OpenSSL::Crypto
        )
        
        target_compile_definitions(test_qlora_training_integration PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        # Add to CTest
        add_test(NAME QLoRATrainingIntegrationTests COMMAND test_qlora_training_integration)
        
        # Set test properties
        set_tests_properties(QLoRATrainingIntegrationTests PROPERTIES
            LABELS "lora;llm;qlora;training;integration"
            TIMEOUT 300
        )
        
        message(STATUS "  - QLoRA training integration tests: End-to-end training")
        message(STATUS "    * Training service QLoRA integration")
        message(STATUS "    * NF4 and INT8 quantization modes")
        message(STATUS "    * Memory reduction validation")
        message(STATUS "    * Configuration system")
    else()
        message(STATUS "QLoRA training integration tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_qlora_training_integration.cpp")
    endif()
else()
    message(STATUS "QLoRA training integration tests disabled (THEMIS_ENABLE_LORA_TESTS=OFF)")
endif()

# ============================================================================
# Paged Optimizer Tests
# ============================================================================

if(THEMIS_ENABLE_LORA_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_paged_optimizer.cpp")
        message(STATUS "Adding paged optimizer tests (THEMIS_ENABLE_LORA_TESTS=ON)")
        
        add_executable(test_paged_optimizer
            test_paged_optimizer.cpp
            ${CMAKE_SOURCE_DIR}/src/llm/lora_framework/paged_memory_manager.cpp
            ${CMAKE_SOURCE_DIR}/src/llm/lora_framework/paged_optimizer.cpp
            ${CMAKE_SOURCE_DIR}/src/llm/lora_framework/lora_layers.cpp
        )
        
        target_link_libraries(test_paged_optimizer PRIVATE
            ${TEST_LIBS}
            spdlog::spdlog
        )
        
        target_compile_definitions(test_paged_optimizer PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        # Add to CTest
        add_test(NAME PagedOptimizerTests COMMAND test_paged_optimizer)
        
        # Set test properties
        set_tests_properties(PagedOptimizerTests PROPERTIES
            LABELS "lora;llm;optimizer;paged"
            TIMEOUT 300
        )
        
        message(STATUS "  - Paged optimizer tests: Memory paging for optimizer states")
        message(STATUS "    * Paged memory manager")
        message(STATUS "    * PagedAdamWOptimizer")
        message(STATUS "    * Memory savings validation")
        message(STATUS "    * Correctness vs non-paged")
    else()
        message(STATUS "Paged optimizer tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_paged_optimizer.cpp")
    endif()
else()
    message(STATUS "Paged optimizer tests disabled (THEMIS_ENABLE_LORA_TESTS=OFF)")
endif()

# ============================================================================
# Library Integration Tests (Phase 1) - NEW
# ============================================================================

message(STATUS "Configuring library integration tests...")

# Test: spdlog Logging Library Integration
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_lib_spdlog_integration.cpp")
    message(STATUS "Adding spdlog library integration tests")
    
    add_executable(test_lib_spdlog_integration
        test_lib_spdlog_integration.cpp
    )
    
    target_link_libraries(test_lib_spdlog_integration PRIVATE
        ${TEST_LIBS}
        spdlog::spdlog
    )
    
    target_compile_definitions(test_lib_spdlog_integration PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME SpdlogLibIntegrationTests COMMAND test_lib_spdlog_integration)
    
    set_tests_properties(SpdlogLibIntegrationTests PROPERTIES
        LABELS "library;integration;spdlog;logging"
        TIMEOUT 120
    )
else()
    message(STATUS "spdlog library integration tests not found")
endif()

# Test: zstd Compression Library Integration
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_lib_zstd_integration.cpp")
    message(STATUS "Adding zstd library integration tests")
    
    add_executable(test_lib_zstd_integration
        test_lib_zstd_integration.cpp
    )
    
    target_link_libraries(test_lib_zstd_integration PRIVATE
        ${TEST_LIBS}
        zstd::libzstd_static
    )
    
    target_compile_definitions(test_lib_zstd_integration PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME ZstdLibIntegrationTests COMMAND test_lib_zstd_integration)
    
    set_tests_properties(ZstdLibIntegrationTests PROPERTIES
        LABELS "library;integration;zstd;compression"
        TIMEOUT 120
    )
else()
    message(STATUS "zstd library integration tests not found")
endif()

# Test: YAML-cpp Advanced Library Integration
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_lib_yaml_advanced.cpp")
    message(STATUS "Adding YAML-cpp advanced library integration tests")
    
    add_executable(test_lib_yaml_advanced
        test_lib_yaml_advanced.cpp
    )
    
    target_link_libraries(test_lib_yaml_advanced PRIVATE
        ${TEST_LIBS}
        yaml-cpp::yaml-cpp
    )
    
    target_compile_definitions(test_lib_yaml_advanced PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME YAMLLibAdvancedTests COMMAND test_lib_yaml_advanced)
    
    set_tests_properties(YAMLLibAdvancedTests PROPERTIES
        LABELS "library;integration;yaml;configuration"
        TIMEOUT 120
    )
else()
    message(STATUS "YAML-cpp advanced library integration tests not found")
endif()

# Test: Arrow/Parquet Library Integration (Optional)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_lib_arrow_integration.cpp")
    message(STATUS "Adding Arrow/Parquet library integration tests")
    
    # Check if Arrow is available
    find_package(Arrow QUIET CONFIG)
    find_package(Parquet QUIET CONFIG)
    
    if(Arrow_FOUND AND Parquet_FOUND)
        message(STATUS "  Arrow/Parquet libraries found - enabling tests")
        
        add_executable(test_lib_arrow_integration
            test_lib_arrow_integration.cpp
        )
        
        target_link_libraries(test_lib_arrow_integration PRIVATE
            ${TEST_LIBS}
            Arrow::arrow_shared
            Parquet::parquet_shared
        )
        
        target_compile_definitions(test_lib_arrow_integration PRIVATE
            THEMIS_TEST_BUILD=1
            THEMIS_ENABLE_ARROW=1
        )
        
        add_test(NAME ArrowLibIntegrationTests COMMAND test_lib_arrow_integration)
        
        set_tests_properties(ArrowLibIntegrationTests PROPERTIES
            LABELS "library;integration;arrow;parquet;columnar"
            TIMEOUT 180
        )
    else()
        message(STATUS "  Arrow/Parquet libraries not found - creating stub test")
        
        add_executable(test_lib_arrow_integration
            test_lib_arrow_integration.cpp
        )
        
        target_link_libraries(test_lib_arrow_integration PRIVATE
            ${TEST_LIBS}
        )
        
        target_compile_definitions(test_lib_arrow_integration PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        add_test(NAME ArrowLibIntegrationTests COMMAND test_lib_arrow_integration)
        
        set_tests_properties(ArrowLibIntegrationTests PROPERTIES
            LABELS "library;integration;arrow;parquet;columnar;skipped"
            TIMEOUT 30
        )
    endif()
else()
    message(STATUS "Arrow/Parquet library integration tests not found")
endif()

message(STATUS "Library integration tests configured")

# ============================================================================
# PITR (Point-in-Time Recovery) Tests - Phase 3
# ============================================================================

# Test: Snapshot Manager
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_snapshot_manager.cpp")
    message(STATUS "Adding Snapshot Manager tests")
    
    add_executable(test_snapshot_manager
        test_snapshot_manager.cpp
    )
    
    target_link_libraries(test_snapshot_manager PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
    )
    
    target_compile_definitions(test_snapshot_manager PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME SnapshotManagerTests COMMAND test_snapshot_manager)
    
    set_tests_properties(SnapshotManagerTests PROPERTIES
        LABELS "pitr;snapshot;mvcc"
        TIMEOUT 60
    )
else()
    message(STATUS "Snapshot Manager tests not found")
endif()

# Test: PITR Manager
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_pitr_manager.cpp")
    message(STATUS "Adding PITR Manager tests")
    
    add_executable(test_pitr_manager
        test_pitr_manager.cpp
    )
    
    target_link_libraries(test_pitr_manager PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
    )
    
    target_compile_definitions(test_pitr_manager PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME PITRManagerTests COMMAND test_pitr_manager)
    
    set_tests_properties(PITRManagerTests PROPERTIES
        LABELS "pitr;recovery;mvcc"
        TIMEOUT 120
    )
else()
    message(STATUS "PITR Manager tests not found")
endif()

# ============================================================================
# Schema Manager Tests (Issue makr-code/ThemisDB#1)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_schema_manager.cpp")
    message(STATUS "Adding Schema Manager tests")
    
    add_executable(test_schema_manager
        test_schema_manager.cpp
    )
    
    target_link_libraries(test_schema_manager PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
    )
    
    target_compile_definitions(test_schema_manager PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    # Add to CTest
    add_test(NAME SchemaManagerTests COMMAND test_schema_manager)
    
    # Set test properties
    set_tests_properties(SchemaManagerTests PROPERTIES
        LABELS "schema;metadata;self-awareness"
        TIMEOUT 300
    )
else()
    message(STATUS "Schema Manager tests not found")
endif()

# ============================================================================
# Snapshot Manager Integration Tests (Phase 1)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_snapshot_integration.cpp")
    message(STATUS "Adding SnapshotManager integration tests")
    
    add_executable(test_snapshot_integration
        test_snapshot_integration.cpp
    )
    
    target_link_libraries(test_snapshot_integration PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        fmt::fmt
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
    )
    
    target_compile_definitions(test_snapshot_integration PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    # Add to CTest
    add_test(NAME SnapshotIntegrationTests COMMAND test_snapshot_integration)
    
    # Set test properties
    set_tests_properties(SnapshotIntegrationTests PROPERTIES
        LABELS "snapshot;transaction;mvcc;integration"
        TIMEOUT 600
    )
else()
    message(STATUS "SnapshotManager integration tests not found")
endif()

# ============================================================================
# Diff Engine Tests (Phase 2: Git-like features)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_diff_engine.cpp")
    message(STATUS "Adding Diff Engine tests")
    
    add_executable(test_diff_engine
        test_diff_engine.cpp
    )
    
    target_link_libraries(test_diff_engine PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        fmt::fmt
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
    )
    
    target_compile_definitions(test_diff_engine PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    # Add to CTest
    add_test(NAME DiffEngineTests COMMAND test_diff_engine)
    
    # Set test properties
    set_tests_properties(DiffEngineTests PROPERTIES
        LABELS "diff;analytics;mvcc"
        TIMEOUT 300
    )
else()
    message(STATUS "Diff Engine tests not found")
endif()

# ============================================================================
# Existing Tests
# ============================================================================

# Add existing test files here as they are discovered
# Example pattern:
# if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp")
#     file(GLOB TEST_SOURCES "test_*.cpp")
#     foreach(TEST_SOURCE ${TEST_SOURCES})
#         get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
#         add_executable(${TEST_NAME} ${TEST_SOURCE})
#         target_link_libraries(${TEST_NAME} PRIVATE ${TEST_LIBS})
#         add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
#     endforeach()
# endif()

# ============================================================================
# RAG Enhancement Tests (Knowledge Gap Detector & LLM-as-Judge)
# ============================================================================

if(THEMIS_ENABLE_LLM)
    # Knowledge Gap Detector Tests
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_knowledge_gap_detector.cpp")
        message(STATUS "Adding Knowledge Gap Detector tests")
        
        add_executable(test_knowledge_gap_detector
            test_knowledge_gap_detector.cpp
        )
        
        target_link_libraries(test_knowledge_gap_detector PRIVATE
            ${TEST_LIBS}
            themis_core
            spdlog::spdlog
            nlohmann_json::nlohmann_json
        )
        
        target_compile_definitions(test_knowledge_gap_detector PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        add_test(NAME KnowledgeGapDetectorTests COMMAND test_knowledge_gap_detector)
        
        set_tests_properties(KnowledgeGapDetectorTests PROPERTIES
            LABELS "rag;llm;knowledge-gap;unit"
            TIMEOUT 60
        )
    else()
        message(STATUS "Knowledge Gap Detector tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_knowledge_gap_detector.cpp")
    endif()
    
    # RAG Judge Tests
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_rag_judge.cpp")
        message(STATUS "Adding RAG Judge tests")
        
        add_executable(test_rag_judge
            test_rag_judge.cpp
        )
        
        target_link_libraries(test_rag_judge PRIVATE
            ${TEST_LIBS}
            themis_core
            spdlog::spdlog
            nlohmann_json::nlohmann_json
        )
        
        target_compile_definitions(test_rag_judge PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        add_test(NAME RAGJudgeTests COMMAND test_rag_judge)
        
        set_tests_properties(RAGJudgeTests PROPERTIES
            LABELS "rag;llm;judge;unit"
            TIMEOUT 60
        )
    else()
        message(STATUS "RAG Judge tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_rag_judge.cpp")
    endif()
    
    # LLM Integration Tests
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_llm_integration.cpp")
        message(STATUS "Adding LLM Integration tests")
        
        add_executable(test_llm_integration
            test_llm_integration.cpp
        )
        
        target_link_libraries(test_llm_integration PRIVATE
            ${TEST_LIBS}
            themis_core
            spdlog::spdlog
            nlohmann_json::nlohmann_json
        )
        
        target_compile_definitions(test_llm_integration PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        add_test(NAME LLMIntegrationTests COMMAND test_llm_integration)
        
        set_tests_properties(LLMIntegrationTests PROPERTIES
            LABELS "rag;llm;integration;unit"
            TIMEOUT 60
        )
    else()
        message(STATUS "LLM Integration tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_llm_integration.cpp")
    endif()
    
    # Model Loading from ThemisDB Tests (NEW - Issue: GAP Model Loading)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/llm/test_model_loading_from_themisdb.cpp")
        message(STATUS "Adding Model Loading from ThemisDB tests")
        
        add_executable(test_model_loading_from_themisdb
            llm/test_model_loading_from_themisdb.cpp
        )
        
        target_link_libraries(test_model_loading_from_themisdb PRIVATE
            ${TEST_LIBS}
            themis_core
            OpenSSL::SSL
            OpenSSL::Crypto
            spdlog::spdlog
            nlohmann_json::nlohmann_json
            RocksDB::rocksdb
        )
        
        target_compile_definitions(test_model_loading_from_themisdb PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        add_test(NAME ModelLoadingFromThemisDBTests COMMAND test_model_loading_from_themisdb)
        
        set_tests_properties(ModelLoadingFromThemisDBTests PROPERTIES
            LABELS "llm;model_loading;blob_storage;themisdb;unit"
            TIMEOUT 120
        )
    else()
        message(STATUS "Model Loading from ThemisDB tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/llm/test_model_loading_from_themisdb.cpp")
    endif()
    
    # Model Loading Performance Benchmarks (NEW - Issue: GAP Model Loading)
    find_package(benchmark QUIET CONFIG)
    if(benchmark_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/llm/bench_model_loading_from_themisdb.cpp")
        message(STATUS "Adding Model Loading performance benchmarks (Google Benchmark)")
        
        add_executable(bench_model_loading_from_themisdb
            llm/bench_model_loading_from_themisdb.cpp
        )
        
        target_link_libraries(bench_model_loading_from_themisdb PRIVATE
            benchmark::benchmark
            benchmark::benchmark_main
            themis_core
            OpenSSL::SSL
            OpenSSL::Crypto
            spdlog::spdlog
            nlohmann_json::nlohmann_json
            RocksDB::rocksdb
        )
        
        target_compile_definitions(bench_model_loading_from_themisdb PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        # Benchmarks are not added to CTest by default (run manually)
        # To run: ./bench_model_loading_from_themisdb
    else()
        if(NOT benchmark_FOUND)
            message(STATUS "Google Benchmark not found - skipping performance benchmarks")
            message(STATUS "  Install with: vcpkg install benchmark")
        else()
            message(STATUS "Model Loading benchmarks not found at ${CMAKE_CURRENT_SOURCE_DIR}/llm/bench_model_loading_from_themisdb.cpp")
        endif()
    endif()
    
    # Claim Extractor Tests
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_claim_extractor.cpp")
        message(STATUS "Adding Claim Extractor tests")
        
        add_executable(test_claim_extractor
            test_claim_extractor.cpp
        )
        
        target_link_libraries(test_claim_extractor PRIVATE
            ${TEST_LIBS}
            themis_core
            spdlog::spdlog
            nlohmann_json::nlohmann_json
        )
        
        target_compile_definitions(test_claim_extractor PRIVATE
            THEMIS_TEST_BUILD=1
        )
        
        add_test(NAME ClaimExtractorTests COMMAND test_claim_extractor)
        
        set_tests_properties(ClaimExtractorTests PROPERTIES
            LABELS "rag;llm;claim;consistency;unit"
            TIMEOUT 60
        )
    else()
        message(STATUS "Claim Extractor tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_claim_extractor.cpp")
    endif()
endif()

# ============================================================================
# Integration Tests Subdirectory
# ============================================================================

# Add integration tests if directory exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/integration/CMakeLists.txt")
    message(STATUS "Adding integration tests from: ${CMAKE_CURRENT_SOURCE_DIR}/integration")
    add_subdirectory(integration)
else()
    message(STATUS "Integration tests directory not found or no CMakeLists.txt")
endif()

# ============================================================================
# Security Signature Verifier Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_signature_verifier.cpp")
    message(STATUS "Adding signature verifier tests")
    
    add_executable(test_signature_verifier
        test_signature_verifier.cpp
    )
    
    target_link_libraries(test_signature_verifier PRIVATE
        ${TEST_LIBS}
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
    )
    
    # Link to signature_verifier object files if available
    if(TARGET signature_verifier_obj)
        target_link_libraries(test_signature_verifier PRIVATE signature_verifier_obj)
    else()
        # Compile the source directly for now
        target_sources(test_signature_verifier PRIVATE
            ${CMAKE_SOURCE_DIR}/src/llm/security/signature_verifier.cpp
        )
    endif()
    
    target_compile_definitions(test_signature_verifier PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    # Add to CTest
    add_test(NAME SignatureVerifierTests COMMAND test_signature_verifier)
    
    # Set test properties
    set_tests_properties(SignatureVerifierTests PROPERTIES
        LABELS "security;signature;crypto"
        TIMEOUT 120
    )
else()
    message(STATUS "Signature verifier tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_signature_verifier.cpp")
endif()

# ============================================================================
# Enhanced Plugin Security Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_enhanced_plugin_security.cpp")
    message(STATUS "Adding enhanced plugin security tests")
    
    add_executable(test_enhanced_plugin_security
        test_enhanced_plugin_security.cpp
        ${CMAKE_SOURCE_DIR}/src/acceleration/plugin_security.cpp
    )
    
    target_link_libraries(test_enhanced_plugin_security PRIVATE
        ${TEST_LIBS}
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
    )
    
    target_compile_definitions(test_enhanced_plugin_security PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    # Add to CTest
    add_test(NAME EnhancedPluginSecurityTests COMMAND test_enhanced_plugin_security)
    
    # Set test properties
    set_tests_properties(EnhancedPluginSecurityTests PROPERTIES
        LABELS "security;plugin;signature;crypto"
        TIMEOUT 120
    )
else()
    message(STATUS "Enhanced plugin security tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_enhanced_plugin_security.cpp")
endif()

# ============================================================================
# Generic Plugin Registry Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_generic_plugin_registry.cpp")
    message(STATUS "Adding generic plugin registry tests")
    
    add_executable(test_generic_plugin_registry
        test_generic_plugin_registry.cpp
    )
    
    target_link_libraries(test_generic_plugin_registry PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
    )
    
    target_compile_definitions(test_generic_plugin_registry PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    # Add to CTest
    add_test(NAME GenericPluginRegistryTests COMMAND test_generic_plugin_registry)
    
    # Set test properties
    set_tests_properties(GenericPluginRegistryTests PROPERTIES
        LABELS "plugins;registry;core"
        TIMEOUT 60
    )
else()
    message(STATUS "Generic plugin registry tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_generic_plugin_registry.cpp")
endif()

# ============================================================================
# Plugin Metrics Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_metrics.cpp")
    message(STATUS "Adding plugin metrics tests")
    
    add_executable(test_plugin_metrics
        test_plugin_metrics.cpp
        ${CMAKE_SOURCE_DIR}/src/plugins/plugin_metrics.cpp
    )
    
    target_link_libraries(test_plugin_metrics PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
    )
    
    target_compile_definitions(test_plugin_metrics PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    # Add to CTest
    add_test(NAME PluginMetricsTests COMMAND test_plugin_metrics)
    
    # Set test properties
    set_tests_properties(PluginMetricsTests PROPERTIES
        LABELS "plugins;metrics;monitoring"
        TIMEOUT 60
    )
else()
    message(STATUS "Plugin metrics tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_metrics.cpp")
endif()

# ============================================================================
# Plugin Metrics Integration Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_metrics_integration.cpp")
    message(STATUS "Adding plugin metrics integration tests")
    
    add_executable(test_plugin_metrics_integration
        test_plugin_metrics_integration.cpp
        ${CMAKE_SOURCE_DIR}/src/plugins/plugin_metrics.cpp
        ${CMAKE_SOURCE_DIR}/src/plugins/plugin_manager.cpp
        ${CMAKE_SOURCE_DIR}/src/plugins/plugin_registry.cpp
        ${CMAKE_SOURCE_DIR}/src/acceleration/plugin_loader.cpp
        ${CMAKE_SOURCE_DIR}/src/acceleration/plugin_security.cpp
    )
    
    target_link_libraries(test_plugin_metrics_integration PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
    )
    
    target_compile_definitions(test_plugin_metrics_integration PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    # Add to CTest
    add_test(NAME PluginMetricsIntegrationTests COMMAND test_plugin_metrics_integration)
    
    # Set test properties
    set_tests_properties(PluginMetricsIntegrationTests PROPERTIES
        LABELS "plugins;metrics;integration"
        TIMEOUT 120
    )
else()
    message(STATUS "Plugin metrics integration tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_metrics_integration.cpp")
endif()

# ============================================================================
# Plugin Hot-Plug Monitoring Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_hot_plug.cpp")
    message(STATUS "Adding plugin hot-plug monitoring tests")
    
    add_executable(test_plugin_hot_plug
        test_plugin_hot_plug.cpp
        ${CMAKE_SOURCE_DIR}/src/plugins/plugin_hot_plug_monitor.cpp
        ${CMAKE_SOURCE_DIR}/src/plugins/plugin_manager.cpp
        ${CMAKE_SOURCE_DIR}/src/plugins/plugin_registry.cpp
        ${CMAKE_SOURCE_DIR}/src/plugins/plugin_metrics.cpp
        ${CMAKE_SOURCE_DIR}/src/acceleration/plugin_loader.cpp
        ${CMAKE_SOURCE_DIR}/src/acceleration/plugin_security.cpp
    )
    
    target_link_libraries(test_plugin_hot_plug PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
    )
    
    target_compile_definitions(test_plugin_hot_plug PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    # Add to CTest
    add_test(NAME PluginHotPlugTests COMMAND test_plugin_hot_plug)
    
    # Set test properties
    set_tests_properties(PluginHotPlugTests PROPERTIES
        LABELS "plugins;monitoring;hot-plug"
        TIMEOUT 120
    )
else()
    message(STATUS "Plugin hot-plug monitoring tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_hot_plug.cpp")
endif()

# ============================================================================
# Enhanced Hot-Reload Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_hot_reload_enhanced.cpp")
    message(STATUS "Adding enhanced hot-reload tests")
    
    add_executable(test_plugin_hot_reload_enhanced
        test_plugin_hot_reload_enhanced.cpp
        ${CMAKE_SOURCE_DIR}/src/plugins/plugin_manager.cpp
        ${CMAKE_SOURCE_DIR}/src/plugins/plugin_registry.cpp
        ${CMAKE_SOURCE_DIR}/src/plugins/plugin_metrics.cpp
        ${CMAKE_SOURCE_DIR}/src/plugins/plugin_hot_plug_monitor.cpp
        ${CMAKE_SOURCE_DIR}/src/acceleration/plugin_loader.cpp
        ${CMAKE_SOURCE_DIR}/src/acceleration/plugin_security.cpp
    )
    
    target_link_libraries(test_plugin_hot_reload_enhanced PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
    )
    
    target_compile_definitions(test_plugin_hot_reload_enhanced PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    # Add to CTest
    add_test(NAME EnhancedHotReloadTests COMMAND test_plugin_hot_reload_enhanced)
    
    # Set test properties
    set_tests_properties(EnhancedHotReloadTests PROPERTIES
        LABELS "plugins;hot-reload;state-preservation"
        TIMEOUT 60
    )
else()
    message(STATUS "Enhanced hot-reload tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_hot_reload_enhanced.cpp")
endif()

# ============================================================================
# Plugin Dependency Resolver Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_dependency_resolver.cpp")
    message(STATUS "Adding plugin dependency resolver tests")
    
    add_executable(test_plugin_dependency_resolver
        test_plugin_dependency_resolver.cpp
    )
    
    target_link_libraries(test_plugin_dependency_resolver PRIVATE
        ${TEST_LIBS}
        themis_core
        nlohmann_json::nlohmann_json
    )
    
    target_compile_definitions(test_plugin_dependency_resolver PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    # Add to CTest
    add_test(NAME PluginDependencyResolverTests COMMAND test_plugin_dependency_resolver)
    
    # Set test properties
    set_tests_properties(PluginDependencyResolverTests PROPERTIES
        LABELS "plugins;dependency;resolver;graph"
        TIMEOUT 60
    )
else()
    message(STATUS "Plugin dependency resolver tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_dependency_resolver.cpp")
endif()

# ============================================================================
# Base Interfaces Tests (Phase 1: Dependency Inversion)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_base_interfaces.cpp")
    message(STATUS "Adding base interfaces tests")
    
    add_executable(test_base_interfaces
        test_base_interfaces.cpp
    )
    
    target_link_libraries(test_base_interfaces PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
    )
    
    target_compile_definitions(test_base_interfaces PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME BaseInterfacesTests COMMAND test_base_interfaces)
    set_tests_properties(BaseInterfacesTests PROPERTIES
        LABELS "base;interfaces;architecture;unit"
        TIMEOUT 60
    )
else()
    message(STATUS "Base interfaces tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_base_interfaces.cpp")
endif()

# ============================================================================
# Auth Middleware Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_auth_middleware.cpp")
    message(STATUS "Adding auth middleware tests")
    
    add_executable(test_auth_middleware
        test_auth_middleware.cpp
    )
    
    target_link_libraries(test_auth_middleware PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
    )
    
    target_compile_definitions(test_auth_middleware PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME AuthMiddlewareTests COMMAND test_auth_middleware)
    
    set_tests_properties(AuthMiddlewareTests PROPERTIES
        LABELS "auth;middleware;security"
        TIMEOUT 120
    )
else()
    message(STATUS "Auth middleware tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_auth_middleware.cpp")
endif()

# ============================================================================
# Huge Pages Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_huge_pages.cpp")
    message(STATUS "Adding huge pages tests")
    
    add_executable(test_huge_pages
        test_huge_pages.cpp
    )
    
    target_link_libraries(test_huge_pages PRIVATE
        ${TEST_LIBS}
    )
    
    target_compile_definitions(test_huge_pages PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    if(THEMIS_ENABLE_HUGE_PAGES)
        message(STATUS "Huge pages support enabled for test_huge_pages")
        target_compile_definitions(test_huge_pages PRIVATE
            THEMIS_USE_HUGE_PAGES=1  # Used by test code to conditionally compile huge pages tests
        )
    else()
        message(STATUS "Huge pages support disabled - tests will skip huge pages allocations")
    endif()
    
    add_test(NAME HugePagesTests COMMAND test_huge_pages)
    
    set_tests_properties(HugePagesTests PROPERTIES
        LABELS "performance;memory;huge_pages"
        TIMEOUT 120
    )
else()
    message(STATUS "Huge pages tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_huge_pages.cpp")
endif()

# ============================================================================
# Storage Engine DI Tests (Phase 2)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_storage_engine_di.cpp")
    message(STATUS "Adding Storage Engine DI tests")
    
    add_executable(test_storage_engine_di
        test_storage_engine_di.cpp
    )
    
    target_link_libraries(test_storage_engine_di PRIVATE
        ${TEST_LIBS}
        GTest::gmock
        themis_core
        spdlog::spdlog
    )
    
    target_compile_definitions(test_storage_engine_di PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME StorageEngineDITests COMMAND test_storage_engine_di)
    
    set_tests_properties(StorageEngineDITests PROPERTIES
        LABELS "storage;di;architecture;unit"
        TIMEOUT 60
    )
else()
    message(STATUS "Storage Engine DI tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_storage_engine_di.cpp")
endif()

# ============================================================================
# IndexManager DI Tests (Phase 4)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_index_manager_di.cpp")
    message(STATUS "Adding IndexManager DI tests")
    
    add_executable(test_index_manager_di
        test_index_manager_di.cpp
    )
    
    target_link_libraries(test_index_manager_di PRIVATE
        ${TEST_LIBS}
        GTest::gmock
        themis_core
        spdlog::spdlog
    )
    
    target_compile_definitions(test_index_manager_di PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME IndexManagerDITests COMMAND test_index_manager_di)
    
    set_tests_properties(IndexManagerDITests PROPERTIES
        LABELS "index;di;architecture;unit"
        TIMEOUT 60
    )
else()
    message(STATUS "IndexManager DI tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_index_manager_di.cpp")
endif()

# ============================================================================
# General Graph Traversal Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_general_traversal.cpp")
    message(STATUS "Adding General Graph Traversal tests")
    
    add_executable(test_general_traversal
        test_general_traversal.cpp
    )
    
    target_link_libraries(test_general_traversal PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
    )
    
    target_compile_definitions(test_general_traversal PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME GeneralTraversalTests COMMAND test_general_traversal)
    
    set_tests_properties(GeneralTraversalTests PROPERTIES
        LABELS "graph;traversal;unit"
        TIMEOUT 120
    )
else()
    message(STATUS "General traversal tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_general_traversal.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_aql_general_traversal.cpp")
    message(STATUS "Adding AQL General Traversal integration tests")
    
    add_executable(test_aql_general_traversal
        test_aql_general_traversal.cpp
    )
    
    target_link_libraries(test_aql_general_traversal PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
    )
    
    target_compile_definitions(test_aql_general_traversal PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME AQLGeneralTraversalTests COMMAND test_aql_general_traversal)
    
    set_tests_properties(AQLGeneralTraversalTests PROPERTIES
        LABELS "aql;graph;traversal;integration"
        TIMEOUT 120
    )
else()
    message(STATUS "AQL General traversal tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_aql_general_traversal.cpp")
endif()

# ============================================================================
# JSON Path Function Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_json_path_functions.cpp")
    message(STATUS "Adding JSON Path function tests")
    
    add_executable(test_json_path_functions
        test_json_path_functions.cpp
    )
    
    target_link_libraries(test_json_path_functions PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
    )
    
    target_compile_definitions(test_json_path_functions PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME JsonPathFunctionTests COMMAND test_json_path_functions)
    
    set_tests_properties(JsonPathFunctionTests PROPERTIES
        LABELS "aql;json;document;query;unit"
        TIMEOUT 60
    )
else()
    message(STATUS "JSON Path function tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_json_path_functions.cpp")
endif()

# ============================================================================
# PageRank Function Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/query/test_pagerank.cpp")
    message(STATUS "Adding PageRank function tests")
    
    add_executable(test_pagerank
        query/test_pagerank.cpp
    )
    
    target_link_libraries(test_pagerank PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
    )
    
    target_compile_definitions(test_pagerank PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME PageRankFunctionTests COMMAND test_pagerank)
    
    set_tests_properties(PageRankFunctionTests PROPERTIES
        LABELS "query;graph;pagerank;unit"
        TIMEOUT 60
    )
else()
    message(STATUS "PageRank function tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/query/test_pagerank.cpp")
endif()

# ============================================================================
# RAG Ethics Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_rag_ethics.cpp")
    message(STATUS "Adding RAG Ethics integration tests")
    
    add_executable(test_rag_ethics
        test_rag_ethics.cpp
    )
    
    target_link_libraries(test_rag_ethics PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
    )

    target_compile_definitions(test_rag_ethics PRIVATE
        THEMIS_TEST_BUILD=1
    )

    add_test(NAME RAGEthicsTests COMMAND test_rag_ethics)
    set_tests_properties(RAGEthicsTests PROPERTIES
        LABELS "rag;llm;ethics;integration"
        TIMEOUT 120
    )
else()
    message(STATUS "RAG Ethics integration tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_rag_ethics.cpp")
endif()

# RAG Enhancement Tests
# ============================================================================

# Knowledge Gap Detector Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_knowledge_gap_detector.cpp")
    message(STATUS "Adding Knowledge Gap Detector tests")
    
    add_executable(test_knowledge_gap_detector
        test_knowledge_gap_detector.cpp
    )
    
    target_link_libraries(test_knowledge_gap_detector PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
    )
    
    target_compile_definitions(test_knowledge_gap_detector PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME KnowledgeGapDetectorTests COMMAND test_knowledge_gap_detector)
    
    set_tests_properties(KnowledgeGapDetectorTests PROPERTIES
        LABELS "rag;llm;knowledge_gap;unit"
        TIMEOUT 120
    )
else()
    message(STATUS "Knowledge Gap Detector tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_knowledge_gap_detector.cpp")
endif()

# RAG Judge Phase 1 Tests
# ============================================================================

if(THEMIS_ENABLE_LLM AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_rag_judge_phase1.cpp")
    message(STATUS "Adding RAG Judge Phase 1 tests")
    
    add_executable(test_rag_judge_phase1
        test_rag_judge_phase1.cpp
    )
    
    target_link_libraries(test_rag_judge_phase1 PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
    )
    
    target_compile_definitions(test_rag_judge_phase1 PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME RAGJudgePhase1Tests COMMAND test_rag_judge_phase1)
    
    set_tests_properties(RAGJudgePhase1Tests PROPERTIES
        LABELS "rag;llm;judge;phase1;unit"
        TIMEOUT 120
    )
else()
    if(NOT THEMIS_ENABLE_LLM)
        message(STATUS "RAG Judge tests skipped (THEMIS_ENABLE_LLM=OFF)")
    else()
        message(STATUS "RAG Judge Phase 1 tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_rag_judge_phase1.cpp")
    endif()
endif()

# RAG Judge Phase 2 Tests
if(THEMIS_ENABLE_LLM AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_rag_judge_phase2.cpp")
    message(STATUS "Adding RAG Judge Phase 2 tests")
    
    add_executable(test_rag_judge_phase2
        test_rag_judge_phase2.cpp
    )
    
    target_link_libraries(test_rag_judge_phase2 PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
    )
    
    target_compile_definitions(test_rag_judge_phase2 PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME RAGJudgePhase2Tests COMMAND test_rag_judge_phase2)
    
    set_tests_properties(RAGJudgePhase2Tests PROPERTIES
        LABELS "rag;llm;judge;phase2;unit;evaluators"
        TIMEOUT 180
    )
else()
    if(NOT THEMIS_ENABLE_LLM)
        message(STATUS "RAG Judge Phase 2 tests skipped (THEMIS_ENABLE_LLM=OFF)")
    else()
        message(STATUS "RAG Judge Phase 2 tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_rag_judge_phase2.cpp")
    endif()
endif()

# RAG Judge Phase 3 Tests
if(THEMIS_ENABLE_LLM AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_rag_judge_phase3.cpp")
    message(STATUS "Adding RAG Judge Phase 3 tests")
    
    add_executable(test_rag_judge_phase3
        test_rag_judge_phase3.cpp
    )
    
    target_link_libraries(test_rag_judge_phase3 PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
    )
    
    target_compile_definitions(test_rag_judge_phase3 PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME RAGJudgePhase3Tests COMMAND test_rag_judge_phase3)
    
    set_tests_properties(RAGJudgePhase3Tests PROPERTIES
        LABELS "rag;llm;judge;phase3;unit;pairwise;ensemble"
        TIMEOUT 240
    )
else()
    if(NOT THEMIS_ENABLE_LLM)
        message(STATUS "RAG Judge Phase 3 tests skipped (THEMIS_ENABLE_LLM=OFF)")
    else()
        message(STATUS "RAG Judge Phase 3 tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_rag_judge_phase3.cpp")
    endif()
endif()

# RAG Judge Phase 4 Tests
if(THEMIS_ENABLE_LLM AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_rag_judge_phase4.cpp")
    message(STATUS "Adding RAG Judge Phase 4 tests")
    
    add_executable(test_rag_judge_phase4
        test_rag_judge_phase4.cpp
    )
    
    target_link_libraries(test_rag_judge_phase4 PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        nlohmann_json::nlohmann_json
    )
    
    target_compile_definitions(test_rag_judge_phase4 PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME RAGJudgePhase4Tests COMMAND test_rag_judge_phase4)
    
    set_tests_properties(RAGJudgePhase4Tests PROPERTIES
        LABELS "rag;llm;judge;phase4;unit;rubric;cot"
        TIMEOUT 240
    )
else()
    if(NOT THEMIS_ENABLE_LLM)
        message(STATUS "RAG Judge Phase 4 tests skipped (THEMIS_ENABLE_LLM=OFF)")
    else()
        message(STATUS "RAG Judge Phase 4 tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_rag_judge_phase4.cpp")
    endif()
endif()

# ============================================================================
# Test Discovery & Reporting
# ============================================================================

# Enable CTest XML output
set(CTEST_OUTPUT_ON_FAILURE ON)

# Add custom target to run all tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
    DEPENDS test_lora_framework
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all ThemisDB tests..."
)

# ============================================================================
# Multi-GPU LoRA Advanced Tests (v1.5.0)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_multi_gpu_lora_advanced.cpp")
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

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_distributed_coordinator.cpp")
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
# Custom Test Targets
# ============================================================================

# Add custom target to run only integration tests
add_custom_target(run_integration_tests
    COMMAND ${CMAKE_CTEST_COMMAND} -R integration --output-on-failure --verbose
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running ThemisDB integration tests..."
)

message(STATUS "Tests configured successfully")
message(STATUS "  - Build tests with: cmake --build . --target test_lora_framework")
message(STATUS "  - Run tests with: ctest --output-on-failure")
message(STATUS "  - Run integration tests with: ctest -R integration")
message(STATUS "  - Or use: make run_tests / make run_integration_tests")
