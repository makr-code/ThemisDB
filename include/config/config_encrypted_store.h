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
     * @brief To Json.
     * @return Return value.
     */
    std::string toJson() const;

    /**
     * @brief From Json.
     * @param[in] json_str Input parameter.
     * @return Return value.
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
     * @brief Set.
     * @param[in] config_key Input parameter.
     * @param[in] plaintext Input parameter.
     */
    void set(const std::string& config_key, const std::string& plaintext);

    /**
     * @brief Get.
     * @param[in] config_key Input parameter.
     * @return Return value.
     */
    std::string get(const std::string& config_key) const;

    /**
     * @brief Try Get.
     * @param[in] config_key Input parameter.
     * @return Return value.
     */
    std::optional<std::string> tryGet(const std::string& config_key) const;

    /**
     * @brief Remove.
     * @param[in] config_key Input parameter.
     * @return True when the operation succeeds.
     */
    bool remove(const std::string& config_key);

    /**
     * @brief Contains.
     * @param[in] config_key Input parameter.
     * @return True when the operation succeeds.
     */
    bool contains(const std::string& config_key) const;

    /**
     * @brief Keys.
     * @return Return value.
     */
    std::vector<std::string> keys() const;

    /**
     * @brief Size.
     * @return Return value.
     */
    std::size_t size() const;

    /**
     * @brief Clear.
     */
    void clear();

    // -------------------------------------------------------------------------
    // Key rotation
    // -------------------------------------------------------------------------

    /**
     * @brief Rotate Key.
     * @return Return value.
     */
    uint32_t rotateKey();

    /**
     * @brief Current Key Version.
     * @return Return value.
     */
    uint32_t currentKeyVersion() const;

    // -------------------------------------------------------------------------
    // Serialisation / deserialisation
    // -------------------------------------------------------------------------

    /**
     * @brief Serialize.
     * @return Return value.
     */
    std::string serialize() const;

    /**
     * @brief Deserialize.
     * @param[in] json_str Input parameter.
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
