/*
 * ThemisDB | File: opentelemetry_tracer.h | Version: 0.0.13
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

/**
 * @file opentelemetry_tracer.h
 * @brief OpenTelemetry Full Integration tracer for ThemisDB (v1.6.0).
 *
 * Provides `OpenTelemetryTracer`, a production-ready ITracer implementation
 * that adds:
 *   - W3C Trace Context propagation (traceparent/tracestate headers)
 *   - W3C Baggage propagation for tenant/user context
 *   - Multiple exporter back-ends: OTLP (gRPC/HTTP), Jaeger, Zipkin
 *   - Automatic instrumentation helpers: recordException(), recordMetrics()
 *   - In-process span ring buffer for local diagnostics
 *   - MetricsCollector integration (span counters / gauges)
 *
 * ### Usage
 * ```cpp
 * OTelConfig cfg;
 * cfg.service_name    = "themisdb";
 * cfg.endpoint        = "http://otel-collector:4317";
 * cfg.exporters       = {"otlp", "jaeger"};
 * cfg.resource_attributes = {{"deployment.environment", "production"}};
 *
 * OpenTelemetryTracer tracer(cfg);
 *
 * // Baggage: carry tenant context across service calls
 * OpenTelemetryTracer::setBaggageItem("tenant-id", "acme");
 * OpenTelemetryTracer::setBaggageItem("user-id",   "u-42");
 *
 * // Start a root span
 * auto span = tracer.startSpan("db.query");
 * span->setAttribute("db.operation", "SELECT");
 *
 * // Attach metrics snapshot
 * SpanMetrics snap;
 * snap.cpu_usage_percent = 45.2;
 * tracer.recordMetrics(*span, snap);
 *
 * span->end();
 *
 * // Outbound call: inject trace context + baggage
 * std::map<std::string, std::string> outbound;
 * tracer.injectContext(outbound);
 * ```
 *
 * ### Thread Safety
 * All public methods are thread-safe.
 */

#include "core/concerns/i_tracer.h"
#include "observability/tracer.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace observability {

// ---------------------------------------------------------------------------
// ExporterType — supported tracing back-ends
// ---------------------------------------------------------------------------

/**
 * @brief Enumeration of supported tracing exporter back-ends.
 *
 * A single `OpenTelemetryTracer` can be configured with multiple exporters
 * (e.g. OTLP + JAEGER for dual-export during a migration).  Specify the
 * desired back-ends via `OTelConfig::exporters`.
 */
enum class ExporterType {
    OTLP,   ///< OTLP gRPC/HTTP (OpenTelemetry native; default)
    JAEGER, ///< Jaeger HTTP collector (also accepts OTLP on modern versions)
    ZIPKIN, ///< Zipkin HTTP collector (B3/JSON format)
};

// ---------------------------------------------------------------------------
// SpanContext — lightweight cross-boundary context carrier
// ---------------------------------------------------------------------------

/**
 * @brief Lightweight immutable snapshot of a span's propagation context.
 *
 * Returned by `OpenTelemetryTracer::extractContext()` so callers can forward
 * the upstream context to a child operation without holding a live `ISpan*`.
 */
struct SpanContext {
    std::string trace_id;  ///< 32-hex W3C trace-id
    std::string span_id;   ///< 16-hex W3C parent-id
    bool        sampled{true};

    /** @return true if both trace_id and span_id are non-empty. */
    bool isValid() const noexcept {
        return !trace_id.empty() && !span_id.empty();
    }
};

// ---------------------------------------------------------------------------
// SpanMetrics — runtime metrics to attach to a span
// ---------------------------------------------------------------------------

/**
 * @brief Snapshot of database runtime metrics that can be recorded as span
 *        attributes via `OpenTelemetryTracer::recordMetrics()`.
 *
 * All fields are optional; zero-values are silently omitted when recording.
 */
struct SpanMetrics {
    double  cpu_usage_percent{0.0};       ///< Host CPU utilisation [0–100]
    double  memory_usage_bytes{0.0};      ///< Process RSS in bytes
    int64_t active_connections{0};        ///< Current open client connections
    int64_t query_count{0};               ///< Queries executed since last flush
    double  cache_hit_rate{0.0};          ///< Block-cache hit rate [0.0–1.0]

    /// Arbitrary additional metrics (e.g. per-shard counters).
    std::map<std::string, double> custom;
};

// ---------------------------------------------------------------------------
// OTelConfig — tracer configuration
// ---------------------------------------------------------------------------

/**
 * @brief Configuration for `OpenTelemetryTracer`.
 *
 * Matches the specification in `src/observability/FUTURE_ENHANCEMENTS.md`
 * (section "OpenTelemetry Full Integration", Target v1.6.0).
 */
struct OTelConfig {
    /// Logical service name embedded in every span and resource.
    std::string service_name = "themisdb";

    /// Semantic-versioning service version (added as resource attribute).
    std::string service_version = "1.6.0";

    /// Exporter endpoint URL.  Protocol-specific:
    ///   - OTLP gRPC:  "http://otel-collector:4317"
    ///   - OTLP HTTP:  "http://otel-collector:4318"
    ///   - Jaeger:     "http://jaeger-collector:14268/api/traces"
    ///   - Zipkin:     "http://zipkin:9411/api/v2/spans"
    ///
    /// **OTLP only**: the tracer automatically appends "/v1/traces" when the
    /// endpoint does not already end with that path, so both base URLs
    /// ("http://otel:4318") and full URLs ("http://otel:4318/v1/traces") are
    /// accepted.  Jaeger and Zipkin endpoints are used exactly as supplied.
    std::string endpoint = "http://otel-collector:4317";

    /// Transport protocol for OTLP: "grpc" (default) or "http".
    /// This field only affects the OTLP exporter back-end; it is ignored
    /// when using Jaeger or Zipkin exporters, which always use their own
    /// fixed HTTP protocol.
    std::string protocol = "grpc";

    /// Span sampling rate in [0.0, 1.0].  1.0 = always-on (default).
    double sample_rate = 1.0;

    /// Additional OTel resource attributes (e.g. "deployment.environment").
    std::map<std::string, std::string> resource_attributes;

    /// List of active exporter back-ends.  Accepted values: "otlp", "jaeger",
    /// "zipkin".  Defaults to OTLP only.
    std::vector<std::string> exporters = {"otlp"};

    /// Maximum completed spans to retain in the in-process ring buffer.
    size_t max_retained_spans = 1000;

    /// When true, publish span counters to MetricsCollector on each startSpan.
    bool publish_metrics = true;
};

// ---------------------------------------------------------------------------
// OpenTelemetryTracer
// ---------------------------------------------------------------------------

/**
 * @brief Full OpenTelemetry integration tracer (v1.6.0).
 *
 * Implements `ITracer` with complete W3C Trace Context and W3C Baggage
 * propagation, configurable exporter back-ends, and higher-level helpers
 * for exception recording and metric attachment.
 *
 * The tracer is **non-copyable** but movable (via std::unique_ptr wrapping).
 * All methods are thread-safe.
 */
class OpenTelemetryTracer : public core::concerns::ITracer {
public:
    /**
     * @brief Construct with the supplied configuration.
     *
     * The tracer is immediately usable; no separate `initialize()` call is
     * required when using this constructor.
     *
     * @param config OTel configuration (service name, endpoint, exporters …)
     */
    explicit OpenTelemetryTracer(const OTelConfig& config = OTelConfig{});

    ~OpenTelemetryTracer() override;

    OpenTelemetryTracer(const OpenTelemetryTracer&)            = delete;
    OpenTelemetryTracer& operator=(const OpenTelemetryTracer&) = delete;

    // -----------------------------------------------------------------------
    // ITracer interface
    // -----------------------------------------------------------------------

    /** @brief Start a new root span (probabilistic sampling applied). */
    std::unique_ptr<ISpan> startSpan(const std::string& name) override;

    /** @brief Start a child span that inherits the parent's trace context. */
    std::unique_ptr<ISpan> startChildSpan(const std::string& name,
                                          const ISpan& parent) override;

    /**
     * @brief Extract W3C `traceparent` (and W3C `baggage`) from inbound
     *        headers and start a span as a child of the upstream trace.
     *
     * Baggage items are extracted into the thread-local `Baggage` store so
     * they are available via `getBaggageItem()` for the duration of the
     * current request.
     */
    std::unique_ptr<ISpan> startSpanFromHeaders(
        const std::string& name,
        const std::map<std::string, std::string>& headers) override;

    /**
     * @brief Inject the active span context and W3C Baggage into outbound
     *        HTTP headers (`traceparent` + `baggage` headers).
     */
    void injectContext(std::map<std::string, std::string>& headers) override;

    /** @brief Re-configure service name and endpoint at runtime. */
    bool initialize(const std::string& serviceName,
                    const std::string& endpoint) override;

    void shutdown() override;
    bool isInitialized() const override;
    void flush() noexcept override;
    core::concerns::ProbeResult isHealthy() const override;

    // -----------------------------------------------------------------------
    // Extended API (OpenTelemetry-specific additions to ITracer)
    // -----------------------------------------------------------------------

    /**
     * @brief Extract the W3C Trace Context from inbound HTTP headers.
     *
     * @param headers Inbound HTTP headers.
     * @return SpanContext with trace_id / span_id, or an invalid context if
     *         no valid `traceparent` header is present.
     */
    SpanContext extractContext(
        const std::map<std::string, std::string>& headers) const;

    /**
     * @brief Inject a specific span's context into outbound headers.
     *
     * Writes both `traceparent` and `baggage` headers.
     *
     * @param span    The span whose context to inject.
     * @param headers Outbound HTTP headers to populate.
     */
    void injectContext(const ISpan& span,
                       std::map<std::string, std::string>& headers);

    /**
     * @brief Record an exception as a span event following OTel conventions.
     *
     * Sets the span status to ERROR and attaches:
     *   - `exception.type`    = typeid(ex).name()
     *   - `exception.message` = ex.what()
     *
     * @param span The span to record the exception on.
     * @param ex   The exception to record.
     */
    void recordException(ISpan& span, const std::exception& ex);

    /**
    * @brief Attach a `SpanMetrics` as span attributes.
     *
     * Each non-zero metric field is added as a span attribute using the
     * `db.metrics.*` namespace (e.g. `db.metrics.cpu_usage_percent`).
    * Custom metrics in `SpanMetrics::custom` are prefixed with
     * `db.metrics.custom.`.
     *
     * @param span    Target span.
     * @param metrics Runtime metrics snapshot to record.
     */
    void recordMetrics(ISpan& span, const SpanMetrics& metrics);

    // -----------------------------------------------------------------------
    // Baggage — tenant / user context propagation
    // -----------------------------------------------------------------------

    /**
     * @brief Set a W3C Baggage item in the current thread's store.
     *
     * Common keys: `"tenant-id"`, `"user-id"`, `"session-id"`.
     * Items are automatically propagated by `injectContext()`.
     */
    static void setBaggageItem(const std::string& key,
                               const std::string& value);

    /**
     * @brief Get a W3C Baggage item from the current thread's store.
     * @return The value, or an empty string if not present.
     */
    static std::string getBaggageItem(const std::string& key);

    /**
     * @brief Remove a W3C Baggage item from the current thread's store.
     */
    static void removeBaggageItem(const std::string& key);

    /**
     * @brief Clear all W3C Baggage items for the current thread.
     */
    static void clearBaggage();

    /**
     * @brief Extract W3C Baggage from inbound HTTP headers into the
     *        current thread's baggage store.
     */
    static void extractBaggage(
        const std::map<std::string, std::string>& headers);

    // -----------------------------------------------------------------------
    // Diagnostics
    // -----------------------------------------------------------------------

    /** @brief Return a snapshot of span counters (total / active / dropped). */
    TracerStats stats() const;

    /** @brief Return a copy of the in-process completed-span ring buffer. */
    std::vector<SpanRecord> completedSpans() const;

    /** @brief Clear the in-process ring buffer. */
    void clearCompletedSpans();

    /** @brief Return the active configuration. */
    OTelConfig getConfig() const;

    /**
     * @brief Return the number of spans successfully exported to the OTLP
     *        backend since the tracer was constructed.
     *
     * Returns 0 when no OTLP exporter is configured or when the exporter has
     * not yet flushed its first batch.
     */
    uint64_t otlpExportedSpanCount() const noexcept;

    /**
     * @brief Return the number of spans dropped by the OTLP queue (due to
     *        back-pressure) since the tracer was constructed.
     */
    uint64_t otlpDroppedSpanCount() const noexcept;

    /**
     * @brief Return the list of configured exporter names (e.g. "otlp",
     *        "jaeger", "zipkin").
     */
    std::vector<std::string> activeExporters() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace observability
} // namespace themis
