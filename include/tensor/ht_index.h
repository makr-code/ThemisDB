/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tensor/ht_index.h                                  ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-07                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🟡 EXPERIMENTAL — Phase 5 (Q1 2028)                         ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file tensor/ht_index.h
 * @brief IHierarchicalTuckerIndex interface and FlatHTIndex linear-scan implementation.
 *
 * Mirrors the ITensorIndex / FlatTensorIndex API for Hierarchical Tucker tensors.
 * All cosine similarity computations operate in the compressed HT domain via
 * HTContractionEngine — no full tensor reconstruction is required.
 */

#pragma once

#include "tensor/ht_train.h"

#include <cstddef>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace themis {
namespace tensor {

// ============================================================================
// SearchResult — one k-NN result
// ============================================================================

struct HTSearchResult {
    std::string id;          ///< User-provided identifier for the stored tensor
    double      similarity;  ///< Cosine similarity ∈ [−1, 1]
};

// ============================================================================
// IHierarchicalTuckerIndex — abstract interface
// ============================================================================

/**
 * @brief Abstract interface for a Hierarchical Tucker (HT) vector index.
 *
 * Implementations may use linear scan (FlatHTIndex) or tree-based search
 * structures (HnswHTBridge, deferred to Q2 2028).
 */
class IHierarchicalTuckerIndex {
public:
    virtual ~IHierarchicalTuckerIndex() = default;

    // ── Mutation ──────────────────────────────────────────────────────────────

    /**
     * @brief Add an HT tensor to the index.
     *
     * @param id     Unique string identifier.  Replaces any existing entry with
     *               the same id.
     * @param train  HT representation of the tensor to add.
     */
    virtual void add(const std::string& id, HTTrain train) = 0;

    /**
     * @brief Remove an entry from the index.
     * @return true if the entry existed and was removed; false otherwise.
     */
    virtual bool remove(const std::string& id) = 0;

    // ── Query ─────────────────────────────────────────────────────────────────

    /**
     * @brief k-nearest-neighbour search by cosine similarity.
     *
     * @param query     HTTrain encoding the query tensor.
     * @param k         Maximum number of results.
     * @return Up to k results in descending similarity order.
     */
    virtual std::vector<HTSearchResult>
    search(const HTTrain& query, std::size_t k) const = 0;

    /// Number of tensors currently stored.
    virtual std::size_t size() const = 0;

    // ── Persistence ───────────────────────────────────────────────────────────

    /**
     * @brief Serialise the full index to a byte blob.
     */
    virtual std::vector<uint8_t> serialize() const = 0;

    /**
     * @brief Deserialise in-place from bytes.
     * @return true on success; false on format error.
     */
    virtual bool deserialize(const std::vector<uint8_t>& bytes) = 0;
};

// ============================================================================
// FlatHTIndex — linear-scan implementation
// ============================================================================

/**
 * @brief Linear-scan Hierarchical Tucker index (FlatHTIndex).
 *
 * Computes exact cosine similarity for every stored entry on each query;
 * suitable for up to ~10 000 entries at modest rank.
 *
 * Thread safety: concurrent reads are safe; writes are serialised by an
 * internal mutex.
 */
class FlatHTIndex final : public IHierarchicalTuckerIndex {
public:
    FlatHTIndex() = default;
    ~FlatHTIndex() override = default;

    // ── IHierarchicalTuckerIndex ───────────────────────────────────────────────

    void add(const std::string& id, HTTrain train) override;
    bool remove(const std::string& id) override;

    std::vector<HTSearchResult>
    search(const HTTrain& query, std::size_t k) const override;

    std::size_t size() const override;

    std::vector<uint8_t> serialize()  const override;
    bool                 deserialize(const std::vector<uint8_t>& bytes) override;

    // ── Extra accessors ────────────────────────────────────────────────────────

    /// Retrieve an HTTrain by id; returns nullopt if not found.
    std::optional<const HTTrain*> get(const std::string& id) const;

private:
    struct Entry {
        std::string id;
        HTTrain     train;
    };

    std::vector<Entry>   entries_;
    mutable std::mutex   mutex_;
};

} // namespace tensor
} // namespace themis
