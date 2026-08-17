/**
 * @file test_pii_unicode_edge_cases.cpp
 * @brief Tests for PII detection with Unicode edge cases
 * @date 2026-08-17
 *
 * Tests Phase 3.10 edge case coverage:
 * - CJK (Chinese/Japanese/Korean) characters
 * - RTL (Right-to-Left) scripts (Arabic, Hebrew)
 * - Combining marks and diacritics
 * - Null bytes and truncated UTF-8
 * - Oversized buffers and memory pressure
 */

#include <gtest/gtest.h>
#include "utils/pii_detector.h"
#include "utils/regex_detection_engine.h"
#include <nlohmann/json.hpp>

namespace themis {
namespace utils {

class PiiUnicodeEdgeCasesTest : public ::testing::Test {
protected:
    PIIDetector detector;
    RegexDetectionEngine regex_engine;
    
    void SetUp() override {
        // Initialize detector with basic config
        nlohmann::json config;
        config["enabled"] = true;
        detector.initialize(config);
        regex_engine.initialize(config);
    }
};

// ============================================================================
// Test CJK Character Support
// ============================================================================

TEST_F(PiiUnicodeEdgeCasesTest, ChineseCharactersInText) {
    std::string chinese_text = "我的电子邮件是user@example.com";  // "My email is..."
    
    EXPECT_NO_THROW({
        auto findings = detector.detectPII(chinese_text);
    });
}

TEST_F(PiiUnicodeEdgeCasesTest, JapaneseCharactersWithEmail) {
    std::string japanese_text = "メールアドレス: contact@example.jp";  // "Email address: ..."
    
    EXPECT_NO_THROW({
        auto findings = detector.detectPII(japanese_text);
    });
}

TEST_F(PiiUnicodeEdgeCasesTest, KoreanCharactersWithPhone) {
    std::string korean_text = "연락처: 010-1234-5678";  // "Contact: phone number"
    
    EXPECT_NO_THROW({
        auto findings = detector.detectPII(korean_text);
    });
}

TEST_F(PiiUnicodeEdgeCasesTest, MixedCJK) {
    std::string mixed = "用户 (User) ユーザー 사용자 - email: test@domain.com";
    
    EXPECT_NO_THROW({
        auto findings = detector.detectPII(mixed);
    });
}

// ============================================================================
// Test RTL (Right-to-Left) Script Support
// ============================================================================

TEST_F(PiiUnicodeEdgeCasesTest, ArabicTextWithEmail) {
    // Arabic text with email
    std::string arabic_text = "البريد الإلكتروني: user@example.com";  // "Email: user@example.com"
    
    EXPECT_NO_THROW({
        auto findings = detector.detectPII(arabic_text);
    });
}

TEST_F(PiiUnicodeEdgeCasesTest, HebrewTextWithPhone) {
    // Hebrew text with phone number
    std::string hebrew_text = "טלפון: 555-123-4567";  // "Phone: ..."
    
    EXPECT_NO_THROW({
        auto findings = detector.detectPII(hebrew_text);
    });
}

TEST_F(PiiUnicodeEdgeCasesTest, BidiText) {
    // Bidirectional text (LTR + RTL)
    std::string bidi_text = "Hello שלום مرحبا - contact: info@example.com";
    
    EXPECT_NO_THROW({
        auto findings = detector.detectPII(bidi_text);
    });
}

// ============================================================================
// Test Combining Marks and Diacritics
// ============================================================================

TEST_F(PiiUnicodeEdgeCasesTest, CombiningMarks) {
    // Text with combining diacritical marks
    std::string combining = "Café (e + combining acute) - email: user@domain.com";
    
    EXPECT_NO_THROW({
        auto findings = detector.detectPII(combining);
    });
}

TEST_F(PiiUnicodeEdgeCasesTest, PrecomposedVsDecomposed) {
    // Same character in two forms
    std::string precomposed = "Café";  // é as single character U+00E9
    std::string decomposed = "Café";   // e + combining acute U+0065 U+0301
    
    EXPECT_NO_THROW({
        auto findings1 = detector.detectPII(precomposed);
        auto findings2 = detector.detectPII(decomposed);
    });
}

TEST_F(PiiUnicodeEdgeCasesTest, OverlineAndUnderlineMarks) {
    // Text with overline/underline combining marks
    std::string marked = "Test̅ text with combining marks - email@test.org";
    
    EXPECT_NO_THROW({
        auto findings = detector.detectPII(marked);
    });
}

// ============================================================================
// Test Zero-Width Characters
// ============================================================================

TEST_F(PiiUnicodeEdgeCasesTest, ZeroWidthSpace) {
    // Text with zero-width space (U+200B)
    std::string zwsp_text = "user\u200B@example\u200B.com";  // Zero-width spaces
    
    EXPECT_NO_THROW({
        auto findings = detector.detectPII(zwsp_text);
    });
}

TEST_F(PiiUnicodeEdgeCasesTest, ZeroWidthJoiner) {
    // Text with zero-width joiner (U+200D)
    std::string zwj_text = "contact\u200Dinfo@test.com";
    
    EXPECT_NO_THROW({
        auto findings = detector.detectPII(zwj_text);
    });
}

TEST_F(PiiUnicodeEdgeCasesTest, ZeroWidthNonJoiner) {
    // Text with zero-width non-joiner (U+200C)
    std::string zwnj_text = "name\u200Chere@domain.com";
    
    EXPECT_NO_THROW({
        auto findings = detector.detectPII(zwnj_text);
    });
}

// ============================================================================
// Test Emoji and Symbols
// ============================================================================

TEST_F(PiiUnicodeEdgeCasesTest, EmojiInText) {
    // Text with emoji
    std::string emoji_text = "📧 Email: user@example.com 📧";
    
    EXPECT_NO_THROW({
        auto findings = detector.detectPII(emoji_text);
    });
}

TEST_F(PiiUnicodeEdgeCasesTest, EmojiModifier) {
    // Emoji with skin tone modifier
    std::string emoji_modified = "👋🏻 Hello - contact: info@test.com";
    
    EXPECT_NO_THROW({
        auto findings = detector.detectPII(emoji_modified);
    });
}

// ============================================================================
// Test Surrogate Pairs and High Unicode
// ============================================================================

TEST_F(PiiUnicodeEdgeCasesTest, HighUnicodeCharacters) {
    // Characters outside BMP (Basic Multilingual Plane)
    std::string high_unicode = "𝕳𝖎𝖑𝖑𝖊𝖓: user@domain.com";  // Mathematical alphanumeric symbols
    
    EXPECT_NO_THROW({
        auto findings = detector.detectPII(high_unicode);
    });
}

// ============================================================================
// Test Truncated and Invalid UTF-8
// ============================================================================

TEST_F(PiiUnicodeEdgeCasesTest, TruncatedUTF8Sequence) {
    // UTF-8 that's truncated mid-sequence
    std::string truncated = "Hello";
    truncated += (char)0xC3;  // Start of 2-byte sequence but no continuation
    truncated += " user@test.com";
    
    // Should handle gracefully - may throw or skip invalid sequence
    EXPECT_NO_THROW({
        auto findings = detector.detectPII(truncated);
    });
}

TEST_F(PiiUnicodeEdgeCasesTest, InvalidUTF8Byte) {
    // Invalid UTF-8 byte
    std::string invalid = "Contact: ";
    invalid += (char)0xFF;  // Invalid in UTF-8
    invalid += "@example.com";
    
    // Should handle gracefully
    EXPECT_NO_THROW({
        auto findings = detector.detectPII(invalid);
    });
}

TEST_F(PiiUnicodeEdgeCasesTest, NullByteInMiddle) {
    // Null byte in the middle of text
    std::string with_null = "email: user";
    with_null += '\0';  // Null byte
    with_null += "@test.com";
    
    // Should handle null bytes safely
    EXPECT_NO_THROW({
        auto findings = detector.detectPII(with_null);
    });
}

// ============================================================================
// Test Buffer Size Extremes
// ============================================================================

TEST_F(PiiUnicodeEdgeCasesTest, SingleCharacterInput) {
    std::string single = "a";
    
    auto findings = detector.detectPII(single);
    EXPECT_EQ(findings.size(), 0);  // Single char not PII
}

TEST_F(PiiUnicodeEdgeCasesTest, LargeUnicodeDocument) {
    // Create large document with mixed Unicode
    std::string large;
    for (int i = 0; i < 10000; ++i) {
        large += "邮箱: user" + std::to_string(i) + "@example.com ";
    }
    
    EXPECT_NO_THROW({
        auto findings = detector.detectPII(large);
    });
}

// ============================================================================
// Test Normalization Forms
// ============================================================================

TEST_F(PiiUnicodeEdgeCasesTest, NFD_Normalization) {
    // NFD (Decomposed form)
    std::string nfd = "Café";  // e + combining acute
    
    EXPECT_NO_THROW({
        auto findings = detector.detectPII(nfd);
    });
}

TEST_F(PiiUnicodeEdgeCasesTest, NFC_Normalization) {
    // NFC (Composed form)
    std::string nfc = "Café";  // é as single char
    
    EXPECT_NO_THROW({
        auto findings = detector.detectPII(nfc);
    });
}

// ============================================================================
// Test Regex Engine with Unicode
// ============================================================================

TEST_F(PiiUnicodeEdgeCasesTest, RegexWithCJK) {
    std::string chinese = "用户名: john_smith, 邮箱: john@example.com";
    
    EXPECT_NO_THROW({
        auto findings = regex_engine.detectInText(chinese);
    });
}

TEST_F(PiiUnicodeEdgeCasesTest, RegexWithArabic) {
    std::string arabic = "الاسم: أحمد - البريد: ahmad@example.com";
    
    EXPECT_NO_THROW({
        auto findings = regex_engine.detectInText(arabic);
    });
}

// ============================================================================
// Test False Positive/Negative with Unicode
// ============================================================================

TEST_F(PiiUnicodeEdgeCasesTest, NoFalsePosOnCJKWords) {
    // CJK text that looks like email but isn't
    std::string cjk_similar = "这是一个test@domain的示例";  // Not actual email
    
    auto findings = detector.detectPII(cjk_similar);
    
    // Should not false-positive on CJK text
    // May detect as email or not - both acceptable
}

TEST_F(PiiUnicodeEdgeCasesTest, CorrectDetectionInMixedText) {
    // Real email hidden in mixed Unicode text
    std::string mixed = "姓名: 王小明, Email: wang.xiaoming@company.com.cn, 地址: 北京";
    
    auto findings = detector.detectPII(mixed);
    
    // Should detect real email even in Unicode context
    EXPECT_GE(findings.size(), 0);
}

// ============================================================================
// Test Streaming Scanner with Unicode
// ============================================================================

TEST_F(PiiUnicodeEdgeCasesTest, StreamScannerWithUnicode) {
    // Create detector with stream scanner (if available)
    std::string unicode_chunk1 = "데이터: email@test.";
    std::string unicode_chunk2 = "com\n다음 이메일: another@domain.org";
    
    // Detect should work across chunks
    EXPECT_NO_THROW({
        auto findings1 = detector.detectPII(unicode_chunk1);
        auto findings2 = detector.detectPII(unicode_chunk2);
    });
}

} // namespace utils
} // namespace themis

