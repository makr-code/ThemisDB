/**
 * @file pki_key_provider.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "security/encryption.h"
#include "utils/pki_client.h"
#include "storage/rocksdb_wrapper.h"
#include <memory>
#include <mutex>
#include <unordered_map>

namespace themis {
namespace security {

class PKIKeyProvider : public KeyProvider {
public:
    PKIKeyProvider(std::shared_ptr<utils::VCCPKIClient> pki,
                   std::shared_ptr<themis::RocksDBWrapper> db,
                   const std::string& service_id);
    
    PKIKeyProvider(const std::string& cert_path,
                   const std::string& private_key_path,
                   std::shared_ptr<themis::RocksDBWrapper> db,
                   const std::string& service_id,
                   bool validate_cert = true);
    
    // KeyProvider interface
    std::vector<uint8_t> getKey(const std::string& key_id) override;
    std::vector<uint8_t> getKey(const std::string& key_id, uint32_t version) override;
    uint32_t rotateKey(const std::string& key_id) override;
    std::vector<KeyMetadata> listKeys() override;
    KeyMetadata getKeyMetadata(const std::string& key_id, uint32_t version = 0) override;
    void deleteKey(const std::string& key_id, uint32_t version) override;
    bool hasKey(const std::string& key_id, uint32_t version = 0) override;
    uint32_t createKeyFromBytes(
        const std::string& key_id,
        const std::vector<uint8_t>& key_bytes,
        const KeyMetadata& metadata = KeyMetadata()) override;
    
    /**
     * @brief Rotate DEK.
     * @return Return value.
     */
    uint32_t rotateDEK();
    
    /**
     * @brief Get Current DEKVersion.
     * @return Return value.
     */
    uint32_t getCurrentDEKVersion() const;
    
    /**
     * @brief Get Group DEK.
     * @param[in] group_name Name of the group.
     * @return Return value.
     */
    std::vector<uint8_t> getGroupDEK(const std::string& group_name);
    
    /**
     * @brief Rotate Group DEK.
     * @param[in] group_name Name of the group.
     * @return Return value.
     */
    uint32_t rotateGroupDEK(const std::string& group_name);
    
    /**
     * @brief Get Group DEKVersion.
     * @param[in] group_name Name of the group.
     * @return Return value.
     */
    uint32_t getGroupDEKVersion(const std::string& group_name) const;
    
    /**
     * @brief List Groups.
     * @return Return value.
     */
    std::vector<std::string> listGroups() const;

private:
    /**
     * @brief Derive KEK.
     * @return Return value.
     */
    std::vector<uint8_t> deriveKEK();
    /**
     * @brief Load Or Create DEK.
     * @param[in] version Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> loadOrCreateDEK(uint32_t version);
    /**
     * @brief Derive Field Key.
     * @param[in] field_context Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> deriveFieldKey(const std::string& field_context);
    /**
     * @brief Dek Db Key.
     * @param[in] version Input parameter.
     * @return Return value.
     */
    std::string dekDbKey(uint32_t version) const;
    
    /**
     * @brief Load Or Create Group DEK.
     * @param[in] group_name Name of the group.
     * @param[in] version Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> loadOrCreateGroupDEK(const std::string& group_name, uint32_t version);
    /**
     * @brief Group Dek Db Key.
     * @param[in] group_name Name of the group.
     * @param[in] version Input parameter.
     * @return Return value.
     */
    std::string groupDekDbKey(const std::string& group_name, uint32_t version) const;
    /**
     * @brief Group Metadata Db Key.
     * @param[in] group_name Name of the group.
     * @return Return value.
     */
    std::string groupMetadataDbKey(const std::string& group_name) const;
    
    std::shared_ptr<utils::VCCPKIClient> pki_;
    std::shared_ptr<themis::RocksDBWrapper> db_;
    std::string service_id_;
    
    mutable std::mutex mu_;
    std::vector<uint8_t> kek_;
    std::unordered_map<uint32_t, std::vector<uint8_t>> dek_cache_; // version -> DEK
    std::unordered_map<std::string, std::vector<uint8_t>> field_key_cache_;
    uint32_t current_dek_version_ = 1;
    
    // Group DEK cache: group_name -> {version -> DEK}
    std::unordered_map<std::string, std::unordered_map<uint32_t, std::vector<uint8_t>>> group_dek_cache_;
    std::unordered_map<std::string, uint32_t> group_versions_; // group_name -> current_version
};

} // namespace security
} // namespace themis
