/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            encrypted_blob_backend.h                           ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-04-17                                         ║
  Author:          ThemisDB Team                                      ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     240                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file encrypted_blob_backend.h
 * @brief Transparent client-side AES-256-GCM encryption decorator for
 *        `IBlobStorageBackend`.
 *
 * `EncryptedBlobBackend` wraps any existing `IBlobStorageBackend` and
 * encrypts every blob before storage and decrypts it transparently on
 * retrieval.  The cipher is **AES-256-GCM** with a 96-bit random IV
 * (nonce) prepended to the ciphertext.
 *
 * ### Key management
 * Key material is supplied by an `IEncryptionKeyProvider`.  The bundled
 * `StaticKeyProvider` accepts a fixed 256-bit key (suitable for tests
 * and single-node deployments).  Production deployments should integrate
 * with an HSM or KMS via a custom `IEncryptionKeyProvider` implementation.
 *
 * ### Wire format
 * Each stored blob is laid out as:
 * ```
 * [ IV (12 bytes) | Ciphertext | GCM Tag (16 bytes) ]
 * ```
 * The BlobRef `hash_sha256` field stores the SHA-256 of the **plaintext**
 * to allow integrity verification after decryption.
 *
 * ### Performance
 * - Encryption overhead ≤ 5 % vs. the unencrypted path on AES-NI-capable
 *   hardware (i.e., any modern x86-64 or aarch64 server CPU).
 *
 * ### Thread safety
 * All public methods are thread-safe.  The underlying backend must also be
 * thread-safe.
 *
 * ### Usage
 * @code
 * auto raw  = std::make_shared<FilesystemBlobBackend>("/data/blobs");
 * auto keys = std::make_shared<StaticKeyProvider>(my_32_byte_key);
 * auto enc  = std::make_shared<EncryptedBlobBackend>(raw, keys);
 *
 * auto ref = enc->put("doc-1", plaintext_bytes);
 * auto out = enc->get(ref.value());   // transparently decrypts
 * @endcode
 *
 * Copyright (c) 2025-2026 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/blob_storage_backend.h"
#include "utils/expected.h"

#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace storage {

// ============================================================================
// IEncryptionKeyProvider
// ============================================================================

/**
 * @brief Abstract provider of symmetric encryption keys.
 *
 * Implementations may delegate to an HSM, AWS KMS, HashiCorp Vault, or a
 * static in-memory key (see `StaticKeyProvider`).
 */
class IEncryptionKeyProvider {
public:
    virtual ~IEncryptionKeyProvider() = default;

    /**
     * @brief Return the current 256-bit (32-byte) encryption key.
     *
     * The returned array must remain valid for the duration of the
     * encryption/decryption call.
     */
    [[nodiscard]] virtual std::array<uint8_t, 32> currentKey() const = 0;

    /**
     * @brief Human-readable key provider name (e.g., "static", "kms", "hsm").
     */
    [[nodiscard]] virtual std::string name() const = 0;
};

// ============================================================================
// StaticKeyProvider
// ============================================================================

/**
 * @brief Simple key provider backed by a fixed 256-bit key.
 *
 * Suitable for unit tests and single-node deployments.  For production use
 * provide a custom `IEncryptionKeyProvider` that retrieves key material from
 * a secrets management service.
 *
 * Thread-safe: `currentKey()` is const and reads an immutable field.
 */
class StaticKeyProvider final : public IEncryptionKeyProvider {
public:
    /**
     * @brief Construct with a fixed 32-byte key.
     * @param key 256-bit key material.
     */
    explicit StaticKeyProvider(std::array<uint8_t, 32> key) noexcept;

    [[nodiscard]] std::array<uint8_t, 32> currentKey() const override;

    [[nodiscard]] std::string name() const override { return "static"; }

private:
    std::array<uint8_t, 32> key_;
};

// ============================================================================
// EncryptionStats
// ============================================================================

/**
 * @brief Cumulative encryption / decryption counters.
 */
struct EncryptionStats {
    uint64_t blobs_encrypted   = 0; ///< Total `put()` calls processed.
    uint64_t blobs_decrypted   = 0; ///< Total `get()` calls processed.
    uint64_t bytes_encrypted   = 0; ///< Plaintext bytes encrypted.
    uint64_t bytes_decrypted   = 0; ///< Plaintext bytes decrypted.
    uint64_t decrypt_failures  = 0; ///< GCM tag verification failures.
};

// ============================================================================
// EncryptedBlobBackend
// ============================================================================

/**
 * @brief AES-256-GCM encryption decorator for `IBlobStorageBackend`.
 *
 * Wraps an existing backend; all data is encrypted before being forwarded to
 * the underlying store and decrypted transparently on retrieval.
 *
 * Thread-safe: protected by an internal `std::mutex`.
 */
class EncryptedBlobBackend final : public IBlobStorageBackend {
public:
    /**
     * @brief Construct an encrypting backend.
     *
     * @param inner    Underlying backend where ciphertext is stored.
     * @param keys     Key provider; must outlive this object.
     * @throws std::invalid_argument if @p inner or @p keys is nullptr.
     */
    EncryptedBlobBackend(std::shared_ptr<IBlobStorageBackend> inner,
                         std::shared_ptr<IEncryptionKeyProvider> keys);

    /**
     * @brief Encrypt @p data and store in the underlying backend.
     *
     * A fresh 12-byte random IV is generated per call.  The ciphertext
     * layout is `[ IV (12 B) | ciphertext | GCM tag (16 B) ]`.
     */
    Result<BlobRef> put(const std::string& blob_id,
                        const std::vector<uint8_t>& data) override;

    /**
     * @brief Retrieve and decrypt a blob.
     *
     * @throws std::runtime_error if GCM authentication tag verification fails.
     */
    Result<std::vector<uint8_t>> get(const BlobRef& ref) override;

    /** @brief Delegate to the underlying backend. */
    Result<void> remove(const BlobRef& ref) override;

    /** @brief Delegate to the underlying backend. */
    bool exists(const BlobRef& ref) override;

    [[nodiscard]] std::string name() const override;

    [[nodiscard]] bool isAvailable() const override;

    /**
     * @brief Read a snapshot of cumulative encryption statistics.
     *
     * Thread-safe (takes the internal mutex).
     */
    [[nodiscard]] EncryptionStats stats() const;

private:
    /// Encrypt plaintext with AES-256-GCM; prepend 12-byte IV.
    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& plaintext) const;

    /// Decrypt ciphertext (with prepended IV).  Returns plaintext or throws.
    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& ciphertext) const;

    std::shared_ptr<IBlobStorageBackend>  inner_;
    std::shared_ptr<IEncryptionKeyProvider> keys_;

    mutable std::mutex  mutex_;
    mutable EncryptionStats stats_;
};

} // namespace storage
} // namespace themis
