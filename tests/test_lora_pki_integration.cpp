#include <gtest/gtest.h>
#include "llm/lora_framework/lora_storage_service.h"
#include "security/pki_key_provider.h"
#include "storage/rocksdb_wrapper.h"
#include <fstream>
#include <cstdio>
#include <filesystem>

using namespace themis::llm::lora;
using namespace themis::security;
namespace fs = std::filesystem;

class LoRAPKIIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directory for test files
        test_dir_ = "/tmp/lora_pki_test_" + std::to_string(std::time(nullptr));
        fs::create_directories(test_dir_);
        
        cert_path_ = test_dir_ + "/test_cert.pem";
        key_path_ = test_dir_ + "/test_key.pem";
        db_path_ = test_dir_ + "/test_db";
        
        // Generate self-signed test certificate
        generateTestCertificate();
        
        // Initialize RocksDB for DEK storage
        db_ = std::make_shared<themis::RocksDBWrapper>(db_path_);
    }
    
    void TearDown() override {
        // Clean up test files
        db_.reset();  // Close DB first
        if (fs::exists(test_dir_)) {
            fs::remove_all(test_dir_);
        }
    }
    
    void generateTestCertificate() {
        // Generate private key
        std::string cmd = "openssl genrsa -out " + key_path_ + " 2048 2>/dev/null";
        int result = system(cmd.c_str());
        ASSERT_EQ(result, 0) << "Failed to generate private key";
        
        // Generate self-signed certificate (valid for 1 day)
        cmd = "openssl req -new -x509 -key " + key_path_ + 
              " -out " + cert_path_ + " -days 1 " +
              "-subj '/CN=ThemisDB-LoRA-Test/O=ThemisDB/C=DE' 2>/dev/null";
        result = system(cmd.c_str());
        ASSERT_EQ(result, 0) << "Failed to generate certificate";
        
        // Verify files exist
        ASSERT_TRUE(fs::exists(cert_path_)) << "Certificate file not created";
        ASSERT_TRUE(fs::exists(key_path_)) << "Key file not created";
    }
    
    std::string test_dir_;
    std::string cert_path_;
    std::string key_path_;
    std::string db_path_;
    std::shared_ptr<themis::RocksDBWrapper> db_;
};

TEST_F(LoRAPKIIntegrationTest, PKIKeyProviderInitialization) {
    // Test that PKIKeyProvider can be initialized with certificate files
    ASSERT_NO_THROW({
        auto provider = std::make_shared<PKIKeyProvider>(
            cert_path_,
            key_path_,
            db_,
            "test_service",
            true  // validate certificate
        );
        EXPECT_TRUE(provider != nullptr);
    });
}

TEST_F(LoRAPKIIntegrationTest, PKIKeyProviderGetKey) {
    auto provider = std::make_shared<PKIKeyProvider>(
        cert_path_,
        key_path_,
        db_,
        "test_service",
        true
    );
    
    // Get encryption key
    auto key = provider->getKey("test_key");
    
    // Verify key properties
    EXPECT_EQ(key.size(), 32) << "Expected 256-bit (32-byte) key";
    EXPECT_FALSE(key.empty());
    
    // Verify key consistency - same key_id should return same key
    auto key2 = provider->getKey("test_key");
    EXPECT_EQ(key, key2) << "Same key_id should return same key";
}

TEST_F(LoRAPKIIntegrationTest, LoRAStorageServiceWithPKI) {
    // Configure LoRA storage service with PKI encryption
    LoRAStorageService::Config config;
    config.backend = LoRAStorageService::Backend::ThemisDB;
    config.db = db_;
    config.enable_encryption = true;
    config.use_pki_for_encryption = true;
    config.pki_cert_path = cert_path_;
    config.pki_private_key_path = key_path_;
    config.pki_verify_certificate = true;
    
    // Initialize storage service
    ASSERT_NO_THROW({
        LoRAStorageService storage(config);
    }) << "LoRAStorageService should initialize with PKI encryption";
}

TEST_F(LoRAPKIIntegrationTest, EncryptDecryptAdapter) {
    // Configure storage service
    LoRAStorageService::Config config;
    config.backend = LoRAStorageService::Backend::ThemisDB;
    config.db = db_;
    config.enable_encryption = true;
    config.use_pki_for_encryption = true;
    config.pki_cert_path = cert_path_;
    config.pki_private_key_path = key_path_;
    config.pki_verify_certificate = true;
    
    LoRAStorageService storage(config);
    
    // Create test adapter
    AdapterWeights weights;
    weights.data = {1, 2, 3, 4, 5, 6, 7, 8};
    weights.size_bytes = weights.data.size();
    weights.format = "test";
    
    AdapterMetadata metadata;
    metadata.adapter_id = "test_adapter";
    metadata.base_model = "test_model";
    metadata.rank = 8;
    
    // Save adapter (will be encrypted)
    bool saved = storage.saveAdapter("test_adapter", weights, metadata);
    EXPECT_TRUE(saved) << "Failed to save encrypted adapter";
    
    // Load adapter (will be decrypted)
    auto loaded = storage.loadAdapter("test_adapter");
    ASSERT_TRUE(loaded.has_value()) << "Failed to load adapter";
    
    // Verify data integrity
    EXPECT_EQ(loaded->data, weights.data) << "Decrypted data doesn't match original";
    EXPECT_EQ(loaded->size_bytes, weights.size_bytes);
    EXPECT_EQ(loaded->format, weights.format);
}

TEST_F(LoRAPKIIntegrationTest, InvalidCertificatePath) {
    // Test with non-existent certificate
    EXPECT_THROW({
        auto provider = std::make_shared<PKIKeyProvider>(
            "/nonexistent/cert.pem",
            key_path_,
            db_,
            "test_service",
            true
        );
    }, std::runtime_error) << "Should throw on invalid certificate path";
}

TEST_F(LoRAPKIIntegrationTest, MissingPKIConfiguration) {
    // Test that missing PKI configuration throws error
    LoRAStorageService::Config config;
    config.backend = LoRAStorageService::Backend::ThemisDB;
    config.db = db_;
    config.enable_encryption = true;
    config.use_pki_for_encryption = true;
    // Don't set cert_path and key_path
    
    EXPECT_THROW({
        LoRAStorageService storage(config);
    }, std::runtime_error) << "Should throw when PKI paths not configured";
}

TEST_F(LoRAPKIIntegrationTest, KeyRotationSupport) {
    auto provider = std::make_shared<PKIKeyProvider>(
        cert_path_,
        key_path_,
        db_,
        "test_service",
        true
    );
    
    // Get initial key version
    auto key_v1 = provider->getKey("test_key");
    auto metadata_v1 = provider->getKeyMetadata("test_key");
    uint32_t version_v1 = metadata_v1.version;
    
    // Rotate key
    uint32_t new_version = provider->rotateKey("test_key");
    EXPECT_GT(new_version, version_v1) << "New version should be greater";
    
    // Get new key
    auto key_v2 = provider->getKey("test_key");
    
    // Keys should be different after rotation
    EXPECT_NE(key_v1, key_v2) << "Key should change after rotation";
    
    // Old version should still be accessible
    auto key_v1_retrieved = provider->getKey("test_key", version_v1);
    EXPECT_EQ(key_v1, key_v1_retrieved) << "Old key version should still be accessible";
}

TEST_F(LoRAPKIIntegrationTest, CertificateValidation) {
    // Test that validation works with valid certificate
    ASSERT_NO_THROW({
        auto provider = std::make_shared<PKIKeyProvider>(
            cert_path_,
            key_path_,
            db_,
            "test_service",
            true  // Enable validation
        );
    }) << "Valid certificate should pass validation";
    
    // Test that validation can be disabled
    ASSERT_NO_THROW({
        auto provider = std::make_shared<PKIKeyProvider>(
            cert_path_,
            key_path_,
            db_,
            "test_service",
            false  // Disable validation
        );
    }) << "Should work with validation disabled";
}

// Test with expired certificate would require generating an expired cert,
// which is complex in a test environment. Skipping for now.
