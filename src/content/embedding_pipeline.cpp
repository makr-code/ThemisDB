/**
 * @file embedding_pipeline.cpp
 * @brief Embedding generation and management pipeline for content vectorization.
 * @version 0.0.15
 * @note Maturity: 🟡 BETA
 * @note Score: 73/100
 * @note Gap Summary: total=13; TODO=2, Stub=1, Unimpl=1, Mock=1, Sim=0, Debt=2, C=1, H=3, M=7, L=0
 * @note Status: Beta; Embedding pipeline scaffolding complete; model optimization and caching under test
 * @note This block is auto-generated and will be overwritten.
 */
#include "content/embedding_pipeline.h"
#include "content/content_metrics.h"
#include "llm/embedded_llm.h"

#include <future>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <exception>
#include <stdexcept>

namespace themis {
namespace content {

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

EmbeddingPipeline::EmbeddingPipeline(const EmbeddingPipelineConfig& config)
    : config_(config)
{
    // Clamp batch_size to the [1, 32] range documented in FUTURE_ENHANCEMENTS.
    if (config_.batch_size < 1) {
      config_.batch_size = 1;
    }
    if (config_.batch_size > 32) {
      config_.batch_size = 32;
    }

    // Minimum meaningful timeout is 100 ms.
    if (config_.timeout_ms < 100) {
      config_.timeout_ms = 100;
    }

    if (config_.embedding_dim > 0) {
        embedding_dim_.store(config_.embedding_dim);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

void EmbeddingPipeline::notifyFailure() const
{
    failure_count_.fetch_add(1, std::memory_order_relaxed);
    if (config_.metrics) {
        config_.metrics->recordEmbeddingFailure();
    }
}

std::vector<float> EmbeddingPipeline::embedWithTimeout(const std::string& text)
{
    if (text.empty()) {
        return {};
    }

    // Launch the embed call on a detached future so we can enforce a deadline.
    auto future = std::async(std::launch::async, [text]() -> std::vector<float> {
        return THEMIS_LLM().embed(text);
    });

    auto status = future.wait_for(std::chrono::milliseconds(config_.timeout_ms));
    if (status != std::future_status::ready) {
        // Timeout – count failure; the future destructor blocks until the
        // thread finishes (bounded by the OS scheduler).
        notifyFailure();
        return {};
    }

    try {
        auto embedding = future.get();
        if (embedding.empty()) {
            notifyFailure();
            return {};
        }

        // Record dimension on first successful call.
        int dim = static_cast<int>(embedding.size());
        int expected = embedding_dim_.load(std::memory_order_relaxed);
        if (expected == 0) {
            embedding_dim_.store(dim, std::memory_order_relaxed);
        }

        return embedding;
    } catch (...) {
        notifyFailure();
        return {};
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

std::vector<float> EmbeddingPipeline::generateEmbedding(const std::string& text)
{
    if (!isEnabled()) {
        return {};
    }
    return embedWithTimeout(text);
}

std::vector<std::vector<float>> EmbeddingPipeline::generateEmbeddingBatch(
    const std::vector<std::string>& texts)
{
    std::vector<std::vector<float>> results(texts.size());

    if (!isEnabled() || texts.empty()) {
        return results;
    }

    // Process in sub-batches of at most config_.batch_size to amortise model
    // overhead while staying within the documented batch_size ≤ 32 constraint.
    const int batch = config_.batch_size;
    for (size_t start = 0; start < texts.size(); start += static_cast<size_t>(batch)) {
        size_t end = std::min(start + static_cast<size_t>(batch),static_cast<int>(texts.size()));
        for (size_t i = start; i < end; ++i) {
            results[i] = embedWithTimeout(texts[i]);
        }
    }

    return results;
}

} // namespace content
} // namespace themis

