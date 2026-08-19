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
        
        /**
         * @brief Finalise the encoded buffer and return the serialized bytes.
         *
         * @return Serialized binary representation of all encoded values.
         *
         * @note Nesting safety: Incomplete array/object nesting (unmatched
         *       begin/end calls) produces an internally consistent buffer but
         *       may fail decoding. Validate with Decoder after encoding.
         *
         * @error_contract
         * | Condition | ErrorCode | Severity | Logging | Recovery |
         * |-----------|-----------|----------|---------|----------|
         * | Memory exhaustion while encode* operations append/grow the buffer | std::bad_alloc (SERIALIZATION_FAILED 9080) | Critical | – | Propagates exception |
         *
         * @bounded_resources Nesting depth is limited by available stack space; callers
         *   are responsible for bounding the number of nested begin/end calls.
         */
        std::vector<uint8_t> finish();
        
    private:
        std::vector<uint8_t> buffer_;
        void writeTag(TypeTag tag);
        void writeUInt32(uint32_t value);
        void writeUInt64(uint64_t value);
    };
    
    /**
     * @brief Decoder component.
     *
     * Decodes bytes produced by Encoder::finish(). All decode methods
     * perform bounds checking before accessing the backing buffer; they
     * return safe defaults (0, false, empty string/vector) rather than
     * crashing on malformed or truncated input.
     *
     * @note Nesting depth: Encoded arrays/objects may be nested up to the
     *   available stack depth. Crafted inputs with extreme nesting may cause
     *   stack overflow; callers that accept untrusted input should bound
     *   maximum nesting depth externally.
     *
     * @error_contract
     * | Condition | ErrorCode | Severity | Logging | Recovery |
     * |-----------|-----------|----------|---------|----------|
     * | Buffer read beyond data end (bounds overflow) | DESERIALIZATION_FAILED (9081) | Error | logErrorWithContext | Returns safe default (0/false/empty) |
     * | Unexpected type tag encountered | SERIALIZATION_FORMAT_INVALID (9082) | Error | logErrorWithContext | Returns safe default |
     * | String/binary size field exceeds buffer | DESERIALIZATION_FAILED (9081) | Error | logErrorWithContext | Returns empty string/vector |
     * | Float-vector element count × sizeof(float) overflows | DESERIALIZATION_FAILED (9081) | Error | logErrorWithContext | Returns empty vector |
     *
     * @degradation All decode failures return safe defaults (never undefined
     *   behaviour). Callers should check hasMore() and peekType() before
     *   decoding if strict error detection is required.
     */
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
        
        size_t beginArray();
        void endArray();
        
        size_t beginObject();
        void endObject();
        
        bool hasMore() const;
        
        TypeTag readTag();
    
    private:
        static constexpr size_t MAX_NESTING_DEPTH = 64; ///< Maximum allowed array/object nesting depth.

        const std::vector<uint8_t>& data_;
        size_t pos_ = 0;
        size_t depth_ = 0; ///< Current array/object nesting depth for overflow protection.

        uint32_t readUInt32();
        uint64_t readUInt64();
    };
};

} // namespace utils
} // namespace themis
