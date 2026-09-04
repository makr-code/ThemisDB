/**
 * @file jaeger_tracer_adapter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "core/concerns/i_tracer.h"
#include "sharding/circuit_breaker.h"
#include "utils/tracing.h"

#include <algorithm>
#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace core {
namespace concerns {

/**
 * @brief Jaeger tracing backend adapter implementing ITracer.
 *
 * Wraps the existing OpenTelemetry-based Tracer to target a Jaeger backend.
 * Modern Jaeger (1.35+) accepts OTLP traces on its OTLP-HTTP port (default
 * 4318), so this adapter uses the same underlying SDK as
 * OpenTelemetryTracerAdapter but applies Jaeger-specific defaults and handles
 * the Jaeger proprietary `uber-trace-id` propagation header.
 *
 * Default collector endpoint: http://localhost:14268/api/traces
 * (Jaeger HTTP collector, Thrift-over-HTTP; also accepted as an OTLP
 * endpoint when Jaeger's all-in-one binary is started with --collector.otlp.enabled).
 *
 * Header propagation:
 *   - startSpanFromHeaders() recognises the standard W3C `traceparent` header
 *     first. If that header is absent, it falls back to the Jaeger
 *     `uber-trace-id` header (format: {traceId}:{spanId}:{parentSpanId}:{flags})
 *     and records the decoded values as span attributes for correlation.
 *   - injectContext() writes both the Jaeger `uber-trace-id` header and the
 *     W3C `traceparent` header so downstream services using either convention
 *     can continue the trace.
 *
 * A circuit breaker guards every span-export call so that a failing or
 * unreachable Jaeger instance does not block the critical path. When the
 * breaker is open, span creation degrades to no-op spans.
 */
class JaegerTracerAdapter : public ITracer {
public:
    /// Default Jaeger HTTP collector endpoint.
    static constexpr const char* kDefaultEndpoint = "http://localhost:14268/api/traces";

    /**
     * @brief Configuration for the circuit breaker that guards Jaeger export.
     */
    struct CircuitBreakerConfig {
        size_t failure_threshold = 5;
        std::chrono::seconds timeout = std::chrono::seconds(30);
        size_t success_threshold = 2;
    };

    explicit JaegerTracerAdapter(const CircuitBreakerConfig& cb_config) {
        sharding::CircuitBreaker::Config cfg;
        cfg.failure_threshold = cb_config.failure_threshold;
        cfg.timeout           = cb_config.timeout;
        cfg.success_threshold = cb_config.success_threshold;
        circuit_breaker_ = std::make_unique<sharding::CircuitBreaker>(cfg);
    }

    /// Construct with default circuit-breaker settings.
    JaegerTracerAdapter() : JaegerTracerAdapter(CircuitBreakerConfig{}) {}

    // -------------------------------------------------------------------------
    // ISpan adapter – delegates to themis::Tracer::Span
    // -------------------------------------------------------------------------

    /** @brief ISpan adapter – delegates to themis::Tracer::Span. */
    class JaegerSpanAdapter : public ISpan {
    public:
        explicit JaegerSpanAdapter(themis::Tracer::Span span)
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

        ~JaegerSpanAdapter() override { span_.end(); }

        themis::Tracer::Span& getSpan()             { return span_; }
        const themis::Tracer::Span& getSpan() const { return span_; }

    private:
        themis::Tracer::Span span_;
    };

    // -------------------------------------------------------------------------
    // ITracer – span creation
    // -------------------------------------------------------------------------

    std::unique_ptr<ISpan> startSpan(const std::string& name) override {
        if (!circuit_breaker_->allowRequest()) {
            return std::make_unique<JaegerSpanAdapter>(themis::Tracer::Span{});
        }
        auto span_ptr = std::make_unique<JaegerSpanAdapter>(themis::Tracer::startSpan(name));
        span_ptr->isValid() ? circuit_breaker_->recordSuccess()
                            : circuit_breaker_->recordFailure();
        return span_ptr;
    }

    std::unique_ptr<ISpan> startChildSpan(const std::string& name,
                                           const ISpan& parent) override {
        if (!circuit_breaker_->allowRequest()) {
            return std::make_unique<JaegerSpanAdapter>(themis::Tracer::Span{});
        }
        std::unique_ptr<JaegerSpanAdapter> span_ptr;
        const auto* jaeger_parent = dynamic_cast<const JaegerSpanAdapter*>(&parent);
        if (jaeger_parent) {
            span_ptr = std::make_unique<JaegerSpanAdapter>(
                themis::Tracer::startChildSpan(name, jaeger_parent->getSpan()));
        } else {
            span_ptr = std::make_unique<JaegerSpanAdapter>(
                themis::Tracer::startSpan(name));
        }
        span_ptr->isValid() ? circuit_breaker_->recordSuccess()
                            : circuit_breaker_->recordFailure();
        return span_ptr;
    }

    /**
    * @brief Extract trace context from inbound headers and start a child span.
     *
    * Checks for headers in the following priority order:
    *  1. W3C `traceparent` header (takes precedence when present).
    *  2. Jaeger `uber-trace-id` header — the decoded IDs are recorded on the
    *     new span so the trace can be correlated in the Jaeger UI even when
    *     the full W3C context is absent.
    *  3. Falls back to a new root span when neither header is present.
     */
    std::unique_ptr<ISpan> startSpanFromHeaders(
            const std::string& name,
            const std::map<std::string, std::string>& carrier_headers) override {
        if (!circuit_breaker_->allowRequest()) {
            return std::make_unique<JaegerSpanAdapter>(themis::Tracer::Span{});
        }

        // W3C traceparent takes precedence – delegate to the base tracer.
        std::string traceparent = headerValueCI(carrier_headers, "traceparent");
        if (!traceparent.empty()) {
            auto span_ptr = std::make_unique<JaegerSpanAdapter>(
                themis::Tracer::startSpanFromHeaders(name, carrier_headers));
            span_ptr->isValid() ? circuit_breaker_->recordSuccess()
                                : circuit_breaker_->recordFailure();
            return span_ptr;
        }

        // Fall back to Jaeger uber-trace-id.
        std::string uber_trace_id = headerValueCI(carrier_headers, "uber-trace-id");
        auto span_ptr = std::make_unique<JaegerSpanAdapter>(
            themis::Tracer::startSpan(name));

        if (!uber_trace_id.empty()) {
            // Record the raw Jaeger header as an attribute for correlation in
            // the Jaeger UI. The parsed IDs are also attached individually so
            // log-trace correlation queries can filter by trace/span ID.
            span_ptr->setAttribute("jaeger.uber-trace-id", uber_trace_id);
            UberTraceIds ids = {};
            if (parseUberTraceId(uber_trace_id, ids)) {
                span_ptr->setAttribute("jaeger.trace_id",  ids.trace_id);
                span_ptr->setAttribute("jaeger.span_id",   ids.span_id);
                span_ptr->setAttribute("jaeger.parent_id", ids.parent_id);
                span_ptr->setAttribute("jaeger.flags",     ids.flags);
            }
        }

        // Also extract W3C Baggage.
        themis::Baggage::extract(carrier_headers);

        span_ptr->isValid() ? circuit_breaker_->recordSuccess()
                            : circuit_breaker_->recordFailure();
        return span_ptr;
    }

    /**
     * @brief Inject trace context into outgoing headers.
     *
    * Writes both the W3C `traceparent` header and the Jaeger `uber-trace-id`
    * header so downstream services using either convention can continue the
    * trace. Also injects W3C Baggage when any items are present.
     */
    void injectContext(std::map<std::string, std::string>& carrier_headers) override {
        std::string trace_id = themis::Tracer::getCurrentTraceId();
        std::string span_id  = themis::Tracer::getCurrentSpanId();

        if (!trace_id.empty() && !span_id.empty()) {
            // W3C traceparent
            carrier_headers["traceparent"] = "00-" + trace_id + "-" + span_id + "-01";

            // Jaeger uber-trace-id: {traceId}:{spanId}:{parentSpanId}:{flags}
            // Use "0" as the parent-span-id for the root of the outbound leg.
            carrier_headers["uber-trace-id"] = trace_id + ":" + span_id + ":0:1";
        }

        themis::Baggage::inject(carrier_headers);
    }

    // -------------------------------------------------------------------------
    // ITracer – lifecycle
    // -------------------------------------------------------------------------

    bool initialize(const std::string& serviceName, const std::string& endpoint) override {
        if (initialized_) {
            return true;
        }
        bool ok = themis::Tracer::initialize(serviceName, endpoint);
        ok ? circuit_breaker_->recordSuccess() : circuit_breaker_->recordFailure();
        initialized_ = ok;
        return ok;
    }

    void shutdown() override {
        themis::Tracer::shutdown();
        initialized_ = false;
    }

    bool isInitialized() const override { return initialized_; }

    void flush() noexcept override {
        themis::Tracer::flush();
    }

    ProbeResult isHealthy() const override {
        if (!initialized_) {
            return ProbeResult::unhealthy("Jaeger tracer not initialized");
        }
        if (circuit_breaker_->getState() == sharding::CircuitBreaker::State::OPEN) {
            return ProbeResult::unhealthy("Jaeger exporter circuit-breaker OPEN");
        }
        return ProbeResult::healthy();
    }

    /// Expose circuit-breaker state for monitoring.
    sharding::CircuitBreaker::State circuitBreakerState() const {
        return circuit_breaker_->getState();
    }

private:
    bool initialized_ = false;
    std::unique_ptr<sharding::CircuitBreaker> circuit_breaker_;

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    /// Case-insensitive header lookup.
    static std::string headerValueCI(const std::map<std::string, std::string>& headers,
                                     const std::string& name) {
        auto it = headers.find(name);
        if (it != headers.end()) {
          return it->second;
        }
        for (const auto& [k, v] : headers) {
            if (k.size() == name.size() &&
                std::equal(k.begin(), k.end(), name.begin(),
                           [](unsigned char a, unsigned char b) {
                               return std::tolower(a) == std::tolower(b);
                           })) {
                return v;
            }
        }
        return {};
    }

    struct UberTraceIds {
        std::string trace_id;
        std::string span_id;
        std::string parent_id;
        std::string flags = {};
    };

    /**
     * @brief Parse a Jaeger `uber-trace-id` header value.
     *
     * Format: {traceId}:{spanId}:{parentSpanId}:{flags}
     * All fields are hex strings.  traceId is 32 hex chars (128-bit);
     * spanId and parentSpanId are up to 16 hex chars (64-bit).
     *
     * @return true if all four fields were successfully parsed.
     */
    static bool parseUberTraceId(const std::string& value, UberTraceIds& out) {
        // Split on ':'
        std::vector<std::string> parts;
        parts.reserve(4);
        std::string::size_type start = 0;
        for (int i = 0; i < 3; ++i) {
            auto pos = value.find(':', start);
            if (pos == std::string::npos) {
              return false;
            }
            parts.push_back(value.substr(start, pos - start));
            start = pos + 1;
        }
        parts.push_back(value.substr(start));
        if (parts.size() != 4) {
          return false;
        }

        // Validate each field is a non-empty hex string.
        auto isHex = [](const std::string& s) {
            if (s.empty()) {
              return false;
            }
            return std::all_of(s.begin(), s.end(), [](unsigned char c) {
                return std::isxdigit(c) != 0;
            });
        };

        if (!isHex(parts[0]) || !isHex(parts[1]) || !isHex(parts[3])) {
          return false;
        }
        // parentSpanId may be "0" (root span) – still valid.
        if (parts[2] != "0" && !isHex(parts[2])) {
          return false;
        }

        out.trace_id  = parts[0];
        out.span_id   = parts[1];
        out.parent_id = parts[2];
        out.flags     = parts[3];
        return true;
    }
};

} // namespace concerns
} // namespace core
} // namespace themis
