/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            export_encryption.h                                ║
  Version:         0.0.34                                             ║
  Last Modified:   2026-02-28                                         ║
  Author:          copilot-swe-agent[bot]                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "security/key_provider.h"
#include <memory>
#include <string>
#include <vector>

namespace themis::exporters {

/**
 * @brief Configuration for AES-256-GCM export-level encryption.
 *
 * Export encryption wraps the output of any exporter in an encrypted
 * container. A per-job data-encryption-key (DEK) is derived from the
 * key-encryption-key (KEK) stored in the KeyProvider using HKDF-SHA256,
 * with the export job ID as the HKDF info string. The DEK is never
 * written to disk; only the KEK reference (kek_id) and the key version
 * are stored in the file header.
 *
 * File format written by ExportEncryption::encryptFile():
 *   - Magic:       4 bytes  "TENC"
 *   - Version:     4 bytes  (little-endian uint32, currently 1)
 *   - job_id_len:  4 bytes  (little-endian uint32)
 *   - job_id:      job_id_len bytes
 *   - kek_id_len:  4 bytes  (little-endian uint32)
 *   - kek_id:      kek_id_len bytes
 *   - key_version: 4 bytes  (little-endian uint32)
 *   - iv:          12 bytes (AES-GCM nonce)
 *   - ct_len:      8 bytes  (little-endian uint64, length of ciphertext)
 *   - ciphertext:  ct_len bytes
 *   - tag:         16 bytes (AES-GCM authentication tag)
 *
 * The additional authenticated data (AAD) passed to AES-GCM covers
 * the header fields up to and including the iv so that any tampering
 * with the header is detected during decryption.
 */
struct ExportEncryptionConfig {
    /// Enable encryption. When false, ExportEncryption is a no-op.
    bool enabled = false;

    /// Logical key ID of the key-encryption-key in the KeyProvider.
    /// The raw key material is fetched at encrypt/decrypt time and is
    /// never stored or logged.
    std::string kek_id;

    /// Unique identifier for this export job. Used as HKDF info when
    /// deriving the per-job DEK, and as AAD for the GCM cipher so that
    /// decryption failures are attributable to a specific job.
    std::string job_id;

    /// Key provider that holds the KEK. Must not be null when enabled == true.
    std::shared_ptr<themis::KeyProvider> key_provider;
};

/**
 * @brief AES-256-GCM file-level encryption for export output.
 *
 * ExportEncryption derives a per-job DEK from the KEK using HKDF-SHA256,
 * then encrypts the entire plaintext payload with AES-256-GCM in a
 * single pass (for files that fit comfortably in memory) and writes a
 * self-describing file header so that decryptFile() can locate all
 * parameters without external state.
 *
 * Thread Safety: instances are not thread-safe; create one instance
 * per export job.
 *
 * Security Properties:
 * - Key derivation: HKDF-SHA256 (RFC 5869), job_id as info string
 * - Cipher: AES-256-GCM (NIST SP 800-38D)
 * - IV: 12 bytes, randomly generated per call to encryptFile()
 * - Tag: 16 bytes (full GCM authentication tag)
 * - AAD: serialized header fields (job_id, kek_id, key_version, iv)
 * - Raw key material is never written to disk or logged
 */
class ExportEncryption {
public:
    explicit ExportEncryption(const ExportEncryptionConfig& config);

    /**
     * @brief Encrypt src_path and write the result to dst_path.
     *
     * Reads the entire contents of src_path into memory, derives a
     * per-job DEK, encrypts with AES-256-GCM, and writes the
     * self-describing encrypted file to dst_path. The source file is
     * left unchanged.
     *
     * @param src_path  Path to the plaintext export file.
     * @param dst_path  Destination path for the encrypted file.
     * @throws std::runtime_error on I/O or encryption failure.
     * @throws std::invalid_argument if the config is incomplete.
     */
    void encryptFile(const std::string& src_path,
                     const std::string& dst_path) const;

    /**
     * @brief Decrypt src_path and write the plaintext to dst_path.
     *
     * Reads the encrypted file at src_path, verifies the header,
     * re-derives the DEK, and decrypts with AES-256-GCM (including
     * AAD verification). Writes plaintext to dst_path.
     *
     * @param src_path  Path to the encrypted export file.
     * @param dst_path  Destination path for the decrypted plaintext.
     * @throws std::runtime_error on I/O, decryption, or authentication failure.
     */
    void decryptFile(const std::string& src_path,
                     const std::string& dst_path) const;

    /**
     * @brief Encrypt a plaintext buffer; returns the encrypted file bytes.
     *
     * Convenience overload that works entirely in memory (no file I/O).
     *
     * @param plaintext  Bytes to encrypt.
     * @return Encrypted container bytes (same format as encryptFile()).
     */
    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& plaintext) const;

    /**
     * @brief Decrypt an encrypted container buffer; returns the plaintext.
     *
     * Convenience overload that works entirely in memory (no file I/O).
     *
     * @param container  Encrypted container bytes produced by encrypt().
     * @return Decrypted plaintext bytes.
     * @throws std::runtime_error on authentication or decryption failure.
     */
    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& container) const;

private:
    ExportEncryptionConfig config_;

    /// Derive the 32-byte per-job DEK from the KEK using HKDF-SHA256.
    /// kek_id and job_id are sourced from config_; key_version selects
    /// the specific KEK version to use.
    std::vector<uint8_t> deriveJobDEK(uint32_t key_version) const;

    /// Build the AAD buffer from the header fields that must be
    /// authenticated. The layout mirrors the on-disk header prefix.
    static std::vector<uint8_t> buildAAD(const std::string& job_id,
                                         const std::string& kek_id,
                                         uint32_t key_version,
                                         const std::vector<uint8_t>& iv);
};

} // namespace themis::exporters
