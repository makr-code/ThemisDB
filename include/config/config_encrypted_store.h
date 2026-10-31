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

class ConfigEncryptionException : public std::runtime_error {
public:
    /**
     * @brief Config Encryption Exception.
     * @param[in] msg Input parameter.
     * @return Return value.
     */
    explicit ConfigEncryptionException(const std::string& msg)
        : std::runtime_error("ConfigEncryptedStore: " + msg) {}
};

class ConfigKeyNotFoundException : public std::runtime_error {
public:
    /**
     * @brief Config Key Not Found Exception.
     * @param[in] key_id Identifier of the key.
     * @return Return value.
     */
    explicit ConfigKeyNotFoundException(const std::string& key_id)
        : std::runtime_error("ConfigEncryptedStore: key not found: " + key_id) {}
};

struct ConfigEncryptedBlob {
    uint32_t             key_version{0};
    std::vector<uint8_t> iv;          ///< 12 bytes (AES-GCM standard)
    std::vector<uint8_t> ciphertext;
    std::vector<uint8_t> tag;         ///< 16 bytes (AES-GCM authentication tag)

    /**
     * @brief Serializes this encrypted entry as a JSON object.
     * @return JSON text containing the key version, IV, ciphertext, and tag.
     */
    std::string toJson() const;

    /**
     * @brief Parses an encrypted entry from its JSON representation.
     * @param[in] json_str JSON object containing the encrypted entry fields.
     * @return The decoded encrypted entry.
     * @throws ConfigEncryptionException if a required field is missing or invalid.
     */
    static ConfigEncryptedBlob fromJson(const std::string& json_str);
};

class ConfigEncryptedStore {
public:
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
     * @brief Encrypts and stores a value under a non-empty configuration key.
     * @param[in] config_key Non-empty key identifying the configuration value.
     * @param[in] plaintext Value to encrypt and retain in memory.
     * @throws std::invalid_argument if config_key is empty.
     * @throws ConfigEncryptionException if encryption fails.
     */
    void set(const std::string& config_key, const std::string& plaintext);

    /**
     * @brief Decrypts and returns the value associated with a key.
     * @param[in] config_key Key to look up.
     * @return The authenticated plaintext value.
     * @throws ConfigKeyNotFoundException if the key is absent.
     * @throws ConfigEncryptionException if the stored blob cannot be decrypted.
     */
    std::string get(const std::string& config_key) const;

    /**
     * @brief Retrieve a value when the key is present.
     * @param[in] config_key Key to look up.
     * @return The plaintext value, or std::nullopt only when the key is absent.
     * @throws ConfigEncryptionException if the stored blob cannot be decrypted.
     */
    std::optional<std::string> tryGet(const std::string& config_key) const;

    /**
     * @brief Removes an entry if present.
     * @param[in] config_key Key to remove.
     * @return True if an entry existed and was removed; otherwise false.
     */
    bool remove(const std::string& config_key);

    /**
     * @brief Checks whether an entry exists without decrypting it.
     * @param[in] config_key Key to look up.
     * @return True if the key is present; otherwise false.
     */
    bool contains(const std::string& config_key) const;

    /**
     * @brief Returns the keys of all stored entries.
     * @return A snapshot of the current key set.
     */
    std::vector<std::string> keys() const;

    /**
     * @brief Returns the number of stored entries.
     * @return The current entry count.
     */
    std::size_t size() const;

    /**
     * @brief Removes all stored entries.
     */
    void clear();

    // -------------------------------------------------------------------------
    // Key rotation
    // -------------------------------------------------------------------------

    /**
     * @brief Re-encrypt all entries with a newly generated key.
     * @return The new key version.
     * @throws ConfigEncryptionException if key generation or re-encryption fails;
     *         on failure the existing key and entries remain unchanged.
     * @note Rotation holds the exclusive store lock for the full operation, so
     *       concurrent reads and writes wait until rotation completes.
     */
    uint32_t rotateKey();

    /**
     * @brief Returns the version of the active encryption key.
     * @return The active key version.
     */
    uint32_t currentKeyVersion() const;

    // -------------------------------------------------------------------------
    // Serialisation / deserialisation
    // -------------------------------------------------------------------------

    /**
     * @brief Serializes the key and encrypted entries to JSON.
     * @return JSON snapshot containing the current key bytes and store entries.
     * @note The snapshot contains sensitive key material and is not ciphertext-only.
     */
    std::string serialize() const;

    /**
     * @brief Replaces the store state from a serialized JSON snapshot.
     * @param[in] json_str JSON snapshot previously produced by serialize().
     * @throws ConfigEncryptionException if the snapshot is malformed or invalid.
     */
    void deserialize(const std::string& json_str);

private:
    // ---- internal types -----

    struct KeyMaterial {
        uint32_t             version{0};
        std::vector<uint8_t> key_bytes; ///< 32 bytes (AES-256)
    };

    /**
     * @brief ---- helpers ----
     * @return Return value.
     */

    static std::vector<uint8_t> generateKey();

    /**
     * @brief Generate IV.
     * @return Return value.
     */
    static std::vector<uint8_t> generateIV();

    /**
     * @brief Aes Gcm Encrypt.
     * @param[in] plaintext Input parameter.
     * @param[in] key Input parameter.
     * @param[in,out] out_iv Input/output parameter.
     * @param[in,out] out_tag Input/output parameter.
     * @return Return value.
     */
    static std::vector<uint8_t> aesGcmEncrypt(
        const std::string&        plaintext,
        const std::vector<uint8_t>& key,
        std::vector<uint8_t>&     out_iv,
        std::vector<uint8_t>&     out_tag);

    /**
     * @brief Aes Gcm Decrypt.
     * @param[in] ciphertext Input parameter.
     * @param[in] key Input parameter.
     * @param[in] iv Input parameter.
     * @param[in] tag Input parameter.
     * @return Return value.
     */
    static std::string aesGcmDecrypt(
        const std::vector<uint8_t>& ciphertext,
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& iv,
        const std::vector<uint8_t>& tag);

    /**
     * @brief Encrypt Value.
     * @param[in] plaintext Input parameter.
     * @return Return value.
     */
    ConfigEncryptedBlob encryptValue(const std::string& plaintext) const;
    /**
     * @brief Decrypt Blob.
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
