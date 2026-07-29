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

/**
 * @brief Heading-aware Markdown chunk splitter producing `WikiChunk` objects.
 *
 * Chunks are bounded by `max_tokens` with a `overlap_tokens` trailing overlap
 * so adjacent chunks share context.  Section boundaries (ATX headings
 * `#`–`######`) always flush the current buffer and start a fresh section.
 *
 * Thread-safe: `split()` is a `const` method; the object holds no mutable
 * state after construction.
 */
class WikiChunkSplitter {
public:
    /**
     * @brief Construct a splitter with the given token window parameters.
     *
     * @param max_tokens      Hard upper bound on tokens per chunk (default 220).
     * @param overlap_tokens  Number of tokens carried over from the previous
     *                        chunk for contextual continuity (default 40).
     */
    explicit WikiChunkSplitter(int max_tokens     = 220,
                               int overlap_tokens = 40);

    /**
     * @brief Split Markdown `content` into `WikiChunk` objects.
     *
     * @param file_path  Source file path stored in `WikiChunk::source_path`
     *                   and used as part of the `chunk_id` hash input.
     * @param content    Full Markdown text of the document.
     * @return           Ordered list of chunks; empty if `content` is blank.
     */
    [[nodiscard]] std::vector<WikiChunk> split(
        const std::string& file_path,
        const std::string& content) const;

    /// @return Maximum tokens per chunk (constructor parameter).
    [[nodiscard]] int maxTokens()     const noexcept { return max_tokens_; }
    /// @return Overlap tokens between consecutive chunks.
    [[nodiscard]] int overlapTokens() const noexcept { return overlap_tokens_; }

private:
    /// @brief Count whitespace-separated `[A-Za-z0-9_\-]+` tokens in `text`.
    [[nodiscard]] static int countTokens(const std::string& text);

    /**
     * @brief Build a deterministic 12-char hex prefix + "-" + seq chunk ID.
     *
     * Uses FNV-1a 64-bit hash over `file_path + ":" + section_title + ":" +
     * std::to_string(seq)` formatted as 16 lowercase hex digits, then
     * truncated to 12.
     *
     * @param file_path     Source file path.
     * @param section_title Section heading text.
     * @param seq           Sequential index within the section.
     * @return              Chunk ID string, e.g. `"3f9a2b1c0d4e-0"`.
     */
    [[nodiscard]] static std::string makeChunkId(
        const std::string& file_path,
        const std::string& section_title,
        int                seq);

    /**
     * @brief Flush accumulated `lines` into chunks and append to `out`.
     *
     * Applies the sliding-window split: lines are grouped until `max_tokens_`
     * is reached, then a new group starts, retaining the last
     * `overlap_tokens_` tokens from the previous group.
     *
     * @param file_path     Source file path (for chunk_id and source_path).
     * @param section_title Current heading text (empty for preamble).
     * @param lines         Raw line strings accumulated in this section.
     * @param line_start    1-based line number of the first element in `lines`.
     * @param seq_base      Starting sequence counter for chunk_id generation.
     * @param out           Destination vector; chunks are appended.
     * @return              Updated sequence counter (first unused value).
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
