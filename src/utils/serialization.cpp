/**
 * @file serialization.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "utils/serialization.h"
#include "utils/safe_cast.h"
#include "utils/error_contracts.h"
#include "utils/logger.h"
#include <cstring>
#include <fmt/format.h>

namespace themis {
namespace utils {

// Encoder implementation

Serialization::Encoder::Encoder() {
    buffer_.reserve(1024); // Pre-allocate
}

void Serialization::Encoder::writeTag(TypeTag tag) {
    buffer_.push_back(static_cast<uint8_t>(tag));
}

void Serialization::Encoder::writeUInt32([[maybe_unused]] uint32_t value) {
    buffer_.push_back((value >> 0) & 0xFF);
    buffer_.push_back((value >> 8) & 0xFF);
    buffer_.push_back((value >> 16) & 0xFF);
    buffer_.push_back((value >> 24) & 0xFF);
}

void Serialization::Encoder::writeUInt64([[maybe_unused]] uint64_t value) {
    for (int i = 0; i < 8; ++i) {
        buffer_.push_back((value >> (i * 8)) & 0xFF);
    }
}

void Serialization::Encoder::encodeNull() {
    writeTag(TypeTag::NULL_VALUE);
}

void Serialization::Encoder::encodeBool([[maybe_unused]] bool value) {
    writeTag(value ? TypeTag::BOOL_TRUE : TypeTag::BOOL_FALSE);
}

void Serialization::Encoder::encodeInt32(int32_t value) {
    writeTag(TypeTag::INT32);
    writeUInt32(static_cast<uint32_t>(value));
}

void Serialization::Encoder::encodeInt64(int64_t value) {
    writeTag(TypeTag::INT64);
    writeUInt64(static_cast<uint64_t>(value));
}

void Serialization::Encoder::encodeUInt32([[maybe_unused]] uint32_t value) {
    writeTag(TypeTag::UINT32);
    writeUInt32(value);
}

void Serialization::Encoder::encodeUInt64([[maybe_unused]] uint64_t value) {
    writeTag(TypeTag::UINT64);
    writeUInt64(value);
}

void Serialization::Encoder::encodeFloat([[maybe_unused]] float value) {
    writeTag(TypeTag::FLOAT);
    // Use safe_cast helper for clarity and consistency
    uint32_t bits = FloatBits::to_u32(value);
    writeUInt32(bits);
}

void Serialization::Encoder::encodeDouble([[maybe_unused]] double value) {
    writeTag(TypeTag::DOUBLE);
    // Use safe_cast helper for clarity and consistency
    uint64_t bits = FloatBits::to_u64(value);
    writeUInt64(bits);
}

void Serialization::Encoder::encodeString(std::string_view str) {
    writeTag(TypeTag::STRING);
    writeUInt32(static_cast<uint32_t>(str.size()));
    buffer_.insert(buffer_.end(), str.begin(), str.end());
}

void Serialization::Encoder::encodeBinary(const std::vector<uint8_t>& data) {
    writeTag(TypeTag::BINARY);
    writeUInt32(static_cast<uint32_t>(data.size()));
    buffer_.insert(buffer_.end(), data.begin(), data.end());
}

void Serialization::Encoder::encodeFloatVector(const std::vector<float>& vec) {
    writeTag(TypeTag::VECTOR_FLOAT);
    writeUInt32(static_cast<uint32_t>(vec.size()));
    
    // Write floats as raw bytes (platform-dependent but fast)
    // Note: reinterpret_cast to uint8_t* (or char*) is explicitly allowed by C++ standard
    // for accessing object representation (not a strict aliasing violation)
    const uint8_t* data = reinterpret_cast<const uint8_t*>(vec.data());
    buffer_.insert(buffer_.end(), data, data + vec.size() * sizeof(float));
}

void Serialization::Encoder::beginArray([[maybe_unused]] size_t size) {
    writeTag(TypeTag::ARRAY);
    writeUInt32(static_cast<uint32_t>(size));
}

void Serialization::Encoder::endArray() {
    // No-op for now
}

void Serialization::Encoder::beginObject([[maybe_unused]] size_t num_fields) {
    writeTag(TypeTag::OBJECT);
    writeUInt32(static_cast<uint32_t>(num_fields));
}

void Serialization::Encoder::endObject() {
    // No-op for now
}

std::vector<uint8_t> Serialization::Encoder::finish() {
    return std::move(buffer_);
}

// Decoder implementation

Serialization::Decoder::Decoder(const std::vector<uint8_t>& data) : data_(data) {}

Serialization::TypeTag Serialization::Decoder::peekType() const {
    if (pos_ >= data_.size()) {
        return TypeTag::NULL_VALUE;
    }
    return static_cast<TypeTag>(data_[pos_]);
}

Serialization::TypeTag Serialization::Decoder::readTag() {
    if (pos_ >= data_.size()) {
        logErrorWithContext(makeErrorContext(
            ErrorCode::DESERIALIZATION_FAILED,
            fmt::format("Decoder read past end: pos={} size={}", pos_, data_.size()),
            "Serialization::Decoder::readTag",
            ErrorSeverity::Warning, /*is_recoverable=*/false));
        return TypeTag::NULL_VALUE;
    }
    return static_cast<TypeTag>(data_[pos_++]);
}

uint32_t Serialization::Decoder::readUInt32() {
    // Phase A.4 Hardening - CRITICAL: Bounds check before reading 4 bytes
    if (pos_ + 4 > data_.size()) {
        // Bounds overflow detected - return 0 and don't advance pos_
        // This prevents silent out-of-bounds reads on malformed data
        return 0;
    }
    uint32_t value = 0;
    for (int i = 0; i < 4; ++i) {
        value |= static_cast<uint32_t>(data_[pos_++]) << (i * 8);
    }
    return value;
}

uint64_t Serialization::Decoder::readUInt64() {
    // Phase A.4 Hardening - CRITICAL: Bounds check before reading 8 bytes
    if (pos_ + 8 > data_.size()) {
        // Bounds overflow detected - return 0 and don't advance pos_
        // This prevents silent out-of-bounds reads on malformed data
        return 0;
    }
    uint64_t value = 0;
    for (int i = 0; i < 8; ++i) {
        value |= static_cast<uint64_t>(data_[pos_++]) << (i * 8);
    }
    return value;
}

bool Serialization::Decoder::isNull() const {
    return peekType() == TypeTag::NULL_VALUE;
}

bool Serialization::Decoder::decodeBool() {
    TypeTag tag = readTag();
    return tag == TypeTag::BOOL_TRUE;
}

int32_t Serialization::Decoder::decodeInt32() {
    readTag(); // Skip type tag
    return static_cast<int32_t>(readUInt32());
}

int64_t Serialization::Decoder::decodeInt64() {
    readTag();
    return static_cast<int64_t>(readUInt64());
}

uint32_t Serialization::Decoder::decodeUInt32() {
    readTag();
    return readUInt32();
}

uint64_t Serialization::Decoder::decodeUInt64() {
    readTag();
    return readUInt64();
}

float Serialization::Decoder::decodeFloat() {
    readTag();
    uint32_t bits = readUInt32();
    // Use safe_cast helper for clarity and consistency
    return FloatBits::from_u32(bits);
}

double Serialization::Decoder::decodeDouble() {
    readTag();
    uint64_t bits = readUInt64();
    // Use safe_cast helper for clarity and consistency
    return FloatBits::from_u64(bits);
}

std::string Serialization::Decoder::decodeString() {
    readTag();
    uint32_t size = readUInt32();
    
    // Phase A.4 Hardening - CRITICAL: Bounds check before creating string
    // Prevent out-of-bounds reads when deserializing untrusted data
    if (pos_ + size > data_.size()) {
        // Malformed: declared string size exceeds available buffer
        // Return empty string instead of reading past buffer
        pos_ = data_.size();  // Advance to EOF to prevent further reads
        return "";
    }
    
    // Note: reinterpret_cast to char* is explicitly allowed by C++ standard
    // for accessing object representation (not a strict aliasing violation)
    std::string str(reinterpret_cast<const char*>(&data_[pos_]), size);
    pos_ += size;
    return str;
}

std::vector<uint8_t> Serialization::Decoder::decodeBinary() {
    readTag();
    uint32_t size = readUInt32();
    
    // Phase A.4 Hardening - CRITICAL: Bounds check before vector construction
    // Prevent out-of-bounds reads and ensure safe vector initialization
    if (pos_ + size > data_.size()) {
        // Malformed: declared binary size exceeds available buffer
        // Return empty vector instead of reading past buffer
        pos_ = data_.size();  // Advance to EOF to prevent further reads
        return std::vector<uint8_t>();
    }
    
    std::vector<uint8_t> binary(data_.begin() + pos_, data_.begin() + pos_ + size);
    pos_ += size;
    return binary;
}

std::vector<float> Serialization::Decoder::decodeFloatVector() {
    readTag();
    uint32_t count = readUInt32();
    
    // Phase A.4 Hardening - CRITICAL: Bounds check before memcpy
    // Prevent out-of-bounds reads when deserializing float vectors
    size_t bytes_needed = static_cast<size_t>(count) * sizeof(float);
    if (pos_ + bytes_needed > data_.size()) {
        // Malformed: declared vector count exceeds available buffer
        pos_ = data_.size();  // Advance to EOF to prevent further reads
        return std::vector<float>();  // Return empty vector
    }
    
    std::vector<float> vec(count);
    const uint8_t* data = &data_[pos_];
    std::memcpy(vec.data(), data, bytes_needed);
    pos_ += bytes_needed;
    
    return vec;
}

size_t Serialization::Decoder::beginArray() {
    // Phase 2.4c Hardening: Check nesting depth to prevent stack overflow
    if (nesting_depth_ >= MAX_NESTING_DEPTH) {
        throw std::length_error(
            fmt::format("Deserialization nesting depth exceeds maximum {} (detected crafted/corrupt input)",
                       MAX_NESTING_DEPTH));
    }
    nesting_depth_++;
    readTag();
    return readUInt32();
}

void Serialization::Decoder::endArray() {
    // Phase 2.4c Hardening: Decrement nesting depth on array exit
    if (nesting_depth_ > 0) {
        nesting_depth_--;
    }
}

size_t Serialization::Decoder::beginObject() {
    // Phase 2.4c Hardening: Check nesting depth to prevent stack overflow
    if (nesting_depth_ >= MAX_NESTING_DEPTH) {
        throw std::length_error(
            fmt::format("Deserialization nesting depth exceeds maximum {} (detected crafted/corrupt input)",
                       MAX_NESTING_DEPTH));
    }
    nesting_depth_++;
    readTag();
    return readUInt32();
}

void Serialization::Decoder::endObject() {
    // Phase 2.4c Hardening: Decrement nesting depth on object exit
    if (nesting_depth_ > 0) {
        nesting_depth_--;
    }
}

bool Serialization::Decoder::hasMore() const {
    return pos_ < data_.size();
}

} // namespace utils
} // namespace themis
