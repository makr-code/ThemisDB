/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            otel_tracer_adapter.h                              ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:37:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     227                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • f20e6e8d74  2026-04-14  fix(build): eliminate remaining MSVC warnings in clean re... ║
    • 2826fa9ccd  2026-04-14  fix(build): eliminate remaining MSVC warnings in clean re... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 522e9ae575  2026-02-24  feat(core): implement OTel tracer adapter flush() via Tra... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "core/concerns/i_tracer.h"
#include "sharding/circuit_breaker.h"
#include "utils/tracing.h"

namespace themis {
namespace core {
namespace concerns {

/**
 * @brief OpenTelemetry adapter implementation of ITracer.
 * 
 * Wraps the existing OpenTelemetry-based tracer to implement the ITracer interface.
 * A circuit breaker guards span-export calls so that a failing or unreachable
 * OTLP endpoint does not block the critical path.  Once the circuit trips it
 * transitions to HALF_OPEN after `timeout` seconds to probe recovery.
 */
class OpenTelemetryTracerAdapter : public ITracer {
public:
    /**
     * @brief Configuration for the circuit breaker that guards OTLP export.
     */
    struct CircuitBreakerConfig {
        size_t failure_threshold = 5;
        std::chrono::seconds timeout = std::chrono::seconds(30);
        size_t success_threshold = 2;
    };

    explicit OpenTelemetryTracerAdapter(
        const CircuitBreakerConfig& cb_config)
    {
        sharding::CircuitBreaker::Config cfg;
        cfg.failure_threshold = cb_config.failure_threshold;
        cfg.timeout           = cb_config.timeout;
        cfg.success_threshold = cb_config.success_threshold;
        circuit_breaker_ = std::make_unique<sharding::CircuitBreaker>(cfg);
    }

    /// Construct with default circuit-breaker settings.
    OpenTelemetryTracerAdapter() : OpenTelemetryTracerAdapter(CircuitBreakerConfig{}) {}

    class OtelSpanAdapter : public ISpan {
    public:
        explicit OtelSpanAdapter(themis::Tracer::Span span)
            : span_(std::move(span)) {}

        void setAttribute(const std::string& key, const std::string& value) override {
            span_.setAttribute(key, value);
        }

        void setAttribute(const std::string& key, int64_t value) override {
            span_.setAttribute(key, value);
        }

        void setAttribute(const std::string& key, double value) override {
            span_.setAttribute(key, value);
        }

        void setAttribute(const std::string& key, bool value) override {
            span_.setAttribute(key, value);
        }

        void recordError(const std::string& errorMessage) override {
            span_.recordError(errorMessage);
        }

        void setStatus(bool ok, const std::string& description = "") override {
            span_.setStatus(ok, description);
        }

        void end() override {
            span_.end();
        }

        bool isValid() const override {
            return span_.isValid();
        }

        /// Explicitly end the span on destruction (RAII guarantee).
        ~OtelSpanAdapter() override { span_.end(); }

        themis::Tracer::Span& getSpan() { return span_; }
        const themis::Tracer::Span& getSpan() const { return span_; }

    private:
        themis::Tracer::Span span_;
    };

    std::unique_ptr<ISpan> startSpan(const std::string& name) override {
        if (!circuit_breaker_->allowRequest()) {
            // Circuit open: return a no-op span to avoid blocking callers
            return std::make_unique<OtelSpanAdapter>(themis::Tracer::Span{});
        }
        auto span_ptr = std::make_unique<OtelSpanAdapter>(themis::Tracer::startSpan(name));
        if (span_ptr->isValid()) {
            circuit_breaker_->recordSuccess();
        } else {
            circuit_breaker_->recordFailure();
        }
        return span_ptr;
    }

    std::unique_ptr<ISpan> startChildSpan(const std::string& name, const ISpan& parent) override {
        if (!circuit_breaker_->allowRequest()) {
            return std::make_unique<OtelSpanAdapter>(themis::Tracer::Span{});
        }
        auto* otelParent = dynamic_cast<const OtelSpanAdapter*>(&parent);
        std::unique_ptr<OtelSpanAdapter> span_ptr;
        if (otelParent) {
            span_ptr = std::make_unique<OtelSpanAdapter>(
                themis::Tracer::startChildSpan(name, otelParent->getSpan())
            );
        } else {
            span_ptr = std::make_unique<OtelSpanAdapter>(themis::Tracer::startSpan(name));
        }
        if (span_ptr->isValid()) {
            circuit_breaker_->recordSuccess();
        } else {
            circuit_breaker_->recordFailure();
        }
        return span_ptr;
    }

    std::unique_ptr<ISpan> startSpanFromHeaders(
            const std::string& name,
            const std::map<std::string, std::string>& carrier_headers) override {
        if (!circuit_breaker_->allowRequest()) {
            return std::make_unique<OtelSpanAdapter>(themis::Tracer::Span{});
        }
        auto span_ptr = std::make_unique<OtelSpanAdapter>(
            themis::Tracer::startSpanFromHeaders(name, carrier_headers));
        if (span_ptr->isValid()) {
            circuit_breaker_->recordSuccess();
        } else {
            circuit_breaker_->recordFailure();
        }
        return span_ptr;
    }

    void injectContext(std::map<std::string, std::string>& carrier_headers) override {
        auto trace_id = themis::Tracer::getCurrentTraceId();
        auto span_id  = themis::Tracer::getCurrentSpanId();
        if (!trace_id.empty() && !span_id.empty()) {
            carrier_headers["traceparent"] = "00-" + trace_id + "-" + span_id + "-01";
        }
        themis::Baggage::inject(carrier_headers);
    }

    bool initialize(const std::string& serviceName, const std::string& endpoint) override {
        if (initialized_) {
            // Already successfully initialized; avoid re-calling the global
            // Tracer which would return false (already initialized) and trip
            // the circuit breaker erroneously.
            return true;
        }
        bool ok = themis::Tracer::initialize(serviceName, endpoint);
        if (ok) {
            circuit_breaker_->recordSuccess();
        } else {
            circuit_breaker_->recordFailure();
        }
        initialized_ = ok;
        return ok;
    }

    void shutdown() override {
        themis::Tracer::shutdown();
        initialized_ = false;
    }

    bool isInitialized() const override {
        return initialized_;
    }

    // Lifecycle hooks
    void flush() noexcept override;

    ProbeResult isHealthy() const override {
        if (!initialized_) {
            return ProbeResult::unhealthy("tracer not initialized");
        }
        if (circuit_breaker_->getState() == sharding::CircuitBreaker::State::OPEN) {
            return ProbeResult::unhealthy("tracing exporter circuit-breaker OPEN");
        }
        return ProbeResult::healthy();
    }

    /** Expose circuit-breaker state for monitoring. */
    sharding::CircuitBreaker::State circuitBreakerState() const {
        return circuit_breaker_->getState();
    }

private:
    bool initialized_ = false;
    std::unique_ptr<sharding::CircuitBreaker> circuit_breaker_;
};

} // namespace concerns
} // namespace core
} // namespace themis
