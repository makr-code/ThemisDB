/**
 * @file wire_protocol_helpers.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB Wire Protocol Helpers
// Manual protobuf parsing/serialization for production use

#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <map>

namespace themis::network {

/**
 * @brief Lightweight protobuf wire format parser
 * 
 * This implements protobuf wire format parsing without requiring
 * protobuf library dependency, suitable for embedded use in the
 * wire protocol server.
 * 
 * Wire Format:
 * - Varint encoding for integers
 * - Length-delimited for strings/bytes
 * - Tag = (field_number << 3) | wire_type
 */
class ProtobufParser {
public:
    /**
     * @brief TBD: Describe ProtobufParser.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    explicit ProtobufParser(const std::vector<uint8_t>& data) 
        : data_(data), pos_(0) {}
    
    /**
     * @brief Parse varint (variable-length integer)
     * @param[in,out] value Input/output parameter.
     * @return True on success.
     */
    bool readVarint(uint64_t& value);
    
    /**
     * @brief Parse fixed 64-bit value
     * @param[in,out] value Input/output parameter.
     * @return True on success.
     */
    bool readFixed64(uint64_t& value);
    
    /**
     * @brief Parse fixed 32-bit value
     * @param[in,out] value Input/output parameter.
     * @return True on success.
     */
    bool readFixed32(uint32_t& value);
    
    /**
     * @brief Parse length-delimited field (string/bytes)
     * @param[in,out] value Input/output parameter.
     * @return True on success.
     */
    bool readLengthDelimited(std::vector<uint8_t>& value);
    /**
     * @brief TBD: Describe readString.
     * @param[in,out] value Input/output parameter.
     * @return True on success.
     */
    bool readString(std::string& value);
    
    /**
     * @brief Parse tag (field number + wire type)
     * @param[in,out] field_number Input/output parameter.
     * @param[in,out] wire_type Input/output parameter.
     * @return True on success.
     */
    bool readTag(uint32_t& field_number, uint32_t& wire_type);
    
    /**
     * @brief Skip unknown field
     * @param[in] wire_type Input parameter.
     * @return True on success.
     */
    bool skipField(uint32_t wire_type);
    
    // Check if at end
    bool atEnd() const { return pos_ >= data_.size(); }
    
    // Get current position
    size_t position() const { return pos_; }
    
private:
    const std::vector<uint8_t>& data_;
    size_t pos_;
};

/**
 * @brief Lightweight protobuf wire format serializer
 */
class ProtobufSerializer {
public:
    ProtobufSerializer() = default;
    
    /**
     * @brief Write varint
     * @param[in] value Input parameter.
     */
    void writeVarint(uint64_t value);
    
    /**
     * @brief Write fixed 64-bit
     * @param[in] value Input parameter.
     */
    void writeFixed64(uint64_t value);
    
    /**
     * @brief Write fixed 32-bit
     * @param[in] value Input parameter.
     */
    void writeFixed32(uint32_t value);
    
    /**
     * @brief Write length-delimited field
     * @param[in] value Input parameter.
     */
    void writeLengthDelimited(const std::vector<uint8_t>& value);
    /**
     * @brief TBD: Describe writeString.
     * @param[in] value Input parameter.
     */
    void writeString(const std::string& value);
    
    /**
     * @brief Write tag
     * @param[in] field_number Input parameter.
     * @param[in] wire_type Input parameter.
     */
    void writeTag(uint32_t field_number, uint32_t wire_type);
    
    /**
     * @brief Write double (as fixed64)
     * @param[in] value Input parameter.
     */
    void writeDouble(double value);
    
    // Get serialized data
    const std::vector<uint8_t>& data() const { return data_; }
    /**
     * @brief TBD: Describe take.
     * @return Return value.
     * @details Calls: std::move().
     */
    std::vector<uint8_t> take() { return std::move(data_); }
    
private:
    std::vector<uint8_t> data_;
};

/**
 * @brief TimeSeriesQueryRequest parser
 * 
 * Parses protobuf message:
 * message TimeSeriesQueryRequest {
 *   string collection = 1;
 *   uint64 start_time_ns = 2;
 *   uint64 end_time_ns = 3;
 *   Aggregation aggregation = 4;
 *   uint64 bucket_size_ns = 5;
 *   map<string, Value> filters = 6;  // Simplified: skip for MVP
 * }
 */
struct TimeSeriesQueryRequest {
    std::string collection;
    uint64_t start_time_ns = 0;
    uint64_t end_time_ns = 0;
    uint32_t aggregation = 0;  // 0=AVG, 1=SUM, 2=MIN, 3=MAX, 4=COUNT
    uint64_t bucket_size_ns = 0;
    
    /**
     * @brief Parse from protobuf wire format
     * @param[in] data Input parameter.
     * @param[in,out] request Input/output parameter.
     * @return True on success.
     */
    static bool parse(const std::vector<uint8_t>& data, TimeSeriesQueryRequest& request);
};

/**
 * @brief TimeSeriesQueryResponse serializer
 * 
 * Serializes protobuf message:
 * message TimeSeriesQueryResponse {
 *   repeated TimeSeriesBucket buckets = 1;
 *   uint64 query_time_us = 2;
 *   TimeSeriesStats stats = 3;
 * }
 */
struct TimeSeriesBucket {
    uint64_t timestamp_ns = 0;
    double value = 0.0;
    uint64_t count = 0;
    double min = 0.0;
    double max = 0.0;
    
    /**
     * @brief Serialize to protobuf wire format
     * @return Return value.
     */
    std::vector<uint8_t> serialize() const;
};

struct TimeSeriesStats {
    uint64_t total_data_points = 0;
    uint64_t buckets_returned = 0;
    double data_density = 0.0;
    
    /**
     * @brief Serialize to protobuf wire format
     * @return Return value.
     */
    std::vector<uint8_t> serialize() const;
};

struct TimeSeriesQueryResponse {
    std::vector<TimeSeriesBucket> buckets;
    uint64_t query_time_us = 0;
    TimeSeriesStats stats;
    
    /**
     * @brief Serialize to protobuf wire format
     * @return Return value.
     */
    std::vector<uint8_t> serialize() const;
};

} // namespace themis::network
