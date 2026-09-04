/**
 * @file compressed_storage.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "storage/compressed_storage.h"
#include <cstring>
#include <array>

namespace themis {
namespace storage {

// ─────────────────────────────────────────────────────────────────────────────
// CRC32 helper (IEEE polynomial, no external dependency)
// ─────────────────────────────────────────────────────────────────────────────
namespace {

static uint32_t cv_crc32(const void* data, size_t len) {
    static const auto table = []() {
        std::array<uint32_t, 256> t{};
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k) {
              c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            }
            t[i] = c;
        }
        return t;
    }();
    uint32_t crc = 0xFFFFFFFFu;
    const auto* p = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < len; ++i) {
      crc = table[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);
    }
    return ~crc;
}

} // anonymous namespace

// ============================================================================
// CompressedValue Implementation
// ============================================================================

std::vector<uint8_t> CompressedValue::serialize() const {
    std::vector<uint8_t> result = {};

    result.reserve(1 + 8 + data.size() + 4);

    // Method (1 byte)
    result.push_back(static_cast<uint8_t>(method));

    // Original size (8 bytes, little-endian)
    uint64_t size = original_size;
    for (int i = 0; i < 8; ++i) {
        result.push_back(static_cast<uint8_t>((size >> (i * 8)) & 0xFF));
    }

    // Data
    result.insert(result.end(), data.begin(), data.end());

    // CRC32 of all previous bytes (4 bytes, little-endian)
    uint32_t crc = cv_crc32(result.data(), result.size());
    for (int i = 0; i < 4; ++i) {
      result.push_back(static_cast<uint8_t>(crc >> (8 * i)));
    }

    return result;
}

std::optional<CompressedValue> CompressedValue::deserialize(const std::vector<uint8_t>& bytes) {
    // Minimum: 1 (method) + 8 (size) + 4 (CRC) = 13
    constexpr size_t kMinWithCrc = 13;
    constexpr size_t kMinLegacy  = 9;

    if (bytes.size() < kMinLegacy) {
        return std::nullopt; // Too small even for legacy format
    }

    // Determine payload boundary: if we have enough bytes for the CRC trailer,
    // verify it.  Legacy records (< 13 bytes or mismatching CRC) fall through.
    size_t payload_end = bytes.size();
    if (bytes.size() >= kMinWithCrc) {
        const size_t crc_off = bytes.size() - 4;
        uint32_t stored_crc  = 0;
        for (int i = 0; i < 4; ++i)
            stored_crc |= (static_cast<uint32_t>(bytes[crc_off + i]) << (8 * i));
        uint32_t computed = cv_crc32(bytes.data(), crc_off);
        if (computed == stored_crc) {
            payload_end = crc_off; // CRC verified — strip trailer
        }
        // If mismatch, treat as legacy (no CRC) and parse full bytes.
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

    // Read compressed data (between fixed header and payload boundary)
    if (payload_end < 9) {
      return std::nullopt;
    }
    result.data.assign(bytes.begin() + 9, bytes.begin() + static_cast<std::ptrdiff_t>(payload_end));

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
    if (!backend_) {
      return false;
    }
    
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
    if (!backend_) {
      return std::nullopt;
    }
    
    // Retrieve serialized data
    auto serialized = backend_->get(key);
    if (!serialized) {
      return std::nullopt;
    }
    
    // Deserialize
    auto cv = CompressedValue::deserialize(*serialized);
    if (!cv) {
      return std::nullopt;
    }
    
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
    if (!backend_) {
      return false;
    }
    
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
    if (!backend_) {
      return std::nullopt;
    }
    
    // Retrieve serialized data
    auto serialized = backend_->get(make_full_key(column, key));
    if (!serialized) {
      return std::nullopt;
    }
    
    // Deserialize
    auto cv = CompressedValue::deserialize(*serialized);
    if (!cv) {
      return std::nullopt;
    }
    
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
    if (!backend_) {
      return false;
    }
    return backend_->del(make_full_key(column, key));
}

std::string ColumnCompressedStorage::get_all_column_stats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string result = "=== Column Compression Statistics ===\n\n";
    
    for (const auto& pair : column_compressors_) {
        const auto metrics = pair.second->get_metrics();
        result.reserve(result.size() + 8 + pair.first.size() + 1 + metrics.size() + 1);
        result.append("Column: ");
        result.append(pair.first);
        result.push_back('\n');
        result.append(metrics);
        result.push_back('\n');
    }
    
    return result;
}

std::string ColumnCompressedStorage::get_column_stats(const std::string& column) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = column_compressors_.find(column);
    if (it == column_compressors_.end()) {
        std::string message = {};
        message.reserve(39 + column.size());
        message.append("Column '");
        message.append(column);
        message.append("' not found or has no statistics.");
        return message;
    }
    return it->second->get_metrics();
}

} // namespace storage
} // namespace themis
