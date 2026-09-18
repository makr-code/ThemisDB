/**
 * @file columnar_format.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
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

    /**
     * @brief Check if a value can be filtered out based on zone map
     * @param[in] value Input parameter.
     * @return True on success.
     */
    bool canSkipForInt(int64_t value) const;
    /**
     * @brief TBD: Describe canSkipForFloat.
     * @param[in] value Input parameter.
     * @return True on success.
     */
    bool canSkipForFloat(double value) const;
    /**
     * @brief TBD: Describe canSkipForString.
     * @param[in] value Input parameter.
     * @return True on success.
     */
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
        if (compressed_size == 0) {
          return 0.0;
        }
        return static_cast<double>(uncompressed_size) / compressed_size;
    }
};

// ============================================================================
// RLE (Run-Length Encoding) Codec
// ============================================================================

/** @brief RLE (Run-Length Encoding) Codec. */
class RLECodec {
public:
    /**
     * @brief Encode integer data with RLE
     * @param[in] data Input parameter.
     * @return Return value.
     */
    static Result<std::vector<uint8_t>> encodeInt32(const std::vector<int32_t>& data);
    /**
     * @brief TBD: Describe encodeInt64.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    static Result<std::vector<uint8_t>> encodeInt64(const std::vector<int64_t>& data);

    /**
     * @brief Decode RLE data
     * @param[in] encoded Input parameter.
     * @return Return value.
     */
    static Result<std::vector<int32_t>> decodeInt32(const std::vector<uint8_t>& encoded);
    /**
     * @brief TBD: Describe decodeInt64.
     * @param[in] encoded Input parameter.
     * @return Return value.
     */
    static Result<std::vector<int64_t>> decodeInt64(const std::vector<uint8_t>& encoded);
};

// ============================================================================
// Dictionary Encoding Codec
// ============================================================================

/** @brief Dictionary Encoding Codec. */
class DictionaryCodec {
public:
    /**
     * @brief Encode string data with dictionary
     * @param[in] data Input parameter.
     * @return Return value.
     */
    static Result<std::vector<uint8_t>> encodeStrings(const std::vector<std::string>& data);

    /**
     * @brief Decode dictionary-encoded data
     * @param[in] encoded Input parameter.
     * @return Return value.
     */
    static Result<std::vector<std::string>> decodeStrings(const std::vector<uint8_t>& encoded);

    // Check if dictionary encoding is beneficial
    static bool shouldUseDictionary(const std::vector<std::string>& data,
                                   double min_compression_ratio = 1.5);
};

// ============================================================================
// Bit-Packing Codec
// ============================================================================

/** @brief Bit-Packing Codec. */
class BitPackingCodec {
public:
    /**
     * @brief Encode integers with minimal bits
     * @param[in] data Input parameter.
     * @return Return value.
     */
    static Result<std::vector<uint8_t>> encodeInt32(const std::vector<int32_t>& data);
    /**
     * @brief TBD: Describe encodeInt64.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    static Result<std::vector<uint8_t>> encodeInt64(const std::vector<int64_t>& data);

    /**
     * @brief Decode bit-packed data
     * @param[in] encoded Input parameter.
     * @return Return value.
     */
    static Result<std::vector<int32_t>> decodeInt32(const std::vector<uint8_t>& encoded);
    /**
     * @brief TBD: Describe decodeInt64.
     * @param[in] encoded Input parameter.
     * @return Return value.
     */
    static Result<std::vector<int64_t>> decodeInt64(const std::vector<uint8_t>& encoded);

private:
    /**
     * @brief Calculate required bits for value range
     * @param[in] min_val Input parameter.
     * @param[in] max_val Input parameter.
     * @return Return value.
     */
    static uint8_t calculateBitsRequired(int64_t min_val, int64_t max_val);
};

// ============================================================================
// Frame-of-Reference Encoding
// ============================================================================

/** @brief Frame-of-Reference Encoding. */
class FrameOfReferenceCodec {
public:
    /**
     * @brief Encode with frame-of-reference (subtract base value)
     * @param[in] data Input parameter.
     * @return Return value.
     */
    static Result<std::vector<uint8_t>> encodeInt32(const std::vector<int32_t>& data);
    /**
     * @brief TBD: Describe encodeInt64.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    static Result<std::vector<uint8_t>> encodeInt64(const std::vector<int64_t>& data);

    /**
     * @brief Decode frame-of-reference data
     * @param[in] encoded Input parameter.
     * @return Return value.
     */
    static Result<std::vector<int32_t>> decodeInt32(const std::vector<uint8_t>& encoded);
    /**
     * @brief TBD: Describe decodeInt64.
     * @param[in] encoded Input parameter.
     * @return Return value.
     */
    static Result<std::vector<int64_t>> decodeInt64(const std::vector<uint8_t>& encoded);
};

// ============================================================================
// Generic Compression Wrapper (LZ4/Snappy)
// ============================================================================

/** @brief Generic Compression Wrapper (LZ4/Snappy). */
class GenericCompressionCodec {
public:
    /**
     * @brief TBD: Describe compressLZ4.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    static Result<std::vector<uint8_t>> compressLZ4(const std::vector<uint8_t>& data);
    /**
     * @brief TBD: Describe decompressLZ4.
     * @param[in] compressed Input parameter.
     * @return Return value.
     */
    static Result<std::vector<uint8_t>> decompressLZ4(const std::vector<uint8_t>& compressed);

    /**
     * @brief TBD: Describe compressSnappy.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    static Result<std::vector<uint8_t>> compressSnappy(const std::vector<uint8_t>& data);
    /**
     * @brief TBD: Describe decompressSnappy.
     * @param[in] compressed Input parameter.
     * @return Return value.
     */
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

    /**
     * @brief Encode data with specified codec
     * @return Return value.
     */
    Result<void> encode();

    /**
     * @brief Decode data
     * @return Return value.
     */
    Result<void> decode();

    /**
     * @brief Serialize to bytes for storage
     * @return Return value.
     */
    std::vector<uint8_t> serialize() const;

    /**
     * @brief Deserialize from bytes
     * @param[in] data Input parameter.
     * @return Return value.
     */
    static Result<ColumnSegment> deserialize(const std::vector<uint8_t>& data);

    // Accessors
    const ColumnMetadata& metadata() const { return metadata_; }
    const std::vector<uint8_t>& encodedData() const { return encoded_data_; }
    const std::vector<uint8_t>& rawData() const { return raw_data_; }

    /**
     * @brief Query optimization support
     * @param[in] filter_value Input parameter.
     * @return True on success.
     */
    bool canSkipSegment(const void* filter_value) const;

private:
    ColumnMetadata metadata_;
    std::vector<uint8_t> raw_data_;
    std::vector<uint8_t> encoded_data_;
    bool is_encoded_ = false;

    /**
     * @brief Build zone map from raw data
     */
    void buildZoneMap();

    /**
     * @brief Select optimal codec based on data patterns
     * @param[in] type Input parameter.
     * @param[in] data Input parameter.
     * @param[in] row_count Input parameter.
     * @return Return value.
     */
    static CompressionCodec selectOptimalCodec(
        ColumnType type,
        const void* data,
        size_t row_count
    );
};

// ============================================================================
// Columnar Format Manager
// ============================================================================

/** @brief Columnar Format Manager. */
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

    /**
     * @brief Column projection - read only specified columns
     * @param[in] segments Input parameter.
     * @param[in] column_indices Input parameter.
     * @return Return value.
     */
    Result<std::vector<ColumnSegment>> projectColumns(
        const std::vector<ColumnSegment>& segments,
        const std::vector<size_t>& column_indices
    );

    /**
     * @brief Apply predicate filtering using zone maps
     * @param[in] segments Input parameter.
     * @param[in] column_index Input parameter.
     * @param[in] filter_value Input parameter.
     * @return Return value.
     */
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

    /**
     * @brief TBD: Describe getCompressionStats.
     * @param[in] segments Input parameter.
     * @return Return value.
     */
    CompressionStats getCompressionStats(const std::vector<ColumnSegment>& segments) const;
};

} // namespace storage
} // namespace themis
