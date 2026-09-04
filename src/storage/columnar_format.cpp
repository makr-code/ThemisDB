/**
 * @file columnar_format.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=7, H=42, M=15, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "storage/columnar_format.h"
#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <spdlog/spdlog.h>
#include <lz4.h>
#include <snappy.h>
#include "utils/expected.h"

namespace themis {
namespace storage {

namespace {

constexpr uint64_t kFNVOffsetBasis = 14695981039346656037ull;
constexpr uint64_t kFNVPrime = 1099511628211ull;

uint64_t calculateSegmentChecksum(const uint8_t* bytes, size_t size) {
    uint64_t hash = kFNVOffsetBasis;
    for (size_t i = 0; i < size; ++i) {
        hash ^= static_cast<uint64_t>(bytes[i]);
        hash *= kFNVPrime;
    }
    return hash;
}

} // namespace

// ============================================================================
// ZoneMap Implementation
// ============================================================================

bool ZoneMap::canSkipForInt(int64_t value) const {
    return value < min_int || value > max_int;
}

bool ZoneMap::canSkipForFloat(double value) const {
    return value < min_float || value > max_float;
}

bool ZoneMap::canSkipForString(const std::string& value) const {
    return value < min_str || value > max_str;
}

// ============================================================================
// RLE (Run-Length Encoding) Implementation
// ============================================================================

Result<std::vector<uint8_t>> RLECodec::encodeInt32(const std::vector<int32_t>& data) {
    if (data.empty()) {
        return std::vector<uint8_t>();
    }

    std::vector<uint8_t> encoded;
    encoded.reserve(data.size() * sizeof(int32_t) / 2); // Estimate

    size_t i = 0;
    while (i < data.size()) {
        int32_t value = data[i];
        size_t run_length = 1;

        // Count consecutive identical values
        while (i + run_length < data.size() &&
               data[i + run_length] == value &&
               run_length < 255) {
            run_length++;
        }

        // Write run length (1 byte)
        encoded.push_back(static_cast<uint8_t>(run_length));

        // Write value (4 bytes)
        // size_assumption scanner alerts throughout this file: every flagged sizeof()
        // call uses a <cstdint> fixed-width type (int32_t, int64_t, uint16_t,
        // uint32_t, uint8_t). sizeof() on fixed-width types is the correct
        // portable way to serialize their exact byte width by type contract —
        // false positives.
        const uint8_t* value_bytes = reinterpret_cast<const uint8_t*>(&value);
        encoded.insert(encoded.end(), value_bytes, value_bytes + sizeof(int32_t));

        i += run_length;
    }

    return encoded;
}

Result<std::vector<uint8_t>> RLECodec::encodeInt64(const std::vector<int64_t>& data) {
    if (data.empty()) {
        return std::vector<uint8_t>();
    }

    std::vector<uint8_t> encoded;
    encoded.reserve(data.size() * sizeof(int64_t) / 2);

    size_t i = 0;
    while (i < data.size()) {
        int64_t value = data[i];
        size_t run_length = 1;

        while (i + run_length < data.size() &&
               data[i + run_length] == value &&
               run_length < 255) {
            run_length++;
        }

        encoded.push_back(static_cast<uint8_t>(run_length));
        const uint8_t* value_bytes = reinterpret_cast<const uint8_t*>(&value);
        encoded.insert(encoded.end(), value_bytes, value_bytes + sizeof(int64_t));

        i += run_length;
    }

    return encoded;
}

Result<std::vector<int32_t>> RLECodec::decodeInt32(const std::vector<uint8_t>& encoded) {
    std::vector<int32_t> decoded;

    size_t pos = 0;
    while (pos < encoded.size()) {
        if (pos + 1 + sizeof(int32_t) > encoded.size()) {
            return tl::unexpected(Error(
                errors::ErrorCode::ERR_COMPRESSION_INVALID_FORMAT,
                "RLE decode: insufficient data"
            ));
        }

        uint8_t run_length = encoded[pos++];

        int32_t value;
        std::memcpy(&value, &encoded[pos], sizeof(int32_t));
        pos += sizeof(int32_t);

        for (uint8_t i = 0; i < run_length; ++i) {
            decoded.push_back(value);
        }
    }

    return decoded;
}

Result<std::vector<int64_t>> RLECodec::decodeInt64(const std::vector<uint8_t>& encoded) {
    std::vector<int64_t> decoded;

    size_t pos = 0;
    while (pos < encoded.size()) {
        if (pos + 1 + sizeof(int64_t) > encoded.size()) {
            return tl::unexpected(Error(
                errors::ErrorCode::ERR_COMPRESSION_INVALID_FORMAT,
                "RLE decode: insufficient data"
            ));
        }

        uint8_t run_length = encoded[pos++];

        int64_t value;
        std::memcpy(&value, &encoded[pos], sizeof(int64_t));
        pos += sizeof(int64_t);

        for (uint8_t i = 0; i < run_length; ++i) {
            decoded.push_back(value);
        }
    }

    return decoded;
}

// ============================================================================
// Dictionary Encoding Implementation
// ============================================================================

Result<std::vector<uint8_t>> DictionaryCodec::encodeStrings(const std::vector<std::string>& data) {
    if (data.empty()) {
        return std::vector<uint8_t>();
    }

    // Build dictionary of unique strings
    std::unordered_map<std::string, uint32_t> dictionary;
    std::vector<std::string> dict_values;
    std::vector<uint32_t> indices;

    for (const auto& str : data) {
        // o_n_squared scanner alert: dictionary is a std::unordered_map, so
        // find() is average O(1) inside this loop, not O(n²) — false positive.
        auto it = dictionary.find(str);
        if (it == dictionary.end()) {
            // Validate dictionary size to prevent overflow
            if (dict_values.size() >= static_cast<size_t>(std::numeric_limits<uint32_t>::max())) {
                return tl::unexpected(Error(
                    errors::ErrorCode::ERR_COMPRESSION_FAILED,
                    "Dictionary encode: dictionary size exceeds uint32_t limit"
                ));
            }
            uint32_t idx = static_cast<uint32_t>(dict_values.size());
            dictionary[str] = idx;
            dict_values.push_back(str);
            indices.push_back(idx);
        } else {
            indices.push_back(it->second);
        }
    }

    // Serialize: [dict_size][dict_entries...][indices...]
    std::vector<uint8_t> encoded;

    // Dictionary size
    uint32_t dict_size = static_cast<uint32_t>(dict_values.size());
    const uint8_t* size_bytes = reinterpret_cast<const uint8_t*>(&dict_size);
    encoded.insert(encoded.end(), size_bytes, size_bytes + sizeof(uint32_t));

    // Dictionary entries
    for (const auto& str : dict_values) {
        // Validate string length to prevent overflow
        if (str.size() > static_cast<size_t>(std::numeric_limits<uint32_t>::max())) {
            return tl::unexpected(Error(
                errors::ErrorCode::ERR_COMPRESSION_FAILED,
                "Dictionary encode: string length exceeds uint32_t limit"
            ));
        }
        uint32_t str_len = static_cast<uint32_t>(str.size());
        const uint8_t* len_bytes = reinterpret_cast<const uint8_t*>(&str_len);
        encoded.insert(encoded.end(), len_bytes, len_bytes + sizeof(uint32_t));
        encoded.insert(encoded.end(), str.begin(), str.end());
    }

    // Indices
    const uint8_t* indices_bytes = reinterpret_cast<const uint8_t*>(indices.data());
    encoded.insert(encoded.end(), indices_bytes,
                  indices_bytes + indices.size() * sizeof(uint32_t));

    return encoded;
}

Result<std::vector<std::string>> DictionaryCodec::decodeStrings(const std::vector<uint8_t>& encoded) {
    if (encoded.size() < sizeof(uint32_t)) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_INVALID_FORMAT,
            "Dictionary decode: insufficient data"
        ));
    }

    size_t pos = 0;

    // Read dictionary size
    uint32_t dict_size;
    std::memcpy(&dict_size, &encoded[pos], sizeof(uint32_t));
    pos += sizeof(uint32_t);

    // Validate dictionary size to prevent excessive memory allocation
    // Limit prevents DoS attacks: 10M entries * ~100 bytes avg = ~1GB max dictionary memory
    constexpr uint32_t kMaxDictSize = 10'000'000;  // 10 million entries
    if (dict_size > kMaxDictSize) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_INVALID_FORMAT,
            "Dictionary decode: dictionary size exceeds maximum limit"
        ));
    }

    // Ensure there is at least enough data for the length prefixes
    if (dict_size > 0 && (encoded.size() - pos) < dict_size * sizeof(uint32_t)) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_INVALID_FORMAT,
            "Dictionary decode: insufficient data for dictionary entries"
        ));
    }

    // Read dictionary entries
    std::vector<std::string> dictionary;
    dictionary.reserve(dict_size);

    for (uint32_t i = 0; i < dict_size; ++i) {
        if (pos + sizeof(uint32_t) > encoded.size()) {
            return tl::unexpected(Error(
                errors::ErrorCode::ERR_COMPRESSION_INVALID_FORMAT,
                "Dictionary decode: truncated dictionary entry"
            ));
        }

        uint32_t str_len;
        std::memcpy(&str_len, &encoded[pos], sizeof(uint32_t));
        pos += sizeof(uint32_t);

        // Validate string length to prevent excessive memory allocation
        // Limit prevents DoS attacks: single string cannot exceed 100MB
        // This is reasonable for categorical data while preventing memory exhaustion
        constexpr uint32_t kMaxStringLen = 100'000'000;  // 100 MB per string
        if (str_len > kMaxStringLen) {
            return tl::unexpected(Error(
                errors::ErrorCode::ERR_COMPRESSION_INVALID_FORMAT,
                "Dictionary decode: string length exceeds maximum limit"
            ));
        }

        if (pos + str_len > encoded.size()) {
            return tl::unexpected(Error(
                errors::ErrorCode::ERR_COMPRESSION_INVALID_FORMAT,
                "Dictionary decode: truncated string"
            ));
        }

        std::string str(reinterpret_cast<const char*>(&encoded[pos]), str_len);
        dictionary.push_back(std::move(str));
        pos += str_len;
    }

    // Calculate number of indices
    size_t remaining = encoded.size() - pos;
    if (remaining % sizeof(uint32_t) != 0) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_INVALID_FORMAT,
            "Dictionary decode: invalid indices size"
        ));
    }

    size_t num_indices = remaining / sizeof(uint32_t);

    // Decode indices to strings
    std::vector<std::string> decoded;
    decoded.reserve(num_indices);

    for (size_t i = 0; i < num_indices; ++i) {
        uint32_t idx;
        std::memcpy(&idx, &encoded[pos], sizeof(uint32_t));
        pos += sizeof(uint32_t);

        if (idx >= dictionary.size()) {
            return tl::unexpected(Error(
                errors::ErrorCode::ERR_COMPRESSION_INVALID_FORMAT,
                "Dictionary decode: invalid index"
            ));
        }

        decoded.push_back(dictionary[idx]);
    }

    return decoded;
}

bool DictionaryCodec::shouldUseDictionary(const std::vector<std::string>& data,
                                         [[maybe_unused]] double min_compression_ratio) {
    if (data.empty()) return false;

    // Calculate unique strings
    std::unordered_set<std::string> unique_strings(data.begin(), data.end());

    // If cardinality is low, dictionary encoding is beneficial
    double cardinality_ratio = static_cast<double>(unique_strings.size()) / data.size();

    // Use dictionary if less than 30% unique values
    // This threshold balances compression ratio with dictionary overhead
    // Typical categorical columns have <10% cardinality
    return cardinality_ratio < 0.3;
}

// ============================================================================
// Bit-Packing Implementation
// ============================================================================

uint8_t BitPackingCodec::calculateBitsRequired(int64_t min_val, int64_t max_val) {
    // Handle edge case where min > max (shouldn't happen, but be defensive)
    if (min_val > max_val) {
        std::swap(min_val, max_val);
    }
    
    // Calculate range safely to avoid overflow
    uint64_t range = 0;
    
    if (min_val >= 0 && max_val >= 0) {
        // Both non-negative: simple unsigned subtraction is safe
        range = static_cast<uint64_t>(max_val) - static_cast<uint64_t>(min_val);
    } else if (min_val < 0 && max_val < 0) {
        // Both negative: compute difference of absolute values
        // For negative int64, use two's complement: abs(x) = ~x + 1 for x < 0
        uint64_t min_abs = static_cast<uint64_t>(~min_val) + 1;
        uint64_t max_abs = static_cast<uint64_t>(~max_val) + 1;
        range = min_abs - max_abs;
    } else {
        // Range crosses zero: abs(min_val) + max_val
        uint64_t min_abs = static_cast<uint64_t>(~min_val) + 1;
        range = min_abs + static_cast<uint64_t>(max_val);
    }
    
    if (range == 0) return 1;

    uint8_t bits = 0;
    while (range > 0) {
        bits++;
        range >>= 1;
    }
    return bits;
}

Result<std::vector<uint8_t>> BitPackingCodec::encodeInt32(const std::vector<int32_t>& data) {
    if (data.empty()) {
        return std::vector<uint8_t>();
    }

    // Find min/max
    int32_t min_val = *std::min_element(data.begin(), data.end());
    int32_t max_val = *std::max_element(data.begin(), data.end());

    uint8_t bits_required = calculateBitsRequired(min_val, max_val);

    // Store header: min_val, bits_required, count
    std::vector<uint8_t> encoded;
    const uint8_t* min_bytes = reinterpret_cast<const uint8_t*>(&min_val);
    encoded.insert(encoded.end(), min_bytes, min_bytes + sizeof(int32_t));
    encoded.push_back(bits_required);

    // Validate data size to prevent overflow
    if (data.size() > static_cast<size_t>(std::numeric_limits<uint32_t>::max())) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_FAILED,
            "Bit-packing encode: data size exceeds uint32_t limit"
        ));
    }

    uint32_t count = static_cast<uint32_t>(data.size());
    const uint8_t* count_bytes = reinterpret_cast<const uint8_t*>(&count);
    encoded.insert(encoded.end(), count_bytes, count_bytes + sizeof(uint32_t));

    // Pack bits: Simple byte-aligned packing for now
    // For better compression, use proper bit-packing library
    // This is a simplified implementation that achieves some compression
    if (bits_required <= 8) {
        // Can fit in uint8_t
        for (int32_t val : data) {
            uint8_t normalized = static_cast<uint8_t>(val - min_val);
            encoded.push_back(normalized);
        }
    } else if (bits_required <= 16) {
        // Can fit in uint16_t
        for (int32_t val : data) {
            uint16_t normalized = static_cast<uint16_t>(val - min_val);
            const uint8_t* norm_bytes = reinterpret_cast<const uint8_t*>(&normalized);
            encoded.insert(encoded.end(), norm_bytes, norm_bytes + sizeof(uint16_t));
        }
    } else {
        // Full int32_t needed
        for (int32_t val : data) {
            int32_t normalized = val - min_val;
            const uint8_t* val_bytes = reinterpret_cast<const uint8_t*>(&normalized);
            encoded.insert(encoded.end(), val_bytes, val_bytes + sizeof(int32_t));
        }
    }

    return encoded;
}

Result<std::vector<uint8_t>> BitPackingCodec::encodeInt64(const std::vector<int64_t>& data) {
    if (data.empty()) {
        return std::vector<uint8_t>();
    }

    int64_t min_val = *std::min_element(data.begin(), data.end());
    int64_t max_val = *std::max_element(data.begin(), data.end());

    uint8_t bits_required = calculateBitsRequired(min_val, max_val);

    std::vector<uint8_t> encoded;
    const uint8_t* min_bytes = reinterpret_cast<const uint8_t*>(&min_val);
    encoded.insert(encoded.end(), min_bytes, min_bytes + sizeof(int64_t));
    encoded.push_back(bits_required);

    // Validate data size to prevent overflow
    if (data.size() > static_cast<size_t>(std::numeric_limits<uint32_t>::max())) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_FAILED,
            "Bit-packing encode: data size exceeds uint32_t limit"
        ));
    }

    uint32_t count = static_cast<uint32_t>(data.size());
    const uint8_t* count_bytes = reinterpret_cast<const uint8_t*>(&count);
    encoded.insert(encoded.end(), count_bytes, count_bytes + sizeof(uint32_t));

    // Byte-aligned packing based on bits required
    if (bits_required <= 8) {
        for (int64_t val : data) {
            uint8_t normalized = static_cast<uint8_t>(val - min_val);
            encoded.push_back(normalized);
        }
    } else if (bits_required <= 16) {
        for (int64_t val : data) {
            uint16_t normalized = static_cast<uint16_t>(val - min_val);
            const uint8_t* norm_bytes = reinterpret_cast<const uint8_t*>(&normalized);
            encoded.insert(encoded.end(), norm_bytes, norm_bytes + sizeof(uint16_t));
        }
    } else if (bits_required <= 32) {
        for (int64_t val : data) {
            uint32_t normalized = static_cast<uint32_t>(val - min_val);
            const uint8_t* norm_bytes = reinterpret_cast<const uint8_t*>(&normalized);
            encoded.insert(encoded.end(), norm_bytes, norm_bytes + sizeof(uint32_t));
        }
    } else {
        for (int64_t val : data) {
            int64_t normalized = val - min_val;
            const uint8_t* val_bytes = reinterpret_cast<const uint8_t*>(&normalized);
            encoded.insert(encoded.end(), val_bytes, val_bytes + sizeof(int64_t));
        }
    }

    return encoded;
}

Result<std::vector<int32_t>> BitPackingCodec::decodeInt32(const std::vector<uint8_t>& encoded) {
    if (encoded.size() < sizeof(int32_t) + 1 + sizeof(uint32_t)) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_INVALID_FORMAT,
            "Bit-packing decode: insufficient header data"
        ));
    }

    size_t pos = 0;

    int32_t min_val;
    std::memcpy(&min_val, &encoded[pos], sizeof(int32_t));
    pos += sizeof(int32_t);

    uint8_t bits_required = encoded[pos++];

    uint32_t count;
    std::memcpy(&count, &encoded[pos], sizeof(uint32_t));
    pos += sizeof(uint32_t);

    // Validate count against remaining encoded size to avoid excessive allocation
    size_t remaining = encoded.size() - pos;
    size_t bytes_per_value;
    if (bits_required <= 8) {
        bytes_per_value = sizeof(uint8_t);
    } else if (bits_required <= 16) {
        bytes_per_value = sizeof(uint16_t);
    } else {
        bytes_per_value = sizeof(int32_t);
    }

    size_t max_count_by_size = remaining / bytes_per_value;
    if (static_cast<size_t>(count) > max_count_by_size) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_INVALID_FORMAT,
            "Bit-packing decode: count exceeds available data"
        ));
    }

    std::vector<int32_t> decoded;
    decoded.reserve(count);

    if (bits_required <= 8) {
        // Stored as uint8_t
        for (uint32_t i = 0; i < count && pos < encoded.size(); ++i) {
            uint8_t normalized = encoded[pos++];
            decoded.push_back(static_cast<int32_t>(normalized) + min_val);
        }
    } else if (bits_required <= 16) {
        // Stored as uint16_t
        for (uint32_t i = 0; i < count && pos + sizeof(uint16_t) <= encoded.size(); ++i) {
            uint16_t normalized;
            std::memcpy(&normalized, &encoded[pos], sizeof(uint16_t));
            pos += sizeof(uint16_t);
            decoded.push_back(static_cast<int32_t>(normalized) + min_val);
        }
    } else {
        // Stored as int32_t
        for (uint32_t i = 0; i < count && pos + sizeof(int32_t) <= encoded.size(); ++i) {
            int32_t normalized;
            std::memcpy(&normalized, &encoded[pos], sizeof(int32_t));
            pos += sizeof(int32_t);
            decoded.push_back(normalized + min_val);
        }
    }

    return decoded;
}

Result<std::vector<int64_t>> BitPackingCodec::decodeInt64(const std::vector<uint8_t>& encoded) {
    if (encoded.size() < sizeof(int64_t) + 1 + sizeof(uint32_t)) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_INVALID_FORMAT,
            "Bit-packing decode: insufficient header data"
        ));
    }

    size_t pos = 0;

    int64_t min_val;
    std::memcpy(&min_val, &encoded[pos], sizeof(int64_t));
    pos += sizeof(int64_t);

    uint8_t bits_required = encoded[pos++];

    uint32_t count;
    std::memcpy(&count, &encoded[pos], sizeof(uint32_t));
    pos += sizeof(uint32_t);

    // Validate count against remaining encoded size to avoid excessive allocation
    size_t remaining = encoded.size() - pos;
    size_t bytes_per_value;
    if (bits_required <= 8) {
        bytes_per_value = sizeof(uint8_t);
    } else if (bits_required <= 16) {
        bytes_per_value = sizeof(uint16_t);
    } else if (bits_required <= 32) {
        bytes_per_value = sizeof(uint32_t);
    } else {
        bytes_per_value = sizeof(int64_t);
    }

    size_t max_count_by_size = remaining / bytes_per_value;
    if (static_cast<size_t>(count) > max_count_by_size) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_INVALID_FORMAT,
            "Bit-packing decode: count exceeds available data"
        ));
    }

    std::vector<int64_t> decoded;
    decoded.reserve(count);

    if (bits_required <= 8) {
        for (uint32_t i = 0; i < count && pos < encoded.size(); ++i) {
            uint8_t normalized = encoded[pos++];
            decoded.push_back(static_cast<int64_t>(normalized) + min_val);
        }
    } else if (bits_required <= 16) {
        for (uint32_t i = 0; i < count && pos + sizeof(uint16_t) <= encoded.size(); ++i) {
            uint16_t normalized;
            std::memcpy(&normalized, &encoded[pos], sizeof(uint16_t));
            pos += sizeof(uint16_t);
            decoded.push_back(static_cast<int64_t>(normalized) + min_val);
        }
    } else if (bits_required <= 32) {
        for (uint32_t i = 0; i < count && pos + sizeof(uint32_t) <= encoded.size(); ++i) {
            uint32_t normalized;
            std::memcpy(&normalized, &encoded[pos], sizeof(uint32_t));
            pos += sizeof(uint32_t);
            decoded.push_back(static_cast<int64_t>(normalized) + min_val);
        }
    } else {
        for (uint32_t i = 0; i < count && pos + sizeof(int64_t) <= encoded.size(); ++i) {
            int64_t normalized;
            std::memcpy(&normalized, &encoded[pos], sizeof(int64_t));
            pos += sizeof(int64_t);
            decoded.push_back(normalized + min_val);
        }
    }

    return decoded;
}

// ============================================================================
// Frame-of-Reference Implementation
// ============================================================================

Result<std::vector<uint8_t>> FrameOfReferenceCodec::encodeInt32(const std::vector<int32_t>& data) {
    if (data.empty()) {
        return std::vector<uint8_t>();
    }

    // Use first value as reference
    // pointer_arithmetic scanner alerts in both frame-of-reference encoders are
    // false positives: each function returns early when data.empty(), so data[0]
    // is only read after a non-empty guard has succeeded.
    int32_t reference = data[0];

    std::vector<uint8_t> encoded;

    // Store reference value
    const uint8_t* ref_bytes = reinterpret_cast<const uint8_t*>(&reference);
    encoded.insert(encoded.end(), ref_bytes, ref_bytes + sizeof(int32_t));

    // Store deltas
    for (size_t i = 1; i < data.size(); ++i) {
        int32_t delta = data[i] - reference;
        const uint8_t* delta_bytes = reinterpret_cast<const uint8_t*>(&delta);
        encoded.insert(encoded.end(), delta_bytes, delta_bytes + sizeof(int32_t));
    }

    return encoded;
}

Result<std::vector<uint8_t>> FrameOfReferenceCodec::encodeInt64(const std::vector<int64_t>& data) {
    if (data.empty()) {
        return std::vector<uint8_t>();
    }

    int64_t reference = data[0];

    std::vector<uint8_t> encoded;
    const uint8_t* ref_bytes = reinterpret_cast<const uint8_t*>(&reference);
    encoded.insert(encoded.end(), ref_bytes, ref_bytes + sizeof(int64_t));

    for (size_t i = 1; i < data.size(); ++i) {
        int64_t delta = data[i] - reference;
        const uint8_t* delta_bytes = reinterpret_cast<const uint8_t*>(&delta);
        encoded.insert(encoded.end(), delta_bytes, delta_bytes + sizeof(int64_t));
    }

    return encoded;
}

Result<std::vector<int32_t>> FrameOfReferenceCodec::decodeInt32(const std::vector<uint8_t>& encoded) {
    if (encoded.size() < sizeof(int32_t)) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_INVALID_FORMAT,
            "Frame-of-reference decode: no reference value"
        ));
    }

    size_t pos = 0;

    int32_t reference;
    std::memcpy(&reference, &encoded[pos], sizeof(int32_t));
    pos += sizeof(int32_t);

    std::vector<int32_t> decoded;
    decoded.push_back(reference);

    while (pos + sizeof(int32_t) <= encoded.size()) {
        int32_t delta;
        std::memcpy(&delta, &encoded[pos], sizeof(int32_t));
        pos += sizeof(int32_t);

        decoded.push_back(reference + delta);
    }

    return decoded;
}

Result<std::vector<int64_t>> FrameOfReferenceCodec::decodeInt64(const std::vector<uint8_t>& encoded) {
    if (encoded.size() < sizeof(int64_t)) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_INVALID_FORMAT,
            "Frame-of-reference decode: no reference value"
        ));
    }

    size_t pos = 0;

    int64_t reference;
    std::memcpy(&reference, &encoded[pos], sizeof(int64_t));
    pos += sizeof(int64_t);

    std::vector<int64_t> decoded;
    decoded.push_back(reference);

    while (pos + sizeof(int64_t) <= encoded.size()) {
        int64_t delta;
        std::memcpy(&delta, &encoded[pos], sizeof(int64_t));
        pos += sizeof(int64_t);

        decoded.push_back(reference + delta);
    }

    return decoded;
}

// ============================================================================
// Generic Compression Implementation (LZ4/Snappy)
// ============================================================================

Result<std::vector<uint8_t>> GenericCompressionCodec::compressLZ4(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return std::vector<uint8_t>();
    }

    // unsanitized_llm_input scanner alert: this LZ4 bounds-checking path
    // operates on binary compression buffers only; no value here flows into an
    // LLM inference call — false positive.
    // prompt_injection scanner alert: this is a binary buffer size guard, not
    // user-facing text or an LLM prompt — false positive.
    // Maximum safe input size - must fit in int for LZ4 API
    constexpr size_t MAX_INPUT_SIZE = static_cast<size_t>(INT_MAX);
    if (data.size() > MAX_INPUT_SIZE) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_FAILED,
            "LZ4 compression: input data too large (exceeds INT_MAX)"
        ));
    }

    // Get maximum compressed size
    int max_compressed_size = LZ4_compressBound(static_cast<int>(data.size()));
    if (max_compressed_size <= 0) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_FAILED,
            "LZ4 compression: invalid compression bound"
        ));
    }

    // Format: [original_size:8][compressed_data...]
    // Store original size for safe decompression
    std::vector<uint8_t> result;
    try {
        result.resize(8 + max_compressed_size);
    } catch (const std::bad_alloc&) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_FAILED,
            "LZ4 compression: failed to allocate output buffer"
        ));
    }

    // Write original size as 8-byte header
    uint64_t original_size = data.size();
    std::memcpy(result.data(), &original_size, 8);

    // Compress the data
    int compressed_size = LZ4_compress_default(
        reinterpret_cast<const char*>(data.data()),
        reinterpret_cast<char*>(result.data() + 8),
        static_cast<int>(data.size()),
        max_compressed_size
    );

    if (compressed_size <= 0) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_FAILED,
            "LZ4 compression failed"
        ));
    }

    // Resize to actual size (header + compressed data)
    result.resize(8 + compressed_size);
    return result;
}

Result<std::vector<uint8_t>> GenericCompressionCodec::decompressLZ4(const std::vector<uint8_t>& compressed) {
    if (compressed.empty()) {
        return std::vector<uint8_t>();
    }

    // Validate minimum size for header
    if (compressed.size() < 8) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_INVALID_FORMAT,
            "LZ4 decompression: compressed data too small (missing header)"
        ));
    }

    // Read original size from 8-byte header
    uint64_t original_size;
    std::memcpy(&original_size, compressed.data(), 8);

    // Validate original size
    constexpr size_t MAX_DECOMPRESSED_SIZE = 1024ULL * 1024 * 1024 * 4; // 4GB
    if (original_size > MAX_DECOMPRESSED_SIZE) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_INVALID_FORMAT,
            "LZ4 decompression: original size too large"
        ));
    }

    // Original size must fit in int for LZ4 API
    if (original_size > static_cast<size_t>(INT_MAX)) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_INVALID_FORMAT,
            "LZ4 decompression: original size exceeds INT_MAX"
        ));
    }

    // Allocate exact buffer for decompression
    std::vector<uint8_t> decompressed;
    try {
        decompressed.resize(original_size);
    } catch (const std::bad_alloc&) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_FAILED,
            "LZ4 decompression: failed to allocate output buffer"
        ));
    }

    // Decompress (skip 8-byte header)
    size_t compressed_data_size = compressed.size() - 8;
    if (compressed_data_size > static_cast<size_t>(INT_MAX)) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_INVALID_FORMAT,
            "LZ4 decompression: compressed data size exceeds INT_MAX"
        ));
    }

    int decompressed_size = LZ4_decompress_safe(
        reinterpret_cast<const char*>(compressed.data() + 8),
        reinterpret_cast<char*>(decompressed.data()),
        static_cast<int>(compressed_data_size),
        static_cast<int>(original_size)
    );

    if (decompressed_size < 0) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_FAILED,
            "LZ4 decompression failed"
        ));
    }

    // Verify decompressed size matches expected
    if (static_cast<size_t>(decompressed_size) != original_size) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_INVALID_FORMAT,
            "LZ4 decompression: size mismatch"
        ));
    }

    return decompressed;
}

Result<std::vector<uint8_t>> GenericCompressionCodec::compressSnappy(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return std::vector<uint8_t>();
    }

    // unsanitized_llm_input scanner alert: this Snappy bounds-checking path
    // handles raw binary compression data and never feeds prompt/model input —
    // false positive.
    // prompt_injection scanner alert: this is a binary buffer size guard, not
    // user-facing text or an LLM prompt — false positive.
    // Maximum safe input size (1GB)
    constexpr size_t MAX_INPUT_SIZE = 1024ULL * 1024 * 1024;
    if (data.size() > MAX_INPUT_SIZE) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_FAILED,
            "Snappy compression: input data too large"
        ));
    }

    // Get maximum compressed size
    size_t max_compressed_size = snappy::MaxCompressedLength(data.size());

    // Allocate output buffer
    std::vector<uint8_t> compressed;
    try {
        compressed.resize(max_compressed_size);
    } catch (const std::bad_alloc&) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_FAILED,
            "Snappy compression: failed to allocate output buffer"
        ));
    }

    // Compress the data
    size_t compressed_size = 0;
    snappy::RawCompress(
        reinterpret_cast<const char*>(data.data()),
        data.size(),
        reinterpret_cast<char*>(compressed.data()),
        &compressed_size
    );

    // Resize to actual compressed size
    compressed.resize(compressed_size);
    return compressed;
}

Result<std::vector<uint8_t>> GenericCompressionCodec::decompressSnappy(const std::vector<uint8_t>& compressed) {
    if (compressed.empty()) {
        return std::vector<uint8_t>();
    }

    // Get uncompressed length
    size_t uncompressed_size = 0;
    if (!snappy::GetUncompressedLength(
            reinterpret_cast<const char*>(compressed.data()),
            compressed.size(),
            &uncompressed_size)) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_FAILED,
            "Snappy decompression: failed to get uncompressed length"
        ));
    }

    // Validate size to prevent excessive memory allocation
    constexpr size_t MAX_DECOMPRESSED_SIZE = 1024ULL * 1024 * 1024 * 4; // 4GB
    if (uncompressed_size > MAX_DECOMPRESSED_SIZE) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_FAILED,
            "Snappy decompression: uncompressed size too large"
        ));
    }

    // Allocate output buffer
    std::vector<uint8_t> decompressed;
    try {
        decompressed.resize(uncompressed_size);
    } catch (const std::bad_alloc&) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_FAILED,
            "Snappy decompression: failed to allocate output buffer"
        ));
    }

    // Decompress the data
    if (!snappy::RawUncompress(
            reinterpret_cast<const char*>(compressed.data()),
            compressed.size(),
            reinterpret_cast<char*>(decompressed.data()))) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_FAILED,
            "Snappy decompression failed"
        ));
    }

    return decompressed;
}

// ============================================================================
// ColumnSegment Implementation
// ============================================================================

void ColumnSegment::buildZoneMap() {
    metadata_.zone_map.row_count = metadata_.row_count;

    if (raw_data_.empty() || metadata_.row_count == 0) {
        return;
    }

    const uint8_t* data_ptr = raw_data_.data();

    switch (metadata_.type) {
        case ColumnType::INT32: {
            const int32_t* values = reinterpret_cast<const int32_t*>(data_ptr);
            int32_t min_val = std::numeric_limits<int32_t>::max();
            int32_t max_val = std::numeric_limits<int32_t>::min();

            for (size_t i = 0; i < metadata_.row_count; ++i) {
                min_val = std::min(min_val, values[i]);
                max_val = std::max(max_val, values[i]);
            }

            metadata_.zone_map.min_int = min_val;
            metadata_.zone_map.max_int = max_val;
            break;
        }

        case ColumnType::INT64: {
            const int64_t* values = reinterpret_cast<const int64_t*>(data_ptr);
            int64_t min_val = std::numeric_limits<int64_t>::max();
            int64_t max_val = std::numeric_limits<int64_t>::min();

            for (size_t i = 0; i < metadata_.row_count; ++i) {
                min_val = std::min(min_val, values[i]);
                max_val = std::max(max_val, values[i]);
            }

            metadata_.zone_map.min_int = min_val;
            metadata_.zone_map.max_int = max_val;
            break;
        }

        case ColumnType::FLOAT32: {
            const float* values = reinterpret_cast<const float*>(data_ptr);
            float min_val = std::numeric_limits<float>::max();
            float max_val = std::numeric_limits<float>::lowest();

            for (size_t i = 0; i < metadata_.row_count; ++i) {
                min_val = std::min(min_val, values[i]);
                max_val = std::max(max_val, values[i]);
            }

            metadata_.zone_map.min_float = min_val;
            metadata_.zone_map.max_float = max_val;
            break;
        }

        case ColumnType::FLOAT64: {
            const double* values = reinterpret_cast<const double*>(data_ptr);
            double min_val = std::numeric_limits<double>::max();
            double max_val = std::numeric_limits<double>::lowest();

            for (size_t i = 0; i < metadata_.row_count; ++i) {
                min_val = std::min(min_val, values[i]);
                max_val = std::max(max_val, values[i]);
            }

            metadata_.zone_map.min_float = min_val;
            metadata_.zone_map.max_float = max_val;
            break;
        }

        default:
            // STRING and BOOL not supported yet for zone maps
            break;
    }
}

CompressionCodec ColumnSegment::selectOptimalCodec(
    ColumnType type,
    [[maybe_unused]] const void* data,
    [[maybe_unused]] size_t row_count
) {
    // Simple heuristic-based codec selection
    switch (type) {
        case ColumnType::INT32:
        [[fallthrough]];\n        case ColumnType::INT64:
            // For integers, check if data is sorted/has patterns
            // Default to RLE for simplicity
            return CompressionCodec::RLE;

        case ColumnType::STRING:
            return CompressionCodec::DICTIONARY;

        case ColumnType::FLOAT32:
        [[fallthrough]];\n        case ColumnType::FLOAT64:
        [[fallthrough]];\n        case ColumnType::BOOL:
        [[fallthrough]];\n        default:
            return CompressionCodec::NONE;
    }
}

Result<ColumnSegment> ColumnSegment::create(
    ColumnType type,
    const void* data,
    size_t row_count,
    CompressionCodec codec
) {
    ColumnSegment segment;
    segment.metadata_.type = type;
    segment.metadata_.row_count = row_count;

    // Calculate data size based on type
    size_t element_size = 0;
    switch (type) {
        case ColumnType::INT32:
        [[fallthrough]];\n        case ColumnType::FLOAT32:
            element_size = 4;
            break;
        case ColumnType::INT64:
        [[fallthrough]];\n        case ColumnType::FLOAT64:
            element_size = 8;
            break;
        case ColumnType::BOOL:
            element_size = 1;
            break;
        case ColumnType::STRING:
            // Strings handled separately
            return tl::unexpected(Error(
                errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                "String columns require special handling"
            ));
    }

    segment.metadata_.uncompressed_size = row_count * element_size;

    // Copy raw data
    const uint8_t* byte_data = static_cast<const uint8_t*>(data);
    segment.raw_data_.assign(byte_data, byte_data + segment.metadata_.uncompressed_size);

    // Build zone map
    segment.buildZoneMap();

    // Auto-select codec if NONE specified
    if (codec == CompressionCodec::NONE) {
        codec = selectOptimalCodec(type, data, row_count);
    }

    segment.metadata_.codec = codec;

    return segment;
}

Result<void> ColumnSegment::encode() {
    if (is_encoded_) {
        spdlog::debug("ColumnSegment::encode: already encoded (row_count={})", metadata_.row_count);
        return {};
    }

    Result<std::vector<uint8_t>> encode_result = Ok(std::vector<uint8_t>{});

    switch (metadata_.codec) {
        case CompressionCodec::RLE: {
            if (metadata_.type == ColumnType::INT32) {
                const int32_t* values = reinterpret_cast<const int32_t*>(raw_data_.data());
                std::vector<int32_t> data(values, values + metadata_.row_count);
                encode_result = RLECodec::encodeInt32(data);
            } else if (metadata_.type == ColumnType::INT64) {
                const int64_t* values = reinterpret_cast<const int64_t*>(raw_data_.data());
                std::vector<int64_t> data(values, values + metadata_.row_count);
                encode_result = RLECodec::encodeInt64(data);
            } else {
                return tl::unexpected(Error(
                    errors::ErrorCode::ERR_COMPRESSION_FAILED,
                    "RLE only supports INT32/INT64"
                ));
            }
            break;
        }

        case CompressionCodec::BIT_PACKING: {
            if (metadata_.type == ColumnType::INT32) {
                const int32_t* values = reinterpret_cast<const int32_t*>(raw_data_.data());
                std::vector<int32_t> data(values, values + metadata_.row_count);
                encode_result = BitPackingCodec::encodeInt32(data);
            } else if (metadata_.type == ColumnType::INT64) {
                const int64_t* values = reinterpret_cast<const int64_t*>(raw_data_.data());
                std::vector<int64_t> data(values, values + metadata_.row_count);
                encode_result = BitPackingCodec::encodeInt64(data);
            } else {
                return tl::unexpected(Error(
                    errors::ErrorCode::ERR_COMPRESSION_FAILED,
                    "Bit-packing only supports INT32/INT64"
                ));
            }
            break;
        }

        case CompressionCodec::FRAME_OF_REF: {
            if (metadata_.type == ColumnType::INT32) {
                const int32_t* values = reinterpret_cast<const int32_t*>(raw_data_.data());
                std::vector<int32_t> data(values, values + metadata_.row_count);
                encode_result = FrameOfReferenceCodec::encodeInt32(data);
            } else if (metadata_.type == ColumnType::INT64) {
                const int64_t* values = reinterpret_cast<const int64_t*>(raw_data_.data());
                std::vector<int64_t> data(values, values + metadata_.row_count);
                encode_result = FrameOfReferenceCodec::encodeInt64(data);
            } else {
                return tl::unexpected(Error(
                    errors::ErrorCode::ERR_COMPRESSION_FAILED,
                    "Frame-of-reference only supports INT32/INT64"
                ));
            }
            break;
        }

        case CompressionCodec::NONE:
            // No encoding needed
            encoded_data_ = raw_data_;
            metadata_.compressed_size = raw_data_.size();
            is_encoded_ = true;
            spdlog::debug("ColumnSegment::encode: codec=NONE, no-op encode ({} bytes)", raw_data_.size());
            return {};

        default:
            return tl::unexpected(Error(
                errors::ErrorCode::ERR_COMPRESSION_FAILED,
                "Unsupported compression codec"
            ));
    }

    if (!encode_result) {
        return tl::unexpected(encode_result.error());
    }

    encoded_data_ = std::move(*encode_result);
    metadata_.compressed_size = encoded_data_.size();
    is_encoded_ = true;

    spdlog::debug("Encoded column: {} -> {} bytes ({}x compression)",
                  metadata_.uncompressed_size,
                  metadata_.compressed_size,
                  metadata_.compressionRatio());

    return {};
}

Result<void> ColumnSegment::decode() {
    if (!is_encoded_) {
        spdlog::debug("ColumnSegment::decode: called on already-decoded segment (row_count={})", metadata_.row_count);
        return {};
    }

    switch (metadata_.codec) {
        case CompressionCodec::NONE: {
            // No-op encoding: encoded_data_ IS raw_data_
            raw_data_ = encoded_data_;
            is_encoded_ = false;
            return {};
        }

        case CompressionCodec::RLE: {
            if (metadata_.type == ColumnType::INT32) {
                auto decode_result = RLECodec::decodeInt32(encoded_data_);
                if (!decode_result) return tl::unexpected(decode_result.error());
                const auto& vals = *decode_result;
                raw_data_.resize(vals.size() * sizeof(int32_t));
                std::memcpy(raw_data_.data(), vals.data(), raw_data_.size());
            } else if (metadata_.type == ColumnType::INT64) {
                auto decode_result = RLECodec::decodeInt64(encoded_data_);
                if (!decode_result) return tl::unexpected(decode_result.error());
                const auto& vals = *decode_result;
                raw_data_.resize(vals.size() * sizeof(int64_t));
                std::memcpy(raw_data_.data(), vals.data(), raw_data_.size());
            } else {
                return tl::unexpected(Error(
                    errors::ErrorCode::ERR_CODEC_NOT_AVAILABLE,
                    "RLE decode: only INT32/INT64 supported"
                ));
            }
            is_encoded_ = false;
            return {};
        }

        case CompressionCodec::BIT_PACKING: {
            if (metadata_.type == ColumnType::INT32) {
                auto decode_result = BitPackingCodec::decodeInt32(encoded_data_);
                if (!decode_result) return tl::unexpected(decode_result.error());
                const auto& vals = *decode_result;
                raw_data_.resize(vals.size() * sizeof(int32_t));
                std::memcpy(raw_data_.data(), vals.data(), raw_data_.size());
            } else if (metadata_.type == ColumnType::INT64) {
                auto decode_result = BitPackingCodec::decodeInt64(encoded_data_);
                if (!decode_result) return tl::unexpected(decode_result.error());
                const auto& vals = *decode_result;
                raw_data_.resize(vals.size() * sizeof(int64_t));
                std::memcpy(raw_data_.data(), vals.data(), raw_data_.size());
            } else {
                return tl::unexpected(Error(
                    errors::ErrorCode::ERR_CODEC_NOT_AVAILABLE,
                    "Bit-packing decode: only INT32/INT64 supported"
                ));
            }
            is_encoded_ = false;
            return {};
        }

        case CompressionCodec::FRAME_OF_REF: {
            if (metadata_.type == ColumnType::INT32) {
                auto decode_result = FrameOfReferenceCodec::decodeInt32(encoded_data_);
                if (!decode_result) return tl::unexpected(decode_result.error());
                const auto& vals = *decode_result;
                raw_data_.resize(vals.size() * sizeof(int32_t));
                std::memcpy(raw_data_.data(), vals.data(), raw_data_.size());
            } else if (metadata_.type == ColumnType::INT64) {
                auto decode_result = FrameOfReferenceCodec::decodeInt64(encoded_data_);
                if (!decode_result) return tl::unexpected(decode_result.error());
                const auto& vals = *decode_result;
                raw_data_.resize(vals.size() * sizeof(int64_t));
                std::memcpy(raw_data_.data(), vals.data(), raw_data_.size());
            } else {
                return tl::unexpected(Error(
                    errors::ErrorCode::ERR_CODEC_NOT_AVAILABLE,
                    "Frame-of-reference decode: only INT32/INT64 supported"
                ));
            }
            is_encoded_ = false;
            return {};
        }

        case CompressionCodec::LZ4: {
            auto decode_result = GenericCompressionCodec::decompressLZ4(encoded_data_);
            if (!decode_result) return tl::unexpected(decode_result.error());
            raw_data_ = std::move(*decode_result);
            is_encoded_ = false;
            return {};
        }

        case CompressionCodec::SNAPPY: {
            auto decode_result = GenericCompressionCodec::decompressSnappy(encoded_data_);
            if (!decode_result) return tl::unexpected(decode_result.error());
            raw_data_ = std::move(*decode_result);
            is_encoded_ = false;
            return {};
        }

        case CompressionCodec::DICTIONARY: {
            // DICTIONARY encoding is string-only; not supported via raw_data_ decode path.
            // Callers using DICTIONARY-encoded segments must decode via DictionaryCodec::decodeStrings().
            return tl::unexpected(Error(
                errors::ErrorCode::ERR_CODEC_NOT_AVAILABLE,
                "DICTIONARY codec decode not available via raw decode path; "
                "use DictionaryCodec::decodeStrings() directly"
            ));
        }

        default:
            return tl::unexpected(Error(
                errors::ErrorCode::ERR_CODEC_NOT_AVAILABLE,
                "ColumnSegment::decode: unsupported compression codec"
            ));
    }
}

std::vector<uint8_t> ColumnSegment::serialize() const {
    std::vector<uint8_t> serialized;

    // Serialize metadata
    serialized.push_back(static_cast<uint8_t>(metadata_.type));
    serialized.push_back(static_cast<uint8_t>(metadata_.codec));

    // Sizes
    auto append_uint64 = [&](uint64_t val) {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&val);
        serialized.insert(serialized.end(), bytes, bytes + sizeof(uint64_t));
    };

    append_uint64(metadata_.uncompressed_size);
    append_uint64(metadata_.compressed_size);
    append_uint64(metadata_.row_count);

    // Encoded data
    append_uint64(encoded_data_.size());
    serialized.insert(serialized.end(), encoded_data_.begin(), encoded_data_.end());
    append_uint64(calculateSegmentChecksum(serialized.data(), serialized.size()));

    return serialized;
}

Result<ColumnSegment> ColumnSegment::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 2 + 4 * sizeof(uint64_t)) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_INVALID_FORMAT,
            "Segment deserialize: insufficient data"
        ));
    }

    ColumnSegment segment;
    size_t pos = 0;

    const auto raw_type = data[pos++];
    const auto raw_codec = data[pos++];

    if (raw_type > static_cast<uint8_t>(ColumnType::BOOL)) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_INVALID_FORMAT,
            "Segment deserialize: invalid column type"
        ));
    }

    if (raw_codec > static_cast<uint8_t>(CompressionCodec::SNAPPY)) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_INVALID_FORMAT,
            "Segment deserialize: invalid compression codec"
        ));
    }

    segment.metadata_.type = static_cast<ColumnType>(raw_type);
    segment.metadata_.codec = static_cast<CompressionCodec>(raw_codec);

    auto read_uint64 = [&]() -> uint64_t {
        if (pos + sizeof(uint64_t) > data.size()) {
            throw std::out_of_range("Segment deserialize: truncated metadata");
        }
        uint64_t val;
        std::memcpy(&val, &data[pos], sizeof(uint64_t));
        pos += sizeof(uint64_t);
        return val;
    };

    uint64_t encoded_size = 0;
    try {
        segment.metadata_.uncompressed_size = read_uint64();
        segment.metadata_.compressed_size = read_uint64();
        segment.metadata_.row_count = read_uint64();
        encoded_size = read_uint64();
    } catch (const std::out_of_range&) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_INVALID_FORMAT,
            "Segment deserialize: truncated metadata"
        ));
    }

    if (encoded_size > data.size() - pos) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_INVALID_FORMAT,
            "Segment deserialize: truncated data"
        ));
    }

    segment.encoded_data_.assign(data.begin() + pos, data.begin() + pos + encoded_size);
    pos += encoded_size;

    const size_t trailing_size = data.size() - pos;
    if (trailing_size == sizeof(uint64_t)) {
        uint64_t expected_checksum = 0;
        std::memcpy(&expected_checksum, &data[pos], sizeof(uint64_t));
        const uint64_t actual_checksum = calculateSegmentChecksum(data.data(), data.size() - sizeof(uint64_t));
        if (actual_checksum != expected_checksum) {
            return tl::unexpected(Error(
                errors::ErrorCode::ERR_COMPRESSION_INVALID_FORMAT,
                "Segment deserialize: checksum mismatch"
            ));
        }
    } else if (trailing_size != 0) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_COMPRESSION_INVALID_FORMAT,
            "Segment deserialize: invalid trailer size"
        ));
    }

    segment.is_encoded_ = true;

    return segment;
}

bool ColumnSegment::canSkipSegment(const void* filter_value) const {
    if (!filter_value) return false;

    switch (metadata_.type) {
        case ColumnType::INT32:
        [[fallthrough]];\n        case ColumnType::INT64: {
            int64_t value = (metadata_.type == ColumnType::INT32)
                ? *static_cast<const int32_t*>(filter_value)
                : *static_cast<const int64_t*>(filter_value);
            return metadata_.zone_map.canSkipForInt(value);
        }

        case ColumnType::FLOAT32:
        [[fallthrough]];\n        case ColumnType::FLOAT64: {
            double value = (metadata_.type == ColumnType::FLOAT32)
                ? *static_cast<const float*>(filter_value)
                : *static_cast<const double*>(filter_value);
            return metadata_.zone_map.canSkipForFloat(value);
        }

        default:
            return false;
    }
}

// ============================================================================
// ColumnarFormatManager Implementation
// ============================================================================

Result<std::vector<ColumnSegment>> ColumnarFormatManager::createSegments(
    const std::vector<ColumnType>& column_types,
    const std::vector<void*>& column_data,
    size_t row_count,
    bool auto_select_codec
) {
    if (column_types.size() != column_data.size()) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
            "Column types and data size mismatch"
        ));
    }

    std::vector<ColumnSegment> segments;
    segments.reserve(column_types.size());

    for (size_t i = 0; i < column_types.size(); ++i) {
        CompressionCodec codec = auto_select_codec
            ? CompressionCodec::NONE  // Will be auto-selected in create()
            : CompressionCodec::NONE;

        auto segment_result = ColumnSegment::create(
            column_types[i],
            column_data[i],
            row_count,
            codec
        );

        if (!segment_result) {
            return tl::unexpected(segment_result.error());
        }

        auto& segment = *segment_result;

        // Encode the segment
        auto encode_result = segment.encode();
        if (!encode_result) {
            return tl::unexpected(encode_result.error());
        }

        segments.push_back(std::move(segment));
    }

    return segments;
}

Result<std::vector<ColumnSegment>> ColumnarFormatManager::projectColumns(
    const std::vector<ColumnSegment>& segments,
    const std::vector<size_t>& column_indices
) {
    std::vector<ColumnSegment> projected;
    projected.reserve(column_indices.size());

    for (size_t idx : column_indices) {
        if (idx >= segments.size()) {
            return tl::unexpected(Error(
                errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                "Column index out of range"
            ));
        }
        projected.push_back(segments[idx]);
    }

    return projected;
}

Result<std::vector<size_t>> ColumnarFormatManager::filterSegments(
    const std::vector<ColumnSegment>& segments,
    size_t column_index,
    const void* filter_value
) {
    if (column_index >= segments.size()) {
        return tl::unexpected(Error(
            errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
            "Column index out of range"
        ));
    }

    std::vector<size_t> matching_indices;

    for (size_t i = 0; i < segments.size(); ++i) {
        if (!segments[i].canSkipSegment(filter_value)) {
            matching_indices.push_back(i);
        }
    }

    return matching_indices;
}

ColumnarFormatManager::CompressionStats
ColumnarFormatManager::getCompressionStats(const std::vector<ColumnSegment>& segments) const {
    CompressionStats stats;

    for (const auto& segment : segments) {
        stats.total_uncompressed += segment.metadata().uncompressed_size;
        stats.total_compressed += segment.metadata().compressed_size;

        auto codec = segment.metadata().codec;
        stats.codec_usage[codec]++;
    }

    if (stats.total_compressed > 0) {
        stats.avg_compression_ratio =
            static_cast<double>(stats.total_uncompressed) / stats.total_compressed;
    }

    return stats;
}

} // namespace storage
} // namespace themis
