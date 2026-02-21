/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            i_tracer.h                                         ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 13:48:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     258                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "core/concerns/lifecycle.h"
#include <string>
#include <memory>
#include <map>

namespace themis {
namespace core {
namespace concerns {

/**
 * @brief Abstract tracer interface for distributed tracing.
 * 
 * Provides a unified interface for distributed tracing that can be
 * implemented by various tracing backends (OpenTelemetry, Jaeger, no-op, etc.).
 * Enables testing with mock tracers and runtime switching of implementations.
 */
class ITracer {
public:
    /**
     * @brief Represents an active trace span with RAII lifetime.
     *
     * Obtain a span via ITracer::startSpan() or ITracer::startChildSpan().
     * Spans are automatically ended when the unique_ptr is destroyed; call
     * end() explicitly if you need to finish the span before destruction.
     */
    class ISpan {
    public:
        virtual ~ISpan() = default;

        /**
         * @brief Set a string attribute on the span.
         * @param key   Attribute name (OpenTelemetry semantic convention recommended).
         * @param value String value.
         */
        virtual void setAttribute(const std::string& key, const std::string& value) = 0;

        /**
         * @brief Set an integer attribute on the span.
         * @param key   Attribute name.
         * @param value 64-bit integer value.
         */
        virtual void setAttribute(const std::string& key, int64_t value) = 0;

        /**
         * @brief Set a double attribute on the span.
         * @param key   Attribute name.
         * @param value Floating-point value.
         */
        virtual void setAttribute(const std::string& key, double value) = 0;

        /**
         * @brief Set a boolean attribute on the span.
         * @param key   Attribute name.
         * @param value Boolean value.
         */
        virtual void setAttribute(const std::string& key, bool value) = 0;

        /**
         * @brief Record an error event on the span.
         *
         * Sets the span status to ERROR and attaches @p errorMessage as an
         * event attribute so the trace backend can display it.
         *
         * @param errorMessage Human-readable description of the error.
         */
        virtual void recordError(const std::string& errorMessage) = 0;

        /**
         * @brief Set the overall status of the span.
         * @param ok          true = OK, false = ERROR.
         * @param description Optional human-readable status description.
         */
        virtual void setStatus(bool ok, const std::string& description = "") = 0;

        /**
         * @brief Finish the span and export it to the backend.
         *
         * Must be called at most once.  After end() no other methods should
         * be called on the span.
         */
        virtual void end() = 0;

        /**
         * @brief Return true if the span represents a real, exportable trace.
         *
         * No-op spans (from NoOpTracer) return false.
         * @return true for real spans, false for no-op spans.
         */
        virtual bool isValid() const = 0;
    };

    virtual ~ITracer() = default;

    // -----------------------------------------------------------------------
    // Span creation methods
    // -----------------------------------------------------------------------

    /**
     * @brief Start a new root span.
     *
     * The returned span becomes the active span and should be ended (via
     * ISpan::end() or by destroying the unique_ptr) when the operation completes.
     *
     * @param name Span name (e.g. "database.query").
     * @return Unique ownership of the new span; never null.
     */
    virtual std::unique_ptr<ISpan> startSpan(const std::string& name) = 0;

    /**
     * @brief Start a child span that continues a parent span's trace.
     *
     * @param name   Child span name.
     * @param parent Parent span whose trace context is propagated.
     * @return Unique ownership of the new child span; never null.
     */
    virtual std::unique_ptr<ISpan> startChildSpan(const std::string& name, const ISpan& parent) = 0;

    // -----------------------------------------------------------------------
    // Initialization and cleanup
    // -----------------------------------------------------------------------

    /**
     * @brief Initialize the tracer and connect to the exporter endpoint.
     *
     * Must be called before any span creation.  Calling it more than once
     * is implementation-defined but typically a no-op if already initialized.
     *
     * @param serviceName Logical service name embedded in every span.
     * @param endpoint    Exporter URL (e.g. "http://localhost:4318" for OTLP/HTTP).
     * @return true on success, false if initialization failed.
     */
    virtual bool initialize(const std::string& serviceName, const std::string& endpoint) = 0;

    /**
     * @brief Shut down the tracer and flush any pending spans.
     *
     * After shutdown() no new spans should be created.
     */
    virtual void shutdown() = 0;

    /**
     * @brief Return true if the tracer has been successfully initialized.
     * @return true after a successful initialize() call.
     */
    virtual bool isInitialized() const = 0;

    // -----------------------------------------------------------------------
    // Lifecycle hooks
    // -----------------------------------------------------------------------

    /**
     * @brief Flush any pending spans to the exporter.
     *
     * Should be called before shutdown() to ensure all in-flight spans
     * are exported.  Default is a no-op.
     */
    virtual void flush() noexcept {}

    /**
     * @brief Probe whether the tracing exporter is reachable and healthy.
     *
     * @return ProbeResult with ok=true when the exporter is reachable,
     *         ok=false (e.g. circuit-breaker OPEN) otherwise.
     */
    virtual ProbeResult isHealthy() const { return ProbeResult::healthy(); }
};

/**
 * @brief RAII helper for scoped spans.
 *
 * Wraps an ISpan unique_ptr and delegates all ISpan methods.  The span is
 * automatically ended when the ScopedSpan object is destroyed.
 *
 * Example:
 * @code
 *   ScopedSpan span(tracer, "db.query");
 *   span.setAttribute("db.table", "users");
 *   // span.end() is called in ~ScopedSpan()
 * @endcode
 */
class ScopedSpan {
public:
    /**
     * @brief Construct and immediately start a new span.
     * @param tracer Tracer that owns the span.
     * @param name   Span name.
     */
    explicit ScopedSpan(ITracer& tracer, const std::string& name)
        : span_(tracer.startSpan(name)) {}

    /// @brief Delegate to ISpan::setAttribute(string).
    void setAttribute(const std::string& key, const std::string& value) {
        if (span_) span_->setAttribute(key, value);
    }

    /// @brief Delegate to ISpan::setAttribute(int64_t).
    void setAttribute(const std::string& key, int64_t value) {
        if (span_) span_->setAttribute(key, value);
    }

    /// @brief Delegate to ISpan::setAttribute(double).
    void setAttribute(const std::string& key, double value) {
        if (span_) span_->setAttribute(key, value);
    }

    /// @brief Delegate to ISpan::setAttribute(bool).
    void setAttribute(const std::string& key, bool value) {
        if (span_) span_->setAttribute(key, value);
    }

    /// @brief Delegate to ISpan::recordError().
    void recordError(const std::string& errorMessage) {
        if (span_) span_->recordError(errorMessage);
    }

    /// @brief Delegate to ISpan::setStatus().
    void setStatus(bool ok, const std::string& description = "") {
        if (span_) span_->setStatus(ok, description);
    }

    /// @brief Return a raw pointer to the underlying ISpan (may be null for no-op).
    ITracer::ISpan* span() { return span_.get(); }

private:
    std::unique_ptr<ITracer::ISpan> span_;
};

} // namespace concerns
} // namespace core
} // namespace themis
