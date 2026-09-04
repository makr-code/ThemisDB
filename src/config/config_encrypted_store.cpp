/**
 * @file config_encrypted_store.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=18, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "config/config_encrypted_store.h"

#include <algorithm>
#include <cstring>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace config {

// ---------------------------------------------------------------------------
// Base64 helpers (no external dependency – implemented locally)
// ---------------------------------------------------------------------------

namespace {

static const char kB64Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64Encode(const std::vector<uint8_t> &data) {
    std::string out = {};
    out.reserve(((static_cast<int>(data.size()) + 2) / 3) * 4);

    const std::size_t len = data.size();
    for (std::size_t i = 0; i < len; i += 3) {
        const bool have2 = (i + 1) < len;
        const bool have3 = (i + 2) < len;

        const uint32_t b0     = data[i];
        const uint32_t b1     = have2 ? data[i + 1] : 0u;
        const uint32_t b2     = have3 ? data[i + 2] : 0u;
        const uint32_t triple = (b0 << 16) | (b1 << 8) | b2;

        out += kB64Chars[(triple >> 18) & 0x3F];
        out += kB64Chars[(triple >> 12) & 0x3F];
        out += have2 ? kB64Chars[(triple >> 6) & 0x3F] : '=';
        out += have3 ? kB64Chars[(triple) & 0x3F] : '=';
    }
    return out;
}

std::vector<uint8_t> base64Decode(const std::string &encoded) {
    auto decodeChar = [](char c) -> int {
        if (c >= 'A' && c <= 'Z') {
            return c - 'A';
        }
        if (c >= 'a' && c <= 'z') {
            return c - 'a' + 26;
        }
        if (c >= '0' && c <= '9') {
            return c - '0' + 52;
        }
        if (c == '+') {
            return 62;
        }
        if (c == '/') {
            return 63;
        }
        return -1; // padding ('=') or invalid
    };

    std::vector<uint8_t> out = {};

    out.reserve((encoded.size() / 4) * 3);

    const std::size_t len = encoded.size();
    for (std::size_t i = 0; i + 3 < len; i += 4) {
        const int a = decodeChar(encoded[i]);
        const int b = decodeChar(encoded[i + 1]);
        const int c = decodeChar(encoded[i + 2]);
        const int d = decodeChar(encoded[i + 3]);

        if (a < 0 || b < 0) {
            break;
        }
        out.push_back(static_cast<uint8_t>((a << 2) | (b >> 4)));
        if (c >= 0) {
            out.push_back(static_cast<uint8_t>(((b & 0x0F) << 4) | (c >> 2)));
        }
        if (d >= 0) {
            out.push_back(static_cast<uint8_t>(((c & 0x03) << 6) | d));
        }
    }
    return out;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// ConfigEncryptedBlob serialisation
// ---------------------------------------------------------------------------

std::string ConfigEncryptedBlob::toJson() const {
    nlohmann::json j;
    j["key_ver"] = key_version;
    j["iv"]      = base64Encode(iv);
    j["ct"]      = base64Encode(ciphertext);
    j["tag"]     = base64Encode(tag);
    return j.dump();
}

ConfigEncryptedBlob ConfigEncryptedBlob::fromJson(const std::string &json_str) {
    try {
        auto j = nlohmann::json::parse(json_str);
        ConfigEncryptedBlob blob;
        blob.key_version = j.at("key_ver").get<uint32_t>();
        blob.iv          = base64Decode(j.at("iv").get<std::string>());
        blob.ciphertext  = base64Decode(j.at("ct").get<std::string>());
        blob.tag         = base64Decode(j.at("tag").get<std::string>());
        return blob;
    } catch (const nlohmann::json::exception &ex) {
        throw ConfigEncryptionException(std::string("malformed blob JSON: ") + ex.what());
    }
}

// ---------------------------------------------------------------------------
// ConfigEncryptedStore – construction
// ---------------------------------------------------------------------------

ConfigEncryptedStore::ConfigEncryptedStore() {
    key_.version   = 1;
    key_.key_bytes = generateKey();
}

// ---------------------------------------------------------------------------
// CRUD
// ---------------------------------------------------------------------------

void ConfigEncryptedStore::set(const std::string &config_key, const std::string &plaintext) {
    if (config_key.empty()) {
        throw std::invalid_argument("ConfigEncryptedStore::set: config_key must not be empty");
    }
    std::unique_lock<std::shared_mutex> lock(mutex_);
    store_[config_key] = encryptValue(plaintext);
}

std::string ConfigEncryptedStore::get(const std::string &config_key) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = store_.find(config_key);
    if (it == store_.end()) {
        throw ConfigKeyNotFoundException(config_key);
    }
    return decryptBlob(it->second);
}

std::optional<std::string> ConfigEncryptedStore::tryGet(const std::string &config_key) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = store_.find(config_key);
    if (it == store_.end()) {
        return std::nullopt;
    }
    try {
        return decryptBlob(it->second);
    } catch (const ConfigEncryptionException &) {
        return std::nullopt;
    } catch (const nlohmann::json::exception &) {
        return std::nullopt;
    } catch (const std::exception &) {
        return std::nullopt;
    } catch (const std::string &) {
        return std::nullopt;
    } catch (const char *) {
        return std::nullopt;
    }
}

bool ConfigEncryptedStore::remove(const std::string &config_key) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    return store_.erase(config_key) > 0;
}

bool ConfigEncryptedStore::contains(const std::string &config_key) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return store_.count(config_key) > 0;
}

std::vector<std::string> ConfigEncryptedStore::keys() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    std::vector<std::string> result = {};

    result.reserve(store_.size());
    for (const auto &kv : store_) {
        result.push_back(kv.first);
    }
    return result;
}

std::size_t ConfigEncryptedStore::size() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return static_cast<int>(store_.size());
}

void ConfigEncryptedStore::clear() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    store_.clear();
}

// ---------------------------------------------------------------------------
// Key rotation
// ---------------------------------------------------------------------------

uint32_t ConfigEncryptedStore::rotateKey() {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    // Generate new key material.
    KeyMaterial new_key;
    new_key.version   = key_.version + 1;
    new_key.key_bytes = generateKey();

    // Re-encrypt all stored values with the new key.
    // We perform the full re-encrypt before swapping to preserve atomicity:
    // if any decryption/encryption fails, we throw and leave the store intact.
    std::unordered_map<std::string, ConfigEncryptedBlob> new_store = {};

    new_store.reserve(store_.size());

    for (const auto &kv : store_) {
        // Decrypt with current (old) key.
        std::string plaintext = decryptBlob(kv.second);

        // Encrypt with new key (temporarily swap for encryptValue helper).
        ConfigEncryptedBlob new_blob;
        std::vector<uint8_t> out_iv, out_tag;
        auto ct              = aesGcmEncrypt(plaintext, new_key.key_bytes, out_iv, out_tag);
        new_blob.key_version = new_key.version;
        new_blob.iv          = std::move(out_iv);
        new_blob.ciphertext  = std::move(ct);
        new_blob.tag         = std::move(out_tag);

        new_store[kv.first] = std::move(new_blob);
    }

    // Securely zero old key bytes before replacement.
    std::fill(key_.key_bytes.begin(), key_.key_bytes.end(), static_cast<uint8_t>(0));

    key_   = std::move(new_key);
    store_ = std::move(new_store);

    return key_.version;
}

uint32_t ConfigEncryptedStore::currentKeyVersion() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return key_.version;
}

// ---------------------------------------------------------------------------
// Serialisation / deserialisation
// ---------------------------------------------------------------------------

std::string ConfigEncryptedStore::serialize() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    nlohmann::json j;
    j["key_version"] = key_.version;
    j["key_bytes"]   = base64Encode(key_.key_bytes);

    nlohmann::json entries = nlohmann::json::object();
    for (const auto &kv : store_) {
        entries[kv.first] = kv.second.toJson();
    }
    j["entries"] = entries;

    return j.dump();
}

void ConfigEncryptedStore::deserialize(const std::string &json_str) {
    try {
        auto j = nlohmann::json::parse(json_str);

        KeyMaterial km;
        km.version   = j.at("key_version").get<uint32_t>();
        km.key_bytes = base64Decode(j.at("key_bytes").get<std::string>());
        if (static_cast<int>(km.key_bytes.size()) != 32) {
            throw ConfigEncryptionException("deserialize: key_bytes must be exactly 32 bytes, got "
                                            + std::to_string(km.key_bytes.size()));
        }

        std::unordered_map<std::string, ConfigEncryptedBlob> new_store;
        auto &entries = j.at("entries");
        for (auto it = entries.begin(); it != entries.end(); ++it) {
            new_store[it.key()] = ConfigEncryptedBlob::fromJson(it.value().get<std::string>());
        }

        std::unique_lock<std::shared_mutex> lock(mutex_);
        // Zero out old key before replacing.
        std::fill(key_.key_bytes.begin(), key_.key_bytes.end(), static_cast<uint8_t>(0));
        key_   = std::move(km);
        store_ = std::move(new_store);
    } catch (const ConfigEncryptionException &) {
        throw;
    } catch (const nlohmann::json::exception &ex) {
        throw ConfigEncryptionException(std::string("deserialize: JSON parse error: ") + ex.what());
    }
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

std::vector<uint8_t> ConfigEncryptedStore::generateKey() {
    std::vector<uint8_t> key(32);
    if (RAND_bytes(key.data(), static_cast<int>(key.size())) != 1) {
        throw ConfigEncryptionException("generateKey: RAND_bytes failed");
    }
    return key;
}

std::vector<uint8_t> ConfigEncryptedStore::generateIV() {
    std::vector<uint8_t> iv(12);
    if (RAND_bytes(iv.data(), static_cast<int>(iv.size())) != 1) {
        throw ConfigEncryptionException("generateIV: RAND_bytes failed");
    }
    return iv;
}

std::vector<uint8_t> ConfigEncryptedStore::aesGcmEncrypt(const std::string &plaintext, const std::vector<uint8_t> &key,
                                                         std::vector<uint8_t> &out_iv, std::vector<uint8_t> &out_tag) {
    out_iv = generateIV();
    out_tag.resize(16);

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw ConfigEncryptionException("aesGcmEncrypt: EVP_CIPHER_CTX_new failed");
    }

    struct CtxGuard {
        EVP_CIPHER_CTX *p;
        ~CtxGuard() {
            EVP_CIPHER_CTX_free(p);
        }
    } guard{ctx};

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        throw ConfigEncryptionException("aesGcmEncrypt: EVP_EncryptInit_ex (cipher) failed");
    }
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr) != 1) {
        throw ConfigEncryptionException("aesGcmEncrypt: set IV length failed");
    }
    if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), out_iv.data()) != 1) {
        throw ConfigEncryptionException("aesGcmEncrypt: EVP_EncryptInit_ex (key/iv) failed");
    }

    const auto *pt   = reinterpret_cast<const unsigned char *>(plaintext.data());
    const int pt_len = static_cast<int>(plaintext.size());

    std::vector<uint8_t> ciphertext(plaintext.size());
    int len = 0;
    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, pt, pt_len) != 1) {
        throw ConfigEncryptionException("aesGcmEncrypt: EVP_EncryptUpdate failed");
    }

    int final_len = 0;
    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &final_len) != 1) {
        throw ConfigEncryptionException("aesGcmEncrypt: EVP_EncryptFinal_ex failed");
    }
    ciphertext.resize(len + final_len);

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, out_tag.data()) != 1) {
        throw ConfigEncryptionException("aesGcmEncrypt: get GCM tag failed");
    }

    return ciphertext;
}

std::string ConfigEncryptedStore::aesGcmDecrypt(const std::vector<uint8_t> &ciphertext, const std::vector<uint8_t> &key,
                                                const std::vector<uint8_t> &iv, const std::vector<uint8_t> &tag) {
    if (static_cast<int>(iv.size()) != 12) {
        throw ConfigEncryptionException("aesGcmDecrypt: IV must be 12 bytes");
    }
    if (static_cast<int>(tag.size()) != 16) {
        throw ConfigEncryptionException("aesGcmDecrypt: tag must be 16 bytes");
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw ConfigEncryptionException("aesGcmDecrypt: EVP_CIPHER_CTX_new failed");
    }

    struct CtxGuard {
        EVP_CIPHER_CTX *p;
        ~CtxGuard() {
            EVP_CIPHER_CTX_free(p);
        }
    } guard{ctx};

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        throw ConfigEncryptionException("aesGcmDecrypt: EVP_DecryptInit_ex (cipher) failed");
    }
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr) != 1) {
        throw ConfigEncryptionException("aesGcmDecrypt: set IV length failed");
    }
    if (EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data()) != 1) {
        throw ConfigEncryptionException("aesGcmDecrypt: EVP_DecryptInit_ex (key/iv) failed");
    }

    std::vector<uint8_t> plaintext_buf(ciphertext.size());
    int len = 0;
    if (EVP_DecryptUpdate(ctx, plaintext_buf.data(), &len, ciphertext.data(), static_cast<int>(ciphertext.size()))
        != 1) {
        throw ConfigEncryptionException("aesGcmDecrypt: EVP_DecryptUpdate failed");
    }

    // Set the expected tag before finalising.
    // EVP_CTRL_GCM_SET_TAG expects a non-const pointer.
    std::vector<uint8_t> tag_copy(tag);
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag_copy.data()) != 1) {
        throw ConfigEncryptionException("aesGcmDecrypt: set GCM tag failed");
    }

    int final_len = 0;
    if (EVP_DecryptFinal_ex(ctx, plaintext_buf.data() + len, &final_len) != 1) {
        // Authentication tag mismatch – data may have been tampered with.
        throw ConfigEncryptionException(
            "aesGcmDecrypt: authentication tag verification failed (data tampered or wrong key)");
    }

    plaintext_buf.resize(len + final_len);
    return std::string(plaintext_buf.begin(), plaintext_buf.end());
}

ConfigEncryptedBlob ConfigEncryptedStore::encryptValue(const std::string &plaintext) const {
    // Caller holds mutex_.
    ConfigEncryptedBlob blob;
    blob.key_version = key_.version;
    blob.ciphertext  = aesGcmEncrypt(plaintext, key_.key_bytes, blob.iv, blob.tag);
    return blob;
}

std::string ConfigEncryptedStore::decryptBlob(const ConfigEncryptedBlob &blob) const {
    // Caller holds mutex_.
    if (blob.key_version != key_.version) {
        // Guard against stale blobs after a rotation if deserialize is misused.
        throw ConfigEncryptionException("decryptBlob: blob key version " + std::to_string(blob.key_version)
                                        + " does not match current key version " + std::to_string(key_.version));
    }
    return aesGcmDecrypt(blob.ciphertext, key_.key_bytes, blob.iv, blob.tag);
}

} // namespace config
} // namespace themis
