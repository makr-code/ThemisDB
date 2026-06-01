/*
 * ThemisDB | File: self_rag.cpp | Version: 1.0.0 | Last Modified: 2026-06-01
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 94/100 | Lines: 182
 * Gap Summary: total=2; TODO=0, Stub=2, Unimpl=0, Mock=0, Sim=0, Debt=0, C=1, H=1, M=0, L=0
 * Status: Production Ready
 * (Wave B — issue #5039)
 */

/**
 * @file rag/self_rag.cpp
 * @brief Self-RAG controller implementation (Wave B B1).
 *
 * ### Stub notes
 *
 * SRG-S01  shouldRetrieve() uses a simple threshold heuristic on the caller-
 *          supplied confidence value.  A production implementation would run
 *          a lightweight binary classifier trained on (query, context) pairs
 *          to predict whether retrieval is beneficial.  Deferred to Phase 3
 *          (Q1 2027) when the embedding pipeline is available.
 *
 * SRG-S02  criticDocuments() uses the document's retrieval score as a proxy
 *          for critic confidence.  A production implementation would run a
 *          fine-tuned NLI model scoring (query, passage) relevance.  Callers
 *          may inject a trained CriticCallback to override this behaviour.
 */

#include "rag/self_rag.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace themis {
namespace rag {

// ============================================================================
// Constructor / Destructor
// ============================================================================

SelfRAGController::SelfRAGController(SelfRAGConfig cfg)
    : cfg_(std::move(cfg))
{
    seen_ids_.reserve(cfg_.max_rounds * cfg_.top_k);
}

SelfRAGController::~SelfRAGController() = default;

// ============================================================================
// Callback injection
// ============================================================================

void SelfRAGController::setRetrievalCallback(RetrievalCallback cb) {
    retrieval_cb_ = std::move(cb);
}

void SelfRAGController::setCriticCallback(CriticCallback cb) {
    critic_cb_ = std::move(cb);
}

// ============================================================================
// shouldRetrieve
// ============================================================================

bool SelfRAGController::shouldRetrieve(const std::string& /*query*/,
                                        double             query_confidence) const {
    // Stub SRG-S01: threshold heuristic; replace with learned classifier.
    return query_confidence < cfg_.retrieval_confidence_threshold;
}

// ============================================================================
// criticDocuments
// ============================================================================

std::vector<RatedDocument> SelfRAGController::criticDocuments(
        const std::string&                  query,
        const std::vector<SelfRAGDocument>& documents) const
{
    std::vector<RatedDocument> rated;
    rated.reserve(documents.size());

    for (const auto& doc : documents) {
        double critic_score;

        if (critic_cb_) {
            // Caller-injected critic model (production path).
            critic_score = critic_cb_(query, doc);
        } else {
            // Stub SRG-S02: use retrieval score normalised to [0,1].
            critic_score = std::max(0.0, std::min(1.0, doc.score));
        }

        CriticVerdict verdict;
        if (critic_score >= cfg_.relevant_threshold) {
            verdict = CriticVerdict::Relevant;
        } else if (critic_score >= cfg_.partial_threshold) {
            verdict = CriticVerdict::Partial;
        } else {
            verdict = CriticVerdict::Irrelevant;
        }

        rated.push_back({doc, verdict, critic_score});
    }

    return rated;
}

// ============================================================================
// Helpers
// ============================================================================

std::vector<SelfRAGDocument> SelfRAGController::deduplicate(
        std::vector<SelfRAGDocument> candidates) const
{
    std::unordered_set<std::string> seen(seen_ids_.begin(), seen_ids_.end());
    std::vector<SelfRAGDocument>    fresh;
    fresh.reserve(candidates.size());

    for (auto& doc : candidates) {
        if (seen.insert(doc.id).second) {
            fresh.push_back(std::move(doc));
        }
    }
    return fresh;
}

// ============================================================================
// runRefinementLoop
// ============================================================================

SelfRAGResult SelfRAGController::runRefinementLoop(const std::string& query,
                                                    double             query_confidence)
{
    SelfRAGResult result;

    if (!shouldRetrieve(query, query_confidence)) {
        return result;  // retrieval_triggered stays false
    }

    result.retrieval_triggered = true;

    if (!retrieval_cb_) {
        throw std::runtime_error(
            "SelfRAGController: no retrieval callback set; "
            "call setRetrievalCallback() before runRefinementLoop()");
    }

    size_t relevant_count = 0;

    for (size_t round = 1; round <= cfg_.max_rounds; ++round) {
        // Retrieve documents for this round.
        auto candidates = retrieval_cb_(query, cfg_.top_k);
        auto fresh      = deduplicate(std::move(candidates));

        // Track seen ids to avoid re-scoring the same passages.
        for (const auto& d : fresh) {
            seen_ids_.push_back(d.id);
        }

        // Critic pass.
        auto rated = criticDocuments(query, fresh);

        // Accumulate per-round stats.
        RefinementRoundStats stats;
        stats.round     = round;
        stats.retrieved = fresh.size();

        for (const auto& r : rated) {
            switch (r.verdict) {
                case CriticVerdict::Relevant:
                    result.relevant_docs.push_back(r);
                    ++stats.relevant;
                    ++relevant_count;
                    break;
                case CriticVerdict::Partial:
                    result.partial_docs.push_back(r);
                    ++stats.partial;
                    break;
                case CriticVerdict::Irrelevant:
                    ++stats.irrelevant;
                    break;
            }
        }

        // Check early-stop target.
        if (relevant_count >= cfg_.target_relevant_docs) {
            stats.stop_early = true;
            result.round_stats.push_back(stats);
            result.total_rounds_used = round;
            return result;
        }

        result.round_stats.push_back(stats);
        result.total_rounds_used = round;
    }

    return result;
}

// ============================================================================
// reset
// ============================================================================

void SelfRAGController::reset() {
    seen_ids_.clear();
}

} // namespace rag
} // namespace themis
