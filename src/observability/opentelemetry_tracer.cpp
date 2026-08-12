/**
 * @file opentelemetry_tracer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "observability/opentelemetry_tracer.h"
#include <stdexcept>
#include "observability/metrics_collector.h"
#include "tracer_utils.h"
#include "api/otlp_exporter.h"
#include "core/concerns/jaeger_tracer_adapter.h"
#include "core/concerns/zipkin_tracer_adapter.h"
#include "utils/tracing.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <iomanip>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <typeinfo>
#include <unordered_map>

namespace themis {
namespace observability {

using namespace detail;  // bring generateTraceId, generateSpanId, etc. into scope

// ---------------------------------------------------------------------------
// OTel-specific helpers
// ---------------------------------------------------------------------------

namespace {

/// Map an exporter name string to ExporterType.
/// Logs an unknown-exporter warning via MetricsCollector counter and falls
/// back to OTLP so the tracer remains functional on misconfiguration.
ExporterType exporterFromString(const std::string& name) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) {
                       return static_cast<char>(std::tolower(c));
                   });
    if (lower == "jaeger") return ExporterType::JAEGER;
    if (lower == "zipkin") return ExporterType::ZIPKIN;
    if (lower != "otlp") {
        MetricsCollector::getInstance().addCounter(
            "themis_otel_unknown_exporter_total",
            1,
            {{"exporter", "unknown"}});
    }
    return ExporterType::OTLP;
}

/// Return the canonical OTLP traces endpoint URL.
std::string resolveOtlpTracesEndpoint(const std::string& base) {
    constexpr std::string_view kSuffix = "/v1/traces";
    if (base.size() >= kSuffix.size() &&
        base.compare(base.size() - kSuffix.size(), kSuffix.size(), kSuffix) == 0) {
        return base;
    }
    return base + std::string(kSuffix);
}

/**
 * @brief Production span used by OpenTelemetryTracer.
 *
 * Records attributes and pushes a SpanRecord into the owning tracer's ring
 * buffer on end().  If an export_cb_ is set (non-null), the completed
 * SpanRecord is also forwarded to the exporter pipeline.
 */
class OtelSpan : public core::concerns::ITracer::ISpan {
public:
    /// Callback invoked once on span end with the finished SpanRecord.
    using ExportCallback = std::function<void(const SpanRecord&)>;

    OtelSpan(std::string                name,
             std::string                trace_id,
             std::string                span_id,
             std::string                parent_span_id,
             std::deque<SpanRecord>*    ring_buf,
             std::mutex*                ring_mu,
             size_t                     max_retained,
             std::atomic<int64_t>*      active_count,
             std::atomic<int64_t>*      total_count,
             ExportCallback             export_cb = nullptr)
        : name_(std::move(name))
        , trace_id_(std::move(trace_id))
        , span_id_(std::move(span_id))
        , parent_span_id_(std::move(parent_span_id))
        , ring_buf_(ring_buf)
        , ring_mu_(ring_mu)
        , max_retained_(max_retained)
        , active_count_(active_count)
        , total_count_(total_count)
        , export_cb_(std::move(export_cb))
        , start_time_(std::chrono::system_clock::now())
    {
        if (active_count_) ++(*active_count_);
        if (total_count_)  ++(*total_count_);
    }

    ~OtelSpan() override { endSpan(); }

    void setAttribute(const std::string& key,
                      const std::string& value) override {
        std::lock_guard<std::mutex> lk(attr_mu_);
        attributes_[key] = value;
    }

    void setAttribute(const std::string& key, int64_t value) override {
        setAttribute(key, std::to_string(value));
    }

    void setAttribute(const std::string& key, double value) override {
        std::ostringstream oss;
        oss << value;
        setAttribute(key, oss.str());
    }

    void setAttribute(const std::string& key, bool value) override {
        setAttribute(key, std::string(value ? "true" : "false"));
    }

    void recordError(const std::string& errorMessage) override {
        setStatus(false, errorMessage);
        setAttribute("error.message", errorMessage);
    }

    void setStatus(bool ok, const std::string& description = "") override {
        ok_                 = ok;
        status_description_ = description;
    }

    void end() override { endSpan(); }

    bool isValid() const override { return true; }

    // Extra accessors for context propagation
    const std::string& traceId()      const noexcept { return trace_id_; }
    const std::string& spanId()       const noexcept { return span_id_; }
    const std::string& parentSpanId() const noexcept { return parent_span_id_; }

private:
    void endSpan() {
        if (ended_.exchange(true)) return; // idempotent

        auto end_time = std::chrono::system_clock::now();
        if (active_count_) --(*active_count_);

        SpanRecord rec;
        {
            std::lock_guard<std::mutex> lk(attr_mu_);
            rec.attributes = attributes_;
        }
        rec.name               = name_;
        rec.trace_id           = trace_id_;
        rec.span_id            = span_id_;
        rec.parent_span_id     = parent_span_id_;
        rec.start_time         = start_time_;
        rec.end_time           = end_time;
        rec.ok                 = ok_;
        rec.status_description = status_description_;

        // Forward to configured exporter backends (OTLP/Jaeger/Zipkin)
        if (export_cb_) {
            try {
                export_cb_(rec);
            } catch (...) {
                // Export is best-effort; never block span completion
            }
        }

        // Retain in in-process ring buffer for local diagnostics
        if (ring_buf_ && ring_mu_ && max_retained_ > 0) {
            std::lock_guard<std::mutex> lk(*ring_mu_);
            ring_buf_->push_back(std::move(rec));
            while (ring_buf_->size() > max_retained_) {
                ring_buf_->pop_front();
            }
        }
    }

    std::string name_;
    std::string trace_id_;
    std::string span_id_;
    std::string parent_span_id_;

    std::deque<SpanRecord>* ring_buf_;
    std::mutex*             ring_mu_;
    size_t                  max_retained_;
    std::atomic<int64_t>*   active_count_;
    std::atomic<int64_t>*   total_count_;
    ExportCallback          export_cb_;

    std::chrono::system_clock::time_point start_time_;
    std::atomic<bool>                     ended_{false};
    bool                                  ok_{true};
    std::string                           status_description_;

    mutable std::mutex                  attr_mu_;
    std::map<std::string, std::string>  attributes_;
};

/**
 * @brief No-op span returned when a trace is sampled out.
 */
class DroppedOtelSpan : public core::concerns::ITracer::ISpan {
public:
    void setAttribute(const std::string&, const std::string&) override {}
    void setAttribute(const std::string&, int64_t)            override {}
    void setAttribute(const std::string&, double)             override {}
    void setAttribute(const std::string&, bool)               override {}
    void recordError(const std::string&)                      override {}
    void setStatus(bool, const std::string& = "")             override {}
    void end()                                                override {}
    bool isValid() const                                      override { return false; }
};

} // anonymous namespace

// ---------------------------------------------------------------------------
// OpenTelemetryTracer::Impl
// ---------------------------------------------------------------------------

/** @brief OpenTelemetryTracer::Impl. */
class OpenTelemetryTracer::Impl {
public:
    explicit Impl(const OTelConfig& cfg)
        : config_(cfg)
        , initialized_(true)
    {
        // Validate exporter types and set up backend instances
        for (const auto& name : cfg.exporters) {
            ExporterType et = exporterFromString(name);
            active_exporter_types_.push_back(et);

            if (et == ExporterType::OTLP && !cfg.endpoint.empty()) {
                // Create and start an OTLP exporter for real span export.
                // resolveOtlpTracesEndpoint() handles the case where the
                // caller has already appended "/v1/traces".
                api::OtlpExporterConfig ocfg;
                ocfg.enabled         = true;
                ocfg.endpoint        = resolveOtlpTracesEndpoint(cfg.endpoint);
                ocfg.service_name    = cfg.service_name;
                ocfg.service_version = cfg.service_version;
                otlp_exporter_ = std::make_shared<api::OtlpExporter>(ocfg);
                otlp_exporter_->start();
            } else if (et == ExporterType::JAEGER) {
                auto jaeger = std::make_unique<core::concerns::JaegerTracerAdapter>();
                jaeger->initialize(cfg.service_name, cfg.endpoint);
                delegate_tracers_.push_back(std::move(jaeger));
            } else if (et == ExporterType::ZIPKIN) {
                auto zipkin = std::make_unique<core::concerns::ZipkinTracerAdapter>();
                zipkin->initialize(cfg.service_name, cfg.endpoint);
                delegate_tracers_.push_back(std::move(zipkin));
            }
        }
        if (active_exporter_types_.empty()) {
            active_exporter_types_.push_back(ExporterType::OTLP);
        }
    }

    ~Impl() {
        if (otlp_exporter_) {
            otlp_exporter_->stop();
            otlp_exporter_.reset(); // release before member destruction
        }
    }

    OTelConfig                config_;
    bool                      initialized_;
    std::vector<ExporterType> active_exporter_types_;

    // OTLP async HTTP exporter (present when "otlp" is in exporters list and
    // endpoint is non-empty).  Stored as shared_ptr so the export callback
    // can safely capture a weak_ptr and avoid use-after-free.
    std::shared_ptr<api::OtlpExporter> otlp_exporter_;

    // Delegate sub-tracers for Jaeger / Zipkin backends
    std::vector<std::unique_ptr<core::concerns::ITracer>> delegate_tracers_;

    mutable std::mutex     ring_mu_;
    std::deque<SpanRecord> ring_buf_;

    std::atomic<int64_t> total_spans_{0};
    std::atomic<int64_t> active_spans_{0};
    std::atomic<int64_t> dropped_spans_{0};

    // Last active span context for the no-arg injectContext() overload
    mutable std::mutex ctx_mu_;
    std::string        last_trace_id_;
    std::string        last_span_id_;

    /// Build the export callback that dispatches a completed SpanRecord to
    /// all configured backends.  Uses std::weak_ptr so the callback is safe
    /// if invoked after the Impl (and OtlpExporter) has been destroyed.
    OtelSpan::ExportCallback makeExportCallback() {
        if (!otlp_exporter_ && delegate_tracers_.empty()) {
            return nullptr; // nothing to forward
        }

        // Capture a weak_ptr: if the Impl is destroyed before the span ends
        // (e.g. a span outlives its tracer) the callback simply no-ops.
        std::weak_ptr<api::OtlpExporter> weak_exporter = otlp_exporter_;

        return [weak_exporter](const SpanRecord& rec) {
            if (auto exporter = weak_exporter.lock()) {
                api::SpanData sd;
                sd.trace_id       = rec.trace_id;
                sd.span_id        = rec.span_id;
                sd.parent_span_id = rec.parent_span_id;
                sd.name           = rec.name;

                // Convert system_clock time points → nanoseconds since epoch
                sd.start_time_unix_nano =
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                        rec.start_time.time_since_epoch()).count();
                sd.end_time_unix_nano =
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                        rec.end_time.time_since_epoch()).count();

                // OTLP status: 0=Unset, 1=OK, 2=Error
                sd.status_code    = rec.ok ? 1 : 2;
                sd.status_message = rec.status_description;

                for (const auto& [k, v] : rec.attributes) {
                    sd.attributes[k] = v;
                }

                exporter->enqueue(std::move(sd));
            }
            // Note: delegate_tracers_ (Jaeger/Zipkin) use themis::Tracer global;
            // context propagation for those backends happens via startSpan/
            // startSpanFromHeaders on their adapters, which the caller invokes.
        };
    }

    std::unique_ptr<core::concerns::ITracer::ISpan> makeSpan(
        const std::string& name,
        const std::string& trace_id,
        const std::string& parent_span_id)
    {
        if (!shouldSample(config_.sample_rate)) {
            ++dropped_spans_;
            return std::make_unique<DroppedOtelSpan>();
        }

        std::string span_id = generateSpanId();

        // Cache last context for injectContext()
        {
            std::lock_guard<std::mutex> lk(ctx_mu_);
            last_trace_id_ = trace_id;
            last_span_id_  = span_id;
        }

        publishMetrics();

        return std::make_unique<OtelSpan>(
            name,
            trace_id,
            span_id,
            parent_span_id,
            config_.max_retained_spans > 0 ? &ring_buf_ : nullptr,
            config_.max_retained_spans > 0 ? &ring_mu_  : nullptr,
            config_.max_retained_spans,
            &active_spans_,
            &total_spans_,
            makeExportCallback());
    }

    void publishMetrics() const {
        if (!config_.publish_metrics) return;
        auto& mc = MetricsCollector::getInstance();
        mc.setGauge("themis_otel_spans_total",
                    static_cast<double>(total_spans_.load()));
        mc.setGauge("themis_otel_active_spans",
                    static_cast<double>(active_spans_.load()));
        mc.setGauge("themis_otel_dropped_spans_total",
                    static_cast<double>(dropped_spans_.load()));
    }
};

// ---------------------------------------------------------------------------
// OpenTelemetryTracer — public API
// ---------------------------------------------------------------------------

OpenTelemetryTracer::OpenTelemetryTracer(const OTelConfig& config)
    : impl_(std::make_unique<Impl>(config))
{}

OpenTelemetryTracer::~OpenTelemetryTracer() = default;

// -- ITracer span creation ---------------------------------------------------

std::unique_ptr<core::concerns::ITracer::ISpan>
OpenTelemetryTracer::startSpan(const std::string& name)
{
    return impl_->makeSpan(name, generateTraceId(), "");
}

std::unique_ptr<core::concerns::ITracer::ISpan>
OpenTelemetryTracer::startChildSpan(const std::string& name,
                                    const ISpan& parent)
{
    const auto* otelParent = dynamic_cast<const OtelSpan*>(&parent);
    if (otelParent) {
        return impl_->makeSpan(name, otelParent->traceId(), otelParent->spanId());
    }
    // Fallback: new root span
    return impl_->makeSpan(name, generateTraceId(), "");
}

std::unique_ptr<core::concerns::ITracer::ISpan>
OpenTelemetryTracer::startSpanFromHeaders(
    const std::string& name,
    const std::map<std::string, std::string>& headers)
{
    // Extract W3C Baggage into thread-local store
    extractBaggage(headers);

    std::string traceparent = findHeader(headers, "traceparent");
    auto [trace_id, parent_span_id] = parseTraceparent(traceparent);

    if (trace_id.empty()) {
        return impl_->makeSpan(name, generateTraceId(), "");
    }
    return impl_->makeSpan(name, trace_id, parent_span_id);
}

// -- ITracer context injection -----------------------------------------------

void OpenTelemetryTracer::injectContext(
    std::map<std::string, std::string>& headers)
{
    {
        std::lock_guard<std::mutex> lk(impl_->ctx_mu_);
        if (!impl_->last_trace_id_.empty() && !impl_->last_span_id_.empty()) {
            headers["traceparent"] = buildTraceparent(
                impl_->last_trace_id_, impl_->last_span_id_, true);
        }
    }
    // Always inject baggage (tenant/user context)
    themis::Baggage::inject(headers);
}

// -- ITracer lifecycle -------------------------------------------------------

bool OpenTelemetryTracer::initialize(const std::string& serviceName,
                                     const std::string& endpoint)
{
    impl_->config_.service_name = serviceName;
    impl_->config_.endpoint     = endpoint;
    impl_->initialized_         = true;
    return true;
}

void OpenTelemetryTracer::shutdown()
{
    impl_->initialized_ = false;
    if (impl_->otlp_exporter_) {
        impl_->otlp_exporter_->stop();
    }
    impl_->publishMetrics();
}

bool OpenTelemetryTracer::isInitialized() const
{
    return impl_->initialized_;
}

void OpenTelemetryTracer::flush() noexcept
{
    impl_->publishMetrics();
}

core::concerns::ProbeResult OpenTelemetryTracer::isHealthy() const
{
    if (!impl_->initialized_) {
        return core::concerns::ProbeResult::unhealthy("tracer not initialized");
    }
    return core::concerns::ProbeResult::healthy();
}

// -- Extended API ------------------------------------------------------------

SpanContext OpenTelemetryTracer::extractContext(
    const std::map<std::string, std::string>& headers) const
{
    std::string traceparent = findHeader(headers, "traceparent");
    auto [trace_id, span_id] = parseTraceparent(traceparent);

    SpanContext ctx;
    ctx.trace_id = trace_id;
    ctx.span_id  = span_id;

    // Parse sampled flag from flags byte (last 2 hex chars of traceparent)
    if (traceparent.size() >= 55) {
        std::string flags = traceparent.substr(53, 2);
        ctx.sampled = (flags == "01");
    }

    return ctx;
}

void OpenTelemetryTracer::injectContext(
    const ISpan& span,
    std::map<std::string, std::string>& headers)
{
    const auto* otelSpan = dynamic_cast<const OtelSpan*>(&span);
    if (otelSpan && !otelSpan->traceId().empty()) {
        headers["traceparent"] = buildTraceparent(
            otelSpan->traceId(), otelSpan->spanId(), true);
    } else {
        // Fall back to the last active span context
        std::lock_guard<std::mutex> lk(impl_->ctx_mu_);
        if (!impl_->last_trace_id_.empty()) {
            headers["traceparent"] = buildTraceparent(
                impl_->last_trace_id_, impl_->last_span_id_, true);
        }
    }
    themis::Baggage::inject(headers);
}

void OpenTelemetryTracer::recordException(ISpan& span,
                                          const std::exception& ex)
{
    // typeid().name() returns the implementation-defined (possibly mangled)
    // type name.  It is useful for diagnostics even in mangled form; operators
    // can compare it to known values or demangle it externally.
    span.setAttribute("exception.type",    std::string(typeid(ex).name()));
    span.setAttribute("exception.message", std::string(ex.what()));
    span.recordError(ex.what());
}

void OpenTelemetryTracer::recordMetrics(ISpan& span,
                                        const SpanMetrics& metrics)
{
    if (metrics.cpu_usage_percent != 0.0) {
        span.setAttribute("db.metrics.cpu_usage_percent",
                          metrics.cpu_usage_percent);
    }
    if (metrics.memory_usage_bytes != 0.0) {
        span.setAttribute("db.metrics.memory_usage_bytes",
                          metrics.memory_usage_bytes);
    }
    if (metrics.active_connections != 0) {
        span.setAttribute("db.metrics.active_connections",
                          metrics.active_connections);
    }
    if (metrics.query_count != 0) {
        span.setAttribute("db.metrics.query_count",
                          metrics.query_count);
    }
    if (metrics.cache_hit_rate != 0.0) {
        span.setAttribute("db.metrics.cache_hit_rate",
                          metrics.cache_hit_rate);
    }
    for (const auto& [key, val] : metrics.custom) {
        span.setAttribute("db.metrics.custom." + key, val);
    }
}

// -- Baggage -----------------------------------------------------------------

void OpenTelemetryTracer::setBaggageItem(const std::string& key,
                                         const std::string& value)
{
    themis::Baggage::set(key, value);
}

std::string OpenTelemetryTracer::getBaggageItem(const std::string& key)
{
    return themis::Baggage::get(key);
}

void OpenTelemetryTracer::removeBaggageItem(const std::string& key)
{
    themis::Baggage::remove(key);
}

void OpenTelemetryTracer::clearBaggage()
{
    themis::Baggage::clear();
}

void OpenTelemetryTracer::extractBaggage(
    const std::map<std::string, std::string>& headers)
{
    themis::Baggage::extract(headers);
}

// -- Diagnostics -------------------------------------------------------------

TracerStats OpenTelemetryTracer::stats() const
{
    TracerStats s;
    s.total_spans   = impl_->total_spans_.load();
    s.active_spans  = impl_->active_spans_.load();
    s.dropped_spans = impl_->dropped_spans_.load();
    return s;
}

std::vector<SpanRecord> OpenTelemetryTracer::completedSpans() const
{
    std::lock_guard<std::mutex> lk(impl_->ring_mu_);
    return {impl_->ring_buf_.begin(), impl_->ring_buf_.end()};
}

void OpenTelemetryTracer::clearCompletedSpans()
{
    std::lock_guard<std::mutex> lk(impl_->ring_mu_);
    impl_->ring_buf_.clear();
}

OTelConfig OpenTelemetryTracer::getConfig() const
{
    return impl_->config_;
}

uint64_t OpenTelemetryTracer::otlpExportedSpanCount() const noexcept
{
    if (impl_->otlp_exporter_) {
        return impl_->otlp_exporter_->exportedSpanCount();
    }
    return 0;
}

uint64_t OpenTelemetryTracer::otlpDroppedSpanCount() const noexcept
{
    if (impl_->otlp_exporter_) {
        return impl_->otlp_exporter_->droppedSpanCount();
    }
    return 0;
}

std::vector<std::string> OpenTelemetryTracer::activeExporters() const
{
    return impl_->config_.exporters;
}

} // namespace observability
} // namespace themis

