/**
 * @file ggml_tensor_bridge.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Gap Summary: total=15; TODO=1, Stub=11, Unimpl=0, Mock=1, Sim=2, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#ifdef THEMIS_ENABLE_GGML_BRIDGE

#include "storage/tensor_train_decomposer.h"
#include "storage/tensor_network_storage_engine.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>

// Forward-declare ggml types to avoid full ggml.h inclusion at ThemisDB level
struct ggml_context;
struct ggml_tensor;

namespace themis {
namespace storage {

// ============================================================================
// GgmlTensorBridgeConfig
// ============================================================================

/**
 * @brief Configuration for the GGML bridge.
 */
struct GgmlTensorBridgeConfig {
    /// Maximum number of concurrently mapped TT-trains.
    std::size_t max_concurrent_mappings = 64;

    /// If true, the bridge will decompress TT-cores to float32 before handing
    /// them to ggml (safe but slower). If false, ggml must understand
    /// GGML_TYPE_TT (requires ggml patch, Q1 2027).
    bool decompress_to_f32 = true;

    /// Memory-mapped window size in bytes (0 = unlimited).
    std::size_t mmap_window_bytes = 0;

    /// Pin mapped pages in physical RAM (prevents page-out during inference).
    bool pin_pages = false;

    /// If true, copies are made for quantised cores before handing to ggml.
    /// Required for NF4 when ggml does not natively support NF4 TT-cores.
    bool copy_quantised_cores = true;

    /// Root directory containing per-tenant RocksDB SST files.
    /// Used by the io_uring prefetch path when THEMIS_HAS_IO_URING is defined.
    /// Example: "/var/lib/themisdb/rocksdb"
    std::string sst_root_dir = "";
};

// ============================================================================
// MappedTTTensor — handle to a live mmap'd TT-train
// ============================================================================

/**
 * @brief RAII handle for a memory-mapped TT-train available to ggml.
 *
 * Returned by GgmlTensorBridge::map().  The mapping stays valid until this
 * handle is destroyed.
 */
class MappedTTTensor {
public:
    MappedTTTensor() = default;
    ~MappedTTTensor();

    // Non-copyable, movable
    MappedTTTensor(const MappedTTTensor&)            = delete;
    MappedTTTensor& operator=(const MappedTTTensor&) = delete;
    MappedTTTensor(MappedTTTensor&&)                 noexcept;
    MappedTTTensor& operator=(MappedTTTensor&&)      noexcept;

    /// Access the ggml_tensor* to inject into the inference graph.
    /// Returns nullptr if mapping failed or bridge is not compiled in.
    ggml_tensor* ggmlTensor() const noexcept;

    /// Logical TT-train metadata (available without decompression).
    const TTTrain*       train()      const noexcept;
    const TensorFieldKey* fieldKey()  const noexcept;

    bool valid() const noexcept;

private:
    friend class GgmlTensorBridge;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// GgmlTensorBridge
// ============================================================================

/**
 * @brief Zero-copy bridge: ThemisDB TT-storage ↔ ggml computation graph.
 *
 * ## Integration Flow
 *
 * ### Pre-inference (FLARE / RAG retrieval step)
 *
 * ```cpp
 * // 1. Retrieve relevant tensor keys from TensorFingerprintGraph
 * auto similar = fingerprint_graph.findSimilar(query_train, top_k=5);
 *
 * // 2. Map their TT-cores into ggml address space
 * std::vector<MappedTTTensor> mapped;
 * for (const auto& r : similar) {
 *     auto handle = bridge.map(ctx, {r.tenant, r.collection, r.field});
 *     if (handle.valid()) mapped.push_back(std::move(handle));
 * }
 *
 * // 3. Inject into llama.cpp's KV-cache or context extension
 * for (auto& h : mapped) {
 *     llama_ctx_inject_tensor(llama_ctx, h.ggmlTensor());
 * }
 *
 * // 4. Run inference — no re-tokenisation, no JSON parsing
 * llama_decode(llama_ctx, batch);
 * ```
 *
 * ### FLARE mid-generation retrieval
 *
 * The FLARE callback can call `bridge.map()` mid-generation and inject the
 * result into the running kv-cache via `llama_kv_cache_inject()` (API planned
 * for llama.cpp Q1 2027).
 *
 * ## PEFT / LoRA Adapter Management
 *
 * ```cpp
 * // Map a specific LoRA adapter stored as a TT-train in ThemisDB
 * auto adapter = bridge.mapAdapter(ctx, adapter_id);
 * llama_lora_apply(llama_ctx, adapter.ggmlTensor(), adapter_scale);
 * // Adapter is unmapped when 'adapter' goes out of scope
 * ```
 *
 * ## Performance Notes
 *
 * - With `decompress_to_f32 = false` and ggml GGML_TYPE_TT support:
 *   TTFT estimated at 40–90 ms (zero copy, direct contraction in ggml).
 * - With `decompress_to_f32 = true` (current default):
 *   TTFT estimated at 80–150 ms (one decompression pass, then zero-copy).
 * - Classical RAG baseline: 150–400 ms TTFT.
 */
class GgmlTensorBridge {
public:
    explicit GgmlTensorBridge(
        std::shared_ptr<TensorNetworkStorageEngine> storage,
        GgmlTensorBridgeConfig                     cfg = {});

    ~GgmlTensorBridge();

    // Non-copyable
    GgmlTensorBridge(const GgmlTensorBridge&)            = delete;
    GgmlTensorBridge& operator=(const GgmlTensorBridge&) = delete;

    /**
     * @brief Map a stored TT-train into a ggml_tensor accessible from `ctx`.
     *
     * The returned handle keeps the mapping alive.  When the handle is
     * destroyed, the mapping is released.
     *
     * @param ctx   ggml context that will own the resulting tensor.
     * @param key   Logical address of the TT-train in ThemisDB.
     * @param version Version to map (0 = latest).
     * @return MappedTTTensor handle (check .valid() before use).
     */
    MappedTTTensor map(ggml_context*        ctx,
                       const TensorFieldKey& key,
                       uint64_t              version = 0);

    /**
     * @brief Map a LoRA adapter stored as a TT-train in ThemisDB.
     *
     * LoRA adapters are stored under the collection `"__lora_adapters__"`.
     *
     * @param ctx        ggml context.
     * @param adapter_id Adapter identifier (= field name in ThemisDB).
     * @param tenant     Tenant that owns the adapter.
     */
    MappedTTTensor mapAdapter(ggml_context*      ctx,
                               const std::string& adapter_id,
                               const std::string& tenant = "");

    /**
     * @brief Preload TT-cores into the mmap window (async, non-blocking).
     *
     * Can be called speculatively while the previous FLARE generation step
     * is still running, to overlap DB I/O with compute.
     *
     * In production mode this method fail-closes (throws) when neither an
     * injected `PrefetchFn` nor the built-in io_uring path is available.
     */
    void prefetch(const TensorFieldKey& key, uint64_t version = 0);

    /**
     * @brief Release all active mappings (called on ggml_context reset).
     */
    void releaseAll();

    struct BridgeStats {
        std::size_t active_mappings    = 0;
        std::size_t total_maps         = 0;
        std::size_t total_releases     = 0;
        std::size_t total_bytes_mapped = 0;
        double      avg_map_latency_us = 0.0;
    };

    BridgeStats stats() const noexcept;

    // ─── GgmlAllocFn bridge (STUB #263a) ──────────────────────────────────

    /**
     * @brief Injectable ggml_tensor allocation function.
     *
     * When set via `setGgmlAllocFn()`, `map()` will call @p fn to obtain a
     * real `ggml_tensor*` for each successfully decompressed TT-train.  The
     * fn receives the live ggml context from `map()` plus the number of
     * float32 elements and must allocate a `ggml_tensor` owned by that
     * context.
     *
     * When not set, `ggmlTensor()` on the returned handle returns nullptr
     * (stub fallback — safe for unit tests, not for llama.cpp inference).
     *
     * Signature: `ggml_tensor* fn(ggml_context* ctx, std::size_t n_elements)`
     */
    using GgmlAllocFn = std::function<ggml_tensor*(ggml_context* /*ctx*/,
                                                   std::size_t   /*n_elements*/)>;

    /**
     * @brief Inject a ggml_tensor allocation function.
     * @param fn  Allocator; pass nullptr / empty fn to clear.
     */
    static void setGgmlAllocFn(GgmlAllocFn fn);

    /** @brief Remove a previously injected GgmlAllocFn. */
    static void clearGgmlAllocFn();

    // ─── PrefetchFn bridge (STUB #263b) ───────────────────────────────────

    /**
     * @brief Injectable prefetch function for TT-core readahead.
     *
     * When set via `setPrefetchFn()`, `prefetch()` delegates to @p fn
     * instead of the no-op stub.  The fn may use `madvise(MADV_SEQUENTIAL)`,
     * `io_uring`, or any OS-level mechanism to warm the page cache.
     *
     * Signature: `void fn(const TensorFieldKey&, uint64_t version)`
     */
    using PrefetchFn = std::function<void(const TensorFieldKey&, uint64_t)>;

    /**
     * @brief Inject a prefetch implementation.
     * @param fn  Prefetch callable; pass empty fn to revert to no-op.
     */
    static void setPrefetchFn(PrefetchFn fn);

    /** @brief Remove a previously injected PrefetchFn. */
    static void clearPrefetchFn();

    // ─── TypeRegistrationFn bridge (STUB #263c) ────────────────────────────

    /**
     * @brief Injectable GGML TT-type registration function.
     *
     * When set via `setTypeRegistrationFn()`, `registerGgmlTypeTT()` delegates
     * to the injected function instead of returning the placeholder id.
     *
     * Signature: `int fn()`
     */
    using TypeRegistrationFn = std::function<int()>;

    /**
     * @brief Inject a GGML type-registration implementation.
     * @param fn  Registration callable; pass empty fn to revert to placeholder.
     */
    static void setTypeRegistrationFn(TypeRegistrationFn fn);

    /** @brief Remove a previously injected TypeRegistrationFn. */
    static void clearTypeRegistrationFn();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// Utility: register GGML_TYPE_TT with ggml
// ============================================================================

/**
 * @brief Register the custom GGML_TYPE_TT type with the ggml runtime.
 *
 * Must be called once before any ggml operation involving TT-type tensors.
 * No-op if GGML_TYPE_TT is already registered.
 *
 * @return ggml type ID assigned to TT-type tensors.
 * @throws std::runtime_error in production mode when neither ggml custom type
 *         registration nor an injected `TypeRegistrationFn` is available.
 *
 * STUB/SIMULATION NOTE:
 * Purpose: Placeholder until ggml custom-type API stabilises.
 * Activation: Q1 2027 after ggml upstream PR merge.
 * Production Delta: Actual ggml_type registration uses ggml_type_register().
 * Removal Plan: Replace with direct ggml API call once merged.
 */
int registerGgmlTypeTT();

} // namespace storage
} // namespace themis

#endif // THEMIS_ENABLE_GGML_BRIDGE
