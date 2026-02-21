/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_hsm_key_provider_adapter.cpp                  ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 11:03:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     403                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 841aa5790  2026-01-16  Integrate HSMProvider (PKCS#11) for LoRA Adapter Encrypti... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "security/hsm_key_provider_adapter.h"
#include "security/hsm_provider.h"
#include <filesystem>

using namespace themis;
using namespace themis::security;

/**
 * HSMKeyProviderAdapter Tests
 * 
 * These tests verify the adapter that bridges HSMProvider with KeyProvider interface
 * for use with FieldEncryption in LoRA adapter storage.
 * 
 * Note: These tests require SoftHSM2 to be installed and configured.
 * See test_hsm_provider.cpp for setup instructions.
 */

class HSMKeyProviderAdapterTest : public ::testing::Test {
protected:
    std::string hsm_library_path;
    std::string hsm_pin;
    uint32_t hsm_slot = 0;
    
    void SetUp() override {
        // Get HSM library path from environment or use default
        const char* env_lib = std::getenv("THEMIS_TEST_HSM_LIBRARY");
        if (env_lib) {
            hsm_library_path = env_lib;
        } else {
            // Try common locations
            std::vector<std::string> common_paths = {
                "/usr/lib/softhsm/libsofthsm2.so",
                "/usr/lib/x86_64-linux-gnu/softhsm/libsofthsm2.so",
                "/usr/local/lib/softhsm/libsofthsm2.so",
                "/opt/homebrew/lib/softhsm/libsofthsm2.so",
            };
            
            for (const auto& path : common_paths) {
                if (std::filesystem::exists(path)) {
                    hsm_library_path = path;
                    break;
                }
            }
        }
        
        // Get PIN from environment or use default
        const char* env_pin = std::getenv("THEMIS_TEST_HSM_PIN");
        hsm_pin = env_pin ? env_pin : "1234";
    }
    
    bool isHSMAvailable() const {
        return !hsm_library_path.empty() && 
               std::filesystem::exists(hsm_library_path);
    }
    
    HSMConfig createTestConfig() {
        HSMConfig config;
        config.library_path = hsm_library_path;
        config.slot_id = hsm_slot;
        config.pin = hsm_pin;
        config.key_label = "lora-adapter-kek";
        config.signature_algorithm = "RSA-SHA256";
        config.verbose = false;
        config.session_pool_size = 2;
        return config;
    }
    
    std::shared_ptr<HSMProvider> createHSM() {
        auto config = createTestConfig();
        auto hsm = std::make_shared<HSMProvider>(config);
        if (!hsm->initialize()) {
            return nullptr;
        }
        return hsm;
    }
};

TEST_F(HSMKeyProviderAdapterTest, ConstructorRequiresValidHSM) {
    if (!isHSMAvailable()) {
        GTEST_SKIP() << "HSM not available - skipping test";
    }
    
    // Null HSM should throw
    EXPECT_THROW({
        HSMKeyProviderAdapter adapter(nullptr);
    }, std::invalid_argument);
    
    // Uninitialized HSM should throw
    auto config = createTestConfig();
    auto hsm = std::make_shared<HSMProvider>(config);
    // Don't call initialize()
    EXPECT_THROW({
        HSMKeyProviderAdapter adapter(hsm);
    }, std::invalid_argument);
}

TEST_F(HSMKeyProviderAdapterTest, ConstructorWithValidHSM) {
    if (!isHSMAvailable()) {
        GTEST_SKIP() << "HSM not available - skipping test";
    }
    
    auto hsm = createHSM();
    if (!hsm) {
        GTEST_SKIP() << "Failed to initialize HSM";
    }
    
    EXPECT_NO_THROW({
        HSMKeyProviderAdapter adapter(hsm);
    });
}

TEST_F(HSMKeyProviderAdapterTest, CreateKeyFromBytes) {
    if (!isHSMAvailable()) {
        GTEST_SKIP() << "HSM not available - skipping test";
    }
    
    auto hsm = createHSM();
    if (!hsm) {
        GTEST_SKIP() << "Failed to initialize HSM";
    }
    
    HSMKeyProviderAdapter adapter(hsm);
    
    // Create a 32-byte key
    std::vector<uint8_t> key_bytes(32, 0x42);
    
    KeyMetadata metadata;
    metadata.status = KeyStatus::ACTIVE;
    metadata.algorithm = "AES-256-GCM";
    
    uint32_t version = adapter.createKeyFromBytes("test-key", key_bytes, metadata);
    
    EXPECT_EQ(version, 1);
    EXPECT_TRUE(adapter.hasKey("test-key"));
    EXPECT_TRUE(adapter.hasKey("test-key", 1));
}

TEST_F(HSMKeyProviderAdapterTest, CreateKeyFromBytesInvalidSize) {
    if (!isHSMAvailable()) {
        GTEST_SKIP() << "HSM not available - skipping test";
    }
    
    auto hsm = createHSM();
    if (!hsm) {
        GTEST_SKIP() << "Failed to initialize HSM";
    }
    
    HSMKeyProviderAdapter adapter(hsm);
    
    // Wrong size (not 32 bytes)
    std::vector<uint8_t> key_bytes(16, 0x42);
    
    EXPECT_THROW({
        adapter.createKeyFromBytes("test-key", key_bytes);
    }, std::invalid_argument);
}

TEST_F(HSMKeyProviderAdapterTest, GetKeyAndCache) {
    if (!isHSMAvailable()) {
        GTEST_SKIP() << "HSM not available - skipping test";
    }
    
    auto hsm = createHSM();
    if (!hsm) {
        GTEST_SKIP() << "Failed to initialize HSM";
    }
    
    HSMKeyProviderAdapter::Config config;
    config.enable_caching = true;
    config.cache_ttl_ms = 5000; // 5 seconds
    
    HSMKeyProviderAdapter adapter(hsm, config);
    
    // Create a key
    std::vector<uint8_t> key_bytes(32, 0x42);
    adapter.createKeyFromBytes("test-key", key_bytes);
    
    // Get key - should be cached after first retrieval
    auto key1 = adapter.getKey("test-key");
    EXPECT_EQ(key1.size(), 32);
    
    // Get key again - should hit cache
    auto key2 = adapter.getKey("test-key");
    EXPECT_EQ(key2.size(), 32);
    
    // Check stats
    auto stats = adapter.getStats();
    EXPECT_GT(stats["cache_hits"].get<uint64_t>(), 0);
}

TEST_F(HSMKeyProviderAdapterTest, RotateKey) {
    if (!isHSMAvailable()) {
        GTEST_SKIP() << "HSM not available - skipping test";
    }
    
    auto hsm = createHSM();
    if (!hsm) {
        GTEST_SKIP() << "Failed to initialize HSM";
    }
    
    HSMKeyProviderAdapter adapter(hsm);
    
    // Create initial key
    std::vector<uint8_t> key_bytes(32, 0x42);
    uint32_t v1 = adapter.createKeyFromBytes("test-key", key_bytes);
    EXPECT_EQ(v1, 1);
    
    // Rotate key
    uint32_t v2 = adapter.rotateKey("test-key");
    EXPECT_EQ(v2, 2);
    
    // Both versions should exist
    EXPECT_TRUE(adapter.hasKey("test-key", 1));
    EXPECT_TRUE(adapter.hasKey("test-key", 2));
    
    // Old version should be deprecated
    auto metadata_v1 = adapter.getKeyMetadata("test-key", 1);
    EXPECT_EQ(metadata_v1.status, KeyStatus::DEPRECATED);
    
    // New version should be active
    auto metadata_v2 = adapter.getKeyMetadata("test-key", 2);
    EXPECT_EQ(metadata_v2.status, KeyStatus::ACTIVE);
    
    // getKey() without version should return active (v2)
    auto key = adapter.getKey("test-key");
    EXPECT_EQ(key.size(), 32);
}

TEST_F(HSMKeyProviderAdapterTest, ListKeys) {
    if (!isHSMAvailable()) {
        GTEST_SKIP() << "HSM not available - skipping test";
    }
    
    auto hsm = createHSM();
    if (!hsm) {
        GTEST_SKIP() << "Failed to initialize HSM";
    }
    
    HSMKeyProviderAdapter adapter(hsm);
    
    // Create multiple keys
    std::vector<uint8_t> key_bytes(32, 0x42);
    adapter.createKeyFromBytes("key1", key_bytes);
    adapter.createKeyFromBytes("key2", key_bytes);
    adapter.createKeyFromBytes("key3", key_bytes);
    
    auto keys = adapter.listKeys();
    EXPECT_EQ(keys.size(), 3);
}

TEST_F(HSMKeyProviderAdapterTest, DeleteKey) {
    if (!isHSMAvailable()) {
        GTEST_SKIP() << "HSM not available - skipping test";
    }
    
    auto hsm = createHSM();
    if (!hsm) {
        GTEST_SKIP() << "Failed to initialize HSM";
    }
    
    HSMKeyProviderAdapter adapter(hsm);
    
    // Create and rotate key
    std::vector<uint8_t> key_bytes(32, 0x42);
    adapter.createKeyFromBytes("test-key", key_bytes);
    adapter.rotateKey("test-key");
    
    // Cannot delete ACTIVE key
    EXPECT_THROW({
        adapter.deleteKey("test-key", 2);
    }, KeyOperationException);
    
    // Can delete DEPRECATED key
    EXPECT_NO_THROW({
        adapter.deleteKey("test-key", 1);
    });
    
    // Key should be marked as DELETED
    auto metadata = adapter.getKeyMetadata("test-key", 1);
    EXPECT_EQ(metadata.status, KeyStatus::DELETED);
}

TEST_F(HSMKeyProviderAdapterTest, GetKeyNotFound) {
    if (!isHSMAvailable()) {
        GTEST_SKIP() << "HSM not available - skipping test";
    }
    
    auto hsm = createHSM();
    if (!hsm) {
        GTEST_SKIP() << "Failed to initialize HSM";
    }
    
    HSMKeyProviderAdapter adapter(hsm);
    
    EXPECT_THROW({
        adapter.getKey("nonexistent-key");
    }, KeyNotFoundException);
    
    EXPECT_THROW({
        adapter.getKey("nonexistent-key", 1);
    }, KeyNotFoundException);
}

TEST_F(HSMKeyProviderAdapterTest, ClearCache) {
    if (!isHSMAvailable()) {
        GTEST_SKIP() << "HSM not available - skipping test";
    }
    
    auto hsm = createHSM();
    if (!hsm) {
        GTEST_SKIP() << "Failed to initialize HSM";
    }
    
    HSMKeyProviderAdapter::Config config;
    config.enable_caching = true;
    
    HSMKeyProviderAdapter adapter(hsm, config);
    
    // Create and get key to populate cache
    std::vector<uint8_t> key_bytes(32, 0x42);
    adapter.createKeyFromBytes("test-key", key_bytes);
    adapter.getKey("test-key");
    
    auto stats_before = adapter.getStats();
    EXPECT_GT(stats_before["cache_size"].get<size_t>(), 0);
    
    // Clear cache
    adapter.clearCache();
    
    auto stats_after = adapter.getStats();
    EXPECT_EQ(stats_after["cache_size"].get<size_t>(), 0);
}

TEST_F(HSMKeyProviderAdapterTest, IsHSMReady) {
    if (!isHSMAvailable()) {
        GTEST_SKIP() << "HSM not available - skipping test";
    }
    
    auto hsm = createHSM();
    if (!hsm) {
        GTEST_SKIP() << "Failed to initialize HSM";
    }
    
    HSMKeyProviderAdapter adapter(hsm);
    
    EXPECT_TRUE(adapter.isHSMReady());
}

TEST_F(HSMKeyProviderAdapterTest, GetStats) {
    if (!isHSMAvailable()) {
        GTEST_SKIP() << "HSM not available - skipping test";
    }
    
    auto hsm = createHSM();
    if (!hsm) {
        GTEST_SKIP() << "Failed to initialize HSM";
    }
    
    HSMKeyProviderAdapter adapter(hsm);
    
    auto stats = adapter.getStats();
    
    // Should have these fields
    EXPECT_TRUE(stats.contains("cache_hits"));
    EXPECT_TRUE(stats.contains("cache_misses"));
    EXPECT_TRUE(stats.contains("cache_hit_rate"));
    EXPECT_TRUE(stats.contains("hsm_encrypt_operations"));
    EXPECT_TRUE(stats.contains("hsm_decrypt_operations"));
    EXPECT_TRUE(stats.contains("hsm_errors"));
    EXPECT_TRUE(stats.contains("key_rotations"));
    EXPECT_TRUE(stats.contains("cache_size"));
    EXPECT_TRUE(stats.contains("total_keys"));
    EXPECT_TRUE(stats.contains("total_key_versions"));
}

// Integration test with FieldEncryption would go here
// but requires more setup and is better as a separate integration test
