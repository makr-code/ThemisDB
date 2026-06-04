/**
 * @file self_rag.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/self_rag.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace themis {
namespace rag {
namespace {

std::string normalizeToken(std::string token) {
    token.erase(std::remove_if(token.begin(), token.end(), [](unsigned char ch) {
                    return !std::isalnum(ch);
                }),
                token.end());
    std::transform(token.begin(), token.end(), token.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return token;
}

std::vector<std::string> tokenizeNormalized(const std::string& text) {
    std::istringstream iss(text);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) {
        token = normalizeToken(token);
        if (!token.empty()) {
            tokens.push_back(std::move(token));
        }
    }
    return tokens;
}

double lexicalOverlapScore(const std::string& query, const std::string& content) {
    const auto q_tokens = tokenizeNormalized(query);
    if (q_tokens.empty()) return 0.0;

    const auto d_tokens = tokenizeNormalized(content);
    if (d_tokens.empty()) return 0.0;

    std::unordered_set<std::string> doc_terms(d_tokens.begin(), d_tokens.end());
    size_t overlap = 0;
    for (const auto& t : q_tokens) {
        if (doc_terms.count(t) > 0) {
            ++overlap;
        }
    }
    return static_cast<double>(overlap) / static_cast<double>(q_tokens.size());
}

double clamp01(double v) {
    return std::max(0.0, std::min(1.0, v));
}

} // namespace

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

bool SelfRAGController::shouldRetrieve(const std::string& query,
                                        double             query_confidence) const {
    const double confidence = clamp01(query_confidence);
    if (confidence < cfg_.retrieval_confidence_threshold) {
        return true;
    }

    const auto query_tokens = tokenizeNormalized(query);
    if (query_tokens.empty()) {
        return false;
    }

    static const std::unordered_set<std::string> evidence_terms = {
        "who", "what", "when", "where", "which", "why", "how",
        "source", "sources", "citation", "citations", "evidence",
        "compare", "benchmark", "metrics", "according"
    };

    size_t evidence_hits = 0;
    for (const auto& token : query_tokens) {
        if (evidence_terms.count(token) > 0) {
            ++evidence_hits;
        }
    }

    const double evidence_cutoff = std::min(0.95, cfg_.retrieval_confidence_threshold + 0.25);
    if (evidence_hits > 0 && confidence < evidence_cutoff) {
        return true;
    }

    const double long_query_cutoff = std::min(0.98, cfg_.retrieval_confidence_threshold + 0.35);
    if (query_tokens.size() >= 14 && confidence < long_query_cutoff) {
        return true;
    }

    return false;
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
            critic_score = clamp01(critic_cb_(query, doc));
        } else {
            const double retrieval_signal = clamp01(doc.score);
            const double overlap_signal   = lexicalOverlapScore(query, doc.content);
            critic_score = clamp01(0.65 * retrieval_signal + 0.35 * overlap_signal);
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
