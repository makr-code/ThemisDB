/**
 * @file test_serialization.cpp
 * @brief Comprehensive tests for Serialization encoder/decoder with bounds checking
 * @date 2026-08-17
 *
 * Tests Phase A.4 hardening for serialization.cpp:
 * - Buffer overflow prevention in decoder
 * - Malformed data handling
 * - Round-trip correctness
 * - Edge cases (empty data, truncated streams, oversized claims)
 */

#include <gtest/gtest.h>
#include "utils/serialization.h"
#include <cstring>

namespace themis {
namespace utils {

class SerializationTest : public ::testing::Test {
protected:
    Serialization::Encoder encoder;
    
    void SetUp() override {
        encoder = Serialization::Encoder();
    }
};

// ============================================================================
// Test Phase A.4 Hardening: Decoder Bounds Checking
// ============================================================================

TEST_F(SerializationTest, DecoderUInt32BoundsOverflow) {
    // Scenario: Decoder claims to read UInt32 from empty buffer
    std::vector<uint8_t> truncated = {
        static_cast<uint8_t>(Serialization::TypeTag::UINT32)
        // Missing the 4 bytes for the value
    };
    
    Serialization::Decoder decoder(truncated);
    TypeTag tag = decoder.readTag();
    EXPECT_EQ(tag, Serialization::TypeTag::UINT32);
    
    // Attempting to read 4 bytes from empty buffer should not crash
    // and should return 0 safely
    uint32_t value = decoder.decodeUInt32();
    EXPECT_EQ(value, 0);
}

TEST_F(SerializationTest, DecoderUInt64BoundsOverflow) {
    // Scenario: Decoder claims to read UInt64 from buffer with only 4 bytes
    std::vector<uint8_t> truncated;
    truncated.push_back(static_cast<uint8_t>(Serialization::TypeTag::UINT64));
    truncated.push_back(0x01);
    truncated.push_back(0x02);
    truncated.push_back(0x03);
    truncated.push_back(0x04);
    // Missing 4 more bytes
    
    Serialization::Decoder decoder(truncated);
    uint64_t value = decoder.decodeUInt64();
    EXPECT_EQ(value, 0);  // Should return 0, not read past buffer
}

TEST_F(SerializationTest, DecoderStringBoundsOverflow) {
    // Scenario: String size declares 100 bytes but buffer only has 10
    std::vector<uint8_t> malformed;
    malformed.push_back(static_cast<uint8_t>(Serialization::TypeTag::STRING));
    
    // Size: 100 (as UInt32 little-endian)
    malformed.push_back(0x64);  // 100 in LE
    malformed.push_back(0x00);
    malformed.push_back(0x00);
    malformed.push_back(0x00);
    
    // Only 6 bytes of data (not 100)
    malformed.push_back('h');
    malformed.push_back('e');
    malformed.push_back('l');
    malformed.push_back('l');
    malformed.push_back('o');
    malformed.push_back(0x00);
    
    Serialization::Decoder decoder(malformed);
    std::string str = decoder.decodeString();
    
    // Should return empty string (or truncated to available), not crash
    EXPECT_TRUE(str.empty() || str.size() <= 6);
}

TEST_F(SerializationTest, DecoderBinaryBoundsOverflow) {
    // Scenario: Binary size declares 1000 bytes but buffer has only 20
    std::vector<uint8_t> malformed;
    malformed.push_back(static_cast<uint8_t>(Serialization::TypeTag::BINARY));
    
    // Size: 1000 (as UInt32 little-endian)
    malformed.push_back(0xE8);  // 1000 in LE
    malformed.push_back(0x03);
    malformed.push_back(0x00);
    malformed.push_back(0x00);
    
    // Only 16 bytes of data
    for (int i = 0; i < 16; ++i) {
        malformed.push_back(0xFF);
    }
    
    Serialization::Decoder decoder(malformed);
    std::vector<uint8_t> binary = decoder.decodeBinary();
    
    // Should return empty vector or truncated, not crash with out-of-bounds
    EXPECT_TRUE(binary.empty() || binary.size() <= 16);
}

TEST_F(SerializationTest, DecoderFloatVectorBoundsOverflow) {
    // Scenario: Float vector claims 1000 floats (4000 bytes) but has only 100
    std::vector<uint8_t> malformed;
    malformed.push_back(static_cast<uint8_t>(Serialization::TypeTag::FLOAT_VECTOR));
    
    // Count: 1000 floats
    malformed.push_back(0xE8);  // 1000 in LE
    malformed.push_back(0x03);
    malformed.push_back(0x00);
    malformed.push_back(0x00);
    
    // Only 96 bytes of data (not 4000)
    for (int i = 0; i < 96; ++i) {
        malformed.push_back(0x00);
    }
    
    Serialization::Decoder decoder(malformed);
    std::vector<float> vec = decoder.decodeFloatVector();
    
    // Should return empty or partial vector, not crash with out-of-bounds memcpy
    EXPECT_TRUE(vec.empty() || vec.size() * sizeof(float) <= 96);
}

// ============================================================================
// Test Correct Round-Trip Encoding/Decoding
// ============================================================================

TEST_F(SerializationTest, RoundTripUInt32) {
    encoder.encodeUInt32(0xDEADBEEF);
    auto buffer = encoder.getBuffer();
    
    Serialization::Decoder decoder(buffer);
    uint32_t decoded = decoder.decodeUInt32();
    
    EXPECT_EQ(decoded, 0xDEADBEEF);
}

TEST_F(SerializationTest, RoundTripUInt64) {
    encoder.encodeUInt64(0x0123456789ABCDEFULL);
    auto buffer = encoder.getBuffer();
    
    Serialization::Decoder decoder(buffer);
    uint64_t decoded = decoder.decodeUInt64();
    
    EXPECT_EQ(decoded, 0x0123456789ABCDEFULL);
}

TEST_F(SerializationTest, RoundTripString) {
    std::string original = "Hello, World! テスト";
    encoder.encodeString(original);
    auto buffer = encoder.getBuffer();
    
    Serialization::Decoder decoder(buffer);
    std::string decoded = decoder.decodeString();
    
    EXPECT_EQ(decoded, original);
}

TEST_F(SerializationTest, RoundTripBinary) {
    std::vector<uint8_t> original = {0x00, 0x11, 0x22, 0x33, 0xFF, 0xFE};
    encoder.encodeBinary(original);
    auto buffer = encoder.getBuffer();
    
    Serialization::Decoder decoder(buffer);
    std::vector<uint8_t> decoded = decoder.decodeBinary();
    
    EXPECT_EQ(decoded, original);
}

TEST_F(SerializationTest, RoundTripFloatVector) {
    std::vector<float> original = {1.0f, 2.5f, 3.14159f, -0.5f};
    encoder.encodeFloatVector(original);
    auto buffer = encoder.getBuffer();
    
    Serialization::Decoder decoder(buffer);
    std::vector<float> decoded = decoder.decodeFloatVector();
    
    EXPECT_EQ(decoded.size(), original.size());
    for (size_t i = 0; i < original.size(); ++i) {
        EXPECT_FLOAT_EQ(decoded[i], original[i]);
    }
}

// ============================================================================
// Test Edge Cases
// ============================================================================

TEST_F(SerializationTest, EmptyBuffer) {
    std::vector<uint8_t> empty;
    Serialization::Decoder decoder(empty);
    
    // Attempting to read from empty buffer should handle gracefully
    TypeTag tag = decoder.readTag();
    EXPECT_EQ(tag, Serialization::TypeTag::NULL_VALUE);
}

TEST_F(SerializationTest, EmptyString) {
    encoder.encodeString("");
    auto buffer = encoder.getBuffer();
    
    Serialization::Decoder decoder(buffer);
    std::string decoded = decoder.decodeString();
    
    EXPECT_EQ(decoded, "");
}

TEST_F(SerializationTest, EmptyBinary) {
    encoder.encodeBinary(std::vector<uint8_t>());
    auto buffer = encoder.getBuffer();
    
    Serialization::Decoder decoder(buffer);
    std::vector<uint8_t> decoded = decoder.decodeBinary();
    
    EXPECT_TRUE(decoded.empty());
}

TEST_F(SerializationTest, LargeString) {
    // Create a large string (1MB)
    std::string large(1024 * 1024, 'A');
    encoder.encodeString(large);
    auto buffer = encoder.getBuffer();
    
    Serialization::Decoder decoder(buffer);
    std::string decoded = decoder.decodeString();
    
    EXPECT_EQ(decoded, large);
}

TEST_F(SerializationTest, LargeBinary) {
    // Create large binary data (2MB)
    std::vector<uint8_t> large(2 * 1024 * 1024, 0xAB);
    encoder.encodeBinary(large);
    auto buffer = encoder.getBuffer();
    
    Serialization::Decoder decoder(buffer);
    std::vector<uint8_t> decoded = decoder.decodeBinary();
    
    EXPECT_EQ(decoded, large);
}

TEST_F(SerializationTest, TruncatedInMiddleOfRead) {
    // Encode multiple values
    encoder.encodeUInt32(12345);
    encoder.encodeString("test");
    auto buffer = encoder.getBuffer();
    
    // Truncate buffer mid-way
    std::vector<uint8_t> truncated(buffer.begin(), buffer.begin() + buffer.size() / 2);
    
    Serialization::Decoder decoder(truncated);
    uint32_t value = decoder.decodeUInt32();
    EXPECT_EQ(value, 12345);
    
    // Next read should handle truncation gracefully
    std::string str = decoder.decodeString();
    EXPECT_TRUE(str.empty() || str.size() < 4);  // Should be truncated or empty
}

} // namespace utils
} // namespace themis

