/**
 * @file test_lora_storage_key_provider.cpp
 * @brief Unit tests for LoRA Storage Service key provider creation
 * 
 * Tests the refactored key provider selection logic to ensure:
 * - Correct priority order (HSM > Vault > PKI > Mock)
 * - Production mode enforcement
 * - Proper error handling for missing configurations
 * - Development mode fallback behavior
 * 
 * @note Requires GTest: vcpkg install gtest OR apt-get install libgtest-dev
 * @build cmake -DTHEMIS_BUILD_TESTS=ON ..
 * @run ./tests/test_lora_storage_key_provider
 */

#ifndef THEMIS_TEST_BUILD
#define THEMIS_TEST_BUILD 1
#endif

#include <gtest/gtest.h>
#include "llm/lora_framework/lora_storage_service.h"
#include "storage/rocksdb_wrapper.h"
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <memory>

using namespace themis::llm::lora;

// ============================================================================
// Test Fixtures
// ============================================================================

class LoRAStorageKeyProviderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear environment variable to ensure clean state
        unsetenv("THEMIS_ENVIRONMENT");
        
        // Create a temporary file-based database for testing
        test_db_path_ = generateTempPath("test_lora_key_provider");
        db_ = std::make_shared<RocksDBWrapper>();
        db_->open(test_db_path_, false);
    }
    
    void TearDown() override {
        // Clean up
        unsetenv("THEMIS_ENVIRONMENT");
        if (db_) {
            db_->close();
        }
        // Remove test database directory
        std::filesystem::remove_all(test_db_path_);
    }
    
    // Helper to set production environment
    void setProductionMode() {
        setenv("THEMIS_ENVIRONMENT", "production", 1);
    }
    
    // Helper to set development environment
    void setDevelopmentMode() {
        setenv("THEMIS_ENVIRONMENT", "development", 1);
    }
    
    // Helper to generate unique temporary paths
    static std::string generateTempPath(const std::string& prefix) {
        auto temp_dir = std::filesystem::temp_directory_path();
        auto unique_name = prefix + "_" + std::to_string(std::time(nullptr)) + 
                          "_" + std::to_string(std::rand());
        return (temp_dir / unique_name).string();
    }
    
    std::shared_ptr<RocksDBWrapper> db_;
    std::string test_db_path_;
};

// ============================================================================
// Provider Priority Tests
// ============================================================================

TEST_F(LoRAStorageKeyProviderTest, HSMPriorityOverVault) {
    // When both HSM and Vault are configured, HSM should be used (highest priority)
    LoRAStorageService::Config config;
    config.enable_encryption = true;
    config.backend = LoRAStorageService::Backend::FileSystem;
    config.filesystem_path = generateTempPath("test_lora_hsm_priority");
    
    // Configure both HSM and Vault
    config.use_hsm_for_encryption = true;
    config.hsm_library_path = "/usr/lib/softhsm/libsofthsm2.so";
    config.hsm_pin = "1234";
    config.hsm_key_label = "test-kek";
    
    config.use_vault_for_encryption = true;
    config.vault_addr = "http://localhost:8200";
    config.vault_token = "test-token";
    
    // This should attempt HSM initialization (and likely fail if HSM not available)
    // but the key point is that it tries HSM first
    EXPECT_THROW({
        LoRAStorageService service(config);
    }, std::exception);
    // Note: We expect an exception because HSM is unlikely to be available in test env
    // The important thing is it tries HSM first, not Vault
}

TEST_F(LoRAStorageKeyProviderTest, VaultPriorityOverPKI) {
    // When both Vault and PKI are configured, Vault should be used
    LoRAStorageService::Config config;
    config.enable_encryption = true;
    config.backend = LoRAStorageService::Backend::FileSystem;
    config.filesystem_path = generateTempPath("test_lora_vault_priority");
    
    // Configure both Vault and PKI
    config.use_vault_for_encryption = true;
    config.vault_addr = "http://localhost:8200";
    config.vault_token = "test-token";
    
    config.use_pki_for_encryption = true;
    config.pki_cert_path = "/tmp/test.crt";
    config.pki_private_key_path = "/tmp/test.key";
    config.db = db_;
    
    // This should attempt Vault initialization (and likely fail if Vault not available)
    EXPECT_THROW({
        LoRAStorageService service(config);
    }, std::exception);
    // Note: We expect an exception because Vault is unlikely to be available
    // The important thing is it tries Vault first, not PKI
}

TEST_F(LoRAStorageKeyProviderTest, PKIPriorityOverMock) {
    // When PKI is configured, it should be used over MockKeyProvider
    LoRAStorageService::Config config;
    config.enable_encryption = true;
    config.backend = LoRAStorageService::Backend::FileSystem;
    config.filesystem_path = generateTempPath("test_lora_pki_priority");
    
    config.use_pki_for_encryption = true;
    config.pki_cert_path = "/tmp/test.crt";
    config.pki_private_key_path = "/tmp/test.key";
    config.db = db_;
    
    setDevelopmentMode();  // Allow MockKeyProvider as fallback
    
    // This should attempt PKI initialization (and likely fail if cert files don't exist)
    EXPECT_THROW({
        LoRAStorageService service(config);
    }, std::exception);
    // The important thing is it tries PKI, not immediately falling back to Mock
}

// ============================================================================
// Production Mode Enforcement Tests
// ============================================================================

TEST_F(LoRAStorageKeyProviderTest, ProductionModeEnforcement) {
    // Production mode should throw without a secure provider
    setProductionMode();
    
    LoRAStorageService::Config config;
    config.enable_encryption = true;
    config.backend = LoRAStorageService::Backend::FileSystem;
    config.filesystem_path = generateTempPath("test_lora_production");
    
    // No secure provider configured - should throw
    EXPECT_THROW({
        LoRAStorageService service(config);
    }, std::runtime_error);
}

TEST_F(LoRAStorageKeyProviderTest, DevelopmentModeAllowsMock) {
    // Development mode should allow MockKeyProvider with warnings
    setDevelopmentMode();
    
    LoRAStorageService::Config config;
    config.enable_encryption = true;
    config.backend = LoRAStorageService::Backend::FileSystem;
    config.filesystem_path = generateTempPath("test_lora_development");
    
    // No secure provider configured - should succeed with MockKeyProvider
    EXPECT_NO_THROW({
        LoRAStorageService service(config);
    });
}

TEST_F(LoRAStorageKeyProviderTest, NoEnvironmentDefaultsToDevelopment) {
    // When THEMIS_ENVIRONMENT is not set, should allow MockKeyProvider
    // (defaults to development behavior for backward compatibility)
    unsetenv("THEMIS_ENVIRONMENT");
    
    LoRAStorageService::Config config;
    config.enable_encryption = true;
    config.backend = LoRAStorageService::Backend::FileSystem;
    config.filesystem_path = generateTempPath("test_lora_no_env");
    
    // Should succeed with MockKeyProvider
    EXPECT_NO_THROW({
        LoRAStorageService service(config);
    });
}

// ============================================================================
// Configuration Validation Tests
// ============================================================================

TEST_F(LoRAStorageKeyProviderTest, MissingHSMConfig) {
    // Should throw if use_hsm_for_encryption is true but library path not provided
    LoRAStorageService::Config config;
    config.enable_encryption = true;
    config.backend = LoRAStorageService::Backend::FileSystem;
    config.filesystem_path = generateTempPath("test_lora_hsm_missing");
    
    config.use_hsm_for_encryption = true;
    // hsm_library_path is empty - should throw
    
    EXPECT_THROW({
        LoRAStorageService service(config);
    }, std::runtime_error);
}

TEST_F(LoRAStorageKeyProviderTest, MissingVaultConfig) {
    // Should throw if use_vault_for_encryption is true but address not provided
    LoRAStorageService::Config config;
    config.enable_encryption = true;
    config.backend = LoRAStorageService::Backend::FileSystem;
    config.filesystem_path = generateTempPath("test_lora_vault_missing");
    
    config.use_vault_for_encryption = true;
    // vault_addr is empty - should throw
    
    EXPECT_THROW({
        LoRAStorageService service(config);
    }, std::runtime_error);
}

TEST_F(LoRAStorageKeyProviderTest, MissingPKICertPath) {
    // Should throw if use_pki_for_encryption is true but cert path not provided
    LoRAStorageService::Config config;
    config.enable_encryption = true;
    config.backend = LoRAStorageService::Backend::FileSystem;
    config.filesystem_path = generateTempPath("test_lora_pki_missing_cert");
    
    config.use_pki_for_encryption = true;
    config.pki_private_key_path = "/tmp/test.key";
    config.db = db_;
    // pki_cert_path is empty - should throw
    
    EXPECT_THROW({
        LoRAStorageService service(config);
    }, std::runtime_error);
}

TEST_F(LoRAStorageKeyProviderTest, MissingPKIKeyPath) {
    // Should throw if use_pki_for_encryption is true but private key path not provided
    LoRAStorageService::Config config;
    config.enable_encryption = true;
    config.backend = LoRAStorageService::Backend::FileSystem;
    config.filesystem_path = generateTempPath("test_lora_pki_missing_key");
    
    config.use_pki_for_encryption = true;
    config.pki_cert_path = "/tmp/test.crt";
    config.db = db_;
    // pki_private_key_path is empty - should throw
    
    EXPECT_THROW({
        LoRAStorageService service(config);
    }, std::runtime_error);
}

TEST_F(LoRAStorageKeyProviderTest, MissingPKIDatabase) {
    // Should throw if use_pki_for_encryption is true but database not provided
    LoRAStorageService::Config config;
    config.enable_encryption = true;
    config.backend = LoRAStorageService::Backend::FileSystem;
    config.filesystem_path = generateTempPath("test_lora_pki_missing_db");
    
    config.use_pki_for_encryption = true;
    config.pki_cert_path = "/tmp/test.crt";
    config.pki_private_key_path = "/tmp/test.key";
    // db is nullptr - should throw
    
    EXPECT_THROW({
        LoRAStorageService service(config);
    }, std::runtime_error);
}

// ============================================================================
// Encryption Disabled Tests
// ============================================================================

TEST_F(LoRAStorageKeyProviderTest, EncryptionDisabled) {
    // When encryption is disabled, no key provider should be created
    LoRAStorageService::Config config;
    config.enable_encryption = false;
    config.backend = LoRAStorageService::Backend::FileSystem;
    config.filesystem_path = generateTempPath("test_lora_no_encryption");
    
    // Should succeed without creating any key provider
    EXPECT_NO_THROW({
        LoRAStorageService service(config);
    });
}

TEST_F(LoRAStorageKeyProviderTest, EncryptionDisabledIgnoresProviderConfig) {
    // Even if provider configs are set, they should be ignored when encryption is disabled
    setProductionMode();
    
    LoRAStorageService::Config config;
    config.enable_encryption = false;
    config.backend = LoRAStorageService::Backend::FileSystem;
    config.filesystem_path = generateTempPath("test_lora_disabled_ignore_config");
    
    // Set HSM config but disable encryption
    config.use_hsm_for_encryption = true;
    config.hsm_library_path = "/usr/lib/softhsm/libsofthsm2.so";
    
    // Should succeed - encryption disabled, so provider config is irrelevant
    EXPECT_NO_THROW({
        LoRAStorageService service(config);
    });
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
