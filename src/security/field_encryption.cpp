// THEMIS_GAP_STATS: gaps=1 unimpl=0 stub=1 mock=0 sim=0 todo=0 debt=0 scanned=2026-05-18
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            field_encryption.cpp                               ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:50:42                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   94.0/100                                       ║
    • Total Lines:     712                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "security/encryption.h"
#include "security/mock_key_provider.h"
#include "themis/runtime_license_gate.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <cstring>
#include <sstream>
#include <iomanip>
#include "utils/hkdf_cache.h"
#include "utils/logger.h"
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <fstream>
#include <filesystem>
#include <chrono>

namespace themis {

// [E-1] key parameter removed: raw key bytes must never be passed into debug utilities.
// SECURITY: THEMIS_DEBUG_ENC_DIR must NEVER be set in production — it writes ciphertext blobs
// (IV, tag, ciphertext) to disk in plaintext JSON.  Enforce absence of this variable via
// your deployment's environment guard or startup validation.
static void write_debug_dump(const std::string& prefix, const EncryptedBlob& blob, bool success) {
    try {
        namespace fs = std::filesystem;

        // Only write debug dumps when user explicitly sets THEMIS_DEBUG_ENC_DIR
        const char* env_dir = std::getenv("THEMIS_DEBUG_ENC_DIR");
        if (!env_dir || !*env_dir) return; // disabled by default

        fs::path dir = fs::path(env_dir);

        try {
            fs::create_directories(dir);
        } catch (const std::exception& e) {
            fprintf(stderr, "write_debug_dump: failed to create directory '%s': %s\n", dir.string().c_str(), e.what());
            return;
        }

        // timestamp
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

        nlohmann::json j = blob.toJson();
        // [E-1] Do NOT include key material in debug dumps.
        // key_fingerprint_prefix was removed because it embeds the first 8 bytes
        // of the raw encryption key in a plain-text on-disk file, violating the
        // principle of minimum key exposure.  Use key_id/key_version (already
        // present via blob.toJson()) to correlate dumps with the key registry.
        j["success"] = success;
        j["ts_ms"] = ms;

        fs::path file = dir / (prefix + "_" + std::to_string(ms) + ".json");
        std::ofstream ofs(file.string());
        if (ofs.is_open()) {
            ofs << j.dump(2) << std::endl;
            fprintf(stderr, "write_debug_dump: wrote '%s'\n", file.string().c_str());
        } else {
            fprintf(stderr, "write_debug_dump: failed to open '%s' for writing\n", file.string().c_str());
        }
    } catch (const std::exception& e) {
        fprintf(stderr, "write_debug_dump: exception: %s\n", e.what());
    } catch (...) {
        fprintf(stderr, "write_debug_dump: unknown exception\n");
    }
}

// ===== Base64 Encoding/Decoding Helpers =====

static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

static std::string fieldBase64Encode(const std::vector<uint8_t>& data) {
    std::string ret;
    int i = 0;
    int j = 0;
    uint8_t char_array_3[3];
    uint8_t char_array_4[4];
    size_t in_len = data.size();
    const uint8_t* bytes_to_encode = data.data();

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; i < 4; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

        for (j = 0; j < i + 1; j++)
            ret += base64_chars[char_array_4[j]];

        while (i++ < 3)
            ret += '=';
    }

    return ret;
}

static bool is_base64(uint8_t c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

static std::vector<uint8_t> fieldBase64Decode(const std::string& encoded_string) {
    size_t in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    uint8_t char_array_4[4], char_array_3[3];
    std::vector<uint8_t> ret;

    while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
        char_array_4[i++] = encoded_string[in_]; in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = static_cast<uint8_t>(base64_chars.find(char_array_4[i]));

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; i < 3; i++)
                ret.push_back(char_array_3[i]);
            i = 0;
        }
    }

    if (i) {
        for (j = 0; j < i; j++)
            char_array_4[j] = static_cast<uint8_t>(base64_chars.find(char_array_4[j]));

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);

        for (j = 0; j < i - 1; j++)
            ret.push_back(char_array_3[j]);
    }

    return ret;
}

// ===== EncryptedBlob Implementation =====

std::string EncryptedBlob::toBase64() const {
    // Format: key_id:version:base64(iv):base64(ciphertext):base64(tag)
    std::ostringstream oss;
    oss << key_id << ":"
        << key_version << ":"
        << fieldBase64Encode(iv) << ":"
        << fieldBase64Encode(ciphertext) << ":"
        << fieldBase64Encode(tag);
    return oss.str();
}

EncryptedBlob EncryptedBlob::fromBase64(const std::string& b64) {
    EncryptedBlob blob;
    
    // Split by ':'
    std::vector<std::string> parts;
    std::stringstream ss(b64);
    std::string part;
    while (std::getline(ss, part, ':')) {
        parts.push_back(part);
    }
    
    if (parts.size() != 5) {
        throw std::runtime_error("Invalid EncryptedBlob format: expected 5 parts, got " + std::to_string(parts.size()));
    }
    
    blob.key_id = parts[0];
    blob.key_version = std::stoul(parts[1]);
    blob.iv = fieldBase64Decode(parts[2]);
    blob.ciphertext = fieldBase64Decode(parts[3]);
    blob.tag = fieldBase64Decode(parts[4]);
    
    return blob;
}

nlohmann::json EncryptedBlob::toJson() const {
    return nlohmann::json{
        {"key_id", key_id},
        {"key_version", key_version},
        {"iv", fieldBase64Encode(iv)},
        {"ciphertext", fieldBase64Encode(ciphertext)},
        {"tag", fieldBase64Encode(tag)}
    };
}

EncryptedBlob EncryptedBlob::fromJson(const nlohmann::json& j) {
    EncryptedBlob blob;

    if (!j.is_object()) {
        throw std::runtime_error("EncryptedBlob::fromJson: expected JSON object");
    }

    try {
        blob.key_id = j.at("key_id").get<std::string>();
        blob.key_version = j.at("key_version").get<uint32_t>();

        std::string iv_b64 = j.at("iv").get<std::string>();
        std::string ct_b64 = j.at("ciphertext").get<std::string>();
        std::string tag_b64 = j.at("tag").get<std::string>();

        blob.iv = fieldBase64Decode(iv_b64);
        blob.ciphertext = fieldBase64Decode(ct_b64);
        blob.tag = fieldBase64Decode(tag_b64);

    } catch (const nlohmann::json::exception& ex) {
        throw std::runtime_error(std::string("EncryptedBlob::fromJson: JSON error: ") + ex.what());
    }

    return blob;
}

std::vector<EncryptedBlob> FieldEncryption::encryptEntityBatch(const std::vector<std::pair<std::string,std::string>>& items,
                                                                const std::string& key_id) {
    std::vector<EncryptedBlob> out;
    out.resize(items.size());

    // Fetch base key once
    auto base_key = key_provider_->getKey(key_id);
    auto metadata = key_provider_->getKeyMetadata(key_id);

    // If environment variable THEMIS_ENC_PARALLEL is set, run encryptions in parallel for stress testing.
    const char* parallel_env = std::getenv("THEMIS_ENC_PARALLEL");
    bool do_parallel = (parallel_env && *parallel_env);
    const auto logDebugDumpFailure = [](size_t index,
                                        bool parallel_path,
                                        const std::exception* ex) {
        if (ex) {
            THEMIS_WARN("FieldEncryption::encryptEntityBatch: debug dump failed "
                        "({} item {}): {}",
                        parallel_path ? "parallel" : "sequential", index, ex->what());
        } else {
            THEMIS_WARN("FieldEncryption::encryptEntityBatch: debug dump failed "
                        "({} item {}) with unknown exception",
                        parallel_path ? "parallel" : "sequential", index);
        }
    };

    if (do_parallel) {
        tbb::parallel_for(tbb::blocked_range<size_t>(0, items.size()), [&](const tbb::blocked_range<size_t>& r) {
            for (size_t i = r.begin(); i != r.end(); ++i) {
                const auto& ent = items[i];
                try {
                    out[i] = encryptWithKey(ent.second, key_id, metadata.version, base_key);
                    // best-effort debug write (opt-in via env)
                    try {
                        write_debug_dump("encrypt", out[i], true);
                    } catch (...) {
                        logDebugDumpFailure(i, true, nullptr);
                    }
                } catch (const std::exception& ex) {
                    // [E-2] Partial encryption is unsafe — propagate failures so callers
                    // cannot silently store default-constructed (empty) EncryptedBlobs.
                    THEMIS_WARN("FieldEncryption::encryptEntityBatch: encryption failed "
                                "(parallel item {}): {}", i, ex.what());
                    throw;
                } catch (...) {
                    THEMIS_WARN("FieldEncryption::encryptEntityBatch: encryption failed "
                                "(parallel item {}) with unknown exception", i);
                    throw;
                }
            }
        });
    } else {
        // Use sequential loop to avoid potential threading issues with OpenSSL in tests.
        for (size_t i = 0; i < items.size(); ++i) {
            const auto& ent = items[i];
            try {
                out[i] = encryptWithKey(ent.second, key_id, metadata.version, base_key);
                // best-effort debug write (opt-in via env)
                try {
                    write_debug_dump("encrypt", out[i], true);
                } catch (...) {
                    logDebugDumpFailure(i, false, nullptr);
                }
            } catch (const std::exception& ex) {
                // [E-2] Partial encryption is unsafe — propagate failures so callers
                // cannot silently store default-constructed (empty) EncryptedBlobs.
                THEMIS_WARN("FieldEncryption::encryptEntityBatch: encryption failed "
                            "(item {}): {}", i, ex.what());
                throw;
            } catch (...) {
                THEMIS_WARN("FieldEncryption::encryptEntityBatch: encryption failed "
                            "(item {}) with unknown exception", i);
                throw;
            }
        }
    }

    return out;
}

// ===== FieldEncryption Implementation =====

FieldEncryption::FieldEncryption(std::shared_ptr<KeyProvider> key_provider)
    : key_provider_(key_provider)
{
    if (!key_provider_) {
        throw std::invalid_argument("FieldEncryption: key_provider cannot be null");
    }
}

FieldEncryption::~FieldEncryption() = default;

std::shared_ptr<FieldEncryption> FieldEncryption::createDefault() {
    // STUB/SIMULATION NOTE:
    // Purpose: Provides a zero-dependency factory for unit tests and demo code
    //          that have no external key-management infrastructure available.
    // Activation: Called by code paths that do not supply an explicit KeyProvider
    //             (e.g. test harnesses, demo_encryption.cpp, some legacy startup paths).
    // Production Delta: MockKeyProvider stores AES-256 keys in plain process memory
    //                   with NO persistence, NO HSM protection, and NO key rotation
    //                   enforcement.  Any restart silently loses all keys.  Ciphertext
    //                   produced by this factory cannot be decrypted after restart.
    // Removal Plan: Production callers must inject a real KeyProvider (VaultKeyProvider,
    //               HsmKeyProviderAdapter, or similar) via the constructor.
    //               This factory should only be invoked through explicit test/demo paths.
    //               See src/security/FUTURE_ENHANCEMENTS.md §Field Encryption Key Provider.

    // [E-4] Runtime guard: refuse to use MockKeyProvider unless explicitly opted in.
    // Set THEMIS_ALLOW_MOCK_KEY_PROVIDER=1 only in test/demo environments.
    const char* allow_env = std::getenv("THEMIS_ALLOW_MOCK_KEY_PROVIDER");
    const bool allow_mock = (allow_env != nullptr) &&
                            (std::string_view(allow_env) == "1" ||
                             std::string_view(allow_env) == "true");
    if (!allow_mock) {
        throw std::runtime_error(
            "FieldEncryption::createDefault() uses MockKeyProvider which is unsafe in "
            "production (keys are in-memory only and will NOT survive restarts). "
            "Inject a real KeyProvider via the constructor. "
            "To explicitly opt in for testing, set THEMIS_ALLOW_MOCK_KEY_PROVIDER=1.");
    }

    THEMIS_WARN("FieldEncryption::createDefault() is using MockKeyProvider — "
                "keys are in-memory only and will NOT survive restarts. "
                "Inject a production KeyProvider for any persistent data.");
    auto mock_provider = std::make_shared<MockKeyProvider>();
    return std::make_shared<FieldEncryption>(mock_provider);
}

void FieldEncryption::setEncryptionConfig(const EncryptionConfig& config) {
    config_ = config;
}

std::string FieldEncryption::getKeyIdForField(const std::string& field_name) const {
    // Check if field has explicit mapping
    auto it = config_.field_key_mapping.find(field_name);
    if (it != config_.field_key_mapping.end()) {
        return it->second;
    }
    
    // Use default key ID
    return config_.default_key_id;
}

bool FieldEncryption::should_encrypt(const std::string& field_name) const {
    // If encrypted_fields is not empty, only encrypt fields in the set
    if (!config_.encrypted_fields.empty()) {
        return config_.encrypted_fields.find(field_name) != config_.encrypted_fields.end();
    }
    
    // If encrypted_fields is empty but field_key_mapping exists, encrypt mapped fields
    if (!config_.field_key_mapping.empty()) {
        return config_.field_key_mapping.find(field_name) != config_.field_key_mapping.end();
    }
    
    // Default: encrypt all fields if no config is set
    return true;
}

std::vector<uint8_t> FieldEncryption::encrypt_field(
    const std::string& field_name,
    const std::vector<uint8_t>& plaintext)
{
    if (!should_encrypt(field_name)) {
        // Pass through without encryption
        return plaintext;
    }
    
    std::string key_id = getKeyIdForField(field_name);
    auto blob = encrypt(plaintext, key_id);
    
    // Serialize the blob to bytes for storage
    std::string serialized = blob.toBase64();
    return std::vector<uint8_t>(serialized.begin(), serialized.end());
}

std::vector<uint8_t> FieldEncryption::decrypt_field(
    const std::string& field_name,
    const std::vector<uint8_t>& ciphertext)
{
    if (!should_encrypt(field_name)) {
        // Pass through - data was not encrypted
        return ciphertext;
    }
    
    // Deserialize the blob from bytes
    std::string serialized(ciphertext.begin(), ciphertext.end());
    auto blob = EncryptedBlob::fromBase64(serialized);
    
    return decryptToBytes(blob);
}

EncryptedBlob FieldEncryption::encrypt(const std::string& plaintext, const std::string& key_id) {
    std::vector<uint8_t> plaintext_bytes(plaintext.begin(), plaintext.end());
    return encrypt(plaintext_bytes, key_id);
}

EncryptedBlob FieldEncryption::encrypt(const std::vector<uint8_t>& plaintext, const std::string& key_id) {
    // Runtime license gate: field-level encryption is an Enterprise/Hyperscaler feature.
    std::string license_error;
    if (!license::RuntimeLicenseGate::instance()
            .isFeatureAllowed("field_encryption", license_error)) {
        throw std::runtime_error("Field encryption unavailable: " + license_error);
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        // Get active key
        auto key = key_provider_->getKey(key_id);
        auto metadata = key_provider_->getKeyMetadata(key_id);
        
        auto result = encryptInternal(plaintext, key_id, metadata.version, key);
        
        // Update metrics
        metrics_.encrypt_operations_total.fetch_add(1, std::memory_order_relaxed);
        metrics_.encrypt_bytes_total.fetch_add(plaintext.size(), std::memory_order_relaxed);
        
        // Track duration
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
        
        if (duration_us <= 100) {
            metrics_.encrypt_duration_le_100us.fetch_add(1, std::memory_order_relaxed);
        } else if (duration_us <= 500) {
            metrics_.encrypt_duration_le_500us.fetch_add(1, std::memory_order_relaxed);
        } else if (duration_us <= 1000) {
            metrics_.encrypt_duration_le_1ms.fetch_add(1, std::memory_order_relaxed);
        } else if (duration_us <= 5000) {
            metrics_.encrypt_duration_le_5ms.fetch_add(1, std::memory_order_relaxed);
        } else if (duration_us <= 10000) {
            metrics_.encrypt_duration_le_10ms.fetch_add(1, std::memory_order_relaxed);
        } else {
            metrics_.encrypt_duration_gt_10ms.fetch_add(1, std::memory_order_relaxed);
        }
        
        return result;
    } catch (...) {
        metrics_.encrypt_errors_total.fetch_add(1, std::memory_order_relaxed);
        throw;
    }
}

std::string FieldEncryption::decryptToString(const EncryptedBlob& blob) {
    auto plaintext_bytes = decryptToBytes(blob);
    return std::string(plaintext_bytes.begin(), plaintext_bytes.end());
}

std::vector<uint8_t> FieldEncryption::decryptToBytes(const EncryptedBlob& blob) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        // Get specific key version
        auto key = key_provider_->getKey(blob.key_id, blob.key_version);
        
        auto result = decryptInternal(blob, key);
        
        // Update metrics
        metrics_.decrypt_operations_total.fetch_add(1, std::memory_order_relaxed);
        metrics_.decrypt_bytes_total.fetch_add(result.size(), std::memory_order_relaxed);
        
        // Track duration
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
        
        if (duration_us <= 100) {
            metrics_.decrypt_duration_le_100us.fetch_add(1, std::memory_order_relaxed);
        } else if (duration_us <= 500) {
            metrics_.decrypt_duration_le_500us.fetch_add(1, std::memory_order_relaxed);
        } else if (duration_us <= 1000) {
            metrics_.decrypt_duration_le_1ms.fetch_add(1, std::memory_order_relaxed);
        } else if (duration_us <= 5000) {
            metrics_.decrypt_duration_le_5ms.fetch_add(1, std::memory_order_relaxed);
        } else if (duration_us <= 10000) {
            metrics_.decrypt_duration_le_10ms.fetch_add(1, std::memory_order_relaxed);
        } else {
            metrics_.decrypt_duration_gt_10ms.fetch_add(1, std::memory_order_relaxed);
        }
        
        return result;
    } catch (...) {
        metrics_.decrypt_errors_total.fetch_add(1, std::memory_order_relaxed);
        throw;
    }
}

EncryptedBlob FieldEncryption::encryptWithKey(const std::string& plaintext,
                                               const std::string& key_id,
                                               uint32_t key_version,
                                               const std::vector<uint8_t>& key) {
    std::vector<uint8_t> plaintext_bytes(plaintext.begin(), plaintext.end());
    return encryptInternal(plaintext_bytes, key_id, key_version, key);
}

std::string FieldEncryption::decryptWithKey(const EncryptedBlob& blob,
                                             const std::vector<uint8_t>& key) {
    auto plaintext_bytes = decryptInternal(blob, key);
    return std::string(plaintext_bytes.begin(), plaintext_bytes.end());
}

std::vector<uint8_t> FieldEncryption::generateIV() const {
    std::vector<uint8_t> iv(12);  // 96 bits for GCM
    
    if (RAND_bytes(iv.data(), static_cast<int>(iv.size())) != 1) {
        throw EncryptionException("Failed to generate random IV");
    }
    
    return iv;
}

EncryptedBlob FieldEncryption::encryptInternal(const std::vector<uint8_t>& plaintext,
                                                const std::string& key_id,
                                                uint32_t key_version,
                                                const std::vector<uint8_t>& key) {
    if (key.size() != 32) {
        throw EncryptionException("Key must be 32 bytes (256 bits)");
    }
    
    EncryptedBlob blob;
    blob.key_id = key_id;
    blob.key_version = key_version;
    blob.iv = generateIV();
    
    // Create and initialize cipher context
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw EncryptionException("Failed to create cipher context");
    }
    
    try {
        // Initialize encryption with AES-256-GCM
        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
            throw EncryptionException("Failed to initialize cipher");
        }
        
        // Set IV length (12 bytes for GCM)
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr) != 1) {
            throw EncryptionException("Failed to set IV length");
        }
        
        // Initialize key and IV
        if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), blob.iv.data()) != 1) {
            throw EncryptionException("Failed to set key and IV");
        }
        
        // Encrypt plaintext
        blob.ciphertext.resize(plaintext.size() + EVP_CIPHER_block_size(EVP_aes_256_gcm()));
        int len = 0;
        if (EVP_EncryptUpdate(ctx, blob.ciphertext.data(), &len, plaintext.data(), static_cast<int>(plaintext.size())) != 1) {
            throw EncryptionException("Encryption failed");
        }
        int ciphertext_len = len;
        
        // Finalize encryption
        if (EVP_EncryptFinal_ex(ctx, blob.ciphertext.data() + len, &len) != 1) {
            throw EncryptionException("Failed to finalize encryption");
        }
        ciphertext_len += len;
        blob.ciphertext.resize(ciphertext_len);
        
        // Get authentication tag
        blob.tag.resize(16);
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, blob.tag.data()) != 1) {
            throw EncryptionException("Failed to get authentication tag");
        }
        
        EVP_CIPHER_CTX_free(ctx);
        
    } catch (...) {
        EVP_CIPHER_CTX_free(ctx);
        throw;
    }
    THEMIS_INFO("encryptInternal: key_id={}, key_ver={}, iv_len={}, ciphertext_len={}, tag_len={}",
                blob.key_id, blob.key_version, blob.iv.size(), blob.ciphertext.size(), blob.tag.size());
    // Write debug dump (best-effort, opt-in via THEMIS_DEBUG_ENC_DIR env var)
    write_debug_dump("encrypt", blob, true);

    return blob;
}

std::vector<uint8_t> FieldEncryption::decryptInternal(const EncryptedBlob& blob,
                                                       const std::vector<uint8_t>& key) {
    if (key.size() != 32) {
        throw DecryptionException("Key must be 32 bytes (256 bits)");
    }
    
    if (blob.iv.size() != 12) {
        throw DecryptionException("IV must be 12 bytes");
    }
    
    if (blob.tag.size() != 16) {
        throw DecryptionException("Tag must be 16 bytes");
    }
    
    // Create and initialize cipher context
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw DecryptionException("Failed to create cipher context");
    }
    
    try {
        // Initialize decryption with AES-256-GCM
        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
            throw DecryptionException("Failed to initialize cipher");
        }
        
        // Set IV length
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr) != 1) {
            throw DecryptionException("Failed to set IV length");
        }
        
        // Initialize key and IV
        if (EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), blob.iv.data()) != 1) {
            throw DecryptionException("Failed to set key and IV");
        }
        
        // Decrypt ciphertext
        std::vector<uint8_t> plaintext(blob.ciphertext.size() + EVP_CIPHER_block_size(EVP_aes_256_gcm()));
        int len = 0;
        if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, blob.ciphertext.data(), static_cast<int>(blob.ciphertext.size())) != 1) {
            throw DecryptionException("Decryption failed");
        }
        int plaintext_len = len;
        
        // Set expected tag value
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, const_cast<uint8_t*>(blob.tag.data())) != 1) {
            throw DecryptionException("Failed to set authentication tag");
        }
        
        // Finalize decryption (verifies authentication tag)
        THEMIS_DEBUG("decryptInternal: key_id={}, key_ver={}, ciphertext_len={}, tag_len={}, iv_len={}, key_len={}",
                    blob.key_id, blob.key_version, blob.ciphertext.size(), blob.tag.size(), blob.iv.size(), key.size());
        int ret = EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len);
        if (ret <= 0) {
            // write debug dump showing failure (opt-in via THEMIS_DEBUG_ENC_DIR)
            write_debug_dump("decrypt_failed", blob, false);
            THEMIS_ERROR("decryptInternal: EVP_DecryptFinal_ex returned {} (auth failed)", ret);
            throw DecryptionException("Authentication failed - data may have been tampered with");
        }
        // write debug dump showing success (opt-in via THEMIS_DEBUG_ENC_DIR)
        write_debug_dump("decrypt_ok", blob, true);
        plaintext_len += len;
        plaintext.resize(plaintext_len);
        
        EVP_CIPHER_CTX_free(ctx);
        
        return plaintext;
        
    } catch (...) {
        EVP_CIPHER_CTX_free(ctx);
        throw;
    }
}

// ===== Lazy Re-Encryption Implementation =====

std::string FieldEncryption::decryptAndReEncrypt(const EncryptedBlob& blob,
                                                  const std::string& key_id,
                                                  std::optional<EncryptedBlob>& updated_blob) {
    // 1. Decrypt with original key version
    auto key_old = key_provider_->getKey(key_id, blob.key_version);
    auto plaintext_bytes = decryptInternal(blob, key_old);
    std::string plaintext(plaintext_bytes.begin(), plaintext_bytes.end());
    
    // 2. Check if re-encryption is needed
    if (needsReEncryption(blob, key_id)) {
        try {
            // 3. Re-encrypt with current key version
            auto new_blob = encrypt(plaintext, key_id);
            updated_blob = new_blob;
            
            // Update metrics
            metrics_.reencrypt_operations_total.fetch_add(1, std::memory_order_relaxed);
            
            THEMIS_INFO("Lazy re-encryption: key_id={}, old_version={}, new_version={}", 
                       key_id, blob.key_version, new_blob.key_version);
        } catch (const std::exception& e) {
            // If re-encryption fails, still return decrypted data
            // but log the failure for monitoring
            metrics_.reencrypt_errors_total.fetch_add(1, std::memory_order_relaxed);
            THEMIS_ERROR("Lazy re-encryption failed for key_id={}: {}", key_id, e.what());
            updated_blob = std::nullopt;
        }
    } else {
        // No re-encryption needed
        metrics_.reencrypt_skipped_total.fetch_add(1, std::memory_order_relaxed);
        updated_blob = std::nullopt;
    }
    
    return plaintext;
}

bool FieldEncryption::needsReEncryption(const EncryptedBlob& blob, const std::string& key_id) {
    try {
        // Use getCurrentVersion() to determine whether the blob's key version is
        // outdated.  KeyProvider::getCurrentVersion() has a default probe
        // implementation that walks getKey(key_id, v) upward; concrete key
        // providers with a version registry override this for O(1) lookup.
        const uint32_t current_version = key_provider_->getCurrentVersion(key_id);
        if (current_version == 0) {
            // Key does not exist — treat as needing re-encryption so callers
            // surface the missing-key error on the next write attempt.
            return true;
        }
        return blob.key_version < current_version;
    } catch (const std::exception& e) {
        THEMIS_WARN("[SECURITY] needsReEncryption: KMS unavailable for key lookup — "
                    "cannot determine re-encryption need. Error: {}. "
                    "Re-encryption check will be retried on next cycle.", e.what());
        // Fail-safe: assume re-encryption is needed when KMS is unavailable.
        // Silently skipping re-encryption on KMS error would leave stale key versions
        // in production and suppress the underlying availability problem.
        return true;
    }
}

}  // namespace themis

