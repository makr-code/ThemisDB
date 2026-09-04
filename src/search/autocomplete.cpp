/**
 * @file autocomplete.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.43
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "search/autocomplete.h"
#include "storage/base_entity.h"
#include "utils/logger.h"
#include <algorithm>
#include <stdexcept>
#include <unordered_set>

namespace themis {

// ============================================================================
// Construction
// ============================================================================

AutocompleteEngine::AutocompleteEngine(SecondaryIndexManager* index,
                                       SearchAnalytics* analytics)
    : AutocompleteEngine(index, analytics, Config{}) {}

AutocompleteEngine::AutocompleteEngine(SecondaryIndexManager* index,
                                        SearchAnalytics* analytics,
                                        const Config& config)
    : index_(index), analytics_(analytics), config_(config) {
    if (config_.max_suggestions == 0) {
        throw std::invalid_argument("AutocompleteEngine: max_suggestions must be > 0");
    }
    if (config_.min_prefix_length == 0) {
        throw std::invalid_argument("AutocompleteEngine: min_prefix_length must be > 0");
    }
}

// ============================================================================
// suggest — combined
// ============================================================================

std::vector<Suggestion> AutocompleteEngine::suggest(const std::string& prefix,
                                                      const std::string& table,
                                                      const std::string& column) const {
    if (prefix.size() < config_.min_prefix_length) {
        return {};
    }

    std::vector<Suggestion> combined;

    if (config_.include_popular && analytics_) {
        auto popular = suggestPopular(prefix, config_.max_suggestions);
        for (auto& s : popular) {
          combined.push_back(std::move(s));
        }
    }

    if (config_.include_prefix && index_ && !table.empty() && !column.empty()) {
        auto prefix_sug = suggestByPrefix(prefix, table, column, config_.max_suggestions);
        for (auto& s : prefix_sug) {
          combined.push_back(std::move(s));
        }
    }

    // Deduplicate by text (keep highest-scoring entry for each text)
    if (config_.deduplicate) {
        std::unordered_set<std::string> seen;
        std::vector<Suggestion> deduped;
        // Sort by score descending first so the highest-scoring copy wins
        std::sort(combined.begin(), combined.end(),
                  [](const Suggestion& a, const Suggestion& b) {
                      return a.score > b.score;
                  });
        for (auto& s : combined) {
            if (seen.insert(s.text).second) {
                deduped.push_back(std::move(s));
            }
        }
        combined = std::move(deduped);
    } else {
        std::sort(combined.begin(), combined.end(),
                  [](const Suggestion& a, const Suggestion& b) {
                      return a.score > b.score;
                  });
    }

    if (combined.size() > config_.max_suggestions) {
        combined.resize(config_.max_suggestions);
    }

    THEMIS_DEBUG("AutocompleteEngine::suggest('{}') -> {} suggestions", prefix, combined.size());
    return combined;
}

// ============================================================================
// suggestByPrefix
// ============================================================================

std::vector<Suggestion> AutocompleteEngine::suggestByPrefix(const std::string& prefix,
                                                              const std::string& table,
                                                              const std::string& column,
                                                              size_t limit) const {
    if (!index_ || prefix.empty() || table.empty() || column.empty()) {
        return {};
    }

    // Upper bound: prefix + '\xff' covers all strings that start with prefix
    // (all ASCII/UTF-8 field values with this prefix sort before it, and using
    // an appended byte avoids the last-byte overflow when prefix ends with '\xff').
    std::string upper_bound = prefix + '\xff';

    auto [st, pks] = index_->scanKeysRange(
        table, column,
        prefix, upper_bound,
        true, false, // [prefix, prefix+'\xff')
        limit
    );

    if (!st.ok) {
        THEMIS_DEBUG("AutocompleteEngine::suggestByPrefix: scanKeysRange failed: {}", st.message);
        return {};
    }

    // Each PK from the range scan is a document PK whose field value starts
    // with the prefix.  We use the entity's field value as the suggestion text.
    std::vector<Suggestion> suggestions;
    suggestions.reserve(pks.size());

    for (const auto& pk : pks) {
        // Fetch the entity to get the actual field value
        auto [es, entities] = index_->scanEntitiesEqual(table, "id", pk);
        if (!es.ok || entities.empty()) {
            // Fallback: use the pk itself as suggestion text (works for value indexes)
            Suggestion s;
            s.text = pk;
            s.score = 1.0;
            suggestions.push_back(s);
            continue;
        }
        auto val_opt = entities[0].getFieldAsString(column);
        if (val_opt.has_value() && val_opt.value().rfind(prefix, 0) == 0) {
            Suggestion s;
            s.text = val_opt.value();
            s.score = 1.0;
            suggestions.push_back(std::move(s));
        }
        if (suggestions.size() >= limit) {
          break;
        }
    }

    return suggestions;
}

// ============================================================================
// suggestPopular
// ============================================================================

std::vector<Suggestion> AutocompleteEngine::suggestPopular(const std::string& prefix,
                                                             size_t limit) const {
    if (!analytics_) return {};

    auto metrics = analytics_->computeMetrics();
    if (metrics.top_queries.empty()) return {};

    // Collect queries that start with the prefix, sorted by frequency descending
    std::vector<std::pair<std::string, size_t>> matches;
    for (const auto& [query, count] : metrics.top_queries) {
        if (query.rfind(prefix, 0) == 0) { // starts with prefix
            matches.emplace_back(query, count);
        }
    }
    std::sort(matches.begin(), matches.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    std::vector<Suggestion> suggestions;
    for (size_t i = 0; i < std::min(limit, matches.size()); ++i) {
        Suggestion s;
        s.text = matches[i].first;
        s.score = static_cast<double>(matches[i].second) * config_.popular_boost;
        s.is_popular = true;
        suggestions.push_back(std::move(s));
    }
    return suggestions;
}

} // namespace themis
