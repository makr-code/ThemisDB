#include <gtest/gtest.h>
#include "utils/string_utils.h"

using namespace themis::utils;

// ─────────────────────────────────────────────────────────────────────────────
// trim() tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(StringUtilsTest, TrimLeadingSpaces) {
    EXPECT_EQ(trim("  hello world"), "hello world");
}

TEST(StringUtilsTest, TrimTrailingSpaces) {
    EXPECT_EQ(trim("hello world  "), "hello world");
}

TEST(StringUtilsTest, TrimBothSides) {
    EXPECT_EQ(trim("  hello world  "), "hello world");
}

TEST(StringUtilsTest, TrimWithTabs) {
    EXPECT_EQ(trim("\t\thello world\t\t"), "hello world");
}

TEST(StringUtilsTest, TrimWithNewlines) {
    EXPECT_EQ(trim("\n\nhello world\n\n"), "hello world");
}

TEST(StringUtilsTest, TrimWithCarriageReturns) {
    EXPECT_EQ(trim("\r\nhello world\r\n"), "hello world");
}

TEST(StringUtilsTest, TrimAllWhitespace) {
    EXPECT_EQ(trim(" \t\r\n"), "");
}

TEST(StringUtilsTest, TrimEmpty) {
    EXPECT_EQ(trim(""), "");
}

TEST(StringUtilsTest, TrimNoWhitespace) {
    EXPECT_EQ(trim("hello"), "hello");
}

TEST(StringUtilsTest, TrimWithCustomCharset) {
    EXPECT_EQ(trim("xxxhello worldxxx", "x"), "hello world");
}

// ─────────────────────────────────────────────────────────────────────────────
// ltrim() tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(StringUtilsTest, LTrimLeading) {
    EXPECT_EQ(ltrim("  hello world"), "hello world");
}

TEST(StringUtilsTest, LTrimNoTrailing) {
    EXPECT_EQ(ltrim("hello world  "), "hello world  ");
}

TEST(StringUtilsTest, LTrimEmpty) {
    EXPECT_EQ(ltrim(""), "");
}

// ─────────────────────────────────────────────────────────────────────────────
// rtrim() tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(StringUtilsTest, RTrimTrailing) {
    EXPECT_EQ(rtrim("hello world  "), "hello world");
}

TEST(StringUtilsTest, RTrimNoLeading) {
    EXPECT_EQ(rtrim("  hello world"), "  hello world");
}

TEST(StringUtilsTest, RTrimEmpty) {
    EXPECT_EQ(rtrim(""), "");
}

// ─────────────────────────────────────────────────────────────────────────────
// trim_view() tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(StringUtilsTest, TrimViewLeading) {
    std::string_view input = "  hello world";
    auto result = trim_view(input);
    EXPECT_EQ(result, "hello world");
}

TEST(StringUtilsTest, TrimViewTrailing) {
    std::string_view input = "hello world  ";
    auto result = trim_view(input);
    EXPECT_EQ(result, "hello world");
}

TEST(StringUtilsTest, TrimViewBothSides) {
    std::string_view input = "  hello world  ";
    auto result = trim_view(input);
    EXPECT_EQ(result, "hello world");
}

TEST(StringUtilsTest, TrimViewAllWhitespace) {
    std::string_view input = " \t\r\n";
    auto result = trim_view(input);
    EXPECT_EQ(result, "");
}

TEST(StringUtilsTest, TrimViewEmpty) {
    std::string_view input = "";
    auto result = trim_view(input);
    EXPECT_EQ(result, "");
}

TEST(StringUtilsTest, TrimViewNoAllocation) {
    // Verify that trim_view doesn't allocate memory
    std::string storage = "  hello world  ";
    std::string_view input(storage);
    auto result = trim_view(input);
    
    // The result should be a view into the original storage
    EXPECT_EQ(result.data(), storage.data() + 2);
    EXPECT_EQ(result.size(), 11);
}

// ─────────────────────────────────────────────────────────────────────────────
// Edge cases
// ─────────────────────────────────────────────────────────────────────────────

TEST(StringUtilsTest, TrimSingleCharacter) {
    EXPECT_EQ(trim(" a "), "a");
}

TEST(StringUtilsTest, TrimMixedWhitespace) {
    EXPECT_EQ(trim(" \t \r\n hello \n\r \t "), "hello");
}

TEST(StringUtilsTest, TrimInternalSpacesPreserved) {
    EXPECT_EQ(trim("  hello   world  "), "hello   world");
}

TEST(StringUtilsTest, TrimUnicodeSpaceNotTrimmed) {
    // Non-breaking space (U+00A0) should not be trimmed by default
    EXPECT_NE(trim("\xC2\xA0hello\xC2\xA0"), "hello");
}

// ─────────────────────────────────────────────────────────────────────────────
// Performance checks (simple)
// ─────────────────────────────────────────────────────────────────────────────

TEST(StringUtilsTest, TrimLargeString) {
    std::string large(10000, ' ');
    large.insert(5000, "content");
    std::string result = trim(large);
    EXPECT_EQ(result, "content");
}
