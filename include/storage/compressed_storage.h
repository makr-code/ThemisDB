/**
 * @file compressed_storage.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "storage/compression_strategy.h"
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <unordered_map>
#include <mutex>

namespace themis {
namespace storage {

/**
 * @brief Storage value with compression metadata
 * 
 * Stores compressed data along with the method used for compression,
 * enabling transparent compression/decompression at the storage layer.
 */
struct CompressedValue {
    std::vector<uint8_t> data;
    compression::CompressionMethod method;
    size_t original_size = {};
    
    // Serialize to storage format: [method:1][original_size:8][data...]
    std::vector<uint8_t> serialize() const;
    
    // Deserialize from storage format
    static std::optional<CompressedValue> deserialize(const std::vector<uint8_t>& bytes);
};

/**
 * @brief Compression-aware storage interface
 * 
 * Wraps any key-value storage backend with transparent compression/decompression.
 * Automatically compresses values on write and decompresses on read.
 */
class CompressedStorageWrapper {
public:
    /**
     * @brief Storage backend interface
     * 
     * Implement this for your specific storage backend (RocksDB, LevelDB, etc.)
     */
    class IStorageBackend {
    public:
        virtual ~IStorageBackend() = default;
        
        [[nodiscard]] virtual bool put(const std::string& key, const std::vector<uint8_t>& value) = 0;
        [[nodiscard]] virtual std::optional<std::vector<uint8_t>> get(const std::string& key) = 0;
        [[nodiscard]] virtual bool del(const std::string& key) = 0;
        [[nodiscard]] virtual bool exists(const std::string& key) = 0;
    };
    
    /**
     * @brief Constructor
     * 
     * @param backend Storage backend implementation
     * @param config Compression configuration
     */
    CompressedStorageWrapper(
        std::shared_ptr<IStorageBackend> backend,
        const compression::CompressionConfig& config = compression::CompressionConfig{}
    );
    
    /**
     * @brief Store value with automatic compression
     * 
     * @param key Storage key
     * @param value Raw value to compress and store
     * @param hint Optional data type hint for adaptive compression
     * @return true if successful
     */
    bool put(
        const std::string& key,
        const std::vector<uint8_t>& value,
        std::optional<compression::DataType> hint = std::nullopt
    );
    
    /**
     * @brief Store string value with automatic compression
     */
    bool put(
        const std::string& key,
        const std::string& value,
        std::optional<compression::DataType> hint = std::nullopt
    ) {
        std::vector<uint8_t> bytes(value.begin(), value.end());
        return put(key, bytes, hint);
    }
    
    /**
     * @brief Retrieve and decompress value
     * 
     * @param key Storage key
     * @return Decompressed value, or nullopt if not found or decompression fails
     */
    std::optional<std::vector<uint8_t>> get(const std::string& key);
    
    /**
     * @brief Retrieve and decompress as string
     */
    std::optional<std::string> get_string(const std::string& key) {
        auto bytes = get(key);
        if (!bytes) {
          return std::nullopt;
        }
        return std::string(bytes->begin(), bytes->end());
    }
    
    /**
     * @brief Delete key
     */
    bool del(const std::string& key) {
        return backend_->del(key);
    }
    
    /**
     * @brief Check if key exists
     */
    bool exists(const std::string& key) {
        return backend_->exists(key);
    }
    
    /**
     * @brief Get compression statistics
     */
    std::string get_compression_stats() const {
        return compressor_.get_metrics();
    }
    
    /**
     * @brief Reset compression statistics
     */
    void reset_compression_stats() {
        compressor_.reset_metrics();
    }
    
    /**
     * @brief Update compression configuration
     */
    void set_compression_config(const compression::CompressionConfig& config) {
        compressor_.set_config(config);
    }
    
    /**
     * @brief Get current compression configuration
     */
    const compression::CompressionConfig& get_compression_config() const {
        return compressor_.get_config();
    }
    
private:
    std::shared_ptr<IStorageBackend> backend_;
    compression::CompressionStrategyManager compressor_;
};

/**
 * @brief Column-aware compressed storage
 * 
 * Allows different compression strategies for different columns/namespaces.
 * Each column can have its own compression configuration optimized for its data type.
 */
class ColumnCompressedStorage {
public:
    ColumnCompressedStorage(std::shared_ptr<CompressedStorageWrapper::IStorageBackend> backend);
    
    /**
     * @brief Configure compression for a specific column
     * 
     * @param column Column name/namespace
     * @param config Compression configuration for this column
     */
    void configure_column(
        const std::string& column,
        const compression::CompressionConfig& config
    );
    
    /**
     * @brief Store value in a specific column
     * 
     * @param column Column name/namespace
     * @param key Key within the column
     * @param value Value to store
     * @param hint Optional data type hint
     */
    bool put(
        const std::string& column,
        const std::string& key,
        const std::vector<uint8_t>& value,
        std::optional<compression::DataType> hint = std::nullopt
    );
    
    /**
     * @brief Retrieve value from a specific column
     * 
     * @param column Column name/namespace
     * @param key Key within the column
     * @return Decompressed value or nullopt
     */
    std::optional<std::vector<uint8_t>> get(
        const std::string& column,
        const std::string& key
    );
    
    /**
     * @brief Delete key from column
     */
    bool del(const std::string& column, const std::string& key);
    
    /**
     * @brief Get compression statistics for all columns
     */
    std::string get_all_column_stats() const;
    
    /**
     * @brief Get compression statistics for specific column
     */
    std::string get_column_stats(const std::string& column) const;
    
private:
    std::string make_full_key(const std::string& column, const std::string& key) const {
        return column + ":" + key;
    }
    
    std::shared_ptr<CompressedStorageWrapper::IStorageBackend> backend_;
    std::unordered_map<std::string, compression::CompressionConfig> column_configs_;
    std::unordered_map<std::string, std::unique_ptr<compression::CompressionStrategyManager>> column_compressors_;
    mutable std::mutex mutex_;
};

} // namespace storage
} // namespace themis
