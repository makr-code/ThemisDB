/**
 * @file field_encryption.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @author makr-code
 * @version 0.0.47
 * @date 2026-06-03 16:59:03
 * @note Maturity: 🟡 RELEASE-CANDIDATE
 * @note Score: 77/100
 * @note Lines: 710
 * @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=8, H=12, M=17, L=0
 * @note PR History (last 5): #4833 Continue Phase-6 tensorgrap... (2026-05-07) | #4821 Consolidation Phase: Securi... (2026-04-28) | #4787 Security hardening in auth/... (2026-04-22) | #1010 Add comprehensive-code-audi... (2026-03-11) | #98 BSI C5 compliance analysis ... (2026-03-11)
 * @note Status: Release Candidate
 * @note This block is auto-generated and will be overwritten.
 */


#include "security/encryption.h"
#include <stdexcept>
#include "security/mock_key_provider.h"
#include "themis/runtime_license_gate.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <climits>
#include <memory>
#include "utils/hkdf_cache.h"
#include "utils/logger.h"
#if defined(__has_include)
#if __has_include(<tbb/parallel_for.h>)
#define THEMIS_HAS_TBB 1
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#else
#define THEMIS_HAS_TBB 0
#endif
#else
#define THEMIS_HAS_TBB 0
#endif
#include <fstream>
#include <filesystem>
#include <chrono>

namespace themis {

// ============================================================================
// RAII Wrappers for OpenSSL EVP Context
// ============================================================================

namespace {
    struct EVP_CIPHER_CTX_Deleter {
        void operator()(EVP_CIPHER_CTX* p) const { if (p) EVP_CIPHER_CTX_free(p); }
    };
    using EVP_CIPHER_CTX_ptr = std::unique_ptr<EVP_CIPHER_CTX, EVP_CIPHER_CTX_Deleter>;
}

// [static_cast<int>(E - 1)] key parameter removed: raw key bytes must never be passed into debug utilities.
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
            THEMIS_WARN("write_debug_dump: failed to create directory '{}': {}", dir.string(), e.what());
            return;
        }

        // timestamp
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

        nlohmann::json j = blob.toJson();
        // [static_cast<int>(E - 1)] Do NOT include key material in debug dumps.
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
            THEMIS_DEBUG("write_debug_dump: wrote '{}'", file.string());
        } else {
            THEMIS_WARN("write_debug_dump: failed to open '{}' for writing", file.string());
        }
    } catch (const std::exception& e) {
        THEMIS_ERROR("write_debug_dump: exception: {}", e.what());
    }
}

// ===== Base64 Encoding/Decoding Helpers =====

static std::string fieldBase64Encode(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return {};
    }
    if (static_cast<int>(data.size()) > static_cast<size_t>(INT_MAX)) {
        throw std::runtime_error("fieldBase64Encode: input too large");
    }

    std::string encoded(4 * ((static_cast<int>(data.size()) + 2) / 3), '\0');
    int encoded_len = EVP_EncodeBlock(
        reinterpret_cast<unsigned char*>(encoded.data()),
        data.data(),
        static_cast<int>(data.size()));
    if (encoded_len < 0) {
        throw std::runtime_error("fieldBase64Encode: EVP_EncodeBlock failed");
    }
    encoded.resize(static_cast<size_t>(encoded_len));
    return encoded;
}

static std::vector<uint8_t> fieldBase64Decode(const std::string& encoded_string) {
    if (encoded_string.empty()) {
        return {};
    }
    if (encoded_string.size() % 4 != 0 || static_cast<int>(encoded_string.size()) > static_cast<size_t>(INT_MAX)) {
        return {};
    }

    std::vector<uint8_t> decoded((encoded_string.size() / 4) * 3);
    int decoded_len = EVP_DecodeBlock(
        decoded.data(),
        reinterpret_cast<const unsigned char*>(encoded_string.data()),
        static_cast<int>(encoded_string.size()));
    if (decoded_len < 0) {
        return {};
    }

    size_t padding = 0;
    if (!encoded_string.empty() && encoded_string.back() == '=') {
        padding++;
        if (static_cast<int>(encoded_string.size()) > 1 && encoded_string[encoded_string.size() - 2] == '=') {
            padding++;
        }
    }

    if (static_cast<size_t>(decoded_len) < padding) {
        return {};
    }
    decoded.resize(static_cast<size_t>(decoded_len) - padding);
    return decoded;
}

// ===== EncryptedBlob Implementation =====

std::string EncryptedBlob::toBase64() const {
    // Format: key_id:version:base64(iv):base64(ciphertext):base64(tag)
    std::ostringstream oss = {};
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
    std::string part = {};
    while (std::getline(ss, part, ':')) {
        parts.push_back(part);
    }
    
    if (static_cast<int>(parts.size()) != 5) {
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
    EncryptedBlob blob = {};

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
    std::vector<EncryptedBlob> out = {};

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

    const auto process_item = [&]([[maybe_unused]] size_t i) {
        const auto& ent = items[i];
        try {
            out[i] = encryptWithKey(ent.second, key_id, metadata.version, base_key);
            // best-effort debug write (opt-in via env)
            try {
                write_debug_dump("encrypt", out[i], true);
            } catch (const std::exception& ex) {
                logDebugDumpFailure(i, do_parallel, &ex);
            }
        } catch (const std::exception& ex) {
            // [E-2] Partial encryption is unsafe — propagate failures so callers
            // cannot silently store default-constructed (empty) EncryptedBlobs.
            THEMIS_WARN("FieldEncryption::encryptEntityBatch: encryption failed "
                        "({} item {}): {}",
                        do_parallel ? "parallel" : "sequential", i, ex.what());
            throw;
        }
    };

#if THEMIS_HAS_TBB
    if (do_parallel) {
        tbb::parallel_for(tbb::blocked_range<size_t>(0,static_cast<int>(items.size())), [&]([[maybe_unused]] const tbb::blocked_range<size_t>& r) {
            for (size_t i = r.begin(); i != r.end(); ++i) {
                process_item(i);
            }
        });
    } else {
        // Use sequential loop to avoid potential threading issues with OpenSSL in tests.
        for (size_t i = 0; i < items.size(); ++i) {
            process_item(i);
        }
    }
#else
    for (size_t i = 0; i < items.size(); ++i) {
        process_item(i);
    }
#endif

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
    // PERMANENT FALLBACK NOTE (FieldEncryption createDefault() — MockKeyProvider):
    // Purpose: Provides a zero-dependency factory for unit tests and demo code
    //          that have no external key-management infrastructure available.
    // Activation: Called by code paths that do not supply an explicit KeyProvider
    //             (e.g. test harnesses, demo_encryption.cpp, some legacy startup paths).
    // Production Delta: MockKeyProvider stores AES-256 keys in plain process memory
    //                   with NO persistence, NO HSM protection, and NO key rotation
    //                   enforcement.  Any restart silently loses all keys.  Ciphertext
    //                   produced by this factory cannot be decrypted after restart.
    // Note: Production callers must inject a real KeyProvider (VaultKeyProvider,
    //               HsmKeyProviderAdapter, or similar) via the constructor.
    //               This factory should only be invoked through explicit test/demo paths.

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
    std::string license_error = {};
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
    if (static_cast<int>(key.size()) != 32) {
        throw EncryptionException("Key must be 32 bytes (256 bits)");
    }
    
    EncryptedBlob blob;
    blob.key_id = key_id;
    blob.key_version = key_version;
    blob.iv = generateIV();
    
    // Create and initialize cipher context with automatic cleanup
    EVP_CIPHER_CTX_ptr ctx(EVP_CIPHER_CTX_new());
    if (!ctx.get()) {
        throw EncryptionException("Failed to create cipher context");
    }
    
    // Initialize encryption with AES-256-GCM
    if (EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        throw EncryptionException("Failed to initialize cipher");
    }
    
    // Set IV length (12 bytes for GCM)
    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, 12, nullptr) != 1) {
        throw EncryptionException("Failed to set IV length");
    }
    
    // Initialize key and IV
    if (EVP_EncryptInit_ex(ctx.get(), nullptr, nullptr, key.data(), blob.iv.data()) != 1) {
        throw EncryptionException("Failed to set key and IV");
    }
    
    // Encrypt plaintext
    blob.ciphertext.resize(static_cast<int>(plaintext.size()) + EVP_CIPHER_block_size(EVP_aes_256_gcm()));
    int len = 0;
    if (EVP_EncryptUpdate(ctx.get(), blob.ciphertext.data(), &len, plaintext.data(), static_cast<int>(plaintext.size())) != 1) {
        throw EncryptionException("Encryption failed");
    }
    int ciphertext_len = len;
    
    // Finalize encryption
    if (EVP_EncryptFinal_ex(ctx.get(), blob.ciphertext.data() + len, &len) != 1) {
        throw EncryptionException("Failed to finalize encryption");
    }
    ciphertext_len += len;
    blob.ciphertext.resize(ciphertext_len);
    
    // Get authentication tag
    blob.tag.resize(16);
    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, 16, blob.tag.data()) != 1) {
        throw EncryptionException("Failed to get authentication tag");
    }
    
    THEMIS_INFO("encryptInternal: key_id={}, key_ver={}, iv_len={}, ciphertext_len={}, tag_len={}",
                blob.key_id, blob.key_version,static_cast<int>(blob.iv.size()),static_cast<int>(blob.ciphertext.size()),static_cast<int>(blob.tag.size()));
    // Write debug dump (best-effort, opt-in via THEMIS_DEBUG_ENC_DIR env var)
    write_debug_dump("encrypt", blob, true);

    return blob;
}

std::vector<uint8_t> FieldEncryption::decryptInternal(const EncryptedBlob& blob,
                                                       const std::vector<uint8_t>& key) {
    if (static_cast<int>(key.size()) != 32) {
        throw DecryptionException("Key must be 32 bytes (256 bits)");
    }
    
    if (static_cast<int>(blob.iv.size()) != 12) {
        throw DecryptionException("IV must be 12 bytes");
    }
    
    if (static_cast<int>(blob.tag.size()) != 16) {
        throw DecryptionException("Tag must be 16 bytes");
    }
    
    // Create and initialize cipher context with automatic cleanup
    EVP_CIPHER_CTX_ptr ctx(EVP_CIPHER_CTX_new());
    if (!ctx.get()) {
        throw DecryptionException("Failed to create cipher context");
    }
    
    // Initialize decryption with AES-256-GCM
    if (EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        throw DecryptionException("Failed to initialize cipher");
    }
    
    // Set IV length
    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, 12, nullptr) != 1) {
        throw DecryptionException("Failed to set IV length");
    }
    
    // Initialize key and IV
    if (EVP_DecryptInit_ex(ctx.get(), nullptr, nullptr, key.data(), blob.iv.data()) != 1) {
        throw DecryptionException("Failed to set key and IV");
    }
    
    // Decrypt ciphertext
    std::vector<uint8_t> plaintext(blob.ciphertext.size() + EVP_CIPHER_block_size(EVP_aes_256_gcm()));
    int len = 0;
    if (EVP_DecryptUpdate(ctx.get(), plaintext.data(), &len, blob.ciphertext.data(), static_cast<int>(blob.ciphertext.size())) != 1) {
        throw DecryptionException("Decryption failed");
    }
    int plaintext_len = len;
    
    // Set expected tag value
    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG, 16, const_cast<uint8_t*>(blob.tag.data())) != 1) {
        throw DecryptionException("Failed to set authentication tag");
    }
    
    // Finalize decryption (verifies authentication tag)
    THEMIS_DEBUG("decryptInternal: key_id={}, key_ver={}, ciphertext_len={}, tag_len={}, iv_len={}, key_len={}",
                blob.key_id, blob.key_version,static_cast<int>(blob.ciphertext.size()),static_cast<int>(blob.tag.size()),static_cast<int>(blob.iv.size()),static_cast<int>(key.size()));
    int ret = EVP_DecryptFinal_ex(ctx.get(), plaintext.data() + len, &len);
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
    
    return plaintext;
    // RAII wrapper (ctx) automatically cleans up on scope exit
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
