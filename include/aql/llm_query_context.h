/**
 * @file llm_query_context.h
 * @brief LLMQueryContext — MVCC-consistent snapshot carrier for LLM inference
 *        requests within the ThemisDB query pipeline.
 *
 * ## Design (P1.1)
 *
 * Every DB retrieval that contributes to an LLM prompt must be anchored to the
 * same HLC read-timestamp so that the assembled context is internally consistent
 * (no phantom reads, no partial writes).  `LLMQueryContext` captures that
 * snapshot at context-assembly time and threads it through the retrieval →
 * prompt-build → inference pipeline.
 *
 * ### Threading model
 * - Context assembly (DB reads) is bound to the snapshot timestamp and runs
 *   synchronously before inference is dispatched.
 * - Inference itself is asynchronous and independent of MVCC state (see
 *   `async_inference_engine.cpp`).  The snapshot is NOT held during inference;
 *   it only governs the retrieval phase.
 *
 * ### Lifecycle
 * 1. Caller creates `LLMQueryContext` with the HLC clock's current timestamp.
 * 2. All DB reads (vector, graph, relational) use `snapshot_ts` as their
 *    consistent read point.
 * 3. Context is passed into `LLMAQLHandler::buildContextWithSnapshot()`.
 * 4. The resulting `InferenceRequest` carries `trace_id` / `span_id` but no
 *    longer depends on the DB snapshot (inference is snapshot-independent).
 *
 * @see src/aql/llm_aql_handler.cpp — `buildContextWithSnapshot()`
 * @see src/storage/mvcc_store.h    — MVCCStore versioned reads
 * @see src/storage/hlc.h           — HLCTimestamp
 */

#pragma once

#include "storage/hlc.h"

#include <cstdint>
#include <string>

namespace themis {
namespace aql {

/**
 * @brief Snapshot context for a single LLM inference request.
 *
 * Binds all DB reads that contribute to the LLM prompt to a single
 * HLC read-timestamp, enforcing snapshot isolation in the retrieval phase.
 */
struct LLMQueryContext {
    /**
     * @brief HLC timestamp that anchors all DB reads for this LLM request.
     *
     * All vector, graph, and relational retrievals contributing to the prompt
     * must read data committed at or before this timestamp.  A zero-value
     * timestamp means "use the latest committed version" (consistent with
     * default RocksDB snapshot behaviour).
     */
    HLCTimestamp snapshot_ts;

    /**
     * @brief Human-readable description of the read isolation used.
     *
     * Carries `"snapshot-isolated"` when a valid snapshot_ts was captured,
     * or `"no-snapshot"` when the context was built without an explicit
     * timestamp (legacy path — avoid in new code).
     */
    std::string isolation_mode = "no-snapshot";

    /**
     * @brief Optional W3C trace-id for end-to-end observability correlation.
     *
     * If set, must be a 32-character lower-case hex string (128-bit).
     * Forwarded verbatim into the downstream `InferenceRequest::trace_id`.
     */
    std::string trace_id;

    /**
     * @brief Optional W3C parent-span-id for nested observability spans.
     *
     * If set, must be a 16-character lower-case hex string (64-bit).
     * Forwarded verbatim into the downstream `InferenceRequest::span_id`.
     */
    std::string span_id;

    /**
     * @brief Monotonically incrementing request counter for this DB instance.
     *
     * Used by the audit log to correlate the retrieval snapshot with the
     * inference invocation.  Set by the handler, not by the caller.
     */
    uint64_t request_seq = 0;

    // -----------------------------------------------------------------------
    // Factory helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Create a snapshot-isolated context from the provided HLC timestamp.
     *
     * @param ts       HLC timestamp captured from the DB's HLC clock before
     *                 any retrieval reads.
     * @param trace_id Optional W3C trace-id.
     * @param span_id  Optional W3C parent-span-id.
     * @return LLMQueryContext with isolation_mode = "snapshot-isolated".
     */
    [[nodiscard]] static LLMQueryContext fromSnapshot(
        HLCTimestamp ts,
        std::string trace_id = {},
        std::string span_id  = {}) noexcept
    {
        LLMQueryContext ctx;
        ctx.snapshot_ts     = ts;
        ctx.isolation_mode  = "snapshot-isolated";
        ctx.trace_id        = std::move(trace_id);
        ctx.span_id         = std::move(span_id);
        return ctx;
    }

    /**
     * @brief Create a legacy (no-snapshot) context for backward compatibility.
     *
     * Prefer `fromSnapshot()` in all new code paths.  This factory is provided
     * solely for migration purposes while existing callers are updated.
     *
     * @param trace_id Optional W3C trace-id.
     * @param span_id  Optional W3C parent-span-id.
     * @return LLMQueryContext with isolation_mode = "no-snapshot" and a zero
     *         snapshot_ts.
     */
    [[nodiscard]] static LLMQueryContext withoutSnapshot(
        std::string trace_id = {},
        std::string span_id  = {}) noexcept
    {
        LLMQueryContext ctx;
        ctx.snapshot_ts    = HLCTimestamp{};   // zero = "latest"
        ctx.isolation_mode = "no-snapshot";
        ctx.trace_id       = std::move(trace_id);
        ctx.span_id        = std::move(span_id);
        return ctx;
    }

    // -----------------------------------------------------------------------
    // Accessors
    // -----------------------------------------------------------------------

    /**
     * @return true when snapshot_ts is non-zero (snapshot isolation active).
     */
    [[nodiscard]] bool hasSnapshot() const noexcept {
        return snapshot_ts.value != 0;
    }
};

} // namespace aql
} // namespace themis
