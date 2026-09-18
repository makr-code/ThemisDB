/**
 * @file mock_key_provider.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "security/mock_key_provider.h"
#include "utils/logger.h"
#include <chrono>
#include <sstream>
#include <stdexcept>

namespace themis {

MockKeyProvider::MockKeyProvider() {
    // Seed random number generator
    std::random_device rd = {};
    rng_.seed(rd());

    THEMIS_WARN("╔═══════════════════════════════════════════════════════════════╗");
    THEMIS_WARN("║  ⚠️  INSECURE CONFIGURATION: MockKeyProvider ACTIVE!  ⚠️      ║");
    THEMIS_WARN("╠═══════════════════════════════════════════════════════════════╣");
    THEMIS_WARN("║  Encryption keys are stored in PROCESS MEMORY only.          ║");
    THEMIS_WARN("║  Keys are NOT persisted and will be lost on restart.          ║");
    THEMIS_WARN("║  This configuration is for DEVELOPMENT / TESTING ONLY.       ║");
    THEMIS_WARN("║                                                               ║");
    THEMIS_WARN("║  Production deployments MUST use a real KeyProvider:          ║");
    THEMIS_WARN("║  - PKIKeyProvider (TLS-backed PKI)                            ║");
    THEMIS_WARN("║  - HSMKeyProvider (hardware security module)                  ║");
    THEMIS_WARN("║  - VaultKeyProvider (HashiCorp Vault / cloud KMS)             ║");
    THEMIS_WARN("╚═══════════════════════════════════════════════════════════════╝");
}

/**
 * @brief Create Key.
 * @param[in] key_id Identifier of the key.
 * @param[in] version Input parameter.
 * @throws KeyOperationException if an error occurs.
 * @details Calls: lock(), count(), std::to_string(), generateRandomKey(), getCurrentTimeMs(), std::move().
 */
void MockKeyProvider::createKey(const std::string& key_id, uint32_t version) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (keys_.count(key_id) && keys_[key_id].count(version)) {
        throw KeyOperationException("Key already exists: " + key_id + " v" + std::to_string(version));
    }
    
    KeyEntry entry;
    entry.key = generateRandomKey();
    entry.metadata.key_id = key_id;
    entry.metadata.version = version;
    entry.metadata.algorithm = "AES-256-GCM";
    entry.metadata.created_at_ms = getCurrentTimeMs();
    entry.metadata.expires_at_ms = 0;  // Never expires
    entry.metadata.status = KeyStatus::ACTIVE;
    
    keys_[key_id][version] = std::move(entry);
}

/**
 * @brief Create Key With Bytes.
 * @param[in] key_id Identifier of the key.
 * @param[in] version Input parameter.
 * @param[in] key_bytes Input parameter.
 * @throws std::invalid_argument if an error occurs.
 * @throws KeyOperationException if an error occurs.
 * @details Calls: size(), lock(), count(), std::to_string(), getCurrentTimeMs(), std::move().
 */
void MockKeyProvider::createKeyWithBytes(const std::string& key_id,
                                        uint32_t version,
                                        const std::vector<uint8_t>& key_bytes) {
    if (key_bytes.size() != 32) {
        throw std::invalid_argument("Key must be exactly 32 bytes (256 bits)");
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (keys_.count(key_id) && keys_[key_id].count(version)) {
        throw KeyOperationException("Key already exists: " + key_id + " v" + std::to_string(version));
    }
    
    KeyEntry entry;
    entry.key = key_bytes;
    entry.metadata.key_id = key_id;
    entry.metadata.version = version;
    entry.metadata.algorithm = "AES-256-GCM";
    entry.metadata.created_at_ms = getCurrentTimeMs();
    entry.metadata.expires_at_ms = 0;
    entry.metadata.status = KeyStatus::ACTIVE;
    
    keys_[key_id][version] = std::move(entry);
}

/**
 * @brief Get Key.
 * @param[in] key_id Identifier of the key.
 * @return Return value.
 * @throws KeyNotFoundException if an error occurs.
 * @throws KeyOperationException if an error occurs.
 * @details Calls: lock(), count(), empty().
 */
std::vector<uint8_t> MockKeyProvider::getKey(const std::string& key_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!keys_.count(key_id) || keys_[key_id].empty()) {
        throw KeyNotFoundException(key_id, 0);
    }
    
    // Find latest ACTIVE key
    uint32_t latest_version = 0;
    for (const auto& [version, entry] : keys_[key_id]) {
        if (entry.metadata.status == KeyStatus::ACTIVE && version > latest_version) {
            latest_version = version;
        }
    }
    
    if (latest_version == 0) {
        throw KeyOperationException("No ACTIVE key found for: " + key_id);
    }
    
    return keys_[key_id][latest_version].key;
}

/**
 * @brief Get Key.
 * @param[in] key_id Identifier of the key.
 * @param[in] version Input parameter.
 * @return Return value.
 * @throws KeyNotFoundException if an error occurs.
 * @throws KeyOperationException if an error occurs.
 * @details Calls: lock(), count(), std::to_string().
 */
std::vector<uint8_t> MockKeyProvider::getKey(const std::string& key_id, uint32_t version) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!keys_.count(key_id) || !keys_[key_id].count(version)) {
        throw KeyNotFoundException(key_id, version);
    }
    
    const auto& entry = keys_[key_id][version];
    
    if (entry.metadata.status == KeyStatus::DELETED) {
        throw KeyOperationException("Key is deleted: " + key_id + " v" + std::to_string(version));
    }
    
    return entry.key;
}

/**
 * @brief Rotate Key.
 * @param[in] key_id Identifier of the key.
 * @return Return value.
 * @throws KeyNotFoundException if an error occurs.
 * @details Calls: lock(), count(), empty(), generateRandomKey(), getCurrentTimeMs(), std::move().
 */
uint32_t MockKeyProvider::rotateKey(const std::string& key_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!keys_.count(key_id) || keys_[key_id].empty()) {
        throw KeyNotFoundException(key_id, 0);
    }
    
    // Find current latest version
    uint32_t max_version = 0;
    for (const auto& [version, entry] : keys_[key_id]) {
        if (version > max_version) {
            max_version = version;
        }
    }
    
    uint32_t new_version = max_version + 1;
    
    // Mark old ACTIVE keys as DEPRECATED
    for (auto& [version, entry] : keys_[key_id]) {
        if (entry.metadata.status == KeyStatus::ACTIVE) {
            entry.metadata.status = KeyStatus::DEPRECATED;
        }
    }
    
    // Create new key
    KeyEntry new_entry;
    new_entry.key = generateRandomKey();
    new_entry.metadata.key_id = key_id;
    new_entry.metadata.version = new_version;
    new_entry.metadata.algorithm = "AES-256-GCM";
    new_entry.metadata.created_at_ms = getCurrentTimeMs();
    new_entry.metadata.expires_at_ms = 0;
    new_entry.metadata.status = KeyStatus::ACTIVE;
    
    keys_[key_id][new_version] = std::move(new_entry);
    
    return new_version;
}

/**
 * @brief List Keys.
 * @return Return value.
 * @details Calls: lock(), push_back().
 */
std::vector<KeyMetadata> MockKeyProvider::listKeys() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<KeyMetadata> result;
    
    for (const auto& [key_id, versions] : keys_) {
        for (const auto& [version, entry] : versions) {
            result.push_back(entry.metadata);
        }
    }
    
    return result;
}

/**
 * @brief Get Key Metadata.
 * @param[in] key_id Identifier of the key.
 * @param[in] version Input parameter.
 * @return Return value.
 * @throws KeyNotFoundException if an error occurs.
 * @throws KeyOperationException if an error occurs.
 * @details Calls: lock(), count().
 */
KeyMetadata MockKeyProvider::getKeyMetadata(const std::string& key_id, uint32_t version) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!keys_.count(key_id)) {
        throw KeyNotFoundException(key_id, version);
    }
    
    if (version == 0) {
        // Get latest ACTIVE
        uint32_t latest_version = 0;
        for (const auto& [v, entry] : keys_[key_id]) {
            if (entry.metadata.status == KeyStatus::ACTIVE && v > latest_version) {
                latest_version = v;
            }
        }
        
        if (latest_version == 0) {
            throw KeyOperationException("No ACTIVE key found for: " + key_id);
        }
        
        return keys_[key_id][latest_version].metadata;
    } else {
        if (!keys_[key_id].count(version)) {
            throw KeyNotFoundException(key_id, version);
        }
        return keys_[key_id][version].metadata;
    }
}

/**
 * @brief Delete Key.
 * @param[in] key_id Identifier of the key.
 * @param[in] version Input parameter.
 * @throws KeyNotFoundException if an error occurs.
 * @throws KeyOperationException if an error occurs.
 * @details Calls: lock(), count(), std::to_string(), clear().
 */
void MockKeyProvider::deleteKey(const std::string& key_id, uint32_t version) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!keys_.count(key_id) || !keys_[key_id].count(version)) {
        throw KeyNotFoundException(key_id, version);
    }
    
    auto& entry = keys_[key_id][version];
    
    if (entry.metadata.status == KeyStatus::ACTIVE) {
        throw KeyOperationException("Cannot delete ACTIVE key: " + key_id + " v" + std::to_string(version));
    }
    
    entry.metadata.status = KeyStatus::DELETED;
    entry.key.clear();  // Erase key material
}

/**
 * @brief Has Key.
 * @param[in] key_id Identifier of the key.
 * @param[in] version Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: lock(), count(), empty().
 */
bool MockKeyProvider::hasKey(const std::string& key_id, uint32_t version) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (version == 0) {
        // Check if any version exists
        return keys_.count(key_id) > 0 && !keys_[key_id].empty();
    }
    
    // Check specific version
    return keys_.count(key_id) > 0 && keys_[key_id].count(version) > 0;
}

/**
 * @brief Create Key From Bytes.
 * @param[in] key_id Identifier of the key.
 * @param[in] key_bytes Input parameter.
 * @param[in] metadata Input parameter.
 * @return Return value.
 * @throws std::invalid_argument if an error occurs.
 * @details Calls: size(), lock(), find(), end(), empty(), getCurrentTimeMs().
 */
uint32_t MockKeyProvider::createKeyFromBytes(
    const std::string& key_id,
    const std::vector<uint8_t>& key_bytes,
    const KeyMetadata& metadata) {
    
    if (key_bytes.size() != 32) {
        throw std::invalid_argument("Key must be 32 bytes for AES-256");
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    // Compute next version without calling getLatestVersion (would re-lock mutex_)
    uint32_t new_version = 1;
    auto it = keys_.find(key_id);
    if (it != keys_.end() && !it->second.empty()) {
        uint32_t max_version = 0;
        for (const auto& [v, _] : it->second) {
            if (v > max_version) {
              max_version = v;
            }
        }
        new_version = max_version + 1;
    }
    
    KeyEntry entry;
    entry.key = key_bytes;
    entry.metadata = metadata;
    entry.metadata.key_id = key_id;
    entry.metadata.version = new_version;
    
    if (entry.metadata.algorithm.empty()) {
        entry.metadata.algorithm = "AES-256-GCM";
    }
    if (entry.metadata.created_at_ms == 0) {
        entry.metadata.created_at_ms = getCurrentTimeMs();
    }
    
    keys_[key_id][new_version] = entry;
    
    return new_version;
}

uint32_t MockKeyProvider::getLatestVersion(const std::string& key_id) const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!keys_.count(key_id) || keys_.at(key_id).empty()) {
        return 0;
    }
    
    uint32_t max_version = 0;
    for (const auto& [version, entry] : keys_.at(key_id)) {
        if (version > max_version) {
            max_version = version;
        }
    }
    
    return max_version;
}

/**
 * @brief Clear.
 * @details Calls: lock().
 */
void MockKeyProvider::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    keys_.clear();
}

/**
 * @brief Generate Random Key.
 * @return Return value.
 * @details Calls: key(), dist().
 */
std::vector<uint8_t> MockKeyProvider::generateRandomKey() {
    std::vector<uint8_t> key(32);  // 256 bits
    
    std::uniform_int_distribution<uint16_t> dist(0, 255);
    for (auto& byte : key) {
        byte = static_cast<uint8_t>(dist(rng_));
    }
    
    return key;
}

std::string MockKeyProvider::makeKeyPath(const std::string& key_id, uint32_t version) const {
    return key_id + ":" + std::to_string(version);
}

int64_t MockKeyProvider::getCurrentTimeMs() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

}  // namespace themis
