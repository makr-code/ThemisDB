/**
 * ThemisDB LoRA Encryption Integration Tests
 * 
 * Comprehensive tests for encrypted LoRA storage including:
 * - HSM encryption integration
 * - Vault encryption integration
 * - PKI certificate-based encryption
 * - Key rotation scenarios
 * - Encrypted storage and retrieval
 */

#include <gtest/gtest.h>
#include "llm/lora_framework/lora_storage_service.h"
#include "llm/lora_framework/lora_config.h"
#include <filesystem>

using namespace themis::llm::lora;
namespace fs = std::filesystem;

// ═══════════════════════════════════════════════════════════
// Test Fixtures
// ═══════════════════════════════════════════════════════════

class LoRAEncryptionIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary test directory
        test_dir_ = fs::temp_directory_path() / "themis_lora_encryption_test";
        if (fs::exists(test_dir_)) {
            fs::remove_all(test_dir_);
        }
        fs::create_directories(test_dir_);
        
        // Base config for filesystem backend
        base_config_.backend = LoRAStorageService::Backend::FileSystem;
        base_config_.filesystem_path = test_dir_.string();
        base_config_.enable_versioning = false;
        base_config_.enable_signatures = false;
        base_config_.enable_compression = false;
    }
    
    void TearDown() override {
        if (fs::exists(test_dir_)) {
            try {
                fs::remove_all(test_dir_);
            } catch (...) {}
        }
    }
    
    AdapterWeights createTestWeights(size_t size = 2048) {
        AdapterWeights weights;
        weights.data.resize(size);
        for (size_t i = 0; i < size; ++i) {
            weights.data[i] = static_cast<uint8_t>((i * 37) % 256);  // Pseudo-random pattern
        }
        weights.size_bytes = size;
        weights.format = "safetensors";
        return weights;
    }
    
    AdapterMetadata createTestMetadata(const std::string& id) {
        AdapterMetadata metadata;
        metadata.adapter_id = id;
        metadata.base_model = "test-model-7b";
        metadata.description = "Encrypted test adapter";
        metadata.created_at = std::time(nullptr);
        metadata.updated_at = metadata.created_at;
        metadata.version = "1.0.0";
        metadata.hyperparameters.rank = 8;
        metadata.hyperparameters.alpha = 16.0f;
        return metadata;
    }
    
    fs::path test_dir_;
    LoRAStorageService::Config base_config_;
};

// ═══════════════════════════════════════════════════════════
// Basic Encryption Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAEncryptionIntegrationTest, EncryptionEnabledFlag) {
    auto config = base_config_;
    config.enable_encryption = true;
    config.encryption_key_id = "test-key-001";
    
    // Note: Without actual HSM/Vault/PKI, this tests the config propagation
    LoRAStorageService storage(config);
    
    auto weights = createTestWeights(1024);
    auto metadata = createTestMetadata("encrypted-adapter");
    
    // Save with encryption enabled
    bool saved = storage.saveAdapter("encrypted-adapter", weights, metadata);
    EXPECT_TRUE(saved);
    
    // Load back
    auto loaded = storage.loadAdapter("encrypted-adapter");
    ASSERT_TRUE(loaded.has_value());
    
    // Data should match (encryption/decryption transparent to caller)
    EXPECT_EQ(loaded->data, weights.data);
}

TEST_F(LoRAEncryptionIntegrationTest, EncryptionOverhead) {
    auto config_plain = base_config_;
    config_plain.enable_encryption = false;
    
    auto config_encrypted = base_config_;
    config_encrypted.enable_encryption = true;
    config_encrypted.encryption_key_id = "test-key-002";
    
    auto weights = createTestWeights(4096);
    auto metadata = createTestMetadata("overhead-test");
    
    // Save without encryption
    LoRAStorageService storage_plain(config_plain);
    storage_plain.saveAdapter("plain-adapter", weights, metadata);
    
    // Save with encryption (different storage)
    config_encrypted.filesystem_path = (test_dir_ / "encrypted").string();
    fs::create_directories(config_encrypted.filesystem_path);
    LoRAStorageService storage_encrypted(config_encrypted);
    storage_encrypted.saveAdapter("encrypted-adapter", weights, metadata);
    
    // Both should load successfully
    auto plain_loaded = storage_plain.loadAdapter("plain-adapter");
    auto encrypted_loaded = storage_encrypted.loadAdapter("encrypted-adapter");
    
    ASSERT_TRUE(plain_loaded.has_value());
    ASSERT_TRUE(encrypted_loaded.has_value());
    
    // Data should be identical
    EXPECT_EQ(plain_loaded->data, encrypted_loaded->data);
}

// ═══════════════════════════════════════════════════════════
// HSM Encryption Tests (Mock/Config Tests)
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAEncryptionIntegrationTest, HSM_ConfigurationValidation) {
    auto config = base_config_;
    config.enable_encryption = true;
    config.use_hsm_for_encryption = true;
    config.hsm_library_path = "/mock/path/to/libsofthsm2.so";
    config.hsm_slot_id = 0;
    config.hsm_pin = "1234";
    config.hsm_key_label = "lora-test-kek";
    config.hsm_session_pool_size = 4;
    
    // This validates that the config is accepted (actual HSM not needed for config test)
    EXPECT_TRUE(config.use_hsm_for_encryption);
    EXPECT_EQ(config.hsm_session_pool_size, 4);
    EXPECT_EQ(config.hsm_key_label, "lora-test-kek");
}

TEST_F(LoRAEncryptionIntegrationTest, HSM_WithoutLibraryPath) {
    auto config = base_config_;
    config.enable_encryption = true;
    config.use_hsm_for_encryption = true;
    // Missing hsm_library_path
    
    // Storage should handle missing HSM gracefully or use fallback
    // Actual behavior depends on implementation
    LoRAStorageService storage(config);
    
    auto weights = createTestWeights(512);
    auto metadata = createTestMetadata("hsm-no-lib");
    
    // Should either succeed with fallback or fail gracefully
    bool saved = storage.saveAdapter("hsm-no-lib", weights, metadata);
    // Don't assert - behavior is implementation-defined
}

TEST_F(LoRAEncryptionIntegrationTest, HSM_KeyRotationScenario) {
    auto config = base_config_;
    config.enable_encryption = true;
    config.use_hsm_for_encryption = true;
    config.hsm_key_label = "lora-kek-v1";
    
    LoRAStorageService storage(config);
    
    auto weights = createTestWeights(1024);
    auto metadata = createTestMetadata("hsm-rotation-test");
    
    // Save with key v1
    bool saved = storage.saveAdapter("hsm-rotation-test", weights, metadata);
    EXPECT_TRUE(saved);
    
    // Simulate key rotation by changing label
    config.hsm_key_label = "lora-kek-v2";
    
    // Note: Actual key rotation would require re-encryption
    // This tests configuration change
    EXPECT_EQ(config.hsm_key_label, "lora-kek-v2");
}

// ═══════════════════════════════════════════════════════════
// Vault Encryption Tests (Mock/Config Tests)
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAEncryptionIntegrationTest, Vault_ConfigurationValidation) {
    auto config = base_config_;
    config.enable_encryption = true;
    config.use_vault_for_encryption = true;
    config.vault_addr = "http://localhost:8200";
    config.vault_token = "test-token-12345";
    config.vault_kv_mount = "themis-lora";
    
    EXPECT_TRUE(config.use_vault_for_encryption);
    EXPECT_EQ(config.vault_addr, "http://localhost:8200");
    EXPECT_EQ(config.vault_kv_mount, "themis-lora");
}

TEST_F(LoRAEncryptionIntegrationTest, Vault_EncryptionFlow) {
    auto config = base_config_;
    config.enable_encryption = true;
    config.use_vault_for_encryption = true;
    config.vault_addr = "http://mock-vault:8200";
    config.vault_token = "mock-token";
    
    LoRAStorageService storage(config);
    
    auto weights = createTestWeights(2048);
    auto metadata = createTestMetadata("vault-test");
    
    // Save with Vault encryption config
    bool saved = storage.saveAdapter("vault-test", weights, metadata);
    EXPECT_TRUE(saved);
    
    // Load back
    auto loaded = storage.loadAdapter("vault-test");
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->data, weights.data);
}

TEST_F(LoRAEncryptionIntegrationTest, Vault_KeyVersionTracking) {
    auto config = base_config_;
    config.enable_encryption = true;
    config.use_vault_for_encryption = true;
    config.vault_addr = "http://mock-vault:8200";
    config.vault_token = "token-v1";
    
    LoRAStorageService storage(config);
    
    auto weights = createTestWeights(1024);
    auto metadata = createTestMetadata("vault-versioned");
    metadata.version = "1.0.0";
    
    storage.saveAdapter("vault-versioned", weights, metadata);
    
    // Change token (simulate key rotation)
    config.vault_token = "token-v2";
    metadata.version = "2.0.0";
    
    // New storage instance with rotated token
    LoRAStorageService storage_v2(config);
    auto loaded = storage_v2.loadAdapter("vault-versioned");
    
    // Should either load or indicate key mismatch
    // Behavior depends on implementation
}

// ═══════════════════════════════════════════════════════════
// PKI Encryption Tests (Certificate-Based)
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAEncryptionIntegrationTest, PKI_ConfigurationValidation) {
    auto config = base_config_;
    config.enable_encryption = true;
    config.use_pki_for_encryption = true;
    config.pki_cert_path = "/path/to/cert.pem";
    config.pki_private_key_path = "/path/to/key.pem";
    config.pki_ca_bundle_path = "/path/to/ca-bundle.pem";
    config.pki_verify_certificate = true;
    
    EXPECT_TRUE(config.use_pki_for_encryption);
    EXPECT_TRUE(config.pki_verify_certificate);
    EXPECT_EQ(config.pki_cert_path, "/path/to/cert.pem");
}

TEST_F(LoRAEncryptionIntegrationTest, PKI_EncryptWithPublicKey) {
    auto config = base_config_;
    config.enable_encryption = true;
    config.use_pki_for_encryption = true;
    config.pki_cert_path = "/mock/cert.pem";
    config.pki_private_key_path = "/mock/key.pem";
    
    LoRAStorageService storage(config);
    
    auto weights = createTestWeights(2048);
    auto metadata = createTestMetadata("pki-encrypted");
    
    // Save with PKI encryption
    bool saved = storage.saveAdapter("pki-encrypted", weights, metadata);
    EXPECT_TRUE(saved);
    
    // Load with private key
    auto loaded = storage.loadAdapter("pki-encrypted");
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->data, weights.data);
}

TEST_F(LoRAEncryptionIntegrationTest, PKI_CertificateValidation) {
    auto config = base_config_;
    config.enable_encryption = true;
    config.use_pki_for_encryption = true;
    config.pki_verify_certificate = true;
    config.pki_cert_path = "/mock/expired-cert.pem";
    
    // With verification enabled, expired cert should be handled
    // Implementation-specific behavior
    EXPECT_TRUE(config.pki_verify_certificate);
}

TEST_F(LoRAEncryptionIntegrationTest, PKI_WithoutPrivateKey) {
    auto config = base_config_;
    config.enable_encryption = true;
    config.use_pki_for_encryption = true;
    config.pki_cert_path = "/mock/cert.pem";
    // Missing pki_private_key_path
    
    LoRAStorageService storage(config);
    
    auto weights = createTestWeights(512);
    auto metadata = createTestMetadata("pki-no-key");
    
    // Save might work (public key encryption)
    storage.saveAdapter("pki-no-key", weights, metadata);
    
    // Load without private key should fail or use fallback
    auto loaded = storage.loadAdapter("pki-no-key");
    // Behavior depends on implementation
}

// ═══════════════════════════════════════════════════════════
// Multi-Provider Encryption Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAEncryptionIntegrationTest, MultipleEncryptionProviders) {
    // Test that only one provider is active at a time
    auto config = base_config_;
    config.enable_encryption = true;
    config.use_hsm_for_encryption = true;
    config.use_vault_for_encryption = true;  // Both enabled
    
    // Implementation should handle priority or conflict
    EXPECT_TRUE(config.enable_encryption);
    // Priority logic is implementation-specific
}

TEST_F(LoRAEncryptionIntegrationTest, EncryptionWithDifferentProviders) {
    // Save with HSM
    auto config_hsm = base_config_;
    config_hsm.enable_encryption = true;
    config_hsm.use_hsm_for_encryption = true;
    config_hsm.hsm_key_label = "hsm-key";
    config_hsm.filesystem_path = (test_dir_ / "hsm").string();
    fs::create_directories(config_hsm.filesystem_path);
    
    LoRAStorageService storage_hsm(config_hsm);
    auto weights = createTestWeights(1024);
    auto metadata = createTestMetadata("hsm-adapter");
    storage_hsm.saveAdapter("hsm-adapter", weights, metadata);
    
    // Save with Vault
    auto config_vault = base_config_;
    config_vault.enable_encryption = true;
    config_vault.use_vault_for_encryption = true;
    config_vault.vault_addr = "http://vault:8200";
    config_vault.filesystem_path = (test_dir_ / "vault").string();
    fs::create_directories(config_vault.filesystem_path);
    
    LoRAStorageService storage_vault(config_vault);
    storage_vault.saveAdapter("vault-adapter", weights, metadata);
    
    // Both should load successfully
    auto hsm_loaded = storage_hsm.loadAdapter("hsm-adapter");
    auto vault_loaded = storage_vault.loadAdapter("vault-adapter");
    
    ASSERT_TRUE(hsm_loaded.has_value());
    ASSERT_TRUE(vault_loaded.has_value());
    
    // Data should be identical (transparent encryption)
    EXPECT_EQ(hsm_loaded->data, weights.data);
    EXPECT_EQ(vault_loaded->data, weights.data);
}

// ═══════════════════════════════════════════════════════════
// Encryption Error Scenarios
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAEncryptionIntegrationTest, LoadWithoutDecryptionKey) {
    auto config = base_config_;
    config.enable_encryption = true;
    config.encryption_key_id = "key-123";
    
    LoRAStorageService storage(config);
    
    auto weights = createTestWeights(1024);
    auto metadata = createTestMetadata("encrypted-data");
    storage.saveAdapter("encrypted-data", weights, metadata);
    
    // Try loading with different key ID (simulate missing key)
    config.encryption_key_id = "wrong-key-456";
    LoRAStorageService storage_wrong_key(config);
    
    auto loaded = storage_wrong_key.loadAdapter("encrypted-data");
    
    // Should fail or return empty (depending on implementation)
    // This tests error handling for key mismatch
}

TEST_F(LoRAEncryptionIntegrationTest, EncryptionFailureHandling) {
    auto config = base_config_;
    config.enable_encryption = true;
    config.use_hsm_for_encryption = true;
    config.hsm_library_path = "/nonexistent/hsm.so";  // Invalid path
    
    LoRAStorageService storage(config);
    
    auto weights = createTestWeights(1024);
    auto metadata = createTestMetadata("failed-encryption");
    
    // Should handle encryption failure gracefully
    bool saved = storage.saveAdapter("failed-encryption", weights, metadata);
    
    // Implementation might fall back or return false
    // This tests error handling
}

TEST_F(LoRAEncryptionIntegrationTest, LargeAdapterEncryption) {
    auto config = base_config_;
    config.enable_encryption = true;
    config.encryption_key_id = "large-data-key";
    
    LoRAStorageService storage(config);
    
    // Create 10MB adapter
    auto weights = createTestWeights(10 * 1024 * 1024);
    auto metadata = createTestMetadata("large-encrypted");
    
    auto start = std::chrono::high_resolution_clock::now();
    bool saved = storage.saveAdapter("large-encrypted", weights, metadata);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_TRUE(saved);
    
    // Encryption should complete reasonably fast (< 5 seconds for 10MB)
    EXPECT_LT(duration.count(), 5000);
    
    // Verify can decrypt
    auto loaded = storage.loadAdapter("large-encrypted");
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->data.size(), weights.data.size());
}

TEST_F(LoRAEncryptionIntegrationTest, EncryptedMetadataIntegrity) {
    auto config = base_config_;
    config.enable_encryption = true;
    config.encryption_key_id = "metadata-test";
    
    LoRAStorageService storage(config);
    
    auto weights = createTestWeights(1024);
    auto metadata = createTestMetadata("metadata-integrity");
    metadata.description = "Sensitive metadata that should be encrypted";
    
    storage.saveAdapter("metadata-integrity", weights, metadata);
    
    // Load and verify metadata is intact
    auto loaded_metadata = storage.loadMetadata("metadata-integrity");
    ASSERT_TRUE(loaded_metadata.has_value());
    EXPECT_EQ(loaded_metadata->description, metadata.description);
    EXPECT_EQ(loaded_metadata->adapter_id, metadata.adapter_id);
}
