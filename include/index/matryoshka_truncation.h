/**
 * @file matryoshka_truncation.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

// Matryoshka Representation Learning (MRL) truncation utilities
//
// MRL (Kusupati et al., NeurIPS 2022) trains embeddings such that every
// prefix of length d < D is itself a useful d-dimensional representation.
// A single full-dimensional embedding can therefore be truncated to any
// standard granularity without retraining, enabling:
//
//   • Multi-stage retrieval: cheap low-D ANN pre-filter → full-D re-rank.
//   • Adaptive precision: 64D for rough candidate selection, 768D for scoring.
//   • Compact indexes: smaller index fits in RAM → faster queries on large corpora.
//
// This file provides two building blocks:
//   1. MatryoshkaTruncation   – stateless helper that truncates + normalises.
//   2. MatryoshkaTruncatedIndex – IAnnIndex wrapper that applies truncation
//      transparently so any existing ANN backend (ScaNN, DiskAnnAdapter, HNSW,
//      etc.) can be used with Matryoshka embeddings.
//
// References:
//   Kusupati, A. et al. "Matryoshka Representation Learning."
//   Advances in Neural Information Processing Systems 35 (NeurIPS 2022).
//   https://arxiv.org/abs/2205.13147
//
//   OpenAI text-embedding-3, Nomic Embed v1.5, and BGE-M3 all ship with
//   native MRL support using the granularities defined as kMRL_* constants
//   below.

#include "index/ann_index.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace themis {
namespace index {

// ---------------------------------------------------------------------------
// Standard Matryoshka granularities
//
// These match the natively-supported prefix lengths of common MRL-trained
// models (OpenAI text-embedding-3-small/-large, Nomic Embed v1.5, BGE-M3).
// Any prefix length is valid, but these are the ones that maximise recall
// at each level according to the original paper.
// ---------------------------------------------------------------------------

inline constexpr size_t kMRL_64   =   64;
inline constexpr size_t kMRL_128  =  128;
inline constexpr size_t kMRL_256  =  256;
inline constexpr size_t kMRL_512  =  512;
inline constexpr size_t kMRL_768  =  768;
inline constexpr size_t kMRL_1024 = 1024;
inline constexpr size_t kMRL_1536 = 1536;

// ---------------------------------------------------------------------------
// MatryoshkaTruncation
//
// Stateless utility that truncates a full-dimensional MRL embedding to the
// first `trunc_dim` dimensions and, optionally, L2-normalises the result so
// that dot-product similarity equals cosine similarity.
// ---------------------------------------------------------------------------

/// Stateless Matryoshka prefix-truncation helper.
class MatryoshkaTruncation {
public:
    /// @param trunc_dim  Target dimensionality after truncation (must be > 0).
    /// @param normalize  If true (default), L2-normalise the truncated vector so
    ///                   that cosine similarity == dot product on the result.
    explicit MatryoshkaTruncation(size_t trunc_dim, bool normalize = true)
        : trunc_dim_(trunc_dim), normalize_(normalize)
    {
        if (trunc_dim_ == 0)
            throw std::invalid_argument(
                "MatryoshkaTruncation: trunc_dim must be > 0");
    }

    /// Truncate @p vector (raw array of length @p full_dim) to a vector of
    /// length trunc_dim_. If @p full_dim >= trunc_dim_, only the first
    /// trunc_dim_ elements are kept. If @p full_dim < trunc_dim_, all
    /// available elements are copied and the remaining positions are
    /// zero-padded up to trunc_dim_, regardless of normalize_.
    [[nodiscard]] std::vector<float> truncate(const float* vector,
                                               size_t full_dim) const
    {
        const size_t out_dim = std::min(full_dim, trunc_dim_);
        std::vector<float> out(trunc_dim_, 0.0f);
        for (size_t i = 0; i < out_dim; ++i)
            out[i] = vector[i];

        if (normalize_) {
            float norm = 0.0f;
            for (size_t i = 0; i < trunc_dim_; ++i)
                norm += out[i] * out[i];
            norm = std::sqrt(norm);
            if (norm > 1e-10f) {
                const float inv = 1.0f / norm;
                for (size_t i = 0; i < trunc_dim_; ++i)
                    out[i] *= inv;
            }
        }
        return out;
    }

    /// Convenience overload for std::vector input.
    [[nodiscard]] std::vector<float> truncate(
            const std::vector<float>& vector) const
    {
        return truncate(vector.data(), vector.size());
    }

    // Legacy compatibility alias.
    [[nodiscard]] std::vector<float> truncateAndNormalize(
            const float* vector,
            size_t full_dim) const
    {
        return truncate(vector, full_dim);
    }

    // Legacy compatibility alias.
    [[nodiscard]] std::vector<float> truncateAndNormalize(
            const std::vector<float>& vector) const
    {
        return truncate(vector);
    }

    size_t trunc_dim()  const noexcept { return trunc_dim_; }
    bool   normalize()  const noexcept { return normalize_; }

private:
    size_t trunc_dim_;
    bool   normalize_;
};

// ---------------------------------------------------------------------------
// MatryoshkaTruncatedIndex
//
// IAnnIndex decorator that transparently truncates every incoming vector
// (build, add, search) before forwarding to the wrapped backend index.
//
// Typical usage — two-stage retrieval pipeline:
//
//   // 1. Build a compact 128-D index over 1M full-768-D embeddings
//   auto backend = std::make_shared<ScaNN>();
//   auto idx     = std::make_shared<MatryoshkaTruncatedIndex>(
//                      backend, kMRL_128);
//   idx->build(raw_embeddings_768d, ids, n, 768);
//
//   // 2. Fast first-stage ANN (128-D search)
//   auto candidates = idx->search(query_768d, 768, /*k=*/100);
//
//   // 3. Re-rank candidates using the caller's full 768-D vectors
//   //    (MatryoshkaTruncatedIndex only handles pre-filtering here)
// ---------------------------------------------------------------------------

/// IAnnIndex wrapper that applies MatryoshkaTruncation before forwarding to
/// the underlying backend.
class MatryoshkaTruncatedIndex final : public IAnnIndex {
public:
    /// @param backend   The wrapped ANN index.  Must not be null.
    /// @param trunc_dim Target dimensionality passed to MatryoshkaTruncation.
    /// @param normalize Whether to L2-normalise after truncation (default: true).
    MatryoshkaTruncatedIndex(std::shared_ptr<IAnnIndex> backend,
                              size_t trunc_dim,
                              bool normalize = true)
        : backend_(std::move(backend))
        , trunc_(trunc_dim, normalize)
    {
        if (!backend_)
            throw std::invalid_argument(
                "MatryoshkaTruncatedIndex: backend must not be null");
    }

    /// Build the backend index from @p count vectors of @p full_dim floats.
    /// Each vector is truncated to trunc_dim before being passed to the backend.
    bool build(const float* vectors, const int64_t* ids,
               size_t count, size_t full_dim) override
    {
        // Empty dataset: treat as a successful no-op to keep semantics consistent
        // across backends (some, e.g. ScaNN, return false for count == 0).
        if (count == 0) {
          return true;
        }

        // Materialise the truncated flat array once and hand it to the backend.
        const size_t td = trunc_.trunc_dim();
        std::vector<float> buf(count * td);
        for (size_t i = 0; i < count; ++i) {
            auto t = trunc_.truncate(vectors + i * full_dim, full_dim);
            std::copy(t.begin(), t.end(), buf.begin() + static_cast<std::ptrdiff_t>(i * td));
        }
        return backend_->build(buf.data(), ids, count, td);
    }

    /// Add a single vector (length @p full_dim) to the index.
    bool add(int64_t id, const float* vector, size_t full_dim) override
    {
        auto t = trunc_.truncate(vector, full_dim);
        return backend_->add(id, t.data(), t.size());
    }

    /// Search for the @p k nearest neighbours of @p query (length @p full_dim).
    std::vector<AnnSearchResult> search(const float* query, size_t full_dim,
                                         int k) const override
    {
        auto t = trunc_.truncate(query, full_dim);
        return backend_->search(t.data(), t.size(), k);
    }

    bool save(const std::string& path) const override
    {
        return backend_->save(path);
    }

    bool load(const std::string& path) override
    {
        return backend_->load(path);
    }

    size_t size() const override { return backend_->size(); }

    /// Access the underlying truncation parameters.
    const MatryoshkaTruncation& truncation() const noexcept { return trunc_; }

    /// Access the wrapped backend index.
    IAnnIndex& backend() { return *backend_; }
    const IAnnIndex& backend() const { return *backend_; }

private:
    std::shared_ptr<IAnnIndex> backend_;
    MatryoshkaTruncation       trunc_;
};

} // namespace index
} // namespace themis
