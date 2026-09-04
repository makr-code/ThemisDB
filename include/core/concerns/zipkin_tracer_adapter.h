/**
 * @file zipkin_tracer_adapter.h
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
 * @brief Zipkin tracing backend adapter implementing ITracer.
 *
 * Wraps the existing OpenTelemetry-based Tracer to target a Zipkin backend.
 * The underlying OTel SDK can export to Zipkin via its Zipkin exporter when
 * THEMIS_ENABLE_TRACING is defined; this adapter adds Zipkin-specific defaults
 * and handles both the B3 multi-header and B3 single-header propagation
 * formats used by Zipkin instrumented services.
 *
 * Default collector endpoint: http://localhost:9411/api/v2/spans
 *
 * Header propagation:
 *   - startSpanFromHeaders() recognises:
 *       1. W3C `traceparent` header (highest priority).
 *       2. B3 single header (`b3: {traceId}-{spanId}-{sampling}-{parentSpanId}`).
 *       3. B3 multi-headers (`X-B3-TraceId`, `X-B3-SpanId`, `X-B3-ParentSpanId`,
 *          `X-B3-Sampled`).
 *     The decoded IDs are attached as span attributes for UI correlation when
 *     full OTLP context propagation is not available.
 *   - injectContext() writes the W3C `traceparent` header plus both the B3
 *     single and multi headers so downstream services using any convention
 *     can continue the trace.
 *
 * A circuit breaker guards every span-export call so that a failing or
 * unreachable Zipkin instance does not block the critical path. When the
 * breaker is open, span creation degrades to no-op spans.
 */
class ZipkinTracerAdapter : public ITracer {
public:
    /// Default Zipkin HTTP collector endpoint.
    static constexpr const char* kDefaultEndpoint = "http://localhost:9411/api/v2/spans";

    /**
     * @brief Configuration for the circuit breaker that guards Zipkin export.
     */
    struct CircuitBreakerConfig {
        size_t failure_threshold = 5;
        std::chrono::seconds timeout = std::chrono::seconds(30);
        size_t success_threshold = 2;
    };

    explicit ZipkinTracerAdapter(const CircuitBreakerConfig& cb_config) {
        sharding::CircuitBreaker::Config cfg;
        cfg.failure_threshold = cb_config.failure_threshold;
        cfg.timeout           = cb_config.timeout;
        cfg.success_threshold = cb_config.success_threshold;
        circuit_breaker_ = std::make_unique<sharding::CircuitBreaker>(cfg);
    }

    /// Construct with default circuit-breaker settings.
    ZipkinTracerAdapter() : ZipkinTracerAdapter(CircuitBreakerConfig{}) {}

    // -------------------------------------------------------------------------
    // ISpan adapter – delegates to themis::Tracer::Span
    // -------------------------------------------------------------------------

    /** @brief ISpan adapter – delegates to themis::Tracer::Span. */
    class ZipkinSpanAdapter : public ISpan {
    public:
        explicit ZipkinSpanAdapter(themis::Tracer::Span span)
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

        ~ZipkinSpanAdapter() override { span_.end(); }

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
            return std::make_unique<ZipkinSpanAdapter>(themis::Tracer::Span{});
        }
        auto span_ptr = std::make_unique<ZipkinSpanAdapter>(themis::Tracer::startSpan(name));
        span_ptr->isValid() ? circuit_breaker_->recordSuccess()
                            : circuit_breaker_->recordFailure();
        return span_ptr;
    }

    std::unique_ptr<ISpan> startChildSpan(const std::string& name,
                                           const ISpan& parent) override {
        if (!circuit_breaker_->allowRequest()) {
            return std::make_unique<ZipkinSpanAdapter>(themis::Tracer::Span{});
        }
        std::unique_ptr<ZipkinSpanAdapter> span_ptr;
        const auto* zipkin_parent = dynamic_cast<const ZipkinSpanAdapter*>(&parent);
        if (zipkin_parent) {
            span_ptr = std::make_unique<ZipkinSpanAdapter>(
                themis::Tracer::startChildSpan(name, zipkin_parent->getSpan()));
        } else {
            span_ptr = std::make_unique<ZipkinSpanAdapter>(
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
    *  1. W3C `traceparent` header (highest priority, handled by base Tracer).
    *  2. B3 single header (`b3`).
    *  3. B3 multi-headers (`X-B3-TraceId` / `X-B3-SpanId` / …).
    *
    * When B3 headers are detected the decoded IDs are attached as span
    * attributes for Zipkin UI correlation. W3C Baggage is also extracted.
     */
    std::unique_ptr<ISpan> startSpanFromHeaders(
            const std::string& name,
            const std::map<std::string, std::string>& carrier_headers) override {
        if (!circuit_breaker_->allowRequest()) {
            return std::make_unique<ZipkinSpanAdapter>(themis::Tracer::Span{});
        }

        // W3C traceparent takes precedence.
        std::string traceparent = headerValueCI(carrier_headers, "traceparent");
        if (!traceparent.empty()) {
            auto span_ptr = std::make_unique<ZipkinSpanAdapter>(
                themis::Tracer::startSpanFromHeaders(name, carrier_headers));
            span_ptr->isValid() ? circuit_breaker_->recordSuccess()
                                : circuit_breaker_->recordFailure();
            return span_ptr;
        }

        // Try B3 single header first.
        std::string b3_single = headerValueCI(carrier_headers, "b3");
        B3Ids ids;
        bool has_b3 = false;

        if (!b3_single.empty()) {
            has_b3 = parseB3Single(b3_single, ids);
        }

        // Fall back to B3 multi-headers.
        if (!has_b3) {
            ids.trace_id  = headerValueCI(carrier_headers, "X-B3-TraceId");
            ids.span_id   = headerValueCI(carrier_headers, "X-B3-SpanId");
            ids.parent_id = headerValueCI(carrier_headers, "X-B3-ParentSpanId");
            ids.sampled   = headerValueCI(carrier_headers, "X-B3-Sampled");
            has_b3 = !ids.trace_id.empty() && !ids.span_id.empty();
        }

        auto span_ptr = std::make_unique<ZipkinSpanAdapter>(
            themis::Tracer::startSpan(name));

        if (has_b3) {
            span_ptr->setAttribute("zipkin.trace_id",  ids.trace_id);
            span_ptr->setAttribute("zipkin.span_id",   ids.span_id);
            if (!ids.parent_id.empty()) {
                span_ptr->setAttribute("zipkin.parent_id", ids.parent_id);
            }
            if (!ids.sampled.empty()) {
                span_ptr->setAttribute("zipkin.sampled", ids.sampled);
            }
        }

        // Extract W3C Baggage.
        themis::Baggage::extract(carrier_headers);

        span_ptr->isValid() ? circuit_breaker_->recordSuccess()
                            : circuit_breaker_->recordFailure();
        return span_ptr;
    }

    /**
     * @brief Inject trace context into outgoing headers.
     *
    * Writes the W3C `traceparent` header, the B3 single header (`b3`), and
    * B3 multi-headers (`X-B3-TraceId`, `X-B3-SpanId`, `X-B3-Sampled`) so
    * downstream services using any of the three conventions can continue the
    * trace. Also injects W3C Baggage when any items are present.
     */
    void injectContext(std::map<std::string, std::string>& carrier_headers) override {
        std::string trace_id = themis::Tracer::getCurrentTraceId();
        std::string span_id  = themis::Tracer::getCurrentSpanId();

        if (!trace_id.empty() && !span_id.empty()) {
            // W3C traceparent
            carrier_headers["traceparent"] = "00-" + trace_id + "-" + span_id + "-01";

            // B3 single header: {traceId}-{spanId}-{sampling}
            carrier_headers["b3"] = trace_id + "-" + span_id + "-1";

            // B3 multi-headers
            carrier_headers["X-B3-TraceId"] = trace_id;
            carrier_headers["X-B3-SpanId"]  = span_id;
            carrier_headers["X-B3-Sampled"] = "1";
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
            return ProbeResult::unhealthy("Zipkin tracer not initialized");
        }
        if (circuit_breaker_->getState() == sharding::CircuitBreaker::State::OPEN) {
            return ProbeResult::unhealthy("Zipkin exporter circuit-breaker OPEN");
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

    struct B3Ids {
        std::string trace_id;
        std::string span_id;
        std::string parent_id;
        std::string sampled;
    };

    /**
     * @brief Parse a B3 single-header value.
     *
     * Formats (per OpenZipkin B3 spec):
     *   Deny:   `b3: 0`
     *   Accept: `b3: {traceId}-{spanId}[-{sampling}[-{parentSpanId}]]`
     *
     * @return true if at least traceId and spanId were successfully parsed.
     */
    static bool parseB3Single(const std::string& value, B3Ids& out) {
        // Reject sampling-deny shorthand.
        if (value == "0" || value == "1") {
            out.sampled = value;
            return value == "1"; // "1" alone is a valid sampling flag but no IDs
        }

        std::vector<std::string> parts;
        parts.reserve(4);
        std::string::size_type start = 0;
        for (int i = 0; i < 3; ++i) {
            auto pos = value.find('-', start);
            if (pos == std::string::npos) {
                parts.push_back(value.substr(start));
                start = std::string::npos;
                break;
            }
            parts.push_back(value.substr(start, pos - start));
            start = pos + 1;
        }
        if (start != std::string::npos) {
            parts.push_back(value.substr(start));
        }

        if (parts.size() < 2) {
          return false;
        }

        auto isHex = [](const std::string& s) {
            if (s.empty()) {
              return false;
            }
            return std::all_of(s.begin(), s.end(), [](unsigned char c) {
                return std::isxdigit(c) != 0;
            });
        };

        if (!isHex(parts[0]) || !isHex(parts[1])) {
          return false;
        }

        out.trace_id = parts[0];
        out.span_id  = parts[1];
        if (parts.size() >= 3) {
          out.sampled   = parts[2];
        }
        if (parts.size() >= 4) {
          out.parent_id = parts[3];
        }
        return true;
    }
};

} // namespace concerns
} // namespace core
} // namespace themis
