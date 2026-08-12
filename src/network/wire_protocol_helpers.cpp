/**
 * @file wire_protocol_helpers.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=11; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=8, Debt=0, C=0, H=1, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB Wire Protocol Helpers Implementation

#include "network/wire_protocol_helpers.h"
#include <algorithm>

namespace themis::network {

// =============================================================================
// ProtobufParser Implementation
// =============================================================================

bool ProtobufParser::readVarint(uint64_t& value) {
    value = 0;
    int shift = 0;
    
    while (pos_ < data_.size()) {
        uint8_t byte = data_[pos_++];
        value |= static_cast<uint64_t>(byte & 0x7F) << shift;
        
        if ((byte & 0x80) == 0) {
            return true;  // Last byte
        }
        
        shift += 7;
        if (shift >= 64) {
            return false;  // Overflow
        }
    }
    
    return false;  // Unexpected end
}

bool ProtobufParser::readFixed64(uint64_t& value) {
    if (pos_ + 8 > data_.size()) {
        return false;
    }
    
    // Protobuf fixed64 is little-endian on wire
    value = 0;
    for (int i = 0; i < 8; ++i) {
        value |= static_cast<uint64_t>(data_[pos_ + i]) << (i * 8);
    }
    pos_ += 8;
    return true;
}

bool ProtobufParser::readFixed32(uint32_t& value) {
    if (pos_ + 4 > data_.size()) {
        return false;
    }
    
    // Protobuf fixed32 is little-endian on wire
    value = 0;
    for (int i = 0; i < 4; ++i) {
        value |= static_cast<uint32_t>(data_[pos_ + i]) << (i * 8);
    }
    pos_ += 4;
    return true;
}

bool ProtobufParser::readLengthDelimited(std::vector<uint8_t>& value) {
    uint64_t length = 0;
    if (!readVarint(length)) {
        return false;
    }
    
    if (pos_ + length > data_.size()) {
        return false;
    }
    
    value.assign(data_.begin() + pos_, data_.begin() + pos_ + length);
    pos_ += length;
    return true;
}

bool ProtobufParser::readString(std::string& value) {
    std::vector<uint8_t> bytes;
    if (!readLengthDelimited(bytes)) {
        return false;
    }
    
    value.assign(bytes.begin(), bytes.end());
    return true;
}

bool ProtobufParser::readTag(uint32_t& field_number, uint32_t& wire_type) {
    uint64_t tag = 0;
    if (!readVarint(tag)) {
        return false;
    }
    
    field_number = static_cast<uint32_t>(tag >> 3);
    wire_type = static_cast<uint32_t>(tag & 0x07);
    return true;
}

bool ProtobufParser::skipField(uint32_t wire_type) {
    switch (wire_type) {
        case 0: {  // Varint
            uint64_t dummy;
            return readVarint(dummy);
        }
        case 1: {  // 64-bit
            uint64_t dummy;
            return readFixed64(dummy);
        }
        case 2: {  // Length-delimited
            std::vector<uint8_t> dummy;
            return readLengthDelimited(dummy);
        }
        case 5: {  // 32-bit
            uint32_t dummy;
            return readFixed32(dummy);
        }
        default:
            return false;  // Unknown wire type
    }
}

// =============================================================================
// ProtobufSerializer Implementation
// =============================================================================

void ProtobufSerializer::writeVarint(uint64_t value) {
    while (value >= 0x80) {
        data_.push_back(static_cast<uint8_t>((value & 0x7F) | 0x80));
        value >>= 7;
    }
    data_.push_back(static_cast<uint8_t>(value & 0x7F));
}

void ProtobufSerializer::writeFixed64(uint64_t value) {
    // Protobuf fixed64 is little-endian on wire
    for (int i = 0; i < 8; ++i) {
        data_.push_back(static_cast<uint8_t>(value & 0xFF));
        value >>= 8;
    }
}

void ProtobufSerializer::writeFixed32(uint32_t value) {
    // Protobuf fixed32 is little-endian on wire
    for (int i = 0; i < 4; ++i) {
        data_.push_back(static_cast<uint8_t>(value & 0xFF));
        value >>= 8;
    }
}

void ProtobufSerializer::writeLengthDelimited(const std::vector<uint8_t>& value) {
    writeVarint(value.size());
    data_.insert(data_.end(), value.begin(), value.end());
}

void ProtobufSerializer::writeString(const std::string& value) {
    writeVarint(value.size());
    data_.insert(data_.end(), value.begin(), value.end());
}

void ProtobufSerializer::writeTag(uint32_t field_number, uint32_t wire_type) {
    writeVarint((field_number << 3) | wire_type);
}

void ProtobufSerializer::writeDouble(double value) {
    uint64_t bits;
    std::memcpy(&bits, &value, sizeof(double));
    writeFixed64(bits);
}

// =============================================================================
// TimeSeriesQueryRequest Implementation
// =============================================================================

bool TimeSeriesQueryRequest::parse(const std::vector<uint8_t>& data, TimeSeriesQueryRequest& request) {
    ProtobufParser parser(data);
    
    while (!parser.atEnd()) {
        uint32_t field_number = 0;
        uint32_t wire_type = 0;
        
        if (!parser.readTag(field_number, wire_type)) {
            return false;
        }
        
        switch (field_number) {
            case 1:  // collection
                if (wire_type != 2) return false;  // Must be length-delimited
                if (!parser.readString(request.collection)) return false;
                break;
                
            case 2:  // start_time_ns
                if (wire_type != 0) return false;  // Must be varint
                if (!parser.readVarint(request.start_time_ns)) return false;
                break;
                
            case 3:  // end_time_ns
                if (wire_type != 0) return false;  // Must be varint
                if (!parser.readVarint(request.end_time_ns)) return false;
                break;
                
            case 4:  // aggregation
                if (wire_type != 0) return false;  // Must be varint
                {
                    uint64_t agg_value = 0;
                    if (!parser.readVarint(agg_value)) return false;
                    request.aggregation = static_cast<uint32_t>(agg_value);
                }
                break;
                
            case 5:  // bucket_size_ns
                if (wire_type != 0) return false;  // Must be varint
                if (!parser.readVarint(request.bucket_size_ns)) return false;
                break;
                
            case 6:  // filters (skip for now - complex map type)
                if (!parser.skipField(wire_type)) return false;
                break;
                
            default:
                // Unknown field - skip it
                if (!parser.skipField(wire_type)) return false;
                break;
        }
    }
    
    // Validate required fields
    if (request.collection.empty()) {
        return false;
    }
    
    return true;
}

// =============================================================================
// TimeSeriesBucket Implementation
// =============================================================================

std::vector<uint8_t> TimeSeriesBucket::serialize() const {
    ProtobufSerializer serializer;
    
    // Field 1: timestamp_ns (varint) - ALWAYS serialize for proper ordering
    serializer.writeTag(1, 0);  // varint
    serializer.writeVarint(timestamp_ns);
    
    // Field 2: value (double/fixed64) - ALWAYS serialize, zero is valid
    serializer.writeTag(2, 1);  // fixed64
    serializer.writeDouble(value);
    
    // Field 3: count (varint) - ALWAYS serialize for completeness
    serializer.writeTag(3, 0);  // varint
    serializer.writeVarint(count);
    
    // Field 4: min (double/fixed64) - ALWAYS serialize, zero is valid
    serializer.writeTag(4, 1);  // fixed64
    serializer.writeDouble(min);
    
    // Field 5: max (double/fixed64) - ALWAYS serialize, zero is valid
    serializer.writeTag(5, 1);  // fixed64
    serializer.writeDouble(max);
    
    return serializer.take();
}

// =============================================================================
// TimeSeriesStats Implementation
// =============================================================================

std::vector<uint8_t> TimeSeriesStats::serialize() const {
    ProtobufSerializer serializer;
    
    // Field 1: total_data_points (varint)
    if (total_data_points != 0) {
        serializer.writeTag(1, 0);
        serializer.writeVarint(total_data_points);
    }
    
    // Field 2: buckets_returned (varint)
    if (buckets_returned != 0) {
        serializer.writeTag(2, 0);
        serializer.writeVarint(buckets_returned);
    }
    
    // Field 3: data_density (double/fixed64)
    if (data_density != 0.0) {
        serializer.writeTag(3, 1);
        serializer.writeDouble(data_density);
    }
    
    return serializer.take();
}

// =============================================================================
// TimeSeriesQueryResponse Implementation
// =============================================================================

std::vector<uint8_t> TimeSeriesQueryResponse::serialize() const {
    ProtobufSerializer serializer;
    
    // Field 1: repeated buckets (length-delimited)
    for (const auto& bucket : buckets) {
        serializer.writeTag(1, 2);  // length-delimited
        auto bucket_data = bucket.serialize();
        serializer.writeLengthDelimited(bucket_data);
    }
    
    // Field 2: query_time_us (varint)
    if (query_time_us != 0) {
        serializer.writeTag(2, 0);
        serializer.writeVarint(query_time_us);
    }
    
    // Field 3: stats (length-delimited)
    {
        serializer.writeTag(3, 2);
        auto stats_data = stats.serialize();
        serializer.writeLengthDelimited(stats_data);
    }
    
    return serializer.take();
}

} // namespace themis::network
