/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            compressed_storage.cpp                             ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:10:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     253                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "storage/compressed_storage.h"
#include <cstring>

namespace themis {
namespace storage {

// ============================================================================
// CompressedValue Implementation
// ============================================================================

std::vector<uint8_t> CompressedValue::serialize() const {
    std::vector<uint8_t> result;
    result.reserve(1 + 8 + data.size());
    
    // Method (1 byte)
    result.push_back(static_cast<uint8_t>(method));
    
    // Original size (8 bytes, little-endian)
    uint64_t size = original_size;
    for (int i = 0; i < 8; ++i) {
        result.push_back(static_cast<uint8_t>((size >> (i * 8)) & 0xFF));
    }
    
    // Data
    result.insert(result.end(), data.begin(), data.end());
    
    return result;
}

std::optional<CompressedValue> CompressedValue::deserialize(const std::vector<uint8_t>& bytes) {
    if (bytes.size() < 9) {
        return std::nullopt; // Too small
    }
    
    CompressedValue result;
    
    // Read method
    result.method = static_cast<compression::CompressionMethod>(bytes[0]);
    
    // Read original size (little-endian)
    uint64_t size = 0;
    for (int i = 0; i < 8; ++i) {
        size |= static_cast<uint64_t>(bytes[1 + i]) << (i * 8);
    }
    result.original_size = static_cast<size_t>(size);
    
    // Read data
    result.data.assign(bytes.begin() + 9, bytes.end());
    
    return result;
}

// ============================================================================
// CompressedStorageWrapper Implementation
// ============================================================================

CompressedStorageWrapper::CompressedStorageWrapper(
    std::shared_ptr<IStorageBackend> backend,
    const compression::CompressionConfig& config
)
    : backend_(std::move(backend))
    , compressor_(config)
{}

bool CompressedStorageWrapper::put(
    const std::string& key,
    const std::vector<uint8_t>& value,
    std::optional<compression::DataType> hint
) {
    if (!backend_) return false;
    
    // Compress the value
    auto result = compressor_.compress(value, hint);
    
    // Create compressed value
    CompressedValue cv;
    cv.data = std::move(result.data);
    cv.method = result.method_used;
    cv.original_size = result.original_size;
    
    // Serialize and store
    auto serialized = cv.serialize();
    return backend_->put(key, serialized);
}

std::optional<std::vector<uint8_t>> CompressedStorageWrapper::get(const std::string& key) {
    if (!backend_) return std::nullopt;
    
    // Retrieve serialized data
    auto serialized = backend_->get(key);
    if (!serialized) return std::nullopt;
    
    // Deserialize
    auto cv = CompressedValue::deserialize(*serialized);
    if (!cv) return std::nullopt;
    
    // Decompress
    if (cv->method == compression::CompressionMethod::NONE) {
        return cv->data;
    }
    
    auto decompressed = compressor_.decompress(cv->data, cv->method);
    if (decompressed.empty() && cv->original_size > 0) {
        return std::nullopt; // Decompression failed
    }
    
    return decompressed;
}

// ============================================================================
// ColumnCompressedStorage Implementation
// ============================================================================

ColumnCompressedStorage::ColumnCompressedStorage(
    std::shared_ptr<CompressedStorageWrapper::IStorageBackend> backend
)
    : backend_(std::move(backend))
{}

void ColumnCompressedStorage::configure_column(
    const std::string& column,
    const compression::CompressionConfig& config
) {
    std::lock_guard<std::mutex> lock(mutex_);
    column_configs_[column] = config;
    column_compressors_[column] = std::make_unique<compression::CompressionStrategyManager>(config);
}

bool ColumnCompressedStorage::put(
    const std::string& column,
    const std::string& key,
    const std::vector<uint8_t>& value,
    std::optional<compression::DataType> hint
) {
    if (!backend_) return false;
    
    // Get or create compressor for this column
    compression::CompressionStrategyManager* compressor = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = column_compressors_.find(column);
        if (it == column_compressors_.end()) {
            // Create default compressor for this column
            column_configs_[column] = compression::CompressionConfig{};
            column_compressors_[column] = std::make_unique<compression::CompressionStrategyManager>();
        }
        compressor = column_compressors_[column].get();
    }
    
    // Compress
    auto result = compressor->compress(value, hint);
    
    // Create compressed value
    CompressedValue cv;
    cv.data = std::move(result.data);
    cv.method = result.method_used;
    cv.original_size = result.original_size;
    
    // Serialize and store
    auto serialized = cv.serialize();
    return backend_->put(make_full_key(column, key), serialized);
}

std::optional<std::vector<uint8_t>> ColumnCompressedStorage::get(
    const std::string& column,
    const std::string& key
) {
    if (!backend_) return std::nullopt;
    
    // Retrieve serialized data
    auto serialized = backend_->get(make_full_key(column, key));
    if (!serialized) return std::nullopt;
    
    // Deserialize
    auto cv = CompressedValue::deserialize(*serialized);
    if (!cv) return std::nullopt;
    
    // Decompress
    if (cv->method == compression::CompressionMethod::NONE) {
        return cv->data;
    }
    
    // Get compressor for this column
    compression::CompressionStrategyManager* compressor = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = column_compressors_.find(column);
        if (it == column_compressors_.end()) {
            // Create default compressor
            column_compressors_[column] = std::make_unique<compression::CompressionStrategyManager>();
        }
        compressor = column_compressors_[column].get();
    }
    
    auto decompressed = compressor->decompress(cv->data, cv->method);
    if (decompressed.empty() && cv->original_size > 0) {
        return std::nullopt; // Decompression failed
    }
    
    return decompressed;
}

bool ColumnCompressedStorage::del(const std::string& column, const std::string& key) {
    if (!backend_) return false;
    return backend_->del(make_full_key(column, key));
}

std::string ColumnCompressedStorage::get_all_column_stats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string result = "=== Column Compression Statistics ===\n\n";
    
    for (const auto& pair : column_compressors_) {
        result += "Column: " + pair.first + "\n";
        result += pair.second->get_metrics();
        result += "\n";
    }
    
    return result;
}

std::string ColumnCompressedStorage::get_column_stats(const std::string& column) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = column_compressors_.find(column);
    if (it == column_compressors_.end()) {
        return "Column '" + column + "' not found or has no statistics.";
    }
    return it->second->get_metrics();
}

} // namespace storage
} // namespace themis
