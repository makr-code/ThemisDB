/**
 * @file compression_strategy.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=2, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "storage/compression_strategy.h"
#include "utils/compression_metrics.h"
#include "utils/zstd_codec.h"
#include "utils/logger.h"
#include <algorithm>
#include <unordered_map>
#include <cstring>
#include <cctype>
#include <limits>
namespace themis {
namespace compression {

// ============================================================================
// CompressionStrategyManager Implementation
// ============================================================================

CompressionStrategyManager::CompressionStrategyManager(const CompressionConfig& config)
    : config_(config)
{}

CompressionResult CompressionStrategyManager::compress(
    const uint8_t* data,
    size_t size,
    std::optional<DataType> hint
) {
    CompressionResult result;
    result.original_size = size;
    
    // Don't compress small data
    if (size < config_.min_size) {
        result.data.assign(data, data + size);
        result.method_used = CompressionMethod::NONE;
        result.compression_ratio = 1.0f;
        result.success = true;
        return result;
    }
    
    // Determine data type
    DataType type = hint.value_or(config_.data_type);
    if (type == DataType::GENERIC && config_.method == CompressionMethod::ADAPTIVE) {
        type = detect_data_type(data, std::min(size, config_.adaptive_sample_size));
    }
    
    // Select compression method
    CompressionMethod method = config_.method;
    if (method == CompressionMethod::ADAPTIVE) {
        method = select_method(data, size, type);
    }
    
    // Apply compression
    utils::CompressionTimer timer(method_to_string(method), size, true);
    
    switch (method) {
        case CompressionMethod::ZSTD:
            result = compress_zstd(data, size);
            break;
        case CompressionMethod::RLE:
            result = compress_rle(data, size);
            break;
        case CompressionMethod::DELTA:
            result = compress_delta(data, size);
            break;
        case CompressionMethod::DICTIONARY:
            result = compress_dictionary(data, size);
            break;
        case CompressionMethod::GPU_ZSTD:
            result = compress_gpu_zstd(data, size);
            break;
        case CompressionMethod::GPU_SNAPPY:
            result = compress_gpu_snappy(data, size);
            break;
        case CompressionMethod::GPU_LZ4:
            result = compress_gpu_lz4(data, size);
            break;
        default:
            // Fallback: no compression
            result.data.assign(data, data + size);
            result.method_used = CompressionMethod::NONE;
            result.compression_ratio = 1.0f;
            result.success = true;
            return result;
    }
    
    if (config_.enable_metrics && result.success) {
        timer.finish(result.data.size());
    }
    
    // Check if compression was beneficial
    if (result.data.size() >= size * 0.95f) {
        // Less than 5% savings, store uncompressed
        result.data.assign(data, data + size);
        result.method_used = CompressionMethod::NONE;
        result.compression_ratio = 1.0f;
    }
    
    return result;
}

std::vector<uint8_t> CompressionStrategyManager::decompress(
    const std::vector<uint8_t>& data,
    CompressionMethod method
) {
    if (method == CompressionMethod::NONE || data.empty()) {
        return data;
    }
    
    utils::CompressionTimer timer(method_to_string(method), data.size(), false);
    std::vector<uint8_t> result;
    
    switch (method) {
        case CompressionMethod::ZSTD:
            result = decompress_zstd(data);
            break;
        case CompressionMethod::RLE:
            result = decompress_rle(data);
            break;
        case CompressionMethod::DELTA:
            result = decompress_delta(data);
            break;
        case CompressionMethod::DICTIONARY:
            result = decompress_dictionary(data);
            break;
        case CompressionMethod::GPU_ZSTD:
            result = decompress_gpu_zstd(data);
            break;
        case CompressionMethod::GPU_SNAPPY:
            result = decompress_gpu_snappy(data);
            break;
        case CompressionMethod::GPU_LZ4:
            result = decompress_gpu_lz4(data);
            break;
        default:
            result = data;
            break;
    }
    
    if (config_.enable_metrics && !result.empty()) {
        timer.finish(result.size());
    }
    
    return result;
}

CompressionMethod CompressionStrategyManager::select_method(
    const uint8_t* /*data*/,
    size_t /*size*/,
    DataType type
) {
    switch (type) {
        case DataType::TEXT:
        [[fallthrough]];\n        case DataType::JSON:
            return CompressionMethod::ZSTD;
            
        case DataType::VECTOR_SPARSE:
            return CompressionMethod::RLE;
            
        case DataType::INTEGER_SEQ:
        [[fallthrough]];\n        case DataType::TIMESERIES:
            return CompressionMethod::DELTA;
            
        case DataType::CATEGORICAL:
            return CompressionMethod::DICTIONARY;
            
        default:
            // For generic data, prefer ZSTD if available
            return CompressionMethod::ZSTD;
    }
}

DataType CompressionStrategyManager::detect_data_type(const uint8_t* data, size_t size) {
    if (size == 0) {
      return DataType::GENERIC;
    }
    
    // Check if mostly text
    if (is_mostly_text(data, size)) {
        // Check for JSON markers
        for (size_t i = 0; i < std::min(size, size_t(100)); ++i) {
            if (data[i] == '{' || data[i] == '[') {
                return DataType::JSON;
            }
        }
        return DataType::TEXT;
    }
    
    // Check for sparse data (lots of zeros)
    if (is_sparse_data(data, size)) {
        return DataType::VECTOR_SPARSE;
    }
    
    return DataType::GENERIC;
}

bool CompressionStrategyManager::is_mostly_text(const uint8_t* data, size_t size) {
    size_t printable = 0;
    size_t sample_size = std::min(size, size_t(1024));
    
    for (size_t i = 0; i < sample_size; ++i) {
        if (std::isprint(data[i]) || std::isspace(data[i])) {
            ++printable;
        }
    }
    
    return (printable * 100 / sample_size) > size_t(80);
}

bool CompressionStrategyManager::is_sparse_data(const uint8_t* data, size_t size) {
    size_t zeros = 0;
    size_t sample_size = std::min(size, size_t(1024));
    
    for (size_t i = 0; i < sample_size; ++i) {
        if (data[i] == 0) {
          ++zeros;
        }
    }
    
    return (static_cast<float>(zeros) / sample_size) > config_.sparse_threshold;
}

// ============================================================================
// Compression Method Implementations
// ============================================================================

CompressionResult CompressionStrategyManager::compress_zstd(const uint8_t* data, size_t size) {
    CompressionResult result;
    result.original_size = size;
    result.method_used = CompressionMethod::ZSTD;
    
    result.data = utils::zstd_compress(data, size, config_.level);
    
    if (!result.data.empty()) {
        result.compression_ratio = static_cast<float>(size) / result.data.size();
        result.success = true;
    } else {
        result.data.assign(data, data + size);
        result.compression_ratio = 1.0f;
        result.success = false;
    }
    
    return result;
}

CompressionResult CompressionStrategyManager::compress_rle(const uint8_t* data, size_t size) {
    CompressionResult result;
    result.original_size = size;
    result.method_used = CompressionMethod::RLE;
    
    result.data = RLECodec::compress(data, size);
    result.compression_ratio = result.data.empty() ? 1.0f : 
        static_cast<float>(size) / result.data.size();
    result.success = !result.data.empty();
    
    if (!result.success) {
        result.data.assign(data, data + size);
    }
    
    return result;
}

CompressionResult CompressionStrategyManager::compress_delta(const uint8_t* data, size_t size) {
    CompressionResult result;
    result.original_size = size;
    result.method_used = CompressionMethod::DELTA;
    
    result.data = DeltaCodec::compress(data, size);
    result.compression_ratio = result.data.empty() ? 1.0f :
        static_cast<float>(size) / result.data.size();
    result.success = !result.data.empty();
    
    if (!result.success) {
        result.data.assign(data, data + size);
    }
    
    return result;
}

CompressionResult CompressionStrategyManager::compress_dictionary(const uint8_t* data, size_t size) {
    CompressionResult result;
    result.original_size = size;
    result.method_used = CompressionMethod::DICTIONARY;
    
    result.data = SimpleDictionaryCodec::compress(data, size);
    result.compression_ratio = result.data.empty() ? 1.0f :
        static_cast<float>(size) / result.data.size();
    result.success = !result.data.empty();
    
    if (!result.success) {
        result.data.assign(data, data + size);
    }
    
    return result;
}

// ============================================================================
// Decompression Method Implementations
// ============================================================================

std::vector<uint8_t> CompressionStrategyManager::decompress_zstd(const std::vector<uint8_t>& data) {
    return utils::zstd_decompress(data);
}

std::vector<uint8_t> CompressionStrategyManager::decompress_rle(const std::vector<uint8_t>& data) {
    return RLECodec::decompress(data);
}

std::vector<uint8_t> CompressionStrategyManager::decompress_delta(const std::vector<uint8_t>& data) {
    return DeltaCodec::decompress(data);
}

std::vector<uint8_t> CompressionStrategyManager::decompress_dictionary(const std::vector<uint8_t>& data) {
    return SimpleDictionaryCodec::decompress(data);
}

// ============================================================================
// GPU-Accelerated Compression Method Implementations
// ============================================================================

themis::storage::GpuCompressionManager& CompressionStrategyManager::gpu_manager() {
    if (!gpu_manager_) {
        gpu_manager_ = std::make_unique<themis::storage::GpuCompressionManager>(
            config_.gpu_config);
    }
    return *gpu_manager_;
}

CompressionResult CompressionStrategyManager::compress_gpu_zstd(
    const uint8_t* data, size_t size)
{
    CompressionResult result;
    result.original_size = size;
    result.method_used   = CompressionMethod::GPU_ZSTD;

    auto gpu_result = gpu_manager().compress(
        data, size, themis::storage::GpuCompressionAlgorithm::ZSTD);

    if (gpu_result.success) {
        result.data              = std::move(gpu_result.data);
        result.compression_ratio = gpu_result.compression_ratio;
        result.success           = true;
    } else {
        result.data.assign(data, data + size);
        result.compression_ratio = 1.0f;
        result.success           = false;
    }
    return result;
}

CompressionResult CompressionStrategyManager::compress_gpu_snappy(
    const uint8_t* data, size_t size)
{
    CompressionResult result;
    result.original_size = size;
    result.method_used   = CompressionMethod::GPU_SNAPPY;

    auto gpu_result = gpu_manager().compress(
        data, size, themis::storage::GpuCompressionAlgorithm::SNAPPY);

    if (gpu_result.success) {
        result.data              = std::move(gpu_result.data);
        result.compression_ratio = gpu_result.compression_ratio;
        result.success           = true;
    } else {
        result.data.assign(data, data + size);
        result.compression_ratio = 1.0f;
        result.success           = false;
    }
    return result;
}

CompressionResult CompressionStrategyManager::compress_gpu_lz4(
    const uint8_t* data, size_t size)
{
    CompressionResult result;
    result.original_size = size;
    result.method_used   = CompressionMethod::GPU_LZ4;

    auto gpu_result = gpu_manager().compress(
        data, size, themis::storage::GpuCompressionAlgorithm::LZ4);

    if (gpu_result.success) {
        result.data              = std::move(gpu_result.data);
        result.compression_ratio = gpu_result.compression_ratio;
        result.success           = true;
    } else {
        result.data.assign(data, data + size);
        result.compression_ratio = 1.0f;
        result.success           = false;
    }
    return result;
}

std::vector<uint8_t> CompressionStrategyManager::decompress_gpu_zstd(
    const std::vector<uint8_t>& data)
{
    return gpu_manager().decompress(
        data, themis::storage::GpuCompressionAlgorithm::ZSTD);
}

std::vector<uint8_t> CompressionStrategyManager::decompress_gpu_snappy(
    const std::vector<uint8_t>& data)
{
    return gpu_manager().decompress(
        data, themis::storage::GpuCompressionAlgorithm::SNAPPY);
}

std::vector<uint8_t> CompressionStrategyManager::decompress_gpu_lz4(
    const std::vector<uint8_t>& data)
{
    return gpu_manager().decompress(
        data, themis::storage::GpuCompressionAlgorithm::LZ4);
}

// ============================================================================
// Utility Methods
// ============================================================================

std::string CompressionStrategyManager::get_metrics() const {
    return utils::CompressionMetrics::instance().get_summary();
}

void CompressionStrategyManager::reset_metrics() {
    utils::CompressionMetrics::instance().reset();
}

std::string CompressionStrategyManager::method_to_string(CompressionMethod method) {
    switch (method) {
        case CompressionMethod::NONE: return "none";
        case CompressionMethod::ZSTD: return "zstd";
        case CompressionMethod::LZ4: return "lz4";
        case CompressionMethod::SNAPPY: return "snappy";
        case CompressionMethod::RLE: return "rle";
        case CompressionMethod::DELTA: return "delta";
        case CompressionMethod::DICTIONARY: return "dictionary";
        case CompressionMethod::SPARSE_CSR: return "sparse_csr";
        case CompressionMethod::ADAPTIVE: return "adaptive";
        case CompressionMethod::GPU_ZSTD: return "gpu_zstd";
        case CompressionMethod::GPU_SNAPPY: return "gpu_snappy";
        case CompressionMethod::GPU_LZ4: return "gpu_lz4";
        default: return "unknown";
    }
}

std::optional<CompressionMethod> CompressionStrategyManager::string_to_method(const std::string& str) {
    static const std::unordered_map<std::string, CompressionMethod> mapping = {
        {"none", CompressionMethod::NONE},
        {"zstd", CompressionMethod::ZSTD},
        {"lz4", CompressionMethod::LZ4},
        {"snappy", CompressionMethod::SNAPPY},
        {"rle", CompressionMethod::RLE},
        {"delta", CompressionMethod::DELTA},
        {"dictionary", CompressionMethod::DICTIONARY},
        {"sparse_csr", CompressionMethod::SPARSE_CSR},
        {"adaptive", CompressionMethod::ADAPTIVE},
        {"gpu_zstd", CompressionMethod::GPU_ZSTD},
        {"gpu_snappy", CompressionMethod::GPU_SNAPPY},
        {"gpu_lz4", CompressionMethod::GPU_LZ4}
    };
    
    // iterator_invalidation scanner alert: this map is immutable static data;
    // find() does not mutate it and no erasing/rehashing occurs here — false positive.
    auto it = mapping.find(str);
    return it != mapping.end() ? std::optional<CompressionMethod>(it->second) : std::nullopt;
}

// ============================================================================
// RLECodec Implementation
// ============================================================================

void RLECodec::encode_varint(std::vector<uint8_t>& output, uint32_t value) {
    while (value >= 0x80) {
        output.push_back(static_cast<uint8_t>(value | 0x80));
        value >>= 7;
    }
    output.push_back(static_cast<uint8_t>(value));
}

uint32_t RLECodec::decode_varint(const uint8_t*& ptr) {
    uint32_t result = 0;
    int shift = 0;
    
    while (true) {
        uint8_t byte = *ptr++;
        result |= static_cast<uint32_t>(byte & 0x7F) << shift;
        if ((byte & 0x80) == 0) {
          break;
        }
        shift += 7;
    }
    
    return result;
}

std::vector<uint8_t> RLECodec::compress(const uint8_t* data, size_t size) {
    if (size == 0) {
        THEMIS_DEBUG("RLECodec::compress: called with size=0");
        return {};
    }
    
    std::vector<uint8_t> result;
    const size_t reserve_size = (size > (std::numeric_limits<size_t>::max() / 2))
        ? std::numeric_limits<size_t>::max()
        : size * 2;
    result.reserve(reserve_size);  // Worst-case: [count=1][value] per input byte
    
    size_t i = 0;
    while (i < size) {
        uint8_t value = data[i];
        size_t run_length = 1;
        
        // Count run
        while (i + run_length < size && data[i + run_length] == value) {
            ++run_length;
        }
        
        // Encode run
        encode_varint(result, static_cast<uint32_t>(run_length));
        result.push_back(value);
        
        i += run_length;
    }
    
    return result;
}

std::vector<uint8_t> RLECodec::decompress(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        THEMIS_DEBUG("RLECodec::decompress: called with empty input");
        return {};
    }
    
    std::vector<uint8_t> result = {};

    const size_t reserve_size = (data.size() > (std::numeric_limits<size_t>::max() / 2))
        ? std::numeric_limits<size_t>::max()
        : data.size() * 2;
    result.reserve(reserve_size);  // Heuristic for fewer reallocations
    const uint8_t* ptr = data.data();
    const uint8_t* end = ptr + data.size();
    
    while (ptr < end) {
        if (ptr + 1 >= end) break;  // Need at least count + value
        
        uint32_t count = decode_varint(ptr);
        if (ptr >= end) {
          break;
        }
        
        uint8_t value = *ptr++;
        
        result.insert(result.end(), count, value);
    }
    
    return result;
}

// ============================================================================
// DeltaCodec Implementation
// ============================================================================

std::vector<uint8_t> DeltaCodec::compress(const uint8_t* data, size_t size) {
    if (size == 0) {
        THEMIS_DEBUG("DeltaCodec::compress: called with size=0");
        return {};
    }
    
    std::vector<uint8_t> result;
    result.reserve(size);
    
    // Store first byte as-is
    result.push_back(data[0]);
    
    // Store deltas
    for (size_t i = 1; i < size; ++i) {
        int16_t delta = static_cast<int16_t>(data[i]) - static_cast<int16_t>(data[static_cast<int>(i - 1)]);
        result.push_back(static_cast<uint8_t>(delta & 0xFF));
    }
    
    return result;
}

std::vector<uint8_t> DeltaCodec::decompress(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        THEMIS_DEBUG("DeltaCodec::decompress: called with empty input");
        return {};
    }
    
    std::vector<uint8_t> result = {};

    result.reserve(data.size());
    
    // First byte is stored as-is
    result.push_back(data[0]);
    
    // Reconstruct from deltas
    for (size_t i = 1; i < data.size(); ++i) {
        int16_t delta = static_cast<int8_t>(data[i]);
        uint8_t value = static_cast<uint8_t>(result[static_cast<int>(i - 1)] + delta);
        result.push_back(value);
    }
    
    return result;
}

// ============================================================================
// SimpleDictionaryCodec Implementation
// ============================================================================

std::vector<uint8_t> SimpleDictionaryCodec::compress(const uint8_t* data, size_t size) {
    if (size == 0) return {};
    
    // Build dictionary of unique bytes
    std::unordered_map<uint8_t, uint8_t> value_to_index;
    std::vector<uint8_t> dictionary;
    std::vector<uint8_t> indices;
    value_to_index.reserve(std::min<size_t>(size, 256));
    dictionary.reserve(std::min<size_t>(size, 256));
    indices.reserve(size);
    
    for (size_t i = 0; i < size; ++i) {
        uint8_t value = data[i];
        // iterator_invalidation scanner alert: we only read the iterator result
        // from find(); mutation happens via operator[] only in the "not found"
        // branch, and we do not reuse the old iterator after mutation.
        auto it = value_to_index.find(value);
        
        if (it == value_to_index.end()) {
            uint8_t idx = static_cast<uint8_t>(dictionary.size());
            dictionary.push_back(value);
            value_to_index[value] = idx;
            indices.push_back(idx);
        } else {
            indices.push_back(it->second);
        }
    }
    
    // Only beneficial if dictionary is small
    if (static_cast<int>(dictionary.size()) > 128) {
        THEMIS_DEBUG("SimpleDictionaryCodec::compress: dictionary too large ({}), skipping compression", dictionary.size());
        return {};  // Not beneficial
    }
    
    // Format: [dict_size:1][dictionary...][indices...]
    std::vector<uint8_t> result = {};

    result.reserve(1 + dictionary.size() + indices.size());
    
    result.push_back(static_cast<uint8_t>(dictionary.size()));
    result.insert(result.end(), dictionary.begin(), dictionary.end());
    result.insert(result.end(), indices.begin(), indices.end());
    
    return result;
}

std::vector<uint8_t> SimpleDictionaryCodec::decompress(const std::vector<uint8_t>& data) {
    if (data.empty()) return {};
    
    // Read dictionary size
    uint8_t dict_size = data[0];
    
    if (data.size() < static_cast<size_t>(1 + dict_size)) {
        THEMIS_WARN("SimpleDictionaryCodec::decompress: invalid format (data.size={} dict_size={})", data.size(), dict_size);
        return {};  // Invalid format
    }
    
    // Read dictionary
    std::vector<uint8_t> dictionary(data.begin() + 1, data.begin() + 1 + dict_size);
    
    // Decode indices
    std::vector<uint8_t> result = {};

    result.reserve(data.size() - 1 - dict_size);
    for (size_t i = 1 + dict_size; i < data.size(); ++i) {
        uint8_t idx = data[i];
        if (idx >= dict_size) {
            THEMIS_WARN("SimpleDictionaryCodec::decompress: invalid dictionary index {} >= {}", idx, dict_size);
            return {};  // Invalid index
        }
        result.push_back(dictionary[idx]);
    }
    
    return result;
}

} // namespace compression
} // namespace themis
