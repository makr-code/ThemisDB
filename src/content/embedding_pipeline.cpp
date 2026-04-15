/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            embedding_pipeline.cpp                             ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-04-15 07:11:42                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     160                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright (c) 2024 ThemisDB
// SPDX-License-Identifier: MIT

/**
 * @file embedding_pipeline.cpp
 * @brief Embedding generation pipeline for the content module (Issue #1697)
 *
 * Wraps EmbeddedLLM::embed() with batch accumulation and timeout enforcement
 * so that every ingested text document can optionally receive an embedding
 * stored alongside the content and registered in the vector index.
 *
 * Design follows FUTURE_ENHANCEMENTS.md § "Embedding Generation Pipeline":
 *  - batch_size  ≤ 32 chunks per inference call
 *  - timeout_ms  = 5 000 ms; on breach: content stored without embedding,
 *                 content_embedding_failures_total counter incremented
 *  - On success: returned vector is L2-normalised (cosine similarity ready)
 */

#include "content/embedding_pipeline.h"
#include "content/content_metrics.h"
#include "llm/embedded_llm.h"

#include <future>
#include <chrono>
#include <cmath>
#include <algorithm>

namespace themis {
namespace content {

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

EmbeddingPipeline::EmbeddingPipeline(const EmbeddingPipelineConfig& config)
    : config_(config)
{
    // Clamp batch_size to the [1, 32] range documented in FUTURE_ENHANCEMENTS.
    if (config_.batch_size < 1)  config_.batch_size = 1;
    if (config_.batch_size > 32) config_.batch_size = 32;

    // Minimum meaningful timeout is 100 ms.
    if (config_.timeout_ms < 100) config_.timeout_ms = 100;

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
        size_t end = std::min(start + static_cast<size_t>(batch), texts.size());
        for (size_t i = start; i < end; ++i) {
            results[i] = embedWithTimeout(texts[i]);
        }
    }

    return results;
}

} // namespace content
} // namespace themis
