/**
 * @file tracing.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <map>
#include <memory>
#include <optional>
#include <chrono>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <functional>

#ifdef THEMIS_ENABLE_TRACING
#include <opentelemetry/trace/provider.h>
#include <opentelemetry/trace/tracer.h>
#include <opentelemetry/context/context.h>
namespace otel = opentelemetry;
#endif

namespace themis {

// ─────────────────────────────────────────────────────────────────────────────
// SamplingStrategy – controls which spans are recorded
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Strategy for sampling traces.
 *
 * Three built-in strategies are provided:
 *  - ALWAYS_ON  (default)  – every span is recorded.
 *  - ALWAYS_OFF            – no spans are recorded (useful for benchmarks).
 *  - PROBABILITY           – each span is independently sampled with the
 *                            configured probability in [0.0, 1.0].
 *  - PARENT_BASED          – follow the sampling decision of the parent span;
 *                            falls back to PROBABILITY for root spans.
 *  - ADAPTIVE              – automatically scales the sample probability down
 *                            when the span creation rate exceeds a configured
 *                            maximum (spans/second), and restores full sampling
 *                            when the rate drops back below the threshold.
 */
class SamplingStrategy {
public:
    enum class Type {
        ALWAYS_ON,
        ALWAYS_OFF,
        PROBABILITY,
        PARENT_BASED,
        ADAPTIVE,
    };

    /// Configuration for the ADAPTIVE sampling strategy.
    struct AdaptiveConfig {
        double max_spans_per_second = 1000.0; ///< Target maximum span creation rate
        double min_rate             = 0.01;   ///< Minimum sample probability (floor)
        std::chrono::milliseconds window{1000}; ///< Rate-measurement window duration
    };

    /// Construct with the given strategy type.
    explicit SamplingStrategy(Type type = Type::ALWAYS_ON, double probability = 1.0)
        : type_(type), probability_(probability) {}

    static SamplingStrategy alwaysOn()  { return SamplingStrategy(Type::ALWAYS_ON, 1.0);  }
    static SamplingStrategy alwaysOff() { return SamplingStrategy(Type::ALWAYS_OFF, 0.0); }
    static SamplingStrategy probability(double p) {
        return SamplingStrategy(Type::PROBABILITY, p);
    }
    static SamplingStrategy parentBased(double root_probability = 1.0) {
        return SamplingStrategy(Type::PARENT_BASED, root_probability);
    }

    /// Adaptive sampler: automatically adjusts the sample probability based on
    /// the current span creation rate to keep throughput near @p config.max_spans_per_second.
    /// Copies of this strategy share the same rate-measurement state.
    static SamplingStrategy adaptive();
    static SamplingStrategy adaptive(AdaptiveConfig config);

    /// Returns true if a new span with the given parent-sampled flag should be recorded.
    bool shouldSample(bool parent_sampled = true) const;

    Type   type()        const { return type_; }
    double probability() const { return probability_; }

    /// Returns the current effective sample rate.
    /// For ADAPTIVE strategies this reflects the most recently computed rate;
    /// for all other strategies it equals probability().
    double getEffectiveRate() const;

private:
    Type   type_;
    double probability_;
    AdaptiveConfig adaptive_config_;

    /// Shared mutable state for ADAPTIVE mode (shared across copies).
    struct AdaptiveState {
        std::mutex mu;
        int64_t    window_count{0};
        std::chrono::steady_clock::time_point window_start{std::chrono::steady_clock::now()};
        double     effective_rate{1.0};
    };
    std::shared_ptr<AdaptiveState> adaptive_state_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Baggage – key/value metadata propagated across service boundaries
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief W3C Baggage – arbitrary key/value pairs propagated with every span.
 *
 * Baggage items are thread-local by default; call Baggage::set() /
 * Baggage::get() to access the current thread's baggage store.
 *
 * The serialised form follows the W3C Baggage header specification:
 *   key=value[,key=value]*
 *
 * Usage:
 *   Baggage::set("tenant-id", "acme");
 *   std::string tid = Baggage::get("tenant-id");
 *   auto headers    = Baggage::inject();          // {"baggage": "tenant-id=acme"}
 *   Baggage::extract(incomingHeaders);            // populate from inbound request
 */
class Baggage {
public:
    using BaggageMap = std::unordered_map<std::string, std::string>;

    /// Set a single baggage item in the current thread's store.
    static void set(const std::string& key, const std::string& value);

    /// Get a single baggage item; returns empty string if not present.
    static std::string get(const std::string& key);

    /// Remove a baggage item from the current thread's store.
    static void remove(const std::string& key);

    /// Clear all baggage items for the current thread.
    static void clear();

    /// Return all baggage items for the current thread.
    static BaggageMap getAll();

    /// Serialize to a W3C Baggage header value string.
    static std::string serialize();

    /// Inject baggage into an outgoing header map.
    static void inject(std::map<std::string, std::string>& headers);

    /// Extract baggage from an incoming header map and merge into the
    /// current thread's store.
    static void extract(const std::map<std::string, std::string>& headers);

private:
    static thread_local BaggageMap thread_baggage_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Tracer
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Tracer wrapper for OpenTelemetry distributed tracing
 * 
 * Provides a simple interface for creating spans and managing trace context.
 * When THEMIS_ENABLE_TRACING is not defined, this becomes a no-op wrapper.
 * 
 * Usage:
 *   auto span = Tracer::startSpan("handleAqlQuery");
 *   span.setAttribute("query.table", "users");
 *   // ... work happens ...
 *   span.end(); // or rely on RAII destructor
 */
class Tracer {
public:
    /**
     * @brief Initialize the global tracer with OTLP HTTP exporter.
     *
     * Probes the collector endpoint (3-second timeout) before registering the
     * OTLP exporter. If the probe fails the tracer is marked initialized but
     * operates in no-op mode (every span is discarded), bounding latency impact.
     *
     * @param serviceName Name of this service (e.g., "themis-server")
     * @param endpoint OTLP HTTP endpoint (e.g., "http://localhost:4318")
     * @return true if initialization succeeded and backend is reachable;
     *         false if backend unreachable (no-op mode, fail-open)
     *
     * @error_contract
     * | Condition | ErrorCode | Severity | Logging | Recovery |
     * |-----------|-----------|----------|---------|----------|
     * | Collector DNS resolution fails | TRACE_EXPORT_FAILED (9031) | Warning | host, port, error | Return false; use no-op spans (fail-open) |
     * | Collector TCP connect fails (timeout 3 s) | TRACE_EXPORT_FAILED (9031) | Warning | host, port, error | Return false; use no-op spans (fail-open) |
     * | Probe throws unexpected exception | TRACE_EXPORT_FAILED (9031) | Warning | error | Return false; use no-op spans (fail-open) |
     *
     * @degradation fail-open – backend unavailable produces no-op spans; latency bounded to 3 s probe
     * @see ErrorCode 9030-9039 for tracing error taxonomy
     */
    static bool initialize(const std::string& serviceName, const std::string& endpoint);
    
    /**
     * Shutdown the tracer and flush remaining spans
     */
    static void shutdown();
    
    /**
     * Span represents an active trace span with RAII lifetime
     */
    class Span {
    public:
        Span() = default;
        ~Span();
        
        // Move-only semantics
        Span(Span&& other) noexcept;
        Span& operator=(Span&& other) noexcept;
        Span(const Span&) = delete;
        Span& operator=(const Span&) = delete;
        
        /**
         * Add an attribute to this span
         */
        void setAttribute(const std::string& key, const std::string& value);
        void setAttribute(const std::string& key, int64_t value);
        void setAttribute(const std::string& key, double value);
        void setAttribute(const std::string& key, bool value);
        
        /**
         * Record an error event on this span
         * 
         * Logs an error to the current span for observability and distributed tracing.
         * Phase 2.10: Comprehensive error contract documentation.
         * 
         * @param errorMessage Human-readable error description
         * 
         * @return void
         * 
         * @error_contract
         * **Phase 2.3 Error Codes (7300-7309, 7362):**
         * - ERR_TRACING_DEGRADED (7362): Error logging fails (non-fatal)
         *   - Recovery: Error is logged locally; span continues
         *   - Severity: WARNING
         *   - User Action: Check tracing backend availability
         * 
         * **Span Error Semantics:**
         * - Error events are aggregated in span attributes
         * - Errors do NOT automatically end span; span continues until end() or destructor
         * - Multiple errors can be recorded per span
         * - Tracing disabled: error call is no-op (valid_ = false)
         * 
         * @thread_safety NOT thread-safe; one span per thread/context
         * @performance O(1) for local error logging; depends on tracing backend
         * 
         * @see setStatus() to mark span with error status code
         * @see ErrorCode::ERR_TRACING_DEGRADED for tracing failures
         */
        void recordError(const std::string& errorMessage);
        
        /**
         * Mark span as error with status code
         */
        void setStatus(bool ok, const std::string& description = "");
        
        /**
         * Explicitly end the span (otherwise destructor will end it)
         */
        void end();
        
        /**
         * Check if this span is valid (i.e., tracing is enabled)
         */
        bool isValid() const { return valid_; }
        
        /**
         * Get span duration in milliseconds (for metrics)
         */
        double durationMs() const;
        
    private:
        friend class Tracer;
        
#ifdef THEMIS_ENABLE_TRACING
        explicit Span(otel::nostd::shared_ptr<otel::trace::Span> span);
        otel::nostd::shared_ptr<otel::trace::Span> span_;
        otel::context::Context context_;
        std::chrono::steady_clock::time_point start_time_;
#endif
        bool valid_ = false;
        bool ended_ = false;
    };
    
    /**
     * Start a new span with the given name
     * 
     * Creates a child span of the currently active span (if any).
     * Phase 2.10: Comprehensive error contract documentation.
     * 
     * @param name Span name for display in tracing backend
     * 
     * @return Span RAII object; span ends when object is destroyed
     * 
     * @error_contract
     * **Phase 2.3 Error Codes (7300-7309, 7362):**
     * - ERR_TRACING_DEGRADED (7362): Span creation fails
     *   - Recovery: Returns invalid span (valid_ = false); no-op operations
     *   - Severity: WARNING
     *   - User Action: Check tracing backend availability
     * 
     * **Span Creation Semantics:**
     * - Span becomes child of active context (if set)
     * - Span lifetime bound to returned object (RAII)
     * - Tracing disabled: returns empty span (valid_ = false)
     * - Span name is immutable; cannot be changed after creation
     * 
     * @bounded_resources
     * - Active spans limited by tracing backend configuration
     * - Resource check: if span count exceeds limit, returns invalid span
     * 
     * @thread_safety NOT thread-safe; call from trace context thread
     * @performance O(1) amortized; O(n) on backend when span count exceeds threshold
     * 
     * @see startChildSpan() to explicitly specify parent
     * @see startSpanFromHeaders() for distributed tracing
     * @see Span::isValid() to check if creation succeeded
     */
    static Span startSpan(const std::string& name);
     
    /**
     * Start a new span as a child of the given parent span
     * 
     * Creates a child span explicitly linked to a parent span.
     * Useful when the parent is not the active context.
     * Phase 2.10: Comprehensive error contract documentation.
     * 
     * @param name Span name for display in tracing backend
     * @param parent Parent span to link this span to
     * 
     * @return Span RAII object; span ends when object is destroyed
     * 
     * @error_contract
     * **Phase 2.3 Error Codes (7300-7309, 7362):**
     * - ERR_TRACING_DEGRADED (7362): Span creation fails
     *   - Recovery: Returns invalid span (valid_ = false); no-op operations
     *   - Severity: WARNING
     *   - User Action: Check tracing backend availability
     * 
     * **Child Span Semantics:**
     * - Creates explicit parent-child relationship in trace tree
     * - Parent span can be on different thread/context
     * - Parent validity is not checked; invalid parent creates orphan span
     * - Child inherits parent's trace context and baggage
     * 
     * @bounded_resources
     * - Span hierarchy depth: limited by tracing backend
     * - Resource check: if span tree exceeds depth limit, returns invalid span
     * 
     * @thread_safety NOT thread-safe; call from trace context thread
     * @performance O(1) amortized; depends on trace tree depth
     * 
     * @see startSpan() for automatic parent detection
     * @see startSpanFromHeaders() for distributed tracing
     */
    static Span startChildSpan(const std::string& name, const Span& parent);

    /**
     * Start a new root span using W3C TraceContext headers for context propagation
     *
     * Extracts the `traceparent` (and optionally `tracestate`) header from the
     * provided header map and creates a span that is a child of the upstream trace
     * context.  This enables distributed tracing across service boundaries: callers
     * (API gateways, service meshes, SDKs) can propagate their trace IDs into
     * ThemisDB so that all internal spans appear in the same distributed trace.
     *
     * Format of `traceparent` (W3C Trace Context Level 1):
     *   version-traceid-parentid-flags
     *   e.g. "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01"
     *
     * When THEMIS_ENABLE_TRACING is not defined, this falls back to startSpan()
     * and records the raw `traceparent` value as an attribute for log correlation.
     *
     * Phase 2.10: Comprehensive error contract documentation.
     *
     * @param name       Span name
     * @param headers    HTTP request headers (case-insensitive key lookup)
     * 
     * @return Span RAII object; span ends when object is destroyed
     * 
     * @error_contract
     * **Phase 2.3 Error Codes (7300-7309, 7362):**
     * - ERR_TRACING_DEGRADED (7362): W3C header parsing fails
     *   - Recovery: Falls back to startSpan() without distributed trace context
     *   - Severity: WARNING
     *   - User Action: Validate traceparent header format
     * 
     * - ERR_TRACING_DEGRADED (7362): Span creation fails after parsing
     *   - Recovery: Returns invalid span (valid_ = false)
     *   - Severity: WARNING
     *   - User Action: Check tracing backend availability
     * 
     * **W3C Trace Context Semantics:**
     * - Parses `traceparent` header using W3C Trace Context Level 1 format
     * - Optional `tracestate` header preserved for backend baggage
     * - If headers missing: creates root span (new trace ID)
     * - Invalid header format: logged as warning; root span created
     * 
     * **Distributed Trace Propagation:**
     * - Enables end-to-end tracing across service boundaries
     * - Incoming trace ID is preserved in resulting span
     * - All child spans inherit propagated trace context
     * - Useful for API gateway → ThemisDB → backend service chains
     * 
     * @bounded_resources
     * - Header parsing: O(header_count) for case-insensitive lookup
     * - Resource check: no additional resources required
     * 
     * @thread_safety NOT thread-safe; call from trace context thread
     * @performance O(header_count) for header parsing; O(1) for span creation
     * 
     * @see startSpan() for basic span creation without distributed context
     * @see startChildSpan() for explicit parent linking
     * @see https://www.w3.org/TR/trace-context/ for W3C Trace Context specification
     */
    static Span startSpanFromHeaders(
        const std::string& name,
        const std::map<std::string, std::string>& headers);
    
    /**
     * Get total number of spans created (for metrics)
     */
    static int64_t getTotalSpans();
    
    /**
     * Get active span count (for metrics)
     */
    static int64_t getActiveSpans();

    /**
     * Configure the sampling strategy used for new root spans.
     * This takes effect for all spans created after this call.
     */
    static void setSamplingStrategy(const SamplingStrategy& strategy);

    /**
     * Get the current sampling strategy.
     */
    static SamplingStrategy getSamplingStrategy();

    /**
     * Get the trace-ID of the most recently started span on this thread
     * as a 32-character hex string, or an empty string when tracing is
     * disabled / no span is active.
     *
     * Primarily intended for injecting the trace-ID into structured log
     * messages to correlate logs with traces.
     */
    static std::string getCurrentTraceId();

    /**
     * Get the span-ID of the most recently started span on this thread
     * as a 16-character hex string, or empty string when unavailable.
     */
    static std::string getCurrentSpanId();

    /**
     * Flush any buffered or in-flight spans to the exporter.
     *
     * Calls ForceFlush() on the underlying SDK TracerProvider so that all
     * spans queued in the processor's internal buffer are exported before the
     * call returns (subject to @p timeout).  When THEMIS_ENABLE_TRACING is
     * not defined this is a no-op and always returns true.
     *
     * @param timeout  Maximum time to wait for the flush to complete.
     *                 Defaults to 5 seconds.
     * @return true if all pending spans were exported within the timeout,
     *         false if the timeout expired or no provider is active.
     */
    static bool flush(std::chrono::microseconds timeout =
                          std::chrono::duration_cast<std::chrono::microseconds>(
                              std::chrono::seconds(5))) noexcept;

private:
#ifdef THEMIS_ENABLE_TRACING
    static otel::nostd::shared_ptr<otel::trace::Tracer> getTracer();
    static otel::nostd::shared_ptr<otel::trace::Tracer> tracer_;
#endif
    static bool initialized_;
    static std::atomic<int64_t> total_spans_;
    static std::atomic<int64_t> active_spans_;
    static SamplingStrategy sampling_strategy_;
    static std::mutex sampling_mu_;
};

/**
 * RAII helper for scoped spans
 * 
 * Usage:
 *   void myFunction() {
 *       ScopedSpan span("myFunction");
 *       span.setAttribute("param", value);
 *       // ... work ...
 *   } // span ends automatically
 */
class ScopedSpan {
public:
    explicit ScopedSpan(const std::string& name) : span_(Tracer::startSpan(name)), name_(name) {}
    
    void setAttribute(const std::string& key, const std::string& value) {
        span_.setAttribute(key, value);
    }
    
    void setAttribute(const std::string& key, int64_t value) {
        span_.setAttribute(key, value);
    }
    
    void setAttribute(const std::string& key, double value) {
        span_.setAttribute(key, value);
    }
    
    void setAttribute(const std::string& key, bool value) {
        span_.setAttribute(key, value);
    }
    
    void recordError(const std::string& errorMessage) {
        span_.recordError(errorMessage);
    }
    
    void setStatus(bool ok, const std::string& description = "") {
        span_.setStatus(ok, description);
    }
    
    Tracer::Span& span() { return span_; }
    
    ~ScopedSpan();
    
private:
    Tracer::Span span_;
    std::string name_;
};

/**
 * RAII helper for scoped spans with automatic metrics recording
 * 
 * Usage:
 *   void myFunction() {
 *       TracedSpan span("myFunction");
 *       span.setAttribute("param", value);
 *       // ... work ...
 *   } // span ends and duration is automatically recorded to Prometheus
 */
class TracedSpan {
public:
    explicit TracedSpan(const std::string& name);
    ~TracedSpan();
    
    void setAttribute(const std::string& key, const std::string& value);
    void setAttribute(const std::string& key, int64_t value);
    void setAttribute(const std::string& key, double value);
    void setAttribute(const std::string& key, bool value);
    void recordError(const std::string& errorMessage);
    void setStatus(bool ok, const std::string& description = "");
    Tracer::Span& span();
    
private:
    Tracer::Span span_;
    std::string name_;
    std::chrono::steady_clock::time_point start_time_;
};

} // namespace themis
