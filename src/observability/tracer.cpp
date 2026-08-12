/**
 * @file tracer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "observability/tracer.h"
#include "observability/metrics_collector.h"
#include "tracer_utils.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <deque>
#include <iomanip>
#include <mutex>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace themis {
namespace observability {

using namespace detail;  // bring generateTraceId, generateSpanId, etc. into scope

// ---------------------------------------------------------------------------
// ObservabilitySpan — ISpan implementation
// ---------------------------------------------------------------------------

/**
 * @brief Production span that records attributes and reports back to the
 *        owning tracer's ring buffer when ended.
 */
class ObservabilitySpan : public core::concerns::ITracer::ISpan {
public:
    ObservabilitySpan(std::string name,
                      std::string trace_id,
                      std::string span_id,
                      std::string parent_span_id,
                      std::deque<SpanRecord>*    ring_buf,
                      std::mutex*                ring_mu,
                      size_t                     max_retained,
                      std::atomic<int64_t>*      active_count,
                      std::atomic<int64_t>*      total_count,
                      std::weak_ptr<ContinuousProfiler> profiler,
                      bool                       attach_profile)
        : name_(std::move(name))
        , trace_id_(std::move(trace_id))
        , span_id_(std::move(span_id))
        , parent_span_id_(std::move(parent_span_id))
        , ring_buf_(ring_buf)
        , ring_mu_(ring_mu)
        , max_retained_(max_retained)
        , active_count_(active_count)
        , total_count_(total_count)
        , profiler_(std::move(profiler))
        , attach_profile_(attach_profile)
        , start_time_(std::chrono::system_clock::now())
    {
        if (active_count_) ++(*active_count_);
        if (total_count_)  ++(*total_count_);
    }

    ~ObservabilitySpan() override {
        endSpan();
    }

    void setAttribute(const std::string& key, const std::string& value) override {
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
        ok_ = ok;
        status_description_ = description;
    }

    void end() override {
        endSpan();
    }

    bool isValid() const override { return true; }

    // Extra accessors used by the tracer for context propagation
    const std::string& traceId()      const { return trace_id_; }
    const std::string& spanId()       const { return span_id_; }
    const std::string& parentSpanId() const { return parent_span_id_; }

private:
    void endSpan() {
        if (ended_.exchange(true)) return;  // idempotent

        auto end_time = std::chrono::system_clock::now();
        if (active_count_) --(*active_count_);

        if (ring_buf_ && ring_mu_ && max_retained_ > 0) {
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

            // ContinuousProfiler integration: attach CPU snapshot when configured
            if (attach_profile_) {
                if (auto prof = profiler_.lock()) {
                    try {
                        auto snap = prof->snapshot(ProfileType::CPU);
                        rec.cpu_profile_folded = snap.dataAsString();
                    } catch (...) {
                        // Profiling is best-effort; never block the span from ending
                    }
                }
            }

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

    std::weak_ptr<ContinuousProfiler>    profiler_;
    bool                                 attach_profile_;
    std::chrono::system_clock::time_point start_time_;
    std::atomic<bool>                     ended_{false};
    bool                                  ok_{true};
    std::string                           status_description_;

    mutable std::mutex                       attr_mu_;
    std::map<std::string, std::string>       attributes_;
};

// ---------------------------------------------------------------------------
// Dropped span — returned when sampling decides not to record
// ---------------------------------------------------------------------------

/** @brief Dropped span — returned when sampling decides not to record. */
class DroppedSpan : public core::concerns::ITracer::ISpan {
public:
    void setAttribute(const std::string&, const std::string&) override {}
    void setAttribute(const std::string&, int64_t) override {}
    void setAttribute(const std::string&, double) override {}
    void setAttribute(const std::string&, bool) override {}
    void recordError(const std::string&) override {}
    void setStatus(bool, const std::string& = "") override {}
    void end() override {}
    bool isValid() const override { return false; }
};

// ---------------------------------------------------------------------------
// ObservabilityTracer::Impl
// ---------------------------------------------------------------------------

/** @brief ObservabilityTracer::Impl. */
class ObservabilityTracer::Impl {
public:
    explicit Impl(const ObservabilityTracerConfig& cfg)
        : config_(cfg)
        , initialized_(false)
    {}

    ObservabilityTracerConfig config_;
    bool                      initialized_;
    mutable std::mutex        ring_mu_;
    std::deque<SpanRecord>    ring_buf_;
    std::atomic<int64_t>      total_spans_{0};
    std::atomic<int64_t>      active_spans_{0};
    std::atomic<int64_t>      dropped_spans_{0};

    // Last active span context for injectContext()
    mutable std::mutex        ctx_mu_;
    std::string               last_trace_id_;
    std::string               last_span_id_;

    std::unique_ptr<ISpan> makeSpan(const std::string& name,
                                    const std::string& trace_id,
                                    const std::string& parent_span_id) {
        if (!shouldSample(config_.sample_rate)) {
            ++dropped_spans_;
            return std::make_unique<DroppedSpan>();
        }

        std::string span_id = generateSpanId();

        // Cache most-recently-started span context for injectContext()
        {
            std::lock_guard<std::mutex> lk(ctx_mu_);
            last_trace_id_ = trace_id;
            last_span_id_  = span_id;
        }

        publishMetrics();

        return std::make_unique<ObservabilitySpan>(
            name,
            trace_id,
            span_id,
            parent_span_id,
            config_.max_retained_spans > 0 ? &ring_buf_ : nullptr,
            config_.max_retained_spans > 0 ? &ring_mu_  : nullptr,
            config_.max_retained_spans,
            &active_spans_,
            &total_spans_,
            config_.profiler,          // weak_ptr to profiler (may be nullptr)
            config_.attach_profile_on_span_end
        );
    }

    void publishMetrics() const {
        if (!config_.publish_metrics) return;
        auto& mc = MetricsCollector::getInstance();
        mc.setGauge("themis_tracer_spans_total",
                    static_cast<double>(total_spans_.load()));
        mc.setGauge("themis_tracer_active_spans",
                    static_cast<double>(active_spans_.load()));
        mc.setGauge("themis_tracer_dropped_spans_total",
                    static_cast<double>(dropped_spans_.load()));
    }
};

// ---------------------------------------------------------------------------
// ObservabilityTracer — public API
// ---------------------------------------------------------------------------

ObservabilityTracer::ObservabilityTracer(const ObservabilityTracerConfig& config)
    : impl_(std::make_unique<Impl>(config))
{
    impl_->initialized_ = true;
}

ObservabilityTracer::~ObservabilityTracer() = default;

std::unique_ptr<core::concerns::ITracer::ISpan>
ObservabilityTracer::startSpan(const std::string& name) {
    return impl_->makeSpan(name, generateTraceId(), "");
}

std::unique_ptr<core::concerns::ITracer::ISpan>
ObservabilityTracer::startChildSpan(const std::string& name,
                                     const ISpan& parent) {
    // Attempt to cast to ObservabilitySpan to extract trace/span context
    const auto* obs = dynamic_cast<const ObservabilitySpan*>(&parent);
    if (obs) {
        return impl_->makeSpan(name, obs->traceId(), obs->spanId());
    }
    // Fallback: start a new root span
    return impl_->makeSpan(name, generateTraceId(), "");
}

std::unique_ptr<core::concerns::ITracer::ISpan>
ObservabilityTracer::startSpanFromHeaders(
    const std::string& name,
    const std::map<std::string, std::string>& headers)
{
    std::string traceparent = findHeader(headers, "traceparent");
    auto [trace_id, parent_span_id] = parseTraceparent(traceparent);

    if (trace_id.empty()) {
        // No valid upstream context; start a root span
        return impl_->makeSpan(name, generateTraceId(), "");
    }
    return impl_->makeSpan(name, trace_id, parent_span_id);
}

void ObservabilityTracer::injectContext(std::map<std::string, std::string>& headers) {
    std::lock_guard<std::mutex> lk(impl_->ctx_mu_);
    if (!impl_->last_trace_id_.empty() && !impl_->last_span_id_.empty()) {
        headers["traceparent"] = buildTraceparent(
            impl_->last_trace_id_, impl_->last_span_id_, true);
    }
}

bool ObservabilityTracer::initialize(const std::string& /*serviceName*/,
                                      const std::string& /*endpoint*/) {
    impl_->initialized_ = true;
    return true;
}

void ObservabilityTracer::shutdown() {
    impl_->initialized_ = false;
    impl_->publishMetrics();
}

bool ObservabilityTracer::isInitialized() const {
    return impl_->initialized_;
}

void ObservabilityTracer::flush() noexcept {
    impl_->publishMetrics();
}

core::concerns::ProbeResult ObservabilityTracer::isHealthy() const {
    return core::concerns::ProbeResult::healthy();
}

TracerStats ObservabilityTracer::stats() const {
    TracerStats s;
    s.total_spans   = impl_->total_spans_.load();
    s.active_spans  = impl_->active_spans_.load();
    s.dropped_spans = impl_->dropped_spans_.load();
    return s;
}

std::vector<SpanRecord> ObservabilityTracer::completedSpans() const {
    std::lock_guard<std::mutex> lk(impl_->ring_mu_);
    return {impl_->ring_buf_.begin(), impl_->ring_buf_.end()};
}

void ObservabilityTracer::clearCompletedSpans() {
    std::lock_guard<std::mutex> lk(impl_->ring_mu_);
    impl_->ring_buf_.clear();
}

ObservabilityTracerConfig ObservabilityTracer::getConfig() const {
    return impl_->config_;
}

} // namespace observability
} // namespace themis

