/**
 * @file embedding_pipeline.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright (c) 2024 ThemisDB
// SPDX-License-Identifier: MIT

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <chrono>
#include <functional>

// Forward declaration — avoids pulling in the full content_metrics.h header
// from headers that only include embedding_pipeline.h.
namespace themis { namespace content { class ContentMetrics; } }

namespace themis {
namespace content {

struct EmbeddingPipelineConfig {
    std::string model_name;

    int batch_size = 32;

    int timeout_ms = 5000;

    int embedding_dim = 0;

    ContentMetrics* metrics = nullptr;
};

class EmbeddingPipeline {
public:
    /**
     * @brief Embedding Pipeline.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit EmbeddingPipeline(const EmbeddingPipelineConfig& config);
    ~EmbeddingPipeline() = default;

    // Non-copyable, movable
    EmbeddingPipeline(const EmbeddingPipeline&) = delete;
    EmbeddingPipeline& operator=(const EmbeddingPipeline&) = delete;
    EmbeddingPipeline(EmbeddingPipeline&&) noexcept = default;
    EmbeddingPipeline& operator=(EmbeddingPipeline&&) noexcept = default;

    /**
     * @brief Generate Embedding.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    std::vector<float> generateEmbedding(const std::string& text);

    /**
     * @brief Generate Embedding Batch.
     * @param[in] texts Input parameter.
     * @return Return value.
     */
    std::vector<std::vector<float>> generateEmbeddingBatch(
        const std::vector<std::string>& texts);

    bool isEnabled() const { return !config_.model_name.empty(); }

    uint64_t getFailureCount() const { return failure_count_.load(); }

    int getEmbeddingDim() const { return embedding_dim_.load(); }

    const EmbeddingPipelineConfig& getConfig() const { return config_; }

private:
    EmbeddingPipelineConfig config_;
    mutable std::atomic<uint64_t> failure_count_{0};
    mutable std::atomic<int> embedding_dim_{0};

    /**
     * @brief Embed With Timeout.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    std::vector<float> embedWithTimeout(const std::string& text);

    /**
     * @brief Notify Failure.
     */
    void notifyFailure() const;
};

} // namespace content
} // namespace themis

