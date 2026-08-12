/**
 * @file totp_secret_encryption.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "auth/totp_secret_encryption.h"

#include <algorithm>
#include <iomanip>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <sstream>
#include <stdexcept>

#include "utils/logger.h"

namespace themis {
namespace auth {

// ============================================================================
// Internal Implementation
// ============================================================================

struct TOTPSecretEncryption::Impl {
    Config config;

    explicit Impl(const Config &cfg) : config(cfg) {
        if (config.master_key.size() != 32) {
            throw std::invalid_argument("Master key must be 32 bytes for AES-256");
        }
    }

    ~Impl() {
        // Explicitly zero the master key before deallocation (defence-in-depth).
        // SecureBuffer's destructor also zeroes this field; the explicit call here
        // ensures zeroing is visible at the point of use and survives any future
        // refactoring that might replace SecureBuffer with a plain std::vector.
        if (!config.master_key.empty()) {
            OPENSSL_cleanse(config.master_key.data(), config.master_key.size() * sizeof(uint8_t));
        }
    }
};

// ============================================================================
// Helper Functions
// ============================================================================

namespace {

// Base64 encode
std::string base64Encode(const std::vector<uint8_t> &data) {
    if (data.empty()) {
        return "";
    }

    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *bio = BIO_new(BIO_s_mem());
    bio      = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, data.data(), static_cast<int>(data.size()));
    BIO_flush(bio);

    BUF_MEM *bufferPtr;
    BIO_get_mem_ptr(bio, &bufferPtr);

    std::string result(bufferPtr->data, bufferPtr->length);

    BIO_free_all(bio);
    return result;
}

// Base64 decode
std::vector<uint8_t> base64Decode(const std::string &input) {
    if (input.empty()) {
        return {};
    }

    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *bio = BIO_new_mem_buf(input.data(), static_cast<int>(input.length()));
    bio      = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

    std::vector<uint8_t> result(input.length());
    int len = BIO_read(bio, result.data(), static_cast<int>(result.size()));

    BIO_free_all(bio);

    if (len > 0) {
        result.resize(len);
    } else {
        result.clear();
    }

    return result;
}

} // anonymous namespace

// ============================================================================
// EncryptedSecret Implementation
// ============================================================================

std::string TOTPSecretEncryption::EncryptedSecret::serialize() const {
    std::ostringstream oss;

    // Format: version|salt|iv|ciphertext|tag
    oss << version << "|" << base64Encode(salt) << "|" << base64Encode(iv) << "|" << base64Encode(ciphertext) << "|"
        << base64Encode(tag);

    return oss.str();
}

TOTPSecretEncryption::EncryptedSecret TOTPSecretEncryption::EncryptedSecret::deserialize(const std::string &data) {
    EncryptedSecret result;

    // Split by '|'
    std::vector<std::string> parts;
    std::istringstream iss(data);
    std::string part;
    while (std::getline(iss, part, '|')) {
        parts.push_back(part);
    }

    if (parts.size() != 5) {
        throw std::runtime_error("Invalid encrypted secret format");
    }

    result.version    = std::stoi(parts[0]);
    result.salt       = base64Decode(parts[1]);
    result.iv         = base64Decode(parts[2]);
    result.ciphertext = base64Decode(parts[3]);
    result.tag        = base64Decode(parts[4]);

    return result;
}

// ============================================================================
// TOTPSecretEncryption Implementation
// ============================================================================

TOTPSecretEncryption::TOTPSecretEncryption(const Config &config) : impl_(std::make_unique<Impl>(config)) {
    utils::Logger::info("TOTP Secret Encryption initialized:");
    utils::Logger::info("  PBKDF2 iterations: {}", config.pbkdf2_iterations);
    utils::Logger::info("  Key version: {}", config.key_version);
}

TOTPSecretEncryption::~TOTPSecretEncryption() = default;

TOTPSecretEncryption::TOTPSecretEncryption(TOTPSecretEncryption &&) noexcept            = default;
TOTPSecretEncryption &TOTPSecretEncryption::operator=(TOTPSecretEncryption &&) noexcept = default;

TOTPSecretEncryption::EncryptedSecret TOTPSecretEncryption::encrypt(const std::string &plaintext_secret) {
    EncryptedSecret result;
    result.version = impl_->config.key_version;

    // Generate random salt and IV
    result.salt = generateRandomBytes(impl_->config.salt_size);
    result.iv   = generateRandomBytes(impl_->config.iv_size);

    // Derive encryption key from master key and salt
    auto derived_key = deriveKey(result.salt);

    // Prepare for encryption
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create cipher context");
    }

    try {
        // Initialize encryption (AES-256-GCM)
        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, derived_key.data(), result.iv.data()) != 1) {
            throw std::runtime_error("Failed to initialize encryption");
        }

        // Encrypt the plaintext
        std::vector<uint8_t> plaintext(plaintext_secret.begin(), plaintext_secret.end());
        result.ciphertext.resize(plaintext.size() + EVP_CIPHER_block_size(EVP_aes_256_gcm()));

        int len;
        if (EVP_EncryptUpdate(ctx, result.ciphertext.data(), &len, plaintext.data(), static_cast<int>(plaintext.size()))
            != 1) {
            throw std::runtime_error("Encryption failed");
        }

        int ciphertext_len = len;

        // Finalize encryption
        if (EVP_EncryptFinal_ex(ctx, result.ciphertext.data() + len, &len) != 1) {
            throw std::runtime_error("Encryption finalization failed");
        }

        ciphertext_len += len;
        result.ciphertext.resize(ciphertext_len);

        // Get authentication tag
        result.tag.resize(impl_->config.tag_size);
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, static_cast<int>(impl_->config.tag_size), result.tag.data())
            != 1) {
            throw std::runtime_error("Failed to get authentication tag");
        }

        EVP_CIPHER_CTX_free(ctx);
        
    } catch (...) {
        EVP_CIPHER_CTX_free(ctx);
        throw;
    }

    return result;
}

std::string TOTPSecretEncryption::decrypt(const EncryptedSecret &encrypted) {
    // Derive decryption key from master key and salt
    auto derived_key = deriveKey(encrypted.salt);

    // Prepare for decryption
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create cipher context");
    }

    try {
        // Initialize decryption (AES-256-GCM)
        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, derived_key.data(), encrypted.iv.data()) != 1) {
            throw std::runtime_error("Failed to initialize decryption");
        }

        // Decrypt the ciphertext
        std::vector<uint8_t> plaintext(encrypted.ciphertext.size() + EVP_CIPHER_block_size(EVP_aes_256_gcm()));

        int len;
        if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, encrypted.ciphertext.data(),
                              static_cast<int>(encrypted.ciphertext.size()))
            != 1) {
            throw std::runtime_error("Decryption failed");
        }

        int plaintext_len = len;

        // Set authentication tag
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, static_cast<int>(encrypted.tag.size()),
                                const_cast<uint8_t *>(encrypted.tag.data()))
            != 1) {
            throw std::runtime_error("Failed to set authentication tag");
        }

        // Finalize decryption (verifies authentication tag)
        if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1) {
            throw std::runtime_error("Decryption finalization failed (authentication failed)");
        }

        plaintext_len += len;
        plaintext.resize(plaintext_len);

        EVP_CIPHER_CTX_free(ctx);

        return std::string(plaintext.begin(), plaintext.end());
        
    } catch (...) {
        EVP_CIPHER_CTX_free(ctx);
        throw;
    }
}

std::string TOTPSecretEncryption::encryptAndSerialize(const std::string &plaintext_secret) {
    auto encrypted = encrypt(plaintext_secret);
    return encrypted.serialize();
}

std::string TOTPSecretEncryption::deserializeAndDecrypt(const std::string &serialized) {
    auto encrypted = EncryptedSecret::deserialize(serialized);
    return decrypt(encrypted);
}

void TOTPSecretEncryption::rotateKey(const SecureBuffer<uint8_t> &new_master_key, int new_version) {
    if (new_master_key.size() != 32) {
        throw std::invalid_argument("New master key must be 32 bytes for AES-256");
    }

    impl_->config.master_key  = new_master_key;
    impl_->config.key_version = new_version;

    utils::Logger::info("Encryption key rotated to version {}", new_version);
}

bool TOTPSecretEncryption::needsReencryption(const EncryptedSecret &encrypted) const {
    return encrypted.version < impl_->config.key_version;
}

TOTPSecretEncryption::EncryptedSecret TOTPSecretEncryption::reencrypt(const EncryptedSecret &old_encrypted) {
    // Decrypt with old key (version embedded in encrypted data)
    std::string plaintext = decrypt(old_encrypted);

    // Re-encrypt with current key
    return encrypt(plaintext);
}

SecureBuffer<uint8_t> TOTPSecretEncryption::deriveKey(const std::vector<uint8_t> &salt) {
    SecureBuffer<uint8_t> derived_key(32); // 256 bits for AES-256, zeroed on scope exit

    // Use PBKDF2-HMAC-SHA256
    if (PKCS5_PBKDF2_HMAC(reinterpret_cast<const char *>(impl_->config.master_key.data()),
                          static_cast<int>(impl_->config.master_key.size()), salt.data(), static_cast<int>(salt.size()),
                          impl_->config.pbkdf2_iterations, EVP_sha256(), static_cast<int>(derived_key.size()),
                          derived_key.data())
        != 1) {
        throw std::runtime_error("Key derivation failed");
    }

    return derived_key;
}

std::vector<uint8_t> TOTPSecretEncryption::generateRandomBytes(size_t size) {
    std::vector<uint8_t> bytes(size);

    if (RAND_bytes(bytes.data(), static_cast<int>(size)) != 1) {
        throw std::runtime_error("Random number generation failed");
    }

    return bytes;
}

// ============================================================================
// TOTPSecretRotationManager Implementation
// ============================================================================

TOTPSecretRotationManager::TOTPSecretRotationManager() : TOTPSecretRotationManager(RotationConfig{}) {}

TOTPSecretRotationManager::TOTPSecretRotationManager(const RotationConfig &config) : config_(config) {
    utils::Logger::info("TOTP Secret Rotation Manager initialized:");
    utils::Logger::info("  Grace period: {} days", config_.grace_period_seconds / (24 * 60 * 60));
    utils::Logger::info("  Auto cleanup: {}", config_.auto_cleanup);
}

TOTPSecretRotationManager::SecretVersion TOTPSecretRotationManager::rotateSecret(const std::string &user_id,
                                                                                 const std::string & /*old_secret*/,
                                                                                 const std::string &new_secret) {
    auto now = std::chrono::system_clock::now();

    // Get or create user's secret list
    auto &secrets = user_secrets_[user_id];

    // Mark old secrets as inactive
    for (auto &secret : secrets) {
        secret.is_active = false;
    }

    // Add new secret
    SecretVersion new_version;
    new_version.secret     = new_secret;
    new_version.version    = static_cast<int>(secrets.size()) + 1;
    new_version.created_at = now;
    new_version.is_active  = true;

    secrets.push_back(new_version);

    utils::Logger::info("Rotated TOTP secret for user: {} (version: {})", user_id, new_version.version);

    return new_version;
}

std::vector<TOTPSecretRotationManager::SecretVersion>
TOTPSecretRotationManager::getActiveSecrets(const std::string &user_id) {
    std::vector<SecretVersion> active_secrets;

    auto it = user_secrets_.find(user_id);
    if (it == user_secrets_.end()) {
        return active_secrets;
    }

    auto now = std::chrono::system_clock::now();

    for (const auto &secret : it->second) {
        if (isSecretValid(secret)) {
            active_secrets.push_back(secret);
        }
    }

    return active_secrets;
}

bool TOTPSecretRotationManager::isSecretValid(const SecretVersion &secret_version) const {
    auto now = std::chrono::system_clock::now();
    auto age = std::chrono::duration_cast<std::chrono::seconds>(now - secret_version.created_at);

    // Active secret is always valid
    if (secret_version.is_active) {
        return true;
    }

    // Inactive secret is valid only within grace period
    return age.count() < config_.grace_period_seconds;
}

size_t TOTPSecretRotationManager::cleanupExpiredSecrets() {
    size_t cleaned = 0;

    for (auto &[user_id, secrets] : user_secrets_) {
        auto original_size = secrets.size();

        secrets.erase(std::remove_if(secrets.begin(), secrets.end(),
                                     [this](const SecretVersion &sv) { return !isSecretValid(sv); }),
                      secrets.end());

        cleaned += (original_size - secrets.size());
    }

    if (cleaned > 0) {
        utils::Logger::info("Cleaned up {} expired TOTP secrets", cleaned);
    }

    return cleaned;
}

} // namespace auth
} // namespace themis

