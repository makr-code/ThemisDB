/**
 * @file wiki_chunk_splitter.h
 * @brief Heading-aware Markdown chunk splitter for the LLM Wiki index.
 *
 * `WikiChunkSplitter` parses Markdown source text into `WikiChunk` objects
 * using a heading-aware sliding-window strategy:
 *
 *  1. Lines are accumulated under the nearest ancestor heading.
 *  2. When a section exceeds `max_tokens`, the buffer is flushed into chunks
 *     of at most `max_tokens` tokens with an `overlap_tokens` sliding tail.
 *  3. Each chunk receives a stable `chunk_id` derived from a FNV-64 hash of
 *     `(file_path + ":" + section_title + ":" + seq_idx)`.
 *
 * Token counting uses the same `[A-Za-z0-9_\-]+` regex as the Python MVP.
 *
 * ## Example
 * @code
 *   WikiChunkSplitter splitter(220, 40);
 *   auto chunks = splitter.split("docs/arch.md", content);
 *   // chunks[0].chunk_id == "3f9a2b1c0d4e-0"  (deterministic)
 * @endcode
 *
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 */

#pragma once

#include "llm/wiki_index_store.h"

#include <regex>
#include <string>
#include <vector>
#include <cstdint>

namespace themis {
namespace llm {

class WikiChunkSplitter {
public:
    explicit WikiChunkSplitter(int max_tokens     = 220,
                               int overlap_tokens = 40);

    [[nodiscard]] std::vector<WikiChunk> split(
        const std::string& file_path,
        const std::string& content) const;

    [[nodiscard]] int maxTokens()     const noexcept { return max_tokens_; }
    [[nodiscard]] int overlapTokens() const noexcept { return overlap_tokens_; }

private:
    [[nodiscard]] static int countTokens(const std::string& text);

    [[nodiscard]] static std::string makeChunkId(
        const std::string& file_path,
        const std::string& section_title,
        int                seq);

    /**
     * @brief Flush Section.
     * @param[in] file_path Path to the file.
     * @param[in] section_title Input parameter.
     * @param[in] lines Input parameter.
     * @param[in] line_start Input parameter.
     * @param[in] seq_base Input parameter.
     * @param[in,out] out Input/output parameter.
     * @return Return value.
     */
    int flushSection(const std::string&        file_path,
                     const std::string&        section_title,
                     const std::vector<std::string>& lines,
                     int                       line_start,
                     int                       seq_base,
                     std::vector<WikiChunk>&   out) const;

    int          max_tokens_;     ///< Token hard cap per chunk
    int          overlap_tokens_; ///< Overlap token budget
    std::regex   heading_re_;     ///< ATX heading detector `^#{1,6}\s+`
    std::regex   token_re_;       ///< Token extractor `[A-Za-z0-9_\-]+`
};

} // namespace llm
} // namespace themis
