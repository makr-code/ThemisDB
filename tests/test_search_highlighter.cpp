#include <gtest/gtest.h>
#include "search/search_highlighter.h"

namespace themis {
namespace {

// ─────────────────────────────────────────────────────────────────────────────
// tokenize()
// ─────────────────────────────────────────────────────────────────────────────

TEST(SearchHighlighterTokenize, EmptyStringReturnsEmptyVector) {
    auto toks = SearchHighlighter::tokenize("");
    EXPECT_TRUE(toks.empty());
}

TEST(SearchHighlighterTokenize, SingleWordReturnsOneToken) {
    auto toks = SearchHighlighter::tokenize("hello");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0], "hello");
}

TEST(SearchHighlighterTokenize, CaseInsensitiveLowercases) {
    auto toks = SearchHighlighter::tokenize("Hello World", true);
    ASSERT_EQ(toks.size(), 2u);
    EXPECT_EQ(toks[0], "hello");
    EXPECT_EQ(toks[1], "world");
}

TEST(SearchHighlighterTokenize, PunctuationActsAsSeparator) {
    auto toks = SearchHighlighter::tokenize("one,two;three");
    EXPECT_EQ(toks.size(), 3u);
}

// ─────────────────────────────────────────────────────────────────────────────
// applyHighlight()
// ─────────────────────────────────────────────────────────────────────────────

TEST(SearchHighlighterApply, EmptyOffsets_ReturnOriginal) {
    std::string text = "no match here";
    auto result = SearchHighlighter::applyHighlight(text, {}, "<mark>", "</mark>");
    EXPECT_EQ(result, text);
}

TEST(SearchHighlighterApply, SingleMatch_WrapsCorrectly) {
    std::string text = "hello world";
    // Match "world" at [6, 11)
    auto result = SearchHighlighter::applyHighlight(text, {{6, 11}}, "<mark>", "</mark>");
    EXPECT_EQ(result, "hello <mark>world</mark>");
}

TEST(SearchHighlighterApply, MultipleMatches) {
    std::string text = "the cat sat on the mat";
    //  "the" at [0,3) and [18,21)
    std::vector<std::pair<size_t,size_t>> offsets = {{0,3}, {18,21}};
    auto result = SearchHighlighter::applyHighlight(text, offsets, "<b>", "</b>");
    EXPECT_NE(result.find("<b>the</b>"), std::string::npos);
}

// ─────────────────────────────────────────────────────────────────────────────
// bestWindowOffset()
// ─────────────────────────────────────────────────────────────────────────────

TEST(SearchHighlighterWindow, EmptyTextReturnsZero) {
    EXPECT_EQ(SearchHighlighter::bestWindowOffset("", {"foo"}, 50), 0u);
}

TEST(SearchHighlighterWindow, TextShorterThanWindow_ReturnsZero) {
    EXPECT_EQ(SearchHighlighter::bestWindowOffset("short text", {"text"}, 100), 0u);
}

TEST(SearchHighlighterWindow, PreferWindowWithMoreTerms) {
    // "alpha beta gamma" is at the end; window should prefer that region
    std::string text =
        "nothing here aaa bbb ccc ddd eee fff ggg hhh iii jjj kkk "
        "alpha beta gamma delta";
    std::vector<std::string> terms = {"alpha", "beta", "gamma"};
    size_t offset = SearchHighlighter::bestWindowOffset(text, terms, 30);
    // The offset should be past the "nothing here..." prefix
    EXPECT_GT(offset, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// highlight()
// ─────────────────────────────────────────────────────────────────────────────

TEST(SearchHighlighterHighlight, EmptyTextReturnsEmpty) {
    SearchHighlighter h;
    EXPECT_TRUE(h.highlight("", {"term"}).empty());
}

TEST(SearchHighlighterHighlight, NoMatchReturnsOriginalText) {
    SearchHighlighter h;
    std::string text = "nothing to match here";
    EXPECT_EQ(h.highlight(text, {"xyz"}), text);
}

TEST(SearchHighlighterHighlight, SingleTermHighlighted) {
    SearchHighlighter h;
    auto result = h.highlight("the quick brown fox", {"quick"});
    EXPECT_NE(result.find("<mark>quick</mark>"), std::string::npos);
    EXPECT_NE(result.find("the "), std::string::npos);
    EXPECT_NE(result.find(" brown fox"), std::string::npos);
}

TEST(SearchHighlighterHighlight, MultipleTermsHighlighted) {
    SearchHighlighter h;
    auto result = h.highlight("the quick brown fox", {"quick", "fox"});
    EXPECT_NE(result.find("<mark>quick</mark>"), std::string::npos);
    EXPECT_NE(result.find("<mark>fox</mark>"), std::string::npos);
}

TEST(SearchHighlighterHighlight, CaseInsensitiveMatch) {
    SearchHighlighter h;
    auto result = h.highlight("The Quick Brown Fox", {"quick"});
    // "Quick" should be highlighted (case-insensitive match)
    EXPECT_NE(result.find("<mark>"), std::string::npos);
}

TEST(SearchHighlighterHighlight, CustomHighlightTags) {
    SearchHighlighter::Config cfg;
    cfg.highlight_open  = "<em>";
    cfg.highlight_close = "</em>";
    SearchHighlighter h(cfg);
    auto result = h.highlight("hello world", {"world"});
    EXPECT_NE(result.find("<em>world</em>"), std::string::npos);
}

// ─────────────────────────────────────────────────────────────────────────────
// snippet()
// ─────────────────────────────────────────────────────────────────────────────

TEST(SearchHighlighterSnippet, EmptyTextReturnsEmpty) {
    SearchHighlighter h;
    EXPECT_TRUE(h.snippet("", {"term"}).empty());
}

TEST(SearchHighlighterSnippet, ShortTextReturnedFully) {
    SearchHighlighter h;
    std::string text = "short doc";
    auto result = h.snippet(text, {"doc"}, 200);
    EXPECT_NE(result.find("doc"), std::string::npos);
}

TEST(SearchHighlighterSnippet, LongTextTruncatedWithEllipsis) {
    SearchHighlighter::Config cfg;
    cfg.max_snippet_len = 100;
    SearchHighlighter h(cfg);
    std::string long_text(500, 'x');
    long_text += " target ";
    long_text += std::string(500, 'y');
    auto result = h.snippet(long_text, {"target"}, 50);
    // Result should be shorter than the full text
    EXPECT_LT(result.size(), long_text.size());
}

TEST(SearchHighlighterSnippet, SnippetContainsMatchedTerm) {
    SearchHighlighter h;
    std::string text =
        "This is the beginning. "
        "The key term appears here in the middle. "
        "And here is some tail content that goes on for a while more text here.";
    auto result = h.snippet(text, {"key", "term"}, 60);
    // The snippet should contain the matched terms somewhere
    EXPECT_NE(result.find("key"), std::string::npos);
}

} // anonymous namespace
} // namespace themis
