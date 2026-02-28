/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            export_encryption.cpp                              ║
  Version:         0.0.35                                             ║
  Last Modified:   2026-02-28                                         ║
  Author:          copilot-swe-agent[bot]                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "exporters/export_encryption.h"
#include "exporters/exporter_errors.h"
#include "utils/hkdf_helper.h"
#include "utils/logger.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace themis::exporters {

// ─────────────────────────────────────────────────────────────────────────────
// Helper: little-endian binary I/O
// ─────────────────────────────────────────────────────────────────────────────

static void writeU8(std::ostream& out, uint8_t v) {
    out.put(static_cast<char>(v));
}

static void writeU16LE(std::ostream& out, uint16_t v) {
    out.put(static_cast<char>(v & 0xFFU));
    out.put(static_cast<char>((v >> 8U) & 0xFFU));
}

static void writeU32LE(std::ostream& out, uint32_t v) {
    out.put(static_cast<char>(v & 0xFFU));
    out.put(static_cast<char>((v >> 8U) & 0xFFU));
    out.put(static_cast<char>((v >> 16U) & 0xFFU));
    out.put(static_cast<char>((v >> 24U) & 0xFFU));
}

static bool readU8(std::istream& in, uint8_t& v) {
    int c = in.get();
    if (c == EOF) return false;
    v = static_cast<uint8_t>(c);
    return true;
}

static bool readU16LE(std::istream& in, uint16_t& v) {
    int lo = in.get(), hi = in.get();
    if (lo == EOF || hi == EOF) return false;
    v = static_cast<uint16_t>(static_cast<uint8_t>(lo) |
                               (static_cast<uint16_t>(static_cast<uint8_t>(hi)) << 8U));
    return true;
}

static bool readU32LE(std::istream& in, uint32_t& v) {
    uint8_t b[4];
    for (int i = 0; i < 4; ++i) {
        int c = in.get();
        if (c == EOF) return false;
        b[i] = static_cast<uint8_t>(c);
    }
    v = static_cast<uint32_t>(b[0]) |
        (static_cast<uint32_t>(b[1]) << 8U) |
        (static_cast<uint32_t>(b[2]) << 16U) |
        (static_cast<uint32_t>(b[3]) << 24U);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// ExportEncryptor: construction
// ─────────────────────────────────────────────────────────────────────────────

ExportEncryptor::ExportEncryptor(const ExportEncryptionConfig& config)
    : config_(config) {}

// ─────────────────────────────────────────────────────────────────────────────
// ExportEncryptor: private helpers
// ─────────────────────────────────────────────────────────────────────────────

std::vector<uint8_t> ExportEncryptor::deriveDataKey(const std::vector<uint8_t>& kek,
                                                     const std::string& job_id) {
    const std::string info = "themis-export-dek:" + job_id;
    return themis::utils::HKDFHelper::derive(kek, {}, info, 32);
}

std::string ExportEncryptor::generateJobId() {
    uint8_t buf[8];
    if (RAND_bytes(buf, static_cast<int>(sizeof(buf))) != 1) {
        throw EncryptionException("Failed to generate random job ID");
    }
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (uint8_t b : buf) {
        oss << std::setw(2) << static_cast<int>(b);
    }
    return oss.str();
}

size_t ExportEncryptor::writeHeader(std::ostream& out,
                                     const std::string& kek_id,
                                     uint32_t kek_version,
                                     const std::string& job_id,
                                     const std::vector<uint8_t>& iv) {
    size_t bytes = 0;

    // magic (4 bytes)
    out.write(kMagic, 4);
    bytes += 4;

    // version (1 byte)
    writeU8(out, kFormatVersion);
    bytes += 1;

    // kek_id: length (2 bytes) + bytes
    const auto kek_id_len = static_cast<uint16_t>(kek_id.size());
    writeU16LE(out, kek_id_len);
    out.write(kek_id.data(), kek_id_len);
    bytes += 2 + kek_id_len;

    // kek_version (4 bytes)
    writeU32LE(out, kek_version);
    bytes += 4;

    // job_id: length (2 bytes) + bytes
    const auto job_id_len = static_cast<uint16_t>(job_id.size());
    writeU16LE(out, job_id_len);
    out.write(job_id.data(), job_id_len);
    bytes += 2 + job_id_len;

    // IV (12 bytes)
    out.write(reinterpret_cast<const char*>(iv.data()), 12);
    bytes += 12;

    return bytes;
}

bool ExportEncryptor::readHeader(std::istream& in,
                                  std::string& kek_id,
                                  uint32_t& kek_version,
                                  std::string& job_id,
                                  std::vector<uint8_t>& iv) {
    // magic
    char magic[4];
    in.read(magic, 4);
    if (in.gcount() != 4) return false;
    if (std::memcmp(magic, kMagic, 4) != 0) return false;

    // version
    uint8_t version = 0;
    if (!readU8(in, version)) return false;
    if (version != kFormatVersion) return false;

    // kek_id
    uint16_t kek_id_len = 0;
    if (!readU16LE(in, kek_id_len)) return false;
    kek_id.resize(kek_id_len);
    if (kek_id_len > 0) {
        in.read(kek_id.data(), kek_id_len);
        if (in.gcount() != kek_id_len) return false;
    }

    // kek_version
    if (!readU32LE(in, kek_version)) return false;

    // job_id
    uint16_t job_id_len = 0;
    if (!readU16LE(in, job_id_len)) return false;
    job_id.resize(job_id_len);
    if (job_id_len > 0) {
        in.read(job_id.data(), job_id_len);
        if (in.gcount() != job_id_len) return false;
    }

    // IV (12 bytes)
    iv.resize(12);
    in.read(reinterpret_cast<char*>(iv.data()), 12);
    if (in.gcount() != 12) return false;

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// ExportEncryptor::encryptFile
// ─────────────────────────────────────────────────────────────────────────────

size_t ExportEncryptor::encryptFile(const std::string& input_path,
                                     const std::string& output_path) const {
    if (config_.empty()) {
        throw EncryptionException("Encryption not configured: kek_id or key_provider is missing");
    }

    // ── 1. Retrieve KEK ───────────────────────────────────────────────────
    std::vector<uint8_t> kek;
    uint32_t kek_version = 0;
    try {
        kek = config_.key_provider->getKey(config_.kek_id);
        kek_version = config_.key_provider->getKeyMetadata(config_.kek_id).version;
    } catch (const std::exception& e) {
        throw EncryptionException(
            std::string("Failed to retrieve KEK '") + config_.kek_id + "': " + e.what());
    }

    // ── 2. Determine job ID ───────────────────────────────────────────────
    const std::string effective_job_id =
        config_.job_id.empty() ? generateJobId() : config_.job_id;

    // ── 3. Derive DEK from KEK using HKDF-SHA256 ─────────────────────────
    std::vector<uint8_t> dek;
    try {
        dek = deriveDataKey(kek, effective_job_id);
    } catch (...) {
        std::fill(kek.begin(), kek.end(), uint8_t{0});
        throw;
    }
    // Clear KEK immediately after DEK derivation — raw key never persists
    std::fill(kek.begin(), kek.end(), uint8_t{0});

    THEMIS_INFO("ExportEncryptor: encrypting '{}' with kek_id='{}' job_id='{}'",
                input_path, config_.kek_id, effective_job_id);

    // ── 4. Generate random IV ─────────────────────────────────────────────
    std::vector<uint8_t> iv(12);
    if (RAND_bytes(iv.data(), 12) != 1) {
        std::fill(dek.begin(), dek.end(), uint8_t{0});
        throw EncryptionException("Failed to generate random IV for export encryption");
    }

    // ── 5. Open I/O streams ───────────────────────────────────────────────
    std::ifstream in_f(input_path, std::ios::binary);
    if (!in_f.is_open()) {
        std::fill(dek.begin(), dek.end(), uint8_t{0});
        throw ExportIOException("Cannot open input file for encryption", input_path);
    }

    std::ofstream out_f(output_path, std::ios::binary | std::ios::trunc);
    if (!out_f.is_open()) {
        std::fill(dek.begin(), dek.end(), uint8_t{0});
        throw ExportIOException("Cannot create output file for encryption", output_path);
    }

    // ── 6. Write header ───────────────────────────────────────────────────
    size_t total_bytes = writeHeader(out_f, config_.kek_id, kek_version,
                                      effective_job_id, iv);

    // ── 7. Initialise AES-256-GCM context ─────────────────────────────────
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        std::fill(dek.begin(), dek.end(), uint8_t{0});
        throw EncryptionException("Failed to allocate EVP cipher context");
    }

    auto cleanup_ctx = [&]() {
        EVP_CIPHER_CTX_free(ctx);
        std::fill(dek.begin(), dek.end(), uint8_t{0});
    };

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        cleanup_ctx();
        throw EncryptionException("EVP_EncryptInit_ex (AES-256-GCM) failed");
    }
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr) != 1) {
        cleanup_ctx();
        throw EncryptionException("EVP_CTRL_GCM_SET_IVLEN failed");
    }
    if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, dek.data(), iv.data()) != 1) {
        cleanup_ctx();
        throw EncryptionException("EVP_EncryptInit_ex (key/IV) failed");
    }

    // ── 8. Feed job_id as AAD ─────────────────────────────────────────────
    {
        int aad_out = 0;
        const auto* aad_ptr =
            reinterpret_cast<const unsigned char*>(effective_job_id.data());
        if (EVP_EncryptUpdate(ctx, nullptr, &aad_out, aad_ptr,
                               static_cast<int>(effective_job_id.size())) != 1) {
            cleanup_ctx();
            throw EncryptionException("Failed to set GCM AAD (job_id)");
        }
    }

    // ── 9. Stream-encrypt input ───────────────────────────────────────────
    std::vector<unsigned char> plain_buf(kChunkSize);
    std::vector<unsigned char> cipher_buf(kChunkSize + 16);

    while (in_f.good()) {
        in_f.read(reinterpret_cast<char*>(plain_buf.data()),
                  static_cast<std::streamsize>(kChunkSize));
        const std::streamsize bytes_read = in_f.gcount();
        if (bytes_read == 0) break;

        int ct_len = 0;
        if (EVP_EncryptUpdate(ctx, cipher_buf.data(), &ct_len,
                               plain_buf.data(),
                               static_cast<int>(bytes_read)) != 1) {
            cleanup_ctx();
            throw EncryptionException("EVP_EncryptUpdate failed during streaming");
        }
        if (ct_len > 0) {
            out_f.write(reinterpret_cast<const char*>(cipher_buf.data()), ct_len);
            total_bytes += static_cast<size_t>(ct_len);
        }
    }

    // ── 10. Finalise and write GCM tag ────────────────────────────────────
    int final_len = 0;
    if (EVP_EncryptFinal_ex(ctx, cipher_buf.data(), &final_len) != 1) {
        cleanup_ctx();
        throw EncryptionException("EVP_EncryptFinal_ex failed");
    }
    if (final_len > 0) {
        out_f.write(reinterpret_cast<const char*>(cipher_buf.data()), final_len);
        total_bytes += static_cast<size_t>(final_len);
    }

    uint8_t tag[16];
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag) != 1) {
        cleanup_ctx();
        throw EncryptionException("Failed to retrieve GCM authentication tag");
    }
    out_f.write(reinterpret_cast<const char*>(tag), 16);
    total_bytes += 16;

    EVP_CIPHER_CTX_free(ctx);
    std::fill(dek.begin(), dek.end(), uint8_t{0});

    if (!out_f.good()) {
        throw ExportIOException("Write error while finalising encrypted file", output_path);
    }
    out_f.close();
    in_f.close();

    THEMIS_INFO("ExportEncryptor: encrypted {} bytes -> '{}'", total_bytes, output_path);
    return total_bytes;
}

// ─────────────────────────────────────────────────────────────────────────────
// ExportEncryptor::decryptFile
// ─────────────────────────────────────────────────────────────────────────────

size_t ExportEncryptor::decryptFile(const std::string& input_path,
                                     const std::string& output_path) const {
    if (!config_.key_provider) {
        throw DecryptionException("No key_provider configured for export decryption");
    }

    // ── 1. Open encrypted input ───────────────────────────────────────────
    std::ifstream in_f(input_path, std::ios::binary | std::ios::ate);
    if (!in_f.is_open()) {
        throw ExportIOException("Cannot open encrypted file for decryption", input_path);
    }
    const auto file_size = static_cast<size_t>(in_f.tellg());
    in_f.seekg(0, std::ios::beg);

    // ── 2. Parse header ───────────────────────────────────────────────────
    std::string kek_id, job_id;
    uint32_t kek_version = 0;
    std::vector<uint8_t> iv;

    if (!readHeader(in_f, kek_id, kek_version, job_id, iv)) {
        throw DecryptionException(
            "Failed to parse encrypted file header in '" + input_path +
            "': invalid magic, version, or truncated header");
    }

    const auto header_size = static_cast<size_t>(in_f.tellg());

    // Ciphertext occupies everything between header and 16-byte tag
    if (file_size < header_size + 16) {
        throw DecryptionException(
            "Encrypted file '" + input_path + "' is too small to contain GCM tag");
    }
    const size_t ciphertext_size = file_size - header_size - 16;

    // ── 3. Read GCM tag (last 16 bytes) ──────────────────────────────────
    in_f.seekg(static_cast<std::streamoff>(header_size + ciphertext_size));
    uint8_t expected_tag[16];
    in_f.read(reinterpret_cast<char*>(expected_tag), 16);
    if (in_f.gcount() != 16) {
        throw DecryptionException("Failed to read GCM tag from '" + input_path + "'");
    }

    // Seek back to start of ciphertext
    in_f.seekg(static_cast<std::streamoff>(header_size));

    // ── 4. Retrieve KEK ───────────────────────────────────────────────────
    std::vector<uint8_t> kek;
    try {
        kek = config_.key_provider->getKey(kek_id, kek_version);
    } catch (const std::exception& e) {
        throw DecryptionException(
            std::string("Failed to retrieve KEK '") + kek_id + "' v" +
            std::to_string(kek_version) + ": " + e.what());
    }

    // ── 5. Re-derive DEK ──────────────────────────────────────────────────
    std::vector<uint8_t> dek;
    try {
        dek = deriveDataKey(kek, job_id);
    } catch (...) {
        std::fill(kek.begin(), kek.end(), uint8_t{0});
        throw;
    }
    std::fill(kek.begin(), kek.end(), uint8_t{0});

    THEMIS_INFO("ExportEncryptor: decrypting '{}' kek_id='{}' job_id='{}'",
                input_path, kek_id, job_id);

    // ── 6. Open output ────────────────────────────────────────────────────
    std::ofstream out_f(output_path, std::ios::binary | std::ios::trunc);
    if (!out_f.is_open()) {
        std::fill(dek.begin(), dek.end(), uint8_t{0});
        throw ExportIOException("Cannot create output file for decryption", output_path);
    }

    // ── 7. Initialise AES-256-GCM decrypt context ─────────────────────────
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        std::fill(dek.begin(), dek.end(), uint8_t{0});
        throw DecryptionException("Failed to allocate EVP cipher context");
    }

    auto cleanup_ctx = [&]() {
        EVP_CIPHER_CTX_free(ctx);
        std::fill(dek.begin(), dek.end(), uint8_t{0});
    };

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        cleanup_ctx();
        throw DecryptionException("EVP_DecryptInit_ex (AES-256-GCM) failed");
    }
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr) != 1) {
        cleanup_ctx();
        throw DecryptionException("EVP_CTRL_GCM_SET_IVLEN failed");
    }
    if (EVP_DecryptInit_ex(ctx, nullptr, nullptr, dek.data(), iv.data()) != 1) {
        cleanup_ctx();
        throw DecryptionException("EVP_DecryptInit_ex (key/IV) failed");
    }

    // ── 8. Feed job_id as AAD ─────────────────────────────────────────────
    {
        int aad_out = 0;
        const auto* aad_ptr = reinterpret_cast<const unsigned char*>(job_id.data());
        if (EVP_DecryptUpdate(ctx, nullptr, &aad_out, aad_ptr,
                               static_cast<int>(job_id.size())) != 1) {
            cleanup_ctx();
            throw DecryptionException("Failed to set GCM AAD (job_id)");
        }
    }

    // ── 9. Stream-decrypt ciphertext ──────────────────────────────────────
    std::vector<unsigned char> cipher_buf(kChunkSize);
    std::vector<unsigned char> plain_buf(kChunkSize + 16);
    size_t remaining = ciphertext_size;
    size_t total_plain_bytes = 0;

    while (remaining > 0) {
        const size_t to_read = std::min(remaining, kChunkSize);
        in_f.read(reinterpret_cast<char*>(cipher_buf.data()),
                  static_cast<std::streamsize>(to_read));
        const size_t bytes_read = static_cast<size_t>(in_f.gcount());
        if (bytes_read == 0) break;
        remaining -= bytes_read;

        int pt_len = 0;
        if (EVP_DecryptUpdate(ctx, plain_buf.data(), &pt_len,
                               cipher_buf.data(),
                               static_cast<int>(bytes_read)) != 1) {
            cleanup_ctx();
            throw DecryptionException("EVP_DecryptUpdate failed during streaming");
        }
        if (pt_len > 0) {
            out_f.write(reinterpret_cast<const char*>(plain_buf.data()), pt_len);
            total_plain_bytes += static_cast<size_t>(pt_len);
        }
    }

    // ── 10. Set expected tag and finalise (verifies GCM authentication) ───
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, expected_tag) != 1) {
        cleanup_ctx();
        throw DecryptionException("Failed to set expected GCM tag");
    }

    int final_len = 0;
    if (EVP_DecryptFinal_ex(ctx, plain_buf.data(), &final_len) != 1) {
        // Authentication tag mismatch — ciphertext has been tampered with
        EVP_CIPHER_CTX_free(ctx);
        std::fill(dek.begin(), dek.end(), uint8_t{0});
        // Remove partially-written output to avoid leaving plaintext on disk
        out_f.close();
        std::error_code ec;
        std::filesystem::remove(output_path, ec);
        throw DecryptionException(
            "GCM authentication tag verification failed for '" + input_path +
            "': ciphertext integrity check failed (job_id='" + job_id + "')");
    }
    if (final_len > 0) {
        out_f.write(reinterpret_cast<const char*>(plain_buf.data()), final_len);
        total_plain_bytes += static_cast<size_t>(final_len);
    }

    EVP_CIPHER_CTX_free(ctx);
    std::fill(dek.begin(), dek.end(), uint8_t{0});

    if (!out_f.good()) {
        throw ExportIOException("Write error while finalising decrypted file", output_path);
    }
    out_f.close();
    in_f.close();

    THEMIS_INFO("ExportEncryptor: decrypted {} bytes -> '{}'", total_plain_bytes, output_path);
    return total_plain_bytes;
}

} // namespace themis::exporters
