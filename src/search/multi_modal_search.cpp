/**
 * @file multi_modal_search.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.43
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


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
                                   VectorIndexManager* vec_index)
    : MultiModalSearch(sec_index, vec_index, Config{}) {}

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
    std::vector<std::string> modality_names;

    for (const auto& q : queries) {
        auto results = executeModal(q, table, column);
        if (!results.empty()) {
            all_lists.push_back(std::move(results));
            weights.push_back(q.weight > 0.0 ? q.weight : 1.0);
            // Build a human-readable modality label
            std::string mod_label;
            switch (q.modality) {
                case Modality::TEXT:   mod_label = "text";   break;
                case Modality::IMAGE:  mod_label = "image";  break;
                case Modality::AUDIO:  mod_label = "audio";  break;
                case Modality::CUSTOM: mod_label = "custom"; break;
            }
            if (!q.embedding_namespace.empty()) {
              mod_label += ":" + q.embedding_namespace;
            }
            modality_names.push_back(std::move(mod_label));
        }
    }

    if (all_lists.empty()) return {};
    return fuseRRF(all_lists, weights, modality_names);
}

std::vector<MultiModalResult> MultiModalSearch::searchTextAndImage(
    const std::string& text_query,
    const std::vector<float>& image_embedding,
    const std::string& image_namespace,
    const std::string& table,
    const std::string& column,
    double text_weight,
    double image_weight) const {

    std::vector<std::vector<std::pair<std::string, double>>> all_lists;
    std::vector<double> weights;
    std::vector<std::string> modality_names;

    if (!text_query.empty()) {
        ModalQuery tq;
        tq.modality = Modality::TEXT;
        tq.text = text_query;
        tq.weight = text_weight;
        auto results = executeModal(tq, table, column);
        if (!results.empty()) {
            all_lists.push_back(std::move(results));
            weights.push_back(text_weight);
            modality_names.push_back("text");
        }
    }

    if (!image_embedding.empty()) {
        ModalQuery iq;
        iq.modality = Modality::IMAGE;
        iq.embedding = image_embedding;
        iq.embedding_namespace = image_namespace;
        iq.weight = image_weight;
        auto results = executeModal(iq, table, column);
        if (!results.empty()) {
            all_lists.push_back(std::move(results));
            weights.push_back(image_weight);
            std::string label = "image";
            if (!image_namespace.empty()) {
              label += ":" + image_namespace;
            }
            modality_names.push_back(std::move(label));
        }
    }

    if (all_lists.empty()) return {};
    return fuseRRF(all_lists, weights, modality_names);
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
        [[fallthrough]];\n        case Modality::AUDIO:
        [[fallthrough]];\n        case Modality::CUSTOM: {
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
    const std::vector<double>& weights,
    const std::vector<std::string>& modality_names) const {

    std::unordered_map<std::string, double> rrf_scores;
    std::unordered_map<std::string, std::string> best_modality;
    std::unordered_map<std::string, double> best_contribution;

    for (size_t list_idx = 0; list_idx < ranked_lists.size(); ++list_idx) {
        const double w = (list_idx < weights.size()) ? weights[list_idx] : 1.0;
        const auto& list = ranked_lists[list_idx];
        const std::string& mod_name = (list_idx < modality_names.size())
            ? modality_names[list_idx]
            : "modal_" + std::to_string(list_idx);

        for (size_t rank = 0; rank < list.size(); ++rank) {
            const std::string& doc_id = list[rank].first;
            double contribution = w / (config_.rrf_k + static_cast<double>(rank + 1));
            rrf_scores[doc_id] += contribution;

            // Track the modality that contributed the most to this document's score
            auto it = best_contribution.find(doc_id);
            if (it == best_contribution.end() || contribution > it->second) {
                best_contribution[doc_id] = contribution;
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
