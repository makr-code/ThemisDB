/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tracing.cpp                                        ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     542                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "utils/tracing.h"
#include "utils/logger.h"
#include "observability/metrics_collector.h"
#include "security/pii_redaction_policy.h"

#include <algorithm>
#include <array>
#include <map>
#include <regex>
#include <string>
#include <utility>

// Ensure correct WinSock include order on Windows before including Boost.Asio
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#endif

#include <boost/asio.hpp>

#ifdef THEMIS_ENABLE_TRACING
#include <opentelemetry/exporters/otlp/otlp_http_exporter_factory.h>
#include <opentelemetry/exporters/otlp/otlp_http_exporter_options.h>
#include <opentelemetry/sdk/trace/simple_processor_factory.h>
#include <opentelemetry/sdk/trace/tracer_provider_factory.h>
#include <opentelemetry/sdk/resource/resource.h>
#include <opentelemetry/trace/provider.h>
#include <opentelemetry/trace/span_context.h>

namespace otel_sdk = opentelemetry::sdk;
namespace otel_trace = opentelemetry::trace;
namespace otel_resource = opentelemetry::sdk::resource;
namespace otel_exporter = opentelemetry::exporter::otlp;
#endif

namespace themis {

#ifdef THEMIS_ENABLE_TRACING
otel::nostd::shared_ptr<otel::trace::Tracer> Tracer::tracer_;
#endif
bool Tracer::initialized_ = false;
std::atomic<int64_t> Tracer::total_spans_{0};
std::atomic<int64_t> Tracer::active_spans_{0};

bool Tracer::initialize([[maybe_unused]] const std::string& serviceName, 
                        [[maybe_unused]] const std::string& endpoint) {
#ifdef THEMIS_ENABLE_TRACING
    if (initialized_) {
        THEMIS_WARN("Tracer already initialized");
        return false;
    }
    
    try {
        // Probe collector reachability first to avoid noisy exporter errors
        auto parse_host_port = [](const std::string& url) -> std::pair<std::string, uint16_t> {
            std::regex re(R"((?:http|https)://([^/:]+)(?::(\d+))?)", std::regex::icase);
            std::smatch m;
            if (std::regex_search(url, m, re)) {
                std::string host = m[1].str();
                uint16_t port = 4318; // default OTLP HTTP
                if (m.size() > 2 && m[2].matched) {
                    port = static_cast<uint16_t>(std::stoi(m[2].str()));
                }
                return {host, port};
            }
            return {url, static_cast<uint16_t>(4318)};
        };

        auto [host, port] = parse_host_port(endpoint);
        try {
            namespace net = boost::asio;
            using tcp = net::ip::tcp;
            net::io_context io;
            tcp::resolver resolver(io);
            boost::system::error_code ec;
            auto results = resolver.resolve(host, std::to_string(port), ec);
            if (ec) {
                THEMIS_WARN("Tracing collector resolve failed ({}:{}): {}. Tracing disabled.", host, port, ec.message());
                initialized_ = true; // mark to avoid repeat warnings
                return false;
            }
            tcp::socket socket(io);
            socket.connect(*results.begin(), ec);
            if (ec) {
                THEMIS_WARN("Tracing collector unreachable ({}:{}): {}. Tracing disabled.", host, port, ec.message());
                initialized_ = true;
                return false;
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("Tracing probe failed: {}. Tracing disabled.", e.what());
            initialized_ = true;
            return false;
        }

        // Create OTLP HTTP exporter
        otel_exporter::OtlpHttpExporterOptions opts;
        opts.url = endpoint + "/v1/traces"; // OTLP HTTP traces endpoint
        
        auto exporter = otel_exporter::OtlpHttpExporterFactory::Create(opts);
        
        // Create simple span processor
        auto processor = otel_sdk::trace::SimpleSpanProcessorFactory::Create(std::move(exporter));
        
        // Create resource with service name
        auto resource_attributes = otel_resource::ResourceAttributes{
            {"service.name", serviceName},
            {"service.version", "0.1.0"}
        };
        auto resource = otel_resource::Resource::Create(resource_attributes);
        
        // Create tracer provider
        std::shared_ptr<otel_sdk::trace::TracerProvider> provider = 
            otel_sdk::trace::TracerProviderFactory::Create(std::move(processor), resource);
        
        // Set as global provider (convert to nostd::shared_ptr)
        otel::trace::Provider::SetTracerProvider(
            otel::nostd::shared_ptr<otel::trace::TracerProvider>(provider));
        
        // Get tracer instance
        tracer_ = provider->GetTracer(serviceName, "0.1.0");
        
        initialized_ = true;
        THEMIS_INFO("OpenTelemetry tracer initialized: service={}, endpoint={}", serviceName, endpoint);
        return true;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to initialize OpenTelemetry tracer: {}", e.what());
        return false;
    }
#else
    THEMIS_INFO("Tracing disabled (THEMIS_ENABLE_TRACING not defined)");
    initialized_ = true; // Mark as "initialized" to prevent repeated logs
    return true;
#endif
}

void Tracer::shutdown() {
#ifdef THEMIS_ENABLE_TRACING
    if (!initialized_) {
        return;
    }
    
    // Shutdown will flush remaining spans
    auto provider = otel::trace::Provider::GetTracerProvider();
    if (provider) {
        auto sdk_provider = static_cast<otel_sdk::trace::TracerProvider*>(provider.get());
        if (sdk_provider) {
            sdk_provider->Shutdown();
        }
    }
    
    initialized_ = false;
    // Note: nostd::shared_ptr doesn't have reset(), just assign nullptr
    tracer_ = nullptr;
    THEMIS_INFO("OpenTelemetry tracer shut down");
#endif
}

#ifdef THEMIS_ENABLE_TRACING
otel::nostd::shared_ptr<otel::trace::Tracer> Tracer::getTracer() {
    if (!initialized_ || tracer_ == nullptr) {
        THEMIS_WARN("Tracer not initialized, call Tracer::initialize() first");
        // Return empty shared_ptr (OTEL doesn't accept nullptr in return)
        return otel::nostd::shared_ptr<otel::trace::Tracer>();
    }
    return tracer_;
}
#endif

Tracer::Span Tracer::startSpan([[maybe_unused]] const std::string& name) {
#ifdef THEMIS_ENABLE_TRACING
    auto tracer = getTracer();
    if (!tracer) {
        return Span(); // Return invalid span
    }
    
    auto span = tracer->StartSpan(name);
    total_spans_++;
    active_spans_++;
    return Span(span);
#else
    return Span(); // No-op span
#endif
}

Tracer::Span Tracer::startChildSpan([[maybe_unused]] const std::string& name, 
                                    [[maybe_unused]] const Span& parent) {
#ifdef THEMIS_ENABLE_TRACING
    auto tracer = getTracer();
    if (!tracer || !parent.valid_) {
        return Span();
    }
    
    // Start span with parent context
    otel::trace::StartSpanOptions options;
    options.parent = parent.context_;
    
    auto span = tracer->StartSpan(name, options);
    total_spans_++;
    active_spans_++;
    return Span(span);
#else
    return Span();
#endif
}

// ============================================================================
// W3C TraceContext propagation helpers
// ============================================================================
namespace {

/// Case-insensitive lookup in a string map.
std::string headerValue(const std::map<std::string, std::string>& headers,
                        const std::string& name) {
    // Exact match first
    auto it = headers.find(name);
    if (it != headers.end()) return it->second;
    // Case-insensitive fallback
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

#ifdef THEMIS_ENABLE_TRACING
/// Parse a W3C `traceparent` header value and fill a SpanContext.
/// Returns true if parsing succeeded.
/// Format: version-traceId(32hex)-parentId(16hex)-flags(2hex)
bool parseTraceparent(const std::string& value,
                      otel::trace::TraceId& trace_id_out,
                      otel::trace::SpanId& parent_id_out,
                      otel::trace::TraceFlags& flags_out) {
    // W3C spec v00 requires exactly 55 characters: "00-<32hex>-<16hex>-<2hex>"
    if (value.size() != 55) return false;
    if (value[2] != '-' || value[35] != '-' || value[52] != '-') return false;

    auto hexByte = [](char hi, char lo, uint8_t& out) -> bool {
        auto fromHex = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return -1;
        };
        int h = fromHex(hi), l = fromHex(lo);
        if (h < 0 || l < 0) return false;
        out = static_cast<uint8_t>((h << 4) | l);
        return true;
    };

    // version (must be "00")
    uint8_t ver{};
    if (!hexByte(value[0], value[1], ver) || ver != 0) return false;

    // traceId (16 bytes = 32 hex chars) starting at offset 3
    std::array<uint8_t, 16> tid{};
    for (int i = 0; i < 16; ++i) {
        if (!hexByte(value[3 + i * 2], value[3 + i * 2 + 1], tid[i])) return false;
    }

    // W3C spec: trace-id MUST NOT be all zeros
    bool all_zeros = true;
    for (auto b : tid) { if (b != 0) { all_zeros = false; break; } }
    if (all_zeros) return false;

    // parentId (8 bytes = 16 hex chars) starting at offset 36
    std::array<uint8_t, 8> pid{};
    for (int i = 0; i < 8; ++i) {
        if (!hexByte(value[36 + i * 2], value[36 + i * 2 + 1], pid[i])) return false;
    }

    // W3C spec: parent-id MUST NOT be all zeros
    all_zeros = true;
    for (auto b : pid) { if (b != 0) { all_zeros = false; break; } }
    if (all_zeros) return false;

    // flags (1 byte = 2 hex chars) starting at offset 53
    uint8_t flg{};
    if (!hexByte(value[53], value[54], flg)) return false;

    trace_id_out  = otel::trace::TraceId(tid);
    parent_id_out = otel::trace::SpanId(pid);
    flags_out     = otel::trace::TraceFlags(flg);
    return true;
}
#endif // THEMIS_ENABLE_TRACING

} // anonymous namespace

Tracer::Span Tracer::startSpanFromHeaders(
        [[maybe_unused]] const std::string& name,
        const std::map<std::string, std::string>& headers) {

    std::string traceparent = headerValue(headers, "traceparent");
    std::string tracestate  = headerValue(headers, "tracestate");

#ifdef THEMIS_ENABLE_TRACING
    auto tracer = getTracer();
    if (tracer && !traceparent.empty()) {
        otel::trace::TraceId  trace_id;
        otel::trace::SpanId   parent_id;
        otel::trace::TraceFlags flags;

        if (parseTraceparent(traceparent, trace_id, parent_id, flags)) {
            // Build a remote SpanContext representing the upstream span
            otel::trace::SpanContext remote_ctx(
                trace_id,
                parent_id,
                flags,
                /*is_remote=*/true);

            otel::trace::StartSpanOptions opts;
            opts.parent = remote_ctx;

            auto span = tracer->StartSpan(name, opts);
            if (!tracestate.empty()) {
                span->SetAttribute("w3c.tracestate", tracestate);
            }
            total_spans_++;
            active_spans_++;
            THEMIS_DEBUG("W3C TraceContext propagated: traceparent={}", traceparent);
            return Span(span);
        } else {
            THEMIS_WARN("Invalid traceparent header ignored: '{}'", traceparent);
        }
    }
    // Fall through to a regular root span when no valid context is present
    return startSpan(name);
#else
    // Without OTel, start a normal span and record the upstream IDs as attributes
    // so they appear in structured logs for manual correlation.
    auto span = startSpan(name);
    if (!traceparent.empty()) {
        span.setAttribute("w3c.traceparent", traceparent);
    }
    if (!tracestate.empty()) {
        span.setAttribute("w3c.tracestate", tracestate);
    }
    return span;
#endif
}

int64_t Tracer::getTotalSpans() {
    return total_spans_.load();
}

int64_t Tracer::getActiveSpans() {
    return active_spans_.load();
}

// Span implementation
#ifdef THEMIS_ENABLE_TRACING
Tracer::Span::Span(otel::nostd::shared_ptr<otel::trace::Span> span)
    : span_(span), valid_(span != nullptr), ended_(false), start_time_(std::chrono::steady_clock::now()) {
    if (span_) {
        context_ = otel::context::RuntimeContext::GetCurrent().SetValue(
            otel::trace::kSpanKey, span_);
    }
}
#endif

Tracer::Span::~Span() {
    if (valid_ && !ended_) {
        end();
    }
    if (valid_) {
        active_spans_--;
    }
}

Tracer::Span::Span(Span&& other) noexcept
    : valid_(other.valid_), ended_(other.ended_) {
#ifdef THEMIS_ENABLE_TRACING
    span_ = std::move(other.span_);
    context_ = std::move(other.context_);
#endif
    other.valid_ = false;
}

Tracer::Span& Tracer::Span::operator=(Span&& other) noexcept {
    if (this != &other) {
        if (valid_ && !ended_) {
            end();
        }
        
        valid_ = other.valid_;
        ended_ = other.ended_;
#ifdef THEMIS_ENABLE_TRACING
        span_ = std::move(other.span_);
        context_ = std::move(other.context_);
#endif
        other.valid_ = false;
    }
    return *this;
}

void Tracer::Span::setAttribute([[maybe_unused]] const std::string& key, 
                                 [[maybe_unused]] const std::string& value) {
#ifdef THEMIS_ENABLE_TRACING
    if (span_) {
        // Redact PII from string attribute values before recording in the trace.
        // redactAttributeValue() applies both key-based (field-name hint) and
        // inline PII redaction without allocating a temporary map.
        std::string safe_value = themis::security::PIIRedactionPolicy::get()
                                     .redactAttributeValue(key, value);
        span_->SetAttribute(key, safe_value);
    }
#endif
}

void Tracer::Span::setAttribute([[maybe_unused]] const std::string& key, 
                                 [[maybe_unused]] int64_t value) {
#ifdef THEMIS_ENABLE_TRACING
    if (span_) {
        span_->SetAttribute(key, value);
    }
#endif
}

void Tracer::Span::setAttribute([[maybe_unused]] const std::string& key, 
                                 [[maybe_unused]] double value) {
#ifdef THEMIS_ENABLE_TRACING
    if (span_) {
        span_->SetAttribute(key, value);
    }
#endif
}

void Tracer::Span::setAttribute([[maybe_unused]] const std::string& key, 
                                 [[maybe_unused]] bool value) {
#ifdef THEMIS_ENABLE_TRACING
    if (span_) {
        span_->SetAttribute(key, value);
    }
#endif
}

void Tracer::Span::recordError([[maybe_unused]] const std::string& errorMessage) {
#ifdef THEMIS_ENABLE_TRACING
    if (span_) {
        // Redact PII from error messages before recording in the trace.
        std::string safe_msg = themis::security::PIIRedactionPolicy::get()
                                   .redactForLog(errorMessage);
        span_->AddEvent("exception", {{"exception.message", safe_msg}});
        span_->SetStatus(otel::trace::StatusCode::kError, safe_msg);
    }
#endif
}

void Tracer::Span::setStatus([[maybe_unused]] bool ok, 
                             [[maybe_unused]] const std::string& description) {
#ifdef THEMIS_ENABLE_TRACING
    if (span_) {
        auto status_code = ok ? otel::trace::StatusCode::kOk : otel::trace::StatusCode::kError;
        span_->SetStatus(status_code, description);
    }
#endif
}

void Tracer::Span::end() {
#ifdef THEMIS_ENABLE_TRACING
    if (span_ && !ended_) {
        span_->End();
        ended_ = true;
    }
#endif
    ended_ = true;
}

double Tracer::Span::durationMs() const {
#ifdef THEMIS_ENABLE_TRACING
    if (!valid_) return 0.0;
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - start_time_);
    return duration.count() / 1000.0;
#else
    return 0.0;
#endif
}

// ScopedSpan implementation
ScopedSpan::~ScopedSpan() {
    // Default implementation, can be extended if needed
}

// TracedSpan implementation
TracedSpan::TracedSpan(const std::string& name) 
    : span_(Tracer::startSpan(name))
    , name_(name)
    , start_time_(std::chrono::steady_clock::now()) {
}

TracedSpan::~TracedSpan() {
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);
    double duration_ms = duration.count() / 1000.0;
    
    // Record span duration to Prometheus metrics
    if (span_.isValid()) {
        observability::MetricsCollector::getInstance().recordSpanDuration(name_, duration_ms);
        observability::MetricsCollector::getInstance().recordActiveSpans(Tracer::getActiveSpans());
        observability::MetricsCollector::getInstance().recordTotalSpans(Tracer::getTotalSpans());
    }
}

void TracedSpan::setAttribute(const std::string& key, const std::string& value) {
    span_.setAttribute(key, value);
}

void TracedSpan::setAttribute(const std::string& key, int64_t value) {
    span_.setAttribute(key, value);
}

void TracedSpan::setAttribute(const std::string& key, double value) {
    span_.setAttribute(key, value);
}

void TracedSpan::setAttribute(const std::string& key, bool value) {
    span_.setAttribute(key, value);
}

void TracedSpan::recordError(const std::string& errorMessage) {
    span_.recordError(errorMessage);
}

void TracedSpan::setStatus(bool ok, const std::string& description) {
    span_.setStatus(ok, description);
}

Tracer::Span& TracedSpan::span() {
    return span_;
}

} // namespace themis
