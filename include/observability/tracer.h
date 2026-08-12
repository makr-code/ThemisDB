/**
 * @file tracer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "core/concerns/i_tracer.h"
#include "observability/continuous_profiler.h"

#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace observability {

/**
 * @brief Statistics snapshot exported by ObservabilityTracer.
 */
struct TracerStats {
    int64_t total_spans{0};   ///< Total spans created since initialization
    int64_t active_spans{0};  ///< Spans that have been started but not yet ended
    int64_t dropped_spans{0}; ///< Spans dropped due to sampling
};

/**
 * @brief Configuration for ObservabilityTracer.
 */
struct ObservabilityTracerConfig {
    /// Logical service name embedded in every span (e.g. "themisdb").
    std::string service_name = "themisdb";

    /// OTLP/HTTP exporter endpoint (e.g. "http://otel-collector:4318").
    /// An empty string disables remote export; spans are tracked in-process only.
    std::string endpoint;

    /// Fraction of spans to sample [0.0, 1.0].  1.0 = always-on.
    double sample_rate = 1.0;

    /// Maximum number of completed span records to retain in the in-process
    /// ring buffer.  0 disables in-process retention.
    size_t max_retained_spans = 1000;

    /// When true, publish span counters to MetricsCollector on each startSpan.
    bool publish_metrics = true;

    /// When true and @c profiler is set, capture a CPU profile snapshot when
    /// a span ends.  The snapshot's folded-stacks text is attached to the
    /// SpanRecord under the "profile.cpu_folded" attribute key so that flame
    /// graphs can be correlated with individual trace spans.
    bool attach_profile_on_span_end = false;

    /// Optional profiler that the tracer notifies on span end so that
    /// profiler snapshots can be correlated with trace spans.
    std::shared_ptr<ContinuousProfiler> profiler;
};

/**
 * @brief Lightweight in-process record of a completed span.
 *
 * Retained in the ring buffer for testing and local diagnostics.
 */
struct SpanRecord {
    std::string name;
    std::string trace_id;
    std::string span_id;
    std::string parent_span_id;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    bool ok{true};
    std::string status_description;
    std::map<std::string, std::string> attributes;

    /**
     * @brief CPU profile snapshot attached when
     *        `ObservabilityTracerConfig::attach_profile_on_span_end` is true
     *        and a `ContinuousProfiler` is configured.
     *
     * Contains the folded-stacks text (pprof format) captured at span end.
     * Empty when profiling is not enabled.
     */
    std::string cpu_profile_folded;
};

/**
 * @brief Standalone observability tracer with W3C Trace Context propagation.
 *
 * `ObservabilityTracer` implements the `core::concerns::ITracer` interface and
 * provides:
 *   - Span lifecycle management (startSpan / startChildSpan / end)
 *   - W3C Trace Context extraction from / injection into HTTP headers
 *     (`traceparent` header: `00-<traceId>-<parentId>-<flags>`)
 *   - Configurable sampling (always-on, always-off, probabilistic)
 *   - In-process ring buffer of completed spans for local diagnostics
 *   - MetricsCollector integration: publishes `themis_tracer_spans_total`,
 *     `themis_tracer_active_spans`, and `themis_tracer_dropped_spans_total`
 *   - Optional ContinuousProfiler attachment: when `attach_profile_on_span_end`
 *     is true, a CPU profiler snapshot is captured at span end and stored in
 *     `SpanRecord::cpu_profile_folded` for flame-graph correlation
 *
 * ### Thread Safety
 * All public methods are thread-safe.
 *
 * ### W3C Trace Context
 * The `traceparent` header format (Level 1):
 * ```
 * 00-<32-hex-traceId>-<16-hex-parentId>-<2-hex-flags>
 * ```
 * Example: `00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01`
 *
 * ### Usage
 * ```cpp
 * ObservabilityTracerConfig cfg;
 * cfg.service_name = "themisdb";
 * cfg.sample_rate  = 0.1;   // 10% sampling
 *
 * ObservabilityTracer tracer(cfg);
 *
 * auto span = tracer.startSpan("db.query");
 * span->setAttribute("db.table", "users");
 * span->end();
 * ```
 */
class ObservabilityTracer : public core::concerns::ITracer {
public:
    explicit ObservabilityTracer(const ObservabilityTracerConfig& config = ObservabilityTracerConfig{});
    ~ObservabilityTracer() override;

    // Non-copyable, movable
    ObservabilityTracer(const ObservabilityTracer&) = delete;
    ObservabilityTracer& operator=(const ObservabilityTracer&) = delete;

    // -----------------------------------------------------------------------
    // ITracer interface
    // -----------------------------------------------------------------------

    /**
     * @brief Start a new root span, optionally sampling it out.
     *
     * @param name Span name (e.g. "db.query", "storage.write").
     * @return Unique ownership of the new span.  Never null; returns a no-op
     *         span when the span is sampled out.
     */
    std::unique_ptr<ISpan> startSpan(const std::string& name) override;

    /**
     * @brief Start a child span that continues the parent's trace.
     *
     * Propagates trace_id from the parent and sets parent_span_id.
     *
     * @param name   Child span name.
     * @param parent Parent span (must have been created by this tracer).
     * @return Unique ownership of the child span.
     */
    std::unique_ptr<ISpan> startChildSpan(const std::string& name,
                                          const ISpan& parent) override;

    /**
     * @brief Extract W3C `traceparent` from inbound headers and start a span.
     *
     * If `traceparent` is absent or malformed, starts a new root span.
     *
     * @param name    Span name.
     * @param headers Inbound HTTP headers.
     * @return Unique ownership of the span.
     */
    std::unique_ptr<ISpan> startSpanFromHeaders(
        const std::string& name,
        const std::map<std::string, std::string>& headers) override;

    /**
     * @brief Inject the active (most-recently-started) span context into
     *        outbound HTTP headers as a `traceparent` value.
     *
     * @param headers Outbound HTTP headers to populate.
     */
    void injectContext(std::map<std::string, std::string>& headers) override;

    /**
     * @brief No-op initialiser (config is passed at construction time).
     *
     * @return Always true.
     */
    bool initialize(const std::string& serviceName,
                    const std::string& endpoint) override;

    void shutdown() override;

    bool isInitialized() const override;

    void flush() noexcept override;

    core::concerns::ProbeResult isHealthy() const override;

    // -----------------------------------------------------------------------
    // Diagnostics
    // -----------------------------------------------------------------------

    /** @brief Return current tracer statistics (spans created/active/dropped). */
    TracerStats stats() const;

    /**
     * @brief Return a copy of the retained completed-span ring buffer.
     *
     * The buffer holds at most `config.max_retained_spans` entries; oldest
     * entries are evicted when full.
     */
    std::vector<SpanRecord> completedSpans() const;

    /** @brief Clear the in-process ring buffer of completed spans. */
    void clearCompletedSpans();

    /** @brief Return the active configuration. */
    ObservabilityTracerConfig getConfig() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace observability
} // namespace themis
