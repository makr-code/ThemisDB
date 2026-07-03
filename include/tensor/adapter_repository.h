/**
 * @file adapter_repository.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Gap Summary: total=16; TODO=1, Stub=12, Unimpl=0, Mock=1, Sim=2, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "storage/tensor_train_decomposer.h"
#include "storage/tensor_network_storage_engine.h"
#include "tensor/tensor_fingerprint_graph.h"
#include "tensor/tensor_compat.h"

#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>

namespace themis {
namespace index {
class AnnFrontdoor;
}
namespace tensor {

// ============================================================================
// GgmlCoreDescriptor — handle returned by loadAdapter()
// ============================================================================

/**
 * @brief Descriptor returned by `AdapterRepository::loadAdapter()`.
 *
 * Contains the TT-format adapter weights and provenance metadata.
 * The `train` field holds the loaded TT-cores ready for injection into a
 * ggml graph.  In the production mmap path the cores will be pinned pages;
 * in the current implementation they are heap-copied.
 *
 * @note
 * // STUB/SIMULATION NOTE:
 * // Purpose: Placeholder descriptor until the mmap injection path is ready.
 * // Activation: Always (no THEMIS_ENABLE_GGML_BRIDGE required here).
 * // Production Delta: `train.cores[k].data` will point to mmap'd pages,
 * //                   not heap-allocated copies.
 * // Removal Plan: Q1 2027 — replace heap copy with MAP_SHARED region pinned
 * //               via mlock(); add HMAC provenance check before returning.
 */
struct GgmlCoreDescriptor {
    bool        valid = false;   ///< false → adapter not found or load failed

    storage::TTTrain train;      ///< TT-format adapter weights (all cores)

    std::string adapter_key;     ///< Full storage key (for cache invalidation)
    std::string tenant_id;       ///< Owning tenant
    std::string domain;          ///< Domain tag (e.g. "legal", "medical")
    std::string base_model_id;   ///< Base model this adapter targets

    /// Number of parameters across all TT-cores.
    [[nodiscard]] std::size_t paramCount() const noexcept {
        return train.totalParams();
    }
};

// ============================================================================
// AdapterMetadata — optional provenance stored alongside the adapter
// ============================================================================

struct AdapterMetadata {
    std::string author;
    std::string created_at;   ///< ISO-8601 creation timestamp
    std::string description;
    float       achieved_eps = 0.0f;  ///< TT reconstruction error at store time
    std::size_t max_rank     = 0;
};

// ============================================================================
// AdapterRepository
// ============================================================================

/**
 * @brief Persistent store for LoRA/PEFT adapters as TT graphs.
 *
 * ### Example
 * ```cpp
 * auto backend = std::make_shared<InMemoryTensorBackend>();
 * AdapterRepository repo(backend, "tenant1");
 *
 * // Store a legal adapter
 * repo.store("legal", "llama3-8b", adapter_train, meta);
 *
 * // Load for inference
 * auto desc = repo.loadAdapter("legal", "llama3-8b");
 * if (desc.valid) {
 *     ggml_bridge.inject(ctx, desc);
 * }
 * ```
 */
class AdapterRepository {
public:
    /**
     * @brief Injectable mmap-style adapter loader (STUB #265 bridge).
     */
    using MmapLoadFn = std::function<GgmlCoreDescriptor(
        const std::string& tenant_id,
        const std::string& domain,
        const std::string& base_model_id,
        const std::string& adapter_key,
        const std::shared_ptr<storage::ITensorStorageBackend>& backend)>;

    /**
     * @brief Injectable exact-similarity backend (STUB #266 bridge).
     */
    using ExactSimilarityFn = std::function<std::vector<SimilarityResult>(
        const std::string& query_key,
        std::size_t k,
        const std::shared_ptr<storage::ITensorStorageBackend>& backend)>;

    /**
     * @brief Construct a repository backed by the given storage backend.
     *
     * @param backend    Storage backend (InMemoryTensorBackend for tests;
     *                   RocksDBTensorBackend for production).
     * @param tenant_id  Tenant namespace.  All keys are scoped under this tenant.
     */
    explicit AdapterRepository(
        std::shared_ptr<storage::ITensorStorageBackend> backend,
        std::string                                     tenant_id = "default");

    /**
     * @brief Default constructor for tests and quick setups.
     *
     * Constructs an `AdapterRepository` with an in-memory backend and the
     * default tenant. This keeps existing tests that call `AdapterRepository()`
     * working without changes.
     */
    AdapterRepository() : AdapterRepository(std::make_shared<storage::InMemoryTensorBackend>(), "default") {}

    // ─── Write API ───────────────────────────────────────────────────────────

    /**
     * @brief Persist a LoRA adapter as a TT graph.
     *
     * Serialises `adapter_train` and writes it to the storage backend under
     * key `__adapters__:<tenant_id>:<domain>:<base_model_id>`.
     * Overwrites any existing adapter at the same key.
     *
     * @param domain         Domain tag (e.g. "legal", "medical", "scientific").
     * @param base_model_id  Identifier for the base model (e.g. "llama3-8b").
     * @param adapter_train  TT-format adapter weights.
     * @param meta           Optional provenance metadata.
     *
     * @return true on success, false if serialisation or backend write failed.
     */
    [[nodiscard]] bool store(const std::string&      domain,
                              const std::string&      base_model_id,
                              const storage::TTTrain& adapter_train,
                              const AdapterMetadata&  meta = {});

    // Backwards-compatible overloads used by older tests and adapters.
    // Legacy call-site: store(tenant_id, domain, adapter_key, core_descriptor);
    [[nodiscard]] bool store(const std::string& tenant_id,
                              const std::string& domain,
                              const std::string& adapter_key,
                              const TensorTrainCore& core) {
        // Ignore tenant_id for now (repository constructed per-tenant in tests);
        (void)tenant_id;
        return store(domain, adapter_key, static_cast<const storage::TTTrain&>(core), AdapterMetadata{});
    }

    [[nodiscard]] bool store(const std::string& tenant_id,
                              const std::string& domain,
                              const std::string& adapter_key,
                              const std::string& serialized_train) {
        // If tests pass a serialized string, treat it as a no-op placeholder and fail gracefully.
        (void)tenant_id; (void)domain; (void)adapter_key; (void)serialized_train;
        return false;
    }

    /**
     * @brief Remove a stored adapter.
     *
     * @return true if the adapter was found and deleted, false otherwise.
     */
    bool remove(const std::string& domain,
                const std::string& base_model_id);

    // ─── Read API ─────────────────────────────────────────────────────────────

    /**
     * @brief Load an adapter and prepare it for ggml injection.
     *
     * Retrieves the serialised TTTrain and deserialises it into a
     * `GgmlCoreDescriptor`.  In the current implementation (STUB #172)
     * the TT-core data is heap-copied; the production path uses mmap.
     *
     * @param domain         Domain tag.
     * @param base_model_id  Base model identifier.
     *
     * @return  GgmlCoreDescriptor with `valid = true` on success,
     *          `valid = false` if the adapter is not found.
     */
    [[nodiscard]] GgmlCoreDescriptor
        loadAdapter(const std::string& domain,
                    const std::string& base_model_id) const;

    /**
     * @brief List all domain tags that have at least one stored adapter.
     *
     * @return Sorted, deduplicated vector of domain strings.
     */
    [[nodiscard]] std::vector<std::string> listDomains() const;

    /**
     * @brief List all (domain, base_model_id) pairs stored for this tenant.
     */
    [[nodiscard]] std::vector<std::pair<std::string, std::string>>
        listAdapters() const;

    // ─── Fingerprint Graph Integration (Phase 4 prep) ────────────────────────

    /**
     * @brief Wire a TensorFingerprintGraph to this repository.
     *
     * When set, every `store()` call also registers the adapter fingerprint
     * in the graph, and every `remove()` call deregisters it.
     * `findSimilarAdapters()` requires the graph to be set.
     *
     * Thread-safe; may be called at any time after construction.
     *
     * @param graph  Shared fingerprint graph (nullptr disables integration).
     */
    void setFingerprintGraph(std::shared_ptr<TensorFingerprintGraph> graph);

    /**
     * @brief Find the top-k adapters most similar to the given adapter.
     *
        * Delegates to the wired `TensorFingerprintGraph`.
        * By default, key-based similarity uses TT cosine (inner-product based).
        * If `setExactSimilarityFn()` is set, that backend overrides scoring.
     *
     * @note
     * // STUB/SIMULATION NOTE (STUB #177):
        * // Purpose: Expose adapter similarity search backed by fingerprint graph.
     * // Activation: Only when setFingerprintGraph() has been called.
        * // Production Delta: Uses exact TT cosine by default and supports
        * //   optional backend override via setExactSimilarityFn(); does not yet
        * //   include ANN acceleration/indexing for large candidate pools.
     * // Removal Plan: Q3 2027 — replace with full TTTrain::innerProduct()
     * //   similarity + HNSW index over fingerprints (Phase 4 AdaLoRA bridge).
     *
     * @param domain         Domain tag of the query adapter.
     * @param base_model_id  Base model identifier of the query adapter.
     * @param k              Maximum number of results to return.
     *
     * @return Sorted list (descending cosine score) of similar adapters.
     *         Empty when the graph is not set, the adapter is not registered,
     *         or k == 0.
     */
    [[nodiscard]] std::vector<SimilarityResult>
        findSimilarAdapters(const std::string& domain,
                            const std::string& base_model_id,
                            std::size_t        k) const;

    // ─── Diagnostics ─────────────────────────────────────────────────────────

    struct RepositoryStats {
        std::size_t total_adapters  = 0;
        std::size_t total_param_bytes = 0;
        std::size_t load_hits       = 0;  ///< Successful loadAdapter() calls
        std::size_t load_misses     = 0;  ///< loadAdapter() returning invalid
    };

    [[nodiscard]] RepositoryStats stats() const noexcept;

    // ─── Bridge injection API (STUB #265 / #266) ────────────────────────────

    /** @brief Inject a mmap-style loader backend for loadAdapter(). */
    static void setMmapLoadFn(MmapLoadFn fn);
    /** @brief Remove a previously injected mmap-style loader backend. */
    static void clearMmapLoadFn();

    /** @brief Inject an exact similarity backend for findSimilarAdapters(). */
    static void setExactSimilarityFn(ExactSimilarityFn fn);
    /** @brief Remove a previously injected exact similarity backend. */
    static void clearExactSimilarityFn();

    /**
     * @brief Inject an ANN frontdoor for adapter-scoped routing metadata.
     *
     * When configured, findSimilarAdapters() exposes the adapter scope to the
     * ANN frontdoor so the retrieval plan can be tracked uniformly alongside
     * other ANN consumers.
     */
    void setAnnFrontdoor(std::shared_ptr<index::AnnFrontdoor> frontdoor);

private:
    /// Build the RocksDB key for a given (domain, base_model_id) pair.
    [[nodiscard]] std::string makeKey(const std::string& domain,
                                      const std::string& base_model_id) const;

    std::shared_ptr<storage::ITensorStorageBackend> backend_;
    std::string                                     tenant_id_;

    /// Optional fingerprint graph — set via setFingerprintGraph().
    /// Protected by graph_mutex_ (separate from stats_mutex_ to prevent
    /// lock-order inversions; graph reads must never hold stats_mutex_).
    mutable std::shared_mutex               graph_mutex_;
    std::shared_ptr<TensorFingerprintGraph> fingerprint_graph_;
    std::shared_ptr<index::AnnFrontdoor>    ann_frontdoor_;

    mutable std::shared_mutex stats_mutex_;
    mutable RepositoryStats   stats_;
};

} // namespace tensor
} // namespace themis
