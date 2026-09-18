/**
 * @file trace_instrumentation.cpp
 * @brief Implementation of trace instrumentation utilities for Phase 2A.
 * @version 2.4.0
 * @date 2026-08-17
 */

#include "observability/trace_instrumentation.h"
#include <mutex>

namespace themis {
namespace observability {

namespace {

// Global distributed tracing SDK instance (singleton)
std::shared_ptr<DistributedTracingSDK> g_tracing_sdk;
std::once_flag g_tracing_sdk_init;

// Thread-local trace context
thread_local std::shared_ptr<DistributedTraceContext> g_current_trace_context;

// Thread-local active span (raw pointer; lifetime owned by the active TraceContextGuard)
thread_local DistributedTraceSpan* g_current_span = nullptr;

}  // anonymous namespace

/**
 * @brief Get Global Tracing SDK.
 * @return Return value.
 * @details Calls: std::call_once().
 */
DistributedTracingSDK& getGlobalTracingSDK() {
    std::call_once(g_tracing_sdk_init, []() {
        DistributedTracingConfig config;
        config.service_name = "themisdb";
        config.default_format = TraceContextFormat::W3C_TRACE_CONTEXT;
        g_tracing_sdk = std::make_shared<DistributedTracingSDK>(config);
    });
    return *g_tracing_sdk;
}

/**
 * @brief Get Current Trace Context.
 * @return Return value.
 * @details Implements getCurrentTraceContext without additional internal calls.
 */
std::shared_ptr<DistributedTraceContext> getCurrentTraceContext() {
    return g_current_trace_context;
}

/**
 * @brief Set Current Trace Context.
 * @param[in] ctx Input parameter.
 * @details Calls: std::move().
 */
void setCurrentTraceContext(std::shared_ptr<DistributedTraceContext> ctx) {
    g_current_trace_context = std::move(ctx);
}

/**
 * @brief Get Current Span.
 * @return Pointer to the result.
 * @details Implements getCurrentSpan without additional internal calls.
 */
DistributedTraceSpan* getCurrentSpan() {
    return g_current_span;
}

/**
 * @brief Set Current Span.
 * @param[in,out] span Input/output parameter.
 * @details Implements setCurrentSpan without additional internal calls.
 */
void setCurrentSpan(DistributedTraceSpan* span) {
    g_current_span = span;
}

} // namespace observability
} // namespace themis
