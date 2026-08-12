/**
 * @file columnar_format.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <memory>
#include <unordered_map>
#include <optional>
#include "utils/expected.h"

namespace themis {
namespace storage {

// Forward declarations
class ColumnSegment;
class ColumnCodec;

// ============================================================================
// Compression Codec Types
// ============================================================================

enum class CompressionCodec : uint8_t {
    NONE = 0,
    RLE = 1,              // Run-Length Encoding
    DICTIONARY = 2,       // Dictionary Encoding
    BIT_PACKING = 3,      // Bit-Packing
    FRAME_OF_REF = 4,     // Frame-of-Reference
    LZ4 = 5,              // LZ4 Compression
    SNAPPY = 6            // Snappy Compression
};

// ============================================================================
// Column Data Types
// ============================================================================

enum class ColumnType : uint8_t {
    INT32 = 0,
    INT64 = 1,
    FLOAT32 = 2,
    FLOAT64 = 3,
    STRING = 4,
    BOOL = 5
};

// ============================================================================
// Zone Map (Min/Max per Block)
// ============================================================================

struct ZoneMap {
    int64_t min_int = 0;
    int64_t max_int = 0;
    double min_float = 0.0;
    double max_float = 0.0;
    std::string min_str;
    std::string max_str;
    size_t null_count = 0;
    size_t row_count = 0;

    // Check if a value can be filtered out based on zone map
    bool canSkipForInt(int64_t value) const;
    bool canSkipForFloat(double value) const;
    bool canSkipForString(const std::string& value) const;
};

// ============================================================================
// Column Segment Metadata
// ============================================================================

struct ColumnMetadata {
    ColumnType type;
    CompressionCodec codec;
    size_t uncompressed_size = 0;
    size_t compressed_size = 0;
    size_t row_count = 0;
    ZoneMap zone_map;

    // Compression ratio
    double compressionRatio() const {
        if (compressed_size == 0) return 0.0;
        return static_cast<double>(uncompressed_size) / compressed_size;
    }
};

// ============================================================================
// RLE (Run-Length Encoding) Codec
// ============================================================================

class RLECodec {
public:
    // Encode integer data with RLE
    static Result<std::vector<uint8_t>> encodeInt32(const std::vector<int32_t>& data);
    static Result<std::vector<uint8_t>> encodeInt64(const std::vector<int64_t>& data);

    // Decode RLE data
    static Result<std::vector<int32_t>> decodeInt32(const std::vector<uint8_t>& encoded);
    static Result<std::vector<int64_t>> decodeInt64(const std::vector<uint8_t>& encoded);
};

// ============================================================================
// Dictionary Encoding Codec
// ============================================================================

class DictionaryCodec {
public:
    // Encode string data with dictionary
    static Result<std::vector<uint8_t>> encodeStrings(const std::vector<std::string>& data);

    // Decode dictionary-encoded data
    static Result<std::vector<std::string>> decodeStrings(const std::vector<uint8_t>& encoded);

    // Check if dictionary encoding is beneficial
    static bool shouldUseDictionary(const std::vector<std::string>& data,
                                   double min_compression_ratio = 1.5);
};

// ============================================================================
// Bit-Packing Codec
// ============================================================================

class BitPackingCodec {
public:
    // Encode integers with minimal bits
    static Result<std::vector<uint8_t>> encodeInt32(const std::vector<int32_t>& data);
    static Result<std::vector<uint8_t>> encodeInt64(const std::vector<int64_t>& data);

    // Decode bit-packed data
    static Result<std::vector<int32_t>> decodeInt32(const std::vector<uint8_t>& encoded);
    static Result<std::vector<int64_t>> decodeInt64(const std::vector<uint8_t>& encoded);

private:
    // Calculate required bits for value range
    static uint8_t calculateBitsRequired(int64_t min_val, int64_t max_val);
};

// ============================================================================
// Frame-of-Reference Encoding
// ============================================================================

class FrameOfReferenceCodec {
public:
    // Encode with frame-of-reference (subtract base value)
    static Result<std::vector<uint8_t>> encodeInt32(const std::vector<int32_t>& data);
    static Result<std::vector<uint8_t>> encodeInt64(const std::vector<int64_t>& data);

    // Decode frame-of-reference data
    static Result<std::vector<int32_t>> decodeInt32(const std::vector<uint8_t>& encoded);
    static Result<std::vector<int64_t>> decodeInt64(const std::vector<uint8_t>& encoded);
};

// ============================================================================
// Generic Compression Wrapper (LZ4/Snappy)
// ============================================================================

class GenericCompressionCodec {
public:
    static Result<std::vector<uint8_t>> compressLZ4(const std::vector<uint8_t>& data);
    static Result<std::vector<uint8_t>> decompressLZ4(const std::vector<uint8_t>& compressed);

    static Result<std::vector<uint8_t>> compressSnappy(const std::vector<uint8_t>& data);
    static Result<std::vector<uint8_t>> decompressSnappy(const std::vector<uint8_t>& compressed);
};

// ============================================================================
// Column Segment
// ============================================================================

class ColumnSegment {
public:
    ColumnSegment() = default;

    // Create segment from raw data
    static Result<ColumnSegment> create(
        ColumnType type,
        const void* data,
        size_t row_count,
        CompressionCodec codec = CompressionCodec::NONE
    );

    // Encode data with specified codec
    Result<void> encode();

    // Decode data
    Result<void> decode();

    // Serialize to bytes for storage
    std::vector<uint8_t> serialize() const;

    // Deserialize from bytes
    static Result<ColumnSegment> deserialize(const std::vector<uint8_t>& data);

    // Accessors
    const ColumnMetadata& metadata() const { return metadata_; }
    const std::vector<uint8_t>& encodedData() const { return encoded_data_; }
    const std::vector<uint8_t>& rawData() const { return raw_data_; }

    // Query optimization support
    bool canSkipSegment(const void* filter_value) const;

private:
    ColumnMetadata metadata_;
    std::vector<uint8_t> raw_data_;
    std::vector<uint8_t> encoded_data_;
    bool is_encoded_ = false;

    // Build zone map from raw data
    void buildZoneMap();

    // Select optimal codec based on data patterns
    static CompressionCodec selectOptimalCodec(
        ColumnType type,
        const void* data,
        size_t row_count
    );
};

// ============================================================================
// Columnar Format Manager
// ============================================================================

class ColumnarFormatManager {
public:
    ColumnarFormatManager() = default;

    // Create columnar segments from row data
    Result<std::vector<ColumnSegment>> createSegments(
        const std::vector<ColumnType>& column_types,
        const std::vector<void*>& column_data,
        size_t row_count,
        bool auto_select_codec = true
    );

    // Column projection - read only specified columns
    Result<std::vector<ColumnSegment>> projectColumns(
        const std::vector<ColumnSegment>& segments,
        const std::vector<size_t>& column_indices
    );

    // Apply predicate filtering using zone maps
    Result<std::vector<size_t>> filterSegments(
        const std::vector<ColumnSegment>& segments,
        size_t column_index,
        const void* filter_value
    );

    // Get compression statistics
    struct CompressionStats {
        size_t total_uncompressed = 0;
        size_t total_compressed = 0;
        double avg_compression_ratio = 0.0;
        std::unordered_map<CompressionCodec, size_t> codec_usage;
    };

    CompressionStats getCompressionStats(const std::vector<ColumnSegment>& segments) const;
};

} // namespace storage
} // namespace themis
