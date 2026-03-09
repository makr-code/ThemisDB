/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            search_highlighter.cpp                             ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-09                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     200                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "search/search_highlighter.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <vector>

namespace themis {

// ============================================================================
// Construction / configuration
// ============================================================================

SearchHighlighter::SearchHighlighter(const Config& config) : config_(config) {
    if (config_.window_size == 0) {
        throw std::invalid_argument("SearchHighlighter: window_size must be > 0");
    }
    if (config_.max_window_size == 0) {
        throw std::invalid_argument("SearchHighlighter: max_window_size must be > 0");
    }
    if (config_.window_size > config_.max_window_size) {
        config_.window_size = config_.max_window_size;
    }
}

void SearchHighlighter::setConfig(const Config& config) {
    if (config.window_size == 0) {
        throw std::invalid_argument("SearchHighlighter: window_size must be > 0");
    }
    if (config.max_window_size == 0) {
        throw std::invalid_argument("SearchHighlighter: max_window_size must be > 0");
    }
    config_ = config;
    if (config_.window_size > config_.max_window_size) {
        config_.window_size = config_.max_window_size;
    }
}

// ============================================================================
// Static helpers
// ============================================================================

std::unordered_set<std::string> SearchHighlighter::tokenize(const std::string& query) {
    std::unordered_set<std::string> terms;
    std::string token;
    for (unsigned char c : query) {
        if (std::isalnum(c)) {
            token += static_cast<char>(std::tolower(c));
        } else {
            if (!token.empty()) {
                terms.insert(std::move(token));
                token.clear();
            }
        }
    }
    if (!token.empty()) {
        terms.insert(std::move(token));
    }
    return terms;
}

std::unordered_set<std::string> SearchHighlighter::toLowerSet(
    const std::vector<std::string>& terms) {
    std::unordered_set<std::string> result;
    for (const auto& t : terms) {
        std::string lower;
        lower.reserve(t.size());
        for (unsigned char c : t) {
            lower += static_cast<char>(std::tolower(c));
        }
        if (!lower.empty()) {
            result.insert(std::move(lower));
        }
    }
    return result;
}

std::string SearchHighlighter::applyHighlight(
    const std::string& text,
    const std::unordered_set<std::string>& terms,
    const std::string& open_tag,
    const std::string& close_tag) {

    if (terms.empty() || text.empty()) return text;

    // Build a lower-case shadow for case-insensitive scanning.
    std::string lower(text.size(), '\0');
    std::transform(text.begin(), text.end(), lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    std::string result;
    result.reserve(text.size() + 64);
    size_t i = 0;

    while (i < text.size()) {
        if (!std::isalnum(static_cast<unsigned char>(text[i]))) {
            result += text[i++];
            continue;
        }
        // Find end of current alnum run (word).
        size_t end = i;
        while (end < text.size() &&
               std::isalnum(static_cast<unsigned char>(text[end]))) {
            ++end;
        }
        const std::string word = lower.substr(i, end - i);
        if (terms.count(word)) {
            result += open_tag;
            result.append(text, i, end - i);
            result += close_tag;
        } else {
            result.append(text, i, end - i);
        }
        i = end;
    }
    return result;
}

size_t SearchHighlighter::bestWindowOffset(
    const std::string& lower_text,
    const std::unordered_set<std::string>& terms,
    size_t window_size) {

    if (lower_text.size() <= window_size) return 0;

    // Collect byte offsets of all term match starts.
    std::vector<size_t> positions;
    size_t i = 0;
    while (i < lower_text.size()) {
        if (!std::isalnum(static_cast<unsigned char>(lower_text[i]))) {
            ++i;
            continue;
        }
        size_t end = i;
        while (end < lower_text.size() &&
               std::isalnum(static_cast<unsigned char>(lower_text[end]))) {
            ++end;
        }
        if (terms.count(lower_text.substr(i, end - i))) {
            positions.push_back(i);
        }
        i = end;
    }

    if (positions.empty()) return 0;

    // Sliding-window: maximise term density inside window_size bytes.
    size_t best_start = 0;
    size_t best_count = 0;
    size_t lo = 0;
    for (size_t hi = 0; hi < positions.size(); ++hi) {
        while (positions[hi] - positions[lo] >= window_size) ++lo;
        size_t count = hi - lo + 1;
        if (count > best_count) {
            best_count = count;
            // Centre the window around the match cluster when possible.
            size_t mid = (positions[lo] + positions[hi]) / 2;
            best_start = mid > window_size / 2 ? mid - window_size / 2 : 0;
        }
    }

    // The centered best_start already positions the match inside the window.
    // No walk-back is performed: aligning to a word boundary risks pushing
    // the match outside the window when best_start falls inside a long alnum run.
    return best_start;
}

// ============================================================================
// highlight() overloads
// ============================================================================

std::string SearchHighlighter::highlight(
    const std::string& text,
    const std::unordered_set<std::string>& terms) const {
    return applyHighlight(text, terms, config_.open_tag, config_.close_tag);
}

std::string SearchHighlighter::highlight(
    const std::string& text,
    const std::string& query) const {
    return highlight(text, tokenize(query));
}

std::string SearchHighlighter::highlight(
    const std::string& text,
    const std::vector<std::string>& terms) const {
    return highlight(text, toLowerSet(terms));
}

// ============================================================================
// snippet() overloads
// ============================================================================

std::string SearchHighlighter::snippet(
    const std::string& text,
    const std::unordered_set<std::string>& terms,
    size_t window_size) const {

    if (text.empty()) return {};

    const size_t win = (window_size > 0) ? window_size : config_.window_size;

    if (text.size() <= win) {
        return applyHighlight(text, terms, config_.open_tag, config_.close_tag);
    }

    // Build lower-case shadow for position scanning.
    std::string lower(text.size(), '\0');
    std::transform(text.begin(), text.end(), lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    const size_t start    = bestWindowOffset(lower, terms, win);
    const bool trunc_left  = (start > 0);
    const bool trunc_right = (start + win < text.size());

    const std::string excerpt  = text.substr(start, win);
    const std::string hl       = applyHighlight(
        excerpt, terms, config_.open_tag, config_.close_tag);

    std::string result;
    if (trunc_left)  result += config_.separator;
    result += hl;
    if (trunc_right) result += config_.separator;
    return result;
}

std::string SearchHighlighter::snippet(
    const std::string& text,
    const std::string& query,
    size_t window_size) const {
    return snippet(text, tokenize(query), window_size);
}

std::string SearchHighlighter::snippet(
    const std::string& text,
    const std::vector<std::string>& terms,
    size_t window_size) const {
    return snippet(text, toLowerSet(terms), window_size);
}

} // namespace themis
