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
#include "llm/multi_lora_manager.h"
#include "llm/lora_framework/lora_storage_service.h"
#include "llm/lora_framework/lora_training_service.h"
#include "llm/lora_framework/lora_orchestrator.h"
#include "llm/lora_framework/lora_audit_logger.h"
#include "llm/applications/themis_help_lora.h"
#include "utils/audit_logger.h"
#include <memory>
#include <thread>
#include <chrono>

using namespace themis::llm;
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
        storage_config.filesystem_path = "/tmp/test_lora_storage";
        storage_config.enable_encryption = false;  // Disable for testing
        storage_config.enable_signatures = false;
        storage_ = std::make_shared<LoRAStorageService>(storage_config);
        
        // Initialize adapter manager
        MultiLoRAManager::Config manager_config;
        manager_config.max_lora_slots = 5;
        manager_config.max_lora_vram_mb = 2048;
        manager_config.lora_ttl = std::chrono::seconds(300);
        manager_ = std::make_shared<MultiLoRAManager>(manager_config);
        
        // Initialize training service
        LoRATrainingService::Config training_config;
        training_config.default_hyperparameters.rank = 8;
        training_config.default_hyperparameters.alpha = 16.0f;
        training_ = std::make_shared<LoRATrainingService>(training_config);
        
        // Initialize audit logger
        themis::utils::AuditLoggerConfig audit_config;
        audit_config.log_path = "/tmp/test_lora_audit.jsonl";
        audit_ = std::make_shared<LoRAAuditLogger>(audit_config);
        
        // Initialize orchestrator with config
        LoRAOrchestrator::Config orch_config;
        orchestrator_ = std::make_unique<LoRAOrchestrator>(orch_config);
    }
    
    void TearDown() override {
        orchestrator_ = nullptr;
        audit_ = nullptr;
        training_ = nullptr;
        manager_ = nullptr;
        storage_ = nullptr;
        
        // Cleanup test files
        system("rm -rf /tmp/test_lora_storage");
        system("rm -f /tmp/test_lora_audit.jsonl");
    }
    
    std::shared_ptr<LoRAStorageService> storage_;
    std::shared_ptr<MultiLoRAManager> manager_;
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
    weights.hyperparameters.rank = 8;
    weights.hyperparameters.alpha = 16.0f;
    weights.data.resize(1024);
    weights.size_bytes = weights.data.size();
    
    AdapterMetadata metadata;
    metadata.adapter_id = "test_adapter";
    metadata.version = "v1.0";
    metadata.base_model = "llama-2-7b";
    metadata.description = "Test adapter for unit tests";
    
    // Save adapter
    bool saved = storage_->saveAdapter("test_adapter", weights, metadata);
    EXPECT_TRUE(saved);
    
    // Load adapter metadata
    auto loaded_metadata = storage_->loadMetadata("test_adapter");
    ASSERT_TRUE(loaded_metadata.has_value());
    EXPECT_EQ(loaded_metadata->adapter_id, "test_adapter");
    EXPECT_EQ(loaded_metadata->version, "v1.0");
    EXPECT_EQ(loaded_metadata->base_model, "llama-2-7b");
}

TEST_F(LoRAFrameworkTest, StorageService_VersionManagement) {
    // Create adapter v1
    AdapterWeights weights_v1;
    weights_v1.hyperparameters.rank = 8;
    weights_v1.data.resize(1024);
    weights_v1.size_bytes = weights_v1.data.size();
    
    AdapterMetadata metadata_v1;
    metadata_v1.adapter_id = "test_adapter";
    metadata_v1.version = "v1.0";
    metadata_v1.base_model = "llama-2-7b";
    
    storage_->saveAdapter("test_adapter", weights_v1, metadata_v1);
    
    // Create adapter v2
    AdapterWeights weights_v2;
    weights_v2.hyperparameters.rank = 16;
    weights_v2.data.resize(2048);
    weights_v2.size_bytes = weights_v2.data.size();
    
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
    
    auto current = storage_->loadMetadata("test_adapter");
    ASSERT_TRUE(current.has_value());
    EXPECT_EQ(current->version, "v1.0");
}

TEST_F(LoRAFrameworkTest, StorageService_DeleteAdapter) {
    // Create and save adapter
    AdapterWeights weights;
    weights.hyperparameters.rank = 8;
    weights.data.resize(1024);
    weights.size_bytes = weights.data.size();
    
    AdapterMetadata metadata;
    metadata.adapter_id = "test_adapter";
    metadata.version = "v1.0";
    metadata.base_model = "llama-2-7b";
    
    storage_->saveAdapter("test_adapter", weights, metadata);
    
    // Verify exists
    bool exists = storage_->exists("test_adapter");
    EXPECT_TRUE(exists);
    
    // Delete adapter
    bool deleted = storage_->deleteAdapter("test_adapter");
    EXPECT_TRUE(deleted);
    
    // Verify deleted
    exists = storage_->exists("test_adapter");
    EXPECT_FALSE(exists);
}

TEST_F(LoRAFrameworkTest, StorageService_VaultConfiguration) {
    // Test Vault configuration fields
    LoRAStorageService::Config vault_config;
    vault_config.filesystem_path = "/tmp/test_lora_vault";
    vault_config.enable_encryption = true;
    vault_config.use_vault_for_encryption = false;  // Use MockKeyProvider for tests
    vault_config.vault_addr = "http://localhost:8200";
    vault_config.vault_token = "test-token";
    vault_config.vault_kv_mount = "themis";
    vault_config.encryption_key_id = "lora_adapters";
    
    // Should initialize with MockKeyProvider since use_vault_for_encryption is false
    auto vault_storage = std::make_shared<LoRAStorageService>(vault_config);
    
    // Test that encryption_key_version field is preserved in metadata
    AdapterWeights weights;
    weights.hyperparameters.rank = 8;
    weights.data.resize(1024);
    weights.size_bytes = weights.data.size();
    
    AdapterMetadata metadata;
    metadata.adapter_id = "vault_test_adapter";
    metadata.version = "v1.0";
    metadata.base_model = "llama-2-7b";
    metadata.encryption_key_version = 1;  // Test key rotation support
    
    bool saved = vault_storage->saveAdapter("vault_test_adapter", weights, metadata);
    EXPECT_TRUE(saved);
    
    auto loaded_metadata = vault_storage->loadMetadata("vault_test_adapter");
    ASSERT_TRUE(loaded_metadata.has_value());
    EXPECT_EQ(loaded_metadata->encryption_key_version, 1);
    
    // Cleanup
    vault_storage->deleteAdapter("vault_test_adapter");
}

// ============================================================================
// Adapter Management Tests (using MultiLoRAManager)
// ============================================================================

TEST_F(LoRAFrameworkTest, AdapterManager_LoadAndUnload) {
    // Create test adapter in storage
    AdapterWeights weights;
    weights.hyperparameters.rank = 8;
    weights.data.resize(1024);
    weights.size_bytes = weights.data.size();
    
    AdapterMetadata metadata;
    metadata.adapter_id = "test_adapter";
    metadata.version = "v1.0";
    metadata.base_model = "llama-2-7b";
    
    storage_->saveAdapter("test_adapter", weights, metadata);
    
    // Load adapter
    bool loaded = manager_->loadLoRA("test_adapter", "/tmp/test_adapter.bin", "llama-2-7b", false, GPUPlacement::SINGLE_GPU, 1.0f);
    EXPECT_TRUE(loaded);
    EXPECT_TRUE(manager_->isLoRALoaded("test_adapter"));
    
    // Get adapter info
    auto info = manager_->getLoRAInfo("test_adapter");
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->adapter_id, "test_adapter");
    
    // Unload adapter
    bool unloaded = manager_->unloadLoRA("test_adapter", false);
    EXPECT_TRUE(unloaded);
    EXPECT_FALSE(manager_->isLoRALoaded("test_adapter"));
}

TEST_F(LoRAFrameworkTest, AdapterManager_HotSwapping) {
    // Create two adapters
    AdapterWeights weights1;
    weights1.hyperparameters.rank = 8;
    weights1.data.resize(1024);
    weights1.size_bytes = weights1.data.size();
    
    AdapterMetadata metadata1;
    metadata1.adapter_id = "adapter1";
    metadata1.version = "v1.0";
    metadata1.base_model = "llama-2-7b";
    
    AdapterWeights weights2;
    weights2.hyperparameters.rank = 16;
    weights2.data.resize(2048);
    weights2.size_bytes = weights2.data.size();
    
    AdapterMetadata metadata2;
    metadata2.adapter_id = "adapter2";
    metadata2.version = "v1.0";
    metadata2.base_model = "llama-2-7b";
    
    storage_->saveAdapter("adapter1", weights1, metadata1);
    storage_->saveAdapter("adapter2", weights2, metadata2);
    
    // Load adapter1
    manager_->loadLoRA("adapter1", "/tmp/adapter1.bin", "llama-2-7b", false, GPUPlacement::SINGLE_GPU, 1.0f);
    EXPECT_TRUE(manager_->isLoRALoaded("adapter1"));
    
    // Hot-swap to adapter2 (unload first, then load)
    manager_->unloadLoRA("adapter1", false);
    EXPECT_FALSE(manager_->isLoRALoaded("adapter1"));
    
    manager_->loadLoRA("adapter2", "/tmp/adapter2.bin", "llama-2-7b", false, GPUPlacement::SINGLE_GPU, 1.0f);
    EXPECT_TRUE(manager_->isLoRALoaded("adapter2"));
}

TEST_F(LoRAFrameworkTest, AdapterManager_CacheEviction) {
    // Create 6 adapters (cache size is 5)
    for (int i = 1; i <= 6; i++) {
        AdapterWeights weights;
        weights.hyperparameters.rank = 8;
        weights.data.resize(1024);
        weights.size_bytes = weights.data.size();
        
        AdapterMetadata metadata;
        metadata.adapter_id = "adapter" + std::to_string(i);
        metadata.version = "v1.0";
        metadata.base_model = "llama-2-7b";
        
        storage_->saveAdapter(metadata.adapter_id, weights, metadata);
        manager_->loadLoRA(metadata.adapter_id, "/tmp/" + metadata.adapter_id + ".bin", "llama-2-7b", false, GPUPlacement::SINGLE_GPU, 1.0f);
    }
    
    // First adapter should be evicted (LRU)
    EXPECT_FALSE(manager_->isLoRALoaded("adapter1"));
    EXPECT_TRUE(manager_->isLoRALoaded("adapter6"));
    
    // Get memory stats (replacing getCacheStats)
    auto stats = manager_->getMemoryStats();
    EXPECT_GT(stats["loras_loaded"], 0);
}

// ============================================================================
// LoRATrainingService Tests
// ============================================================================

TEST_F(LoRAFrameworkTest, TrainingService_OnTheFlyTraining) {
    // Create training data
    TrainingData data;
    data.samples = {
        {"What is ThemisDB?", "ThemisDB is a multi-model database", json{}},
        {"How do I enable sharding?", "Use SHARDING_ENABLE=true in config", json{}}
    };
    
    // LoRA hyperparameters
    LoRAHyperparameters hyper;
    hyper.rank = 8;
    hyper.alpha = 16.0f;
    hyper.learning_rate = 0.0001f;
    hyper.num_epochs = 1;
    
    // Train adapter (placeholder - actual training requires LLM)
    auto result = training_->trainOnTheFly("test_adapter", data, hyper);
    EXPECT_TRUE(result.success);
}

TEST_F(LoRAFrameworkTest, TrainingService_BatchTraining) {
    // Create batch training data
    TrainingData data;
    for (int i = 0; i < 3; i++) {
        TrainingDataSample sample;
        sample.input = "Question " + std::to_string(i);
        sample.output = "Answer " + std::to_string(i);
        data.samples.push_back(sample);
    }
    
    // Train batch
    auto result = training_->trainOnTheFly("batch_adapter", data);
    EXPECT_TRUE(result.success);
}

// ============================================================================
// Training Control Tests (Stop, Checkpoint, Resume)
// ============================================================================

TEST_F(LoRAFrameworkTest, TrainingControl_StopTraining) {
    // Create training data with many samples to ensure training takes some time
    TrainingData data;
    for (int i = 0; i < 100; i++) {
        TrainingDataSample sample;
        sample.input = "Question " + std::to_string(i);
        sample.output = "Answer " + std::to_string(i);
        data.samples.push_back(sample);
    }
    
    // Configure for longer training
    LoRAHyperparameters hyper;
    hyper.rank = 8;
    hyper.alpha = 16.0f;
    hyper.learning_rate = 0.0001f;
    hyper.num_epochs = 10;  // Many epochs
    hyper.batch_size = 2;
    
    // Start training in a background thread
    std::atomic<bool> training_started(false);
    std::thread training_thread([&]() {
        training_started.store(true);
        auto result = training_->trainOnTheFly("stop_test_adapter", data, hyper);
        // Training should be stopped, so success should be false
        EXPECT_FALSE(result.success);
        EXPECT_EQ(result.error_message, "Training stopped by user request");
    });
    
    // Wait for training to start
    while (!training_started.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Give it a moment to actually start training
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Verify training is in progress
    EXPECT_TRUE(training_->isTraining());
    
    // Stop training
    training_->stopTraining();
    
    // Wait for training thread to complete
    training_thread.join();
    
    // Verify training is no longer active
    EXPECT_FALSE(training_->isTraining());
}

TEST_F(LoRAFrameworkTest, TrainingControl_CheckpointSaveLoad) {
    // Create cross-platform temp directory for checkpoints
    auto temp_dir = std::filesystem::temp_directory_path() / "test_checkpoints";
    
    // Configure checkpointing
    LoRATrainingService::Config config;
    config.enable_checkpointing = true;
    config.checkpoint_dir = temp_dir.string();
    config.checkpoint_interval_steps = 5;  // Save every 5 steps
    training_->setTrainingConfig(config);
    
    // Create checkpoint directory
    std::filesystem::create_directories(temp_dir);
    
    // Create training data
    TrainingData data;
    for (int i = 0; i < 20; i++) {
        TrainingDataSample sample;
        sample.input = "Question " + std::to_string(i);
        sample.output = "Answer " + std::to_string(i);
        data.samples.push_back(sample);
    }
    
    // Configure training
    LoRAHyperparameters hyper;
    hyper.rank = 8;
    hyper.alpha = 16.0f;
    hyper.learning_rate = 0.0001f;
    hyper.num_epochs = 2;
    hyper.batch_size = 4;
    
    // Train - this should create checkpoints
    auto result = training_->trainOnTheFly("checkpoint_adapter", data, hyper);
    EXPECT_TRUE(result.success);
    
    // Verify checkpoint files were created
    bool checkpoint_found = false;
    for (const auto& entry : std::filesystem::directory_iterator(temp_dir)) {
        if (entry.path().extension() == ".json") {
            checkpoint_found = true;
            break;
        }
    }
    EXPECT_TRUE(checkpoint_found);
    
    // Cleanup
    std::filesystem::remove_all(temp_dir);
}

TEST_F(LoRAFrameworkTest, TrainingControl_PeriodicCheckpointing) {
    // Create cross-platform temp directory for checkpoints
    auto temp_dir = std::filesystem::temp_directory_path() / "test_periodic_checkpoints";
    
    // Configure aggressive checkpointing
    LoRATrainingService::Config config;
    config.enable_checkpointing = true;
    config.checkpoint_dir = temp_dir.string();
    config.checkpoint_interval_steps = 3;  // Very frequent
    training_->setTrainingConfig(config);
    
    // Create checkpoint directory
    std::filesystem::create_directories(temp_dir);
    
    // Create training data
    TrainingData data;
    for (int i = 0; i < 30; i++) {
        TrainingDataSample sample;
        sample.input = "Q" + std::to_string(i);
        sample.output = "A" + std::to_string(i);
        data.samples.push_back(sample);
    }
    
    // Configure training
    LoRAHyperparameters hyper;
    hyper.rank = 8;
    hyper.alpha = 16.0f;
    hyper.learning_rate = 0.0001f;
    hyper.num_epochs = 2;
    hyper.batch_size = 2;
    
    // Train
    auto result = training_->trainOnTheFly("periodic_adapter", data, hyper);
    EXPECT_TRUE(result.success);
    
    // Count checkpoint files
    int checkpoint_count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(temp_dir)) {
        if (entry.path().extension() == ".json") {
            checkpoint_count++;
        }
    }
    
    // Should have created multiple checkpoints
    EXPECT_GT(checkpoint_count, 1);
    
    // Cleanup
    std::filesystem::remove_all(temp_dir);
}

TEST_F(LoRAFrameworkTest, TrainingControl_StopWithCheckpoint) {
    // Create cross-platform temp directory for checkpoints
    auto temp_dir = std::filesystem::temp_directory_path() / "test_stop_checkpoint";
    
    // Configure checkpointing
    LoRATrainingService::Config config;
    config.enable_checkpointing = true;
    config.checkpoint_dir = temp_dir.string();
    config.checkpoint_interval_steps = 100;  // Won't trigger during test
    training_->setTrainingConfig(config);
    
    // Create checkpoint directory
    std::filesystem::create_directories(temp_dir);
    
    // Create training data
    TrainingData data;
    for (int i = 0; i < 100; i++) {
        TrainingDataSample sample;
        sample.input = "Question " + std::to_string(i);
        sample.output = "Answer " + std::to_string(i);
        data.samples.push_back(sample);
    }
    
    // Configure for longer training
    LoRAHyperparameters hyper;
    hyper.rank = 8;
    hyper.alpha = 16.0f;
    hyper.learning_rate = 0.0001f;
    hyper.num_epochs = 10;
    hyper.batch_size = 2;
    
    // Start training in background
    std::atomic<bool> training_started(false);
    std::thread training_thread([&]() {
        training_started.store(true);
        auto result = training_->trainOnTheFly("stop_checkpoint_adapter", data, hyper);
        EXPECT_FALSE(result.success);
    });
    
    // Wait for training to start
    while (!training_started.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Stop training - should trigger checkpoint save
    training_->stopTraining();
    
    // Wait for completion
    training_thread.join();
    
    // Verify checkpoint was saved on stop
    bool checkpoint_found = false;
    for (const auto& entry : std::filesystem::directory_iterator(temp_dir)) {
        if (entry.path().extension() == ".json") {
            checkpoint_found = true;
            break;
        }
    }
    EXPECT_TRUE(checkpoint_found);
    
    // Cleanup
    std::filesystem::remove_all(temp_dir);
}

TEST_F(LoRAFrameworkTest, TrainingControl_Metrics) {
    // Create training data
    TrainingData data;
    for (int i = 0; i < 10; i++) {
        TrainingDataSample sample;
        sample.input = "Q" + std::to_string(i);
        sample.output = "A" + std::to_string(i);
        data.samples.push_back(sample);
    }
    
    // Set up callback to track metrics
    std::vector<TrainingMetrics> metrics_history;
    training_->registerCallback([&](const TrainingMetrics& metrics) {
        metrics_history.push_back(metrics);
    });
    
    // Configure training
    LoRAHyperparameters hyper;
    hyper.rank = 8;
    hyper.alpha = 16.0f;
    hyper.learning_rate = 0.0001f;
    hyper.num_epochs = 2;
    hyper.batch_size = 2;
    
    // Train
    auto result = training_->trainOnTheFly("metrics_adapter", data, hyper);
    EXPECT_TRUE(result.success);
    
    // Verify metrics were collected
    EXPECT_GT(metrics_history.size(), 0);
    
    // Verify final metrics
    auto final_metrics = training_->getMetrics();
    EXPECT_EQ(final_metrics.status, "completed");
    EXPECT_EQ(final_metrics.current_epoch, 2);
    EXPECT_GT(final_metrics.current_step, 0);
}

// ============================================================================
// LoRAOrchestrator Tests
// ============================================================================

TEST_F(LoRAFrameworkTest, Orchestrator_CreateAdapter) {
    // Create adapter via orchestrator
    TrainingData data;
    data.samples = {
        {"Test question", "Test answer", json{}}
    };
    
    std::string job_id = orchestrator_->createAdapter("orchestrated_adapter", data);
    EXPECT_FALSE(job_id.empty());
    
    // Verify adapter exists
    auto info = orchestrator_->getAdapter("orchestrated_adapter");
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->adapter_id, "orchestrated_adapter");
}

TEST_F(LoRAFrameworkTest, Orchestrator_UpdateAdapter) {
    // Create adapter
    TrainingData data;
    data.samples = {{"Q1", "A1", json{}}};
    
    orchestrator_->createAdapter("update_adapter", data);
    
    // Update adapter
    TrainingData update_data;
    update_data.samples = {{"Q2", "A2", json{}}};
    
    std::string job_id = orchestrator_->updateAdapter("update_adapter", update_data);
    EXPECT_FALSE(job_id.empty());
}

TEST_F(LoRAFrameworkTest, Orchestrator_ListAdapters) {
    // Create multiple adapters
    for (int i = 1; i <= 3; i++) {
        TrainingData data;
        data.samples = {{"Q", "A", json{}}};
        
        orchestrator_->createAdapter("list_adapter" + std::to_string(i), data);
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
    
    // Query history - API not yet implemented
    // TODO: Implement queryAuditLog API
    /*
    AuditQuery query;
    query.adapter_id = "test_adapter";
    query.limit = 3;
    
    auto results = audit_->queryAuditLog(query);
    EXPECT_EQ(results.size(), 3);
    */
    GTEST_SKIP() << "queryAuditLog API not yet implemented";
}

// ============================================================================
// themis_help_lora Tests
// ============================================================================

TEST_F(LoRAFrameworkTest, ThemisHelpLoRA_Query) {
    // Initialize themis_help_lora
    ThemisHelpLoRA::Config help_config;
    help_config.adapter_id = "themis_help_lora";
    help_config.base_model_id = "llama-2-7b";
    
    ThemisHelpLoRA help(help_config);
    
    // Query
    std::string response = help.query("How do I enable sharding?");
    EXPECT_FALSE(response.empty());
}

// TODO: Re-enable after getFeedbackStats() is implemented
/*
TEST_F(LoRAFrameworkTest, ThemisHelpLoRA_FeedbackCollection) {
    // Test disabled - API not yet implemented
}
*/

TEST_F(LoRAFrameworkTest, ThemisHelpLoRA_Training) {
    ThemisHelpLoRA::Config help_config;
    help_config.adapter_id = "themis_help_lora";
    help_config.base_model_id = "llama-2-7b";
    
    ThemisHelpLoRA help(help_config);
    
    // Add feedback
    for (int i = 0; i < 100; i++) {
        help.addPositiveFeedback("Question " + std::to_string(i), 
                                "Answer " + std::to_string(i));
    }
    
    // Train from feedback
    bool result = help.trainFromFeedback();
    EXPECT_TRUE(result);
    
    // Check version
    std::string version = help.getVersion();
    EXPECT_FALSE(version.empty());
}

TEST_F(LoRAFrameworkTest, ThemisHelpLoRA_ModelPathProviderIsUsed) {
    ThemisHelpLoRA::Config help_config;
    help_config.adapter_id = "themis_help_lora";
    help_config.base_model_id = "llama-2-7b";

    bool provider_called = false;
    help_config.model_path_provider = [&](std::string_view model_id) {
        provider_called = true;
        EXPECT_EQ(model_id, "llama-2-7b");
        return std::string("/tmp/nonexistent-model-for-provider-test.gguf");
    };

    ThemisHelpLoRA help(help_config);
    auto response = help.query("How do I enable sharding?");

    EXPECT_TRUE(provider_called);
    EXPECT_FALSE(response.empty());
}

// ============================================================================
// Integration Tests
// ============================================================================

// TODO: Re-enable after fixing API mismatches
/*
TEST_F(LoRAFrameworkTest, Integration_EndToEndWorkflow) {
    // Test code disabled - needs API updates
    GTEST_SKIP();
}
*/

// ============================================================================
// Performance Tests (basic)
// ============================================================================

TEST_F(LoRAFrameworkTest, Performance_AdapterLoading) {
    // Create adapter
    AdapterWeights weights;
    weights.hyperparameters.rank = 8;
    weights.data.resize(1024 * 1024);  // 1M bytes
    weights.size_bytes = weights.data.size();
    
    AdapterMetadata metadata;
    metadata.adapter_id = "perf_adapter";
    metadata.version = "v1.0";
    metadata.base_model = "llama-2-7b";
    
    storage_->saveAdapter("perf_adapter", weights, metadata);
    
    // Measure load time
    auto start = std::chrono::high_resolution_clock::now();
    manager_->loadLoRA("perf_adapter", "/tmp/perf_adapter.bin", "llama-2-7b", false, GPUPlacement::SINGLE_GPU, 1.0f);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Load should be < 500ms
    EXPECT_LT(duration.count(), 500);
    
    std::cout << "Adapter load time: " << duration.count() << " ms" << std::endl;
}

TEST_F(LoRAFrameworkTest, Performance_HotSwapping) {
    // Create two adapters
    AdapterWeights weights1;
    weights1.hyperparameters.rank = 8;
    weights1.data.resize(1024 * 1024);
    weights1.size_bytes = weights1.data.size();
    
    AdapterWeights weights2;
    weights2.hyperparameters.rank = 8;
    weights2.data.resize(1024 * 1024);
    weights2.size_bytes = weights2.data.size();
    
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
    
    manager_->loadLoRA("swap1", "/tmp/swap1.bin", "llama-2-7b", false, GPUPlacement::SINGLE_GPU, 1.0f);
    
    // Measure hot-swap time (unload + load)
    auto start = std::chrono::high_resolution_clock::now();
    manager_->unloadLoRA("swap1", false);
    manager_->loadLoRA("swap2", "/tmp/swap2.bin", "llama-2-7b", false, GPUPlacement::SINGLE_GPU, 1.0f);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Hot-swap should be < 10ms
    EXPECT_LT(duration.count(), 10);
    
    std::cout << "Hot-swap time: " << duration.count() << " ms" << std::endl;
}

// ============================================================================
// Main
// ============================================================================

