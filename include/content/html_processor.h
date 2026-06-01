/*
 * ThemisDB | File: html_processor.h | Version: 0.0.15 | Last Modified: 2026-05-20 19:53:17
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 201
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #3012 [content] HTML content extr... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// Copyright (c) 2024 ThemisDB
// SPDX-License-Identifier: MIT

/**
 * @file html_processor.h
 * @brief HTML Content Processor for ThemisDB
 *
 * Extracts plain text from HTML documents with boilerplate removal.
 * Strips navigation, headers, footers, scripts, and styles.
 * Extracts metadata from <title> and <meta> tags.
 *
 * Pure C++ implementation — no external HTML parsing library required.
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
 * @brief HTML Content Processor
 *
 * Handles HTML documents:
 * - Boilerplate removal (<nav>, <header>, <footer>, <aside>, <script>, <style>, <form>)
 * - Plain text extraction preserving semantic structure (headings, paragraphs)
 * - Metadata extraction (<title>, <meta name="description"/"keywords"/"author">)
 * - HTML entity decoding (`&amp;`, `<`, `>`, `&quot;`, `&#NNN;`, `&#xHH;`)
 * - Chunking by paragraph / heading boundary
 */
class HtmlProcessor : public IContentProcessor {
public:
    /**
     * @brief Configuration for HTML processing
     */
    struct Config {
         /// Remove <nav>, <header>, <footer>, <aside>, <form> boilerplate blocks
        bool remove_boilerplate = true;
         /// Strip <script> and <style> elements and their content
        bool remove_scripts_styles = true;
        /// Decode HTML entities in extracted text
        bool decode_entities = true;
        /// Preserve heading levels as text markers (e.g. "# Title\n")
        bool preserve_heading_markers = false;
        /// Max characters of extracted text (0 = unlimited)
        size_t max_text_length = 0;
    };

    HtmlProcessor();
    explicit HtmlProcessor(Config config);
    ~HtmlProcessor() override = default;

    /**
     * @brief Extract plain text and metadata from an HTML document
     *
     * @param blob  Raw HTML bytes
     * @param content_type  Content type info (mime_type should be "text/html")
     * @return ExtractionResult with plain text in `.text` and metadata in `.metadata`
     */
    ExtractionResult extract(
        const std::string& blob,
        const ContentType& content_type
    ) override;

    /**
     * @brief Chunk extracted HTML text for RAG / search
     *
     * Splits on paragraph / heading boundaries, then falls back to token-count
     * windows when individual paragraphs exceed chunk_size.
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
     * @brief Generate a simple hash-based embedding for a text chunk
     *
     * Uses the same deterministic approach as TextProcessor so that HTML chunks
     * are compatible with the same vector index as plain-text chunks.
     *
     * @param chunk_data  Plain text of the chunk
     * @return 768-dimensional embedding vector (L2-normalised)
     */
    std::vector<float> generateEmbedding(const std::string& chunk_data) override;

    std::string getName() const override { return "HtmlProcessor"; }

    std::vector<ContentCategory> getSupportedCategories() const override {
        return {ContentCategory::TEXT};
    }

    // ----------------------------------------------------------------
    // Public static helpers (exposed for unit tests)
    // ----------------------------------------------------------------

    /**
     * @brief Remove boilerplate HTML blocks
     *
      * Strips the full content of <nav>, <header>, <footer>, <aside>, and <form>
     * elements (including nested tags) from @p html.
     *
     * @param html  Raw HTML string
     * @return HTML with boilerplate sections removed
     */
    static std::string removeBoilerplate(const std::string& html);

    /**
      * @brief Remove <script> and <style> elements and their content
     *
     * @param html  Raw HTML string
     * @return HTML with script/style elements removed
     */
    static std::string removeScriptsAndStyles(const std::string& html);

    /**
     * @brief Strip all remaining HTML tags, returning plain text
     *
     * Inserts whitespace at block-level tag boundaries so that words
     * from adjacent elements are not concatenated.
     *
     * @param html  HTML string (should have scripts/styles already removed)
     * @param preserve_headings  When true, replaces <h1>–<h6>
     *                           opening tags with markdown-style markers
     *                           ("# ", "## ", …"###### ") before stripping.
     * @return Plain text
     */
    static std::string stripTags(const std::string& html,
                                  bool preserve_headings = false);

    /**
     * @brief Decode common HTML entities
     *
      * Handles named entities (`&amp;` `<` `>` `&quot;` `&apos;` `&nbsp;`),
      * decimal references (`&#NNN;`), and hex references (`&#xHH;`).
     *
     * @param text  Text that may contain HTML entities
     * @return Decoded text
     */
    static std::string decodeEntities(const std::string& text);

    /**
      * @brief Extract <title> and <meta> tag values from an HTML document
     *
     * @param html  Raw HTML string
     * @return JSON object with keys: title, description, keywords, author
     */
    static json extractMetaTags(const std::string& html);

private:
    Config config_;

    // Collapse runs of whitespace to a single space / newline
    static std::string normalizeWhitespace(const std::string& text);

    // Count whitespace-delimited tokens
    static int countTokens(const std::string& text);

    // Remove the full content of a block-level element by tag name
    // (handles nested elements of the same name)
    static std::string removeElement(const std::string& html, const std::string& tag);
};

/**
 * @brief Factory function for HtmlProcessor
 *
 * @return Unique pointer to HtmlProcessor with default configuration
 */
std::unique_ptr<IContentProcessor> createHtmlProcessor();

/**
 * @brief Factory function for HtmlProcessor with custom configuration
 *
 * @param config  Processor configuration
 * @return Unique pointer to HtmlProcessor
 */
std::unique_ptr<IContentProcessor> createHtmlProcessor(HtmlProcessor::Config config);

} // namespace content
} // namespace themis

