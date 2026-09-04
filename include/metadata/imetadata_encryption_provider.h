/**
 * @file imetadata_encryption_provider.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>

namespace themis {
namespace metadata {

// ── MetadataEncryptionAlgorithm ───────────────────────────────────────────────

/**
 * @brief Identifies the encryption algorithm used by a provider instance.
 */
enum class MetadataEncryptionAlgorithm {
    NONE,        ///< No encryption (pass-through)
    XOR_BASIC,   ///< Byte-wise XOR with repeating key (demo / test only)
    AES_GCM_256, ///< AES-256 in GCM mode (production-grade; external impl required)
    CUSTOM,      ///< Caller-supplied algorithm (documentation via algorithm())
};

// ── MetadataEncryptionException ───────────────────────────────────────────────

/**
 * @brief Exception thrown when an encryption or decryption operation fails.
 *
 * Carries the affected field name and a human-readable reason so that
 * callers can distinguish configuration errors (e.g. empty key) from
 * runtime errors (e.g. corrupt ciphertext).
 */
class MetadataEncryptionException : public std::runtime_error {
public:
    MetadataEncryptionException(std::string_view field_name,
                                std::string_view reason)
        : std::runtime_error(
              "MetadataEncryptionException: field='" + std::string(field_name) +
              "' reason='" + std::string(reason) + "'")
        , field_name_(field_name)
        , reason_(reason) {}

    /// The metadata field that triggered the exception.
    const std::string& fieldName() const noexcept { return field_name_; }

    /// Human-readable description of the failure.
    const std::string& reason()    const noexcept { return reason_; }

private:
    std::string field_name_;
    std::string reason_;
};

// ── IMetadataEncryptionProvider ───────────────────────────────────────────────

/**
 * @brief Abstract interface for field-level metadata encryption.
 *
 * Implementations MUST be thread-safe and stateless with respect to the
 * encrypt() / decrypt() operations (i.e. concurrent calls with different
 * field names must not interfere with each other).
 */
class IMetadataEncryptionProvider {
public:
    virtual ~IMetadataEncryptionProvider() = default;

    /**
     * @brief Returns true if the named field should be encrypted.
     *
     * @param field_name  Metadata field name (e.g. "connection_string").
     * @return true if this provider will encrypt values for that field.
     */
    [[nodiscard]] virtual bool shouldEncrypt(std::string_view field_name) const = 0;

    /**
     * @brief Encrypt @p value for the field identified by @p field_name.
     *
     * @param field_name  Metadata field name; used for key-derivation or AAD.
     * @param value       Plain-text value to encrypt.
     * @return            Cipher-text representation (algorithm-specific encoding).
     * @throws MetadataEncryptionException on key / configuration errors.
     */
    [[nodiscard]] virtual std::string encrypt(std::string_view field_name,
                                std::string_view value) const = 0;

    /**
     * @brief Decrypt @p cipher_text for the field identified by @p field_name.
     *
     * @param field_name   Metadata field name.
     * @param cipher_text  Cipher-text previously produced by encrypt().
     * @return             Recovered plain-text value.
     * @throws MetadataEncryptionException on key / configuration errors or
     *         if the cipher-text is malformed.
     */
    [[nodiscard]] virtual std::string decrypt(std::string_view field_name,
                                std::string_view cipher_text) const = 0;

    /**
     * @brief Returns the algorithm identifier for this provider.
     */
    [[nodiscard]] virtual MetadataEncryptionAlgorithm algorithm() const = 0;
};

// ── NoOpMetadataEncryptionProvider ────────────────────────────────────────────

/**
 * @brief Default no-op implementation that never encrypts anything.
 *
 * Suitable for development environments or when field-level encryption is not
 * required.  All values are returned unchanged by both encrypt() and decrypt().
 */
class NoOpMetadataEncryptionProvider : public IMetadataEncryptionProvider {
public:
    bool shouldEncrypt(std::string_view /*field_name*/) const override {
        return false;
    }

    std::string encrypt(std::string_view /*field_name*/,
                        std::string_view value) const override {
        return std::string(value);
    }

    std::string decrypt(std::string_view /*field_name*/,
                        std::string_view cipher_text) const override {
        return std::string(cipher_text);
    }

    MetadataEncryptionAlgorithm algorithm() const override {
        return MetadataEncryptionAlgorithm::NONE;
    }
};

// ── FieldSetMetadataEncryptionProvider ────────────────────────────────────────

/**
 * @brief Thread-safe XOR-based encryption provider for a configurable field set.
 *
 * @warning  XOR with a repeating key is NOT cryptographically secure.
 *           This implementation is provided for demonstration, testing, and
 *           local development only.  For production use, replace this with an
 *           AES-GCM-256 provider backed by a proper KMS.
 *
 * Features:
 *  - Maintains a set of field names whose values should be encrypted.
 *  - The special field name "*" causes ALL fields to be encrypted.
 *  - encrypt() and decrypt() are symmetric (XOR is its own inverse).
 *  - The encryption key must be non-empty; an empty key throws
 *    MetadataEncryptionException at call time.
 *  - All mutations to the field set are protected by std::mutex.
 *
 * Example:
 * @code
 *   FieldSetMetadataEncryptionProvider enc("s3cr3t");
 *   enc.addField("connection_string");
 *   enc.addField("api_key");
 *
 *   auto cipher = enc.encrypt("api_key", "hunter2");
 *   auto plain  = enc.decrypt("api_key", cipher);   // == "hunter2"
 * @endcode
 */
class FieldSetMetadataEncryptionProvider : public IMetadataEncryptionProvider {
public:
    /**
     * @brief Construct a provider with the given repeating XOR key.
     *
     * @param key  Non-empty byte string used as the XOR mask.
     * @throws MetadataEncryptionException if @p key is empty.
     */
    explicit FieldSetMetadataEncryptionProvider(std::string_view key)
        : key_(key) {
        if (key_.empty()) {
            throw MetadataEncryptionException(
                "*", "Encryption key must not be empty");
        }
    }

    // ── Field set management ──────────────────────────────────────────────────

    /**
     * @brief Register @p field_name as a field that should be encrypted.
     *
     * Pass "*" to encrypt every field regardless of name.
     */
    void addField(std::string_view field_name) {
        std::unique_lock<std::mutex> lk(mutex_);
        fields_.insert(std::string(field_name));
    }

    /**
     * @brief Deregister @p field_name.  No-op if it was not registered.
     */
    void removeField(std::string_view field_name) {
        std::unique_lock<std::mutex> lk(mutex_);
        fields_.erase(std::string(field_name));
    }

    /**
     * @brief Returns the number of explicitly registered field names.
     *
     * Note: when the set contains "*", every field is encrypted even though
     * fieldCount() may return 1.
     */
    size_t fieldCount() const {
        std::unique_lock<std::mutex> lk(mutex_);
        return fields_.size();
    }

    // ── IMetadataEncryptionProvider ───────────────────────────────────────────

    bool shouldEncrypt(std::string_view field_name) const override {
        std::unique_lock<std::mutex> lk(mutex_);
        return fields_.count("*") > 0 ||
               fields_.count(std::string(field_name)) > 0;
    }

    /**
     * @brief XOR-encrypt @p value.
     *
     * Each byte of the value is XOR'd with the corresponding byte of the key
     * (wrapping around using modulo indexing).
     *
     * @throws MetadataEncryptionException if the key is empty (should not
     *         happen after construction, but guards against future mutations).
     */
    std::string encrypt(std::string_view field_name,
                        std::string_view value) const override {
        return xorTransform(field_name, value);
    }

    /**
     * @brief XOR-decrypt @p cipher_text.
     *
     * Because XOR is its own inverse, this is identical to encrypt().
     */
    std::string decrypt(std::string_view field_name,
                        std::string_view cipher_text) const override {
        return xorTransform(field_name, cipher_text);
    }

    MetadataEncryptionAlgorithm algorithm() const override {
        return MetadataEncryptionAlgorithm::XOR_BASIC;
    }

private:
    std::string xorTransform(std::string_view field_name,
                             std::string_view data) const {
        if (key_.empty()) {
            throw MetadataEncryptionException(
                field_name, "Encryption key must not be empty");
        }
        std::string result = {};
        result.resize(data.size());
        for (size_t i = 0; i < data.size(); ++i) {
            result[i] = static_cast<char>(
                static_cast<unsigned char>(data[i]) ^
                static_cast<unsigned char>(key_[i % key_.size()]));
        }
        return result;
    }

    mutable std::mutex  mutex_;
    std::set<std::string> fields_;
    const std::string   key_;
};

} // namespace metadata
} // namespace themis
