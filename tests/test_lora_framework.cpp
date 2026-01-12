/**
 * @file test_lora_framework.cpp
 * @brief Comprehensive unit tests for LoRA Adapter Framework
 * 
 * Tests all components of the LoRA framework including:
 * - Adapter Manager (lifecycle, caching, hot-swapping)
 * - Storage Service (ThemisDB integration, versioning, encryption)
 * - Training Service (on-the-fly and batch training)
 * - Orchestrator (CRUD operations, monitoring)
 * - Audit Logger (inference tracing, compliance)
 * - themis_help_lora (documentation assistant)
 * 
 * @note Requires GTest: vcpkg install gtest OR apt-get install libgtest-dev
 * @build cmake -DTHEMIS_BUILD_TESTS=ON ..
 * @run ./tests/test_lora_framework
 */

#ifndef THEMIS_TEST_BUILD
#define THEMIS_TEST_BUILD 1
#endif

#include <gtest/gtest.h>
#include "llm/lora_framework/lora_adapter_manager.h"
#include "llm/lora_framework/lora_storage_service.h"
#include "llm/lora_framework/lora_training_service.h"
#include "llm/lora_framework/lora_orchestrator.h"
#include "llm/lora_framework/lora_audit_logger.h"
#include "llm/applications/themis_help_lora.h"
#include <memory>
#include <thread>
#include <chrono>

using namespace themis::llm::lora;
using namespace themis::llm::applications;

// ============================================================================
// Test Fixtures
// ============================================================================

class LoRAFrameworkTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize storage service
        LoRAStorageService::Config storage_config;
        storage_config.storage_path = "/tmp/test_lora_storage";
        storage_config.enable_encryption = false;  // Disable for testing
        storage_config.enable_signatures = false;
        storage_ = std::make_shared<LoRAStorageService>(storage_config);
        
        // Initialize adapter manager
        LoRAAdapterManager::Config manager_config;
        manager_config.cache_size = 5;
        manager_config.cache_ttl_seconds = 300;
        manager_ = std::make_shared<LoRAAdapterManager>(manager_config, storage_);
        
        // Initialize training service
        LoRATrainingService::Config training_config;
        training_config.default_rank = 8;
        training_config.default_alpha = 16.0;
        training_ = std::make_shared<LoRATrainingService>(training_config);
        
        // Initialize audit logger
        LoRAAuditLogger::Config audit_config;
        audit_config.log_path = "/tmp/test_lora_audit.jsonl";
        audit_config.enable_hash_chain = false;  // Disable for testing
        audit_ = std::make_shared<LoRAAuditLogger>(audit_config);
        
        // Initialize orchestrator
        orchestrator_ = std::make_unique<LoRAOrchestrator>(
            storage_, manager_, training_, audit_
        );
    }
    
    void TearDown() override {
        orchestrator_.reset();
        audit_.reset();
        training_.reset();
        manager_.reset();
        storage_.reset();
        
        // Cleanup test files
        system("rm -rf /tmp/test_lora_storage");
        system("rm -f /tmp/test_lora_audit.jsonl");
    }
    
    std::shared_ptr<LoRAStorageService> storage_;
    std::shared_ptr<LoRAAdapterManager> manager_;
    std::shared_ptr<LoRATrainingService> training_;
    std::shared_ptr<LoRAAuditLogger> audit_;
    std::unique_ptr<LoRAOrchestrator> orchestrator_;
};

// ============================================================================
// LoRAStorageService Tests
// ============================================================================

TEST_F(LoRAFrameworkTest, StorageService_SaveAndLoadAdapter) {
    // Create test adapter
    AdapterWeights weights;
    weights.rank = 8;
    weights.alpha = 16.0;
    weights.weights_data.resize(1024, 0.5f);
    
    AdapterMetadata metadata;
    metadata.adapter_id = "test_adapter";
    metadata.version = "v1.0";
    metadata.base_model = "llama-2-7b";
    metadata.description = "Test adapter for unit tests";
    
    // Save adapter
    bool saved = storage_->saveAdapter("test_adapter", weights, metadata);
    EXPECT_TRUE(saved);
    
    // Load adapter
    auto loaded_info = storage_->getAdapterInfo("test_adapter");
    ASSERT_TRUE(loaded_info.has_value());
    EXPECT_EQ(loaded_info->metadata.adapter_id, "test_adapter");
    EXPECT_EQ(loaded_info->metadata.version, "v1.0");
    EXPECT_EQ(loaded_info->metadata.base_model, "llama-2-7b");
}

TEST_F(LoRAFrameworkTest, StorageService_VersionManagement) {
    // Create adapter v1
    AdapterWeights weights_v1;
    weights_v1.rank = 8;
    weights_v1.weights_data.resize(1024, 0.5f);
    
    AdapterMetadata metadata_v1;
    metadata_v1.adapter_id = "test_adapter";
    metadata_v1.version = "v1.0";
    metadata_v1.base_model = "llama-2-7b";
    
    storage_->saveAdapter("test_adapter", weights_v1, metadata_v1);
    
    // Create adapter v2
    AdapterWeights weights_v2;
    weights_v2.rank = 16;
    weights_v2.weights_data.resize(2048, 0.7f);
    
    AdapterMetadata metadata_v2;
    metadata_v2.adapter_id = "test_adapter";
    metadata_v2.version = "v2.0";
    metadata_v2.base_model = "llama-2-7b";
    
    storage_->saveAdapter("test_adapter", weights_v2, metadata_v2);
    
    // List versions
    auto versions = storage_->listVersions("test_adapter");
    EXPECT_GE(versions.size(), 2);
    
    // Rollback to v1
    bool rolled_back = storage_->rollbackToVersion("test_adapter", "v1.0");
    EXPECT_TRUE(rolled_back);
    
    auto current = storage_->getAdapterInfo("test_adapter");
    ASSERT_TRUE(current.has_value());
    EXPECT_EQ(current->metadata.version, "v1.0");
}

TEST_F(LoRAFrameworkTest, StorageService_DeleteAdapter) {
    // Create and save adapter
    AdapterWeights weights;
    weights.rank = 8;
    weights.weights_data.resize(1024, 0.5f);
    
    AdapterMetadata metadata;
    metadata.adapter_id = "test_adapter";
    metadata.version = "v1.0";
    metadata.base_model = "llama-2-7b";
    
    storage_->saveAdapter("test_adapter", weights, metadata);
    
    // Verify exists
    auto info = storage_->getAdapterInfo("test_adapter");
    ASSERT_TRUE(info.has_value());
    
    // Delete adapter
    bool deleted = storage_->deleteAdapter("test_adapter");
    EXPECT_TRUE(deleted);
    
    // Verify deleted
    info = storage_->getAdapterInfo("test_adapter");
    EXPECT_FALSE(info.has_value());
}

// ============================================================================
// LoRAAdapterManager Tests
// ============================================================================

TEST_F(LoRAFrameworkTest, AdapterManager_LoadAndUnload) {
    // Create test adapter in storage
    AdapterWeights weights;
    weights.rank = 8;
    weights.weights_data.resize(1024, 0.5f);
    
    AdapterMetadata metadata;
    metadata.adapter_id = "test_adapter";
    metadata.version = "v1.0";
    metadata.base_model = "llama-2-7b";
    
    storage_->saveAdapter("test_adapter", weights, metadata);
    
    // Load adapter
    bool loaded = manager_->loadAdapter("test_adapter");
    EXPECT_TRUE(loaded);
    EXPECT_TRUE(manager_->isLoaded("test_adapter"));
    
    // Get adapter info
    auto info = manager_->getAdapterInfo("test_adapter");
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->adapter_id, "test_adapter");
    
    // Unload adapter
    bool unloaded = manager_->unloadAdapter("test_adapter");
    EXPECT_TRUE(unloaded);
    EXPECT_FALSE(manager_->isLoaded("test_adapter"));
}

TEST_F(LoRAFrameworkTest, AdapterManager_HotSwapping) {
    // Create two adapters
    AdapterWeights weights1;
    weights1.rank = 8;
    weights1.weights_data.resize(1024, 0.5f);
    
    AdapterMetadata metadata1;
    metadata1.adapter_id = "adapter1";
    metadata1.version = "v1.0";
    metadata1.base_model = "llama-2-7b";
    
    AdapterWeights weights2;
    weights2.rank = 16;
    weights2.weights_data.resize(2048, 0.7f);
    
    AdapterMetadata metadata2;
    metadata2.adapter_id = "adapter2";
    metadata2.version = "v1.0";
    metadata2.base_model = "llama-2-7b";
    
    storage_->saveAdapter("adapter1", weights1, metadata1);
    storage_->saveAdapter("adapter2", weights2, metadata2);
    
    // Load adapter1
    manager_->loadAdapter("adapter1");
    EXPECT_TRUE(manager_->isLoaded("adapter1"));
    EXPECT_FALSE(manager_->isLoaded("adapter2"));
    
    // Hot-swap to adapter2
    bool swapped = manager_->switchAdapter("adapter1", "adapter2");
    EXPECT_TRUE(swapped);
    EXPECT_FALSE(manager_->isLoaded("adapter1"));
    EXPECT_TRUE(manager_->isLoaded("adapter2"));
}

TEST_F(LoRAFrameworkTest, AdapterManager_CacheEviction) {
    // Create 6 adapters (cache size is 5)
    for (int i = 1; i <= 6; i++) {
        AdapterWeights weights;
        weights.rank = 8;
        weights.weights_data.resize(1024, 0.5f);
        
        AdapterMetadata metadata;
        metadata.adapter_id = "adapter" + std::to_string(i);
        metadata.version = "v1.0";
        metadata.base_model = "llama-2-7b";
        
        storage_->saveAdapter(metadata.adapter_id, weights, metadata);
        manager_->loadAdapter(metadata.adapter_id);
    }
    
    // First adapter should be evicted (LRU)
    EXPECT_FALSE(manager_->isLoaded("adapter1"));
    EXPECT_TRUE(manager_->isLoaded("adapter6"));
    
    // Get cache stats
    auto stats = manager_->getCacheStats();
    EXPECT_EQ(stats.size, 5);
    EXPECT_GT(stats.evictions, 0);
}

// ============================================================================
// LoRATrainingService Tests
// ============================================================================

TEST_F(LoRAFrameworkTest, TrainingService_OnTheFlyTraining) {
    // Create training data
    TrainingData data;
    data.adapter_id = "test_adapter";
    data.base_model = "llama-2-7b";
    data.training_samples = {
        {"What is ThemisDB?", "ThemisDB is a multi-model database"},
        {"How do I enable sharding?", "Use SHARDING_ENABLE=true in config"}
    };
    
    // Set training config
    LoRAConfig config;
    config.rank = 8;
    config.alpha = 16.0;
    config.learning_rate = 0.0001;
    config.num_epochs = 1;
    
    training_->setTrainingConfig(config);
    
    // Train adapter (placeholder - actual training requires LLM)
    auto result = training_->trainOnTheFly(data);
    EXPECT_TRUE(result.success);
}

TEST_F(LoRAFrameworkTest, TrainingService_BatchTraining) {
    // Create batch training data
    std::vector<TrainingData> dataset;
    for (int i = 0; i < 3; i++) {
        TrainingData data;
        data.adapter_id = "batch_adapter";
        data.base_model = "llama-2-7b";
        data.training_samples = {
            {"Question " + std::to_string(i), "Answer " + std::to_string(i)}
        };
        dataset.push_back(data);
    }
    
    // Train batch
    auto result = training_->trainBatch(dataset);
    EXPECT_TRUE(result.success);
}

// ============================================================================
// LoRAOrchestrator Tests
// ============================================================================

TEST_F(LoRAFrameworkTest, Orchestrator_CreateAdapter) {
    // Create adapter via orchestrator
    TrainingData data;
    data.adapter_id = "orchestrated_adapter";
    data.base_model = "llama-2-7b";
    data.training_samples = {
        {"Test question", "Test answer"}
    };
    
    bool created = orchestrator_->createAdapter("orchestrated_adapter", data);
    EXPECT_TRUE(created);
    
    // Verify adapter exists
    auto info = orchestrator_->getAdapter("orchestrated_adapter");
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->adapter_id, "orchestrated_adapter");
}

TEST_F(LoRAFrameworkTest, Orchestrator_UpdateAdapter) {
    // Create adapter
    TrainingData data;
    data.adapter_id = "update_adapter";
    data.base_model = "llama-2-7b";
    data.training_samples = {{"Q1", "A1"}};
    
    orchestrator_->createAdapter("update_adapter", data);
    
    // Update adapter
    TrainingData update_data;
    update_data.adapter_id = "update_adapter";
    update_data.base_model = "llama-2-7b";
    update_data.training_samples = {{"Q2", "A2"}};
    
    bool updated = orchestrator_->updateAdapter("update_adapter", update_data);
    EXPECT_TRUE(updated);
}

TEST_F(LoRAFrameworkTest, Orchestrator_ListAdapters) {
    // Create multiple adapters
    for (int i = 1; i <= 3; i++) {
        TrainingData data;
        data.adapter_id = "list_adapter" + std::to_string(i);
        data.base_model = "llama-2-7b";
        data.training_samples = {{"Q", "A"}};
        
        orchestrator_->createAdapter(data.adapter_id, data);
    }
    
    // List adapters
    auto adapters = orchestrator_->listAdapters();
    EXPECT_GE(adapters.size(), 3);
}

TEST_F(LoRAFrameworkTest, Orchestrator_HealthCheck) {
    // Run health check
    bool healthy = orchestrator_->healthCheck();
    EXPECT_TRUE(healthy);
    
    // Get stats
    auto stats = orchestrator_->getStats();
    EXPECT_TRUE(stats.contains("adapters_loaded"));
    EXPECT_TRUE(stats.contains("cache_size"));
}

// ============================================================================
// LoRAAuditLogger Tests
// ============================================================================

TEST_F(LoRAFrameworkTest, AuditLogger_LogInference) {
    // Create inference audit entry
    LoRAInferenceAudit audit;
    audit.request_id = "test-001";
    audit.base_model_id = "llama-2-7b";
    audit.base_model_version = "2.0";
    audit.adapter_id = "themis_help_lora";
    audit.adapter_version = "v1.0";
    audit.prompt = "Test question";
    audit.response = "Test answer";
    audit.user_id = "test_user";
    audit.success = true;
    
    // Log inference
    audit_->logInference(audit);
    
    // Query audit history
    auto history = audit_->getInferenceHistory("themis_help_lora");
    EXPECT_GE(history.size(), 1);
    
    auto& entry = history[0];
    EXPECT_EQ(entry.request_id, "test-001");
    EXPECT_EQ(entry.adapter_id, "themis_help_lora");
}

TEST_F(LoRAFrameworkTest, AuditLogger_QueryHistory) {
    // Log multiple inferences
    for (int i = 1; i <= 5; i++) {
        LoRAInferenceAudit audit;
        audit.request_id = "test-" + std::to_string(i);
        audit.adapter_id = "test_adapter";
        audit.prompt = "Question " + std::to_string(i);
        audit.response = "Answer " + std::to_string(i);
        audit.user_id = "test_user";
        audit.success = true;
        
        audit_->logInference(audit);
    }
    
    // Query history with filters
    AuditQuery query;
    query.adapter_id = "test_adapter";
    query.limit = 3;
    
    auto results = audit_->queryAuditLog(query);
    EXPECT_EQ(results.size(), 3);
}

// ============================================================================
// themis_help_lora Tests
// ============================================================================

TEST_F(LoRAFrameworkTest, ThemisHelpLoRA_Query) {
    // Initialize themis_help_lora
    ThemisHelpLoRA::Config help_config;
    help_config.adapter_id = "themis_help_lora";
    help_config.base_model = "llama-2-7b";
    
    ThemisHelpLoRA help(help_config);
    
    // Query (no user_id parameter)
    std::string response = help.query("How do I enable sharding?");
    EXPECT_FALSE(response.empty());
}

TEST_F(LoRAFrameworkTest, ThemisHelpLoRA_FeedbackCollection) {
    ThemisHelpLoRA::Config help_config;
    help_config.adapter_id = "themis_help_lora";
    help_config.base_model = "llama-2-7b";
    
    ThemisHelpLoRA help(help_config);
    
    // Add positive feedback (no user_id parameter)
    help.addPositiveFeedback("Good question", "Good answer");
    
    // Add negative feedback with correction (no user_id parameter)
    help.addNegativeFeedback("Wrong question", "Wrong answer", 
                            "Corrected answer");
    
    // Get feedback stats (returns json)
    auto stats = help.getFeedbackStats();
    EXPECT_EQ(stats["positive_feedback"], 1);
    EXPECT_EQ(stats["negative_feedback"], 1);
}

TEST_F(LoRAFrameworkTest, ThemisHelpLoRA_Training) {
    ThemisHelpLoRA::Config help_config;
    help_config.adapter_id = "themis_help_lora";
    help_config.base_model = "llama-2-7b";
    
    ThemisHelpLoRA help(help_config);
    
    // Add feedback
    for (int i = 0; i < 100; i++) {
        help.addPositiveFeedback("Question " + std::to_string(i), 
                                "Answer " + std::to_string(i));
    }
    
    // Train from feedback (returns TrainingResult)
    auto result = help.trainFromFeedback();
    EXPECT_TRUE(result.success);
    
    // Check version incremented (compare semantically, not lexicographically)
    std::string new_version = help.getAdapterVersion();
    EXPECT_NE(new_version, "v1.0");  // Version should have changed
    // Note: Full semantic version comparison would be better but requires additional utility
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(LoRAFrameworkTest, Integration_EndToEndWorkflow) {
    // 1. Create adapter via orchestrator
    TrainingData data;
    data.adapter_id = "e2e_adapter";
    data.base_model = "llama-2-7b";
    data.training_samples = {{"Q1", "A1"}, {"Q2", "A2"}};
    
    bool created = orchestrator_->createAdapter("e2e_adapter", data);
    ASSERT_TRUE(created);
    
    // 2. Load adapter
    bool loaded = orchestrator_->loadAdapter("e2e_adapter");
    ASSERT_TRUE(loaded);
    
    // 3. Verify adapter is loaded
    EXPECT_TRUE(orchestrator_->isAdapterLoaded("e2e_adapter"));
    
    // 4. Get adapter info
    auto info = orchestrator_->getAdapter("e2e_adapter");
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->adapter_id, "e2e_adapter");
    
    // 5. Update adapter
    TrainingData update_data;
    update_data.adapter_id = "e2e_adapter";
    update_data.base_model = "llama-2-7b";
    update_data.training_samples = {{"Q3", "A3"}};
    
    bool updated = orchestrator_->updateAdapter("e2e_adapter", update_data);
    ASSERT_TRUE(updated);
    
    // 6. Unload adapter
    bool unloaded = orchestrator_->unloadAdapter("e2e_adapter");
    ASSERT_TRUE(unloaded);
    
    // 7. Delete adapter
    bool deleted = orchestrator_->deleteAdapter("e2e_adapter");
    EXPECT_TRUE(deleted);
}

// ============================================================================
// Performance Tests (basic)
// ============================================================================

TEST_F(LoRAFrameworkTest, Performance_AdapterLoading) {
    // Create adapter
    AdapterWeights weights;
    weights.rank = 8;
    weights.weights_data.resize(1024 * 1024, 0.5f);  // 1M weights
    
    AdapterMetadata metadata;
    metadata.adapter_id = "perf_adapter";
    metadata.version = "v1.0";
    metadata.base_model = "llama-2-7b";
    
    storage_->saveAdapter("perf_adapter", weights, metadata);
    
    // Measure load time
    auto start = std::chrono::high_resolution_clock::now();
    manager_->loadAdapter("perf_adapter");
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Load should be < 500ms
    EXPECT_LT(duration.count(), 500);
    
    std::cout << "Adapter load time: " << duration.count() << " ms" << std::endl;
}

TEST_F(LoRAFrameworkTest, Performance_HotSwapping) {
    // Create two adapters
    AdapterWeights weights1;
    weights1.rank = 8;
    weights1.weights_data.resize(1024 * 1024, 0.5f);
    
    AdapterWeights weights2;
    weights2.rank = 8;
    weights2.weights_data.resize(1024 * 1024, 0.7f);
    
    AdapterMetadata metadata1;
    metadata1.adapter_id = "swap1";
    metadata1.version = "v1.0";
    metadata1.base_model = "llama-2-7b";
    
    AdapterMetadata metadata2;
    metadata2.adapter_id = "swap2";
    metadata2.version = "v1.0";
    metadata2.base_model = "llama-2-7b";
    
    storage_->saveAdapter("swap1", weights1, metadata1);
    storage_->saveAdapter("swap2", weights2, metadata2);
    
    manager_->loadAdapter("swap1");
    
    // Measure hot-swap time
    auto start = std::chrono::high_resolution_clock::now();
    manager_->switchAdapter("swap1", "swap2");
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Hot-swap should be < 10ms
    EXPECT_LT(duration.count(), 10);
    
    std::cout << "Hot-swap time: " << duration.count() << " ms" << std::endl;
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
