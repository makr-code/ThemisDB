/**
 * @file retrieval_observability.h
 * @brief Observability and governance hooks for the layered retrieval stack.
 *
 * Provides structured tracing, latency metrics, confidence propagation, and
 * governance boundary enforcement across all four retrieval layers.
 *
 * Planned in: src/retrieval/README.md (sub-issue 1.7)
 */

#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis::retrieval {

/// Layer identifier for span attribution.
enum class RetrievalLayer {
    AnnFrontdoor,
    TensorMidlayer,
    GraphValidator,
    LoraFinalLayer,
};

/// A single telemetry span covering one retrieval layer invocation.
struct RetrievalSpan {
    std::string   trace_id;
    std::string   span_id;
    RetrievalLayer layer;
    double        latency_ms = 0.0;
    bool          cache_hit  = false;
    float         confidence = 0.0f;    ///< Layer-local confidence score
    std::unordered_map<std::string, std::string> tags;
};

/// Aggregated trace for a full end-to-end retrieval request.
struct RetrievalTrace {
    std::string               trace_id;
    std::vector<RetrievalSpan> spans;
    double                    total_latency_ms = 0.0;
    float                     final_confidence = 0.0f;
};

/// Governance decision for a retrieval request.
enum class GovernanceDecision {
    Allow,
    Warn,   ///< Proceed with a logged warning
    Block,  ///< Request denied by policy
};

/// Governance context evaluated against active policies.
struct GovernanceContext {
    std::string  user_id;
    std::string  query_hash;      ///< SHA-256 of the query text
    std::string  namespace_scope;
    bool         contains_pii = false;
    float        estimated_cost = 0.0f;
};

/**
 * @brief Observability provider interface for the retrieval stack.
 *
 * All retrieval layers call into this interface to emit spans, metrics, and
 * governance signals without coupling to a specific telemetry backend.
 */
class IRetrievalObservability {
public:
    virtual ~IRetrievalObservability() = default;

    /// Emit a completed span.
    virtual void emitSpan(const RetrievalSpan& span) = 0;

    /// Emit a completed end-to-end trace.
    virtual void emitTrace(const RetrievalTrace& trace) = 0;

    /// Evaluate governance policies and return a decision.
    virtual GovernanceDecision checkGovernance(
        const GovernanceContext& ctx) const = 0;

    /// Increment a named counter by delta.
    virtual void incrementCounter(const std::string& name,
                                   double delta = 1.0) = 0;

    /// Record a histogram observation (e.g., latency).
    virtual void recordHistogram(const std::string& name,
                                  double value) = 0;

    /// Register a callback invoked whenever governance blocks a request.
    using BlockCallback = std::function<void(const GovernanceContext&)>;
    virtual void onGovernanceBlock(BlockCallback cb) = 0;
};

/// Factory: create a no-op observability provider (safe default for tests).
std::unique_ptr<IRetrievalObservability> makeNoopObservability();

} // namespace themis::retrieval
