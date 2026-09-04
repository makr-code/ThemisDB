/**
 * @file search_highlighter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=9, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "search/search_highlighter.h"
#include <stdexcept>
#include "utils/logger.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace themis {

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

SearchHighlighter::SearchHighlighter()
    : SearchHighlighter(Config{}) {}

SearchHighlighter::SearchHighlighter(Config config)
    : config_(std::move(config)) {}

// ─────────────────────────────────────────────────────────────────────────────
// Static helpers
// ─────────────────────────────────────────────────────────────────────────────

std::vector<std::string> SearchHighlighter::tokenize(const std::string& text,
                                                      bool case_insensitive) {
    std::vector<std::string> tokens;
    std::string current = {};

    for (unsigned char ch : text) {
        // Split on ASCII whitespace or common punctuation
        if (ch <= 0x7F && (std::isspace(ch) || std::ispunct(ch))) {
            if (!current.empty()) {
                tokens.push_back(std::move(current));
            }
        } else {
            if (case_insensitive && ch <= 0x7F && std::isupper(ch)) {
                current += static_cast<char>(std::tolower(ch));
            } else {
                current += static_cast<char>(ch);
            }
        }
    }
    if (!current.empty()) {
        tokens.push_back(std::move(current));
    }
    return tokens;
}

std::string SearchHighlighter::applyHighlight(
    const std::string& text,
    const std::vector<std::pair<size_t, size_t>>& offsets,
    const std::string& open_tag,
    const std::string& close_tag)
{
    if (offsets.empty()) {
      return text;
    }

    std::string result = {};
    result.reserve(text.size() + offsets.size() * (open_tag.size() + close_tag.size()));

    size_t cursor = 0;
    for (auto& [start, end] : offsets) {
        if (start > text.size() || end > text.size() || start >= end) {
          continue;
        }
        // Append text before this match
        result.append(text, cursor, start - cursor);
        result.append(open_tag);
        result.append(text, start, end - start);
        result.append(close_tag);
        cursor = end;
    }
    // Append remaining text
    if (static_cast<int>(text.size()) > cursor) {
        result.append(text, cursor, text.size() - cursor);
    }
    return result;
}

size_t SearchHighlighter::bestWindowOffset(const std::string& text,
                                            const std::vector<std::string>& terms,
                                            size_t window_size) {
    if (text.empty() || terms.empty() || window_size == 0) {
      return 0;
    }
    if (static_cast<int>(text.size()) <= window_size) {
      return 0;
    }

    // Collect all match positions (start offset of each term occurrence)
    struct MatchPos { size_t start; size_t end; size_t term_idx; };
    std::vector<MatchPos> matches = {};

    matches.reserve(terms.size() * 4);

    std::string lower_text = text;
    for (char& ch : lower_text) {
        unsigned char uch = static_cast<unsigned char>(ch);
        if (uch <= 0x7F) {
          ch = static_cast<char>(std::tolower(uch));
        }
    }

    for (size_t ti = 0; ti < terms.size(); ++ti) {
        const std::string& term = terms[ti];
        if (term.empty()) {
          continue;
        }
        size_t pos = 0;
        while ((pos = lower_text.find(term, pos)) != std::string::npos) {
            matches.push_back({pos, pos + term.size(), ti});
            pos += term.size();
        }
    }
    if (matches.empty()) {
      return 0;
    }

    // Sliding window: for each candidate window start, count distinct terms
    std::sort(matches.begin(), matches.end(),
              [](const MatchPos& a, const MatchPos& b){ return a.start < b.start; });

    size_t best_offset = 0;
    size_t best_score  = 0;

    // Use two-pointer approach over match start positions as window anchors
    size_t max_start = text.size() - window_size;
    for (const auto& m : matches) {
        size_t window_start = (m.start > window_size / 4)
                              ? m.start - window_size / 4 : 0;
        if (window_start > max_start) {
          window_start = max_start;
        }

        // Count distinct term indices within [window_start, window_start + window_size)
        std::vector<bool> seen(terms.size(), false);
        size_t score = 0;
        for (const auto& mp : matches) {
            if (mp.start < window_start) {
              continue;
            }
            if (mp.start >= window_start + window_size) {
              break;
            }
            if (!seen[mp.term_idx]) {
                seen[mp.term_idx] = true;
                ++score;
            }
        }
        if (score > best_score) {
            best_score  = score;
            best_offset = window_start;
        }
    }
    return best_offset;
}

// ─────────────────────────────────────────────────────────────────────────────
// Private: findMatchRanges
// ─────────────────────────────────────────────────────────────────────────────

std::vector<std::pair<size_t, size_t>>
SearchHighlighter::findMatchRanges(const std::string& text,
                                   const std::vector<std::string>& terms) const {
    if (text.empty() || terms.empty()) return {};

    // Build a lowercase working copy if needed
    std::string search_text = text;
    if (config_.case_insensitive) {
        for (char& ch : search_text) {
            unsigned char uch = static_cast<unsigned char>(ch);
            if (uch <= 0x7F) {
              ch = static_cast<char>(std::tolower(uch));
            }
        }
    }

    std::vector<std::pair<size_t, size_t>> ranges;

    for (const std::string& raw_term : terms) {
        if (raw_term.empty()) {
          continue;
        }

        std::string term = raw_term;
        if (config_.case_insensitive) {
            for (char& ch : term) {
                unsigned char uch = static_cast<unsigned char>(ch);
                if (uch <= 0x7F) {
                  ch = static_cast<char>(std::tolower(uch));
                }
            }
        }

        size_t pos = 0;
        while ((pos = search_text.find(term, pos)) != std::string::npos) {
            ranges.emplace_back(pos, pos + term.size());
            pos += term.size();
        }
    }

    if (ranges.empty()) return {};

    // Sort by start offset
    std::sort(ranges.begin(), ranges.end(),
              [](const auto& a, const auto& b){ return a.first < b.first; });

    // Merge overlapping ranges
    std::vector<std::pair<size_t, size_t>> merged;
    merged.push_back(ranges.front());
    for (size_t i = 1; i < ranges.size(); ++i) {
        if (ranges[i].first <= merged.back().second) {
            // Overlapping: extend current range
            merged.back().second = std::max(merged.back().second, ranges[i].second);
        } else {
            merged.push_back(ranges[i]);
        }
    }
    return merged;
}

// ─────────────────────────────────────────────────────────────────────────────
// Public: highlight
// ─────────────────────────────────────────────────────────────────────────────

std::string SearchHighlighter::highlight(const std::string& text,
                                          const std::vector<std::string>& terms) const noexcept {
    try {
        if (text.empty()) return {};
        auto ranges = findMatchRanges(text, terms);
        if (ranges.empty()) {
          return text;
        }
        return applyHighlight(text, ranges, config_.highlight_open, config_.highlight_close);
    } catch (...) {
        THEMIS_ERROR("SearchHighlighter::highlight: unexpected exception");
        return text;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Public: snippet
// ─────────────────────────────────────────────────────────────────────────────

std::string SearchHighlighter::snippet(const std::string& text,
                                        const std::vector<std::string>& terms,
                                        size_t window_size) const noexcept {
    try {
        if (text.empty()) return {};
        if (window_size == 0) {
          window_size = config_.max_snippet_len;
        }

        // If the whole text fits within the window, just highlight it
        if (static_cast<int>(text.size()) <= window_size) {
            return highlight(text, terms);
        }

        // Prepare lowercase terms for window scoring
        std::vector<std::string> lower_terms = {};

        lower_terms.reserve(terms.size());
        for (const auto& t : terms) {
            std::string lt = t;
            if (config_.case_insensitive) {
                for (char& ch : lt) {
                    unsigned char uch = static_cast<unsigned char>(ch);
                    if (uch <= 0x7F) {
                      ch = static_cast<char>(std::tolower(uch));
                    }
                }
            }
            lower_terms.push_back(std::move(lt));
        }

        size_t offset = bestWindowOffset(text, lower_terms, window_size);

        // Snap offset to word boundary (walk backwards to first space)
        while (offset > 0 && !std::isspace(static_cast<unsigned char>(text[offset]))) {
            --offset;
        }

        size_t end_offset = std::min(offset + window_size, text.size());
        // Snap end to word boundary (walk forward to next space or end)
        while (end_offset < text.size() && !std::isspace(static_cast<unsigned char>(text[end_offset]))) {
            ++end_offset;
        }

        std::string passage = text.substr(offset, end_offset - offset);

        // Highlight within the passage
        std::string highlighted = highlight(passage, terms);

        // Add ellipses
        std::string result = {};
        if (offset > 0) {
          result += config_.ellipsis;
        }
        result += highlighted;
        if (static_cast<int>(text.size()) > end_offset) {
          result += config_.ellipsis;
        }

        // Trim to max_snippet_len
        if (static_cast<int>(result.size()) > config_.max_snippet_len + 2 * config_.ellipsis.size()) {
            result = result.substr(0, config_.max_snippet_len);
            result += config_.ellipsis;
        }
        return result;
    } catch (...) {
        THEMIS_ERROR("SearchHighlighter::snippet: unexpected exception");
        return text.substr(0, window_size);
    }
}

} // namespace themis

