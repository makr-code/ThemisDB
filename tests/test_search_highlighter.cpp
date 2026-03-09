/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_search_highlighter.cpp                        ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-09                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_search_highlighter.cpp
 * @brief Unit tests for SearchHighlighter (v2.1.0, Issue #2457)
 *
 * Coverage:
 *  - Config validation (window_size == 0, max_window_size clamping)
 *  - setConfig() validation
 *  - tokenize() static helper
 *  - highlight() — basic, case-insensitive, query-string overload,
 *    vector overload, empty text, empty terms, no-match
 *  - applyHighlight() static helper with custom tags
 *  - snippet() — text fits window, text truncated (left, right, both),
 *    no-match fallback (head returned), custom window_size override,
 *    custom separator, query-string overload, vector overload
 *  - bestWindowOffset() static helper
 */

#include <gtest/gtest.h>
#include "search/search_highlighter.h"
#include <string>
#include <unordered_set>
#include <vector>

using namespace themis;

// ============================================================================
// Config validation
// ============================================================================

TEST(SearchHighlighterConfig, DefaultConstructionIsValid) {
    EXPECT_NO_THROW(SearchHighlighter{});
}

TEST(SearchHighlighterConfig, ZeroWindowSizeThrows) {
    SearchHighlighter::Config cfg;
    cfg.window_size = 0;
    EXPECT_THROW(SearchHighlighter{cfg}, std::invalid_argument);
}

TEST(SearchHighlighterConfig, ZeroMaxWindowSizeThrows) {
    SearchHighlighter::Config cfg;
    cfg.max_window_size = 0;
    EXPECT_THROW(SearchHighlighter{cfg}, std::invalid_argument);
}

TEST(SearchHighlighterConfig, WindowSizeClampedToMaxWindowSize) {
    SearchHighlighter::Config cfg;
    cfg.window_size     = 500;
    cfg.max_window_size = 100;
    SearchHighlighter h{cfg};
    EXPECT_EQ(h.getConfig().window_size, 100u);
}

TEST(SearchHighlighterConfig, ConfigRoundtrip) {
    SearchHighlighter::Config cfg;
    cfg.open_tag    = "<mark>";
    cfg.close_tag   = "</mark>";
    cfg.separator   = " … ";
    cfg.window_size = 150;
    SearchHighlighter h{cfg};
    EXPECT_EQ(h.getConfig().open_tag,    "<mark>");
    EXPECT_EQ(h.getConfig().close_tag,   "</mark>");
    EXPECT_EQ(h.getConfig().separator,   " … ");
    EXPECT_EQ(h.getConfig().window_size, 150u);
}

TEST(SearchHighlighterConfig, SetConfigZeroWindowSizeThrows) {
    SearchHighlighter h;
    SearchHighlighter::Config cfg;
    cfg.window_size = 0;
    EXPECT_THROW(h.setConfig(cfg), std::invalid_argument);
}

TEST(SearchHighlighterConfig, SetConfigValid) {
    SearchHighlighter h;
    SearchHighlighter::Config cfg;
    cfg.open_tag    = "<b>";
    cfg.close_tag   = "</b>";
    cfg.window_size = 80;
    EXPECT_NO_THROW(h.setConfig(cfg));
    EXPECT_EQ(h.getConfig().open_tag, "<b>");
}

// ============================================================================
// tokenize() static helper
// ============================================================================

TEST(SearchHighlighterTokenize, BasicSplit) {
    auto terms = SearchHighlighter::tokenize("machine learning");
    EXPECT_EQ(terms.count("machine"), 1u);
    EXPECT_EQ(terms.count("learning"), 1u);
    EXPECT_EQ(terms.size(), 2u);
}

TEST(SearchHighlighterTokenize, LowerCases) {
    auto terms = SearchHighlighter::tokenize("Machine LEARNING");
    EXPECT_EQ(terms.count("machine"), 1u);
    EXPECT_EQ(terms.count("learning"), 1u);
}

TEST(SearchHighlighterTokenize, Deduplicates) {
    auto terms = SearchHighlighter::tokenize("cat cat cat");
    EXPECT_EQ(terms.size(), 1u);
    EXPECT_EQ(terms.count("cat"), 1u);
}

TEST(SearchHighlighterTokenize, EmptyQueryReturnsEmpty) {
    auto terms = SearchHighlighter::tokenize("");
    EXPECT_TRUE(terms.empty());
}

TEST(SearchHighlighterTokenize, PunctuationSeparators) {
    auto terms = SearchHighlighter::tokenize("hello,world!foo");
    EXPECT_EQ(terms.count("hello"), 1u);
    EXPECT_EQ(terms.count("world"), 1u);
    EXPECT_EQ(terms.count("foo"),   1u);
}

// ============================================================================
// applyHighlight() static helper
// ============================================================================

TEST(SearchHighlighterApply, BasicMatch) {
    std::unordered_set<std::string> terms = {"machine"};
    auto result = SearchHighlighter::applyHighlight(
        "Machine learning is great", terms, "<em>", "</em>");
    EXPECT_EQ(result, "<em>Machine</em> learning is great");
}

TEST(SearchHighlighterApply, MultipleTerms) {
    std::unordered_set<std::string> terms = {"machine", "learning"};
    auto result = SearchHighlighter::applyHighlight(
        "Machine Learning is great", terms, "<em>", "</em>");
    EXPECT_EQ(result, "<em>Machine</em> <em>Learning</em> is great");
}

TEST(SearchHighlighterApply, CaseInsensitive) {
    std::unordered_set<std::string> terms = {"database"};
    auto result = SearchHighlighter::applyHighlight(
        "DATABASE performance", terms, "<em>", "</em>");
    EXPECT_EQ(result, "<em>DATABASE</em> performance");
}

TEST(SearchHighlighterApply, CustomTags) {
    std::unordered_set<std::string> terms = {"hello"};
    auto result = SearchHighlighter::applyHighlight(
        "Say hello world", terms, "<b>", "</b>");
    EXPECT_EQ(result, "Say <b>hello</b> world");
}

TEST(SearchHighlighterApply, EmptyTextReturnsEmpty) {
    std::unordered_set<std::string> terms = {"foo"};
    EXPECT_EQ(SearchHighlighter::applyHighlight("", terms, "<em>", "</em>"), "");
}

TEST(SearchHighlighterApply, EmptyTermsReturnsOriginal) {
    std::unordered_set<std::string> terms;
    EXPECT_EQ(SearchHighlighter::applyHighlight("hello world", terms, "<em>", "</em>"),
              "hello world");
}

TEST(SearchHighlighterApply, NoMatchReturnsOriginal) {
    std::unordered_set<std::string> terms = {"foo"};
    auto result = SearchHighlighter::applyHighlight(
        "hello world", terms, "<em>", "</em>");
    EXPECT_EQ(result, "hello world");
}

// ============================================================================
// highlight() — instance methods
// ============================================================================

TEST(SearchHighlighterHighlight, SetOverload) {
    SearchHighlighter h;
    std::unordered_set<std::string> terms = {"neural"};
    auto result = h.highlight("Neural networks are powerful", terms);
    EXPECT_NE(result.find("<em>Neural</em>"), std::string::npos);
}

TEST(SearchHighlighterHighlight, StringOverload) {
    SearchHighlighter h;
    auto result = h.highlight("Machine Learning is great", "machine learning");
    EXPECT_NE(result.find("<em>Machine</em>"),   std::string::npos);
    EXPECT_NE(result.find("<em>Learning</em>"),  std::string::npos);
}

TEST(SearchHighlighterHighlight, VectorOverload) {
    SearchHighlighter h;
    auto result = h.highlight("Database tuning tips",
                               std::vector<std::string>{"Database", "tuning"});
    EXPECT_NE(result.find("<em>Database</em>"), std::string::npos);
    EXPECT_NE(result.find("<em>tuning</em>"),   std::string::npos);
}

TEST(SearchHighlighterHighlight, UsesConfigTags) {
    SearchHighlighter::Config cfg;
    cfg.open_tag  = "<mark>";
    cfg.close_tag = "</mark>";
    SearchHighlighter h{cfg};
    auto result = h.highlight("hello world", "hello");
    EXPECT_NE(result.find("<mark>hello</mark>"), std::string::npos);
    EXPECT_EQ(result.find("<em>"),               std::string::npos);
}

// ============================================================================
// snippet() — text fits window
// ============================================================================

TEST(SearchHighlighterSnippet, ShortTextReturnsFull) {
    SearchHighlighter h;
    auto result = h.snippet("machine learning is great", "machine");
    // Text fits in the default 200-char window — no separators
    EXPECT_NE(result.find("<em>machine</em>"), std::string::npos);
    EXPECT_EQ(result.find("..."), std::string::npos);
}

TEST(SearchHighlighterSnippet, ShortTextPreservesWholeContent) {
    SearchHighlighter h;
    const std::string text = "short text";
    auto result = h.snippet(text, "short");
    EXPECT_NE(result.find("<em>short</em>"), std::string::npos);
    EXPECT_NE(result.find("text"),           std::string::npos);
}

// ============================================================================
// snippet() — text requires truncation
// ============================================================================

TEST(SearchHighlighterSnippet, LongTextTruncated) {
    SearchHighlighter h;
    // Build a document with the keyword buried in the middle
    std::string longText =
        std::string(300, 'a') + " target keyword " + std::string(300, 'z');
    auto result = h.snippet(longText, "target", 60);
    // Result must be short
    EXPECT_LT(result.size(), 200u);
    // Separator must appear (truncated on at least one side)
    EXPECT_NE(result.find("..."), std::string::npos);
    // Keyword must be highlighted
    EXPECT_NE(result.find("<em>target</em>"), std::string::npos);
}

TEST(SearchHighlighterSnippet, TruncationFromLeft) {
    SearchHighlighter h;
    // Keyword is near the end → window starts after beginning → left separator
    std::string text = std::string(300, 'x') + " keyword";
    auto result = h.snippet(text, "keyword", 30);
    EXPECT_NE(result.find("..."), std::string::npos);
    EXPECT_NE(result.find("<em>keyword</em>"), std::string::npos);
}

TEST(SearchHighlighterSnippet, TruncationFromRight) {
    SearchHighlighter h;
    // Keyword is near the beginning → window starts at 0 → right separator only
    std::string text = "keyword " + std::string(300, 'z');
    auto result = h.snippet(text, "keyword", 30);
    // window starts at 0 → no left separator
    EXPECT_EQ(result.substr(0, 3), "<em");  // starts with tag, not separator
    // right truncation
    EXPECT_NE(result.find("..."), std::string::npos);
}

// ============================================================================
// snippet() — no match falls back to head of text
// ============================================================================

TEST(SearchHighlighterSnippet, NoMatchReturnsHead) {
    SearchHighlighter h;
    const std::string text = std::string(300, 'q');
    auto result = h.snippet(text, "notfound", 50);
    // No separator on the left (starts at offset 0)
    // Right separator because text is longer than 50
    EXPECT_NE(result.find("..."), std::string::npos);
    // No highlighting
    EXPECT_EQ(result.find("<em>"), std::string::npos);
}

// ============================================================================
// snippet() — custom separator
// ============================================================================

TEST(SearchHighlighterSnippet, CustomSeparator) {
    SearchHighlighter::Config cfg;
    cfg.separator   = " … ";
    cfg.window_size = 40;
    SearchHighlighter h{cfg};
    std::string text =
        std::string(200, 'x') + " keyword " + std::string(200, 'y');
    auto result = h.snippet(text, "keyword");
    EXPECT_NE(result.find(" … "), std::string::npos);
    EXPECT_EQ(result.find("..."), std::string::npos);  // default not used
    EXPECT_NE(result.find("<em>keyword</em>"), std::string::npos);
}

// ============================================================================
// snippet() — convenience overloads
// ============================================================================

TEST(SearchHighlighterSnippet, StringOverload) {
    SearchHighlighter h;
    auto result = h.snippet("Neural networks are powerful", "neural");
    EXPECT_NE(result.find("<em>Neural</em>"), std::string::npos);
}

TEST(SearchHighlighterSnippet, VectorOverload) {
    SearchHighlighter h;
    auto result = h.snippet("Database performance tips",
                             std::vector<std::string>{"performance"});
    EXPECT_NE(result.find("<em>performance</em>"), std::string::npos);
}

TEST(SearchHighlighterSnippet, EmptyTextReturnsEmpty) {
    SearchHighlighter h;
    EXPECT_EQ(h.snippet("", "query"), "");
}

// ============================================================================
// bestWindowOffset() static helper
// ============================================================================

TEST(SearchHighlighterBestWindow, OffsetZeroWhenTextFitsWindow) {
    std::unordered_set<std::string> terms = {"foo"};
    std::string lower = "foo bar";
    EXPECT_EQ(SearchHighlighter::bestWindowOffset(lower, terms, 100u), 0u);
}

TEST(SearchHighlighterBestWindow, OffsetZeroWhenNoTermFound) {
    std::unordered_set<std::string> terms = {"xyz"};
    std::string lower(300, 'a');
    EXPECT_EQ(SearchHighlighter::bestWindowOffset(lower, terms, 50u), 0u);
}

TEST(SearchHighlighterBestWindow, FindsTermInMiddle) {
    std::unordered_set<std::string> terms = {"key"};
    const std::string lower =
        std::string(100, 'a') + " key " + std::string(100, 'z');
    size_t off = SearchHighlighter::bestWindowOffset(lower, terms, 20u);
    // The offset should position the keyword within the window
    EXPECT_LT(off, lower.size());
    // The window at 'off' must contain the keyword
    std::string window = lower.substr(off, 20u);
    EXPECT_NE(window.find("key"), std::string::npos);
}
