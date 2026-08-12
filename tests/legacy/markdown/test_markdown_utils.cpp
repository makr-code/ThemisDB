#include <gtest/gtest.h>
#include "prompt_engineering/markdown_utils.h"

using namespace themis::prompt_engineering;

// ─────────────────────────────────────────────────────────────────────────────
// stripMarkdownFences() tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(MarkdownUtilsTest, StripBasicFences) {
    std::string input = "```\nselect * from table\n```";
    std::string expected = "select * from table";
    EXPECT_EQ(stripMarkdownFences(input), expected);
}

TEST(MarkdownUtilsTest, StripFencesWithLanguageTag) {
    std::string input = "```sql\nselect * from table\n```";
    std::string expected = "select * from table";
    EXPECT_EQ(stripMarkdownFences(input), expected);
}

TEST(MarkdownUtilsTest, StripFencesWithJsonTag) {
    std::string input = "```json\n{\"key\": \"value\"}\n```";
    std::string expected = "{\"key\": \"value\"}";
    EXPECT_EQ(stripMarkdownFences(input), expected);
}

TEST(MarkdownUtilsTest, StripFencesWithCRLF) {
    std::string input = "```\r\nselect * from table\r\n```";
    std::string expected = "select * from table";
    EXPECT_EQ(stripMarkdownFences(input), expected);
}

TEST(MarkdownUtilsTest, StripFencesWithLanguageTagCRLF) {
    std::string input = "```json\r\n{\"key\": \"value\"}\r\n```";
    std::string expected = "{\"key\": \"value\"}";
    EXPECT_EQ(stripMarkdownFences(input), expected);
}

TEST(MarkdownUtilsTest, StripFencesWithExtraWhitespace) {
    std::string input = "```\n  select * from table  \n```";
    std::string expected = "select * from table";
    EXPECT_EQ(stripMarkdownFences(input), expected);
}

TEST(MarkdownUtilsTest, StripFencesNoFences) {
    std::string input = "select * from table";
    EXPECT_EQ(stripMarkdownFences(input), input);
}

TEST(MarkdownUtilsTest, StripFencesEmpty) {
    std::string input = "";
    EXPECT_EQ(stripMarkdownFences(input), "");
}

TEST(MarkdownUtilsTest, StripFencesOnlyFences) {
    std::string input = "```\n```";
    EXPECT_EQ(stripMarkdownFences(input), "");
}

// ─────────────────────────────────────────────────────────────────────────────
// stripMarkdownFences() with language tag output parameter
// ─────────────────────────────────────────────────────────────────────────────

TEST(MarkdownUtilsTest, StripFencesExtractLanguageTag) {
    std::string input = "```sql\nselect * from table\n```";
    std::string language_tag;
    std::string result = stripMarkdownFences(input, &language_tag);
    EXPECT_EQ(result, "select * from table");
    EXPECT_EQ(language_tag, "sql");
}

TEST(MarkdownUtilsTest, StripFencesExtractJsonTag) {
    std::string input = "```json\n{\"key\": \"value\"}\n```";
    std::string language_tag;
    std::string result = stripMarkdownFences(input, &language_tag);
    EXPECT_EQ(result, "{\"key\": \"value\"}");
    EXPECT_EQ(language_tag, "json");
}

TEST(MarkdownUtilsTest, StripFencesNoLanguageTag) {
    std::string input = "```\nselect * from table\n```";
    std::string language_tag;
    std::string result = stripMarkdownFences(input, &language_tag);
    EXPECT_EQ(result, "select * from table");
    EXPECT_EQ(language_tag, "");
}

// ─────────────────────────────────────────────────────────────────────────────
// stripMarkdownAndComments() tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(MarkdownUtilsTest, StripFencesAndComments) {
    std::string input = "```\nselect * from table // comment\n```";
    std::string expected = "select * from table";
    EXPECT_EQ(stripMarkdownAndComments(input), expected);
}

TEST(MarkdownUtilsTest, StripFencesAndMultipleComments) {
    std::string input = "```sql\n// first line\nselect * from table // inline comment\n// last line\n```";
    std::string expected = "select * from table";
    EXPECT_EQ(stripMarkdownAndComments(input), expected);
}

TEST(MarkdownUtilsTest, StripCommentsNoFences) {
    std::string input = "select * from table // comment";
    std::string expected = "select * from table";
    EXPECT_EQ(stripMarkdownAndComments(input), expected);
}

// ─────────────────────────────────────────────────────────────────────────────
// isWrappedInMarkdownFences() tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(MarkdownUtilsTest, IsWrappedTrue) {
    std::string input = "```\ncontent\n```";
    EXPECT_TRUE(isWrappedInMarkdownFences(input));
}

TEST(MarkdownUtilsTest, IsWrappedWithLanguageTag) {
    std::string input = "```json\ncontent\n```";
    EXPECT_TRUE(isWrappedInMarkdownFences(input));
}

TEST(MarkdownUtilsTest, IsWrappedWithCRLF) {
    std::string input = "```\r\ncontent\r\n```";
    EXPECT_TRUE(isWrappedInMarkdownFences(input));
}

TEST(MarkdownUtilsTest, IsNotWrapped) {
    std::string input = "select * from table";
    EXPECT_FALSE(isWrappedInMarkdownFences(input));
}

TEST(MarkdownUtilsTest, IsNotWrappedPartial) {
    std::string input = "```\ncontent";
    EXPECT_FALSE(isWrappedInMarkdownFences(input));
}

TEST(MarkdownUtilsTest, IsNotWrappedEmpty) {
    std::string input = "";
    EXPECT_FALSE(isWrappedInMarkdownFences(input));
}

// ─────────────────────────────────────────────────────────────────────────────
// Edge cases and complex scenarios
// ─────────────────────────────────────────────────────────────────────────────

TEST(MarkdownUtilsTest, NestedBackticks) {
    std::string input = "```\nselect `column` from table\n```";
    std::string expected = "select `column` from table";
    EXPECT_EQ(stripMarkdownFences(input), expected);
}

TEST(MarkdownUtilsTest, MultilineContent) {
    std::string input = "```\nline1\nline2\nline3\n```";
    std::string expected = "line1\nline2\nline3";
    EXPECT_EQ(stripMarkdownFences(input), expected);
}

TEST(MarkdownUtilsTest, ContentWithLeadingTrailingNewlines) {
    std::string input = "```\n\n\ncontent\n\n\n```";
    std::string expected = "content";
    EXPECT_EQ(stripMarkdownFences(input), expected);
}

TEST(MarkdownUtilsTest, JsonWithComplexStructure) {
    std::string input = R"(```json
{
  "key1": "value1",
  "key2": {
    "nested": true
  }
}
```)";
    std::string result = stripMarkdownFences(input);
    EXPECT_TRUE(result.find("\"nested\": true") != std::string::npos);
}

// ─────────────────────────────────────────────────────────────────────────────
// Performance checks
// ─────────────────────────────────────────────────────────────────────────────

TEST(MarkdownUtilsTest, LargeContent) {
    std::string large(10000, 'x');
    std::string input = "```\n" + large + "\n```";
    std::string result = stripMarkdownFences(input);
    EXPECT_EQ(result.size(), 10000);
    EXPECT_EQ(result, large);
}
