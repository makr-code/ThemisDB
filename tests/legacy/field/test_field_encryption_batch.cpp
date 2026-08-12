#include <gtest/gtest.h>
#include "security/encryption.h"
#include "security/mock_key_provider.h"

using namespace themis;

TEST(FieldEncryptionBatch, RoundtripEncryptDecrypt) {
    auto provider = std::make_shared<MockKeyProvider>();
    // Create a deterministic test key (create with bytes) to avoid randomness in CI
    std::vector<uint8_t> key_bytes(32, 0x42);
    provider->createKeyFromBytes("user_pii", key_bytes);

    FieldEncryption enc(provider);

    std::vector<std::pair<std::string,std::string>> items = {
        {"salt-1", "hello world"},
        {"salt-2", "The quick brown fox"},
        {"salt-3", "Lorem ipsum"}
    };

    auto out = enc.encryptEntityBatch(items, "user_pii");
    std::cerr << "debug: out.size=" << out.size() << " items.size=" << items.size() << std::endl;
    ASSERT_EQ(out.size(), items.size());

    for (size_t i = 0; i < out.size(); ++i) {
        // Debug: print encrypted blob JSON to inspect IV/tag/ciphertext
        std::cerr << out[i].toJson().dump() << std::endl;
        auto decrypted = enc.decryptToString(out[i]);
        EXPECT_EQ(decrypted, items[i].second);
    }
}

// ============================================================================
// needsReEncryption via KeyProvider::getCurrentVersion (#145)
// ============================================================================

TEST(FieldEncryptionBatch, NeedsReEncryptionFalseForLatestVersion) {
    try {
        auto provider = std::make_shared<MockKeyProvider>();
        std::vector<uint8_t> key_bytes(32, 0xAB);
        provider->createKeyFromBytes("re_enc_key", key_bytes);  // version 1

        FieldEncryption enc(provider);

        // Encrypt produces blob at current version (1).
        auto blob = enc.encrypt("secret", "re_enc_key");
        EXPECT_FALSE(enc.needsReEncryption(blob, "re_enc_key"));
    } catch (const std::runtime_error& error) {
        if (std::string(error.what()).find("Field encryption unavailable") != std::string::npos) {
            GTEST_SKIP() << error.what();
        }
        throw;
    }
}

TEST(FieldEncryptionBatch, NeedsReEncryptionTrueAfterRotation) {
    try {
        auto provider = std::make_shared<MockKeyProvider>();
        std::vector<uint8_t> key_bytes(32, 0xCD);
        provider->createKeyFromBytes("rotate_key", key_bytes);   // version 1

        FieldEncryption enc(provider);

        // Encrypt at version 1.
        auto blob = enc.encrypt("secret", "rotate_key");
        ASSERT_EQ(blob.key_version, static_cast<uint32_t>(1));

        // Rotate to version 2.
        provider->rotateKey("rotate_key");

        // Now the blob is outdated.
        EXPECT_TRUE(enc.needsReEncryption(blob, "rotate_key"));
    } catch (const std::runtime_error& error) {
        if (std::string(error.what()).find("Field encryption unavailable") != std::string::npos) {
            GTEST_SKIP() << error.what();
        }
        throw;
    }
}

TEST(FieldEncryptionBatch, GetCurrentVersionReturnsProbeResult) {
    auto provider = std::make_shared<MockKeyProvider>();
    std::vector<uint8_t> key_bytes(32, 0xEF);
    provider->createKeyFromBytes("ver_key", key_bytes);   // version 1
    provider->rotateKey("ver_key");                        // version 2

    // Default probe implementation must find version 2.
    EXPECT_EQ(provider->getCurrentVersion("ver_key"), static_cast<uint32_t>(2));
}

