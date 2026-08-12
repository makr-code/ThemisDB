#include <gtest/gtest.h>
#include "security/hsm_key_provider_adapter.h"
#include "security/hsm_provider.h"
#include <filesystem>
#include <cstdlib>

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

// ============================================================================
// Stub-mode tests: do NOT require SoftHSM2, validate wrap/unwrap round-trip
// ============================================================================

class HSMKeyProviderAdapterStubTest : public ::testing::Test {
protected:
    std::shared_ptr<HSMProvider> createStubHSM() {
        HSMConfig cfg;
        cfg.library_path = ""; // Force stub mode (no real PKCS#11 library)
        auto hsm = std::make_shared<HSMProvider>(cfg);
        if (!hsm->initialize()) {
            return nullptr; // production-mode env may block stub
        }
        return hsm;
    }
};

struct HsmEnvGuard {
    std::string name;
    std::string previous;
    bool had_previous{false};

    HsmEnvGuard(std::string var_name, std::string value) : name(std::move(var_name)) {
        const char* existing = std::getenv(name.c_str());
        had_previous = (existing != nullptr);
        if (had_previous) {
            previous = existing;
        }
#ifdef _WIN32
        _putenv_s(name.c_str(), value.c_str());
#else
        ::setenv(name.c_str(), value.c_str(), 1);
#endif
    }

    ~HsmEnvGuard() {
#ifdef _WIN32
        if (had_previous) {
            _putenv_s(name.c_str(), previous.c_str());
        } else {
            _putenv_s(name.c_str(), "");
        }
#else
        if (had_previous) {
            ::setenv(name.c_str(), previous.c_str(), 1);
        } else {
            ::unsetenv(name.c_str());
        }
#endif
    }
};

struct HsmEnvUnsetGuard {
    std::string name;
    std::string previous;
    bool had_previous{false};

    explicit HsmEnvUnsetGuard(std::string var_name) : name(std::move(var_name)) {
        const char* existing = std::getenv(name.c_str());
        had_previous = (existing != nullptr);
        if (had_previous) {
            previous = existing;
        }
#ifdef _WIN32
        _putenv_s(name.c_str(), "");
#else
        ::unsetenv(name.c_str());
#endif
    }

    ~HsmEnvUnsetGuard() {
#ifdef _WIN32
        if (had_previous) {
            _putenv_s(name.c_str(), previous.c_str());
        } else {
            _putenv_s(name.c_str(), "");
        }
#else
        if (had_previous) {
            ::setenv(name.c_str(), previous.c_str(), 1);
        } else {
            ::unsetenv(name.c_str());
        }
#endif
    }
};

TEST_F(HSMKeyProviderAdapterStubTest, StubWrapBlockedByDefault) {
    HsmEnvUnsetGuard allow_stub("THEMIS_ALLOW_HSM_STUB");

    auto hsm = createStubHSM();
    if (!hsm) {
        GTEST_SKIP() << "Stub HSM could not initialize (production mode env?)";
    }
    ASSERT_TRUE(hsm->isStubProvider());

    HSMKeyProviderAdapter adapter(hsm);
    std::vector<uint8_t> original_key(32, 0x11);
    KeyMetadata meta;
    meta.status = KeyStatus::ACTIVE;

    EXPECT_THROW(
        adapter.createKeyFromBytes("blocked-wrap-key", original_key, meta),
        KeyOperationException
    );
}

TEST_F(HSMKeyProviderAdapterStubTest, WrapUnwrapRoundTrip) {
    HsmEnvGuard allow_stub("THEMIS_ALLOW_HSM_STUB", "1");

    auto hsm = createStubHSM();
    if (!hsm) {
        GTEST_SKIP() << "Stub HSM could not initialize (production mode env?)";
    }
    ASSERT_TRUE(hsm->isStubProvider());

    HSMKeyProviderAdapter adapter(hsm);

    std::vector<uint8_t> original_key(32);
    for (uint8_t i = 0; i < 32; ++i) original_key[i] = i;

    KeyMetadata meta;
    meta.status = KeyStatus::ACTIVE;
    meta.algorithm = "AES-256-GCM";
    adapter.createKeyFromBytes("round-trip-key", original_key, meta);

    auto retrieved = adapter.getKey("round-trip-key");
    ASSERT_EQ(retrieved.size(), 32u);
    EXPECT_EQ(retrieved, original_key) << "DEK round-trip failed: unwrapped key does not match original";
}

TEST_F(HSMKeyProviderAdapterStubTest, MultipleKeysIndependentWrap) {
    HsmEnvGuard allow_stub("THEMIS_ALLOW_HSM_STUB", "1");

    auto hsm = createStubHSM();
    if (!hsm) {
        GTEST_SKIP() << "Stub HSM could not initialize (production mode env?)";
    }

    HSMKeyProviderAdapter adapter(hsm);

    std::vector<uint8_t> key_a(32, 0xAA);
    std::vector<uint8_t> key_b(32, 0xBB);

    KeyMetadata meta;
    meta.status = KeyStatus::ACTIVE;

    adapter.createKeyFromBytes("key-a", key_a, meta);
    adapter.createKeyFromBytes("key-b", key_b, meta);

    EXPECT_EQ(adapter.getKey("key-a"), key_a);
    EXPECT_EQ(adapter.getKey("key-b"), key_b);
    EXPECT_NE(adapter.getKey("key-a"), adapter.getKey("key-b"));
}

TEST_F(HSMKeyProviderAdapterStubTest, RotatedKeyRoundTrip) {
    HsmEnvGuard allow_stub("THEMIS_ALLOW_HSM_STUB", "1");

    auto hsm = createStubHSM();
    if (!hsm) {
        GTEST_SKIP() << "Stub HSM could not initialize (production mode env?)";
    }

    HSMKeyProviderAdapter adapter(hsm);

    std::vector<uint8_t> initial_key(32, 0x11);
    KeyMetadata meta;
    meta.status = KeyStatus::ACTIVE;
    adapter.createKeyFromBytes("rot-key", initial_key, meta);

    // Rotate key
    uint32_t new_version = adapter.rotateKey("rot-key");
    EXPECT_EQ(new_version, 2u);

    // Both versions should be retrievable with correct content
    auto v1 = adapter.getKey("rot-key", 1);
    ASSERT_EQ(v1.size(), 32u);
    EXPECT_EQ(v1, initial_key);

    // New (rotated) version is a fresh random key, should be 32 bytes
    auto v2 = adapter.getKey("rot-key", 2);
    ASSERT_EQ(v2.size(), 32u);
    // Rotated key is random; just verify it is distinct from v1
    EXPECT_NE(v2, v1);
}
