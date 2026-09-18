/**
 * @file blob_storage_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "storage/blob_storage_backend.h"
#include <unordered_map>
#include <mutex>

namespace themis {
namespace storage {

/**
 * @brief Blob Storage Manager
 * 
 * Orchestrates multiple blob storage backends and automatically selects
 * the appropriate backend based on blob size and configuration.
 * 
 * Selection Strategy:
 * - < inline_threshold_bytes: INLINE (stored in RocksDB)
 * - < rocksdb_blob_threshold_bytes: ROCKSDB_BLOB (RocksDB BlobDB)
 * - >= rocksdb_blob_threshold_bytes: External backend (Filesystem/S3/Azure/WebDAV)
 * 
 * Thread-Safety: All methods are thread-safe.
 */
class BlobStorageManager {
private:
    BlobStorageConfig config_;
    std::unordered_map<BlobStorageType, std::shared_ptr<IBlobStorageBackend>> backends_;
    mutable std::mutex mutex_;
    
    BlobStorageType selectBackendType(size_t blob_size) const {
        if (blob_size < static_cast<size_t>(config_.inline_threshold_bytes)) {
            return BlobStorageType::INLINE;
        }
        
        if (blob_size < static_cast<size_t>(config_.rocksdb_blob_threshold_bytes)) {
            return BlobStorageType::ROCKSDB_BLOB;
        }
        
        // Select external backend (prefer enabled backends in order)
        if (config_.enable_webdav) {
            return BlobStorageType::WEBDAV;
        }
        if (config_.enable_s3) {
            return BlobStorageType::S3;
        }
        if (config_.enable_azure) {
            return BlobStorageType::AZURE_BLOB;
        }
        if (config_.enable_gcs) {
            return BlobStorageType::GCS;
        }
        if (config_.enable_filesystem) {
            return BlobStorageType::FILESYSTEM;
        }
        
        // Default to filesystem
        return BlobStorageType::FILESYSTEM;
    }
    
public:
    /**
     * @brief TBD: Describe BlobStorageManager.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit BlobStorageManager(const BlobStorageConfig& config)
        : config_(config) {}
    
    /**
     * @brief Register a blob storage backend
     * @param type Backend type
     * @param backend Backend implementation
     * @details Calls: lock().
     */
    void registerBackend(BlobStorageType type, std::shared_ptr<IBlobStorageBackend> backend) {
        /**
         * @brief TBD: Describe lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        backends_[type] = backend;
    }
    
    /**
     * @brief Store a blob with automatic backend selection
     * @param blob_id Unique blob identifier
     * @param data Blob data
     * @return BlobRef Reference to stored blob
     * @throws std::runtime_error if no suitable backend available
     * @details Calls: selectBackendType(), size(), lock(), find(), end(), isAvailable(), has_value(), error().
     */
    BlobRef put(const std::string& blob_id, const std::vector<uint8_t>& data) {
        BlobStorageType target_type = selectBackendType(data.size());
        
        std::shared_ptr<IBlobStorageBackend> backend;
        {
            /**
             * @brief TBD: Describe lock.
             * @param[in] mutex_ Input parameter.
             * @return Return value.
             */
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = backends_.find(target_type);
            if (it != backends_.end() && it->second && it->second->isAvailable()) {
                backend = it->second;
            } else {
                // Fallback to filesystem
                auto fs_it = backends_.find(BlobStorageType::FILESYSTEM);
                if (fs_it != backends_.end() && fs_it->second && fs_it->second->isAvailable()) {
                    backend = fs_it->second;
                    target_type = BlobStorageType::FILESYSTEM;
                }
            }
        }
        
        if (!backend) {
            throw std::runtime_error("No suitable blob storage backend available");
        }
        
        auto result = backend->put(blob_id, data);
        if (!result.has_value()) {
            throw std::runtime_error(result.error().message());
        }
        return result.value();
    }
    
    /**
     * @brief Retrieve a blob
     * @param ref Blob reference
     * @return Blob data or nullopt if not found
     * @throws std::runtime_error if an error occurs.
     * @details Calls: lock(), find(), end(), std::to_string(), has_value(), value().
     */
    std::optional<std::vector<uint8_t>> get(const BlobRef& ref) {
        std::shared_ptr<IBlobStorageBackend> backend;
        {
            /**
             * @brief TBD: Describe lock.
             * @param[in] mutex_ Input parameter.
             * @return Return value.
             */
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = backends_.find(ref.type);
            if (it != backends_.end()) {
                backend = it->second;
            }
        }
        
        if (!backend) {
            throw std::runtime_error(
                "Backend not registered for type: " + std::to_string(static_cast<int>(ref.type))
            );
        }
        
        auto result = backend->get(ref);
        if (!result.has_value()) {
            return std::nullopt;
        }
        return result.value();
    }
    
    /**
     * @brief Delete a blob
     * @param ref Blob reference
     * @return true if deleted
     * @details Calls: lock(), find(), end(), has_value().
     */
    bool remove(const BlobRef& ref) {
        std::shared_ptr<IBlobStorageBackend> backend;
        {
            /**
             * @brief TBD: Describe lock.
             * @param[in] mutex_ Input parameter.
             * @return Return value.
             */
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = backends_.find(ref.type);
            if (it != backends_.end()) {
                backend = it->second;
            }
        }
        
        if (!backend) {
            return false;
        }
        
        auto result = backend->remove(ref);
        if (!result.has_value()) {
            return false;
        }
        return true;
    }
    
    /**
     * @brief Check if blob exists
     * @param ref Blob reference
     * @return true if exists
     * @details Calls: lock(), find(), end().
     */
    bool exists(const BlobRef& ref) {
        std::shared_ptr<IBlobStorageBackend> backend;
        {
            /**
             * @brief TBD: Describe lock.
             * @param[in] mutex_ Input parameter.
             * @return Return value.
             */
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = backends_.find(ref.type);
            if (it != backends_.end()) {
                backend = it->second;
            }
        }
        
        if (!backend) {
            return false;
        }
        
        return backend->exists(ref);
    }
    
    /**
     * @brief Get configuration
     */
    const BlobStorageConfig& getConfig() const {
        return config_;
    }
    
    /**
     * @brief Get registered backend types
     */
    std::vector<BlobStorageType> getRegisteredBackends() const {
        /**
         * @brief TBD: Describe lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<BlobStorageType> types = {};

        for (const auto& pair : backends_) {
            types.push_back(pair.first);
        }
        return types;
    }
};

} // namespace storage
} // namespace themis

