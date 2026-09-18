/**
 * @file serialization.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
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

/**
 * @brief Write Tag.
 * @param[in] tag Input parameter.
 * @details Calls: push_back().
 */
void Serialization::Encoder::writeTag(TypeTag tag) {
    buffer_.push_back(static_cast<uint8_t>(tag));
}

/**
 * @brief Write UInt32.
 * @param[in] value Input parameter.
 * @details Calls: push_back().
 */
void Serialization::Encoder::writeUInt32(uint32_t value) {
    buffer_.push_back((value >> 0) & 0xFF);
    buffer_.push_back((value >> 8) & 0xFF);
    buffer_.push_back((value >> 16) & 0xFF);
    buffer_.push_back((value >> 24) & 0xFF);
}

/**
 * @brief Write UInt64.
 * @param[in] value Input parameter.
 * @details Calls: push_back().
 */
void Serialization::Encoder::writeUInt64(uint64_t value) {
    for (int i = 0; i < 8; ++i) {
        buffer_.push_back((value >> (i * 8)) & 0xFF);
    }
}

/**
 * @brief Encode Null.
 * @details Calls: writeTag().
 */
void Serialization::Encoder::encodeNull() {
    writeTag(TypeTag::NULL_VALUE);
}

/**
 * @brief Encode Bool.
 * @param[in] value Input parameter.
 * @details Calls: writeTag().
 */
void Serialization::Encoder::encodeBool(bool value) {
    writeTag(value ? TypeTag::BOOL_TRUE : TypeTag::BOOL_FALSE);
}

/**
 * @brief Encode Int32.
 * @param[in] value Input parameter.
 * @details Calls: writeTag(), writeUInt32().
 */
void Serialization::Encoder::encodeInt32(int32_t value) {
    writeTag(TypeTag::INT32);
    writeUInt32(static_cast<uint32_t>(value));
}

/**
 * @brief Encode Int64.
 * @param[in] value Input parameter.
 * @details Calls: writeTag(), writeUInt64().
 */
void Serialization::Encoder::encodeInt64(int64_t value) {
    writeTag(TypeTag::INT64);
    writeUInt64(static_cast<uint64_t>(value));
}

/**
 * @brief Encode UInt32.
 * @param[in] value Input parameter.
 * @details Calls: writeTag(), writeUInt32().
 */
void Serialization::Encoder::encodeUInt32(uint32_t value) {
    writeTag(TypeTag::UINT32);
    writeUInt32(value);
}

/**
 * @brief Encode UInt64.
 * @param[in] value Input parameter.
 * @details Calls: writeTag(), writeUInt64().
 */
void Serialization::Encoder::encodeUInt64(uint64_t value) {
    writeTag(TypeTag::UINT64);
    writeUInt64(value);
}

/**
 * @brief Encode Float.
 * @param[in] value Input parameter.
 * @details Calls: writeTag(), FloatBits::to_u32(), writeUInt32().
 */
void Serialization::Encoder::encodeFloat(float value) {
    writeTag(TypeTag::FLOAT);
    // Use safe_cast helper for clarity and consistency
    uint32_t bits = FloatBits::to_u32(value);
    writeUInt32(bits);
}

/**
 * @brief Encode Double.
 * @param[in] value Input parameter.
 * @details Calls: writeTag(), FloatBits::to_u64(), writeUInt64().
 */
void Serialization::Encoder::encodeDouble(double value) {
    writeTag(TypeTag::DOUBLE);
    // Use safe_cast helper for clarity and consistency
    uint64_t bits = FloatBits::to_u64(value);
    writeUInt64(bits);
}

/**
 * @brief Encode String.
 * @param[in] str Input parameter.
 * @details Calls: writeTag(), writeUInt32(), size(), insert(), end(), begin().
 */
void Serialization::Encoder::encodeString(std::string_view str) {
    writeTag(TypeTag::STRING);
    writeUInt32(static_cast<uint32_t>(str.size()));
    buffer_.insert(buffer_.end(), str.begin(), str.end());
}

/**
 * @brief Encode Binary.
 * @param[in] data Input parameter.
 * @details Calls: writeTag(), writeUInt32(), size(), insert(), end(), begin().
 */
void Serialization::Encoder::encodeBinary(const std::vector<uint8_t>& data) {
    writeTag(TypeTag::BINARY);
    writeUInt32(static_cast<uint32_t>(data.size()));
    buffer_.insert(buffer_.end(), data.begin(), data.end());
}

/**
 * @brief Encode Float Vector.
 * @param[in] vec Input parameter.
 * @details Calls: writeTag(), writeUInt32(), size(), data(), insert(), end().
 */
void Serialization::Encoder::encodeFloatVector(const std::vector<float>& vec) {
    writeTag(TypeTag::VECTOR_FLOAT);
    writeUInt32(static_cast<uint32_t>(vec.size()));
    
    // Write floats as raw bytes (platform-dependent but fast)
    // Note: reinterpret_cast to uint8_t* (or char*) is explicitly allowed by C++ standard
    // for accessing object representation (not a strict aliasing violation)
    const uint8_t* data = reinterpret_cast<const uint8_t*>(vec.data());
    buffer_.insert(buffer_.end(), data, data + (vec.size() * sizeof(float)));
}

/**
 * @brief Begin Array.
 * @param[in] size Input parameter.
 * @details Calls: writeTag(), writeUInt32().
 */
void Serialization::Encoder::beginArray(size_t size) {
    writeTag(TypeTag::ARRAY);
    writeUInt32(static_cast<uint32_t>(size));
}

/**
 * @brief End Array.
 * @details Implements endArray without additional internal calls.
 */
void Serialization::Encoder::endArray() {
    // No-op for now
}

/**
 * @brief Begin Object.
 * @param[in] num_fields Input parameter.
 * @details Calls: writeTag(), writeUInt32().
 */
void Serialization::Encoder::beginObject(size_t num_fields) {
    writeTag(TypeTag::OBJECT);
    writeUInt32(static_cast<uint32_t>(num_fields));
}

/**
 * @brief End Object.
 * @details Implements endObject without additional internal calls.
 */
void Serialization::Encoder::endObject() {
    // No-op for now
}

/**
 * @brief Finish.
 * @return Return value.
 * @details Calls: std::move().
 */
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

/**
 * @brief Read Tag.
 * @return Return value.
 * @details Calls: size(), logErrorWithContext(), makeErrorContext(), fmt::format().
 */
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

/**
 * @brief Read UInt32.
 * @return Return value.
 * @details Calls: size().
 */
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

/**
 * @brief Read UInt64.
 * @return Return value.
 * @details Calls: size().
 */
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

/**
 * @brief Decode Bool.
 * @return True when the operation succeeds.
 * @details Calls: readTag().
 */
bool Serialization::Decoder::decodeBool() {
    TypeTag tag = readTag();
    return tag == TypeTag::BOOL_TRUE;
}

/**
 * @brief Decode Int32.
 * @return Return value.
 * @details Calls: readTag(), readUInt32().
 */
int32_t Serialization::Decoder::decodeInt32() {
    readTag(); // Skip type tag
    return static_cast<int32_t>(readUInt32());
}

/**
 * @brief Decode Int64.
 * @return Return value.
 * @details Calls: readTag(), readUInt64().
 */
int64_t Serialization::Decoder::decodeInt64() {
    readTag();
    return static_cast<int64_t>(readUInt64());
}

/**
 * @brief Decode UInt32.
 * @return Return value.
 * @details Calls: readTag(), readUInt32().
 */
uint32_t Serialization::Decoder::decodeUInt32() {
    readTag();
    return readUInt32();
}

/**
 * @brief Decode UInt64.
 * @return Return value.
 * @details Calls: readTag(), readUInt64().
 */
uint64_t Serialization::Decoder::decodeUInt64() {
    readTag();
    return readUInt64();
}

/**
 * @brief Decode Float.
 * @return Return value.
 * @details Calls: readTag(), readUInt32(), FloatBits::from_u32().
 */
float Serialization::Decoder::decodeFloat() {
    readTag();
    uint32_t bits = readUInt32();
    // Use safe_cast helper for clarity and consistency
    return FloatBits::from_u32(bits);
}

/**
 * @brief Decode Double.
 * @return Return value.
 * @details Calls: readTag(), readUInt64(), FloatBits::from_u64().
 */
double Serialization::Decoder::decodeDouble() {
    readTag();
    uint64_t bits = readUInt64();
    // Use safe_cast helper for clarity and consistency
    return FloatBits::from_u64(bits);
}

/**
 * @brief Decode String.
 * @return Return value.
 * @details Calls: readTag(), readUInt32(), size(), str().
 */
std::string Serialization::Decoder::decodeString() {
    readTag();
    const size_t size = static_cast<size_t>(readUInt32());
    
    // Phase A.4 Hardening - CRITICAL: Bounds check before creating string
    // Prevent out-of-bounds reads when deserializing untrusted data
    if (pos_ > data_.size() || size > data_.size() - pos_) {
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

/**
 * @brief Decode Binary.
 * @return Return value.
 * @details Calls: readTag(), readUInt32(), size(), binary(), begin().
 */
std::vector<uint8_t> Serialization::Decoder::decodeBinary() {
    readTag();
    const size_t size = static_cast<size_t>(readUInt32());
    
    // Phase A.4 Hardening - CRITICAL: Bounds check before vector construction
    // Prevent out-of-bounds reads and ensure safe vector initialization
    if (pos_ > data_.size() || size > data_.size() - pos_) {
        // Malformed: declared binary size exceeds available buffer
        // Return empty vector instead of reading past buffer
        pos_ = data_.size();  // Advance to EOF to prevent further reads
        return std::vector<uint8_t>();
    }
    
    std::vector<uint8_t> binary(data_.begin() + pos_, data_.begin() + pos_ + size);
    pos_ += size;
    return binary;
}

/**
 * @brief Decode Float Vector.
 * @return Return value.
 * @details Calls: readTag(), readUInt32(), size(), vec(), std::memcpy(), data().
 */
std::vector<float> Serialization::Decoder::decodeFloatVector() {
    readTag();
    const size_t count = static_cast<size_t>(readUInt32());
    
    // Phase A.4 Hardening - CRITICAL: Bounds check before memcpy
    // Prevent out-of-bounds reads when deserializing float vectors
    const size_t bytes_needed = count * sizeof(float);
    if (pos_ > data_.size() || bytes_needed > data_.size() - pos_) {
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

/**
 * @brief Begin Array.
 * @return Return value.
 * @throws std::length_error if an error occurs.
 * @details Calls: fmt::format(), readTag(), readUInt32().
 */
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

/**
 * @brief End Array.
 * @details Implements endArray without additional internal calls.
 */
void Serialization::Decoder::endArray() {
    // Phase 2.4c Hardening: Decrement nesting depth on array exit
    if (nesting_depth_ > 0) {
        nesting_depth_--;
    }
}

/**
 * @brief Begin Object.
 * @return Return value.
 * @throws std::length_error if an error occurs.
 * @details Calls: fmt::format(), readTag(), readUInt32().
 */
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

/**
 * @brief End Object.
 * @details Implements endObject without additional internal calls.
 */
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
