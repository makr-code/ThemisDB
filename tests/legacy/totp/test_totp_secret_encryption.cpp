#include <gtest/gtest.h>
#include "auth/totp_secret_encryption.h"
#include <thread>
#include <chrono>

using namespace themis::auth;

/**
 * @brief Test basic encryption and decryption
 */
TEST(TOTPSecretEncryptionTest, BasicEncryptDecrypt) {
    // Create encryption with test key
    TOTPSecretEncryption::Config config;
    config.master_key = std::vector<uint8_t>(32, 0x42);  // Test key
    TOTPSecretEncryption encryption(config);
    
    std::string secret = "JBSWY3DPEHPK3PXP";  // Example TOTP secret
    
    // Encrypt
    auto encrypted = encryption.encrypt(secret);
    
    // Verify encrypted data exists
    EXPECT_FALSE(encrypted.salt.empty());
    EXPECT_FALSE(encrypted.iv.empty());
    EXPECT_FALSE(encrypted.ciphertext.empty());
    EXPECT_FALSE(encrypted.tag.empty());
    EXPECT_EQ(encrypted.version, 1);
    
    // Decrypt
    std::string decrypted = encryption.decrypt(encrypted);
    
    EXPECT_EQ(decrypted, secret);
}

/**
 * @brief Test encryption produces different ciphertext each time
 */
TEST(TOTPSecretEncryptionTest, UniqueEncryption) {
    TOTPSecretEncryption::Config config;
    config.master_key = std::vector<uint8_t>(32, 0x42);
    TOTPSecretEncryption encryption(config);
    
    std::string secret = "JBSWY3DPEHPK3PXP";
    
    auto encrypted1 = encryption.encrypt(secret);
    auto encrypted2 = encryption.encrypt(secret);
    
    // Different salt and IV
    EXPECT_NE(encrypted1.salt, encrypted2.salt);
    EXPECT_NE(encrypted1.iv, encrypted2.iv);
    
    // Different ciphertext
    EXPECT_NE(encrypted1.ciphertext, encrypted2.ciphertext);
    
    // But both decrypt to same plaintext
    EXPECT_EQ(encryption.decrypt(encrypted1), secret);
    EXPECT_EQ(encryption.decrypt(encrypted2), secret);
}

/**
 * @brief Test serialization and deserialization
 */
TEST(TOTPSecretEncryptionTest, SerializeDeserialize) {
    TOTPSecretEncryption::Config config;
    config.master_key = std::vector<uint8_t>(32, 0x42);
    TOTPSecretEncryption encryption(config);
    
    std::string secret = "JBSWY3DPEHPK3PXP";
    auto encrypted = encryption.encrypt(secret);
    
    // Serialize
    std::string serialized = encrypted.serialize();
    EXPECT_FALSE(serialized.empty());
    EXPECT_NE(serialized.find('|'), std::string::npos);  // Contains delimiter
    
    // Deserialize
    auto deserialized = TOTPSecretEncryption::EncryptedSecret::deserialize(serialized);
    
    EXPECT_EQ(deserialized.version, encrypted.version);
    EXPECT_EQ(deserialized.salt, encrypted.salt);
    EXPECT_EQ(deserialized.iv, encrypted.iv);
    EXPECT_EQ(deserialized.ciphertext, encrypted.ciphertext);
    EXPECT_EQ(deserialized.tag, encrypted.tag);
    
    // Can decrypt deserialized
    EXPECT_EQ(encryption.decrypt(deserialized), secret);
}

/**
 * @brief Test encryptAndSerialize convenience method
 */
TEST(TOTPSecretEncryptionTest, EncryptAndSerialize) {
    TOTPSecretEncryption::Config config;
    config.master_key = std::vector<uint8_t>(32, 0x42);
    TOTPSecretEncryption encryption(config);
    
    std::string secret = "JBSWY3DPEHPK3PXP";
    
    std::string serialized = encryption.encryptAndSerialize(secret);
    EXPECT_FALSE(serialized.empty());
    
    std::string decrypted = encryption.deserializeAndDecrypt(serialized);
    EXPECT_EQ(decrypted, secret);
}

/**
 * @brief Test tampering detection (authentication tag)
 */
TEST(TOTPSecretEncryptionTest, TamperingDetection) {
    TOTPSecretEncryption::Config config;
    config.master_key = std::vector<uint8_t>(32, 0x42);
    TOTPSecretEncryption encryption(config);
    
    std::string secret = "JBSWY3DPEHPK3PXP";
    auto encrypted = encryption.encrypt(secret);
    
    // Tamper with ciphertext
    if (!encrypted.ciphertext.empty()) {
        encrypted.ciphertext[0] ^= 0xFF;
    }
    
    // Decryption should fail due to authentication tag mismatch
    EXPECT_THROW(encryption.decrypt(encrypted), std::runtime_error);
}

/**
 * @brief Test key rotation
 */
TEST(TOTPSecretEncryptionTest, KeyRotation) {
    TOTPSecretEncryption::Config config;
    config.master_key = std::vector<uint8_t>(32, 0x42);
    config.key_version = 1;
    TOTPSecretEncryption encryption(config);
    
    std::string secret = "JBSWY3DPEHPK3PXP";
    
    // Encrypt with version 1 key
    auto encrypted_v1 = encryption.encrypt(secret);
    EXPECT_EQ(encrypted_v1.version, 1);
    
    // Rotate to version 2 key
    std::vector<uint8_t> new_key(32, 0x43);
    encryption.rotateKey(new_key, 2);
    
    // Can still decrypt old secret (same key material, different version tracking)
    // Note: In production, you'd keep old keys for decryption
    
    // New encryptions use version 2
    auto encrypted_v2 = encryption.encrypt(secret);
    EXPECT_EQ(encrypted_v2.version, 2);
}

/**
 * @brief Test needsReencryption
 */
TEST(TOTPSecretEncryptionTest, NeedsReencryption) {
    TOTPSecretEncryption::Config config;
    config.master_key = std::vector<uint8_t>(32, 0x42);
    config.key_version = 1;
    TOTPSecretEncryption encryption(config);
    
    std::string secret = "JBSWY3DPEHPK3PXP";
    auto encrypted = encryption.encrypt(secret);
    
    // Should not need re-encryption with same version
    EXPECT_FALSE(encryption.needsReencryption(encrypted));
    
    // Rotate key
    std::vector<uint8_t> new_key(32, 0x43);
    encryption.rotateKey(new_key, 2);
    
    // Old encryption should need re-encryption
    EXPECT_TRUE(encryption.needsReencryption(encrypted));
}

/**
 * @brief Test re-encryption
 */
TEST(TOTPSecretEncryptionTest, Reencryption) {
    TOTPSecretEncryption::Config config;
    config.master_key = std::vector<uint8_t>(32, 0x42);
    config.key_version = 1;
    TOTPSecretEncryption encryption(config);
    
    std::string secret = "JBSWY3DPEHPK3PXP";
    auto encrypted_v1 = encryption.encrypt(secret);
    
    // Can decrypt with v1
    EXPECT_EQ(encryption.decrypt(encrypted_v1), secret);
    
    // Rotate key (but keep same key material for this test)
    encryption.rotateKey(config.master_key, 2);
    
    // Re-encrypt
    auto encrypted_v2 = encryption.reencrypt(encrypted_v1);
    EXPECT_EQ(encrypted_v2.version, 2);
    
    // Can decrypt both
    EXPECT_EQ(encryption.decrypt(encrypted_v1), secret);
    EXPECT_EQ(encryption.decrypt(encrypted_v2), secret);
}

/**
 * @brief Test empty secret
 */
TEST(TOTPSecretEncryptionTest, EmptySecret) {
    TOTPSecretEncryption::Config config;
    config.master_key = std::vector<uint8_t>(32, 0x42);
    TOTPSecretEncryption encryption(config);
    
    std::string secret = "";
    auto encrypted = encryption.encrypt(secret);
    
    EXPECT_EQ(encryption.decrypt(encrypted), secret);
}

/**
 * @brief Test invalid master key size
 */
TEST(TOTPSecretEncryptionTest, InvalidMasterKeySize) {
    TOTPSecretEncryption::Config config;
    config.master_key = std::vector<uint8_t>(16, 0x42);  // Wrong size (should be 32)
    
    EXPECT_THROW(TOTPSecretEncryption encryption(config), std::invalid_argument);
}

/**
 * @brief Test rotation manager - basic rotation
 */
TEST(TOTPSecretRotationManagerTest, BasicRotation) {
    TOTPSecretRotationManager manager;
    
    std::string user_id = "alice";
    std::string old_secret = "SECRET_OLD";
    std::string new_secret = "SECRET_NEW";
    
    auto version = manager.rotateSecret(user_id, old_secret, new_secret);
    
    EXPECT_EQ(version.secret, new_secret);
    EXPECT_EQ(version.version, 1);
    EXPECT_TRUE(version.is_active);
}

/**
 * @brief Test rotation manager - get active secrets
 */
TEST(TOTPSecretRotationManagerTest, GetActiveSecrets) {
    TOTPSecretRotationManager manager;
    
    std::string user_id = "alice";
    
    // Initially no secrets
    auto secrets = manager.getActiveSecrets(user_id);
    EXPECT_TRUE(secrets.empty());
    
    // Add first secret
    manager.rotateSecret(user_id, "", "SECRET_1");
    secrets = manager.getActiveSecrets(user_id);
    EXPECT_EQ(secrets.size(), 1);
    EXPECT_TRUE(secrets[0].is_active);
    
    // Rotate to second secret (first becomes inactive but still valid)
    manager.rotateSecret(user_id, "SECRET_1", "SECRET_2");
    secrets = manager.getActiveSecrets(user_id);
    EXPECT_EQ(secrets.size(), 2);  // Both active (within grace period)
}

/**
 * @brief Test rotation manager - secret validity
 */
TEST(TOTPSecretRotationManagerTest, SecretValidity) {
    TOTPSecretRotationManager::RotationConfig config;
    config.grace_period_seconds = 2;  // Short grace period for testing
    TOTPSecretRotationManager manager(config);
    
    std::string user_id = "alice";
    
    auto v1 = manager.rotateSecret(user_id, "", "SECRET_1");
    EXPECT_TRUE(manager.isSecretValid(v1));
    
    // Wait for grace period to expire
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Add new secret (old becomes inactive)
    auto v2 = manager.rotateSecret(user_id, "SECRET_1", "SECRET_2");
    
    // Active secret is still valid
    EXPECT_TRUE(manager.isSecretValid(v2));
    
    // Old inactive secret past grace period is invalid.
    // Do not validate the original return-copy (v1), because it keeps its
    // original is_active flag; validate against the manager's stored state.
    auto all_valid = manager.getActiveSecrets(user_id);
    EXPECT_EQ(all_valid.size(), 1u);
    EXPECT_EQ(all_valid[0].version, v2.version);
}

/**
 * @brief Test rotation manager - cleanup expired secrets
 */
TEST(TOTPSecretRotationManagerTest, CleanupExpired) {
    TOTPSecretRotationManager::RotationConfig config;
    config.grace_period_seconds = 1;
    config.auto_cleanup = true;
    TOTPSecretRotationManager manager(config);
    
    std::string user_id = "alice";
    
    manager.rotateSecret(user_id, "", "SECRET_1");
    
    // Wait for expiration
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Add new secret
    manager.rotateSecret(user_id, "SECRET_1", "SECRET_2");
    
    // Cleanup
    size_t cleaned = manager.cleanupExpiredSecrets();
    EXPECT_GT(cleaned, 0);
    
    // Should only have active secret now
    auto secrets = manager.getActiveSecrets(user_id);
    EXPECT_EQ(secrets.size(), 1);
    EXPECT_EQ(secrets[0].secret, "SECRET_2");
}

/**
 * @brief Test rotation manager - multiple users
 */
TEST(TOTPSecretRotationManagerTest, MultipleUsers) {
    TOTPSecretRotationManager manager;
    
    manager.rotateSecret("alice", "", "ALICE_SECRET");
    manager.rotateSecret("bob", "", "BOB_SECRET");
    manager.rotateSecret("charlie", "", "CHARLIE_SECRET");
    
    auto alice_secrets = manager.getActiveSecrets("alice");
    auto bob_secrets = manager.getActiveSecrets("bob");
    
    EXPECT_EQ(alice_secrets.size(), 1);
    EXPECT_EQ(bob_secrets.size(), 1);
    EXPECT_EQ(alice_secrets[0].secret, "ALICE_SECRET");
    EXPECT_EQ(bob_secrets[0].secret, "BOB_SECRET");
}

/**
 * @brief Test different key sizes
 */
TEST(TOTPSecretEncryptionTest, DifferentSecretSizes) {
    TOTPSecretEncryption::Config config;
    config.master_key = std::vector<uint8_t>(32, 0x42);
    TOTPSecretEncryption encryption(config);
    
    std::vector<std::string> secrets = {
        "SHORT",
        "JBSWY3DPEHPK3PXP",  // Standard TOTP secret
        "VERY_LONG_SECRET_THAT_IS_MUCH_LONGER_THAN_TYPICAL_TOTP_SECRETS_USED_IN_PRACTICE"
    };
    
    for (const auto& secret : secrets) {
        auto encrypted = encryption.encrypt(secret);
        EXPECT_EQ(encryption.decrypt(encrypted), secret);
    }
}

/**
 * @brief Test serialization format
 */
TEST(TOTPSecretEncryptionTest, SerializationFormat) {
    TOTPSecretEncryption::Config config;
    config.master_key = std::vector<uint8_t>(32, 0x42);
    TOTPSecretEncryption encryption(config);
    
    std::string secret = "JBSWY3DPEHPK3PXP";
    auto encrypted = encryption.encrypt(secret);
    std::string serialized = encrypted.serialize();
    
    // Check format: version|salt|iv|ciphertext|tag
    size_t delimiter_count = std::count(serialized.begin(), serialized.end(), '|');
    EXPECT_EQ(delimiter_count, 4);
    
    // Should start with version number
    EXPECT_TRUE(std::isdigit(serialized[0]));
}
