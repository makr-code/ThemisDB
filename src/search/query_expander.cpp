/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            query_expander.cpp                                 ║
  Version:         0.0.28                                             ║
  Last Modified:   2026-02-23 03:58:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     420                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "search/query_expander.h"
#include "utils/logger.h"
#include <algorithm>
#include <cctype>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace themis {

// ============================================================================
// Construction
// ============================================================================

QueryExpander::QueryExpander(const Config& config) : config_(config) {
    if (config_.max_edit_distance < 0) {
        throw std::invalid_argument("QueryExpander: max_edit_distance must be >= 0");
    }
    if (config_.max_expansions == 0) {
        throw std::invalid_argument("QueryExpander: max_expansions must be > 0");
    }
    THEMIS_DEBUG("QueryExpander initialized (synonyms={}, spelling={}, max_exp={})",
                 config_.use_synonyms, config_.correct_spelling, config_.max_expansions);
}

// ============================================================================
// Dictionary management
// ============================================================================

void QueryExpander::addSynonyms(const std::string& term,
                                 const std::vector<std::string>& synonyms) {
    const std::string key = toLower(term);
    auto& entry = synonyms_[key];
    for (const auto& syn : synonyms) {
        std::string s = toLower(syn);
        // Avoid duplicates and self-references
        if (s != key && std::find(entry.begin(), entry.end(), s) == entry.end()) {
            entry.push_back(s);
        }
    }
}

void QueryExpander::addVocabulary(const std::vector<std::string>& words) {
    for (const auto& w : words) {
        vocabulary_.insert(toLower(w));
    }
}

// ============================================================================
// Core operations
// ============================================================================

ExpandedQuery QueryExpander::expand(const std::string& query) const {
    ExpandedQuery result;
    result.original = query;

    auto tokens = tokenize(query);
    if (tokens.empty()) {
        return result;
    }

    // Step 1: Spelling correction (per-token)
    std::vector<std::string> corrected_tokens;
    bool any_correction = false;
    for (const auto& tok : tokens) {
        std::string c = config_.correct_spelling ? correctSpelling(tok) : tok;
        if (c != tok) any_correction = true;
        corrected_tokens.push_back(c);
    }
    if (any_correction) {
        std::ostringstream oss;
        for (size_t i = 0; i < corrected_tokens.size(); ++i) {
            if (i) oss << ' ';
            oss << corrected_tokens[i];
        }
        result.corrected = oss.str();
    }

    // Step 2: Synonym expansion (on corrected tokens)
    const auto& work_tokens = any_correction ? corrected_tokens : tokens;
    std::vector<std::string> added_synonyms;

    if (config_.use_synonyms) {
        for (const auto& tok : work_tokens) {
            auto it = synonyms_.find(tok);
            if (it == synonyms_.end()) continue;
            for (const auto& syn : it->second) {
                if (added_synonyms.size() >= config_.max_expansions) break;
                // Deduplicate across all added and original
                if (std::find(work_tokens.begin(), work_tokens.end(), syn) == work_tokens.end() &&
                    std::find(added_synonyms.begin(), added_synonyms.end(), syn) == added_synonyms.end()) {
                    added_synonyms.push_back(syn);
                }
            }
            if (added_synonyms.size() >= config_.max_expansions) break;
        }
        result.synonyms = added_synonyms;
    }

    // Step 3: Build all_terms = original (corrected) tokens + synonyms
    result.all_terms = work_tokens;
    for (const auto& syn : added_synonyms) {
        // For multi-word phrases, add the whole phrase; for single words add as token
        result.all_terms.push_back(syn);
    }

    // Step 4: Zero-result relaxation
    result.relaxed_terms = work_tokens;
    if (!result.relaxed_terms.empty()) {
        result.relaxed_terms.pop_back(); // drop last (least specific) token
    }

    THEMIS_DEBUG("QueryExpander::expand('{}') -> {} synonyms, corrected='{}'",
                 query, added_synonyms.size(),
                 result.corrected.empty() ? "(none)" : result.corrected);

    return result;
}

std::string QueryExpander::correctSpelling(const std::string& word) const {
    if (!config_.correct_spelling || vocabulary_.empty()) {
        return word;
    }
    const std::string lower = toLower(word);
    // If it's already in the vocabulary, no correction needed
    if (vocabulary_.count(lower)) {
        return word;
    }
    // Find the vocabulary word closest in edit distance
    std::string best = word;
    int best_dist = std::numeric_limits<int>::max();
    for (const auto& vocab_word : vocabulary_) {
        int d = editDistance(lower, vocab_word);
        if (d < best_dist && d <= config_.max_edit_distance) {
            best_dist = d;
            best = vocab_word;
        }
    }
    return best;
}

std::vector<SpellingCorrection> QueryExpander::suggestSpellingCorrections(
    const std::string& word,
    size_t max_suggestions) const {

    if (!config_.correct_spelling || vocabulary_.empty() || max_suggestions == 0) {
        return {};
    }
    const std::string lower = toLower(word);
    // If the word is already correct, no suggestions needed
    if (vocabulary_.count(lower)) {
        return {};
    }

    // Collect all candidates within max_edit_distance
    std::vector<SpellingCorrection> candidates;
    for (const auto& vocab_word : vocabulary_) {
        int d = editDistance(lower, vocab_word);
        if (d <= config_.max_edit_distance) {
            SpellingCorrection sc;
            sc.suggestion = vocab_word;
            sc.edit_distance = d;
            candidates.push_back(sc);
        }
    }

    if (candidates.empty()) {
        return {};
    }

    // Sort by edit distance ascending, then alphabetically for stability
    std::sort(candidates.begin(), candidates.end(),
              [](const SpellingCorrection& a, const SpellingCorrection& b) {
                  if (a.edit_distance != b.edit_distance) {
                      return a.edit_distance < b.edit_distance;
                  }
                  return a.suggestion < b.suggestion;
              });

    // Assign confidence scores: linearly decaying with edit distance
    // distance 0 → 1.0, distance max_edit_distance → 1/(max_edit_distance+1)
    const double max_d = static_cast<double>(config_.max_edit_distance);
    for (auto& sc : candidates) {
        sc.confidence = 1.0 - (static_cast<double>(sc.edit_distance) / (max_d + 1.0));
    }

    if (candidates.size() > max_suggestions) {
        candidates.resize(max_suggestions);
    }

    THEMIS_DEBUG("QueryExpander::suggestSpellingCorrections('{}') -> {} candidates",
                 word, candidates.size());
    return candidates;
}

std::vector<SpellingCorrection> QueryExpander::suggestQueryCorrections(
    const std::string& query,
    size_t max_suggestions) const {

    if (!config_.correct_spelling || vocabulary_.empty() || max_suggestions == 0) {
        return {};
    }

    auto tokens = tokenize(query);
    if (tokens.empty()) {
        return {};
    }

    // For each token, get its top correction (if any)
    bool any_token_needs_correction = false;
    std::vector<std::string> best_corrections(tokens.size());
    std::vector<int> best_distances(tokens.size(), 0);

    for (size_t i = 0; i < tokens.size(); ++i) {
        const std::string lower = toLower(tokens[i]);
        if (vocabulary_.count(lower)) {
            best_corrections[i] = tokens[i];
        } else {
            auto candidates = suggestSpellingCorrections(tokens[i], 1);
            if (!candidates.empty()) {
                best_corrections[i] = candidates[0].suggestion;
                best_distances[i]   = candidates[0].edit_distance;
                any_token_needs_correction = true;
            } else {
                best_corrections[i] = tokens[i];
            }
        }
    }

    if (!any_token_needs_correction) {
        return {};
    }

    // Build full-query suggestions: substitute each corrected token one at a time,
    // then produce an "all corrected" variant.
    std::vector<SpellingCorrection> results;
    std::unordered_set<std::string> seen;

    auto addSuggestion = [&](const std::vector<std::string>& parts, int total_dist) {
        std::ostringstream oss;
        for (size_t i = 0; i < parts.size(); ++i) {
            if (i) oss << ' ';
            oss << parts[i];
        }
        std::string full = oss.str();
        if (full == query || seen.count(full)) return;
        seen.insert(full);
        SpellingCorrection sc;
        sc.suggestion   = full;
        sc.edit_distance = total_dist;
        const double max_d = static_cast<double>(config_.max_edit_distance);
        sc.confidence = std::max(0.0, 1.0 - (static_cast<double>(total_dist) / (max_d + 1.0)));
        results.push_back(sc);
    };

    // Per-token substitution variants
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (best_distances[i] == 0) continue;
        std::vector<std::string> parts = tokens;
        parts[i] = best_corrections[i];
        addSuggestion(parts, best_distances[i]);
    }

    // All-corrected variant
    {
        int total = 0;
        for (int d : best_distances) total += d;
        if (total > 0) {
            addSuggestion(best_corrections, total);
        }
    }

    // Sort by total edit distance ascending
    std::sort(results.begin(), results.end(),
              [](const SpellingCorrection& a, const SpellingCorrection& b) {
                  if (a.edit_distance != b.edit_distance) {
                      return a.edit_distance < b.edit_distance;
                  }
                  return a.suggestion < b.suggestion;
              });

    if (results.size() > max_suggestions) {
        results.resize(max_suggestions);
    }

    THEMIS_DEBUG("QueryExpander::suggestQueryCorrections('{}') -> {} suggestions",
                 query, results.size());
    return results;
}

std::vector<std::string> QueryExpander::suggestAlternatives(const std::string& query) const {
    auto tokens = tokenize(query);
    std::vector<std::string> alternatives;
    if (tokens.empty() || !config_.use_synonyms) {
        return alternatives;
    }

    for (const auto& tok : tokens) {
        auto it = synonyms_.find(tok);
        if (it == synonyms_.end()) continue;
        for (const auto& syn : it->second) {
            if (alternatives.size() >= config_.max_expansions) break;
            // Build a variant of the query with this token replaced by its synonym
            std::ostringstream oss;
            bool first = true;
            for (const auto& t : tokens) {
                if (!first) oss << ' ';
                oss << (t == tok ? syn : t);
                first = false;
            }
            std::string alt = oss.str();
            if (std::find(alternatives.begin(), alternatives.end(), alt) == alternatives.end()) {
                alternatives.push_back(alt);
            }
        }
        if (alternatives.size() >= config_.max_expansions) break;
    }
    return alternatives;
}

std::string QueryExpander::relaxQuery(const std::string& query) const {
    auto tokens = tokenize(query);
    if (tokens.size() <= 1) {
        return {}; // Cannot relax a single-token (or empty) query
    }
    tokens.pop_back();
    std::ostringstream oss;
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (i) oss << ' ';
        oss << tokens[i];
    }
    return oss.str();
}

// ============================================================================
// Private helpers
// ============================================================================

std::vector<std::string> QueryExpander::tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::istringstream iss(text);
    std::string tok;
    while (iss >> tok) {
        // Lowercase and strip leading/trailing punctuation
        std::string clean;
        for (char c : tok) {
            if (std::isalnum(static_cast<unsigned char>(c))) {
                clean += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
        }
        if (!clean.empty()) {
            tokens.push_back(clean);
        }
    }
    return tokens;
}

std::string QueryExpander::toLower(const std::string& s) {
    std::string result = s;
    for (char& c : result) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return result;
}

int QueryExpander::editDistance(const std::string& a, const std::string& b) {
    const size_t la = a.size(), lb = b.size();
    // Fast early exits
    if (la == 0) return static_cast<int>(lb);
    if (lb == 0) return static_cast<int>(la);
    if (std::abs(static_cast<int>(la) - static_cast<int>(lb)) > 3) {
        return static_cast<int>(std::abs(static_cast<int>(la) - static_cast<int>(lb)));
    }
    // Standard DP Levenshtein
    std::vector<int> prev(lb + 1), curr(lb + 1);
    for (size_t j = 0; j <= lb; ++j) prev[j] = static_cast<int>(j);
    for (size_t i = 1; i <= la; ++i) {
        curr[0] = static_cast<int>(i);
        for (size_t j = 1; j <= lb; ++j) {
            int cost = (a[i-1] == b[j-1]) ? 0 : 1;
            curr[j] = std::min({prev[j] + 1, curr[j-1] + 1, prev[j-1] + cost});
        }
        std::swap(prev, curr);
    }
    return prev[lb];
}

} // namespace themis
