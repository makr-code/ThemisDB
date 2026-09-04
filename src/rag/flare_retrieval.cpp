/**
 * @file flare_retrieval.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=0, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/flare_retrieval.h"

#include <algorithm>
#include <cstdio>
#include <mutex>
#include <numeric>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace rag {

// ============================================================================
// EmbeddingQueryFn injection bridge
// ============================================================================

static std::mutex& embeddingQueryFnMutex() { static std::mutex m; return m; }
static FlareRetrieval::EmbeddingQueryFn& embeddingQueryFnStorage() {
    static FlareRetrieval::EmbeddingQueryFn fn;
    return fn;
}

/*static*/
void FlareRetrieval::setEmbeddingQueryFn(EmbeddingQueryFn fn) {
    std::lock_guard<std::mutex> lk(embeddingQueryFnMutex());
    embeddingQueryFnStorage() = std::move(fn);
}

// ============================================================================
// Constructor
// ============================================================================

FlareRetrieval::FlareRetrieval(FlareConfig cfg)
    : cfg_(std::move(cfg))
{
    window_.reserve(cfg_.query_window_tokens > 0 ? cfg_.query_window_tokens : 64);
    // Initialise log-prob stats to sentinels that will be overwritten on first token.
    stats_.min_log_prob = 0.0f;
    stats_.max_log_prob = 0.0f;
}

// ============================================================================
// notifyTokenEmitted
// ============================================================================

void FlareRetrieval::notifyTokenEmitted(const std::string& token_text,
                                        float              log_prob) {
    // ── Maintain sliding window ──────────────────────────────────────────
    const bool uncertain = (log_prob < cfg_.confidence_threshold);
    TokenEntry entry{token_text, log_prob, uncertain};

    if (cfg_.query_window_tokens > 0 &&
        static_cast<int>(window_.size()) >= cfg_.query_window_tokens) {
        window_.erase(window_.begin());
    }
    window_.push_back(std::move(entry));

    // ── Update statistics ────────────────────────────────────────────────
    ++stats_.tokens_emitted;

    // Welford online mean update.
    stats_.mean_log_prob +=
        (static_cast<double>(log_prob) - stats_.mean_log_prob)
        / static_cast<double>(stats_.tokens_emitted);

    if (stats_.tokens_emitted == 1) {
        stats_.min_log_prob = log_prob;
        stats_.max_log_prob = log_prob;
    } else {
        if (log_prob < stats_.min_log_prob) {
          stats_.min_log_prob = log_prob;
        }
        if (log_prob > stats_.max_log_prob) {
          stats_.max_log_prob = log_prob;
        }
    }

    // ── Uncertainty tracking ─────────────────────────────────────────────
    if (uncertain) {
        ++consecutive_uncertain_;
    } else {
        consecutive_uncertain_ = 0;
    }

    // ── Advance cooldown ─────────────────────────────────────────────────
    if (cooldown_remaining_ > 0) {
        --cooldown_remaining_;
    }

    // ── Determine pending_retrieval_ ─────────────────────────────────────
    pending_retrieval_ = false;

    if (cooldown_remaining_ > 0) {
        ++stats_.cooldown_skips;
        return;
    }

    if (cfg_.max_retrieval_steps > 0 &&
        retrieval_steps_done_ >= cfg_.max_retrieval_steps) {
        ++stats_.max_steps_skips;
        return;
    }

    if (consecutive_uncertain_ >= cfg_.min_consecutive_uncertain) {
        pending_retrieval_ = true;
        ++stats_.retrieval_triggers;
    }
}

// ============================================================================
// decide
// ============================================================================

FlareDecision FlareRetrieval::decide() const noexcept {
    FlareDecision d;
    d.should_retrieve    = pending_retrieval_;
    d.in_cooldown        = (cooldown_remaining_ > 0);
    d.max_steps_reached  = (cfg_.max_retrieval_steps > 0 &&
                             retrieval_steps_done_ >= cfg_.max_retrieval_steps);
    d.last_log_prob      = window_.empty() ? 0.0f : window_.back().log_prob;
    d.consecutive_uncertain = consecutive_uncertain_;
    d.retrieval_steps_done  = retrieval_steps_done_;
    return d;
}

// ============================================================================
// shouldRetrieve
// ============================================================================

bool FlareRetrieval::shouldRetrieve() const noexcept {
    return pending_retrieval_;
}

// ============================================================================
// buildQuery
// ============================================================================

std::string FlareRetrieval::buildQuery() const {
    if (window_.empty()) return {};

    // Implementation note:
    // Purpose: join token text with spaces; uncertain tokens replaced by mask.
    //          Real implementation should call buildQueryEmbedding() which uses
    //          the injected EmbeddingQueryFn to produce a float vector query.
    // Activation: Always (string path is the canonical public API).
    // Production Delta: String-based query vs. semantic embedding vector.
    // Status: EmbeddingQueryFn bridge is available via setEmbeddingQueryFn().

    std::ostringstream oss = {};
    bool first = true;
    for (const auto& entry : window_) {
        if (!first) {
          oss << ' ';
        }
        first = false;
        if (cfg_.mask_uncertain_tokens && entry.uncertain) {
            oss << cfg_.mask_token;
        } else {
            oss << entry.text;
        }
    }
    return oss.str();
}

// ============================================================================
// buildQueryEmbedding
// ============================================================================

std::vector<float> FlareRetrieval::buildQueryEmbedding() const {
    EmbeddingQueryFn fn_copy;
    {
        std::lock_guard<std::mutex> lk(embeddingQueryFnMutex());
        fn_copy = embeddingQueryFnStorage();
    }
    if (!fn_copy) {
        // No embedding backend injected — return empty (caller must embed).
        return {};
    }
    try {
        return fn_copy(buildQuery());
    } catch (...) {
        // Fail-closed: embedding fn threw; return empty rather than propagating.
        // This is distinct from the "no fn wired" path above — the backend is
        // registered but failed at runtime. Operators should diagnose the root cause.
        std::fprintf(stderr,
            "[ThemisDB][WARN] FlareRetrieval::buildQueryEmbedding: EmbeddingQueryFn "
            "threw; returning empty embedding (fail-closed).\n");
        return {};
    }
}

// ============================================================================
// notifyRetrievalExecuted
// ============================================================================

void FlareRetrieval::notifyRetrievalExecuted() {
    consecutive_uncertain_ = 0;
    pending_retrieval_     = false;
    cooldown_remaining_    = cfg_.retrieval_cooldown_tokens;
    ++retrieval_steps_done_;
}

// ============================================================================
// reset
// ============================================================================

void FlareRetrieval::reset() {
    window_.clear();
    consecutive_uncertain_ = 0;
    cooldown_remaining_    = 0;
    retrieval_steps_done_  = 0;
    pending_retrieval_     = false;
    stats_                 = {};
}

} // namespace rag
} // namespace themis

