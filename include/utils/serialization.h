/**
 * @file serialization.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace themis {
namespace utils {

/**
 * Serialization utilities for Base Entity blobs
 * 
 * Uses a compact binary format similar to VelocyPack or MessagePack
 * 
 * Sources:
 * - Inspired by: VelocyPack (ArangoDB)
 *   Repository: https://github.com/arangodb/velocypack
 *   License: Apache 2.0
 * - Inspired by: MessagePack
 *   Website: https://msgpack.org/
 *   License: Apache 2.0
 * - ThemisDB Implementation: Custom binary format optimized for:
 *   - Compact representation
 *   - Fast encoding/decoding
 *   - Native float vector support for embeddings
 *   - Zero-copy operations where possible
 */
class Serialization {
public:
    /// Type tags for serialized values
    enum class TypeTag : uint8_t {
        NULL_VALUE = 0x00,
        BOOL_FALSE = 0x01,
        BOOL_TRUE = 0x02,
        INT32 = 0x10,
        INT64 = 0x11,
        UINT32 = 0x12,
        UINT64 = 0x13,
        FLOAT = 0x20,
        DOUBLE = 0x21,
        STRING = 0x30,
        BINARY = 0x40,
        ARRAY = 0x50,
        OBJECT = 0x60,
        VECTOR_FLOAT = 0x70,  // Optimized for embeddings
    };
    
    /** @brief Encoder component. */
    class Encoder {
    public:
        Encoder();
        
        void encodeNull();
        void encodeBool(bool value);
        void encodeInt32(int32_t value);
        void encodeInt64(int64_t value);
        void encodeUInt32(uint32_t value);
        void encodeUInt64(uint64_t value);
        void encodeFloat(float value);
        void encodeDouble(double value);
        void encodeString(std::string_view str);
        void encodeBinary(const std::vector<uint8_t>& data);
        
        /// Encode float vector (for embeddings) - optimized format
        void encodeFloatVector(const std::vector<float>& vec);
        
        /// Begin/end array
        void beginArray(size_t size);
        void endArray();
        
        /// Begin/end object
        void beginObject(size_t num_fields);
        void endObject();
        
        /// Get encoded bytes
        std::vector<uint8_t> finish();
        
    private:
        std::vector<uint8_t> buffer_;
        void writeTag(TypeTag tag);
        void writeUInt32(uint32_t value);
        void writeUInt64(uint64_t value);
    };
    
    /** @brief Decoder component. */
    class Decoder {
    public:
        explicit Decoder(const std::vector<uint8_t>& data);
        
        TypeTag peekType() const;
        
        bool isNull() const;
        bool decodeBool();
        int32_t decodeInt32();
        int64_t decodeInt64();
        uint32_t decodeUInt32();
        uint64_t decodeUInt64();
        float decodeFloat();
        double decodeDouble();
        
        /**
         * @brief Decodes a string from the binary buffer with bounds checking.
         *
         * @return Decoded string value (empty string if size field is invalid or exceeds buffer bounds).
         *
         * @note Bounds-Checked: Verifies (pos_ + size_bytes) <= data_.size() before reading.
         * @note Fail-Safe: Returns empty string on bounds overflow instead of crashing.
         * @note Security: Prevents buffer overflow attacks from malformed serialized data.
         *
         * @throws May throw during UTF-8 validation if enabled, but not on bounds overflow.
         */
        std::string decodeString();
        
        /**
         * @brief Decodes binary data from the buffer with bounds checking.
         *
         * @return Decoded binary vector (empty if size field is invalid or exceeds buffer bounds).
         *
         * @note Bounds-Checked: Verifies (pos_ + size_bytes) <= data_.size() before reading.
         * @note Fail-Safe: Returns empty vector on bounds overflow.
         * @note Security: Prevents out-of-bounds reads on corrupted serialized data.
         */
        std::vector<uint8_t> decodeBinary();
        
        /**
         * @brief Decodes a float vector with overflow protection.
         *
         * @return Decoded vector of floats (empty if size exceeds buffer bounds).
         *
         * @note Bounds-Checked: Verifies (pos_ + size*sizeof(float)) <= data_.size() before reading.
         * @note Fail-Safe: Returns empty vector if array would exceed remaining buffer.
         * @note Security: Prevents integer overflow in size*sizeof calculation by explicit checks.
         */
        std::vector<float> decodeFloatVector();
        
        /**
         * @brief Begin array decoding with depth limiting for nested structures.
         *
         * @return Number of elements in the array.
         *
         * @note Depth-Limited: Arrays/objects nested >64 levels deep are rejected
         *       to prevent stack overflow attacks from malicious input.
         * @note Phase 2.4 Hardening: Bounds checking on nesting depth.
         *
         * @error Returns 0 if depth limit exceeded.
         */
        size_t beginArray();
        void endArray();
        
        /**
         * @brief Begin object decoding with depth limiting for nested structures.
         *
         * @return Number of fields in the object.
         *
         * @note Depth-Limited: Objects nested >64 levels deep are rejected
         *       to prevent stack overflow attacks from crafted deserialization.
         * @note Phase 2.4 Hardening: Bounds checking on nesting depth.
         *
         * @error Returns 0 if depth limit exceeded.
         */
        size_t beginObject();
        void endObject();
        
        bool hasMore() const;
        
        TypeTag readTag();
    
    private:
        const std::vector<uint8_t>& data_;
        size_t pos_ = 0;
        size_t depth_ = 0;  ///< Current nesting depth for arrays/objects (Phase 2.4 hardening)
        static constexpr size_t MAX_NESTING_DEPTH = 64;  ///< Prevent stack overflow from deep nesting
        
        uint32_t readUInt32();
        uint64_t readUInt64();
    };
};

} // namespace utils
} // namespace themis
