/**
 * @file multi_field_search.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "search/multi_field_search.h"
#include "utils/logger.h"
#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <limits>

namespace themis {

// ============================================================================
// Construction
// ============================================================================

MultiFieldBoostedSearch::MultiFieldBoostedSearch(SecondaryIndexManager* index)
    : MultiFieldBoostedSearch(index, Config{}) {}

MultiFieldBoostedSearch::MultiFieldBoostedSearch(SecondaryIndexManager* index,
                                                 const Config& config)
    : index_(index), config_(config) {
    if (config_.k == 0) {
        throw std::invalid_argument("MultiFieldBoostedSearch: k must be > 0");
    }
    if (config_.candidates_per_field == 0) {
        throw std::invalid_argument(
            "MultiFieldBoostedSearch: candidates_per_field must be > 0");
    }
}

// ============================================================================
// normalizeScores
// ============================================================================

void MultiFieldBoostedSearch::normalizeScores(
    std::vector<std::pair<std::string, double>>& scored) {
    if (scored.empty()) {
      return;
    }

    double min_score = std::numeric_limits<double>::max();
    double max_score = std::numeric_limits<double>::lowest();
    for (const auto& [doc_id, s] : scored) {
        min_score = std::min(min_score, s);
        max_score = std::max(max_score, s);
    }

    const double range = max_score - min_score;
    if (range > 0.0) {
        for (auto& [doc_id, s] : scored) {
            s = (s - min_score) / range;
        }
    } else {
        // All scores are equal: map to 1.0 if positive, 0.0 otherwise
        const double normalized = (max_score > 0.0) ? 1.0 : 0.0;
        for (auto& [doc_id, s] : scored) {
            s = normalized;
        }
    }
}

// ============================================================================
// defaultFields
// ============================================================================

std::vector<MultiFieldBoostedSearch::FieldConfig>
MultiFieldBoostedSearch::defaultFields(const std::string& table) {
    return {
        {table, "title", 3.0},
        {table, "body",  1.0},
        {table, "tags",  0.5},
    };
}

// ============================================================================
// search
// ============================================================================

std::vector<MultiFieldBoostedSearch::Result> MultiFieldBoostedSearch::search(
    const std::string& query,
    const std::vector<FieldConfig>& fields) const {

    if (query.empty() || fields.empty()) {
        return {};
    }

    // doc_id → accumulated (boosted) score + per-field breakdown
    struct DocAccumulator {
        double score = 0.0;
        std::vector<std::pair<std::string, double>> field_scores;
    };
    std::unordered_map<std::string, DocAccumulator> accum;

    for (const auto& field : fields) {
        if (field.table.empty() || field.column.empty()) {
            THEMIS_WARN("MultiFieldBoostedSearch: skipping field with empty table/column");
            continue;
        }
        if (field.boost < 0.0) {
            THEMIS_WARN("MultiFieldBoostedSearch: skipping field '{}' with negative boost",
                        field.column);
            continue;
        }

        if (!index_) {
            THEMIS_DEBUG("MultiFieldBoostedSearch: no index available for field '{}'",
                         field.column);
            continue;
        }

        std::vector<std::pair<std::string, double>> scored;
        try {
            auto [status, results] = index_->scanFulltextWithScores(
                field.table,
                field.column,
                query,
                config_.candidates_per_field);

            if (!status.ok) {
                THEMIS_WARN("MultiFieldBoostedSearch: BM25 scan failed for field '{}': {}",
                            field.column, status.message);
                continue;
            }

            scored.reserve(results.size());
            for (const auto& r : results) {
                scored.emplace_back(r.pk, r.score);
            }
        } catch (const std::exception& e) {
            THEMIS_ERROR("MultiFieldBoostedSearch: exception scanning field '{}': {}",
                         field.column, e.what());
            continue;
        }

        normalizeScores(scored);

        for (const auto& [doc_id, norm_score] : scored) {
            auto& entry = accum[doc_id];
            entry.score += field.boost * norm_score;
            entry.field_scores.emplace_back(field.column, norm_score);
        }
    }

    // Build result list from accumulator
    std::vector<Result> results = {};

    results.reserve(accum.size());
    for (auto& [doc_id, entry] : accum) {
        Result r;
        r.document_id  = doc_id;
        r.score        = entry.score;
        r.field_scores = std::move(entry.field_scores);
        results.push_back(std::move(r));
    }

    // Sort by combined score descending
    std::sort(results.begin(), results.end(),
              [](const Result& a, const Result& b) {
                  return a.score > b.score;
              });

    if (static_cast<int>(results.size()) > config_.k) {
        results.resize(config_.k);
    }

    THEMIS_INFO("MultiFieldBoostedSearch: query='{}' fields={} -> {} results",
                query, fields.size(), results.size());

    return results;
}

} // namespace themis
