/**
 * Integration Tests for Vector Encryption (Phase 1 + Phase 2)
 * 
 * Tests the complete encryption workflow including:
 * - Phase 1: Vector encryption in RocksDB
 * - Phase 2: HNSW index file encryption
 * - End-to-end encryption with both phases enabled
 * - Backward compatibility with plaintext data
 * 
 * Build: Add to CMakeLists.txt test section
 * Run: ./test_vector_encryption_integration
 */

#include <gtest/gtest.h>
#include "index/vector_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "security/encryption.h"
#include "security/mock_key_provider.h"
#include "utils/logger.h"
#include <filesystem>
#include <vector>
#include <memory>
#include <fstream>

using namespace themis;
namespace fs = std::filesystem;

class VectorEncryptionIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clean up test directories
        test_db_path_ = "/tmp/test_vector_enc_integration";
        test_hnsw_path_ = "/tmp/test_hnsw_enc_integration";
        
        fs::remove_all(test_db_path_);
        fs::remove_all(test_hnsw_path_);
        
        // Initialize encryption
        key_provider_ = std::make_shared<MockKeyProvider>();
        key_provider_->createKey("vector_embeddings", 1);
        key_provider_->createKey("hnsw_index", 1);
        
        field_encryption_ = std::make_shared<FieldEncryption>(key_provider_);
        
        // Set global FieldEncryption for EncryptedField templates
        EncryptedField<std::vector<float>>::setFieldEncryption(field_encryption_);
        EncryptedField<std::vector<uint8_t>>::setFieldEncryption(field_encryption_);
        
        // Create database
        RocksDBWrapper::Config cfg;
        cfg.db_path = test_db_path_;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());
    }
    
    void TearDown() override {
        db_.reset();
        fs::remove_all(test_db_path_);
        fs::remove_all(test_hnsw_path_);
    }
    
    std::string test_db_path_;
    std::string test_hnsw_path_;
    std::shared_ptr<MockKeyProvider> key_provider_;
    std::shared_ptr<FieldEncryption> field_encryption_;
    std::unique_ptr<RocksDBWrapper> db_;
};

// Test 1: Phase 1 Only - Vector Encryption
TEST_F(VectorEncryptionIntegrationTest, Phase1_VectorEncryptionOnly) {
    VectorIndexManager vim(*db_);
    auto status = vim.init("documents", 128, VectorIndexManager::Metric::COSINE);
    ASSERT_TRUE(status.ok);
    
    // Enable Phase 1 encryption
    vim.setVectorEncryptionEnabled(true);
    EXPECT_TRUE(vim.isVectorEncryptionEnabled());
    
    // Add encrypted vectors
    for (int i = 0; i < 100; ++i) {
        std::vector<float> embedding(128);
        for (int j = 0; j < 128; ++j) {
            embedding[j] = static_cast<float>(i + j) / 1000.0f;
        }
        
        BaseEntity entity("doc" + std::to_string(i));
        entity.setField("embedding", embedding);
        entity.setField("title", "Document " + std::to_string(i));
        
        auto add_status = vim.addEntity(entity);
        ASSERT_TRUE(add_status.ok) << "Failed to add entity " << i;
    }
    
    // Flush pending encrypted writes
    vim.flushEncryptedWrites();
    
    // Verify encrypted storage
    auto stored = db_->get("documents:doc0");
    ASSERT_TRUE(stored.has_value());
    
    BaseEntity loaded = BaseEntity::deserialize("doc0", *stored);
    EXPECT_TRUE(loaded.hasField("embedding_encrypted"));
    EXPECT_FALSE(loaded.hasField("embedding")); // Plaintext removed
    
    // Verify search works (vectors decrypted in memory)
    std::vector<float> query(128, 0.05f);
    auto [search_status, results] = vim.searchKnn(query, 10);
    ASSERT_TRUE(search_status.ok);
    ASSERT_EQ(results.size(), 10);
    EXPECT_EQ(results[0].pk, "doc50"); // Closest match
}

// Test 2: Phase 2 Only - HNSW Index Encryption
TEST_F(VectorEncryptionIntegrationTest, Phase2_HnswEncryptionOnly) {
    VectorIndexManager vim(*db_);
    auto status = vim.init("documents", 128, VectorIndexManager::Metric::COSINE);
    ASSERT_TRUE(status.ok);
    
    // Add vectors (plaintext in RocksDB for this test)
    for (int i = 0; i < 100; ++i) {
        std::vector<float> embedding(128, static_cast<float>(i) / 100.0f);
        BaseEntity entity("doc" + std::to_string(i));
        entity.setField("embedding", embedding);
        vim.addEntity(entity);
    }
    
    // Enable Phase 2 encryption
    vim.setHnswEncryptionEnabled(true);
    EXPECT_TRUE(vim.isHnswEncryptionEnabled());
    
    // Save encrypted HNSW index
    auto save_status = vim.saveIndex(test_hnsw_path_);
    ASSERT_TRUE(save_status.ok);
    
    // Verify encrypted files
    EXPECT_TRUE(fs::exists(test_hnsw_path_ + "/index.bin.encrypted"));
    EXPECT_FALSE(fs::exists(test_hnsw_path_ + "/index.bin")); // Plaintext not created
    EXPECT_TRUE(fs::exists(test_hnsw_path_ + "/meta.txt"));
    
    // Verify meta.txt contains "encrypted" flag
    std::ifstream meta(test_hnsw_path_ + "/meta.txt");
    std::string line;
    bool found_encrypted_flag = false;
    while (std::getline(meta, line)) {
        if (line == "encrypted") {
            found_encrypted_flag = true;
            break;
        }
    }
    EXPECT_TRUE(found_encrypted_flag);
    
    // Load encrypted index
    VectorIndexManager vim2(*db_);
    vim2.init("documents", 128, VectorIndexManager::Metric::COSINE);
    auto load_status = vim2.loadIndex(test_hnsw_path_);
    ASSERT_TRUE(load_status.ok);
    
    // Verify search works after loading encrypted index
    std::vector<float> query(128, 0.5f);
    auto [search_status, results] = vim2.searchKnn(query, 5);
    ASSERT_TRUE(search_status.ok);
    EXPECT_EQ(results.size(), 5);
}

// Test 3: Full Encryption - Both Phase 1 + Phase 2
TEST_F(VectorEncryptionIntegrationTest, FullEncryption_BothPhases) {
    VectorIndexManager vim(*db_);
    auto status = vim.init("documents", 128, VectorIndexManager::Metric::COSINE);
    ASSERT_TRUE(status.ok);
    
    // Enable BOTH Phase 1 and Phase 2 encryption
    vim.setVectorEncryptionEnabled(true);
    vim.setHnswEncryptionEnabled(true);
    
    EXPECT_TRUE(vim.isVectorEncryptionEnabled());
    EXPECT_TRUE(vim.isHnswEncryptionEnabled());
    
    // Add encrypted vectors
    for (int i = 0; i < 200; ++i) {
        std::vector<float> embedding(128);
        for (int j = 0; j < 128; ++j) {
            embedding[j] = std::sin(static_cast<float>(i + j) * 0.1f);
        }
        
        BaseEntity entity("doc" + std::to_string(i));
        entity.setField("embedding", embedding);
        vim.addEntity(entity);
    }
    
    // Save encrypted HNSW index
    auto save_status = vim.saveIndex(test_hnsw_path_);
    ASSERT_TRUE(save_status.ok);
    
    // Simulate server restart
    VectorIndexManager vim2(*db_);
    vim2.init("documents", 128, VectorIndexManager::Metric::COSINE);
    
    // Load encrypted HNSW index
    auto load_status = vim2.loadIndex(test_hnsw_path_);
    ASSERT_TRUE(load_status.ok);
    
    // Verify search works with full encryption
    std::vector<float> query(128, 0.5f);
    auto [search_status, results] = vim2.searchKnn(query, 10);
    ASSERT_TRUE(search_status.ok);
    EXPECT_EQ(results.size(), 10);
    
    // Verify no plaintext on disk
    EXPECT_FALSE(fs::exists(test_hnsw_path_ + "/index.bin"));
    EXPECT_TRUE(fs::exists(test_hnsw_path_ + "/index.bin.encrypted"));
}

// Test 4: Backward Compatibility - Load Plaintext Index
TEST_F(VectorEncryptionIntegrationTest, BackwardCompatibility_PlaintextIndex) {
    // Step 1: Create plaintext index
    {
        VectorIndexManager vim(*db_);
        vim.init("documents", 128, VectorIndexManager::Metric::COSINE);
        
        // Add vectors without encryption
        for (int i = 0; i < 50; ++i) {
            std::vector<float> embedding(128, static_cast<float>(i) / 50.0f);
            BaseEntity entity("doc" + std::to_string(i));
            entity.setField("embedding", embedding);
            vim.addEntity(entity);
        }
        
        // Save plaintext index
        auto save_status = vim.saveIndex(test_hnsw_path_);
        ASSERT_TRUE(save_status.ok);
        
        EXPECT_TRUE(fs::exists(test_hnsw_path_ + "/index.bin"));
        EXPECT_FALSE(fs::exists(test_hnsw_path_ + "/index.bin.encrypted"));
    }
    
    // Step 2: Load plaintext index (without requiring encryption)
    {
        VectorIndexManager vim2(*db_);
        vim2.init("documents", 128, VectorIndexManager::Metric::COSINE);
        
        auto load_status = vim2.loadIndex(test_hnsw_path_);
        ASSERT_TRUE(load_status.ok) << "Should load plaintext index successfully";
        
        // Verify search works
        std::vector<float> query(128, 0.5f);
        auto [search_status, results] = vim2.searchKnn(query, 5);
        ASSERT_TRUE(search_status.ok);
        EXPECT_EQ(results.size(), 5);
    }
}

// Test 5: Mixed Mode - Encrypted Vectors + Plaintext Vectors
TEST_F(VectorEncryptionIntegrationTest, MixedMode_EncryptedAndPlaintext) {
    VectorIndexManager vim(*db_);
    vim.init("documents", 128, VectorIndexManager::Metric::COSINE);
    
    // Add plaintext vectors
    for (int i = 0; i < 50; ++i) {
        std::vector<float> embedding(128, static_cast<float>(i) / 100.0f);
        BaseEntity entity("plaintext_doc" + std::to_string(i));
        entity.setField("embedding", embedding);
        vim.addEntity(entity);
    }
    
    // Enable encryption
    vim.setVectorEncryptionEnabled(true);
    
    // Add encrypted vectors
    for (int i = 0; i < 50; ++i) {
        std::vector<float> embedding(128, static_cast<float>(i + 50) / 100.0f);
        BaseEntity entity("encrypted_doc" + std::to_string(i));
        entity.setField("embedding", embedding);
        vim.addEntity(entity);
    }
    
    // Rebuild from storage (should handle both encrypted and plaintext)
    auto rebuild_status = vim.rebuildFromStorage();
    ASSERT_TRUE(rebuild_status.ok);
    
    // Verify search works with mixed data
    std::vector<float> query(128, 0.5f);
    auto [search_status, results] = vim.searchKnn(query, 10);
    ASSERT_TRUE(search_status.ok);
    EXPECT_EQ(results.size(), 10);
}

// Test 6: Performance Test - Encryption Overhead
TEST_F(VectorEncryptionIntegrationTest, Performance_EncryptionOverhead) {
    const int NUM_VECTORS = 1000;
    const int DIM = 768;
    
    VectorIndexManager vim(*db_);
    vim.init("documents", DIM, VectorIndexManager::Metric::COSINE);
    vim.setVectorEncryptionEnabled(true);
    vim.setHnswEncryptionEnabled(true);
    
    // Measure insertion time with encryption
    auto start = std::chrono::steady_clock::now();
    
    for (int i = 0; i < NUM_VECTORS; ++i) {
        std::vector<float> embedding(DIM, 0.5f);
        BaseEntity entity("doc" + std::to_string(i));
        entity.setField("embedding", embedding);
        vim.addEntity(entity);
    }
    
    auto insert_elapsed = std::chrono::steady_clock::now() - start;
    auto insert_ms = std::chrono::duration_cast<std::chrono::milliseconds>(insert_elapsed).count();
    
    std::cout << "Encrypted insert (" << NUM_VECTORS << " vectors): " << insert_ms << " ms" << std::endl;
    std::cout << "Per-vector: " << (insert_ms / static_cast<double>(NUM_VECTORS)) << " ms" << std::endl;
    
    // Measure HNSW save time
    start = std::chrono::steady_clock::now();
    vim.saveIndex(test_hnsw_path_);
    auto save_elapsed = std::chrono::steady_clock::now() - start;
    auto save_ms = std::chrono::duration_cast<std::chrono::milliseconds>(save_elapsed).count();
    
    std::cout << "Encrypted HNSW save: " << save_ms << " ms" << std::endl;
    
    // Measure HNSW load time
    VectorIndexManager vim2(*db_);
    vim2.init("documents", DIM, VectorIndexManager::Metric::COSINE);
    
    start = std::chrono::steady_clock::now();
    vim2.loadIndex(test_hnsw_path_);
    auto load_elapsed = std::chrono::steady_clock::now() - start;
    auto load_ms = std::chrono::duration_cast<std::chrono::milliseconds>(load_elapsed).count();
    
    std::cout << "Encrypted HNSW load: " << load_ms << " ms" << std::endl;
    
    // Performance targets (adjust based on actual performance)
    EXPECT_LT(insert_ms / static_cast<double>(NUM_VECTORS), 5.0) << "Insert should be < 5ms per vector";
    EXPECT_LT(save_ms, 30000) << "Save should be < 30 seconds";
    EXPECT_LT(load_ms, 30000) << "Load should be < 30 seconds";
}

// Test 7: Error Handling - Missing Keys
TEST_F(VectorEncryptionIntegrationTest, ErrorHandling_MissingEncryptionKey) {
    // Don't set FieldEncryption (simulate missing key scenario)
    EncryptedField<std::vector<float>>::setFieldEncryption(nullptr);
    
    VectorIndexManager vim(*db_);
    vim.init("documents", 128, VectorIndexManager::Metric::COSINE);
    vim.setVectorEncryptionEnabled(true);
    
    std::vector<float> embedding(128, 0.5f);
    BaseEntity entity("doc1");
    entity.setField("embedding", embedding);
    
    // This should fail gracefully
    auto status = vim.addEntity(entity);
    EXPECT_FALSE(status.ok);
    EXPECT_TRUE(status.message.find("encryption") != std::string::npos ||
                status.message.find("FieldEncryption") != std::string::npos);
    
    // Restore for cleanup
    EncryptedField<std::vector<float>>::setFieldEncryption(field_encryption_);
}

// Test 8: Auto-Save on Shutdown
TEST_F(VectorEncryptionIntegrationTest, AutoSave_OnShutdown) {
    VectorIndexManager vim(*db_);
    vim.init("documents", 128, VectorIndexManager::Metric::COSINE);
    vim.setVectorEncryptionEnabled(true);
    vim.setHnswEncryptionEnabled(true);
    
    // Add vectors
    for (int i = 0; i < 50; ++i) {
        std::vector<float> embedding(128, static_cast<float>(i) / 50.0f);
        BaseEntity entity("doc" + std::to_string(i));
        entity.setField("embedding", embedding);
        vim.addEntity(entity);
    }
    
    // Enable auto-save
    vim.setAutoSavePath(test_hnsw_path_, true);
    
    // Shutdown should trigger auto-save
    auto status = vim.shutdown();
    ASSERT_TRUE(status.ok);
    
    // Verify encrypted index was saved
    EXPECT_TRUE(fs::exists(test_hnsw_path_ + "/index.bin.encrypted"));
}

// Note: No custom main here; linked with GTest::gtest_main
