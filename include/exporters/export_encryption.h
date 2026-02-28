/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            export_encryption.h                                ║
  Version:         0.0.35                                             ║
  Last Modified:   2026-02-28                                         ║
  Author:          copilot-swe-agent[bot]                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "security/encryption.h"
#include "security/key_provider.h"
#include <memory>
#include <string>
#include <vector>

namespace themis::exporters {

/// Configuration for AES-256-GCM export file encryption.
///
/// When kek_id is non-empty and key_provider is set, the export file is
/// encrypted using AES-256-GCM after the export completes.  A per-job
/// data-encryption-key (DEK) is derived from the key-encryption-key (KEK)
/// referenced by kek_id using HKDF-SHA256, ensuring raw key material never
/// appears in logs or output files.
///
/// Decryption requires a KeyProvider that can retrieve the same KEK by kek_id
/// and kek_version recorded in the encrypted file header.
struct ExportEncryptionConfig {
    /// Logical key-encryption-key identifier stored in the KeyProvider.
    /// The raw KEK is never stored on disk; only this identifier is recorded
    /// in the encrypted file header so that the correct key can be retrieved
    /// during decryption.
    std::string kek_id;

    /// Unique export job identifier used as:
    ///   - HKDF info string: "themis-export-dek:<job_id>"
    ///   - GCM Additional Authenticated Data (AAD) so that decryption
    ///     failures are attributable to a specific job.
    /// If empty at encryption time, a random 16-byte hex identifier is
    /// generated and recorded in the output file header.
    std::string job_id;

    /// Key management provider used to retrieve the KEK.
    /// Must be non-null when kek_id is non-empty.
    std::shared_ptr<themis::KeyProvider> key_provider;

    /// Returns true if encryption is not configured (either kek_id is empty
    /// or key_provider is null).
    bool empty() const { return kek_id.empty() || !key_provider; }
};

/// AES-256-GCM streaming export file encryptor / decryptor.
///
/// Encryption workflow (per encryptFile() call):
///   1. Retrieve KEK bytes from the KeyProvider using kek_id.
///   2. Derive DEK = HKDF-SHA256(IKM=KEK, salt={},
///                                info="themis-export-dek:<job_id>", len=32).
///   3. Generate a random 12-byte IV (per-job uniqueness).
///   4. Stream-encrypt the input file with AES-256-GCM; job_id is used as
///      Additional Authenticated Data (AAD) so any decryption failure can be
///      traced to the originating export job.
///   5. Write the self-describing encrypted output:
///        [MAGIC "TMEX"][VERSION][kek_id][kek_version][job_id][IV]
///        [ciphertext][GCM tag (16 bytes)]
///   6. Zero DEK bytes from memory immediately after use.
///
/// Raw KEK material is never written to disk or emitted in any log message.
/// Only key IDs and algorithm identifiers appear in logs.
///
/// Decryption workflow (per decryptFile() call):
///   1. Parse the file header to recover kek_id, kek_version, job_id, IV.
///   2. Retrieve KEK from KeyProvider (using the recorded version).
///   3. Re-derive the same DEK with the same HKDF parameters.
///   4. Stream-decrypt and verify the GCM authentication tag.
///   5. Zero DEK bytes from memory.
class ExportEncryptor {
public:
    /// Binary file format version written in the header.
    static constexpr uint8_t kFormatVersion = 1;

    /// 4-byte magic written at the start of every encrypted export file.
    static constexpr char kMagic[4] = {'T', 'M', 'E', 'X'};

    /// Chunk size used for streaming I/O (64 KiB).
    static constexpr size_t kChunkSize = 65536;

    /// @param config  Encryption configuration (kek_id, job_id, key_provider).
    explicit ExportEncryptor(const ExportEncryptionConfig& config);

    /// Encrypt the plaintext file at @p input_path and write the ciphertext
    /// to @p output_path.
    ///
    /// The two paths may differ (recommended: write to a temp file and rename
    /// into place after success).  The caller is responsible for removing
    /// @p input_path when it is no longer needed.
    ///
    /// @param input_path   Plaintext export file to encrypt.
    /// @param output_path  Destination for the encrypted output.
    /// @returns Total bytes written to @p output_path (header + ciphertext + tag).
    /// @throws themis::EncryptionException on OpenSSL or key errors.
    /// @throws ExportIOException on file I/O errors.
    size_t encryptFile(const std::string& input_path,
                       const std::string& output_path) const;

    /// Decrypt the encrypted export file at @p input_path and write the
    /// recovered plaintext to @p output_path.
    ///
    /// @param input_path   Encrypted file produced by encryptFile().
    /// @param output_path  Destination for the decrypted plaintext.
    /// @returns Total bytes written to @p output_path.
    /// @throws themis::DecryptionException on authentication failure or key error.
    /// @throws ExportIOException on file I/O errors.
    size_t decryptFile(const std::string& input_path,
                       const std::string& output_path) const;

    const ExportEncryptionConfig& getConfig() const { return config_; }

private:
    ExportEncryptionConfig config_;

    /// Derive a 32-byte DEK from @p kek using HKDF-SHA256.
    /// info = "themis-export-dek:" + job_id
    /// The returned vector must be zeroed by the caller after use.
    static std::vector<uint8_t> deriveDataKey(const std::vector<uint8_t>& kek,
                                               const std::string& job_id);

    /// Generate a random hex job ID (16 hex chars = 8 random bytes).
    static std::string generateJobId();

    // ── Header binary layout ───────────────────────────────────────────────
    // Offset  Size  Field
    //   0       4   magic "TMEX"
    //   4       1   format version (uint8)
    //   5       2   kek_id length (uint16 LE)
    //   7       N   kek_id bytes
    //   7+N     4   kek_version (uint32 LE)
    //   11+N    2   job_id length (uint16 LE)
    //   13+N    M   job_id bytes
    //   13+N+M  12  IV (random, 96-bit GCM IV)
    // After header: [ciphertext bytes] [16-byte GCM tag]
    // ──────────────────────────────────────────────────────────────────────

    /// Serialise and write the file header.  Returns total bytes written.
    static size_t writeHeader(std::ostream& out,
                               const std::string& kek_id,
                               uint32_t kek_version,
                               const std::string& job_id,
                               const std::vector<uint8_t>& iv);

    /// Parse the file header.  Returns false if magic / version are invalid
    /// or the stream ends prematurely.
    static bool readHeader(std::istream& in,
                            std::string& kek_id,
                            uint32_t& kek_version,
                            std::string& job_id,
                            std::vector<uint8_t>& iv);
};

} // namespace themis::exporters
