/**
 * @file pki_key_provider.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Production KeyProvider with PKI-based 3-tier key hierarchy
 * 
 * Key Hierarchy:
 * 1. KEK (Key Encryption Key) - Derived from VCC-PKI service certificate
 * 2. DEK (Data Encryption Key) - Random 256-bit AES key, encrypted with KEK
 * 3. Field Keys - Derived from DEK using HKDF with field-specific context
 * 
 * Advantages:
 * - KEK rotation: Update certificate, re-encrypt DEK (no data re-encryption)
 * - DEK rotation: Generate new DEK, re-encrypt data (lazy migration possible)
 * - Per-field keys: Derived on-demand, no storage overhead
 */
class PKIKeyProvider : public KeyProvider {
public:
    /**
     * @brief Initialize with PKI client and persistent storage
     * @param pki VCC-PKI client for certificate operations
     * @param db RocksDB for encrypted DEK storage
     * @param service_id Service identifier for certificate lookup
     */
    PKIKeyProvider(std::shared_ptr<utils::VCCPKIClient> pki,
                   std::shared_ptr<themis::RocksDBWrapper> db,
                   const std::string& service_id);
    
    /**
     * @brief Initialize with certificate file paths (file-based mode)
     * 
     * This constructor enables certificate-based key derivation without requiring
     * a VCC-PKI service. Keys are derived from the certificate's public key using HKDF.
     * 
     * @param cert_path Path to X.509 certificate file (PEM format)
     * @param private_key_path Path to private key file (PEM format) - optional, used for validation
     * @param db RocksDB for encrypted DEK storage
     * @param service_id Service identifier for key derivation context
     * @param validate_cert If true, validate certificate expiration and properties
     * @throws std::runtime_error if certificate cannot be loaded or is invalid
     */
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
     * @brief Rotate DEK (generates new DEK, marks old as deprecated)
     * @return New DEK version number
     */
    uint32_t rotateDEK();
    
    /**
     * @brief Get current DEK version
     */
    uint32_t getCurrentDEKVersion() const;
    
    /**
     * @brief Get or create Group DEK for multi-party access
     * @param group_name Group identifier (e.g., "hr_team", "finance_dept")
     * @return Group-specific DEK (32 bytes, AES-256)
     * 
     * Group DEKs enable multiple users to decrypt the same data.
     * Encrypted with KEK and stored in DB under key:group:{group_name}
     */
    std::vector<uint8_t> getGroupDEK(const std::string& group_name);
    
    /**
     * @brief Rotate Group DEK (invalidates old key, requires data re-encryption)
     * @param group_name Group identifier
     * @return New DEK version for this group
     * 
     * Use case: When a group member leaves and should lose access
     */
    uint32_t rotateGroupDEK(const std::string& group_name);
    
    /**
     * @brief Get current version of Group DEK
     * @param group_name Group identifier
     * @return Current version number (0 if group doesn't exist)
     */
    uint32_t getGroupDEKVersion(const std::string& group_name) const;
    
    /**
     * @brief List all groups with DEKs
     * @return Vector of group names
     */
    std::vector<std::string> listGroups() const;

private:
    std::vector<uint8_t> deriveKEK();
    std::vector<uint8_t> loadOrCreateDEK(uint32_t version);
    std::vector<uint8_t> deriveFieldKey(const std::string& field_context);
    std::string dekDbKey(uint32_t version) const;
    
    // Group DEK helpers
    std::vector<uint8_t> loadOrCreateGroupDEK(const std::string& group_name, uint32_t version);
    std::string groupDekDbKey(const std::string& group_name, uint32_t version) const;
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
