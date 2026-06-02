/*
 * ThemisDB | File: markdown_processor.h | Version: 0.0.15 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 173
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// Copyright (c) 2024 ThemisDB
// SPDX-License-Identifier: MIT

/**
 * @file markdown_processor.h
 * @brief Markdown Content Processor for ThemisDB
 *
 * Parses YAML frontmatter and extracts plain text from Markdown documents.
 * Supports ATX and setext headings, emphasis, links, images, code blocks,
 * blockquotes, lists, and tables.
 *
 * Pure C++ implementation — no external Markdown parsing library required.
 *
 * @author ThemisDB Team
 */

#pragma once

#include "content/content_processor.h"
#include <string>
#include <vector>

namespace themis {
namespace content {

/**
 * @brief Markdown Content Processor
 *
 * Handles Markdown documents (text/markdown, .md, .markdown):
 * - YAML frontmatter parsing (title, author, date, tags, description, etc.)
 * - Plain text extraction with Markdown syntax stripping
 * - Optional preservation of heading markers ("# ", "## ", …)
 * - Chunking by heading / paragraph boundary
 * - Hash-based 768-dim embedding generation (compatible with TextProcessor)
 */
class MarkdownProcessor : public IContentProcessor {
public:
    /**
     * @brief Configuration for Markdown processing
     */
    struct Config {
        /// Parse YAML frontmatter block (between leading --- delimiters)
        bool parse_frontmatter = true;
        /// Preserve ATX heading markers in extracted text ("# Title\n")
        bool preserve_heading_markers = false;
        /// Strip code block contents (fenced ``` and indented blocks)
        bool strip_code_blocks = false;
        /// Max characters of extracted text (0 = unlimited)
        size_t max_text_length = 0;
    };

    MarkdownProcessor();
    explicit MarkdownProcessor(Config config);
    ~MarkdownProcessor() override = default;

    /**
     * @brief Extract plain text and metadata from a Markdown document
     *
     * @param blob   Raw Markdown bytes
     * @param content_type  Content type info (mime_type should be "text/markdown")
     * @return ExtractionResult with plain text in `.text` and frontmatter in `.metadata`
     */
    ExtractionResult extract(
        const std::string& blob,
        const ContentType& content_type
    ) override;

    /**
     * @brief Chunk extracted Markdown text for RAG / search
     *
     * Splits on heading and paragraph boundaries, then falls back to
     * token-count windows when individual sections exceed chunk_size.
     *
     * @param extraction_result  Result of a previous extract() call
     * @param chunk_size  Target chunk size in whitespace-delimited tokens
     * @param overlap     Token overlap between consecutive chunks
     * @return Vector of chunk JSON objects
     */
    std::vector<json> chunk(
        const ExtractionResult& extraction_result,
        int chunk_size,
        int overlap
    ) override;

    /**
     * @brief Generate a deterministic hash-based embedding for a text chunk
     *
     * Uses the same approach as HtmlProcessor so that Markdown chunks are
     * compatible with the same vector index as plain-text and HTML chunks.
     *
     * @param chunk_data  Plain text of the chunk
     * @return 768-dimensional embedding vector (L2-normalised)
     */
    std::vector<float> generateEmbedding(const std::string& chunk_data) override;

    std::string getName() const override { return "MarkdownProcessor"; }

    std::vector<ContentCategory> getSupportedCategories() const override {
        return {ContentCategory::TEXT};
    }

    // ----------------------------------------------------------------
    // Public static helpers (exposed for unit tests)
    // ----------------------------------------------------------------

    /**
     * @brief Parse YAML frontmatter from a Markdown document
     *
     * Reads the block between the opening `---` line and the closing `---`
     * or `...` line at the start of @p markdown.  Only simple scalar
     * key: value pairs and inline lists (key: [a, b]) are handled; nested
     * mappings are stored as raw strings.
     *
     * @param markdown  Raw Markdown string
     * @param body_out  Receives the remainder of the document after frontmatter
     * @return JSON object with parsed frontmatter fields (empty object if none)
     */
    static json parseFrontmatter(const std::string& markdown,
                                  std::string& body_out);

    /**
     * @brief Strip Markdown syntax, returning plain text
     *
     * Removes ATX headings prefixes, setext underlines, emphasis markers,
     * inline code, link/image syntax, blockquote markers, horizontal rules,
     * and table delimiters.  Code block contents are preserved verbatim
     * unless @p strip_code is true.
     *
     * @param markdown         Markdown text (frontmatter already removed)
     * @param preserve_headings  When true, retains "# ", "## ", … markers
     * @param strip_code       When true, removes fenced code block content
     * @return Plain text
     */
    static std::string stripMarkdown(const std::string& markdown,
                                      bool preserve_headings = false,
                                      bool strip_code = false);

private:
    Config config_;

    // Collapse runs of whitespace to a single space / newline
    static std::string normalizeWhitespace(const std::string& text);

    // Count whitespace-delimited tokens
    static int countTokens(const std::string& text);
};

/**
 * @brief Factory function for MarkdownProcessor
 *
 * @return Unique pointer to MarkdownProcessor with default configuration
 */
std::unique_ptr<IContentProcessor> createMarkdownProcessor();

/**
 * @brief Factory function for MarkdownProcessor with custom configuration
 *
 * @param config  Processor configuration
 * @return Unique pointer to MarkdownProcessor
 */
std::unique_ptr<IContentProcessor> createMarkdownProcessor(MarkdownProcessor::Config config);

} // namespace content
} // namespace themis
