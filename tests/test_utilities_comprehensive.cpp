/**
 * @file test_utilities_comprehensive.cpp
 * @brief Comprehensive unit tests for ThemisDB utility headers:
 *        string_utils, safe_arithmetic, safe_cast, serialization.
 */

#include <gtest/gtest.h>
#include "utils/string_utils.h"
#include "utils/safe_arithmetic.h"
#include "utils/safe_cast.h"
#include "utils/serialization.h"
#include <cstdint>
#include <cmath>
#include <limits>
#include <optional>
#include <string>

namespace themis {
namespace utils {
namespace test {

// ═══════════════════════════════════════════════════════════
// string_utils tests
// ═══════════════════════════════════════════════════════════

TEST(StringUtils, ContainsCaseInsensitive_BasicMatch) {
    EXPECT_TRUE(containsCaseInsensitive("Hello World", "world"));
    EXPECT_TRUE(containsCaseInsensitive("ThemisDB", "THEMIS"));
    EXPECT_TRUE(containsCaseInsensitive("abcDEF", "cde"));
}

TEST(StringUtils, ContainsCaseInsensitive_NoMatch) {
    EXPECT_FALSE(containsCaseInsensitive("Hello", "xyz"));
    EXPECT_FALSE(containsCaseInsensitive("", "a"));
}

TEST(StringUtils, ContainsCaseInsensitive_EmptySubstring) {
    // An empty needle always matches
    EXPECT_TRUE(containsCaseInsensitive("hello", ""));
}

TEST(StringUtils, ToLower_AlreadyLower) {
    EXPECT_EQ(toLower("hello"), "hello");
}

TEST(StringUtils, ToLower_MixedCase) {
    EXPECT_EQ(toLower("HeLLo WOrLD"), "hello world");
}

TEST(StringUtils, ToLower_EmptyString) {
    EXPECT_EQ(toLower(""), "");
}

TEST(StringUtils, EqualsCaseInsensitive_Equal) {
    EXPECT_TRUE(equalsCaseInsensitive("ThemisDB", "themisdb"));
    EXPECT_TRUE(equalsCaseInsensitive("ABC", "abc"));
}

TEST(StringUtils, EqualsCaseInsensitive_NotEqual) {
    EXPECT_FALSE(equalsCaseInsensitive("abc", "xyz"));
    EXPECT_FALSE(equalsCaseInsensitive("abc", "abcd"));  // different lengths
}

TEST(StringUtils, EqualsCaseInsensitive_EmptyStrings) {
    EXPECT_TRUE(equalsCaseInsensitive("", ""));
}

// ═══════════════════════════════════════════════════════════
// safe_arithmetic tests
// ═══════════════════════════════════════════════════════════

TEST(SafeArithmetic, SafeAdd_PositiveOffset) {
    auto result = safe_add(static_cast<size_t>(10), 5);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 15u);
}

TEST(SafeArithmetic, SafeAdd_NegativeOffset) {
    auto result = safe_add(static_cast<size_t>(10), -4);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 6u);
}

TEST(SafeArithmetic, SafeAdd_Underflow) {
    auto result = safe_add(static_cast<size_t>(3), -10);
    EXPECT_FALSE(result.has_value());
}

TEST(SafeArithmetic, SafeAdd_ZeroOffset) {
    auto result = safe_add(static_cast<size_t>(7), 0);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 7u);
}

TEST(SafeArithmetic, SafeSub_Normal) {
    auto result = safe_sub(static_cast<size_t>(10), static_cast<size_t>(4));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 6u);
}

TEST(SafeArithmetic, SafeSub_ExactZero) {
    auto result = safe_sub(static_cast<size_t>(5), static_cast<size_t>(5));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0u);
}

TEST(SafeArithmetic, SafeSub_Underflow) {
    auto result = safe_sub(static_cast<size_t>(3), static_cast<size_t>(5));
    EXPECT_FALSE(result.has_value());
}

// ═══════════════════════════════════════════════════════════
// safe_cast tests
// ═══════════════════════════════════════════════════════════

TEST(SafeCast, FloatToUint32RoundTrip) {
    float f = 3.14f;
    uint32_t bits = safe_cast<uint32_t>(f);
    float back = safe_cast<float>(bits);
    EXPECT_FLOAT_EQ(back, f);
}

TEST(SafeCast, FloatBits_KnownValue) {
    // IEEE 754: 1.0f == 0x3F800000
    float one = 1.0f;
    uint32_t bits = FloatBits::to_u32(one);
    EXPECT_EQ(bits, 0x3F800000u);
    EXPECT_FLOAT_EQ(FloatBits::from_u32(bits), 1.0f);
}

TEST(SafeCast, DoubleBits_KnownValue) {
    // IEEE 754: 1.0 == 0x3FF0000000000000
    double one = 1.0;
    uint64_t bits = FloatBits::to_u64(one);
    EXPECT_EQ(bits, 0x3FF0000000000000ULL);
    EXPECT_DOUBLE_EQ(FloatBits::from_u64(bits), 1.0);
}

TEST(SafeCast, FloatBits_NaN) {
    float nan_val = std::numeric_limits<float>::quiet_NaN();
    uint32_t bits = FloatBits::to_u32(nan_val);
    float back = FloatBits::from_u32(bits);
    EXPECT_TRUE(std::isnan(back));
}

TEST(SafeCast, FloatBits_Infinity) {
    float inf = std::numeric_limits<float>::infinity();
    uint32_t bits = FloatBits::to_u32(inf);
    EXPECT_FLOAT_EQ(FloatBits::from_u32(bits), inf);
}

TEST(SafeCast, Int32ToUint32SameBitPattern) {
    int32_t neg = -1;
    uint32_t u = safe_cast<uint32_t>(neg);
    EXPECT_EQ(u, 0xFFFFFFFFu);
}

// ═══════════════════════════════════════════════════════════
// serialization tests (Serialization::Encoder / Decoder)
// ═══════════════════════════════════════════════════════════

TEST(Serialization, EncodeDecodeInt32) {
    Serialization::Encoder enc;
    enc.encodeInt32(42);
    auto bytes = enc.finish();

    Serialization::Decoder dec(bytes);
    EXPECT_EQ(dec.peekType(), Serialization::TypeTag::INT32);
    EXPECT_EQ(dec.decodeInt32(), 42);
}

TEST(Serialization, EncodeDecodeNegativeInt32) {
    Serialization::Encoder enc;
    enc.encodeInt32(-99);
    auto bytes = enc.finish();

    Serialization::Decoder dec(bytes);
    EXPECT_EQ(dec.decodeInt32(), -99);
}

TEST(Serialization, EncodeDecodeFloat) {
    Serialization::Encoder enc;
    enc.encodeFloat(3.14f);
    auto bytes = enc.finish();

    Serialization::Decoder dec(bytes);
    EXPECT_EQ(dec.peekType(), Serialization::TypeTag::FLOAT);
    EXPECT_FLOAT_EQ(dec.decodeFloat(), 3.14f);
}

TEST(Serialization, EncodeDecodeDouble) {
    Serialization::Encoder enc;
    enc.encodeDouble(2.71828182845);
    auto bytes = enc.finish();

    Serialization::Decoder dec(bytes);
    EXPECT_DOUBLE_EQ(dec.decodeDouble(), 2.71828182845);
}

TEST(Serialization, EncodeDecodeUInt64) {
    Serialization::Encoder enc;
    enc.encodeUInt64(0xDEADBEEFCAFEBABEULL);
    auto bytes = enc.finish();

    Serialization::Decoder dec(bytes);
    EXPECT_EQ(dec.decodeUInt64(), 0xDEADBEEFCAFEBABEULL);
}

TEST(Serialization, EncodeDecodeString) {
    Serialization::Encoder enc;
    enc.encodeString("ThemisDB");
    auto bytes = enc.finish();

    Serialization::Decoder dec(bytes);
    EXPECT_EQ(dec.decodeString(), "ThemisDB");
}

TEST(Serialization, EncodeDecodeBool) {
    {
        Serialization::Encoder enc;
        enc.encodeBool(true);
        auto bytes = enc.finish();
        Serialization::Decoder dec(bytes);
        EXPECT_TRUE(dec.decodeBool());
    }
    {
        Serialization::Encoder enc;
        enc.encodeBool(false);
        auto bytes = enc.finish();
        Serialization::Decoder dec(bytes);
        EXPECT_FALSE(dec.decodeBool());
    }
}

TEST(Serialization, EncodeDecodeFloatVector) {
    std::vector<float> vec = {1.0f, 2.0f, 3.0f, 4.0f};
    Serialization::Encoder enc;
    enc.encodeFloatVector(vec);
    auto bytes = enc.finish();

    Serialization::Decoder dec(bytes);
    auto restored = dec.decodeFloatVector();
    ASSERT_EQ(restored.size(), vec.size());
    for (size_t i = 0; i < vec.size(); ++i) {
        EXPECT_FLOAT_EQ(restored[i], vec[i]);
    }
}

}  // namespace test
}  // namespace utils
}  // namespace themis
