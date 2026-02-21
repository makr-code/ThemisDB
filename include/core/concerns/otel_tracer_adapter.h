/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            otel_tracer_adapter.h                              ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-02-21 13:56:32                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     194                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
        const CircuitBreakerConfig& cb_config = CircuitBreakerConfig{})
    {
        sharding::CircuitBreaker::Config cfg;
        cfg.failure_threshold = cb_config.failure_threshold;
        cfg.timeout           = cb_config.timeout;
        cfg.success_threshold = cb_config.success_threshold;
        circuit_breaker_ = std::make_unique<sharding::CircuitBreaker>(cfg);
    }

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

        themis::Tracer::Span& getSpan() { return span_; }

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
                themis::Tracer::startChildSpan(name, const_cast<OtelSpanAdapter*>(otelParent)->getSpan())
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

    bool initialize(const std::string& serviceName, const std::string& endpoint) override {
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
    void flush() noexcept override {
        // Spans are exported asynchronously; shutdown/restart would be
        // destructive.  A best-effort flush is performed via the OTLP
        // exporter's internal queue drain – no public API available here,
        // so this is intentionally a no-op at the adapter level.
    }

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
