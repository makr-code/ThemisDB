/*
 * ThemisDB | File: llm_api_contract.h | Version: 1.0.0
 * Author: ThemisDB Contributors | Maturity: 🟢 PRODUCTION-READY
 * Status: Phase 1 — Frozen Contract
 * Purpose: Frozen LLM module API contract semantics for the active v1.x major line.
 */

/**
 * @file llm_api_contract.h
 * @brief Frozen LLM module API contracts for the active v1.x line.
 *
 * This header defines the normative contract for all LLM module inference,
 * embedding, streaming, plugin/adapter lifecycle, cancellation, resource
 * ownership, and concurrency guarantees.
 *
 * @section scope Contract Scope
 *
 * The contracts below are binding for all implementations that participate in
 * the ThemisDB LLM pipeline:
 *   - Inference engine (generate, generateBatch, generateStream)
 *   - Embedding engine (embed, embedBatch)
 *   - Plugin/adapter lifecycle manager (load, warm, infer, unload)
 *   - Cancellation token infrastructure (CancellationToken)
 *   - VRAM/RAM resource manager (ActiveVramAllocator)
 *   - Model loader (ModelLoader, LlamaWrapper)
 *   - Streaming callback dispatch
 *
 * @section versioning Versioning
 *
 * This contract is stable within v1.x. Breaking changes require a v2.0 bump
 * with migration notes and a CHANGELOG entry.
 *
 * @see src/llm/ROADMAP.md — Phase 1 item
 * @see tests/llm/test_llm_api_contract_hardening_focused.cpp — LAC-01..LAC-20
 * @see benchmarks/llm/bench_llm_hotpaths.cpp — LLM-01..LLM-08
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <string>

namespace themis {
namespace llm {

// ============================================================================
// § 1  Inference API Input / Output Ownership
//
// generate() / generateBatch() / generateStream():
//   - Caller owns the input prompt string; the engine does not retain a
//     reference beyond the duration of the synchronous call.
//   - The returned output string is owned by the caller from the moment
//     generate() returns.
//   - generateBatch() returns a vector whose entries correspond 1:1 to the
//     input batch in the same order (batch consistency guarantee).
//   - generateStream() dispatches tokens to the provided callback; the
//     callback is invoked synchronously from the engine thread. The caller
//     MUST NOT block the callback for more than kStreamCallbackMaxBlockMs.
//   - Exception safety: if generate() throws, no partial output is returned
//     and the engine state remains consistent (strong guarantee).
// ============================================================================

/// Maximum input prompt length in bytes accepted by the inference engine.
inline constexpr std::size_t kMaxPromptBytes = 1u * 1024u * 1024u;  // 1 MiB

/// Maximum batch size for generateBatch() / embedBatch().
inline constexpr std::size_t kMaxBatchSize = 256;

/// Maximum time a stream callback may block the engine dispatch thread.
inline constexpr std::chrono::milliseconds kStreamCallbackMaxBlockMs{100};

/// Default inference timeout per request.
inline constexpr std::chrono::seconds kDefaultInferenceTimeout{120};

/// Maximum allowed inference timeout (operator-configurable upper bound).
inline constexpr std::chrono::seconds kMaxInferenceTimeout{600};

// ============================================================================
// § 2  Embedding API Contract
//
// embed() / embedBatch():
//   - Output vectors are L2-normalised before return. ‖v‖₂ = 1.0 ± 1e-5.
//   - An empty input string returns a zero vector (no error).
//   - embedBatch() order is preserved: output[i] corresponds to input[i].
//   - The returned float vector has dimension equal to the model's embedding
//     dimension; this is fixed per loaded model.
//   - embed() is thread-safe when called from different threads concurrently,
//     provided each call uses a separate inference context handle.
// ============================================================================

/// L2-normalisation tolerance for embed() output vectors.
inline constexpr float kEmbedL2NormTolerance = 1e-5f;

/// Maximum input string length for embed() / embedBatch() in bytes.
inline constexpr std::size_t kMaxEmbedInputBytes = 128u * 1024u;  // 128 KiB

// ============================================================================
// § 3  Plugin / Adapter Lifecycle Contract
//
// Plugin lifecycle: load → warm → infer → unload.
//   - load(): acquires all resources (VRAM, file handles, thread pool slots).
//     On failure throws with LLM_PLUGIN_LOAD_FAILED; no resources are retained.
//   - warm(): optional; pre-allocates KV cache and context buffers.
//   - infer(): delegates to the loaded plugin; caller holds a shared_ptr to the
//     plugin handle. The engine guarantees the handle remains valid for the
//     duration of infer().
//   - unload(): releases all plugin resources. After unload(), the plugin
//     handle is invalid. Calling unload() on an already-unloaded plugin is
//     safe (idempotent / no-op).
//   - double-unload: second call is a no-op, not an error.
//   - null plugin: passing a null handle to any lifecycle call throws with
//     LLM_PLUGIN_NULL_HANDLE.
// ============================================================================

// ============================================================================
// § 4  Streaming Callback Ownership and Exception Safety
//
// generateStream() callback contract:
//   - The callback function object is owned by the caller; the engine holds a
//     non-owning reference during the stream lifetime.
//   - If the callback throws an exception, the engine MUST catch it, abort the
//     stream, and return STREAM_ABORTED to the caller. The exception MUST NOT
//     propagate to the engine's internal thread pool.
//   - After cancellation (via CancellationToken), the callback will not be
//     invoked again. Any pending callback invocations already in flight are
//     allowed to complete.
//   - The callback is NOT called after generateStream() returns.
// ============================================================================

// ============================================================================
// § 5  Cancellation Token Semantics
//
// CancellationToken contract:
//   - Cancellation is checked at the pre-inference boundary only.
//   - A cancelled token causes the engine to return INFERENCE_CANCELLED before
//     any computation begins.
//   - Mid-inference cancellation is NOT guaranteed; the engine may complete
//     the current token generation step before honouring cancellation.
//   - CancellationToken::cancel() is thread-safe and may be called from any
//     thread.
//   - Checking isCancelled() is O(1) (atomic load); no blocking occurs.
// ============================================================================

/// Maximum overhead for a CancellationToken::isCancelled() check (atomic load).
/// Implementations exceeding this overhead violate the contract.
inline constexpr std::chrono::microseconds kCancellationCheckMaxOverhead{1};

// ============================================================================
// § 6  Resource Contracts: VRAM / RAM
//
// VRAM ownership:
//   - The VRAM allocator grants exclusive ownership of each allocation.
//   - On VRAM pressure, the eviction policy is LRU over inactive model contexts.
//   - If VRAM cannot be allocated, the engine throws with VRAM_EXHAUSTED.
//   - Evicted model contexts must be reloaded before next infer(); the engine
//     handles reload transparently unless kDisableAutoReload is set.
//
// RAM ownership:
//   - All heap allocations for inference context buffers use RAII (unique_ptr /
//     shared_ptr). No raw new/delete in the inference hot path.
//   - On OOM, std::bad_alloc propagates to the caller; the engine state is
//     consistent (basic guarantee).
// ============================================================================

/// Default VRAM allocation timeout before throwing VRAM_EXHAUSTED.
inline constexpr std::chrono::milliseconds kVramAllocTimeout{5000};

// ============================================================================
// § 7  Error Taxonomy
//
// All LLM components MUST map internal error states to one of these canonical
// error codes. This enables uniform operator diagnostics.
// ============================================================================

/**
 * @brief Canonical LLM error codes.
 *
 * Values are stable across v1.x. Any addition requires a CHANGELOG entry.
 * Removal or renumbering requires a v2.0 major bump.
 */
enum class LlmErrorCode : int {
    /// Operation succeeded.
    OK                    =  0,
    /// Model is not loaded; load() must be called before infer/embed.
    MODEL_NOT_LOADED      =  1,
    /// Inference did not complete within the configured timeout.
    INFERENCE_TIMEOUT     =  2,
    /// GPU VRAM allocation failed; no eviction candidate available.
    VRAM_EXHAUSTED        =  3,
    /// Plugin load() failed (file not found, signature mismatch, ABI error).
    PLUGIN_LOAD_FAILED    =  4,
    /// Stream was aborted (callback threw, or cancellation mid-stream).
    STREAM_ABORTED        =  5,
    /// Per-tenant or global inference quota exceeded.
    QUOTA_EXCEEDED        =  6,
    /// Batch size exceeds kMaxBatchSize.
    BATCH_SIZE_EXCEEDED   =  7,
    /// Input prompt exceeds kMaxPromptBytes.
    INPUT_TOO_LARGE       =  8,
    /// Model is loaded but context is invalid or corrupted.
    MODEL_CONTEXT_INVALID =  9,
    /// Inference cancelled via CancellationToken before computation began.
    INFERENCE_CANCELLED   = 10,
    /// Null plugin handle passed to a lifecycle call.
    PLUGIN_NULL_HANDLE    = 11,
    /// Plugin registry lookup returned no matching adapter.
    PLUGIN_NOT_FOUND      = 12,
    /// Unclassified internal LLM engine error.
    INTERNAL_ERROR        = 13,
};

// ============================================================================
// § 8  Concurrency Contract
//
// Thread-safety guarantees by path:
//   - generate() / embed(): thread-safe when called concurrently with distinct
//     inference context handles. Shared model weights are read-only after load().
//   - generateBatch() / embedBatch(): internally serialised per model instance;
//     callers requiring concurrent batch execution must use separate model handles.
//   - generateStream(): the stream for a given handle is serialised; concurrent
//     streams on the same handle require external serialisation by the caller.
//   - Plugin load() / unload(): serialised via the plugin registry mutex.
//     Concurrent load/unload of the same plugin name is safe but one call will
//     block until the other completes.
//   - CancellationToken::cancel(): thread-safe (atomic write).
//   - Quota check (in-memory): thread-safe (atomic counter per tenant).
// ============================================================================

/// Default quota synchronisation interval for multi-node quota enforcement.
inline constexpr std::chrono::milliseconds kQuotaSyncInterval{1000};

} // namespace llm
} // namespace themis
