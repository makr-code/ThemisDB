/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cot_tracer.h                                       ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-04-15 18:46:13                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     281                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 93aebd9731  2026-03-23  feat(prompt_engineering): CoT Step Tracer — IChainOfThoug... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file cot_tracer.h
 * @brief Chain-of-Thought per-step tracing interface and implementations.
 *
 * Provides `IChainOfThoughtTracer`, a pluggable tracing interface for
 * `ChainOfThoughtBuilder` that records the begin and end of each reasoning
 * step together with construction latency.  Key components:
 *
 * - **`IChainOfThoughtTracer`** — interface; both callbacks are `noexcept`
 *   so they can never interrupt the prompt-construction hot path.
 * - **`CoTSpanRecord`** — immutable value type capturing one complete step
 *   span: index, label, content, token_count (approximate), and duration.
 * - **`RecordingCoTTracer`** — concrete tracer that stores spans in a vector;
 *   designed for unit testing and offline analysis.
 * - **`CoTTraceCollector`** — fan-out tracer that forwards to N registered
 *   child tracers; serializes the accumulated trace to JSON.
 *
 * Grounded in:
 *   - Wei et al. (NeurIPS 2022) "Chain-of-Thought Prompting Elicits Reasoning"
 *     [arXiv:2201.11903]
 *   - Wang et al. (ICLR 2023) "Self-Consistency Improves Chain of Thought
 *     Reasoning" [arXiv:2203.11171]
 *
 * Performance target: tracing overhead per CoT step ≤ 0.2 ms (P99).
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace themis {
namespace prompt_engineering {

// ============================================================================
// StepId
// ============================================================================

/** @brief 0-based index that identifies a reasoning step within one build(). */
using StepId = std::size_t;

// ============================================================================
// CoTSpanRecord
// ============================================================================

/**
 * @brief Immutable record of a single chain-of-thought step span.
 *
 * Produced by `RecordingCoTTracer` and `CoTTraceCollector::spans()`.
 */
struct CoTSpanRecord {
    StepId      step_index   = 0;
    std::string label;                          ///< Auto-numbered or explicit label
    std::string content;                        ///< Step content text
    std::size_t token_count  = 0;               ///< Approximate BPE token count (chars/4)
    std::chrono::microseconds duration{0};      ///< Wall-clock span duration
    std::chrono::system_clock::time_point start_time; ///< Span start timestamp

    /** @brief Serialise to JSON for logging / timeseries storage. */
    nlohmann::json toJson() const;
};

// ============================================================================
// IChainOfThoughtTracer
// ============================================================================

/**
 * @brief Pluggable tracer interface for `ChainOfThoughtBuilder`.
 *
 * Both methods are `noexcept` so that a misbehaving implementation can never
 * interrupt the prompt-construction path.  Implementations must handle all
 * errors internally (log + swallow).
 *
 * Grounded in OpenTelemetry span model:
 *   - `onStepBegin()` ≈ Span.Start
 *   - `onStepEnd()` ≈ Span.End + attribute recording
 */
class IChainOfThoughtTracer {
public:
    virtual ~IChainOfThoughtTracer() = default;

    /**
     * @brief Called immediately before a reasoning step is rendered into
     *        the prompt string.
     *
     * @param step_index  0-based index of the step being rendered.
     * @param label       Step label (auto-numbered or explicit).
     */
    virtual void onStepBegin(StepId             step_index,
                             const std::string& label) noexcept = 0;

    /**
     * @brief Called immediately after a reasoning step has been appended to
     *        the output buffer.
     *
     * @param step_index  Same index as the corresponding `onStepBegin` call.
     * @param content     The step content that was appended.
     * @param duration    Wall-clock time elapsed since `onStepBegin` for this
     *                    step (precision: microseconds).
     */
    virtual void onStepEnd(StepId                    step_index,
                           const std::string&        content,
                           std::chrono::microseconds duration) noexcept = 0;

    /**
     * @brief Human-readable name of this tracer implementation.
     * @return Tracer name string, e.g. @c "recording-cot-tracer".
     */
    [[nodiscard]] virtual std::string name() const = 0;
};

// ============================================================================
// RecordingCoTTracer
// ============================================================================

/**
 * @brief Concrete `IChainOfThoughtTracer` that records all spans in memory.
 *
 * Intended for unit testing and offline analysis.  Thread-safe: internal
 * storage is protected by a mutex so the same instance can be shared by
 * concurrent builders (though that is unusual in practice).
 */
class RecordingCoTTracer final : public IChainOfThoughtTracer {
public:
    RecordingCoTTracer() = default;

    void onStepBegin(StepId             step_index,
                     const std::string& label) noexcept override;

    void onStepEnd(StepId                    step_index,
                   const std::string&        content,
                   std::chrono::microseconds duration) noexcept override;

    std::string name() const override { return "recording-cot-tracer"; }

    // -------------------------------------------------------------------------
    // Accessors
    // -------------------------------------------------------------------------

    /** @brief Return a snapshot of all recorded spans. */
    std::vector<CoTSpanRecord> spans() const;

    /** @brief Return the number of complete spans recorded so far. */
    std::size_t spanCount() const noexcept;

    /** @brief Return `true` when at least one span has been recorded. */
    bool hasSpans() const noexcept;

    /** @brief Remove all recorded spans. */
    void reset();

    /** @brief Serialise all recorded spans to a JSON array. */
    nlohmann::json toJson() const;

private:
    mutable std::mutex          mutex_;
    std::vector<CoTSpanRecord>  spans_;

    // Pending begin events (step_index → {label, start_time}).
    struct PendingBegin {
        std::string label;
        std::chrono::system_clock::time_point start_time;
    };
    std::vector<PendingBegin>   pending_; // indexed by step_index
};

// ============================================================================
// CoTTraceCollector
// ============================================================================

/**
 * @brief Fan-out `IChainOfThoughtTracer` that forwards to N child tracers
 *        and accumulates spans itself.
 *
 * Also acts as an aggregate span store: `spans()` returns all complete spans
 * received from all registered builders during the collector's lifetime.
 *
 * Thread-safe.
 */
class CoTTraceCollector final : public IChainOfThoughtTracer {
public:
    CoTTraceCollector() = default;

    void onStepBegin(StepId             step_index,
                     const std::string& label) noexcept override;

    void onStepEnd(StepId                    step_index,
                   const std::string&        content,
                   std::chrono::microseconds duration) noexcept override;

    std::string name() const override { return "cot-trace-collector"; }

    // -------------------------------------------------------------------------
    // Child tracer management
    // -------------------------------------------------------------------------

    /**
     * @brief Register a child tracer.
     * @param tracer  Non-null child implementation; `onStepBegin`/`onStepEnd`
     *                are forwarded to it on every call.
     */
    void addTracer(std::shared_ptr<IChainOfThoughtTracer> tracer);

    /**
     * @brief Remove a previously registered child tracer by pointer identity.
     */
    void removeTracer(const IChainOfThoughtTracer* tracer);

    /** @brief Return the number of registered child tracers. */
    std::size_t tracerCount() const noexcept;

    // -------------------------------------------------------------------------
    // Span access
    // -------------------------------------------------------------------------

    /** @brief Return a snapshot of all accumulated spans. */
    std::vector<CoTSpanRecord> spans() const;

    /** @brief Return the total number of complete spans accumulated. */
    std::size_t spanCount() const noexcept;

    /** @brief Remove all accumulated spans and pending begin events. */
    void reset();

    /** @brief Serialise all accumulated spans to a JSON array. */
    nlohmann::json toJson() const;

    /**
     * @brief Total steps traced across all builds since last `reset()`.
     * Monotonically increasing; useful for latency / throughput monitoring.
     */
    std::size_t totalStepsTraced() const noexcept;

private:
    mutable std::mutex mutex_;

    // Own span store (same logic as RecordingCoTTracer).
    std::vector<CoTSpanRecord> spans_;
    struct PendingBegin {
        std::string label;
        std::chrono::system_clock::time_point start_time;
    };
    std::vector<PendingBegin> pending_;

    // Child tracers.
    std::vector<std::shared_ptr<IChainOfThoughtTracer>> children_;

    std::atomic<std::size_t> total_steps_traced_{0};
};

} // namespace prompt_engineering
} // namespace themis
