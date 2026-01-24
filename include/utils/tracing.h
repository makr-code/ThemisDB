#pragma once

#include <string>
#include <memory>
#include <optional>

#ifdef THEMIS_ENABLE_TRACING
#include <opentelemetry/trace/provider.h>
#include <opentelemetry/trace/tracer.h>
#include <opentelemetry/context/context.h>
namespace otel = opentelemetry;
#endif

namespace themis {

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
     * Initialize the global tracer with OTLP HTTP exporter
     * @param serviceName Name of this service (e.g., "themis-server")
     * @param endpoint OTLP HTTP endpoint (e.g., "http://localhost:4318")
     * @return true if initialization succeeded
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
     * The span will be a child of the current active span (if any)
     */
    static Span startSpan(const std::string& name);
    
    /**
     * Start a new span as a child of the given parent span
     */
    static Span startChildSpan(const std::string& name, const Span& parent);
    
    /**
     * Get total number of spans created (for metrics)
     */
    static int64_t getTotalSpans();
    
    /**
     * Get active span count (for metrics)
     */
    static int64_t getActiveSpans();
    
private:
#ifdef THEMIS_ENABLE_TRACING
    static otel::nostd::shared_ptr<otel::trace::Tracer> getTracer();
    static otel::nostd::shared_ptr<otel::trace::Tracer> tracer_;
#endif
    static bool initialized_;
    static std::atomic<int64_t> total_spans_;
    static std::atomic<int64_t> active_spans_;
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
