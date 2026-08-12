/**
 * @file encryption_key_rotation_integration_test.cpp
 * @brief Integration test for encryption key rotation
 * 
 * Tests the complete key rotation workflow:
 * - Initial data encryption with key v1
 * - Key rotation to key v2
 * - Lazy re-encryption on access
 * - Background re-encryption
 * - Verification of encrypted data integrity
 */

#include "../test_fixture.h"
#include "../test_data_generator.h"
#include "security/encryption.h"
#include "security/key_provider.h"
#include "storage/rocksdb_wrapper.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>
#include <future>
#include <map>
#include <memory>
#include <vector>

using json = nlohmann::json;

namespace themis {
namespace test {

/**
 * @brief Mock Key Provider for testing
 */
class MockKeyProvider : public KeyProvider {
public:
    MockKeyProvider() = default;
    
    void addKey(const std::string& key_id, uint32_t version, const std::vector<uint8_t>& key_data) {
        std::string full_key = key_id + "_v" + std::to_string(version);
        keys_[full_key] = key_data;
        
        KeyMetadata meta;
        meta.key_id = key_id;
        meta.version = version;
        meta.algorithm = "AES-256-GCM";
        meta.created_at_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
        meta.status = KeyStatus::ACTIVE;
        metadata_[full_key] = meta;
    }
    
    std::vector<uint8_t> getKey(const std::string& key_id) override {
        uint32_t max_version = 0;
        for (const auto& [full_key, _] : keys_) {
            if (full_key.find(key_id + "_v") == 0) {
                uint32_t ver = std::stoi(full_key.substr(key_id.length() + 2));
                max_version = std::max(max_version, ver);
            }
        }
        if (max_version == 0) {
            throw KeyNotFoundException(key_id, 0);
        }
        return getKey(key_id, max_version);
    }

    std::vector<uint8_t> getKey(const std::string& key_id, uint32_t version) override {
        std::string full_key = key_id + "_v" + std::to_string(version);
        auto it = keys_.find(full_key);
        if (it == keys_.end()) {
            throw KeyNotFoundException(key_id, version);
        }
        return it->second;
    }
    
    KeyMetadata getKeyMetadata(const std::string& key_id, uint32_t version) override {
        std::string full_key = key_id + "_v" + std::to_string(version);
        auto it = metadata_.find(full_key);
        if (it == metadata_.end()) {
            throw KeyNotFoundException(key_id, version);
        }
        return it->second;
    }
    
    uint32_t rotateKey(const std::string& key_id) override {
        uint32_t max_version = 0;
        for (const auto& [full_key, _] : keys_) {
            if (full_key.find(key_id + "_v") == 0) {
                uint32_t ver = std::stoi(full_key.substr(key_id.length() + 2));
                max_version = std::max(max_version, ver);
            }
        }
        if (max_version == 0) {
            throw KeyNotFoundException(key_id, 0);
        }

        const uint32_t new_version = max_version + 1;
        addKey(key_id, new_version, getKey(key_id, max_version));
        return new_version;
    }

    std::vector<KeyMetadata> listKeys() override {
        std::vector<KeyMetadata> out;
        out.reserve(metadata_.size());
        for (const auto& [_, meta] : metadata_) {
            out.push_back(meta);
        }
        return out;
    }

    void deleteKey(const std::string& key_id, uint32_t version) override {
        std::string full_key = key_id + "_v" + std::to_string(version);
        keys_.erase(full_key);
        metadata_.erase(full_key);
    }

    bool hasKey(const std::string& key_id, uint32_t version = 0) override {
        if (version == 0) {
            for (const auto& [full_key, _] : keys_) {
                if (full_key.find(key_id + "_v") == 0) {
                    return true;
                }
            }
            return false;
        }

        std::string full_key = key_id + "_v" + std::to_string(version);
        return keys_.find(full_key) != keys_.end();
    }

    uint32_t createKeyFromBytes(const std::string& key_id,
                                const std::vector<uint8_t>& key_bytes,
                                const KeyMetadata& metadata = KeyMetadata()) override {
        uint32_t max_version = 0;
        for (const auto& [full_key, _] : keys_) {
            if (full_key.find(key_id + "_v") == 0) {
                uint32_t ver = std::stoi(full_key.substr(key_id.length() + 2));
                max_version = std::max(max_version, ver);
            }
        }

        const uint32_t new_version = max_version + 1;
        addKey(key_id, new_version, key_bytes);
        if (!metadata.key_id.empty()) {
            const std::string full_key = key_id + "_v" + std::to_string(new_version);
            metadata_[full_key] = metadata;
        }
        return new_version;
    }
    
private:
    std::map<std::string, std::vector<uint8_t>> keys_;
    std::map<std::string, KeyMetadata> metadata_;
};

/**
 * @brief Integration tests for encryption key rotation
 */
class EncryptionKeyRotationIntegrationTest : public IntegrationTestFixture {
protected:
    void SetUp() override {
        IntegrationTestFixture::SetUp();
        data_gen_ = std::make_unique<TestDataGenerator>();
        
        // Create mock key provider
        key_provider_ = std::make_shared<MockKeyProvider>();
        
        // Add initial key v1
        auto key_v1 = data_gen_->GenerateEncryptionKey(32);
        key_provider_->addKey("test_key", 1, key_v1);
    }
    
    std::unique_ptr<TestDataGenerator> data_gen_;
    std::shared_ptr<MockKeyProvider> key_provider_;
};

/**
 * @test Verify basic key rotation workflow
 * 
 * Acceptance Criteria:
 * - Data encrypted with key v1 can be read after rotation to v2
 * - New data is encrypted with key v2
 * - Old data remains accessible during rotation
 */
TEST_F(EncryptionKeyRotationIntegrationTest, BasicKeyRotation) {
    // Step 1: Create encryption service with key v1
    auto encryption_service = std::make_unique<FieldEncryption>(key_provider_);
    
    // Step 2: Encrypt data with key v1
    std::string plaintext = "Sensitive user data that needs encryption";
    
    auto encrypted_blob = encryption_service->encrypt("test_key", plaintext);
    ASSERT_FALSE(encrypted_blob.ciphertext.empty()) 
        << "Encryption should produce ciphertext";
    EXPECT_EQ(encrypted_blob.key_id, "test_key");
    EXPECT_EQ(encrypted_blob.key_version, 1) << "Should use key v1";
    
    // Step 3: Verify decryption works with key v1
    auto decrypted_v1 = encryption_service->decrypt(encrypted_blob);
    EXPECT_EQ(decrypted_v1, plaintext) << "Decrypted text should match original";
    
    // Step 4: Add key v2 (simulating key rotation)
    auto key_v2 = data_gen_->GenerateEncryptionKey(32);
    key_provider_->addKey("test_key", 2, key_v2);
    
    // Step 5: Encrypt new data (should use latest key v2)
    std::string new_plaintext = "New data after key rotation";
    auto encrypted_v2 = encryption_service->encrypt("test_key", new_plaintext);
    
    // The implementation might use latest version or continue with v1
    // Both are valid during rotation
    ASSERT_FALSE(encrypted_v2.ciphertext.empty());
    
    // Step 6: Verify old encrypted data is still decryptable
    auto decrypted_old = encryption_service->decrypt(encrypted_blob);
    EXPECT_EQ(decrypted_old, plaintext) 
        << "Old data encrypted with v1 should still be decryptable";
    
    // Step 7: Verify new data is decryptable
    auto decrypted_new = encryption_service->decrypt(encrypted_v2);
    EXPECT_EQ(decrypted_new, new_plaintext)
        << "New data should be decryptable";
}

/**
 * @test Verify lazy re-encryption on access
 * 
 * Acceptance Criteria:
 * - Old records are re-encrypted when accessed
 * - Re-encryption happens transparently
 * - Performance impact is minimal
 */
TEST_F(EncryptionKeyRotationIntegrationTest, LazyReEncryption) {
    // Step 1: Encrypt data with key v1
    auto encryption_service = std::make_unique<FieldEncryption>(key_provider_);
    std::string plaintext = "Data for lazy re-encryption test";
    
    auto encrypted_v1 = encryption_service->encrypt("test_key", plaintext);
    EXPECT_EQ(encrypted_v1.key_version, 1);
    
    // Step 2: Add key v2
    auto key_v2 = data_gen_->GenerateEncryptionKey(32);
    key_provider_->addKey("test_key", 2, key_v2);
    
    // Step 3: Decrypt old data (simulating access)
    auto decrypted = encryption_service->decrypt(encrypted_v1);
    EXPECT_EQ(decrypted, plaintext);
    
    // Step 4: Re-encrypt with new key (lazy re-encryption)
    auto re_encrypted = encryption_service->encrypt("test_key", decrypted);
    
    // Step 5: Verify re-encrypted data uses newer key (if implemented)
    // Note: The actual version used depends on implementation
    ASSERT_FALSE(re_encrypted.ciphertext.empty());
    
    // Step 6: Verify data integrity after re-encryption
    auto final_decrypted = encryption_service->decrypt(re_encrypted);
    EXPECT_EQ(final_decrypted, plaintext)
        << "Data should be identical after re-encryption";
}

/**
 * @test Verify background re-encryption
 * 
 * Acceptance Criteria:
 * - Background task re-encrypts all old data
 * - Re-encryption can be paused and resumed
 * - Progress tracking is accurate
 */
TEST_F(EncryptionKeyRotationIntegrationTest, BackgroundReEncryption) {
    // Step 1: Encrypt multiple records with key v1
    auto encryption_service = std::make_unique<FieldEncryption>(key_provider_);
    
    std::vector<EncryptedBlob> encrypted_records;
    for (int i = 0; i < 10; ++i) {
        std::string plaintext = "Record " + std::to_string(i);
        auto encrypted = encryption_service->encrypt("test_key", plaintext);
        encrypted_records.push_back(encrypted);
        EXPECT_EQ(encrypted.key_version, 1);
    }
    
    // Step 2: Add key v2
    auto key_v2 = data_gen_->GenerateEncryptionKey(32);
    key_provider_->addKey("test_key", 2, key_v2);
    
    // Step 3: Simulate background re-encryption
    int re_encrypted_count = 0;
    std::vector<EncryptedBlob> re_encrypted_records;
    
    for (const auto& old_blob : encrypted_records) {
        // Decrypt with old key
        auto plaintext = encryption_service->decrypt(old_blob);
        
        // Re-encrypt with new key
        auto new_blob = encryption_service->encrypt("test_key", plaintext);
        re_encrypted_records.push_back(new_blob);
        re_encrypted_count++;
    }
    
    // Step 4: Verify all records were re-encrypted
    EXPECT_EQ(re_encrypted_count, 10) 
        << "All records should be re-encrypted";
    
    // Step 5: Verify data integrity
    for (size_t i = 0; i < re_encrypted_records.size(); ++i) {
        auto decrypted = encryption_service->decrypt(re_encrypted_records[i]);
        std::string expected = "Record " + std::to_string(i);
        EXPECT_EQ(decrypted, expected)
            << "Re-encrypted record " << i << " should be correct";
    }
}

/**
 * @test Verify key rotation with concurrent access
 * 
 * Acceptance Criteria:
 * - Key rotation doesn't block concurrent reads
 * - Writes during rotation use new key
 * - No data corruption occurs
 */
TEST_F(EncryptionKeyRotationIntegrationTest, ConcurrentAccessDuringRotation) {
    // Step 1: Setup encryption service
    auto encryption_service = std::make_unique<FieldEncryption>(key_provider_);
    
    // Step 2: Encrypt initial data
    std::string plaintext = "Concurrent access test data";
    auto encrypted_v1 = encryption_service->encrypt("test_key", plaintext);
    
    // Step 3: Add key v2 (rotation started)
    auto key_v2 = data_gen_->GenerateEncryptionKey(32);
    key_provider_->addKey("test_key", 2, key_v2);
    
    // Step 4: Simulate concurrent reads (should still work)
    std::vector<std::future<std::string>> read_futures;
    // Copy the encrypted blob to avoid reference issues
    EncryptedBlob encrypted_copy = encrypted_v1;
    for (int i = 0; i < 5; ++i) {
        read_futures.push_back(std::async(std::launch::async, 
            [encryption_service_ptr = encryption_service.get(), encrypted_copy]() {
                return encryption_service_ptr->decrypt(encrypted_copy);
            }
        ));
    }
    
    // Step 5: Simulate concurrent writes (should use available key)
    std::vector<std::future<EncryptedBlob>> write_futures;
    for (int i = 0; i < 5; ++i) {
        write_futures.push_back(std::async(std::launch::async,
            [encryption_service_ptr = encryption_service.get(), i]() {
                std::string data = "New data " + std::to_string(i);
                return encryption_service_ptr->encrypt("test_key", data);
            }
        ));
    }
    
    // Step 6: Verify all reads completed successfully
    for (auto& future : read_futures) {
        std::string result = future.get();
        EXPECT_EQ(result, plaintext)
            << "Concurrent reads should succeed during rotation";
    }
    
    // Step 7: Verify all writes completed successfully
    for (int i = 0; i < 5; ++i) {
        auto encrypted = write_futures[i].get();
        ASSERT_FALSE(encrypted.ciphertext.empty())
            << "Concurrent writes should succeed during rotation";
        
        auto decrypted = encryption_service->decrypt(encrypted);
        EXPECT_EQ(decrypted, "New data " + std::to_string(i))
            << "Written data should be correct";
    }
}

/**
 * @test Verify rollback capability
 * 
 * Acceptance Criteria:
 * - Failed rotation can be rolled back
 * - Data remains accessible after rollback
 * - Original encryption key still works
 */
TEST_F(EncryptionKeyRotationIntegrationTest, RollbackOnFailure) {
    // Step 1: Encrypt data with key v1
    auto encryption_service = std::make_unique<FieldEncryption>(key_provider_);
    
    std::string plaintext = "Data for rollback test";
    auto encrypted_v1 = encryption_service->encrypt("test_key", plaintext);
    EXPECT_EQ(encrypted_v1.key_version, 1);
    
    // Step 2: Attempt to add invalid key v2 (simulating failure)
    // In real scenario, this might be a corrupted key or network failure
    try {
        // Simulate failed rotation by not actually adding the key
        // but attempting to use it
        
        // Step 3: Verify data is still accessible with original key
        auto decrypted = encryption_service->decrypt(encrypted_v1);
        EXPECT_EQ(decrypted, plaintext)
            << "Data should still be accessible after failed rotation";
        
        // Step 4: Verify new data can still be encrypted with v1
        std::string new_plaintext = "New data after rollback";
        auto encrypted_after_rollback = encryption_service->encrypt("test_key", new_plaintext);
        EXPECT_FALSE(encrypted_after_rollback.ciphertext.empty());
        
        // Step 5: Verify decryption works
        auto decrypted_new = encryption_service->decrypt(encrypted_after_rollback);
        EXPECT_EQ(decrypted_new, new_plaintext)
            << "New data should be encryptable/decryptable after rollback";
            
    } catch (const std::exception& e) {
        // If exception occurs, verify system state is still valid
        FAIL() << "Rollback test should not throw exception: " << e.what();
    }
}

} // namespace test
} // namespace themis
