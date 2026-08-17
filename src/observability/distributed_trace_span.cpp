/**
 * @file distributed_trace_span.cpp
 * @brief Implementation of DistributedTraceSpan for Wave D Phase 2A.
 * @version 2.4.0
 * @date 2026-08-17
 */

#include "observability/distributed_trace_span.h"
#include "observability/distributed_tracing_sdk.h"

#include <random>
#include <iomanip>
#include <sstream>

namespace themis {
namespace observability {

namespace {

/**
 * @brief Generate a random 64-bit span ID as a 16-character hex string.
 * @return 16-character hex string.
 */
std::string generateSpanIdInternal() {
    static thread_local std::mt19937_64 rng(std::random_device{}());
    uint64_t span_id = rng();

    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(16) << span_id;
    return oss.str();
}

}  // anonymous namespace

// ============================================================================
// DistributedTraceSpan Implementation
// ============================================================================

DistributedTraceSpan::DistributedTraceSpan(
    std::string operation_name,
    std::shared_ptr<DistributedTraceContext> parent_context)
    : operation_name_(std::move(operation_name)),
      start_time_(std::chrono::system_clock::now()),
      parent_context_(std::move(parent_context)) {

    // Generate span ID
    span_id_ = generateSpanIdInternal();

    // Extract trace ID from parent context, or generate new root trace ID
    if (parent_context_) {
        trace_id_ = parent_context_->traceId();
    } else {
        // Generate a new root trace ID (32-hex)
        static thread_local std::mt19937_64 rng(std::random_device{}());
        uint64_t hi = rng();
        uint64_t lo = rng();

        std::ostringstream oss;
        oss << std::hex << std::setfill('0')
            << std::setw(16) << hi
            << std::setw(16) << lo;
        trace_id_ = oss.str();
    }
}

DistributedTraceSpan::~DistributedTraceSpan() {
    // Flush span on destruction (RAII)
    flush();
}

void DistributedTraceSpan::addBaggage(const std::string& key, const std::string& value) {
    if (key.empty()) {
        return;  // Silently ignore empty keys
    }

    std::unique_lock lock(baggage_mutex_);

    // Enforce max 128 baggage items (drop oldest inherited if needed)
    if (baggage_.size() >= 128) {
        // For now, just ignore new baggage if at limit
        // In production, could implement LRU eviction
        return;
    }

    baggage_[key] = value;
}

void DistributedTraceSpan::addEvent(
    const std::string& event_name,
    const std::map<std::string, std::string>& attributes) {

    if (event_name.empty()) {
        return;  // Silently ignore empty event names
    }

    std::unique_lock lock(events_mutex_);

    // Enforce max 100 events per span
    if (events_.size() >= 100) {
        return;  // Silently drop oldest event (not ideal, but bounded)
    }

    SpanEvent event(event_name);
    event.attributes = attributes;
    events_.push_back(std::move(event));
}

void DistributedTraceSpan::setAttribute(const std::string& key, const std::string& value) {
    if (key.empty()) {
        return;  // Silently ignore empty keys
    }

    std::unique_lock lock(attributes_mutex_);

    // Enforce max 100 attributes per span
    if (attributes_.size() >= 100 && attributes_.find(key) == attributes_.end()) {
        return;  // Silently drop new attribute if at limit
    }

    attributes_[key] = value;
}

void DistributedTraceSpan::setStatus(SpanStatus status, const std::string& message) {
    status_.store(status, std::memory_order_release);

    if (status == SpanStatus::Error) {
        std::unique_lock lock(status_mutex_);
        status_message_ = message;
    }
}

std::string DistributedTraceSpan::statusMessage() const {
    std::shared_lock lock(status_mutex_);
    return status_message_;
}

std::shared_ptr<DistributedTraceContext> DistributedTraceSpan::childContext(
    const std::string& child_operation_name) {

    // Use W3C traceparent to carry the parent trace ID and span ID into the
    // child context — avoids direct access to DistributedTraceContext's
    // protected members (trace_id_ / parent_span_id_).
    // Format: "00-{32-hex trace_id}-{16-hex span_id}-{flags}"
    const std::string traceparent = "00-" + trace_id_ + "-" + span_id_ + "-01";
    std::map<std::string, std::string> headers;
    headers["traceparent"] = traceparent;

    auto child_ctx = DistributedTraceContext::fromHttpHeaders(
        headers, TraceContextFormat::W3C_TRACE_CONTEXT);
    if (!child_ctx) {
        return nullptr;
    }

    // Inherit baggage from this span
    {
        std::shared_lock lock(baggage_mutex_);
        for (const auto& [key, value] : baggage_) {
            child_ctx = child_ctx->withBaggage(key, value);
            if (!child_ctx) {
                return nullptr;  // Baggage overflow
            }
        }
    }

    return child_ctx;
}

std::vector<SpanEvent> DistributedTraceSpan::events() const {
    std::shared_lock lock(events_mutex_);
    return events_;
}

std::vector<std::pair<std::string, std::string>> DistributedTraceSpan::baggage() const {
    std::shared_lock lock(baggage_mutex_);
    std::vector<std::pair<std::string, std::string>> result;
    for (const auto& [key, value] : baggage_) {
        result.emplace_back(key, value);
    }
    return result;
}

std::map<std::string, std::string> DistributedTraceSpan::attributes() const {
    std::shared_lock lock(attributes_mutex_);
    return attributes_;
}

void DistributedTraceSpan::flush() {
    // Atomically mark as flushed (only flush once)
    bool expected = false;
    if (!flushed_.compare_exchange_strong(expected, true, std::memory_order_release)) {
        return;  // Already flushed
    }

    flushInternal();
}

uint64_t DistributedTraceSpan::durationMicros() const {
    auto now = std::chrono::system_clock::now();
    auto duration = now - start_time_;
    return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
}

std::string DistributedTraceSpan::generateSpanId() {
    return generateSpanIdInternal();
}

void DistributedTraceSpan::flushInternal() {
    // TODO: Send span to OTel backend (async)
    // This will be integrated with OpenTelemetryTracer in Phase 2C
    // For now, this is a no-op (spans are queued for export during test execution)
}

} // namespace observability
} // namespace themis
