/**
 * @file wiki_chunk_splitter.cpp
 * @brief Heading-aware Markdown chunk splitter for the LLM Wiki index.
 *
 * Implements `WikiChunkSplitter::split()` using a two-pass line scan:
 *  1. Lines are accumulated under the current section heading.
 *  2. When a section boundary is detected the buffer is flushed via
 *     `flushSection()`, which applies a sliding-window split with overlap.
 *
 * Chunk IDs are stable within a process run: they are derived from a
 * FNV-1a 64-bit hash of `(file_path + ":" + section_title + ":" + seq)`.
 *
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 */

#include "llm/wiki_chunk_splitter.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace llm {

// ─────────────────────────────────────────────────────────────────────────────
// FNV-1a 64-bit helper (deterministic, no external dependency)
// ─────────────────────────────────────────────────────────────────────────────
namespace {

constexpr std::uint64_t kFnvOffset = 14695981039346656037;
constexpr std::uint64_t kFnvPrime  = 1099511628211;

[[nodiscard]] std::uint64_t fnv1a64(const std::string& s) noexcept {
    std::uint64_t h = kFnvOffset;
    for (unsigned char c : s) {
        h ^= static_cast<std::uint64_t>(c);
        h *= kFnvPrime;
    }
    return h;
}

/// Format the lower 48 bits (12 hex chars) of `v`.
[[nodiscard]] std::string hexPrefix12(std::uint64_t v) {
    std::ostringstream oss = {};
    oss << std::hex << std::setfill('0') << std::setw(12) << (v & 0x0000'FFFF'FFFF'FFFFULL);
    return oss.str();
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

WikiChunkSplitter::WikiChunkSplitter(int max_tokens, int overlap_tokens)
    : max_tokens_(max_tokens > 0 ? max_tokens : 220)
    , overlap_tokens_(overlap_tokens >= 0 ? overlap_tokens : 40)
    , heading_re_(R"(^#{1,6}\s+)", std::regex::ECMAScript)
    , token_re_(R"([A-Za-z0-9_\-]+)", std::regex::ECMAScript)
{
    if (overlap_tokens_ >= max_tokens_) {
        overlap_tokens_ = max_tokens_ / 4;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Token counting
// ─────────────────────────────────────────────────────────────────────────────

int WikiChunkSplitter::countTokens(const std::string& text) {
    static const std::regex tok_re(R"([A-Za-z0-9_\-]+)", std::regex::ECMAScript);
    auto begin = std::sregex_iterator(text.begin(), text.end(), tok_re);
    auto end   = std::sregex_iterator();
    return static_cast<int>(std::distance(begin, end));
}

// ─────────────────────────────────────────────────────────────────────────────
// Chunk ID generation
// ─────────────────────────────────────────────────────────────────────────────

std::string WikiChunkSplitter::makeChunkId(const std::string& file_path,
                                            const std::string& section_title,
                                            int                seq) {
    const std::string key = file_path + ":" + section_title + ":" + std::to_string(seq);
    return hexPrefix12(fnv1a64(key)) + "-" + std::to_string(seq);
}

// ─────────────────────────────────────────────────────────────────────────────
// Section flusher (sliding-window split)
// ─────────────────────────────────────────────────────────────────────────────

int WikiChunkSplitter::flushSection(const std::string&              file_path,
                                     const std::string&              section_title,
                                     const std::vector<std::string>& lines,
                                     int                             line_start,
                                     int                             seq_base,
                                     std::vector<WikiChunk>&         out) const {
    if (lines.empty()) {
        return seq_base;
    }

    int seq = seq_base;

    // Tokenise every line once for efficient sliding-window accounting
    // line_tokens[i] contains the token count for lines[i].
    std::vector<int> line_tokens(lines.size());
    for (std::size_t i = 0; i < lines.size(); ++i) {
        line_tokens[i] = countTokens(lines[i]);
    }

    // Build chunks using greedy window accumulation.
    // window_start / window_end are indices into `lines`.
    std::size_t window_start = 0;
    int running_tokens = 0;
    std::size_t chunk_start = 0;

    auto emit = [&](std::size_t from, std::size_t to) {
        // to is exclusive
        std::string text = {};
        for (std::size_t k = from; k < to; ++k) {
            if (!text.empty()) {
              text += '\n';
            }
            text += lines[k];
        }
        WikiChunk c;
        c.chunk_id      = makeChunkId(file_path, section_title, seq++);
        c.doc_id        = file_path;
        c.section_title = section_title;
        c.line_start    = line_start + static_cast<int>(from);
        c.line_end      = line_start + static_cast<int>(to) - 1;
        c.text          = std::move(text);
        c.source_path   = file_path;
        out.push_back(std::move(c));
    };

    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (running_tokens + line_tokens[i] > max_tokens_ && i > chunk_start) {
            // Flush [chunk_start, i)
            emit(chunk_start, i);

            // Compute overlap: walk backwards from i until we've consumed
            // overlap_tokens_ worth of tokens, then restart from there.
            int overlap_acc = 0;
            std::size_t new_start = i;
            while (new_start > chunk_start && overlap_acc < overlap_tokens_) {
                --new_start;
                overlap_acc += line_tokens[new_start];
            }
            chunk_start    = new_start;
            running_tokens = overlap_acc;
        }
        running_tokens += line_tokens[i];
    }
    // Flush remaining
    if (static_cast<int>(lines.size()) > chunk_start) {
        emit(chunk_start, lines.size());
    }

    (void)window_start; // suppress unused warning
    return seq;
}

// ─────────────────────────────────────────────────────────────────────────────
// Public split()
// ─────────────────────────────────────────────────────────────────────────────

std::vector<WikiChunk> WikiChunkSplitter::split(const std::string& file_path,
                                                 const std::string& content) const {
    if (content.empty()) {
        return {};
    }

    std::vector<WikiChunk> result;

    // Split content into lines
    std::vector<std::string> all_lines;
    {
        std::istringstream ss(content);
        std::string line = {};
        while (std::getline(ss, line)) {
            all_lines.push_back(std::move(line));
        }
    }

    std::string              current_title;      // heading text
    std::vector<std::string> section_lines;      // buffered lines
    int                      section_line_start = 1; // 1-based
    int                      seq = 0;

    for (std::size_t i = 0; i < all_lines.size(); ++i) {
        const std::string& line = all_lines[i];

        // Check if this line is an ATX heading
        if (std::regex_search(line, heading_re_)) {
            // Flush accumulated section
            seq = flushSection(file_path, current_title,
                               section_lines, section_line_start,
                               seq, result);
            section_lines.clear();
            section_line_start = static_cast<int>(i) + 1; // next line is content

            // Extract heading text (strip leading #+ and whitespace)
            current_title = std::regex_replace(line,
                                               std::regex(R"(^#{1,6}\s*)"),
                                               "");
            // Don't add the heading line itself to section_lines
        } else {
            section_lines.push_back(line);
        }
    }

    // Flush final section
    flushSection(file_path, current_title,
                 section_lines, section_line_start,
                 seq, result);

    return result;
}

} // namespace llm
} // namespace themis
