/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            export_encryption.cpp                              ║
  Version:         0.0.34                                             ║
  Last Modified:   2026-02-28                                         ║
  Author:          copilot-swe-agent[bot]                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "exporters/export_encryption.h"
#include "utils/hkdf_helper.h"
#include "utils/logger.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace themis::exporters {

// ─────────────────────────────────────────────────────────────────────────────
// File-format constants
// ─────────────────────────────────────────────────────────────────────────────

static constexpr uint8_t  MAGIC[4]   = {'T', 'E', 'N', 'C'};
static constexpr uint32_t FORMAT_VER = 1u;
static constexpr size_t   IV_LEN     = 12u; // AES-GCM recommended nonce size
static constexpr size_t   TAG_LEN    = 16u; // Full GCM authentication tag
static constexpr size_t   KEY_LEN    = 32u; // AES-256

// ─────────────────────────────────────────────────────────────────────────────
// Little-endian I/O helpers
// ─────────────────────────────────────────────────────────────────────────────

static void writeU32(std::vector<uint8_t>& buf, uint32_t v) {
    buf.push_back(static_cast<uint8_t>(v & 0xFFu));
    buf.push_back(static_cast<uint8_t>((v >> 8)  & 0xFFu));
    buf.push_back(static_cast<uint8_t>((v >> 16) & 0xFFu));
    buf.push_back(static_cast<uint8_t>((v >> 24) & 0xFFu));
}

static void writeU64(std::vector<uint8_t>& buf, uint64_t v) {
    for (int i = 0; i < 8; ++i) {
        buf.push_back(static_cast<uint8_t>((v >> (i * 8)) & 0xFFu));
    }
}

static void writeBytes(std::vector<uint8_t>& buf,
                       const uint8_t* data, size_t len) {
    buf.insert(buf.end(), data, data + len);
}

static void writeString(std::vector<uint8_t>& buf, const std::string& s) {
    writeU32(buf, static_cast<uint32_t>(s.size()));
    writeBytes(buf, reinterpret_cast<const uint8_t*>(s.data()), s.size());
}

static uint32_t readU32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0])
         | (static_cast<uint32_t>(p[1]) << 8)
         | (static_cast<uint32_t>(p[2]) << 16)
         | (static_cast<uint32_t>(p[3]) << 24);
}

static uint64_t readU64(const uint8_t* p) {
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) {
        v |= (static_cast<uint64_t>(p[i]) << (i * 8));
    }
    return v;
}

// Maximum length accepted for job_id and kek_id strings in the TENC header.
// Prevents allocation of enormous strings from malformed or malicious files.
static constexpr uint32_t MAX_HEADER_STRING_LEN = 4096u;

static std::string readString(const uint8_t* buf, size_t buf_size,
                               size_t& offset) {
    if (offset + 4 > buf_size) {
        throw std::runtime_error("ExportEncryption: truncated string length field");
    }
    uint32_t len = readU32(buf + offset);
    offset += 4;
    if (len > MAX_HEADER_STRING_LEN) {
        throw std::runtime_error(
            "ExportEncryption: header string exceeds maximum length ("
            + std::to_string(len) + " > "
            + std::to_string(MAX_HEADER_STRING_LEN) + ")");
    }
    if (offset + len > buf_size) {
        throw std::runtime_error("ExportEncryption: truncated string data");
    }
    std::string s(reinterpret_cast<const char*>(buf + offset), len);
    offset += len;
    return s;
}

// ─────────────────────────────────────────────────────────────────────────────
// ExportEncryption
// ─────────────────────────────────────────────────────────────────────────────

ExportEncryption::ExportEncryption(const ExportEncryptionConfig& config)
    : config_(config) {}

// Build the AAD buffer authenticated by the GCM tag.
// Covers job_id, kek_id, key_version, and IV so that any header
// tampering is detected at decryption time.
/*static*/
std::vector<uint8_t> ExportEncryption::buildAAD(const std::string& job_id,
                                                  const std::string& kek_id,
                                                  uint32_t key_version,
                                                  const std::vector<uint8_t>& iv) {
    std::vector<uint8_t> aad;
    aad.reserve(8 + job_id.size() + 4 + kek_id.size() + 4 + IV_LEN);
    writeString(aad, job_id);
    writeString(aad, kek_id);
    writeU32(aad, key_version);
    writeBytes(aad, iv.data(), iv.size());
    return aad;
}

// Derive a 32-byte per-job DEK using HKDF-SHA256.
// The IKM is the raw KEK bytes from the KeyProvider; the info string
// is the export job_id so each job produces a distinct key.
std::vector<uint8_t>
ExportEncryption::deriveJobDEK(uint32_t key_version) const {
    if (!config_.key_provider) {
        throw std::invalid_argument(
            "ExportEncryption: key_provider is null");
    }
    if (config_.kek_id.empty()) {
        throw std::invalid_argument(
            "ExportEncryption: kek_id is empty");
    }

    // Fetch KEK – referenced by ID only; raw bytes are never logged.
    auto kek = config_.key_provider->getKey(config_.kek_id, key_version);
    if (kek.size() != KEY_LEN) {
        throw std::runtime_error(
            "ExportEncryption: KEK must be 32 bytes (AES-256)");
    }

    // HKDF-SHA256: IKM = KEK, info = job_id, no explicit salt
    std::vector<uint8_t> dek = themis::utils::HKDFHelper::derive(
        kek,
        {} /*salt*/,
        config_.job_id,
        KEY_LEN);

    // Securely zero the KEK copy held on the stack once DEK is derived.
    OPENSSL_cleanse(kek.data(), kek.size());

    return dek;
}

std::vector<uint8_t>
ExportEncryption::encrypt(const std::vector<uint8_t>& plaintext) const {
    if (!config_.enabled) {
        return plaintext;
    }
    if (config_.kek_id.empty() || !config_.key_provider) {
        throw std::invalid_argument(
            "ExportEncryption: encryption enabled but kek_id/key_provider missing");
    }
    if (config_.job_id.empty()) {
        throw std::invalid_argument(
            "ExportEncryption: job_id must not be empty");
    }

    // Fetch key metadata to record the version we used.
    auto meta = config_.key_provider->getKeyMetadata(config_.kek_id);
    const uint32_t key_version = meta.version;

    // Derive per-job DEK.
    auto dek = deriveJobDEK(key_version);

    // Generate a random 12-byte IV.
    std::vector<uint8_t> iv(IV_LEN);
    if (RAND_bytes(iv.data(), static_cast<int>(IV_LEN)) != 1) {
        OPENSSL_cleanse(dek.data(), dek.size());
        throw std::runtime_error(
            "ExportEncryption: failed to generate random IV");
    }

    // Build AAD.
    auto aad = buildAAD(config_.job_id, config_.kek_id, key_version, iv);

    // AES-256-GCM encrypt.
    std::vector<uint8_t> ciphertext(plaintext.size());
    std::vector<uint8_t> tag(TAG_LEN);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        OPENSSL_cleanse(dek.data(), dek.size());
        throw std::runtime_error(
            "ExportEncryption: failed to create cipher context");
    }

    try {
        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(),
                               nullptr, nullptr, nullptr) != 1) {
            throw std::runtime_error("ExportEncryption: EncryptInit failed");
        }
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN,
                                 static_cast<int>(IV_LEN), nullptr) != 1) {
            throw std::runtime_error("ExportEncryption: set IV length failed");
        }
        if (EVP_EncryptInit_ex(ctx, nullptr, nullptr,
                               dek.data(), iv.data()) != 1) {
            throw std::runtime_error("ExportEncryption: set key/IV failed");
        }

        // Feed AAD.
        int out_len = 0;
        if (!aad.empty() &&
            EVP_EncryptUpdate(ctx, nullptr, &out_len,
                              aad.data(),
                              static_cast<int>(aad.size())) != 1) {
            throw std::runtime_error("ExportEncryption: AAD feed failed");
        }

        // Encrypt plaintext.
        if (!plaintext.empty()) {
            // Guard against truncation in the OpenSSL int API
            if (plaintext.size() > static_cast<size_t>(INT_MAX)) {
                throw std::runtime_error(
                    "ExportEncryption: plaintext exceeds maximum supported size (INT_MAX)");
            }
            if (EVP_EncryptUpdate(ctx, ciphertext.data(), &out_len,
                                  plaintext.data(),
                                  static_cast<int>(plaintext.size())) != 1) {
                throw std::runtime_error("ExportEncryption: EncryptUpdate failed");
            }
        }
        int final_len = 0;
        if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + out_len,
                                &final_len) != 1) {
            throw std::runtime_error("ExportEncryption: EncryptFinal failed");
        }
        // For GCM with no padding, final_len is always 0.

        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG,
                                 static_cast<int>(TAG_LEN),
                                 tag.data()) != 1) {
            throw std::runtime_error("ExportEncryption: get tag failed");
        }
    } catch (...) {
        EVP_CIPHER_CTX_free(ctx);
        OPENSSL_cleanse(dek.data(), dek.size());
        throw;
    }
    EVP_CIPHER_CTX_free(ctx);
    OPENSSL_cleanse(dek.data(), dek.size());

    // Assemble the container:
    //   [magic][format_ver][job_id][kek_id][key_version][iv][ct_len][ct][tag]
    std::vector<uint8_t> container;
    container.reserve(4 + 4
        + 4 + config_.job_id.size()
        + 4 + config_.kek_id.size()
        + 4 + IV_LEN + 8
        + plaintext.size() + TAG_LEN);

    writeBytes(container, MAGIC, 4);
    writeU32(container, FORMAT_VER);
    writeString(container, config_.job_id);
    writeString(container, config_.kek_id);
    writeU32(container, key_version);
    writeBytes(container, iv.data(), IV_LEN);
    writeU64(container, static_cast<uint64_t>(ciphertext.size()));
    writeBytes(container, ciphertext.data(), ciphertext.size());
    writeBytes(container, tag.data(), TAG_LEN);

    THEMIS_INFO(
        "ExportEncryption: encrypted {} bytes -> {} bytes "
        "(job_id={}, kek_id={}, key_ver={})",
        plaintext.size(), container.size(),
        config_.job_id, config_.kek_id, key_version);

    return container;
}

std::vector<uint8_t>
ExportEncryption::decrypt(const std::vector<uint8_t>& container) const {
    if (!config_.enabled) {
        return container;
    }

    // ── Parse header ──────────────────────────────────────────────────────
    size_t offset = 0;
    const size_t total = container.size();
    const uint8_t* buf = container.data();

    // Magic
    if (total < 4 || std::memcmp(buf, MAGIC, 4) != 0) {
        throw std::runtime_error(
            "ExportEncryption: invalid magic; not a TENC file");
    }
    offset += 4;

    // Format version
    if (offset + 4 > total) {
        throw std::runtime_error("ExportEncryption: truncated header (version)");
    }
    const uint32_t ver = readU32(buf + offset);
    offset += 4;
    if (ver != FORMAT_VER) {
        throw std::runtime_error(
            "ExportEncryption: unsupported file format version " +
            std::to_string(ver));
    }

    // job_id
    std::string file_job_id = readString(buf, total, offset);

    // kek_id
    std::string file_kek_id = readString(buf, total, offset);

    // key_version
    if (offset + 4 > total) {
        throw std::runtime_error("ExportEncryption: truncated header (key_version)");
    }
    const uint32_t key_version = readU32(buf + offset);
    offset += 4;

    // IV
    if (offset + IV_LEN > total) {
        throw std::runtime_error("ExportEncryption: truncated header (iv)");
    }
    std::vector<uint8_t> iv(buf + offset, buf + offset + IV_LEN);
    offset += IV_LEN;

    // ciphertext length
    if (offset + 8 > total) {
        throw std::runtime_error("ExportEncryption: truncated header (ct_len)");
    }
    const uint64_t ct_len = readU64(buf + offset);
    offset += 8;

    // ciphertext
    if (offset + ct_len > total) {
        throw std::runtime_error("ExportEncryption: truncated ciphertext");
    }
    const uint8_t* ct_ptr = buf + offset;
    offset += ct_len;

    // tag
    if (offset + TAG_LEN > total) {
        throw std::runtime_error("ExportEncryption: truncated authentication tag");
    }
    std::vector<uint8_t> tag(buf + offset, buf + offset + TAG_LEN);

    // ── Derive DEK ────────────────────────────────────────────────────────
    // Temporarily override the config job_id / kek_id with values read
    // from the file header so that the DEK derivation matches encryption.
    if (!config_.key_provider) {
        throw std::invalid_argument(
            "ExportEncryption: key_provider is null");
    }
    if (file_kek_id.empty()) {
        throw std::runtime_error("ExportEncryption: file header has empty kek_id");
    }

    // Use the job_id from the file; fall back to config_.job_id if the
    // header job_id matches (normal case) or the caller deliberately sets
    // it. Build a temporary config just for derivation.
    ExportEncryptionConfig dec_cfg = config_;
    dec_cfg.kek_id = file_kek_id;
    dec_cfg.job_id = file_job_id;
    ExportEncryption dec_helper(dec_cfg);
    auto dek = dec_helper.deriveJobDEK(key_version);

    // ── Build AAD ─────────────────────────────────────────────────────────
    auto aad = buildAAD(file_job_id, file_kek_id, key_version, iv);

    // ── AES-256-GCM decrypt ───────────────────────────────────────────────
    std::vector<uint8_t> plaintext(static_cast<size_t>(ct_len));

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        OPENSSL_cleanse(dek.data(), dek.size());
        throw std::runtime_error(
            "ExportEncryption: failed to create cipher context");
    }

    try {
        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(),
                               nullptr, nullptr, nullptr) != 1) {
            throw std::runtime_error("ExportEncryption: DecryptInit failed");
        }
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN,
                                 static_cast<int>(IV_LEN), nullptr) != 1) {
            throw std::runtime_error("ExportEncryption: set IV length failed");
        }
        if (EVP_DecryptInit_ex(ctx, nullptr, nullptr,
                               dek.data(), iv.data()) != 1) {
            throw std::runtime_error("ExportEncryption: set key/IV failed");
        }

        // Feed AAD.
        int out_len = 0;
        if (!aad.empty() &&
            EVP_DecryptUpdate(ctx, nullptr, &out_len,
                              aad.data(),
                              static_cast<int>(aad.size())) != 1) {
            throw std::runtime_error("ExportEncryption: AAD feed failed");
        }

        // Decrypt ciphertext.
        if (ct_len > 0) {
            // Guard against truncation in the OpenSSL int API
            if (ct_len > static_cast<uint64_t>(INT_MAX)) {
                throw std::runtime_error(
                    "ExportEncryption: ciphertext exceeds maximum supported size (INT_MAX)");
            }
            if (EVP_DecryptUpdate(ctx, plaintext.data(), &out_len,
                                  ct_ptr,
                                  static_cast<int>(ct_len)) != 1) {
                throw std::runtime_error("ExportEncryption: DecryptUpdate failed");
            }
        }

        // Set expected tag before finalising.
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG,
                                 static_cast<int>(TAG_LEN),
                                 const_cast<uint8_t*>(tag.data())) != 1) {
            throw std::runtime_error("ExportEncryption: set tag failed");
        }

        // Verify tag and finalise.
        int final_len = 0;
        if (EVP_DecryptFinal_ex(ctx, plaintext.data() + out_len,
                                &final_len) <= 0) {
            throw std::runtime_error(
                "ExportEncryption: authentication tag mismatch – "
                "file may be corrupted or tampered with");
        }
    } catch (...) {
        EVP_CIPHER_CTX_free(ctx);
        OPENSSL_cleanse(dek.data(), dek.size());
        throw;
    }
    EVP_CIPHER_CTX_free(ctx);
    OPENSSL_cleanse(dek.data(), dek.size());

    THEMIS_INFO(
        "ExportEncryption: decrypted {} bytes -> {} bytes "
        "(job_id={}, kek_id={}, key_ver={})",
        container.size(), plaintext.size(),
        file_job_id, file_kek_id, key_version);

    return plaintext;
}

void ExportEncryption::encryptFile(const std::string& src_path,
                                   const std::string& dst_path) const {
    if (!config_.enabled) {
        // Copy the file as-is when encryption is disabled.
        if (src_path != dst_path) {
            std::ifstream src(src_path, std::ios::binary);
            if (!src.is_open()) {
                throw std::runtime_error(
                    "ExportEncryption: cannot open source file: " + src_path);
            }
            std::ofstream dst(dst_path, std::ios::binary | std::ios::trunc);
            if (!dst.is_open()) {
                throw std::runtime_error(
                    "ExportEncryption: cannot open destination file: " +
                    dst_path);
            }
            dst << src.rdbuf();
        }
        return;
    }

    // Read plaintext.
    std::ifstream src(src_path, std::ios::binary | std::ios::ate);
    if (!src.is_open()) {
        throw std::runtime_error(
            "ExportEncryption: cannot open source file: " + src_path);
    }
    const auto file_size = static_cast<size_t>(src.tellg());
    src.seekg(0);
    std::vector<uint8_t> plaintext(file_size);
    if (file_size > 0 &&
        !src.read(reinterpret_cast<char*>(plaintext.data()),
                  static_cast<std::streamsize>(file_size))) {
        throw std::runtime_error(
            "ExportEncryption: failed to read source file: " + src_path);
    }
    src.close();

    // Encrypt.
    auto container = encrypt(plaintext);

    // Securely zero the plaintext buffer now that encryption is done.
    OPENSSL_cleanse(plaintext.data(), plaintext.size());

    // Write encrypted container.
    std::ofstream dst(dst_path, std::ios::binary | std::ios::trunc);
    if (!dst.is_open()) {
        throw std::runtime_error(
            "ExportEncryption: cannot open destination file: " + dst_path);
    }
    if (!container.empty() &&
        !dst.write(reinterpret_cast<const char*>(container.data()),
                   static_cast<std::streamsize>(container.size()))) {
        throw std::runtime_error(
            "ExportEncryption: failed to write encrypted file: " + dst_path);
    }
}

void ExportEncryption::decryptFile(const std::string& src_path,
                                   const std::string& dst_path) const {
    if (!config_.enabled) {
        if (src_path != dst_path) {
            std::ifstream src(src_path, std::ios::binary);
            if (!src.is_open()) {
                throw std::runtime_error(
                    "ExportEncryption: cannot open source file: " + src_path);
            }
            std::ofstream dst(dst_path, std::ios::binary | std::ios::trunc);
            if (!dst.is_open()) {
                throw std::runtime_error(
                    "ExportEncryption: cannot open destination file: " +
                    dst_path);
            }
            dst << src.rdbuf();
        }
        return;
    }

    // Read encrypted container.
    std::ifstream src(src_path, std::ios::binary | std::ios::ate);
    if (!src.is_open()) {
        throw std::runtime_error(
            "ExportEncryption: cannot open source file: " + src_path);
    }
    const auto file_size = static_cast<size_t>(src.tellg());
    src.seekg(0);
    std::vector<uint8_t> container(file_size);
    if (file_size > 0 &&
        !src.read(reinterpret_cast<char*>(container.data()),
                  static_cast<std::streamsize>(file_size))) {
        throw std::runtime_error(
            "ExportEncryption: failed to read source file: " + src_path);
    }
    src.close();

    // Decrypt.
    auto plaintext = decrypt(container);

    // Write plaintext.
    std::ofstream dst(dst_path, std::ios::binary | std::ios::trunc);
    if (!dst.is_open()) {
        OPENSSL_cleanse(plaintext.data(), plaintext.size());
        throw std::runtime_error(
            "ExportEncryption: cannot open destination file: " + dst_path);
    }
    if (!plaintext.empty() &&
        !dst.write(reinterpret_cast<const char*>(plaintext.data()),
                   static_cast<std::streamsize>(plaintext.size()))) {
        OPENSSL_cleanse(plaintext.data(), plaintext.size());
        throw std::runtime_error(
            "ExportEncryption: failed to write decrypted file: " + dst_path);
    }

    // Securely zero the decrypted buffer after the file has been written.
    OPENSSL_cleanse(plaintext.data(), plaintext.size());
}

} // namespace themis::exporters
