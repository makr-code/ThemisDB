// Copyright (c) 2024 ThemisDB
// SPDX-License-Identifier: MIT

/**
 * @file test_content_markdown_processor.cpp
 * @brief Unit tests for MarkdownProcessor
 *
 * Tests cover:
 * - YAML frontmatter parsing (title, author, date, tags, inline lists)
 * - ATX heading stripping / preservation
 * - Emphasis stripping (bold, italic, bold+italic, strikethrough)
 * - Inline code and fenced code block handling
 * - Link and image extraction
 * - Blockquote stripping
 * - Horizontal rule stripping
 * - Table stripping
 * - Unordered and ordered list marker stripping
 * - Empty / malformed input handling
 * - extract() / chunk() / generateEmbedding() interface
 * - max_text_length config
 * - Factory functions
 */

#include <gtest/gtest.h>
#include "content/markdown_processor.h"
#include "content/content_type.h"
#include <string>
#include <vector>

using namespace themis::content;

// ============================================================================
// Helper
// ============================================================================

static ContentType makeMarkdownType() {
    ContentType ct;
    ct.mime_type = "text/markdown";
    ct.category  = ContentCategory::TEXT;
    ct.supports_text_extraction = true;
    return ct;
}

// ============================================================================
// parseFrontmatter
// ============================================================================

TEST(MarkdownProcessorTest, ParseFrontmatter_BasicScalars) {
    std::string md =
        "---\n"
        "title: My Document\n"
        "author: Jane Doe\n"
        "date: 2024-01-15\n"
        "---\n"
        "Body text here.";
    std::string body = {};
    auto fm = MarkdownProcessor::parseFrontmatter(md, body);

    EXPECT_EQ(fm["title"].get<std::string>(), "My Document");
    EXPECT_EQ(fm["author"].get<std::string>(), "Jane Doe");
    EXPECT_EQ(fm["date"].get<std::string>(), "2024-01-15");
    EXPECT_NE(body.find("Body text here"), std::string::npos);
    EXPECT_EQ(body.find("---"), std::string::npos);
}

TEST(MarkdownProcessorTest, ParseFrontmatter_QuotedValues) {
    std::string md =
        "---\n"
        "title: \"Quoted Title\"\n"
        "description: 'Single quoted'\n"
        "---\n";
    std::string body = {};
    auto fm = MarkdownProcessor::parseFrontmatter(md, body);
    EXPECT_EQ(fm["title"].get<std::string>(), "Quoted Title");
    EXPECT_EQ(fm["description"].get<std::string>(), "Single quoted");
}

TEST(MarkdownProcessorTest, ParseFrontmatter_InlineList) {
    std::string md =
        "---\n"
        "tags: [cpp, database, testing]\n"
        "---\n";
    std::string body = {};
    auto fm = MarkdownProcessor::parseFrontmatter(md, body);
    ASSERT_TRUE(fm["tags"].is_array());
    EXPECT_EQ(fm["tags"].size(), 3u);
    EXPECT_EQ(fm["tags"][0].get<std::string>(), "cpp");
    EXPECT_EQ(fm["tags"][1].get<std::string>(), "database");
    EXPECT_EQ(fm["tags"][2].get<std::string>(), "testing");
}

TEST(MarkdownProcessorTest, ParseFrontmatter_NoFrontmatter) {
    std::string md = "# Just a heading\n\nParagraph.";
    std::string body = {};
    auto fm = MarkdownProcessor::parseFrontmatter(md, body);
    EXPECT_TRUE(fm.empty());
    EXPECT_EQ(body, md);
}

TEST(MarkdownProcessorTest, ParseFrontmatter_ClosedWithDots) {
    std::string md =
        "---\n"
        "title: Dot Closed\n"
        "...\n"
        "Rest of document.";
    std::string body = {};
    auto fm = MarkdownProcessor::parseFrontmatter(md, body);
    EXPECT_EQ(fm["title"].get<std::string>(), "Dot Closed");
    EXPECT_NE(body.find("Rest of document"), std::string::npos);
}

TEST(MarkdownProcessorTest, ParseFrontmatter_MissingClosingDelimiter) {
    // No closing --- → not valid frontmatter, body = original
    std::string md =
        "---\n"
        "title: Unclosed\n"
        "# Heading\n";
    std::string body = {};
    auto fm = MarkdownProcessor::parseFrontmatter(md, body);
    EXPECT_TRUE(fm.empty());
    EXPECT_EQ(body, md);
}

TEST(MarkdownProcessorTest, ParseFrontmatter_EmptyInput) {
    std::string body = {};
    auto fm = MarkdownProcessor::parseFrontmatter("", body);
    EXPECT_TRUE(fm.empty());
    EXPECT_EQ(body, "");
}

// ============================================================================
// stripMarkdown — ATX headings
// ============================================================================

TEST(MarkdownProcessorTest, Strip_AtxH1_Stripped) {
    std::string result = MarkdownProcessor::stripMarkdown("# Hello World\n");
    EXPECT_NE(result.find("Hello World"), std::string::npos);
    EXPECT_EQ(result.find('#'), std::string::npos);
}

TEST(MarkdownProcessorTest, Strip_AtxH1_Preserved) {
    std::string result = MarkdownProcessor::stripMarkdown("# Hello World\n",
                                                           /*preserve_headings=*/true);
    EXPECT_NE(result.find("# Hello World"), std::string::npos);
}

TEST(MarkdownProcessorTest, Strip_AllHeadingLevels_Preserved) {
    std::string md = "# H1\n## H2\n### H3\n#### H4\n##### H5\n###### H6\n";
    std::string result = MarkdownProcessor::stripMarkdown(md, true);
    EXPECT_NE(result.find("# H1"), std::string::npos);
    EXPECT_NE(result.find("## H2"), std::string::npos);
    EXPECT_NE(result.find("### H3"), std::string::npos);
    EXPECT_NE(result.find("#### H4"), std::string::npos);
    EXPECT_NE(result.find("##### H5"), std::string::npos);
    EXPECT_NE(result.find("###### H6"), std::string::npos);
}

TEST(MarkdownProcessorTest, Strip_AtxHeading_ClosingHashes_Stripped) {
    // Closing hashes should be removed: "## Title ##" → "Title" (no preserve)
    std::string result = MarkdownProcessor::stripMarkdown("## Title ##\n");
    EXPECT_NE(result.find("Title"), std::string::npos);
    EXPECT_EQ(result.find("##"), std::string::npos);
}

// ============================================================================
// stripMarkdown — Emphasis
// ============================================================================

TEST(MarkdownProcessorTest, Strip_Bold_Asterisks) {
    std::string result = MarkdownProcessor::stripMarkdown("This is **bold** text.\n");
    EXPECT_NE(result.find("bold"), std::string::npos);
    EXPECT_EQ(result.find("**"), std::string::npos);
}

TEST(MarkdownProcessorTest, Strip_Bold_Underscores) {
    std::string result = MarkdownProcessor::stripMarkdown("This is __bold__ text.\n");
    EXPECT_NE(result.find("bold"), std::string::npos);
    EXPECT_EQ(result.find("__"), std::string::npos);
}

TEST(MarkdownProcessorTest, Strip_Italic_Asterisk) {
    std::string result = MarkdownProcessor::stripMarkdown("This is *italic* text.\n");
    EXPECT_NE(result.find("italic"), std::string::npos);
    EXPECT_EQ(result.find("*italic*"), std::string::npos);
}

TEST(MarkdownProcessorTest, Strip_BoldItalic) {
    std::string result = MarkdownProcessor::stripMarkdown("This is ***bold italic*** text.\n");
    EXPECT_NE(result.find("bold italic"), std::string::npos);
    EXPECT_EQ(result.find("***"), std::string::npos);
}

TEST(MarkdownProcessorTest, Strip_Strikethrough) {
    std::string result = MarkdownProcessor::stripMarkdown("This is ~~deleted~~ text.\n");
    EXPECT_NE(result.find("deleted"), std::string::npos);
    EXPECT_EQ(result.find("~~"), std::string::npos);
}

// ============================================================================
// stripMarkdown — Links and images
// ============================================================================

TEST(MarkdownProcessorTest, Strip_Link_KeepsText) {
    std::string result = MarkdownProcessor::stripMarkdown("[ThemisDB](https://example.com)\n");
    EXPECT_NE(result.find("ThemisDB"), std::string::npos);
    EXPECT_EQ(result.find("https://"), std::string::npos);
    EXPECT_EQ(result.find("[ThemisDB]"), std::string::npos);
}

TEST(MarkdownProcessorTest, Strip_Image_KeepsAlt) {
    std::string result = MarkdownProcessor::stripMarkdown("![Logo](logo.png)\n");
    EXPECT_NE(result.find("Logo"), std::string::npos);
    EXPECT_EQ(result.find("logo.png"), std::string::npos);
    EXPECT_EQ(result.find("!["), std::string::npos);
}

// ============================================================================
// stripMarkdown — Inline code
// ============================================================================

TEST(MarkdownProcessorTest, Strip_InlineCode_Preserved) {
    std::string result = MarkdownProcessor::stripMarkdown("Use `printf()` function.\n");
    EXPECT_NE(result.find("printf()"), std::string::npos);
    EXPECT_EQ(result.find('`'), std::string::npos);
}

TEST(MarkdownProcessorTest, Strip_InlineCode_Stripped) {
    std::string result = MarkdownProcessor::stripMarkdown(
        "Use `printf()` function.\n",
        /*preserve_headings=*/false,
        /*strip_code=*/true);
    EXPECT_EQ(result.find("printf"), std::string::npos);
}

// ============================================================================
// stripMarkdown — Fenced code blocks
// ============================================================================

TEST(MarkdownProcessorTest, Strip_FencedCode_Preserved) {
    std::string md = "Text before.\n```\nint x = 42;\n```\nText after.\n";
    std::string result = MarkdownProcessor::stripMarkdown(md);
    EXPECT_NE(result.find("Text before"), std::string::npos);
    EXPECT_NE(result.find("x = 42"), std::string::npos);
    EXPECT_NE(result.find("Text after"), std::string::npos);
    EXPECT_EQ(result.find("```"), std::string::npos);
}

TEST(MarkdownProcessorTest, Strip_FencedCode_Stripped) {
    std::string md = "Text before.\n```\nint x = 42;\n```\nText after.\n";
    std::string result = MarkdownProcessor::stripMarkdown(md, false, /*strip_code=*/true);
    EXPECT_NE(result.find("Text before"), std::string::npos);
    EXPECT_EQ(result.find("x = 42"), std::string::npos);
    EXPECT_NE(result.find("Text after"), std::string::npos);
}

// ============================================================================
// stripMarkdown — Blockquotes
// ============================================================================

TEST(MarkdownProcessorTest, Strip_Blockquote) {
    std::string result = MarkdownProcessor::stripMarkdown("> Quoted text here.\n");
    EXPECT_NE(result.find("Quoted text here"), std::string::npos);
    EXPECT_EQ(result.find('>'), std::string::npos);
}

TEST(MarkdownProcessorTest, Strip_NestedBlockquote) {
    std::string result = MarkdownProcessor::stripMarkdown("> > Nested quote.\n");
    EXPECT_NE(result.find("Nested quote"), std::string::npos);
    EXPECT_EQ(result.find('>'), std::string::npos);
}

// ============================================================================
// stripMarkdown — Horizontal rules
// ============================================================================

TEST(MarkdownProcessorTest, Strip_HorizontalRule_Dashes) {
    std::string result = MarkdownProcessor::stripMarkdown("Before\n---\nAfter\n");
    EXPECT_NE(result.find("Before"), std::string::npos);
    EXPECT_NE(result.find("After"), std::string::npos);
}

TEST(MarkdownProcessorTest, Strip_HorizontalRule_Asterisks) {
    std::string result = MarkdownProcessor::stripMarkdown("Before\n***\nAfter\n");
    EXPECT_NE(result.find("Before"), std::string::npos);
    EXPECT_NE(result.find("After"), std::string::npos);
}

// ============================================================================
// stripMarkdown — Lists
// ============================================================================

TEST(MarkdownProcessorTest, Strip_UnorderedList_Dash) {
    std::string md = "- Item one\n- Item two\n- Item three\n";
    std::string result = MarkdownProcessor::stripMarkdown(md);
    EXPECT_NE(result.find("Item one"), std::string::npos);
    EXPECT_NE(result.find("Item two"), std::string::npos);
}

TEST(MarkdownProcessorTest, Strip_UnorderedList_Asterisk) {
    std::string md = "* Item one\n* Item two\n";
    std::string result = MarkdownProcessor::stripMarkdown(md);
    EXPECT_NE(result.find("Item one"), std::string::npos);
    EXPECT_NE(result.find("Item two"), std::string::npos);
}

TEST(MarkdownProcessorTest, Strip_OrderedList) {
    std::string md = "1. First item\n2. Second item\n3. Third item\n";
    std::string result = MarkdownProcessor::stripMarkdown(md);
    EXPECT_NE(result.find("First item"), std::string::npos);
    EXPECT_NE(result.find("Second item"), std::string::npos);
    EXPECT_EQ(result.find("1."), std::string::npos);
}

// ============================================================================
// stripMarkdown — Tables
// ============================================================================

TEST(MarkdownProcessorTest, Strip_Table_SeparatorRowSkipped) {
    std::string md = "| Col1 | Col2 |\n|------|------|\n| A    | B    |\n";
    std::string result = MarkdownProcessor::stripMarkdown(md);
    // Separator row should not appear
    EXPECT_EQ(result.find("|---"), std::string::npos);
    // Data should be present
    EXPECT_NE(result.find("A"), std::string::npos);
    EXPECT_NE(result.find("B"), std::string::npos);
}

// ============================================================================
// extract() — full pipeline
// ============================================================================

TEST(MarkdownProcessorTest, Extract_BasicMarkdown) {
    MarkdownProcessor proc;
    ContentType ct = makeMarkdownType();
    std::string md = "# Hello\n\nThis is a **test** document.\n";
    auto result = proc.extract(md, ct);
    EXPECT_TRUE(result.ok);
    EXPECT_NE(result.text.find("Hello"), std::string::npos);
    EXPECT_NE(result.text.find("test"), std::string::npos);
    EXPECT_EQ(result.text.find("**"), std::string::npos);
}

TEST(MarkdownProcessorTest, Extract_WithFrontmatter) {
    MarkdownProcessor proc;
    ContentType ct = makeMarkdownType();
    std::string md =
        "---\n"
        "title: Test Doc\n"
        "author: Tester\n"
        "---\n"
        "# Introduction\n\nDocument body.\n";
    auto result = proc.extract(md, ct);
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.metadata["title"].get<std::string>(), "Test Doc");
    EXPECT_EQ(result.metadata["author"].get<std::string>(), "Tester");
    EXPECT_NE(result.text.find("Document body"), std::string::npos);
    // Frontmatter delimiters should NOT be in the extracted text
    EXPECT_EQ(result.text.find("---"), std::string::npos);
}

TEST(MarkdownProcessorTest, Extract_EmptyBlob) {
    MarkdownProcessor proc;
    ContentType ct = makeMarkdownType();
    auto result = proc.extract("", ct);
    EXPECT_FALSE(result.ok);
    EXPECT_FALSE(result.error_message.empty());
}

TEST(MarkdownProcessorTest, Extract_MetadataPopulated) {
    MarkdownProcessor proc;
    ContentType ct = makeMarkdownType();
    std::string md = "Some text with multiple words here.";
    auto result = proc.extract(md, ct);
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.metadata["mime_type"].get<std::string>(), "text/markdown");
    EXPECT_GT(result.metadata["original_size_bytes"].get<int64_t>(), 0);
    EXPECT_GT(result.metadata["token_count"].get<int>(), 0);
    EXPECT_TRUE(result.metadata.contains("has_frontmatter"));
}

TEST(MarkdownProcessorTest, Extract_HasFrontmatter_True) {
    MarkdownProcessor proc;
    ContentType ct = makeMarkdownType();
    std::string md = "---\ntitle: X\n---\nText.";
    auto result = proc.extract(md, ct);
    EXPECT_TRUE(result.ok);
    EXPECT_TRUE(result.metadata["has_frontmatter"].get<bool>());
}

TEST(MarkdownProcessorTest, Extract_HasFrontmatter_False) {
    MarkdownProcessor proc;
    ContentType ct = makeMarkdownType();
    std::string md = "# No frontmatter here.\n\nJust text.";
    auto result = proc.extract(md, ct);
    EXPECT_TRUE(result.ok);
    EXPECT_FALSE(result.metadata["has_frontmatter"].get<bool>());
}

TEST(MarkdownProcessorTest, Extract_MaxTextLength) {
    MarkdownProcessor::Config cfg;
    cfg.max_text_length = 10;
    MarkdownProcessor proc(cfg);
    ContentType ct = makeMarkdownType();
    std::string md = "This is a longer markdown text that exceeds the limit.\n";
    auto result = proc.extract(md, ct);
    EXPECT_TRUE(result.ok);
    EXPECT_LE(result.text.size(), 10u);
}

TEST(MarkdownProcessorTest, Extract_DisableFrontmatterParsing) {
    MarkdownProcessor::Config cfg;
    cfg.parse_frontmatter = false;
    MarkdownProcessor proc(cfg);
    ContentType ct = makeMarkdownType();
    std::string md = "---\ntitle: Title\n---\nBody.";
    auto result = proc.extract(md, ct);
    EXPECT_TRUE(result.ok);
    // With frontmatter parsing disabled, "---" delimiters appear in text
    EXPECT_FALSE(result.metadata.contains("title"));
}

TEST(MarkdownProcessorTest, Extract_PreserveHeadingMarkers) {
    MarkdownProcessor::Config cfg;
    cfg.preserve_heading_markers = true;
    MarkdownProcessor proc(cfg);
    ContentType ct = makeMarkdownType();
    std::string md = "# Section One\n\nBody text.\n\n## Sub Section\n\nMore text.\n";
    auto result = proc.extract(md, ct);
    EXPECT_TRUE(result.ok);
    EXPECT_NE(result.text.find("# Section One"), std::string::npos);
    EXPECT_NE(result.text.find("## Sub Section"), std::string::npos);
}

// ============================================================================
// chunk() tests
// ============================================================================

TEST(MarkdownProcessorTest, Chunk_BasicSplitting) {
    MarkdownProcessor proc;
    ContentType ct = makeMarkdownType();
    std::string md = {};
    for (int i = 0; i < 10; ++i) {
        md += "## Section " + std::to_string(i) + "\n\n";
        md += "This section contains some text content to ensure token count.\n\n";
    }
    auto extraction = proc.extract(md, ct);
    EXPECT_TRUE(extraction.ok);

    auto chunks = proc.chunk(extraction, 20, 5);
    EXPECT_GT(chunks.size(), 1u);

    for (const auto& c : chunks) {
        EXPECT_TRUE(c.contains("text"));
        EXPECT_FALSE(c["text"].get<std::string>().empty());
        EXPECT_TRUE(c.contains("seq_num"));
        EXPECT_EQ(c["chunk_type"].get<std::string>(), "text");
    }
}

TEST(MarkdownProcessorTest, Chunk_SingleChunkForSmallContent) {
    MarkdownProcessor proc;
    ContentType ct = makeMarkdownType();
    std::string md = "Short markdown text.";
    auto extraction = proc.extract(md, ct);
    auto chunks = proc.chunk(extraction, 512, 50);
    EXPECT_EQ(chunks.size(), 1u);
}

TEST(MarkdownProcessorTest, Chunk_EmptyExtraction) {
    MarkdownProcessor proc;
    ExtractionResult empty;
    empty.ok   = true;
    empty.text = "";
    auto chunks = proc.chunk(empty, 512, 50);
    EXPECT_TRUE(chunks.empty());
}

TEST(MarkdownProcessorTest, Chunk_SeqNumsAreSequential) {
    MarkdownProcessor proc;
    ContentType ct = makeMarkdownType();
    std::string md = {};
    for (int i = 0; i < 20; ++i) {
        md += "Paragraph " + std::to_string(i) + " with enough words to fill chunks.\n\n";
    }
    auto extraction = proc.extract(md, ct);
    auto chunks = proc.chunk(extraction, 10, 2);
    for (size_t i = 0; i < chunks.size(); ++i) {
        EXPECT_EQ(chunks[i]["seq_num"].get<int>(), static_cast<int>(i));
    }
}

// ============================================================================
// generateEmbedding() tests
// ============================================================================

TEST(MarkdownProcessorTest, GenerateEmbedding_CorrectDimension) {
    MarkdownProcessor proc;
    auto emb = proc.generateEmbedding("hello world test");
    EXPECT_EQ(emb.size(), 768u);
}

TEST(MarkdownProcessorTest, GenerateEmbedding_NonZeroForNonEmpty) {
    MarkdownProcessor proc;
    auto emb = proc.generateEmbedding("some text here");
    float norm = 0.0f;
    for (float v : emb) {
      norm += v * v;
    }
    EXPECT_GT(norm, 0.0f);
}

TEST(MarkdownProcessorTest, GenerateEmbedding_ZeroForEmpty) {
    MarkdownProcessor proc;
    auto emb = proc.generateEmbedding("");
    for (float v : emb) {
      EXPECT_EQ(v, 0.0f);
    }
}

TEST(MarkdownProcessorTest, GenerateEmbedding_Deterministic) {
    MarkdownProcessor proc;
    auto emb1 = proc.generateEmbedding("the quick brown fox");
    auto emb2 = proc.generateEmbedding("the quick brown fox");
    EXPECT_EQ(emb1, emb2);
}

TEST(MarkdownProcessorTest, GenerateEmbedding_DifferentTexts) {
    MarkdownProcessor proc;
    auto emb1 = proc.generateEmbedding("text about databases");
    auto emb2 = proc.generateEmbedding("completely different content");
    EXPECT_NE(emb1, emb2);
}

// ============================================================================
// getName / getSupportedCategories
// ============================================================================

TEST(MarkdownProcessorTest, GetName) {
    MarkdownProcessor proc;
    EXPECT_EQ(proc.getName(), "MarkdownProcessor");
}

TEST(MarkdownProcessorTest, GetSupportedCategories) {
    MarkdownProcessor proc;
    auto cats = proc.getSupportedCategories();
    ASSERT_EQ(cats.size(), 1u);
    EXPECT_EQ(cats[0], ContentCategory::TEXT);
}

// ============================================================================
// Factory function tests
// ============================================================================

TEST(MarkdownProcessorTest, CreateMarkdownProcessor_Default) {
    auto proc = createMarkdownProcessor();
    ASSERT_NE(proc, nullptr);
    EXPECT_EQ(proc->getName(), "MarkdownProcessor");
}

TEST(MarkdownProcessorTest, CreateMarkdownProcessor_WithConfig) {
    MarkdownProcessor::Config cfg;
    cfg.preserve_heading_markers = true;
    auto proc = createMarkdownProcessor(cfg);
    ASSERT_NE(proc, nullptr);
    ContentType ct = makeMarkdownType();
    auto result = proc->extract("# My Heading\n\nText.", ct);
    EXPECT_TRUE(result.ok);
    EXPECT_NE(result.text.find("# My Heading"), std::string::npos);
}
