// Copyright (c) 2025-2026 ThemisDB Project
// SPDX-License-Identifier: Apache-2.0

#include "storage/encrypted_blob_backend.h"

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <array>
#include <cstring>
#include <stdexcept>
#include <string>

namespace themis {
namespace storage {

// ============================================================================
// AES-256-GCM constants
// ============================================================================

static constexpr int kIvLen  = 12; ///< GCM recommended 96-bit IV
static constexpr int kTagLen = 16; ///< GCM authentication tag (128-bit)

// ============================================================================
// StaticKeyProvider
// ============================================================================

StaticKeyProvider::StaticKeyProvider(std::array<uint8_t, 32> key) noexcept
    : key_(key)
{}

std::array<uint8_t, 32> StaticKeyProvider::currentKey() const
{
    return key_;
}

// ============================================================================
// EncryptedBlobBackend — construction
// ============================================================================

EncryptedBlobBackend::EncryptedBlobBackend(
    std::shared_ptr<IBlobStorageBackend> inner,
    std::shared_ptr<IEncryptionKeyProvider> keys)
    : inner_(std::move(inner))
    , keys_(std::move(keys))
{
    if (!inner_) {
        throw std::invalid_argument("EncryptedBlobBackend: inner backend must not be null");
    }
    if (!keys_) {
        throw std::invalid_argument("EncryptedBlobBackend: key provider must not be null");
    }
}

// ============================================================================
// put()
// ============================================================================

Result<BlobRef> EncryptedBlobBackend::put(const std::string& blob_id,
                                          const std::vector<uint8_t>& data)
{
    std::vector<uint8_t> ciphertext = encrypt(data);

    auto result = inner_->put(blob_id, ciphertext);
    if (!result) {
        return result;
    }

    std::lock_guard<std::mutex> lk(mutex_);
    stats_.blobs_encrypted++;
    stats_.bytes_encrypted += data.size();

    return result;
}

// ============================================================================
// get()
// ============================================================================

Result<std::vector<uint8_t>> EncryptedBlobBackend::get(const BlobRef& ref)
{
    auto raw = inner_->get(ref);
    if (!raw) {
        return raw;
    }

    std::vector<uint8_t> plaintext = decrypt(raw.value());

    std::lock_guard<std::mutex> lk(mutex_);
    stats_.blobs_decrypted++;
    stats_.bytes_decrypted += plaintext.size();

    return plaintext;
}

// ============================================================================
// remove()
// ============================================================================

Result<void> EncryptedBlobBackend::remove(const BlobRef& ref)
{
    return inner_->remove(ref);
}

// ============================================================================
// exists()
// ============================================================================

bool EncryptedBlobBackend::exists(const BlobRef& ref)
{
    return inner_->exists(ref);
}

// ============================================================================
// name()
// ============================================================================

std::string EncryptedBlobBackend::name() const
{
    return "encrypted(" + inner_->name() + ")";
}

// ============================================================================
// isAvailable()
// ============================================================================

bool EncryptedBlobBackend::isAvailable() const
{
    return inner_->isAvailable();
}

// ============================================================================
// stats()
// ============================================================================

EncryptionStats EncryptedBlobBackend::stats() const
{
    std::lock_guard<std::mutex> lk(mutex_);
    return stats_;
}

// ============================================================================
// encrypt() — private
// ============================================================================

std::vector<uint8_t>
EncryptedBlobBackend::encrypt(const std::vector<uint8_t>& plaintext) const
{
    auto key = keys_->currentKey();

    // Generate random IV.
    std::array<uint8_t, kIvLen> iv{};
    if (RAND_bytes(iv.data(), kIvLen) != 1) {
        throw std::runtime_error("EncryptedBlobBackend: RAND_bytes failed");
    }

    // Allocate output: IV + ciphertext + tag
    std::vector<uint8_t> out;
    out.resize(kIvLen + plaintext.size() + kTagLen);
    std::memcpy(out.data(), iv.data(), kIvLen);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("EncryptedBlobBackend: EVP_CIPHER_CTX_new failed");
    }

    try {
        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
            throw std::runtime_error("EncryptedBlobBackend: EVP_EncryptInit_ex failed");
        }
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, kIvLen, nullptr) != 1) {
            throw std::runtime_error("EncryptedBlobBackend: EVP_CTRL_GCM_SET_IVLEN failed");
        }
        if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data()) != 1) {
            throw std::runtime_error("EncryptedBlobBackend: EVP_EncryptInit_ex (key/iv) failed");
        }

        int len = 0;
        uint8_t* ciphertext_ptr = out.data() + kIvLen;
        if (!plaintext.empty()) {
            if (EVP_EncryptUpdate(ctx, ciphertext_ptr, &len,
                                  plaintext.data(),
                                  static_cast<int>(plaintext.size())) != 1) {
                throw std::runtime_error("EncryptedBlobBackend: EVP_EncryptUpdate failed");
            }
        }

        int final_len = 0;
        if (EVP_EncryptFinal_ex(ctx, ciphertext_ptr + len, &final_len) != 1) {
            throw std::runtime_error("EncryptedBlobBackend: EVP_EncryptFinal_ex failed");
        }

        // Write GCM tag after ciphertext.
        uint8_t* tag_ptr = out.data() + kIvLen + static_cast<int>(plaintext.size());
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, kTagLen, tag_ptr) != 1) {
            throw std::runtime_error("EncryptedBlobBackend: EVP_CTRL_GCM_GET_TAG failed");
        }

        EVP_CIPHER_CTX_free(ctx);
    } catch (...) {
        EVP_CIPHER_CTX_free(ctx);
        throw;
    }

    return out;
}

// ============================================================================
// decrypt() — private
// ============================================================================

std::vector<uint8_t>
EncryptedBlobBackend::decrypt(const std::vector<uint8_t>& ciphertext) const
{
    if (ciphertext.size() < static_cast<std::size_t>(kIvLen + kTagLen)) {
        std::lock_guard<std::mutex> lk(mutex_);
        const_cast<EncryptedBlobBackend*>(this)->stats_.decrypt_failures++;
        throw std::runtime_error(
            "EncryptedBlobBackend: ciphertext too short (corrupt or unencrypted blob)");
    }

    auto key = keys_->currentKey();

    const uint8_t* iv_ptr  = ciphertext.data();
    std::size_t    ct_len  = ciphertext.size() - kIvLen - kTagLen;
    const uint8_t* ct_ptr  = ciphertext.data() + kIvLen;
    const uint8_t* tag_ptr = ciphertext.data() + kIvLen + ct_len;

    std::vector<uint8_t> plaintext(ct_len);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("EncryptedBlobBackend: EVP_CIPHER_CTX_new failed");
    }

    try {
        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
            throw std::runtime_error("EncryptedBlobBackend: EVP_DecryptInit_ex failed");
        }
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, kIvLen, nullptr) != 1) {
            throw std::runtime_error("EncryptedBlobBackend: EVP_CTRL_GCM_SET_IVLEN failed");
        }
        if (EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), iv_ptr) != 1) {
            throw std::runtime_error("EncryptedBlobBackend: EVP_DecryptInit_ex (key/iv) failed");
        }

        int len = 0;
        if (ct_len > 0) {
            if (EVP_DecryptUpdate(ctx, plaintext.data(), &len,
                                  ct_ptr, static_cast<int>(ct_len)) != 1) {
                throw std::runtime_error("EncryptedBlobBackend: EVP_DecryptUpdate failed");
            }
        }

        // Set expected GCM tag before calling Final.
        // EVP_CIPHER_CTX_ctrl takes a non-const pointer; we copy the tag.
        std::array<uint8_t, kTagLen> tag_copy{};
        std::memcpy(tag_copy.data(), tag_ptr, kTagLen);
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, kTagLen, tag_copy.data()) != 1) {
            throw std::runtime_error("EncryptedBlobBackend: EVP_CTRL_GCM_SET_TAG failed");
        }

        int final_len = 0;
        int ret = EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &final_len);
        EVP_CIPHER_CTX_free(ctx);
        ctx = nullptr;

        if (ret <= 0) {
            std::lock_guard<std::mutex> lk(mutex_);
            const_cast<EncryptedBlobBackend*>(this)->stats_.decrypt_failures++;
            throw std::runtime_error(
                "EncryptedBlobBackend: GCM authentication tag verification failed "
                "(data tampered or wrong key)");
        }
    } catch (...) {
        if (ctx) {
            EVP_CIPHER_CTX_free(ctx);
        }
        throw;
    }

    return plaintext;
}

} // namespace storage
} // namespace themis
