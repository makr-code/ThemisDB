/**
 * @file distributed_tracing_sdk.cpp
 * @brief Implementation of distributed tracing SDK for cross-service span propagation.
 *
 * Provides utilities for extracting and propagating W3C Trace Context,
 * Jaeger Baggage, and B3 trace context headers across service boundaries.
 */

#include "observability/distributed_tracing_sdk.h"
#include "observability/observability_api_contract.h"
#include <random>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <shared_mutex>

namespace themis {
namespace observability {

namespace {

// Random number generation for trace/span ID
class RandomIdGenerator {
public:
    explicit RandomIdGenerator(std::uint32_t seed) : gen_(seed) {}

    std::string generateHex(std::size_t bytes) {
        std::uniform_int_distribution<> dis(0, 15);
        std::string result = {};
        result.reserve(bytes * 2);
        for (std::size_t i = 0; i < bytes; ++i) {
            int val = dis(gen_);
            result += "0123456789abcdef"[val];
        }
        return result;
    }

private:
    std::mt19937 gen_;
};

// String utilities
std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

bool isValidHexString(const std::string& str) {
    if (str.empty()) {
      return false;
    }
    for (char c : str) {
        if (!std::isxdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    return true;
}

// W3C Trace Context parsing
std::pair<std::string, std::string> parseW3CTraceparent(const std::string& traceparent) {
    // Format: version-trace_id-parent_id-trace_flags
    // Example: 00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01
    std::istringstream iss(traceparent);
    std::string version, trace_id, span_id, flags;

    if (!std::getline(iss, version, '-') ||
        !std::getline(iss, trace_id, '-') ||
        !std::getline(iss, span_id, '-') ||
        !std::getline(iss, flags, '-')) {
        return {"", ""};
    }

    if (version != "00" || trace_id.length() != 32 || span_id.length() != 16 || flags.length() != 2) {
        return {"", ""};
    }

    if (!isValidHexString(trace_id) || !isValidHexString(span_id) || !isValidHexString(flags)) {
        return {"", ""};
    }

    return {trace_id, span_id};
}

// Jaeger trace ID parsing
std::pair<std::string, std::string> parseJaegerTraceId(const std::string& uber_trace_id) {
    // Format: trace_id:span_id:parent_span_id:sampled
    // Example: 4bf92f3577b34da6a3ce929d0e0e4736:00f067aa0ba902b7:0:1
    std::istringstream iss(uber_trace_id);
    std::string trace_id, span_id;

    if (!std::getline(iss, trace_id, ':') ||
        !std::getline(iss, span_id, ':')) {
        return {"", ""};
    }

    if (!isValidHexString(trace_id) || !isValidHexString(span_id)) {
        return {"", ""};
    }

    return {trace_id, span_id};
}

} // namespace

// ============================================================================
// DistributedTraceContext Implementation
// ============================================================================

std::shared_ptr<DistributedTraceContext> DistributedTraceContext::createRoot() {
    auto ctx = std::make_shared<DistributedTraceContext>();
    RandomIdGenerator gen(std::random_device{}());
    ctx->trace_id_ = gen.generateHex(16);  // 128-bit trace ID
    ctx->parent_span_id_ = "";  // Empty = root context
    ctx->trace_state_ = "";
    ctx->trace_sampled_ = true;
    ctx->created_at_ = std::chrono::system_clock::now();
    return ctx;
}

std::shared_ptr<DistributedTraceContext> DistributedTraceContext::fromHttpHeaders(
    const std::map<std::string, std::string>& headers,
    TraceContextFormat format) {

    auto ctx = std::make_shared<DistributedTraceContext>();

    switch (format) {
        case TraceContextFormat::W3C_TRACE_CONTEXT: {
            // Look for traceparent header
            auto it = headers.find("traceparent");
            if (it != headers.end()) {
                auto [trace_id, span_id] = parseW3CTraceparent(it->second);
                if (!trace_id.empty()) {
                    ctx->trace_id_ = trace_id;
                    ctx->parent_span_id_ = span_id;

                    // Check for tracestate
                    auto ts_it = headers.find("tracestate");
                    if (ts_it != headers.end()) {
                        ctx->trace_state_ = ts_it->second;
                    }

                    // Check trace-sampled flag from traceparent
                    auto sampled_it = headers.find("traceparent");
                    if (sampled_it != headers.end() && sampled_it->second.length() >= 29) {
                        ctx->trace_sampled_ = (sampled_it->second[29] == '1');
                    }

                    ctx->created_at_ = std::chrono::system_clock::now();
                    return ctx;
                }
            }
            break;
        }

        case TraceContextFormat::JAEGER_BAGGAGE: {
            auto it = headers.find("uber-trace-id");
            if (it != headers.end()) {
                auto [trace_id, span_id] = parseJaegerTraceId(it->second);
                if (!trace_id.empty()) {
                    ctx->trace_id_ = trace_id;
                    ctx->parent_span_id_ = span_id;
                    ctx->trace_sampled_ = true;

                    // Parse jaeger-baggage header
                    auto bg_it = headers.find("jaeger-baggage");
                    if (bg_it != headers.end()) {
                        // Simple baggage parsing: comma-separated key=value pairs
                        std::istringstream iss(bg_it->second);
                        std::string item = {};
                        while (std::getline(iss, item, ',')) {
                            size_t eq = item.find('=');
                            if (eq != std::string::npos) {
                                BaggageItem baggage;
                                baggage.key = item.substr(0, eq);
                                baggage.value = item.substr(eq + 1);
                                baggage.inherited = true;
                                ctx->baggage_.push_back(baggage);
                            }
                        }
                    }

                    ctx->created_at_ = std::chrono::system_clock::now();
                    return ctx;
                }
            }
            break;
        }

        case TraceContextFormat::B3_SINGLE: {
            auto it = headers.find("b3");
            if (it != headers.end()) {
                // Format: trace_id-span_id-sampled-parent_span_id
                std::istringstream iss(it->second);
                std::string trace_id, span_id;
                if (std::getline(iss, trace_id, '-') &&
                    std::getline(iss, span_id, '-')) {
                    if (isValidHexString(trace_id) && isValidHexString(span_id)) {
                        ctx->trace_id_ = trace_id;
                        ctx->parent_span_id_ = span_id;
                        ctx->trace_sampled_ = true;
                        ctx->created_at_ = std::chrono::system_clock::now();
                        return ctx;
                    }
                }
            }
            break;
        }

        case TraceContextFormat::B3_MULTI: {
            // B3 Multi: x-b3-traceid, x-b3-spanid, x-b3-sampled, x-b3-parentspanid
            auto tid_it = headers.find("x-b3-traceid");
            auto sid_it = headers.find("x-b3-spanid");
            if (tid_it != headers.end() && sid_it != headers.end()) {
                if (isValidHexString(tid_it->second) && isValidHexString(sid_it->second)) {
                    ctx->trace_id_ = tid_it->second;
                    ctx->parent_span_id_ = sid_it->second;

                    auto sampled_it = headers.find("x-b3-sampled");
                    ctx->trace_sampled_ = (sampled_it != headers.end() && sampled_it->second == "1");

                    ctx->created_at_ = std::chrono::system_clock::now();
                    return ctx;
                }
            }
            break;
        }
    }

    // If parsing failed, create a new root context
    return createRoot();
}

std::map<std::string, std::string> DistributedTraceContext::toHttpHeaders(
    TraceContextFormat format) const {

    std::map<std::string, std::string> headers;

    switch (format) {
        case TraceContextFormat::W3C_TRACE_CONTEXT: {
            // Build traceparent header
            std::string sampled = trace_sampled_ ? "01" : "00";
            std::string traceparent = "00-" + trace_id_ + "-" + parent_span_id_ + "-" + sampled;
            headers["traceparent"] = traceparent;

            if (!trace_state_.empty()) {
                headers["tracestate"] = trace_state_;
            }
            break;
        }

        case TraceContextFormat::JAEGER_BAGGAGE: {
            // Build uber-trace-id
            std::string sampled = trace_sampled_ ? "1" : "0";
            std::string uber_trace_id = trace_id_ + ":" + parent_span_id_ + ":0:" + sampled;
            headers["uber-trace-id"] = uber_trace_id;

            // Build jaeger-baggage
            if (!baggage_.empty()) {
                std::string baggage_str = {};
                for (size_t i = 0; i < baggage_.size(); ++i) {
                    if (i > 0) {
                      baggage_str += ",";
                    }
                    baggage_str += baggage_[i].key + "=" + baggage_[i].value;
                }
                headers["jaeger-baggage"] = baggage_str;
            }
            break;
        }

        case TraceContextFormat::B3_SINGLE: {
            std::string sampled = trace_sampled_ ? "1" : "0";
            std::string b3 = trace_id_ + "-" + parent_span_id_ + "-" + sampled;
            headers["b3"] = b3;
            break;
        }

        case TraceContextFormat::B3_MULTI: {
            headers["x-b3-traceid"] = trace_id_;
            headers["x-b3-spanid"] = parent_span_id_;
            headers["x-b3-sampled"] = trace_sampled_ ? "1" : "0";
            break;
        }
    }

    return headers;
}

std::shared_ptr<DistributedTraceContext> DistributedTraceContext::withBaggage(
    const std::string& key,
    const std::string& value) const {

    auto ctx = std::make_shared<DistributedTraceContext>();
    ctx->trace_id_ = trace_id_;
    ctx->parent_span_id_ = parent_span_id_;
    ctx->trace_state_ = trace_state_;
    ctx->trace_sampled_ = trace_sampled_;
    ctx->baggage_ = baggage_;
    ctx->created_at_ = created_at_;

    // Add new baggage item
    if (ctx-> static_cast<int>(baggage_.size()) >= kMaxBaggageItems) {
        // Remove oldest inherited baggage item to make room
        auto it = std::find_if(ctx->baggage_.begin(), ctx->baggage_.end(),
                              [](const BaggageItem& b) { return b.inherited; });
        if (it != ctx->baggage_.end()) {
            ctx->baggage_.erase(it);
        }
    }

    BaggageItem item;
    item.key = key;
    item.value = value;
    item.inherited = false;
    ctx->baggage_.push_back(item);

    return ctx;
}

// ============================================================================
// DistributedTracingSDK Implementation
// ============================================================================

DistributedTracingSDK::DistributedTracingSDK(const DistributedTracingConfig& config)
    : config_(config) {}

std::shared_ptr<DistributedTraceContext> DistributedTracingSDK::extractContextFromHeaders(
    const std::map<std::string, std::string>& headers,
    const TraceContextFormat* format) {

    std::shared_lock<std::shared_mutex> lock(config_mutex_);
    TraceContextFormat fmt = format ? *format : config_.default_format;
    return DistributedTraceContext::fromHttpHeaders(headers, fmt);
}

std::map<std::string, std::string> DistributedTracingSDK::propagateContextToHeaders(
    const std::shared_ptr<DistributedTraceContext>& context,
    const TraceContextFormat* format) {

    if (!context) {
        return {};
    }

    std::shared_lock<std::shared_mutex> lock(config_mutex_);
    TraceContextFormat fmt = format ? *format : config_.default_format;
    return context->toHttpHeaders(fmt);
}

DistributedTraceResult DistributedTracingSDK::validateTraceContext(
    const std::shared_ptr<DistributedTraceContext>& context) {

    DistributedTraceResult result = {};

    if (!context) {
        result.success = false;
        result.error_code = static_cast<int>(ObservabilityErrorCode::DTI_INVALID_TRACE_CONTEXT);
        result.error_message = "Context is null";
        return result;
    }

    // Validate trace ID
    if (context->traceId().empty() || context->traceId().length() != 32) {
        result.success = false;
        result.error_code = static_cast<int>(ObservabilityErrorCode::DTI_INVALID_TRACE_CONTEXT);
        result.error_message = "Invalid trace ID format (expected 32-char hex)";
        return result;
    }

    if (!isValidHexString(context->traceId())) {
        result.success = false;
        result.error_code = static_cast<int>(ObservabilityErrorCode::DTI_INVALID_TRACE_CONTEXT);
        result.error_message = "Trace ID contains non-hex characters";
        return result;
    }

    // Validate parent span ID if present
    if (!context->parentSpanId().empty()) {
        if (context->parentSpanId().length() != 16) {
            result.success = false;
            result.error_code = static_cast<int>(ObservabilityErrorCode::DTI_INVALID_TRACE_CONTEXT);
            result.error_message = "Invalid parent span ID format (expected 16-char hex)";
            return result;
        }
        if (!isValidHexString(context->parentSpanId())) {
            result.success = false;
            result.error_code = static_cast<int>(ObservabilityErrorCode::DTI_INVALID_TRACE_CONTEXT);
            result.error_message = "Parent span ID contains non-hex characters";
            return result;
        }
    }

    // Validate baggage
    if (context->baggage().size() > kMaxBaggageItems) {
        result.success = false;
        result.error_code = static_cast<int>(ObservabilityErrorCode::DTI_BAGGAGE_OVERFLOW);
        result.error_message = "Baggage item count exceeds limit";
        return result;
    }

    for (const auto& item : context->baggage()) {
        if (item.key.length() > kMaxBaggageKeyBytes) {
            result.success = false;
            result.error_code = static_cast<int>(ObservabilityErrorCode::DTI_BAGGAGE_OVERFLOW);
            result.error_message = "Baggage key exceeds maximum length";
            return result;
        }
        if (item.value.length() > kMaxBaggageValueBytes) {
            result.success = false;
            result.error_code = static_cast<int>(ObservabilityErrorCode::DTI_BAGGAGE_OVERFLOW);
            result.error_message = "Baggage value exceeds maximum length";
            return result;
        }
    }

    result.success = true;
    result.error_code = 0;
    return result;
}

std::shared_ptr<DistributedTraceContext> DistributedTracingSDK::createChildContext(
    const std::shared_ptr<DistributedTraceContext>& parent_context,
    const std::string& new_span_id) {

    if (!parent_context) {
        throw std::invalid_argument("Parent context is null");
    }

    // Validate parent context first
    auto validation = validateTraceContext(parent_context);
    if (!validation.success) {
        throw std::invalid_argument("Parent context is invalid: " + validation.error_message);
    }

    auto child_ctx = std::make_shared<DistributedTraceContext>();
    child_ctx->trace_id_ = parent_context->traceId();
    child_ctx->trace_state_ = parent_context->traceState();
    child_ctx->trace_sampled_ = parent_context->isTraceSampled();

    // Set parent span ID to the parent's parent span ID
    child_ctx->parent_span_id_ = new_span_id.empty() ? generateSpanId() : new_span_id;

    // Inherit baggage if configured
    {
        std::shared_lock<std::shared_mutex> lock(config_mutex_);
        if (config_.inherit_baggage) {
            child_ctx->baggage_ = parent_context->baggage();
            // Mark baggage items as inherited
            for (auto& item : child_ctx->baggage_) {
                item.inherited = true;
            }
        }
    }

    child_ctx->created_at_ = std::chrono::system_clock::now();
    return child_ctx;
}

std::string DistributedTracingSDK::generateTraceId() {
    RandomIdGenerator gen(std::random_device{}());
    return gen.generateHex(16);
}

std::string DistributedTracingSDK::generateSpanId() {
    RandomIdGenerator gen(std::random_device{}());
    return gen.generateHex(8);
}

} // namespace observability
} // namespace themis
