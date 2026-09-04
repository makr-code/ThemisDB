// Copyright (c) 2024 ThemisDB
// SPDX-License-Identifier: MIT

/**
 * @file test_content_html_processor.cpp
 * @brief Unit tests for HtmlProcessor
 *
 * Tests cover:
 * - Basic HTML tag stripping
 * - Boilerplate removal (<nav>, <header>, <footer>, <aside>, <form>)
 * - Script and style element removal
 * - HTML entity decoding (named, decimal, hex)
 * - Metadata extraction (<title>, <meta name="...">)
 * - Text chunking by paragraph boundary
 * - Empty / malformed input handling
 * - extract() / chunk() / generateEmbedding() interface
 */

#include <gtest/gtest.h>
#include "content/html_processor.h"
#include "content/content_type.h"
#include <string>
#include <vector>

using namespace themis::content;

// ============================================================================
// Helper
// ============================================================================

static ContentType makeHtmlType() {
    ContentType ct;
    ct.mime_type = "text/html";
    ct.category  = ContentCategory::TEXT;
    ct.supports_text_extraction = true;
    return ct;
}

// ============================================================================
// removeScriptsAndStyles
// ============================================================================

TEST(HtmlProcessorTest, RemoveScript_BasicScript) {
    std::string html = "<p>Hello</p><script>alert('xss')</script><p>World</p>";
    std::string result = HtmlProcessor::removeScriptsAndStyles(html);
    EXPECT_EQ(result.find("alert"), std::string::npos);
    EXPECT_NE(result.find("Hello"), std::string::npos);
    EXPECT_NE(result.find("World"), std::string::npos);
}

TEST(HtmlProcessorTest, RemoveStyle_BasicStyle) {
    std::string html = "<style>body { color: red; }</style><p>Content</p>";
    std::string result = HtmlProcessor::removeScriptsAndStyles(html);
    EXPECT_EQ(result.find("color"), std::string::npos);
    EXPECT_NE(result.find("Content"), std::string::npos);
}

TEST(HtmlProcessorTest, RemoveScript_MultiLine) {
    std::string html = "<p>Before</p>\n<script type=\"text/javascript\">\nvar x = 1;\n</script>\n<p>After</p>";
    std::string result = HtmlProcessor::removeScriptsAndStyles(html);
    EXPECT_EQ(result.find("var x"), std::string::npos);
    EXPECT_NE(result.find("Before"), std::string::npos);
    EXPECT_NE(result.find("After"), std::string::npos);
}

TEST(HtmlProcessorTest, RemoveScript_NoScript) {
    std::string html = "<p>No scripts here</p>";
    std::string result = HtmlProcessor::removeScriptsAndStyles(html);
    EXPECT_NE(result.find("No scripts"), std::string::npos);
}

// ============================================================================
// removeBoilerplate
// ============================================================================

TEST(HtmlProcessorTest, RemoveBoilerplate_Nav) {
    std::string html = "<nav><a href='/'>Home</a><a href='/about'>About</a></nav>"
                       "<main><p>Article content</p></main>";
    std::string result = HtmlProcessor::removeBoilerplate(html);
    EXPECT_EQ(result.find("Home"), std::string::npos);
    EXPECT_EQ(result.find("About"), std::string::npos);
    EXPECT_NE(result.find("Article content"), std::string::npos);
}

TEST(HtmlProcessorTest, RemoveBoilerplate_Header) {
    std::string html = "<header><h1>Site Title</h1><nav>...</nav></header>"
                       "<article><p>Real content</p></article>";
    std::string result = HtmlProcessor::removeBoilerplate(html);
    EXPECT_EQ(result.find("Site Title"), std::string::npos);
    EXPECT_NE(result.find("Real content"), std::string::npos);
}

TEST(HtmlProcessorTest, RemoveBoilerplate_Footer) {
    std::string html = "<p>Main text</p>"
                       "<footer><p>Copyright 2024</p></footer>";
    std::string result = HtmlProcessor::removeBoilerplate(html);
    EXPECT_EQ(result.find("Copyright"), std::string::npos);
    EXPECT_NE(result.find("Main text"), std::string::npos);
}

TEST(HtmlProcessorTest, RemoveBoilerplate_Aside) {
    std::string html = "<p>Body text</p>"
                       "<aside><p>Sidebar ad</p></aside>";
    std::string result = HtmlProcessor::removeBoilerplate(html);
    EXPECT_EQ(result.find("Sidebar ad"), std::string::npos);
    EXPECT_NE(result.find("Body text"), std::string::npos);
}

TEST(HtmlProcessorTest, RemoveBoilerplate_Form) {
    std::string html = "<form><input type='text' name='q'/><button>Search</button></form>"
                       "<p>Article</p>";
    std::string result = HtmlProcessor::removeBoilerplate(html);
    EXPECT_EQ(result.find("Search"), std::string::npos);
    EXPECT_NE(result.find("Article"), std::string::npos);
}

TEST(HtmlProcessorTest, RemoveBoilerplate_NestedNav) {
    std::string html = "<nav><ul><li><a>Item1</a></li><li><nav>Sub</nav></li></ul></nav><p>Content</p>";
    std::string result = HtmlProcessor::removeBoilerplate(html);
    EXPECT_EQ(result.find("Item1"), std::string::npos);
    EXPECT_EQ(result.find("Sub"), std::string::npos);
    EXPECT_NE(result.find("Content"), std::string::npos);
}

// ============================================================================
// stripTags
// ============================================================================

TEST(HtmlProcessorTest, StripTags_BasicTags) {
    std::string html = "<p>Hello <b>world</b></p>";
    std::string result = HtmlProcessor::stripTags(html);
    EXPECT_NE(result.find("Hello"), std::string::npos);
    EXPECT_NE(result.find("world"), std::string::npos);
    EXPECT_EQ(result.find('<'), std::string::npos);
}

TEST(HtmlProcessorTest, StripTags_PreservesText) {
    std::string html = "<h1>Title</h1><p>Paragraph one.</p><p>Paragraph two.</p>";
    std::string result = HtmlProcessor::stripTags(html);
    EXPECT_NE(result.find("Title"), std::string::npos);
    EXPECT_NE(result.find("Paragraph one"), std::string::npos);
    EXPECT_NE(result.find("Paragraph two"), std::string::npos);
}

TEST(HtmlProcessorTest, StripTags_EmptyInput) {
    EXPECT_EQ(HtmlProcessor::stripTags(""), "");
}

TEST(HtmlProcessorTest, StripTags_OnlyTags) {
    std::string result = HtmlProcessor::stripTags("<div><span></span></div>");
    // Only whitespace or empty after stripping
    auto trimmed = result;
    trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
    trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);
    EXPECT_TRUE(trimmed.empty());
}

// ============================================================================
// decodeEntities
// ============================================================================

TEST(HtmlProcessorTest, DecodeEntities_Named) {
    EXPECT_EQ(HtmlProcessor::decodeEntities("AT&amp;T"), "AT&T");
    EXPECT_EQ(HtmlProcessor::decodeEntities("&lt;tag&gt;"), "<tag>");
    EXPECT_EQ(HtmlProcessor::decodeEntities("say &quot;hello&quot;"), "say \"hello\"");
    EXPECT_EQ(HtmlProcessor::decodeEntities("it&apos;s"), "it's");
}

TEST(HtmlProcessorTest, DecodeEntities_Nbsp) {
    std::string result = HtmlProcessor::decodeEntities("hello&nbsp;world");
    // &nbsp; should decode to a space or non-breaking space
    EXPECT_NE(result.find("hello"), std::string::npos);
    EXPECT_NE(result.find("world"), std::string::npos);
}

TEST(HtmlProcessorTest, DecodeEntities_Decimal) {
    EXPECT_EQ(HtmlProcessor::decodeEntities("&#65;"), "A");   // 'A'
    EXPECT_EQ(HtmlProcessor::decodeEntities("&#60;"), "<");   // '<'
}

TEST(HtmlProcessorTest, DecodeEntities_Hex) {
    EXPECT_EQ(HtmlProcessor::decodeEntities("&#x41;"), "A");  // 'A'
    EXPECT_EQ(HtmlProcessor::decodeEntities("&#x3C;"), "<");  // '<'
}

TEST(HtmlProcessorTest, DecodeEntities_NoEntities) {
    EXPECT_EQ(HtmlProcessor::decodeEntities("plain text"), "plain text");
}

TEST(HtmlProcessorTest, DecodeEntities_Unknown) {
    // Unknown entity should be kept as-is
    std::string result = HtmlProcessor::decodeEntities("&unknown;");
    EXPECT_NE(result.find("unknown"), std::string::npos);
}

// ============================================================================
// extractMetaTags
// ============================================================================

TEST(HtmlProcessorTest, ExtractMeta_Title) {
    std::string html = "<html><head><title>My Page Title</title></head><body></body></html>";
    auto meta = HtmlProcessor::extractMetaTags(html);
    EXPECT_EQ(meta["title"].get<std::string>(), "My Page Title");
}

TEST(HtmlProcessorTest, ExtractMeta_Description) {
    std::string html = R"(<meta name="description" content="A great article about testing">)";
    auto meta = HtmlProcessor::extractMetaTags(html);
    EXPECT_EQ(meta["description"].get<std::string>(), "A great article about testing");
}

TEST(HtmlProcessorTest, ExtractMeta_Keywords) {
    std::string html = R"(<meta name="keywords" content="C++, testing, html">)";
    auto meta = HtmlProcessor::extractMetaTags(html);
    EXPECT_EQ(meta["keywords"].get<std::string>(), "C++, testing, html");
}

TEST(HtmlProcessorTest, ExtractMeta_Author) {
    std::string html = R"(<meta name="author" content="Jane Doe">)";
    auto meta = HtmlProcessor::extractMetaTags(html);
    EXPECT_EQ(meta["author"].get<std::string>(), "Jane Doe");
}

TEST(HtmlProcessorTest, ExtractMeta_AllFields) {
    std::string html =
        "<html><head>"
        "<title>Full Page</title>"
        R"(<meta name="description" content="Desc">)"
        R"(<meta name="keywords" content="kw1,kw2">)"
        R"(<meta name="author" content="Author Name">)"
        "</head><body><p>Text</p></body></html>";
    auto meta = HtmlProcessor::extractMetaTags(html);
    EXPECT_EQ(meta["title"].get<std::string>(), "Full Page");
    EXPECT_EQ(meta["description"].get<std::string>(), "Desc");
    EXPECT_EQ(meta["keywords"].get<std::string>(), "kw1,kw2");
    EXPECT_EQ(meta["author"].get<std::string>(), "Author Name");
}

TEST(HtmlProcessorTest, ExtractMeta_MissingFields) {
    std::string html = "<html><body><p>No metadata</p></body></html>";
    auto meta = HtmlProcessor::extractMetaTags(html);
    EXPECT_EQ(meta["title"].get<std::string>(), "");
    EXPECT_EQ(meta["description"].get<std::string>(), "");
}

// ============================================================================
// extract() - full pipeline
// ============================================================================

TEST(HtmlProcessorTest, Extract_BasicHtml) {
    HtmlProcessor proc;
    ContentType ct = makeHtmlType();
    std::string html = "<html><head><title>Test</title></head>"
                       "<body><h1>Hello</h1><p>World content here.</p></body></html>";
    auto result = proc.extract(html, ct);
    EXPECT_TRUE(result.ok);
    EXPECT_NE(result.text.find("Hello"), std::string::npos);
    EXPECT_NE(result.text.find("World content"), std::string::npos);
    EXPECT_EQ(result.text.find('<'), std::string::npos);
}

TEST(HtmlProcessorTest, Extract_BoilerplateRemoved) {
    HtmlProcessor proc;
    ContentType ct = makeHtmlType();
    std::string html =
        "<nav><a>Nav link</a></nav>"
        "<main><p>Actual article content</p></main>"
        "<footer><p>Footer text</p></footer>";
    auto result = proc.extract(html, ct);
    EXPECT_TRUE(result.ok);
    EXPECT_NE(result.text.find("Actual article content"), std::string::npos);
    EXPECT_EQ(result.text.find("Nav link"), std::string::npos);
    EXPECT_EQ(result.text.find("Footer text"), std::string::npos);
}

TEST(HtmlProcessorTest, Extract_ScriptRemoved) {
    HtmlProcessor proc;
    ContentType ct = makeHtmlType();
    std::string html =
        "<p>Content</p>"
        "<script>var evil = 'xss';</script>"
        "<p>More content</p>";
    auto result = proc.extract(html, ct);
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.text.find("evil"), std::string::npos);
    EXPECT_NE(result.text.find("Content"), std::string::npos);
    EXPECT_NE(result.text.find("More content"), std::string::npos);
}

TEST(HtmlProcessorTest, Extract_EntitiesDecoded) {
    HtmlProcessor proc;
    ContentType ct = makeHtmlType();
    std::string html = "<p>AT&amp;T &lt;rocks&gt;</p>";
    auto result = proc.extract(html, ct);
    EXPECT_TRUE(result.ok);
    EXPECT_NE(result.text.find("AT&T"), std::string::npos);
    EXPECT_NE(result.text.find("<rocks>"), std::string::npos);
}

TEST(HtmlProcessorTest, Extract_EmptyBlob) {
    HtmlProcessor proc;
    ContentType ct = makeHtmlType();
    auto result = proc.extract("", ct);
    EXPECT_FALSE(result.ok);
    EXPECT_FALSE(result.error_message.empty());
}

TEST(HtmlProcessorTest, Extract_MetadataPopulated) {
    HtmlProcessor proc;
    ContentType ct = makeHtmlType();
    std::string html =
        "<html><head><title>Meta Test</title>"
        R"(<meta name="description" content="Test description">)"
        "</head><body><p>Body text</p></body></html>";
    auto result = proc.extract(html, ct);
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.metadata["title"].get<std::string>(), "Meta Test");
    EXPECT_EQ(result.metadata["description"].get<std::string>(), "Test description");
    EXPECT_EQ(result.metadata["mime_type"].get<std::string>(), "text/html");
    EXPECT_GT(result.metadata["original_size_bytes"].get<int64_t>(), 0);
    EXPECT_GT(result.metadata["token_count"].get<int>(), 0);
}

TEST(HtmlProcessorTest, Extract_MaxTextLength) {
    HtmlProcessor::Config cfg;
    cfg.max_text_length = 10;
    HtmlProcessor proc(cfg);
    ContentType ct = makeHtmlType();
    std::string html = "<p>This is a longer text that exceeds the limit.</p>";
    auto result = proc.extract(html, ct);
    EXPECT_TRUE(result.ok);
    EXPECT_LE(result.text.size(), 10u);
}

TEST(HtmlProcessorTest, Extract_DisableBoilerplateRemoval) {
    HtmlProcessor::Config cfg;
    cfg.remove_boilerplate = false;
    HtmlProcessor proc(cfg);
    ContentType ct = makeHtmlType();
    std::string html = "<nav><a>Nav</a></nav><p>Content</p>";
    auto result = proc.extract(html, ct);
    EXPECT_TRUE(result.ok);
    // With boilerplate removal disabled, nav content should appear
    EXPECT_NE(result.text.find("Nav"), std::string::npos);
}

// ============================================================================
// chunk() tests
// ============================================================================

TEST(HtmlProcessorTest, Chunk_BasicSplitting) {
    HtmlProcessor proc;
    ContentType ct = makeHtmlType();
    // Build HTML with many paragraphs
    std::string html = "<html><body>";
    for (int i = 0; i < 10; ++i) {
        html += "<p>This is paragraph number " + std::to_string(i) +
                " with some text content to ensure token count.</p>";
    }
    html += "</body></html>";

    auto extraction = proc.extract(html, ct);
    EXPECT_TRUE(extraction.ok);

    // Small chunk size forces multiple chunks
    auto chunks = proc.chunk(extraction, 20, 5);
    EXPECT_GT(chunks.size(), 1u);

    for (const auto& c : chunks) {
        EXPECT_TRUE(c.contains("text"));
        EXPECT_FALSE(c["text"].get<std::string>().empty());
        EXPECT_TRUE(c.contains("seq_num"));
        EXPECT_EQ(c["chunk_type"].get<std::string>(), "text");
    }
}

TEST(HtmlProcessorTest, Chunk_SingleChunkForSmallContent) {
    HtmlProcessor proc;
    ContentType ct = makeHtmlType();
    std::string html = "<p>Short text.</p>";
    auto extraction = proc.extract(html, ct);
    auto chunks = proc.chunk(extraction, 512, 50);
    EXPECT_EQ(chunks.size(), 1u);
}

TEST(HtmlProcessorTest, Chunk_EmptyExtraction) {
    HtmlProcessor proc;
    ExtractionResult empty_result;
    empty_result.ok   = true;
    empty_result.text = "";
    auto chunks = proc.chunk(empty_result, 512, 50);
    EXPECT_TRUE(chunks.empty());
}

TEST(HtmlProcessorTest, Chunk_SeqNumsAreSequential) {
    HtmlProcessor proc;
    ContentType ct = makeHtmlType();
    std::string html = "<html><body>";
    for (int i = 0; i < 20; ++i) {
        html += "<p>Paragraph " + std::to_string(i) + " with enough words to fill a chunk properly.</p>";
    }
    html += "</body></html>";
    auto extraction = proc.extract(html, ct);
    auto chunks = proc.chunk(extraction, 15, 3);
    for (size_t i = 0; i < chunks.size(); ++i) {
        EXPECT_EQ(chunks[i]["seq_num"].get<int>(), static_cast<int>(i));
    }
}

// ============================================================================
// generateEmbedding() tests
// ============================================================================

TEST(HtmlProcessorTest, GenerateEmbedding_CorrectDimension) {
    HtmlProcessor proc;
    auto emb = proc.generateEmbedding("hello world test");
    EXPECT_EQ(emb.size(), 768u);
}

TEST(HtmlProcessorTest, GenerateEmbedding_NonZeroForNonEmpty) {
    HtmlProcessor proc;
    auto emb = proc.generateEmbedding("some text here");
    float norm = 0.0f;
    for (float v : emb) {
      norm += v * v;
    }
    EXPECT_GT(norm, 0.0f);
}

TEST(HtmlProcessorTest, GenerateEmbedding_ZeroForEmpty) {
    HtmlProcessor proc;
    auto emb = proc.generateEmbedding("");
    for (float v : emb) {
      EXPECT_EQ(v, 0.0f);
    }
}

TEST(HtmlProcessorTest, GenerateEmbedding_Deterministic) {
    HtmlProcessor proc;
    auto emb1 = proc.generateEmbedding("the quick brown fox");
    auto emb2 = proc.generateEmbedding("the quick brown fox");
    EXPECT_EQ(emb1, emb2);
}

TEST(HtmlProcessorTest, GenerateEmbedding_DifferentTextsDifferent) {
    HtmlProcessor proc;
    auto emb1 = proc.generateEmbedding("text about databases");
    auto emb2 = proc.generateEmbedding("completely different content");
    EXPECT_NE(emb1, emb2);
}

// ============================================================================
// getName / getSupportedCategories
// ============================================================================

TEST(HtmlProcessorTest, GetName) {
    HtmlProcessor proc;
    EXPECT_EQ(proc.getName(), "HtmlProcessor");
}

TEST(HtmlProcessorTest, GetSupportedCategories) {
    HtmlProcessor proc;
    auto cats = proc.getSupportedCategories();
    ASSERT_EQ(cats.size(), 1u);
    EXPECT_EQ(cats[0], ContentCategory::TEXT);
}

// ============================================================================
// preserve_heading_markers
// ============================================================================

TEST(HtmlProcessorTest, StripTags_PreserveHeadings_H1) {
    std::string html = "<h1>Main Title</h1><p>Text</p>";
    std::string result = HtmlProcessor::stripTags(html, /*preserve_headings=*/true);
    EXPECT_NE(result.find("# Main Title"), std::string::npos);
    EXPECT_NE(result.find("Text"), std::string::npos);
    EXPECT_EQ(result.find('<'), std::string::npos);
}

TEST(HtmlProcessorTest, StripTags_PreserveHeadings_AllLevels) {
    std::string html =
        "<h1>H1</h1><h2>H2</h2><h3>H3</h3>"
        "<h4>H4</h4><h5>H5</h5><h6>H6</h6>";
    std::string result = HtmlProcessor::stripTags(html, /*preserve_headings=*/true);
    EXPECT_NE(result.find("# H1"), std::string::npos);
    EXPECT_NE(result.find("## H2"), std::string::npos);
    EXPECT_NE(result.find("### H3"), std::string::npos);
    EXPECT_NE(result.find("#### H4"), std::string::npos);
    EXPECT_NE(result.find("##### H5"), std::string::npos);
    EXPECT_NE(result.find("###### H6"), std::string::npos);
}

TEST(HtmlProcessorTest, StripTags_NoPreserveHeadings_Default) {
    // Without preserve_headings, headings should not produce # markers
    std::string html = "<h1>Title</h1><p>Content</p>";
    std::string result = HtmlProcessor::stripTags(html);  // default: preserve_headings=false
    // Heading text preserved but no # prefix
    EXPECT_NE(result.find("Title"), std::string::npos);
    EXPECT_EQ(result.find("# Title"), std::string::npos);
}

TEST(HtmlProcessorTest, Extract_PreserveHeadingMarkers) {
    HtmlProcessor::Config cfg;
    cfg.preserve_heading_markers = true;
    HtmlProcessor proc(cfg);
    ContentType ct = makeHtmlType();
    std::string html = "<h1>Section One</h1><p>Body text.</p><h2>Sub Section</h2><p>More text.</p>";
    auto result = proc.extract(html, ct);
    EXPECT_TRUE(result.ok);
    EXPECT_NE(result.text.find("# Section One"), std::string::npos);
    EXPECT_NE(result.text.find("## Sub Section"), std::string::npos);
    EXPECT_NE(result.text.find("Body text"), std::string::npos);
}

// ============================================================================
// Factory function tests
// ============================================================================

TEST(HtmlProcessorTest, CreateHtmlProcessor_Default) {
    auto proc = createHtmlProcessor();
    ASSERT_NE(proc, nullptr);
    EXPECT_EQ(proc->getName(), "HtmlProcessor");
}

TEST(HtmlProcessorTest, CreateHtmlProcessor_WithConfig) {
    HtmlProcessor::Config cfg;
    cfg.remove_boilerplate = false;
    auto proc = createHtmlProcessor(cfg);
    ASSERT_NE(proc, nullptr);
    // With boilerplate removal disabled, nav content should appear
    ContentType ct = makeHtmlType();
    auto result = proc->extract("<nav><a>Nav</a></nav><p>Content</p>", ct);
    EXPECT_TRUE(result.ok);
    EXPECT_NE(result.text.find("Nav"), std::string::npos);
}

// ============================================================================
// application/xhtml+xml routing
// ============================================================================

TEST(HtmlProcessorTest, Extract_XhtmlMimeType) {
    HtmlProcessor proc;
    ContentType ct;
    ct.mime_type = "application/xhtml+xml";
    ct.category  = ContentCategory::TEXT;
    std::string xhtml = R"xhtml(<?xml version="1.0"?>
<html xmlns="http://www.w3.org/1999/xhtml">
<head><title>XHTML Page</title></head>
<body><p>XHTML content</p></body></html>)xhtml";
    auto result = proc.extract(xhtml, ct);
    EXPECT_TRUE(result.ok);
    EXPECT_NE(result.text.find("XHTML content"), std::string::npos);
    EXPECT_EQ(result.metadata["mime_type"].get<std::string>(), "application/xhtml+xml");
}
