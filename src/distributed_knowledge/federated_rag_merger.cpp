/**
 * @file federated_rag_merger.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 93/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=7, H=21, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "distributed_knowledge/federated_rag_merger.h"

#include <algorithm>
#include <limits>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <queue>

namespace themis::distributed_knowledge {

// ─────────────────────────────────────────────────────────────────────────────
// MergedRAGContext::buildPromptContext
// ─────────────────────────────────────────────────────────────────────────────

std::string MergedRAGContext::buildPromptContext(size_t max_docs, size_t max_chars) const {
    std::ostringstream oss;
    size_t count = 0;
    size_t chars = 0;

    for (const auto &doc : documents) {
        if (max_docs > 0 && count >= max_docs) {
            break;
        }

        std::string snippet = "[Shard: " + doc.shard_id + "] " + doc.content;

        // Append entity summary if present
        const auto it = doc.metadata.find("_entities");
        if (it != doc.metadata.end() && !it->second.empty()) {
            snippet += "\n  Entities: " + it->second;
        }

        if (max_chars > 0 && chars + snippet.size() > max_chars) {
            // Truncate last document to fit budget
            const size_t remaining = max_chars - chars;
            if (remaining > 20) {
                snippet = snippet.substr(0, remaining - 3) + "...";
                oss << snippet << "\n\n";
            }
            break;
        }

        oss << snippet << "\n\n";
        chars += snippet.size() + 2;
        ++count;
    }

    return oss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// FederatedRAGMerger
// ─────────────────────────────────────────────────────────────────────────────

FederatedRAGMerger::FederatedRAGMerger(FederatedRAGMergerConfig config) : config_(std::move(config)) {
    if (!config_.isValid()) {
        throw std::invalid_argument("FederatedRAGMerger: invalid config");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// merge
// ─────────────────────────────────────────────────────────────────────────────

MergedRAGContext FederatedRAGMerger::merge(const std::vector<ShardRetrievalResult> &shard_results) const {
    // ── DK-OR-T: shard_timeout_ms handling ───────────────────────────────────
    // shard_timeout_ms == 0 → immediate timeout for all shards
    if (config_.shard_timeout_ms == 0 && !shard_results.empty()) {
        throw std::runtime_error("all shards timed out");
    }

    // Count timed-out shards (pre-resolved results with timed_out==true)
    if (config_.shard_timeout_ms != std::numeric_limits<size_t>::max()) {
        const size_t timed_out_count = static_cast<size_t>(std::count_if(
            shard_results.begin(), shard_results.end(), [](const ShardRetrievalResult &r) { return r.timed_out; }));
        if (!shard_results.empty() && timed_out_count == shard_results.size()) {
            throw std::runtime_error("all shards timed out");
        }
    }
    // ─────────────────────────────────────────────────────────────────────────

    MergedRAGContext ctx;
    ctx.strategy_used  = config_.strategy;
    ctx.shards_queried = shard_results.size();

    // Count candidates before merge (skip timed-out shards)
    for (const auto &sr : shard_results) {
        if (sr.ok && !sr.timed_out) {
            ++ctx.shards_responded;
            ctx.total_candidate_count += sr.documents.size();
        }
    }

    // Run merge strategy
    std::vector<RetrievedDocument> merged;
    switch (config_.strategy) {
        case MergeStrategy::RECIPROCAL_RANK_FUSION:
            merged = mergeRRF(shard_results);
            break;
        case MergeStrategy::SCORE_WEIGHTED:
            merged = mergeScoreWeighted(shard_results);
            break;
        case MergeStrategy::ROUND_ROBIN:
            merged = mergeRoundRobin(shard_results);
            break;
    }

    // Dedup
    if (config_.deduplicate) {
        merged = deduplicate(std::move(merged));
    }
    ctx.unique_doc_count = merged.size();

    // Trim to top_k
    if (merged.size() > config_.top_k) {
        merged.resize(config_.top_k);
    }

    ctx.documents = std::move(merged);
    return ctx;
}

std::string FederatedRAGMerger::mergeAndBuildContext(const std::vector<ShardRetrievalResult> &shard_results,
                                                     size_t max_docs, size_t max_chars) const {
    return merge(shard_results).buildPromptContext(max_docs, max_chars);
}

// ─────────────────────────────────────────────────────────────────────────────
// mergeRRF (internal) - QW-025: O(n²) → K-way merge with heap O(n log k)
// ─────────────────────────────────────────────────────────────────────────────

std::vector<RetrievedDocument> FederatedRAGMerger::mergeRRF(const std::vector<ShardRetrievalResult> &results) const {
    // QW-025: Efficient k-way merge using priority queue (max-heap)
    // Reduces complexity from O(n²) to O(n log k) where k = number of shards
    
    // Build initial heap of top results from each shard
    struct HeapEntry {
        double score;
        std::string doc_id;
        RetrievedDocument doc;
        size_t shard_idx;
        size_t doc_idx;
        
        // For max-heap: reverse comparison
        bool operator<(const HeapEntry& other) const {
            if (score != other.score) {
                return score < other.score;  // max-heap: greater scores have higher priority
            }
            return doc_id > other.doc_id;  // tie-break by lexical order
        }
    };
    
    std::priority_queue<HeapEntry> heap;
    std::unordered_map<std::string, double> rrf_scores;
    std::unordered_map<std::string, RetrievedDocument> best_doc;
    
    // Initialize heap with first document from each shard
    size_t shard_idx = 0;
    for (const auto &sr : results) {
        if (!sr.ok || sr.timed_out || sr.documents.empty()) {
            ++shard_idx;
            continue;
        }
        
        // Calculate RRF score for first document in this shard
        double shard_boost = 1.0;
        if (config_.boost_specialised && sr.adapter_accuracy_delta > 0.0) {
            shard_boost = config_.specialisation_boost;
        }
        
        const auto &doc = sr.documents[0];
        const double rrf = shard_boost / (config_.rrf_constant + 1.0);
        
        heap.push({rrf, doc.doc_id, doc, shard_idx, 0});
        rrf_scores[doc.doc_id] = rrf;
        best_doc[doc.doc_id] = doc;
        
        ++shard_idx;
    }
    
    // Pop-and-refill to merge all documents efficiently
    size_t total_merged = 0;
    const size_t max_merge_size = config_.top_k * 10;  // Reasonable limit for intermediate results
    
    while (!heap.empty() && total_merged < max_merge_size) {
        auto top = heap.top();
        heap.pop();
        
        // Refill from same shard's next document
        if (top.doc_idx + 1 < results[top.shard_idx].documents.size()) {
            const auto &sr = results[top.shard_idx];
            const auto &next_doc = sr.documents[top.doc_idx + 1];
            
            double shard_boost = 1.0;
            if (config_.boost_specialised && sr.adapter_accuracy_delta > 0.0) {
                shard_boost = config_.specialisation_boost;
            }
            
            const size_t next_rank = top.doc_idx + 2;
            const double rrf = shard_boost / (config_.rrf_constant + static_cast<double>(next_rank));
            
            heap.push({rrf, next_doc.doc_id, next_doc, top.shard_idx, top.doc_idx + 1});
            
            // Update accumulated score
            if (rrf_scores.find(next_doc.doc_id) == rrf_scores.end()) {
                rrf_scores[next_doc.doc_id] = rrf;
            } else {
                rrf_scores[next_doc.doc_id] += rrf;
            }
            
            // Keep best version of document
            if (best_doc.find(next_doc.doc_id) == best_doc.end()) {
                best_doc[next_doc.doc_id] = next_doc;
            }
        }
        
        ++total_merged;
    }
    
    // Collect results and sort by final RRF scores
    std::vector<RetrievedDocument> merged;
    merged.reserve(best_doc.size());
    for (auto &[doc_id, doc] : best_doc) {
        doc.relevance_score = rrf_scores[doc_id];
        merged.push_back(std::move(doc));
    }
    
    std::sort(merged.begin(), merged.end(), [](const RetrievedDocument &a, const RetrievedDocument &b) {
        return a.relevance_score > b.relevance_score;
    });
    
    return merged;
}

// ─────────────────────────────────────────────────────────────────────────────
// mergeScoreWeighted (internal)
// ─────────────────────────────────────────────────────────────────────────────

std::vector<RetrievedDocument>
FederatedRAGMerger::mergeScoreWeighted(const std::vector<ShardRetrievalResult> &results) const {
    std::unordered_map<std::string, double> sum_scores;
    std::unordered_map<std::string, RetrievedDocument> best_doc;

    for (const auto &sr : results) {
        if (!sr.ok || sr.timed_out) {
            continue;
        }

        const double shard_weight = 1.0 + (config_.boost_specialised ? sr.adapter_accuracy_delta : 0.0);

        for (const auto &doc : sr.documents) {
            sum_scores[doc.doc_id] += doc.relevance_score * std::max(0.01, shard_weight);
            if (!best_doc.count(doc.doc_id)) {
                best_doc[doc.doc_id] = doc;
            }
        }
    }

    std::vector<RetrievedDocument> merged;
    merged.reserve(sum_scores.size());
    for (auto &[doc_id, doc] : best_doc) {
        doc.relevance_score = sum_scores[doc_id];
        merged.push_back(std::move(doc));
    }
    std::sort(merged.begin(), merged.end(), [](const RetrievedDocument &a, const RetrievedDocument &b) {
        return a.relevance_score > b.relevance_score;
    });
    return merged;
}

// ─────────────────────────────────────────────────────────────────────────────
// mergeRoundRobin (internal)
// ─────────────────────────────────────────────────────────────────────────────

std::vector<RetrievedDocument>
FederatedRAGMerger::mergeRoundRobin(const std::vector<ShardRetrievalResult> &results) const {
    std::vector<const std::vector<RetrievedDocument> *> lists;
    for (const auto &sr : results) {
        if (sr.ok && !sr.timed_out && !sr.documents.empty()) {
            lists.push_back(&sr.documents);
        }
    }

    std::vector<RetrievedDocument> merged;
    bool any_remaining = true;
    size_t pos         = 0;

    while (any_remaining && merged.size() < config_.top_k * 2) {
        any_remaining = false;
        for (const auto *list : lists) {
            if (pos < list->size()) {
                merged.push_back((*list)[pos]);
                any_remaining = true;
            }
        }
        ++pos;
    }
    return merged;
}

// ─────────────────────────────────────────────────────────────────────────────
// deduplicate (internal)
// ─────────────────────────────────────────────────────────────────────────────

std::vector<RetrievedDocument> FederatedRAGMerger::deduplicate(std::vector<RetrievedDocument> docs) const {
    std::unordered_set<std::string> seen;
    std::vector<RetrievedDocument> result;
    result.reserve(docs.size());
    for (auto &doc : docs) {
        if (seen.insert(doc.doc_id).second) {
            result.push_back(std::move(doc));
        }
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// DK-OR: GDPR erase (clears cached merge context)
// ─────────────────────────────────────────────────────────────────────────────

themis::governance::StoreErasureResult FederatedRAGMerger::erase(const std::string & /*subject_id*/,
                                                                 themis::governance::Regulation /*regulation*/) {
    ++erase_count_;
    // FederatedRAGMerger is stateless (no cached merge contexts); erase is a
    // no-op beyond incrementing the counter for audit.
    themis::governance::StoreErasureResult result;
    result.store_id       = "FederatedRAGMerger";
    result.records_erased = 0;
    result.success        = true;
    return result;
}

} // namespace themis::distributed_knowledge
