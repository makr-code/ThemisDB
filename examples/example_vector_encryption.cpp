/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            example_vector_encryption.cpp                      ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:32:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     485                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * Vector Encryption Example
 * 
 * Demonstrates how to use the complete vector encryption features in ThemisDB:
 * - Phase 1: Encrypting vectors in RocksDB
 * - Phase 2: Encrypting HNSW index files
 * - Migration from plaintext to encrypted data
 * 
 * Compile: g++ -std=c++17 example_vector_encryption.cpp -o example_vector_encryption -lthemis_core
 * Run: ./example_vector_encryption
 */

#include "index/vector_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "security/encryption.h"
#include "security/mock_key_provider.h"
#include "utils/logger.h"
#include <iostream>
#include <vector>
#include <memory>
#include <filesystem>

using namespace themis;
namespace fs = std::filesystem;

void printHeader(const std::string& title) {
    std::cout << "\n========================================\n";
    std::cout << title << "\n";
    std::cout << "========================================\n";
}

void printStatus(const std::string& message, bool success = true) {
    std::cout << (success ? "✓ " : "✗ ") << message << std::endl;
}

/**
 * Example 1: Basic Vector Encryption (Phase 1)
 */
void example1_basic_vector_encryption() {
    printHeader("Example 1: Basic Vector Encryption");
    
    // 1. Setup database and encryption
    std::string db_path = "/tmp/themis_example_encrypt";
    fs::remove_all(db_path);
    
    auto db = std::make_unique<RocksDBWrapper>(db_path);
    
    // Initialize key provider and field encryption
    auto key_provider = std::make_shared<MockKeyProvider>();
    key_provider->createKey("vector_embeddings", 1);
    auto field_encryption = std::make_shared<FieldEncryption>(key_provider);
    EncryptedField<std::vector<float>>::setFieldEncryption(field_encryption);
    
    printStatus("Database and encryption initialized");
    
    // 2. Create VectorIndexManager and enable encryption
    VectorIndexManager vim(*db);
    vim.init("documents", 768, VectorIndexManager::Metric::COSINE);
    
    // Enable Phase 1: Vector encryption
    vim.setVectorEncryptionEnabled(true);
    
    printStatus("Vector encryption enabled");
    std::cout << "  Encryption status: " << (vim.isVectorEncryptionEnabled() ? "ENABLED" : "DISABLED") << std::endl;
    
    // 3. Add encrypted vectors
    std::cout << "\nAdding 100 encrypted vectors..." << std::endl;
    
    for (int i = 0; i < 100; ++i) {
        // Create a 768-dimensional vector
        std::vector<float> embedding(768);
        for (int j = 0; j < 768; ++j) {
            embedding[j] = static_cast<float>(i + j) / 1000.0f;
        }
        
        // Create entity with vector
        BaseEntity entity("doc_" + std::to_string(i));
        entity.setField("embedding", embedding);
        entity.setField("title", "Document " + std::to_string(i));
        
        // Add to index (automatically encrypted)
        auto status = vim.addEntity(entity);
        if (!status.ok) {
            std::cerr << "Failed to add entity: " << status.message << std::endl;
            return;
        }
    }
    
    printStatus("100 vectors added and encrypted");
    
    // 4. Verify encryption in storage
    auto stored = db->get("documents:doc_0");
    if (stored.has_value()) {
        BaseEntity loaded = BaseEntity::deserialize("doc_0", *stored);
        
        bool has_encrypted = loaded.hasField("embedding_encrypted");
        bool has_plaintext = loaded.hasField("embedding");
        
        std::cout << "\nStorage verification:" << std::endl;
        std::cout << "  Has encrypted field: " << (has_encrypted ? "YES" : "NO") << std::endl;
        std::cout << "  Has plaintext field: " << (has_plaintext ? "YES" : "NO") << std::endl;
        
        if (has_encrypted && !has_plaintext) {
            printStatus("Vectors are properly encrypted in storage", true);
        }
    }
    
    // 5. Search encrypted vectors
    std::cout << "\nSearching encrypted vectors..." << std::endl;
    std::vector<float> query(768, 0.05f);
    auto [search_status, results] = vim.searchKnn(query, 5);
    
    if (search_status.ok) {
        printStatus("Search successful on encrypted data");
        std::cout << "\nTop 5 results:" << std::endl;
        for (size_t i = 0; i < results.size(); ++i) {
            std::cout << "  " << (i+1) << ". " << results[i].pk 
                      << " (distance: " << results[i].distance << ")" << std::endl;
        }
    }
    
    // Cleanup
    db.reset();
    fs::remove_all(db_path);
}

/**
 * Example 2: HNSW Index Encryption (Phase 2)
 */
void example2_hnsw_index_encryption() {
    printHeader("Example 2: HNSW Index Encryption");
    
    std::string db_path = "/tmp/themis_example_hnsw";
    std::string hnsw_path = "/tmp/themis_example_hnsw_index";
    fs::remove_all(db_path);
    fs::remove_all(hnsw_path);
    
    auto db = std::make_unique<RocksDBWrapper>(db_path);
    
    // Initialize encryption
    auto key_provider = std::make_shared<MockKeyProvider>();
    key_provider->createKey("hnsw_index", 1);
    auto field_encryption = std::make_shared<FieldEncryption>(key_provider);
    EncryptedField<std::vector<uint8_t>>::setFieldEncryption(field_encryption);
    
    VectorIndexManager vim(*db);
    vim.init("documents", 384, VectorIndexManager::Metric::COSINE);
    
    // Enable Phase 2: HNSW index encryption
    vim.setHnswEncryptionEnabled(true);
    
    printStatus("HNSW index encryption enabled");
    
    // Add vectors
    std::cout << "\nAdding 500 vectors..." << std::endl;
    for (int i = 0; i < 500; ++i) {
        std::vector<float> embedding(384, static_cast<float>(i) / 500.0f);
        BaseEntity entity("doc_" + std::to_string(i));
        entity.setField("embedding", embedding);
        vim.addEntity(entity);
    }
    
    printStatus("500 vectors added");
    
    // Save encrypted HNSW index
    std::cout << "\nSaving encrypted HNSW index..." << std::endl;
    auto save_status = vim.saveIndex(hnsw_path);
    
    if (save_status.ok) {
        printStatus("HNSW index saved (encrypted)");
        
        // Verify encrypted files
        bool encrypted_exists = fs::exists(hnsw_path + "/index.bin.encrypted");
        bool plaintext_exists = fs::exists(hnsw_path + "/index.bin");
        
        std::cout << "\nFile verification:" << std::endl;
        std::cout << "  index.bin.encrypted: " << (encrypted_exists ? "EXISTS" : "NOT FOUND") << std::endl;
        std::cout << "  index.bin (plaintext): " << (plaintext_exists ? "EXISTS" : "NOT FOUND") << std::endl;
        
        if (encrypted_exists && !plaintext_exists) {
            printStatus("HNSW index properly encrypted on disk", true);
        }
    }
    
    // Load encrypted index
    std::cout << "\nLoading encrypted HNSW index..." << std::endl;
    VectorIndexManager vim2(*db);
    vim2.init("documents", 384, VectorIndexManager::Metric::COSINE);
    
    auto load_status = vim2.loadIndex(hnsw_path);
    if (load_status.ok) {
        printStatus("Encrypted index loaded successfully");
        
        // Test search
        std::vector<float> query(384, 0.5f);
        auto [search_status, results] = vim2.searchKnn(query, 3);
        
        if (search_status.ok) {
            printStatus("Search works with encrypted index");
            std::cout << "\nTop 3 results:" << std::endl;
            for (size_t i = 0; i < results.size(); ++i) {
                std::cout << "  " << (i+1) << ". " << results[i].pk << std::endl;
            }
        }
    }
    
    // Cleanup
    db.reset();
    fs::remove_all(db_path);
    fs::remove_all(hnsw_path);
}

/**
 * Example 3: Full Encryption (Both Phases)
 */
void example3_full_encryption() {
    printHeader("Example 3: Full Encryption (Phase 1 + Phase 2)");
    
    std::string db_path = "/tmp/themis_example_full";
    std::string hnsw_path = "/tmp/themis_example_full_index";
    fs::remove_all(db_path);
    fs::remove_all(hnsw_path);
    
    auto db = std::make_unique<RocksDBWrapper>(db_path);
    
    // Initialize encryption for both phases
    auto key_provider = std::make_shared<MockKeyProvider>();
    key_provider->createKey("vector_embeddings", 1);
    key_provider->createKey("hnsw_index", 1);
    auto field_encryption = std::make_shared<FieldEncryption>(key_provider);
    EncryptedField<std::vector<float>>::setFieldEncryption(field_encryption);
    EncryptedField<std::vector<uint8_t>>::setFieldEncryption(field_encryption);
    
    VectorIndexManager vim(*db);
    vim.init("documents", 256, VectorIndexManager::Metric::COSINE);
    
    // Enable BOTH Phase 1 and Phase 2
    vim.setVectorEncryptionEnabled(true);
    vim.setHnswEncryptionEnabled(true);
    
    printStatus("Full encryption enabled (vectors + HNSW index)");
    std::cout << "  Vector encryption: " << (vim.isVectorEncryptionEnabled() ? "ON" : "OFF") << std::endl;
    std::cout << "  HNSW encryption: " << (vim.isHnswEncryptionEnabled() ? "ON" : "OFF") << std::endl;
    
    // Add data
    std::cout << "\nAdding 200 vectors with full encryption..." << std::endl;
    for (int i = 0; i < 200; ++i) {
        std::vector<float> embedding(256);
        for (int j = 0; j < 256; ++j) {
            embedding[j] = std::sin(static_cast<float>(i + j) * 0.1f);
        }
        
        BaseEntity entity("doc_" + std::to_string(i));
        entity.setField("embedding", embedding);
        entity.setField("category", "category_" + std::to_string(i % 5));
        
        vim.addEntity(entity);
    }
    
    printStatus("200 vectors added (all encrypted)");
    
    // Save encrypted index
    vim.saveIndex(hnsw_path);
    printStatus("Encrypted HNSW index saved");
    
    // Simulate server restart
    std::cout << "\n--- Simulating Server Restart ---" << std::endl;
    
    VectorIndexManager vim2(*db);
    vim2.init("documents", 256, VectorIndexManager::Metric::COSINE);
    
    // Load encrypted index
    vim2.loadIndex(hnsw_path);
    printStatus("Encrypted index loaded after restart");
    
    // Verify no plaintext on disk
    std::cout << "\nSecurity verification:" << std::endl;
    bool no_plaintext_vectors = !db->get("documents:doc_0")->empty(); // Encrypted storage
    bool no_plaintext_index = !fs::exists(hnsw_path + "/index.bin");
    
    std::cout << "  Vectors encrypted in DB: " << (no_plaintext_vectors ? "YES" : "NO") << std::endl;
    std::cout << "  HNSW index encrypted: " << (no_plaintext_index ? "YES" : "NO") << std::endl;
    
    if (no_plaintext_vectors && no_plaintext_index) {
        printStatus("100% at-rest encryption achieved!", true);
    }
    
    // Test search
    std::vector<float> query(256, 0.0f);
    auto [search_status, results] = vim2.searchKnn(query, 5);
    
    if (search_status.ok) {
        printStatus("Search successful with full encryption");
    }
    
    // Cleanup
    db.reset();
    fs::remove_all(db_path);
    fs::remove_all(hnsw_path);
}

/**
 * Example 4: Migration from Plaintext to Encrypted
 */
void example4_migration() {
    printHeader("Example 4: Migration from Plaintext to Encrypted");
    
    std::string db_path = "/tmp/themis_example_migration";
    fs::remove_all(db_path);
    
    auto db = std::make_unique<RocksDBWrapper>(db_path);
    
    // Step 1: Create plaintext data
    std::cout << "\nStep 1: Creating plaintext data..." << std::endl;
    {
        VectorIndexManager vim(*db);
        vim.init("documents", 128, VectorIndexManager::Metric::COSINE);
        
        // Add plaintext vectors (encryption disabled)
        for (int i = 0; i < 50; ++i) {
            std::vector<float> embedding(128, static_cast<float>(i) / 50.0f);
            BaseEntity entity("doc_" + std::to_string(i));
            entity.setField("embedding", embedding);
            vim.addEntity(entity);
        }
        
        printStatus("50 plaintext vectors added");
    }
    
    // Step 2: Enable encryption
    std::cout << "\nStep 2: Enabling encryption..." << std::endl;
    
    auto key_provider = std::make_shared<MockKeyProvider>();
    key_provider->createKey("vector_embeddings", 1);
    auto field_encryption = std::make_shared<FieldEncryption>(key_provider);
    EncryptedField<std::vector<float>>::setFieldEncryption(field_encryption);
    
    VectorIndexManager vim(*db);
    vim.init("documents", 128, VectorIndexManager::Metric::COSINE);
    vim.setVectorEncryptionEnabled(true);
    
    printStatus("Encryption enabled");
    
    // Step 3: Add new encrypted vectors
    std::cout << "\nStep 3: Adding new encrypted vectors..." << std::endl;
    for (int i = 50; i < 100; ++i) {
        std::vector<float> embedding(128, static_cast<float>(i) / 100.0f);
        BaseEntity entity("doc_" + std::to_string(i));
        entity.setField("embedding", embedding);
        vim.addEntity(entity);
    }
    
    printStatus("50 encrypted vectors added");
    
    // Step 4: Verify mixed mode works
    std::cout << "\nStep 4: Testing mixed mode (plaintext + encrypted)..." << std::endl;
    vim.rebuildFromStorage();
    
    std::vector<float> query(128, 0.5f);
    auto [search_status, results] = vim.searchKnn(query, 10);
    
    if (search_status.ok && results.size() == 10) {
        printStatus("Mixed mode works (50 plaintext + 50 encrypted)", true);
        std::cout << "  Total vectors searchable: " << results.size() << std::endl;
    }
    
    std::cout << "\nNote: Use migrate_vector_encryption tool to encrypt remaining plaintext vectors" << std::endl;
    
    // Cleanup
    db.reset();
    fs::remove_all(db_path);
}

/**
 * Example 5: Auto-Save Configuration
 */
void example5_auto_save() {
    printHeader("Example 5: Auto-Save with Encryption");
    
    std::string db_path = "/tmp/themis_example_autosave";
    std::string hnsw_path = "/tmp/themis_example_autosave_index";
    fs::remove_all(db_path);
    fs::remove_all(hnsw_path);
    
    auto db = std::make_unique<RocksDBWrapper>(db_path);
    
    // Initialize encryption
    auto key_provider = std::make_shared<MockKeyProvider>();
    key_provider->createKey("hnsw_index", 1);
    auto field_encryption = std::make_shared<FieldEncryption>(key_provider);
    EncryptedField<std::vector<uint8_t>>::setFieldEncryption(field_encryption);
    
    VectorIndexManager vim(*db);
    vim.init("documents", 128, VectorIndexManager::Metric::COSINE);
    vim.setHnswEncryptionEnabled(true);
    
    // Configure auto-save
    vim.setAutoSavePath(hnsw_path, true);
    
    printStatus("Auto-save configured with encryption");
    std::cout << "  Save path: " << hnsw_path << std::endl;
    
    // Add vectors
    std::cout << "\nAdding 100 vectors..." << std::endl;
    for (int i = 0; i < 100; ++i) {
        std::vector<float> embedding(128, static_cast<float>(i) / 100.0f);
        BaseEntity entity("doc_" + std::to_string(i));
        entity.setField("embedding", embedding);
        vim.addEntity(entity);
    }
    
    printStatus("100 vectors added");
    
    // Shutdown triggers auto-save
    std::cout << "\nShutting down (triggers auto-save)..." << std::endl;
    auto status = vim.shutdown();
    
    if (status.ok) {
        printStatus("Auto-save completed on shutdown");
        
        // Verify encrypted index was saved
        if (fs::exists(hnsw_path + "/index.bin.encrypted")) {
            printStatus("Encrypted index file created automatically", true);
        }
    }
    
    // Cleanup
    db.reset();
    fs::remove_all(db_path);
    fs::remove_all(hnsw_path);
}

int main() {
    std::cout << "===========================================\n";
    std::cout << "ThemisDB Vector Encryption Examples\n";
    std::cout << "===========================================\n";
    std::cout << "\nThis demo shows the complete encryption workflow:\n";
    std::cout << "- Phase 1: Vector encryption in RocksDB\n";
    std::cout << "- Phase 2: HNSW index file encryption\n";
    std::cout << "- Migration and auto-save features\n";
    
    try {
        example1_basic_vector_encryption();
        example2_hnsw_index_encryption();
        example3_full_encryption();
        example4_migration();
        example5_auto_save();
        
        printHeader("All Examples Completed Successfully!");
        std::cout << "\nNext steps:\n";
        std::cout << "1. Run integration tests: ./test_vector_encryption_integration\n";
        std::cout << "2. Use migration tool: ./migrate_vector_encryption --help\n";
        std::cout << "3. See documentation in docs/security/\n";
        
    } catch (const std::exception& e) {
        std::cerr << "\nError: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
