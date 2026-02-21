#include "search/multi_modal_search.h"
#include "utils/logger.h"
#include <algorithm>
#include <stdexcept>
#include <unordered_map>

namespace themis {

// ============================================================================
// Construction
// ============================================================================

MultiModalSearch::MultiModalSearch(SecondaryIndexManager* sec_index,
                                    VectorIndexManager*    vec_index,
                                    const Config& config)
    : sec_index_(sec_index), vec_index_(vec_index), config_(config) {
    if (config_.k == 0) {
        throw std::invalid_argument("MultiModalSearch: k must be > 0");
    }
    if (config_.rrf_k <= 0.0) {
        throw std::invalid_argument("MultiModalSearch: rrf_k must be > 0");
    }
    if (config_.candidates_per_modal == 0) {
        throw std::invalid_argument("MultiModalSearch: candidates_per_modal must be > 0");
    }
}

// ============================================================================
// search
// ============================================================================

std::vector<MultiModalResult> MultiModalSearch::search(
    const std::vector<ModalQuery>& queries,
    const std::string& table,
    const std::string& column) const {

    if (queries.empty()) return {};

    std::vector<std::vector<std::pair<std::string, double>>> all_lists;
    std::vector<double> weights;

    for (const auto& q : queries) {
        auto results = executeModal(q, table, column);
        if (!results.empty()) {
            all_lists.push_back(std::move(results));
            weights.push_back(q.weight > 0.0 ? q.weight : 1.0);
        }
    }

    if (all_lists.empty()) return {};
    return fuseRRF(all_lists, weights);
}

std::vector<MultiModalResult> MultiModalSearch::searchTextAndImage(
    const std::string& text_query,
    const std::vector<float>& image_embedding,
    const std::string& image_namespace,
    const std::string& table,
    const std::string& column,
    double text_weight,
    double image_weight) const {

    std::vector<ModalQuery> queries;

    if (!text_query.empty()) {
        ModalQuery tq;
        tq.modality = Modality::TEXT;
        tq.text = text_query;
        tq.weight = text_weight;
        queries.push_back(std::move(tq));
    }

    if (!image_embedding.empty()) {
        ModalQuery iq;
        iq.modality = Modality::IMAGE;
        iq.embedding = image_embedding;
        iq.embedding_namespace = image_namespace;
        iq.weight = image_weight;
        queries.push_back(std::move(iq));
    }

    return search(queries, table, column);
}

// ============================================================================
// executeModal
// ============================================================================

std::vector<std::pair<std::string, double>> MultiModalSearch::executeModal(
    const ModalQuery& query,
    const std::string& table,
    const std::string& column) const {

    std::vector<std::pair<std::string, double>> results;

    switch (query.modality) {
        case Modality::TEXT: {
            if (!sec_index_ || table.empty() || column.empty() || query.text.empty()) {
                THEMIS_DEBUG("MultiModalSearch: TEXT modality skipped (null index or empty args)");
                return results;
            }
            auto [st, ft_results] = sec_index_->scanFulltextWithScores(
                table, column, query.text, config_.candidates_per_modal);
            if (!st.ok) {
                THEMIS_WARN("MultiModalSearch: TEXT search failed: {}", st.message);
                return results;
            }
            results.reserve(ft_results.size());
            for (const auto& r : ft_results) {
                results.emplace_back(r.pk, r.score);
            }
            break;
        }

        case Modality::IMAGE:
        case Modality::AUDIO:
        case Modality::CUSTOM: {
            if (!vec_index_ || query.embedding.empty()) {
                THEMIS_DEBUG("MultiModalSearch: embedding modality skipped (null index or empty embedding)");
                return results;
            }
            // Switch to the embedding namespace if specified
            if (!query.embedding_namespace.empty()) {
                // Note: VectorIndexManager is single-namespace per instance;
                // multi-namespace requires separate instances per modality.
                // We issue the search on the current vec_index_ which must
                // already be initialized for this namespace.
            }
            auto [st, knn_results] = vec_index_->searchKnn(
                query.embedding,
                config_.candidates_per_modal
            );
            if (!st.ok) {
                THEMIS_WARN("MultiModalSearch: KNN search failed: {}", st.message);
                return results;
            }
            results.reserve(knn_results.size());
            for (const auto& r : knn_results) {
                // distance is smaller-is-better; convert to score (larger-is-better)
                double sim = 1.0 / (1.0 + static_cast<double>(r.distance));
                results.emplace_back(r.pk, sim);
            }
            break;
        }
    }

    // Sort by score descending
    std::sort(results.begin(), results.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    return results;
}

// ============================================================================
// fuseRRF
// ============================================================================

std::vector<MultiModalResult> MultiModalSearch::fuseRRF(
    const std::vector<std::vector<std::pair<std::string, double>>>& ranked_lists,
    const std::vector<double>& weights) const {

    std::unordered_map<std::string, double> rrf_scores;
    std::unordered_map<std::string, std::string> best_modality;

    for (size_t list_idx = 0; list_idx < ranked_lists.size(); ++list_idx) {
        const double w = (list_idx < weights.size()) ? weights[list_idx] : 1.0;
        const auto& list = ranked_lists[list_idx];
        const std::string mod_name = "modal_" + std::to_string(list_idx);

        for (size_t rank = 0; rank < list.size(); ++rank) {
            const std::string& doc_id = list[rank].first;
            double contribution = w / (config_.rrf_k + static_cast<double>(rank + 1));
            rrf_scores[doc_id] += contribution;

            // Track which modality gave the best contribution
            auto it = best_modality.find(doc_id);
            if (it == best_modality.end()) {
                best_modality[doc_id] = mod_name;
            }
        }
    }

    // Build result list
    std::vector<MultiModalResult> results;
    results.reserve(rrf_scores.size());
    for (const auto& [doc_id, score] : rrf_scores) {
        MultiModalResult r;
        r.document_id = doc_id;
        r.score = score;
        r.matched_modality = best_modality.count(doc_id) ? best_modality[doc_id] : "";
        results.push_back(std::move(r));
    }

    // Sort by score descending, cap at k
    std::sort(results.begin(), results.end(),
              [](const MultiModalResult& a, const MultiModalResult& b) {
                  return a.score > b.score;
              });
    if (results.size() > config_.k) {
        results.resize(config_.k);
    }

    THEMIS_DEBUG("MultiModalSearch::fuseRRF: {} lists -> {} results (k={})",
                 ranked_lists.size(), results.size(), config_.k);
    return results;
}

} // namespace themis
