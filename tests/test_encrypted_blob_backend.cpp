/**
 * @file test_encrypted_blob_backend.cpp
 * @brief Unit tests for storage::EncryptedBlobBackend and
 *        storage::StaticKeyProvider.
 *
 * Test IDs: EBB-01 … EBB-10
 *
 * The tests use an in-memory stub backend to avoid filesystem I/O.
 */

#include <gtest/gtest.h>
#include "storage/encrypted_blob_backend.h"
#include "utils/expected.h"

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

using namespace themis::storage;

// ============================================================================
// InMemoryStubBackend — test double
// ============================================================================

class InMemoryStubBackend final : public IBlobStorageBackend {
public:
    themis::Result<BlobRef> put(const std::string& blob_id,
                                const std::vector<uint8_t>& data) override
    {
        store_[blob_id] = data;
        BlobRef ref;
        ref.id   = blob_id;
        ref.type = BlobStorageType::INLINE;
        ref.uri  = "mem://" + blob_id;
        ref.size_bytes = static_cast<int64_t>(data.size());
        return ref;
    }

    themis::Result<std::vector<uint8_t>> get(const BlobRef& ref) override
    {
        auto it = store_.find(ref.id);
        if (it == store_.end()) {
            return themis::Err<std::vector<uint8_t>>(
                themis::errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND,
                "blob not found: " + ref.id);
        }
        return it->second;
    }

    themis::Result<void> remove(const BlobRef& ref) override
    {
        store_.erase(ref.id);
        return themis::OkVoid();
    }

    bool exists(const BlobRef& ref) override
    {
        return store_.count(ref.id) > 0;
    }

    std::string name() const override { return "in_memory_stub"; }
    bool isAvailable() const override { return true; }

    const std::vector<uint8_t>* raw(const std::string& id) const
    {
        auto it = store_.find(id);
        return (it != store_.end()) ? &it->second : nullptr;
    }

private:
    std::unordered_map<std::string, std::vector<uint8_t>> store_;
};

// ============================================================================
// Fixture
// ============================================================================

class EncryptedBlobBackendTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // Fixed 256-bit test key (32 bytes of 0xAB).
        key_.fill(0xAB);

        stub_  = std::make_shared<InMemoryStubBackend>();
        keys_  = std::make_shared<StaticKeyProvider>(key_);
        enc_   = std::make_shared<EncryptedBlobBackend>(stub_, keys_);
    }

    std::array<uint8_t, 32>              key_{};
    std::shared_ptr<InMemoryStubBackend> stub_;
    std::shared_ptr<StaticKeyProvider>   keys_;
    std::shared_ptr<EncryptedBlobBackend> enc_;
};

// ============================================================================
// EBB-01 — round-trip: encrypt then decrypt yields original plaintext
// ============================================================================
TEST_F(EncryptedBlobBackendTest, EBB_01_RoundTrip)
{
    std::vector<uint8_t> plaintext = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};

    auto put_result = enc_->put("blob-1", plaintext);
    ASSERT_TRUE(put_result.has_value()) << "put() failed";

    auto get_result = enc_->get(put_result.value());
    ASSERT_TRUE(get_result.has_value()) << "get() failed";

    EXPECT_EQ(get_result.value(), plaintext);
}

// ============================================================================
// EBB-02 — stored data is not equal to plaintext (actually encrypted)
// ============================================================================
TEST_F(EncryptedBlobBackendTest, EBB_02_CiphertextNotEqualToPlaintext)
{
    std::vector<uint8_t> plaintext(64, 0xCC);

    auto put_result = enc_->put("blob-2", plaintext);
    ASSERT_TRUE(put_result.has_value());

    const auto* raw = stub_->raw("blob-2");
    ASSERT_NE(raw, nullptr);
    EXPECT_NE(*raw, plaintext) << "Stored data must differ from plaintext";
    // Stored size = IV (12) + plaintext (64) + GCM tag (16) = 92 bytes.
    EXPECT_EQ(raw->size(), 12u + 64u + 16u);
}

// ============================================================================
// EBB-03 — empty blob round-trip
// ============================================================================
TEST_F(EncryptedBlobBackendTest, EBB_03_EmptyBlobRoundTrip)
{
    std::vector<uint8_t> empty;

    auto put_result = enc_->put("empty-blob", empty);
    ASSERT_TRUE(put_result.has_value());

    auto get_result = enc_->get(put_result.value());
    ASSERT_TRUE(get_result.has_value());
    EXPECT_TRUE(get_result.value().empty());
}

// ============================================================================
// EBB-04 — GCM tag verification catches tampering
// ============================================================================
TEST_F(EncryptedBlobBackendTest, EBB_04_TamperDetection)
{
    std::vector<uint8_t> plaintext(32, 0x55);

    auto put_result = enc_->put("tampered", plaintext);
    ASSERT_TRUE(put_result.has_value());

    // Flip a byte in the middle of the ciphertext in the stub store.
    const std::string id = "tampered";
    auto it = stub_->raw(id);
    ASSERT_NE(it, nullptr);
    // Corrupt the ciphertext region (byte 13, after 12-byte IV).
    // We need a mutable reference — re-put a modified copy.
    std::vector<uint8_t> corrupted = *it;
    corrupted[13] ^= 0xFF;
    (void)stub_->put(id, corrupted);

    // Decryption should throw due to GCM tag mismatch.
    EXPECT_THROW((void)enc_->get(put_result.value()), std::runtime_error);
}

// ============================================================================
// EBB-05 — stats() counts encrypt/decrypt calls
// ============================================================================
TEST_F(EncryptedBlobBackendTest, EBB_05_Stats)
{
    std::vector<uint8_t> data(10, 0x01);

    (void)enc_->put("s1", data);
    (void)enc_->put("s2", data);

    BlobRef ref1; ref1.id = "s1"; ref1.type = BlobStorageType::INLINE; ref1.uri = "mem://s1";
    BlobRef ref2; ref2.id = "s2"; ref2.type = BlobStorageType::INLINE; ref2.uri = "mem://s2";
    (void)ref2;
    (void)enc_->get(ref1);

    auto st = enc_->stats();
    EXPECT_EQ(st.blobs_encrypted, 2u);
    EXPECT_EQ(st.blobs_decrypted, 1u);
    EXPECT_EQ(st.bytes_encrypted, 20u);
    EXPECT_EQ(st.bytes_decrypted, 10u);
    EXPECT_EQ(st.decrypt_failures, 0u);
}

// ============================================================================
// EBB-06 — name() incorporates inner backend name
// ============================================================================
TEST_F(EncryptedBlobBackendTest, EBB_06_Name)
{
    EXPECT_EQ(enc_->name(), "encrypted(in_memory_stub)");
}

// ============================================================================
// EBB-07 — isAvailable() delegates to inner backend
// ============================================================================
TEST_F(EncryptedBlobBackendTest, EBB_07_IsAvailable)
{
    EXPECT_TRUE(enc_->isAvailable());
}

// ============================================================================
// EBB-08 — remove() delegates to inner backend
// ============================================================================
TEST_F(EncryptedBlobBackendTest, EBB_08_Remove)
{
    std::vector<uint8_t> data = {1, 2, 3};
    auto put_result = enc_->put("to-remove", data);
    ASSERT_TRUE(put_result.has_value());

    auto remove_result = enc_->remove(put_result.value());
    EXPECT_TRUE(remove_result.has_value());

    // After removal, exists() should return false.
    EXPECT_FALSE(enc_->exists(put_result.value()));
}

// ============================================================================
// EBB-09 — null inner backend throws at construction
// ============================================================================
TEST_F(EncryptedBlobBackendTest, EBB_09_NullInnerThrows)
{
    EXPECT_THROW(
        EncryptedBlobBackend(nullptr, keys_),
        std::invalid_argument);
}

// ============================================================================
// EBB-10 — null key provider throws at construction
// ============================================================================
TEST_F(EncryptedBlobBackendTest, EBB_10_NullKeyProviderThrows)
{
    EXPECT_THROW(
        EncryptedBlobBackend(stub_, nullptr),
        std::invalid_argument);
}
