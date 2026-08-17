/**
 * @file tracing.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=2, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "utils/tracing.h"
#include "utils/logger.h"
#include "utils/error_contracts.h"
#include "observability/metrics_collector.h"
#include "security/pii_redaction_policy.h"

#include <algorithm>
#include <array>
#include <map>
#include <random>
#include <regex>
#include <sstream>
#include <string>
#include <utility>

// Ensure correct WinSock include order on Windows before including Boost.Asio
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#else
#include <sys/socket.h>
#include <sys/time.h>
#endif

#if defined(THEMIS_ENABLE_TRACING) && defined(THEMIS_HAS_OPENTELEMETRY)
#include <boost/asio.hpp>
#endif

#if defined(THEMIS_ENABLE_TRACING) && defined(THEMIS_HAS_OPENTELEMETRY)
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

// ─────────────────────────────────────────────────────────────────────────────
// SamplingStrategy
// ─────────────────────────────────────────────────────────────────────────────

SamplingStrategy SamplingStrategy::adaptive() {
    return adaptive(AdaptiveConfig{});
}

SamplingStrategy SamplingStrategy::adaptive(AdaptiveConfig config) {
    SamplingStrategy s(Type::ADAPTIVE, config.min_rate);
    s.adaptive_config_ = config;
    s.adaptive_state_  = std::make_shared<AdaptiveState>();
    return s;
}

double SamplingStrategy::getEffectiveRate() const {
    if (type_ != Type::ADAPTIVE || !adaptive_state_) {
        return probability_;
    }
    std::lock_guard<std::mutex> lk(adaptive_state_->mu);
    return adaptive_state_->effective_rate;
}

bool SamplingStrategy::shouldSample(bool parent_sampled) const {
    switch (type_) {
        case Type::ALWAYS_ON:
            return true;
        case Type::ALWAYS_OFF:
            return false;
        case Type::PROBABILITY: {
            thread_local std::mt19937_64 rng{std::random_device{}()};
            thread_local std::uniform_real_distribution<double> dist(0.0, 1.0);
            return dist(rng) < probability_;
        }
        case Type::PARENT_BASED:
            if (parent_sampled) return true;
            {
                thread_local std::mt19937_64 rng{std::random_device{}()};
                thread_local std::uniform_real_distribution<double> dist(0.0, 1.0);
                return dist(rng) < probability_;
            }
        case Type::ADAPTIVE: {
            if (!adaptive_state_) return true; // safety fallback

            auto now = std::chrono::steady_clock::now();
            std::lock_guard<std::mutex> lk(adaptive_state_->mu);

            // Advance window when the measurement period has elapsed
            auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - adaptive_state_->window_start).count();

            if (elapsed_ms >= adaptive_config_.window.count()) {
                double window_seconds = elapsed_ms / 1000.0;
                if (window_seconds > 0.0 && adaptive_state_->window_count > 0) {
                    double observed_rate =
                        static_cast<double>(adaptive_state_->window_count) / window_seconds;
                    if (observed_rate > adaptive_config_.max_spans_per_second) {
                        adaptive_state_->effective_rate = std::max(
                            adaptive_config_.min_rate,
                            adaptive_config_.max_spans_per_second / observed_rate);
                    } else {
                        adaptive_state_->effective_rate = 1.0;
                    }
                }
                adaptive_state_->window_count = 0;
                adaptive_state_->window_start = now;
            }

            adaptive_state_->window_count++;

            double rate = adaptive_state_->effective_rate;
            if (rate >= 1.0) return true;

            thread_local std::mt19937_64 rng{std::random_device{}()};
            thread_local std::uniform_real_distribution<double> dist(0.0, 1.0);
            return dist(rng) < rate;
        }
        default:
            return true;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Baggage
// ─────────────────────────────────────────────────────────────────────────────

thread_local Baggage::BaggageMap Baggage::thread_baggage_;

void Baggage::set(const std::string& key, const std::string& value) {
    thread_baggage_[key] = value;
}

std::string Baggage::get(const std::string& key) {
    auto it = thread_baggage_.find(key);
    return it != thread_baggage_.end() ? it->second : std::string{};
}

void Baggage::remove(const std::string& key) {
    thread_baggage_.erase(key);
}

void Baggage::clear() {
    thread_baggage_.clear();
}

Baggage::BaggageMap Baggage::getAll() {
    return thread_baggage_;
}

std::string Baggage::serialize() {
    std::string out;
    for (const auto& [k, v] : thread_baggage_) {
        if (!out.empty()) out += ',';
        out += k + '=' + v;
    }
    return out;
}

void Baggage::inject(std::map<std::string, std::string>& headers) {
    auto serialized = serialize();
    if (!serialized.empty()) {
        headers["baggage"] = std::move(serialized);
    }
}

void Baggage::extract(const std::map<std::string, std::string>& headers) {
    std::string value;
    for (const auto& [k, v] : headers) {
        std::string lower_k = k;
        std::transform(lower_k.begin(), lower_k.end(), lower_k.begin(),
                       [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
        if (lower_k == "baggage") { value = v; break; }
    }
    if (value.empty()) return;

    std::istringstream ss(value);
    std::string token;
    while (std::getline(ss, token, ',')) {
        auto eq = token.find('=');
        if (eq == std::string::npos) continue;
        auto key = token.substr(0, eq);
        auto val = token.substr(eq + 1);
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        val.erase(0, val.find_first_not_of(" \t"));
        val.erase(val.find_last_not_of(" \t") + 1);
        if (!key.empty()) thread_baggage_[key] = val;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Tracer – static members
// ─────────────────────────────────────────────────────────────────────────────

#if defined(THEMIS_ENABLE_TRACING) && defined(THEMIS_HAS_OPENTELEMETRY)
otel::nostd::shared_ptr<otel::trace::Tracer> Tracer::tracer_;
#endif
bool Tracer::initialized_ = false;
std::atomic<int64_t> Tracer::total_spans_{0};
std::atomic<int64_t> Tracer::active_spans_{0};
SamplingStrategy Tracer::sampling_strategy_{SamplingStrategy::alwaysOn()};
std::mutex Tracer::sampling_mu_;

bool Tracer::initialize([[maybe_unused]] const std::string& serviceName, 
                        [[maybe_unused]] const std::string& endpoint) {
#if defined(THEMIS_ENABLE_TRACING) && defined(THEMIS_HAS_OPENTELEMETRY)
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
                uint16_t port = 4318;
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
                // Fail-open: no-op span, latency bounded to probe timeout (3s).
                auto ctx = themis::utils::makeErrorContext(
                    themis::utils::ErrorCode::TRACE_EXPORT_FAILED,
                    "Tracing collector DNS resolution failed – using no-op spans (fail-open); host=" +
                        host + " port=" + std::to_string(port) + " error=" + ec.message(),
                    "Tracer::initialize",
                    themis::utils::ErrorSeverity::Warning,
                    true);
                themis::utils::logErrorWithContext(ctx);
                THEMIS_WARN("Tracing collector resolve failed ({}:{}): {}. Tracing disabled.", host, port, ec.message());
                initialized_ = true;
                return false;
            }
            tcp::socket socket(io);
            // Enforce a 3-second connect timeout using async operations.
            // SO_SNDTIMEO does not reliably bound a blocking connect(); we
            // use Boost.Asio's async_connect + a steady_timer instead so the
            // probe always completes within the deadline regardless of OS.
            boost::system::error_code connect_ec{
                boost::asio::error::operation_aborted};
            net::steady_timer timeout(io);
            timeout.expires_after(std::chrono::seconds(3));
            timeout.async_wait([&socket](const boost::system::error_code& te) {
                if (!te) {
                    boost::system::error_code ignored;
                    socket.cancel(ignored);
                }
            });
            net::async_connect(socket, results,
                [&](const boost::system::error_code& ce,
                    const tcp::endpoint&) {
                    connect_ec = ce;
                    timeout.cancel();
                });
            io.run();
            ec = connect_ec;
            if (ec) {
                // Fail-open: degrade to no-op spans, latency bounded.
                auto ctx2 = themis::utils::makeErrorContext(
                    themis::utils::ErrorCode::TRACE_EXPORT_FAILED,
                    "Tracing collector TCP connect failed – using no-op spans (fail-open); host=" +
                        host + " port=" + std::to_string(port) + " error=" + ec.message(),
                    "Tracer::initialize",
                    themis::utils::ErrorSeverity::Warning,
                    true);
                themis::utils::logErrorWithContext(ctx2);
                THEMIS_WARN("Tracing collector unreachable ({}:{}): {}. Tracing disabled.", host, port, ec.message());
                initialized_ = true;
                return false;
            }
        } catch (const std::exception& e) {
            auto ctx3 = themis::utils::makeErrorContext(
                themis::utils::ErrorCode::TRACE_EXPORT_FAILED,
                "Tracing collector probe threw exception – using no-op spans (fail-open); error=" +
                    std::string(e.what()),
                "Tracer::initialize",
                themis::utils::ErrorSeverity::Warning,
                true);
            themis::utils::logErrorWithContext(ctx3);
            THEMIS_WARN("Tracing probe failed: {}. Tracing disabled.", e.what());
            initialized_ = true;
            return false;
        }

        otel_exporter::OtlpHttpExporterOptions opts;
        opts.url = endpoint + "/v1/traces";
        
        auto exporter = otel_exporter::OtlpHttpExporterFactory::Create(opts);
        auto processor = otel_sdk::trace::SimpleSpanProcessorFactory::Create(std::move(exporter));
        
        auto resource_attributes = otel_resource::ResourceAttributes{
            {"service.name", serviceName},
            {"service.version", "0.1.0"}
        };
        auto resource = otel_resource::Resource::Create(resource_attributes);
        
        std::shared_ptr<otel_sdk::trace::TracerProvider> provider = 
            otel_sdk::trace::TracerProviderFactory::Create(std::move(processor), resource);
        
        otel::trace::Provider::SetTracerProvider(
            otel::nostd::shared_ptr<otel::trace::TracerProvider>(provider));
        
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
    initialized_ = true;
    return true;
#endif
}

void Tracer::shutdown() {
#if defined(THEMIS_ENABLE_TRACING) && defined(THEMIS_HAS_OPENTELEMETRY)
    if (!initialized_) {
        return;
    }
    
    auto provider = otel::trace::Provider::GetTracerProvider();
    if (provider) {
        auto sdk_provider = static_cast<otel_sdk::trace::TracerProvider*>(provider.get());
        if (sdk_provider) {
            sdk_provider->Shutdown();
        }
    }
    
    initialized_ = false;
    tracer_ = nullptr;
    THEMIS_INFO("OpenTelemetry tracer shut down");
#endif
}

bool Tracer::flush([[maybe_unused]] std::chrono::microseconds timeout) noexcept {
#if defined(THEMIS_ENABLE_TRACING) && defined(THEMIS_HAS_OPENTELEMETRY)
    auto provider = otel::trace::Provider::GetTracerProvider();
    if (!provider) {
        return false;
    }
    auto sdk_provider = static_cast<otel_sdk::trace::TracerProvider*>(provider.get());
    if (!sdk_provider) {
        return false;
    }
    return sdk_provider->ForceFlush(timeout);
#else
    return true;
#endif
}

#if defined(THEMIS_ENABLE_TRACING) && defined(THEMIS_HAS_OPENTELEMETRY)
otel::nostd::shared_ptr<otel::trace::Tracer> Tracer::getTracer() {
    if (!initialized_ || tracer_ == nullptr) {
        THEMIS_WARN("Tracer not initialized, call Tracer::initialize() first");
        return otel::nostd::shared_ptr<otel::trace::Tracer>();
    }
    return tracer_;
}
#endif

void Tracer::setSamplingStrategy(const SamplingStrategy& strategy) {
    std::lock_guard<std::mutex> lk(sampling_mu_);
    sampling_strategy_ = strategy;
}

SamplingStrategy Tracer::getSamplingStrategy() {
    std::lock_guard<std::mutex> lk(sampling_mu_);
    return sampling_strategy_;
}

std::string Tracer::getCurrentTraceId() {
#if defined(THEMIS_ENABLE_TRACING) && defined(THEMIS_HAS_OPENTELEMETRY)
    auto ctx = otel::context::RuntimeContext::GetCurrent();
    auto val = ctx.GetValue(otel::trace::kSpanKey);
    if (otel::nostd::holds_alternative<
            otel::nostd::shared_ptr<otel::trace::Span>>(val)) {
        auto sp = otel::nostd::get<
            otel::nostd::shared_ptr<otel::trace::Span>>(val);
        if (sp && sp->GetContext().IsValid()) {
            auto tid = sp->GetContext().trace_id();
            char buf[33] = {};
            tid.ToLowerBase16({buf, 32});
            return std::string(buf, 32);
        }
    }
#endif
    return {};
}

std::string Tracer::getCurrentSpanId() {
#if defined(THEMIS_ENABLE_TRACING) && defined(THEMIS_HAS_OPENTELEMETRY)
    auto ctx = otel::context::RuntimeContext::GetCurrent();
    auto val = ctx.GetValue(otel::trace::kSpanKey);
    if (otel::nostd::holds_alternative<
            otel::nostd::shared_ptr<otel::trace::Span>>(val)) {
        auto sp = otel::nostd::get<
            otel::nostd::shared_ptr<otel::trace::Span>>(val);
        if (sp && sp->GetContext().IsValid()) {
            auto sid = sp->GetContext().span_id();
            char buf[17] = {};
            sid.ToLowerBase16({buf, 16});
            return std::string(buf, 16);
        }
    }
#endif
    return {};
}

Tracer::Span Tracer::startSpan([[maybe_unused]] const std::string& name) {
#if defined(THEMIS_ENABLE_TRACING) && defined(THEMIS_HAS_OPENTELEMETRY)
    {
        std::lock_guard<std::mutex> lk(sampling_mu_);
        if (!sampling_strategy_.shouldSample()) {
            return Span();
        }
    }

    auto tracer = getTracer();
    if (!tracer) {
        return Span();
    }
    
    auto span = tracer->StartSpan(name);
    total_spans_++;
    active_spans_++;
    return Span(span);
#else
    return Span();
#endif
}

Tracer::Span Tracer::startChildSpan([[maybe_unused]] const std::string& name, 
                                    [[maybe_unused]] const Span& parent) {
#if defined(THEMIS_ENABLE_TRACING) && defined(THEMIS_HAS_OPENTELEMETRY)
    auto tracer = getTracer();
    if (!tracer || !parent.valid_) {
        return Span();
    }

    {
        std::lock_guard<std::mutex> lk(sampling_mu_);
        if (!sampling_strategy_.shouldSample(parent.valid_)) {
            return Span();
        }
    }
    
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

std::string headerValue(const std::map<std::string, std::string>& headers,
                        const std::string& name) {
    auto it = headers.find(name);
    if (it != headers.end()) return it->second;
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

#if defined(THEMIS_ENABLE_TRACING) && defined(THEMIS_HAS_OPENTELEMETRY)
bool parseTraceparent(const std::string& value,
                      otel::trace::TraceId& trace_id_out,
                      otel::trace::SpanId& parent_id_out,
                      otel::trace::TraceFlags& flags_out) {
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

    uint8_t ver{};
    if (!hexByte(value[0], value[1], ver) || ver != 0) return false;

    std::array<uint8_t, 16> tid{};
    for (int i = 0; i < 16; ++i) {
        if (!hexByte(value[3 + i * 2], value[3 + i * 2 + 1], tid[i])) return false;
    }

    bool all_zeros = true;
    for (auto b : tid) { if (b != 0) { all_zeros = false; break; } }
    if (all_zeros) return false;

    std::array<uint8_t, 8> pid{};
    for (int i = 0; i < 8; ++i) {
        if (!hexByte(value[36 + i * 2], value[36 + i * 2 + 1], pid[i])) return false;
    }

    all_zeros = true;
    for (auto b : pid) { if (b != 0) { all_zeros = false; break; } }
    if (all_zeros) return false;

    uint8_t flg{};
    if (!hexByte(value[53], value[54], flg)) return false;

    trace_id_out = otel::trace::TraceId(
        otel::nostd::span<const uint8_t, otel::trace::TraceId::kSize>(tid.data(), tid.size()));
    parent_id_out = otel::trace::SpanId(
        otel::nostd::span<const uint8_t, otel::trace::SpanId::kSize>(pid.data(), pid.size()));
    flags_out     = otel::trace::TraceFlags(flg);
    return true;
}
#endif

} // anonymous namespace

Tracer::Span Tracer::startSpanFromHeaders(
        [[maybe_unused]] const std::string& name,
        const std::map<std::string, std::string>& headers) {

    std::string traceparent = headerValue(headers, "traceparent");
    std::string tracestate  = headerValue(headers, "tracestate");

    // Extract baggage from incoming headers
    Baggage::extract(headers);

#if defined(THEMIS_ENABLE_TRACING) && defined(THEMIS_HAS_OPENTELEMETRY)
    auto tracer = getTracer();
    if (tracer && !traceparent.empty()) {
        otel::trace::TraceId  trace_id;
        otel::trace::SpanId   parent_id;
        otel::trace::TraceFlags flags;

        if (parseTraceparent(traceparent, trace_id, parent_id, flags)) {
            otel::trace::SpanContext remote_ctx(trace_id, parent_id, flags, /*is_remote=*/true);
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
    return startSpan(name);
#else
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
#if defined(THEMIS_ENABLE_TRACING) && defined(THEMIS_HAS_OPENTELEMETRY)
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
#if defined(THEMIS_ENABLE_TRACING) && defined(THEMIS_HAS_OPENTELEMETRY)
    span_ = std::move(other.span_);
    context_ = std::move(other.context_);
    start_time_ = other.start_time_;
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
#if defined(THEMIS_ENABLE_TRACING) && defined(THEMIS_HAS_OPENTELEMETRY)
        span_ = std::move(other.span_);
        context_ = std::move(other.context_);
        start_time_ = other.start_time_;
#endif
        other.valid_ = false;
    }
    return *this;
}

void Tracer::Span::setAttribute([[maybe_unused]] const std::string& key, 
                                 [[maybe_unused]] const std::string& value) {
#if defined(THEMIS_ENABLE_TRACING) && defined(THEMIS_HAS_OPENTELEMETRY)
    if (span_) {
        std::string safe_value = themis::security::PIIRedactionPolicy::get()
                                     .redactAttributeValue(key, value);
        span_->SetAttribute(key, safe_value);
    }
#endif
}

void Tracer::Span::setAttribute([[maybe_unused]] const std::string& key, 
                                 [[maybe_unused]] int64_t value) {
#if defined(THEMIS_ENABLE_TRACING) && defined(THEMIS_HAS_OPENTELEMETRY)
    if (span_) {
        span_->SetAttribute(key, value);
    }
#endif
}

void Tracer::Span::setAttribute([[maybe_unused]] const std::string& key, 
                                 [[maybe_unused]] double value) {
#if defined(THEMIS_ENABLE_TRACING) && defined(THEMIS_HAS_OPENTELEMETRY)
    if (span_) {
        span_->SetAttribute(key, value);
    }
#endif
}

void Tracer::Span::setAttribute([[maybe_unused]] const std::string& key, 
                                 [[maybe_unused]] bool value) {
#if defined(THEMIS_ENABLE_TRACING) && defined(THEMIS_HAS_OPENTELEMETRY)
    if (span_) {
        span_->SetAttribute(key, value);
    }
#endif
}

void Tracer::Span::recordError([[maybe_unused]] const std::string& errorMessage) {
#if defined(THEMIS_ENABLE_TRACING) && defined(THEMIS_HAS_OPENTELEMETRY)
    if (span_) {
        std::string safe_msg = themis::security::PIIRedactionPolicy::get()
                                   .redactForLog(errorMessage);
        span_->AddEvent("exception", {{"exception.message", safe_msg}});
        span_->SetStatus(otel::trace::StatusCode::kError, safe_msg);
    }
#endif
}

void Tracer::Span::setStatus([[maybe_unused]] bool ok, 
                             [[maybe_unused]] const std::string& description) {
#if defined(THEMIS_ENABLE_TRACING) && defined(THEMIS_HAS_OPENTELEMETRY)
    if (span_) {
        auto status_code = ok ? otel::trace::StatusCode::kOk : otel::trace::StatusCode::kError;
        span_->SetStatus(status_code, description);
    }
#endif
}

void Tracer::Span::end() {
#if defined(THEMIS_ENABLE_TRACING) && defined(THEMIS_HAS_OPENTELEMETRY)
    if (span_ && !ended_) {
        span_->End();
        ended_ = true;
    }
#endif
    ended_ = true;
}

double Tracer::Span::durationMs() const {
#if defined(THEMIS_ENABLE_TRACING) && defined(THEMIS_HAS_OPENTELEMETRY)
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
