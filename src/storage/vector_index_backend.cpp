/*
 * ThemisDB | File: vector_index_backend.cpp | Version: 0.0.1 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 210
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=32 | delta=29 | status=divergent
 * External Severity (v3): C=0, H=30, M=2
 * PR: none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// Copyright (c) 2025-2026 ThemisDB Project
// SPDX-License-Identifier: Apache-2.0
//

#include "storage/vector_index_backend.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace themis {
namespace storage {

// ============================================================================
// InMemoryVectorIndex — construction
// ============================================================================

InMemoryVectorIndex::InMemoryVectorIndex(const VectorIndexConfig& cfg)
    : cfg_(cfg)
{
    if (cfg_.dim == 0) {
        throw std::invalid_argument("VectorIndexConfig::dim must be > 0");
    }
    vectors_.reserve(cfg_.max_elements);
}

// ============================================================================
// add()
// ============================================================================

void InMemoryVectorIndex::add(const std::string& id,
                              const std::vector<float>& embedding)
{
    if (embedding.size() != cfg_.dim) {
        throw std::invalid_argument(
            "Embedding dimension mismatch: expected " +
            std::to_string(cfg_.dim) + ", got " +
            std::to_string(embedding.size()));
    }

    std::vector<float> stored = embedding;
    if (cfg_.metric == DistanceMetric::COSINE) {
        normalise(stored);
    }

    std::lock_guard<std::mutex> lk(mutex_);
    vectors_[id] = std::move(stored);
}

// ============================================================================
// search()
// ============================================================================

std::vector<KnnResult>
InMemoryVectorIndex::search(const std::vector<float>& query,
                             std::size_t k) const
{
    if (query.size() != cfg_.dim) {
        throw std::invalid_argument(
            "Query dimension mismatch: expected " +
            std::to_string(cfg_.dim) + ", got " +
            std::to_string(query.size()));
    }
    if (k == 0) {
        return {};
    }

    // Normalise query for COSINE metric.
    std::vector<float> q = query;
    if (cfg_.metric == DistanceMetric::COSINE) {
        normalise(q);
    }

    std::vector<KnnResult> results;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        results.reserve(vectors_.size());
        for (const auto& [id, vec] : vectors_) {
            float dist = computeDistance(q, vec);
            results.push_back({id, dist, toScore(dist)});
        }
    }

    // Sort by ascending distance; for DOT_PRODUCT/COSINE, lower raw distance
    // means higher similarity because we negate the dot product.
    std::sort(results.begin(), results.end(),
              [](const KnnResult& a, const KnnResult& b) {
                  return a.distance < b.distance;
              });

    if (results.size() > k) {
        results.resize(k);
    }
    return results;
}

// ============================================================================
// remove()
// ============================================================================

void InMemoryVectorIndex::remove(const std::string& id)
{
    std::lock_guard<std::mutex> lk(mutex_);
    vectors_.erase(id);
}

// ============================================================================
// size()
// ============================================================================

std::size_t InMemoryVectorIndex::size() const noexcept
{
    std::lock_guard<std::mutex> lk(mutex_);
    return vectors_.size();
}

// ============================================================================
// config()
// ============================================================================

const VectorIndexConfig& InMemoryVectorIndex::config() const noexcept
{
    return cfg_;
}

// ============================================================================
// computeDistance() — private
// ============================================================================

float InMemoryVectorIndex::computeDistance(const std::vector<float>& a,
                                            const std::vector<float>& b) const noexcept
{
    float result = 0.0f;
    switch (cfg_.metric) {
    case DistanceMetric::L2: {
        for (std::size_t i = 0; i < cfg_.dim; ++i) {
            float d = a[i] - b[i];
            result += d * d;
        }
        break;
    }
    case DistanceMetric::DOT_PRODUCT:
    case DistanceMetric::COSINE: {
        // For COSINE, vectors are pre-normalised; dot product == cosine.
        // We negate so that higher similarity → lower "distance" →
        // natural ascending sort gives best matches first.
        float dot = 0.0f;
        for (std::size_t i = 0; i < cfg_.dim; ++i) {
            dot += a[i] * b[i];
        }
        result = -dot;  // negate: distance = -similarity
        break;
    }
    }
    return result;
}

// ============================================================================
// normalise() — private static
// ============================================================================

void InMemoryVectorIndex::normalise(std::vector<float>& v) noexcept
{
    float norm = 0.0f;
    for (float x : v) {
        norm += x * x;
    }
    if (norm <= 0.0f) {
        return;  // zero vector — leave unchanged
    }
    norm = std::sqrt(norm);
    for (float& x : v) {
        x /= norm;
    }
}

// ============================================================================
// toScore() — private
// ============================================================================

float InMemoryVectorIndex::toScore(float distance) const noexcept
{
    switch (cfg_.metric) {
    case DistanceMetric::L2:
        // score = 1 / (1 + L2_distance); ∈ (0, 1]
        return 1.0f / (1.0f + distance);

    case DistanceMetric::DOT_PRODUCT:
    case DistanceMetric::COSINE: {
        // distance = -dot; dot ∈ [-1, 1] → score = (dot + 1) / 2 ∈ [0, 1]
        float dot = -distance;
        float s = (dot + 1.0f) * 0.5f;
        if (s < 0.0f) { s = 0.0f; }
        if (s > 1.0f) { s = 1.0f; }
        return s;
    }
    }
    return 0.0f;
}

} // namespace storage
} // namespace themis
