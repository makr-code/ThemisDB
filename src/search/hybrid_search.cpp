/**
 * @file hybrid_search.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "search/hybrid_search.h"
#include "index/ann_frontdoor.h"
#include "index/secondary_index.h"
#include "index/vector_index.h"
#include "utils/logger.h"
#include <algorithm>
#include <unordered_map>
#include <cmath>
#include <stdexcept>

namespace {
    // Helper: Convert vector distance to similarity score based on metric
    double distanceToSimilarity(float distance, themis::VectorIndexManager::Metric metric) {
        using Metric = themis::VectorIndexManager::Metric;
        switch (metric) {
            case Metric::COSINE:
                // Cosine: 1 - distance (distance is already cosine distance)
                return 1.0 - distance;
            case Metric::DOT:
                // Dot product: higher is better (already similarity-like)
                return distance;
            case Metric::L2:
                // L2: inverse distance
                return 1.0 / (1.0 + distance);
            default:
                return 1.0 - distance;  // Default to cosine
        }
    }
}

namespace themis {

HybridSearch::HybridSearch(
    SecondaryIndexManager* fulltext_index,
    VectorIndexManager* vector_index,
    const Config& config
) : fulltext_index_(fulltext_index),
    vector_index_(vector_index),
    config_(config) {
    
    if (config_.k == 0) {
        throw std::invalid_argument("HybridSearch: Config::k must be > 0");
    }
    if (config_.max_k == 0) {
        throw std::invalid_argument("HybridSearch: Config::max_k must be > 0");
    }
    if (config_.k > config_.max_k) {
        throw std::invalid_argument("HybridSearch: Config::k exceeds max_k");
    }
    if (config_.max_candidates == 0) {
        throw std::invalid_argument("HybridSearch: Config::max_candidates must be > 0");
    }
    if (config_.k_bm25 > config_.max_candidates) {
        throw std::invalid_argument("HybridSearch: Config::k_bm25 exceeds max_candidates");
    }
    if (config_.k_vector > config_.max_candidates) {
        throw std::invalid_argument("HybridSearch: Config::k_vector exceeds max_candidates");
    }
    if (config_.rrf_k <= 0.0) {
        throw std::invalid_argument("HybridSearch: Config::rrf_k must be > 0");
    }
    if (config_.bm25_weight < 0.0 || config_.vector_weight < 0.0) {
        throw std::invalid_argument("HybridSearch: weights must be non-negative");
    }
    if (config_.default_table.empty()) {
        throw std::invalid_argument("HybridSearch: Config::default_table must not be empty");
    }
    if (config_.default_column.empty()) {
        throw std::invalid_argument("HybridSearch: Config::default_column must not be empty");
    }

    THEMIS_INFO("HybridSearch initialized (RRF={}, k={}, metric={})", 
                config_.use_rrf, config_.k,
                static_cast<int>(config_.vector_metric));
}

HybridSearch::~HybridSearch() noexcept = default;

// ============================================================================
// Reranker attachment
// ============================================================================

void HybridSearch::setReranker(ILlmReranker::LlmBackend backend,
                                const ILlmReranker::Config& config) {
    if (!backend) {
        reranker_.reset();
        THEMIS_DEBUG("HybridSearch: LLM re-ranker disabled");
        return;
    }
    // Try to create an implementation via factory. If none registered,
    // disable the reranker and log a warning to avoid linking heavy code.
    auto impl = search::createLlmReranker(config);
    if (impl) {
        impl->setBackend(std::move(backend));
        reranker_ = std::shared_ptr<ILlmReranker>(impl.release());
        THEMIS_DEBUG("HybridSearch: LLM re-ranker attached (batch_size={}, llm_weight={:.2f})",
                     config.batch_size, config.llm_weight);
    } else {
        THEMIS_WARN("HybridSearch: no LlmReranker factory registered; reranker disabled");
        reranker_.reset();
    }
}

void HybridSearch::setAnnFrontdoor(std::shared_ptr<index::AnnFrontdoor> frontdoor) {
    ann_frontdoor_ = std::move(frontdoor);
}

std::vector<HybridSearch::Result> HybridSearch::search(
    const std::string& text_query,
    const float* vector_query,
    size_t vector_dim,
    SearchStats* stats
) {
    std::vector<Result> bm25_results;
    std::vector<Result> vector_results;
    bool bm25_ok = false;
    bool vector_ok = false;
    
    // BM25 fulltext search
    if (!text_query.empty() && fulltext_index_) {
        try {
            auto [status, ft_results] = fulltext_index_->scanFulltextWithScores(
                config_.default_table,
                config_.default_column, 
                text_query,
                config_.k_bm25
            );
            
            if (status.ok) {
                bm25_results.reserve(ft_results.size());
                for (size_t i = 0; i < ft_results.size(); ++i) {
                    const auto& ft_result = ft_results[i];
                    Result r;
                    r.document_id = ft_result.pk;
                    r.bm25_score = ft_result.score;
                    r.bm25_rank = static_cast<int>(i + 1);
                    bm25_results.push_back(r);
                }
                bm25_ok = true;
                THEMIS_DEBUG("BM25 search returned {} results", bm25_results.size());
            } else {
                THEMIS_WARN("BM25 search failed: {}", status.message);
            }
        } catch (const std::exception& e) {
            THEMIS_ERROR("BM25 search exception: {}", e.what());
        }
    }
    
    // Vector ANN search
    if (vector_query && vector_dim > 0 && (ann_frontdoor_ || vector_index_)) {
        try {
            std::vector<float> query_vec(vector_query, vector_query + vector_dim);
            if (ann_frontdoor_) {
                index::AnnQueryContext context;
                auto frontdoor_result = ann_frontdoor_->search(
                    query_vec.data(),
                    vector_dim,
                    static_cast<int>(config_.k_vector),
                    context
                );

                vector_results.reserve(frontdoor_result.candidates.size());
                for (size_t i = 0; i < frontdoor_result.candidates.size(); ++i) {
                    const auto& candidate = frontdoor_result.candidates[i];
                    Result r;
                    r.document_id = std::to_string(candidate.id);
                    r.vector_score = distanceToSimilarity(candidate.distance,
                                                          config_.vector_metric);
                    r.vector_rank = static_cast<int>(i + 1);
                    vector_results.push_back(r);
                }

                if (vector_results.empty() &&
                    vector_index_ != nullptr &&
                    (frontdoor_result.strategy_used == index::AnnStrategy::FLAT_BRUTE_FORCE ||
                     frontdoor_result.strategy_used == index::AnnStrategy::HNSW)) {
                    auto [status, vec_results] = vector_index_->searchKnn(
                        query_vec,
                        config_.k_vector
                    );

                    if (status.ok) {
                        vector_results.reserve(vec_results.size());
                        for (size_t i = 0; i < vec_results.size(); ++i) {
                            const auto& vec_result = vec_results[i];
                            Result r;
                            r.document_id = vec_result.pk;
                            r.vector_score = distanceToSimilarity(vec_result.distance,
                                                                  config_.vector_metric);
                            r.vector_rank = static_cast<int>(i + 1);
                            vector_results.push_back(r);
                        }
                    } else {
                        THEMIS_WARN("HybridSearch: legacy vector fallback after AnnFrontdoor returned no candidates failed: {}",
                                    status.message);
                    }
                }

                vector_ok = true;
                THEMIS_DEBUG("Vector search returned {} results via AnnFrontdoor",
                             vector_results.size());
            } else {
                auto [status, vec_results] = vector_index_->searchKnn(
                    query_vec,
                    config_.k_vector
                );

                if (status.ok) {
                    vector_results.reserve(vec_results.size());
                    for (size_t i = 0; i < vec_results.size(); ++i) {
                        const auto& vec_result = vec_results[i];
                        Result r;
                        r.document_id = vec_result.pk;
                        // Convert distance to similarity based on configured metric
                        r.vector_score = distanceToSimilarity(vec_result.distance,
                                                              config_.vector_metric);
                        r.vector_rank = static_cast<int>(i + 1);
                        vector_results.push_back(r);
                    }
                    vector_ok = true;
                    THEMIS_DEBUG("Vector search returned {} results", vector_results.size());
                } else {
                    THEMIS_WARN("Vector search failed: {}", status.message);
                }
            }
        } catch (const std::exception& e) {
            THEMIS_ERROR("Vector search exception: {}", e.what());
        }
    }

    // Detect partial-result condition: both sources were available and attempted,
    // but exactly one of them failed while the other returned candidates.
    bool bm25_attempted  = !text_query.empty() && fulltext_index_ != nullptr;
    bool vector_attempted = vector_query != nullptr && vector_dim > 0 &&
                            (ann_frontdoor_ != nullptr || vector_index_ != nullptr);
    bool bm25_failed_but_vector_succeeded =
        bm25_attempted && !bm25_ok && vector_ok && !vector_results.empty();
    bool vector_failed_but_bm25_succeeded =
        vector_attempted && !vector_ok && bm25_ok && !bm25_results.empty();
    bool partial = bm25_failed_but_vector_succeeded || vector_failed_but_bm25_succeeded;
    if (partial) {
        THEMIS_WARN("HybridSearch returning partial results: bm25_ok={} vector_ok={}",
                    bm25_ok, vector_ok);
    }

    if (stats) {
        stats->bm25_ok       = bm25_ok;
        stats->vector_ok     = vector_ok;
        stats->partial_result = partial;
        stats->bm25_count    = bm25_results.size();
        stats->vector_count  = vector_results.size();
    }

    // Normalize scores if configured (for RRF) or always for linear combination
    // (linear combination requires comparable [0,1] scores for correct weighting)
    if (config_.normalize_scores || !config_.use_rrf) {
        normalizeScores(bm25_results, true);
        normalizeScores(vector_results, false);
    }
    
    // Fuse results – wrapped in a safety-net catch to guarantee search() never throws
    try {
        std::vector<Result> fused;
        if (config_.use_rrf) {
            fused = reciprocalRankFusion(bm25_results, vector_results);
            THEMIS_INFO("Hybrid search: {} BM25 + {} vector -> {} fused results",
                       bm25_results.size(), vector_results.size(), fused.size());
        } else {
            // Linear combination fallback
            std::unordered_map<std::string, Result> doc_map;
            
            for (const auto& r : bm25_results) {
                auto& doc = doc_map[r.document_id];
                doc = r;
                doc.hybrid_score += config_.bm25_weight * r.bm25_score;
            }
            
            for (const auto& r : vector_results) {
                auto& doc = doc_map[r.document_id];
                if (doc.document_id.empty()) doc = r;
                doc.vector_score = r.vector_score;
                doc.vector_rank = r.vector_rank;
                doc.hybrid_score += config_.vector_weight * r.vector_score;
            }
            
            fused.reserve(doc_map.size());
            for (const auto& [_, result] : doc_map) {
                fused.push_back(result);
            }
            
            std::sort(fused.begin(), fused.end(),
                      [](const Result& a, const Result& b) {
                          return a.hybrid_score > b.hybrid_score;
                      });
            
            if (fused.size() > config_.k) {
                fused.resize(config_.k);
            }
            
            THEMIS_INFO("Hybrid search (linear): {} BM25 + {} vector -> {} combined results",
                       bm25_results.size(), vector_results.size(), fused.size());
        }

        // LLM re-ranking: optional Phase-3 post-processing step
        if (reranker_ && !fused.empty() && !text_query.empty()) {
            std::vector<LlmRerankCandidate> candidates;
            candidates.reserve(fused.size());
            for (const auto& r : fused) {
                LlmRerankCandidate c;
                c.document_id   = r.document_id;
                c.content       = r.content;
                c.initial_score = r.hybrid_score;
                candidates.push_back(std::move(c));
            }

            auto reranked = reranker_->rerank(text_query, candidates);

            // Rebuild Result list in LLM-determined order, updating hybrid_score
            std::vector<Result> reranked_results;
            reranked_results.reserve(reranked.size());
            // Build a lookup map for O(1) access
            std::unordered_map<std::string, const Result*> result_map;
            for (const auto& r : fused) {
                result_map[r.document_id] = &r;
            }
            for (const auto& rr : reranked) {
                auto it = result_map.find(rr.document_id);
                if (it != result_map.end()) {
                    Result out = *(it->second);
                    out.hybrid_score = rr.final_score;
                    reranked_results.push_back(std::move(out));
                }
            }
            THEMIS_INFO("LLM re-ranker: {} -> {} results", fused.size(), reranked_results.size());
            return reranked_results;
        }

        return fused;
    } catch (const std::exception& e) {
        THEMIS_ERROR("HybridSearch fusion exception (returning empty): {}", e.what());
        return {};
    }
}

std::vector<HybridSearch::Result> HybridSearch::reciprocalRankFusion(
    const std::vector<Result>& bm25_results,
    const std::vector<Result>& vector_results
) {
    // RRF: score(d) = sum(1 / (k + rank_i(d)))
    std::unordered_map<std::string, Result> doc_map;
    
    // Process BM25 results
    for (size_t i = 0; i < bm25_results.size(); ++i) {
        const auto& r = bm25_results[i];
        auto& doc = doc_map[r.document_id];
        doc.document_id = r.document_id;
        doc.bm25_score = r.bm25_score;
        doc.bm25_rank = static_cast<int>(i + 1);
        doc.content = r.content;
        
        // RRF contribution from BM25
        double rrf_score = 1.0 / (config_.rrf_k + (i + 1));
        doc.hybrid_score += config_.bm25_weight * rrf_score;
    }
    
    // Process vector results
    for (size_t i = 0; i < vector_results.size(); ++i) {
        const auto& r = vector_results[i];
        auto& doc = doc_map[r.document_id];
        doc.document_id = r.document_id;
        doc.vector_score = r.vector_score;
        doc.vector_rank = static_cast<int>(i + 1);
        if (doc.content.empty()) doc.content = r.content;
        
        // RRF contribution from vector search
        double rrf_score = 1.0 / (config_.rrf_k + (i + 1));
        doc.hybrid_score += config_.vector_weight * rrf_score;
    }
    
    // Convert map to vector and sort by hybrid score
    std::vector<Result> fused_results;
    for (const auto& [_, result] : doc_map) {
        fused_results.push_back(result);
    }
    
    std::sort(fused_results.begin(), fused_results.end(),
              [](const Result& a, const Result& b) {
                  return a.hybrid_score > b.hybrid_score;
              });
    
    // Limit to top-k
    if (fused_results.size() > config_.k) {
        fused_results.resize(config_.k);
    }
    
    THEMIS_INFO("RRF fusion: {} BM25 + {} vector -> {} results",
                bm25_results.size(), vector_results.size(), fused_results.size());
    
    return fused_results;
}

void HybridSearch::normalizeScores(std::vector<Result>& results, bool is_bm25) {
    if (results.empty()) return;
    
    // Find min/max scores
    double min_score = std::numeric_limits<double>::max();
    double max_score = std::numeric_limits<double>::lowest();
    
    for (const auto& r : results) {
        double score = is_bm25 ? r.bm25_score : r.vector_score;
        min_score = std::min(min_score, score);
        max_score = std::max(max_score, score);
    }
    
    // Normalize to [0, 1]
    double range = max_score - min_score;
    if (range > 0.0) {
        for (auto& r : results) {
            if (is_bm25) {
                r.bm25_score = (r.bm25_score - min_score) / range;
            } else {
                r.vector_score = (r.vector_score - min_score) / range;
            }
        }
    } else {
        // All scores are equal (single result or tied): set to 1.0 if score > 0, 0.0 otherwise
        double normalized = (max_score > 0.0) ? 1.0 : 0.0;
        for (auto& r : results) {
            if (is_bm25) {
                r.bm25_score = normalized;
            } else {
                r.vector_score = normalized;
            }
        }
    }
}

} // namespace themis

