/**
 * @file config_encrypted_store.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace config {

/**
 * @brief Exception thrown when an encryption or decryption operation fails.
 */
class ConfigEncryptionException : public std::runtime_error {
public:
    /**
     * @brief TBD: Describe ConfigEncryptionException.
     * @param[in] msg Input parameter.
     * @return Return value.
     */
    explicit ConfigEncryptionException(const std::string& msg)
        : std::runtime_error("ConfigEncryptedStore: " + msg) {}
};

/**
 * @brief Exception thrown when a requested key ID is not found.
 */
class ConfigKeyNotFoundException : public std::runtime_error {
public:
    /**
     * @brief TBD: Describe ConfigKeyNotFoundException.
     * @param[in] key_id Input parameter.
     * @return Return value.
     */
    explicit ConfigKeyNotFoundException(const std::string& key_id)
        : std::runtime_error("ConfigEncryptedStore: key not found: " + key_id) {}
};

/**
 * @brief An encrypted blob produced by ConfigEncryptedStore.
 *
 * Serialisation format (Base64-encoded JSON stored inside the store):
 * @code
 * {
 *   "key_ver": <uint32_t>,
 *   "iv":      "<base64 12 bytes>",
 *   "ct":      "<base64 ciphertext>",
 *   "tag":     "<base64 16 bytes>"
 * }
 * @endcode
 */
struct ConfigEncryptedBlob {
    uint32_t             key_version{0};
    std::vector<uint8_t> iv;          ///< 12 bytes (AES-GCM standard)
    std::vector<uint8_t> ciphertext;
    std::vector<uint8_t> tag;         ///< 16 bytes (AES-GCM authentication tag)

    /**
     * @brief Serialise this encrypted blob to a compact JSON string.
     * @return JSON string containing key version, IV, ciphertext, and tag.
     */
    std::string toJson() const;

    /**
     * @brief Parse an encrypted blob from a JSON string previously produced by toJson().
     * @param json_str Compact JSON string containing the encoded blob fields.
     * @return Parsed ConfigEncryptedBlob value.
     */
    static ConfigEncryptedBlob fromJson(const std::string& json_str);
};

/**
 * @brief Encrypted key-value store for sensitive configuration values.
 *
 * ConfigEncryptedStore wraps any string-valued config key with AES-256-GCM
 * authenticated encryption.  It is designed for storing small numbers of
 * sensitive configuration values (passwords, tokens, API keys) that must
 * survive process restarts when serialised and restored.
 *
 * ### Encryption scheme
 * - Algorithm : AES-256-GCM (NIST SP 800-38D)
 * - Key size  : 256 bits (32 bytes) per key version
 * - IV size   : 96 bits (12 bytes), randomly generated per encryption
 * - Tag size  : 128 bits (16 bytes), verified on every decryption
 *
 * ### Key rotation
 * rotateKey() generates a new 256-bit key version, re-encrypts all currently
 * stored values with the new key, and retires the old version.  The operation
 * is atomic with respect to the internal mutex: no read can observe a
 * partially rotated state.
 *
 * ### Thread safety
 * All public methods are thread-safe; they acquire the internal mutex.
 *
 * ### Persistence
 * serialize() returns a JSON string that captures all encrypted values and the
 * current key material.  deserialize() restores the store from such a string.
 * The serialised form contains the key bytes in plaintext — callers are
 * responsible for protecting it (e.g. wrapping it in a master-key envelope
 * before writing to disk).
 *
 * Example usage:
 * @code
 * ConfigEncryptedStore store;
 * store.set("db_password", "hunter2");
 * store.set("api_token",   "tok_abc123");
 *
 * std::string snapshot = store.serialize();
 * // ... persist snapshot ...
 *
 * ConfigEncryptedStore restored;
 * restored.deserialize(snapshot);
 * assert(restored.get("db_password") == "hunter2");
 *
 * // Key rotation — re-encrypts all values with a new AES-256 key.
 * store.rotateKey();
 * assert(store.get("db_password") == "hunter2");
 * @endcode
 */
class ConfigEncryptedStore {
public:
    /**
     * @brief Construct a store and generate an initial AES-256 key (version 1).
     *
     * @throws ConfigEncryptionException if the OS PRNG is unavailable.
     */
    ConfigEncryptedStore();

    ~ConfigEncryptedStore() = default;

    // Non-copyable, non-movable.
    ConfigEncryptedStore(const ConfigEncryptedStore&)            = delete;
    ConfigEncryptedStore& operator=(const ConfigEncryptedStore&) = delete;
    ConfigEncryptedStore(ConfigEncryptedStore&&)                 noexcept = delete;
    ConfigEncryptedStore& operator=(ConfigEncryptedStore&&)      noexcept = delete;

    // -------------------------------------------------------------------------
    // CRUD
    // -------------------------------------------------------------------------

    /**
     * @brief Encrypt and store a config value.
     *
     * If a value already exists for @p config_key it is silently replaced.
     *
     * @param config_key  Logical key name (must not be empty).
     * @param plaintext   The string value to protect.
     * @throws ConfigEncryptionException on AES failure.
     * @throws std::invalid_argument     if config_key is empty.
     */
    void set(const std::string& config_key, const std::string& plaintext);

    /**
     * @brief Decrypt and return a stored config value.
     *
     * @param config_key  Logical key name.
     * @return Decrypted plaintext string.
     * @throws ConfigKeyNotFoundException if no value is stored for config_key.
     * @throws ConfigEncryptionException  if decryption or tag verification fails.
     */
    std::string get(const std::string& config_key) const;

    /**
     * @brief Return a stored value, or std::nullopt if it does not exist.
     *
     * Unlike get(), this never throws ConfigKeyNotFoundException.
     * @param config_key Logical key name.
     * @return Stored plaintext, or std::nullopt when the key is absent.
     */
    std::optional<std::string> tryGet(const std::string& config_key) const;

    /**
     * @brief Remove a stored config value.
     *
     * @param config_key Logical key name to erase.
     * @return true if the key existed and was removed, false otherwise.
     */
    bool remove(const std::string& config_key);

    /**
     * @brief Check whether a config key has a stored value.
     * @param config_key Logical key name to probe.
     * @return true if the key exists in the store, false otherwise.
     */
    bool contains(const std::string& config_key) const;

    /**
     * @brief Return the list of stored config key names (not their values).
     * @return Snapshot of all logical config keys currently stored.
     */
    std::vector<std::string> keys() const;

    /**
     * @brief Return the number of stored config entries.
     * @return Count of encrypted config entries currently stored.
     */
    std::size_t size() const;

    /**
     * @brief Remove all stored config entries (key material is retained).
     */
    void clear();

    // -------------------------------------------------------------------------
    // Key rotation
    // -------------------------------------------------------------------------

    /**
     * @brief Rotate to a freshly generated AES-256 key.
     *
     * Process (atomic under the internal mutex):
     * 1. Generate a new 256-bit key (version = current_version + 1).
     * 2. Decrypt every stored value with the old key.
     * 3. Re-encrypt each value with the new key.
     * 4. Replace the in-memory key with the new version.
     *
     * The old key is securely zeroed from memory after rotation.
     *
     * @return The new key version number.
     * @throws ConfigEncryptionException if re-encryption of any value fails.
     */
    uint32_t rotateKey();

    /**
     * @brief Return the current key version number (starts at 1).
     * @return Active encryption-key version used for new writes.
     */
    uint32_t currentKeyVersion() const;

    // -------------------------------------------------------------------------
    // Serialisation / deserialisation
    // -------------------------------------------------------------------------

    /**
     * @brief Serialise the store (key material + encrypted values) to JSON.
     *
     * The returned string contains the AES key in plaintext.  Callers must
     * protect it appropriately (e.g. encrypt under a master key) before
     * writing to persistent storage.
     *
     * @return JSON string representation.
     */
    std::string serialize() const;

    /**
     * @brief Restore the store from a JSON string produced by serialize().
     *
     * Replaces all current state (key material and stored values).
     *
     * @param json_str JSON string previously produced by serialize().
     * @throws ConfigEncryptionException if the JSON is malformed or the
     *         key material has an unexpected size.
     */
    void deserialize(const std::string& json_str);

private:
    // ---- internal types -----

    struct KeyMaterial {
        uint32_t             version{0};
        std::vector<uint8_t> key_bytes; ///< 32 bytes (AES-256)
    };

    // ---- helpers ----

    /// Generate 32 cryptographically random bytes.
    static std::vector<uint8_t> generateKey();

    /// Generate 12 cryptographically random bytes for use as an AES-GCM IV.
    static std::vector<uint8_t> generateIV();

    /// AES-256-GCM encrypt.  Returns ciphertext; populates iv and tag.
    static std::vector<uint8_t> aesGcmEncrypt(
        const std::string&        plaintext,
        const std::vector<uint8_t>& key,
        std::vector<uint8_t>&     out_iv,
        std::vector<uint8_t>&     out_tag);

    /// AES-256-GCM decrypt.  Verifies the authentication tag.
    static std::string aesGcmDecrypt(
        const std::vector<uint8_t>& ciphertext,
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& iv,
        const std::vector<uint8_t>& tag);

    /**
     * @brief TBD: Describe encryptValue.
     * @param[in] plaintext Input parameter.
     * @return Return value.
     */
    ConfigEncryptedBlob encryptValue(const std::string& plaintext) const;
    /**
     * @brief TBD: Describe decryptBlob.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    std::string         decryptBlob(const ConfigEncryptedBlob& blob) const;

    // ---- state ----

    mutable std::shared_mutex                              mutex_;
    KeyMaterial                                            key_;
    std::unordered_map<std::string, ConfigEncryptedBlob>   store_;
};

} // namespace config
} // namespace themis
