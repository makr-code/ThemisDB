/**
 * @file distributed_hybrid_search.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 2.2.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 87/100
 * @note Gap Summary: Phase 2 hardening complete; degradation flags integrated
 * @note Status: Production Ready (Phase 2)
 * @note This block is auto-generated and will be overwritten.
 */


#include "search/distributed_hybrid_search.h"
#include "utils/logger.h"
#include <algorithm>
#include <chrono>
#include <future>
#include <stdexcept>
#include <unordered_map>

namespace themis {

// ============================================================================
// Construction
// ============================================================================

DistributedHybridSearch::DistributedHybridSearch(
    HybridSearch* local_search,
    themis::sharding::URNResolver* resolver,
    themis::sharding::RemoteExecutor* executor
) : DistributedHybridSearch(local_search, resolver, executor, Config{}) {}

DistributedHybridSearch::DistributedHybridSearch(
    HybridSearch* local_search,
    themis::sharding::URNResolver* resolver,
    themis::sharding::RemoteExecutor* executor,
    const Config& config
) : local_search_(local_search),
    resolver_(resolver),
    executor_(executor),
    config_(config) {

    if (config_.k == 0) {
        throw std::invalid_argument(
            "DistributedHybridSearch: Config::k must be > 0");
    }
    if (config_.rrf_k <= 0.0) {
        throw std::invalid_argument(
            "DistributedHybridSearch: Config::rrf_k must be > 0");
    }
    if (config_.max_concurrent_shards == 0) {
        throw std::invalid_argument(
            "DistributedHybridSearch: Config::max_concurrent_shards must be > 0");
    }
    if (config_.search_endpoint.empty()) {
        throw std::invalid_argument(
            "DistributedHybridSearch: Config::search_endpoint must not be empty");
    }

    THEMIS_INFO("DistributedHybridSearch initialized (k={}, rrf_k={:.1f}, "
                "local_shard='{}', skip_failed={})",
                config_.k, config_.rrf_k,
                config_.local_shard_id, config_.skip_failed_shards);
}

// ============================================================================
// Distributed Search
// ============================================================================

std::vector<HybridSearch::Result> DistributedHybridSearch::search(
    const std::string& text_query,
    const std::vector<float>& vector_query,
    SearchStats* stats
) {
    total_searches_++;

    std::vector<ShardSearchResult> shard_results;

    // --- Local shard ---
    {
        ShardSearchResult local;
        local.shard_id = config_.local_shard_id.empty()
                             ? "local"
                             : config_.local_shard_id;

        if (local_search_) {
            try {
                auto t_start = std::chrono::steady_clock::now();

                HybridSearch::SearchStats local_stats;
                local.results = local_search_->search(
                    text_query,
                    vector_query.empty() ? nullptr : vector_query.data(),
                    vector_query.size(),
                    &local_stats
                );
                local.success = true;

                auto t_end = std::chrono::steady_clock::now();
                local.execution_time_ms =
                    static_cast<uint64_t>(
                        std::chrono::duration_cast<std::chrono::milliseconds>(
                            t_end - t_start).count());

                THEMIS_DEBUG("DistributedHybridSearch: local shard '{}' "
                             "returned {} results in {} ms",
                             local.shard_id,static_cast<int>(local.results.size()),
                             local.execution_time_ms);
            } catch (const std::exception& e) {
                local.success = false;
                local.error_msg =
                    std::string("Local search exception: ") + e.what();
                THEMIS_ERROR("DistributedHybridSearch: local shard error: {}",
                             local.error_msg);
            }
        } else {
            // No local search engine attached; treat as empty success
            local.success = true;
            THEMIS_DEBUG("DistributedHybridSearch: no local_search attached "
                         "(shard='{}')", local.shard_id);
        }

        shard_results.push_back(std::move(local));
    }

    // --- Remote shards ---
    if (resolver_ && executor_) {
        try {
            auto healthy_shards = resolver_->getHealthyShards();

            // Collect remote shards (exclude the local one)
            std::vector<themis::sharding::ShardInfo> remote_shards = {};

            remote_shards.reserve(healthy_shards.size());
            for (const auto& shard : healthy_shards) {
                if (shard.shard_id != config_.local_shard_id) {
                    remote_shards.push_back(shard);
                }
            }

            const size_t max_concurrent = std::min(
                config_.max_concurrent_shards,static_cast<int>(remote_shards.size()));

            for (size_t batch_start = 0;
                 batch_start < remote_shards.size();
                 batch_start += max_concurrent) {

                const size_t batch_end = std::min(
                    batch_start + max_concurrent,static_cast<int>(remote_shards.size()));

                std::vector<std::future<ShardSearchResult>> futures;
                futures.reserve(batch_end - batch_start);

                for (size_t i = batch_start; i < batch_end; ++i) {
                    auto shard = remote_shards[i];
                    futures.push_back(std::async(
                        std::launch::async,
                        [this, shard, &text_query, &vector_query]()
                            -> ShardSearchResult {
                            return searchRemoteShard(
                                shard, text_query, vector_query, config_.k);
                        }));
                }

                const auto timeout =
                    std::chrono::milliseconds(config_.shard_timeout_ms);

                for (size_t i = 0; i < futures.size(); ++i) {
                    const auto& shard = remote_shards[batch_start + i];
                    try {
                        if (futures[i].wait_for(timeout) ==
                            std::future_status::ready) {
                            shard_results.push_back(futures[i].get());
                        } else {
                            // Timeout
                            ShardSearchResult timeout_result;
                            timeout_result.shard_id = shard.shard_id;
                            timeout_result.success = false;
                            timeout_result.error_msg = "shard timeout";
                            timeout_result.execution_time_ms =
                                config_.shard_timeout_ms;
                            shard_results.push_back(std::move(timeout_result));
                            THEMIS_WARN(
                                "DistributedHybridSearch: shard '{}' timed out "
                                "after {} ms",
                                shard.shard_id, config_.shard_timeout_ms);
                        }
                    } catch (const std::exception& e) {
                        ShardSearchResult err_result;
                        err_result.shard_id = shard.shard_id;
                        err_result.success = false;
                        err_result.error_msg =
                            std::string("future exception: ") + e.what();
                        shard_results.push_back(std::move(err_result));
                    }
                }
            }
        } catch (const std::exception& e) {
            THEMIS_ERROR("DistributedHybridSearch: resolver error: {}",
                         e.what());
        }
    }

    // --- Statistics ---
    size_t n_success = 0;
    size_t n_failed = 0;
    for (const auto& sr : shard_results) {
        if (sr.success) {
            ++n_success;
        } else {
            ++n_failed;
        }
    }

    total_shards_queried_ += shard_results.size();
    total_shard_failures_ += n_failed;

    if (stats) {
        stats->shards_queried   = shard_results.size();
        stats->shards_succeeded = n_success;
        stats->shards_failed    = n_failed;
        stats->partial_result   = (n_failed > 0 && n_success > 0);
        
        // Phase 2: Populate failed_shard_reasons for operator diagnostics
        for (const auto& sr : shard_results) {
            if (!sr.success && !sr.error_msg.empty()) {
                stats->failed_shard_reasons.push_back(
                    sr.shard_id + ": " + sr.error_msg);
            }
        }
    }

    if (n_failed > 0) {
        THEMIS_WARN("DistributedHybridSearch: {}/{} shards failed",
                    n_failed,static_cast<int>(shard_results.size()));
    }

    // Abort if any shard failed and skip_failed_shards is false
    if (!config_.skip_failed_shards && n_failed > 0) {
        THEMIS_ERROR("DistributedHybridSearch: aborting due to shard failure "
                     "(skip_failed_shards=false)");
        return {};
    }

    // --- Merge ---
    return mergeShardResults(shard_results, stats);
}

// ============================================================================
// Cross-Shard RRF Merge
// ============================================================================

std::vector<HybridSearch::Result> DistributedHybridSearch::mergeShardResults(
    const std::vector<ShardSearchResult>& shard_results,
    SearchStats* stats
) const {
    // Phase 2: Enhanced merge with degradation tracking
    
    // Accumulate global RRF scores; pick best per-field metadata
    struct Accum {
        double rrf_score = 0.0;
        // Keep the highest per-source scores seen across shards
        double best_bm25   = 0.0;
        double best_vector = 0.0;
        int    best_bm25_rank   = -1;
        int    best_vector_rank = -1;
        std::string content = {};
        size_t appearance_count = 0; ///< Track high-overlap variance
    };

    std::unordered_map<std::string, Accum> doc_map = {};

    doc_map.reserve(config_.k * shard_results.size());

    size_t successful_shards = 0;
    for (const auto& sr : shard_results) {
        if (sr.success) {
          ++successful_shards;
        }
        
        if (!sr.success && config_.skip_failed_shards) {
            continue;
        }

        for (size_t rank = 0; rank < sr.results.size(); ++rank) {
            const auto& r = sr.results[rank];
            if (r.document_id.empty()) {
              continue;
            }

            auto& acc = doc_map[r.document_id];
            acc.appearance_count++;

            // RRF contribution from this shard's ranking
            acc.rrf_score += 1.0 / (config_.rrf_k + static_cast<double>(rank + 1));

            // Carry over the best individual scores
            if (r.bm25_score > acc.best_bm25) {
                acc.best_bm25      = r.bm25_score;
                acc.best_bm25_rank = r.bm25_rank;
            }
            if (r.vector_score > acc.best_vector) {
                acc.best_vector      = r.vector_score;
                acc.best_vector_rank = r.vector_rank;
            }
            if (acc.content.empty() && !r.content.empty()) {
                acc.content = r.content;
            }
        }
    }

    // Build result vector
    std::vector<HybridSearch::Result> merged = {};

    merged.reserve(doc_map.size());
    
    // Track high-overlap variance: documents appearing in many shards
    size_t high_overlap_count = 0;
    for (const auto& [doc_id, acc] : doc_map) {
        HybridSearch::Result r;
        r.document_id  = doc_id;
        r.hybrid_score = acc.rrf_score;
        r.bm25_score   = acc.best_bm25;
        r.vector_score = acc.best_vector;
        r.bm25_rank    = acc.best_bm25_rank;
        r.vector_rank  = acc.best_vector_rank;
        r.content      = acc.content;
        merged.push_back(std::move(r));
        
        // Phase 2: Detect high-cardinality overlap (document in >50% of shards)
        if (successful_shards > 0 && 
            acc.appearance_count > successful_shards / 2) {
            ++high_overlap_count;
        }
    }

    std::sort(merged.begin(), merged.end(),
              [](const HybridSearch::Result& a, const HybridSearch::Result& b) {
                  return a.hybrid_score > b.hybrid_score;
              });

    // Phase 2: Track merge underflow (insufficient candidates)
    bool merge_underflow = false;
    if (static_cast<int>(merged.size()) < config_.k) {
        merge_underflow = true;
        THEMIS_WARN("DistributedHybridSearch: merge underflow "
                    "(expected {} results, got {})",
                    config_.k,static_cast<int>(merged.size()));
    }
    
    if (static_cast<int>(merged.size()) > config_.k) {
        merged.resize(config_.k);
    }

    // Populate degradation flags if stats provided
    if (stats) {
        stats->merge_underflow = merge_underflow;
        stats->high_overlap_variance = (high_overlap_count > config_.k / 2);
    }

    THEMIS_INFO("DistributedHybridSearch: merged {} shards -> {} results "
                "(underflow={}, high_overlap={})",
                shard_results.size(),static_cast<int>(merged.size()),
                merge_underflow, stats ? stats->high_overlap_variance : false);

    return merged;
}

// ============================================================================
// Remote shard dispatch
// ============================================================================

DistributedHybridSearch::ShardSearchResult
DistributedHybridSearch::searchRemoteShard(
    const themis::sharding::ShardInfo& shard,
    const std::string& text_query,
    const std::vector<float>& vector_query,
    size_t k
) {
    ShardSearchResult result;
    result.shard_id = shard.shard_id;

    auto t_start = std::chrono::steady_clock::now();

    try {
        // Build request payload
        nlohmann::json payload;
        payload["query"]  = text_query;
        payload["k"]      = k;
        if (!vector_query.empty()) {
            payload["vector_query"] = vector_query;
        }

        auto exec_result = executor_->post(
            shard, config_.search_endpoint, payload);

        auto t_end = std::chrono::steady_clock::now();
        result.execution_time_ms = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                t_end - t_start).count());

        if (!exec_result.success) {
            result.success   = false;
            result.error_msg = exec_result.error;
            THEMIS_WARN("DistributedHybridSearch: shard '{}' returned error: {}",
                        shard.shard_id, exec_result.error);
            return result;
        }

        result.results = parseShardResponse(exec_result.data);
        result.success = true;

        THEMIS_DEBUG(
            "DistributedHybridSearch: shard '{}' returned {} results "
            "in {} ms",
            shard.shard_id,static_cast<int>(result.results.size()),
            result.execution_time_ms);

    } catch (const std::exception& e) {
        auto t_end = std::chrono::steady_clock::now();
        result.execution_time_ms = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                t_end - t_start).count());
        result.success   = false;
        result.error_msg = std::string("exception: ") + e.what();
        THEMIS_ERROR("DistributedHybridSearch: shard '{}' exception: {}",
                     shard.shard_id, result.error_msg);
    }

    return result;
}

// ============================================================================
// JSON deserialization
// ============================================================================

std::vector<HybridSearch::Result> DistributedHybridSearch::parseShardResponse(
    const nlohmann::json& data
) {
    std::vector<HybridSearch::Result> results;

    // Response may be a direct array or wrapped in a "results" field
    const nlohmann::json* arr = nullptr;
    if (data.is_array()) {
        arr = &data;
    } else if (data.is_object() && data.contains("results") &&
               data["results"].is_array()) {
        arr = &data["results"];
    } else {
        THEMIS_WARN("DistributedHybridSearch: unexpected shard response format");
        return results;
    }

    results.reserve(arr->size());
    for (const auto& item : *arr) {
        if (!item.is_object()) {
          continue;
        }

        HybridSearch::Result r;
        if (item.contains("document_id") && item["document_id"].is_string()) {
            r.document_id = item["document_id"].get<std::string>();
        }
        if (r.document_id.empty()) continue;  // skip malformed entries

        if (item.contains("bm25_score") && item["bm25_score"].is_number()) {
            r.bm25_score = item["bm25_score"].get<double>();
        }
        if (item.contains("vector_score") && item["vector_score"].is_number()) {
            r.vector_score = item["vector_score"].get<double>();
        }
        if (item.contains("hybrid_score") && item["hybrid_score"].is_number()) {
            r.hybrid_score = item["hybrid_score"].get<double>();
        }
        if (item.contains("bm25_rank") && item["bm25_rank"].is_number()) {
            r.bm25_rank = item["bm25_rank"].get<int>();
        }
        if (item.contains("vector_rank") && item["vector_rank"].is_number()) {
            r.vector_rank = item["vector_rank"].get<int>();
        }
        if (item.contains("content") && item["content"].is_string()) {
            r.content = item["content"].get<std::string>();
        }
        results.push_back(std::move(r));
    }

    return results;
}

} // namespace themis
