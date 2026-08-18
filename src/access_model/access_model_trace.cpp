/**
 * @file access_model_trace.cpp
 * @brief Implementation of trace context and correlation ID management.
 *
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2024 ThemisDB Contributors
 */

#include "access_model/access_model_trace.h"

#include <random>
#include <sstream>

namespace themis {
namespace access_model {

// ============================================================================
// § 1  Thread-Local Storage for TraceContext
// ============================================================================

thread_local TraceContext g_thread_local_context;
thread_local bool g_context_initialized = false;

// ============================================================================
// § 2  CorrelationID Generation
// ============================================================================

CorrelationID TraceContextManager::generateCorrelationID(
    const std::string& prefix) {
    static std::random_device rd;
    static thread_local std::mt19937 gen(rd() ^ static_cast<uint32_t>(
        std::chrono::system_clock::now().time_since_epoch().count()));
    std::uniform_int_distribution<uint64_t> dis;
    
    uint64_t part1 = dis(gen);
    uint64_t part2 = dis(gen);
    
    std::ostringstream oss;
    oss << prefix << "-" << std::hex << part1 << "-" << part2;
    return oss.str();
}

// ============================================================================
// § 3  TraceContextManager Implementation
// ============================================================================

void TraceContextManager::setContext(const TraceContext& ctx) {
    g_thread_local_context = ctx;
    g_context_initialized = true;
}

TraceContext TraceContextManager::getContext() {
    if (!g_context_initialized) {
        g_thread_local_context = TraceContext{};
        g_context_initialized = true;
    }
    return g_thread_local_context;
}

void TraceContextManager::clearContext() {
    g_thread_local_context = TraceContext{};
    g_context_initialized = false;
}

CorrelationID TraceContextManager::currentCorrelationID() {
    return getContext().correlation_id;
}

// ============================================================================
// § 4  ScopedContext RAII Helper
// ============================================================================

TraceContextManager::ScopedContext::ScopedContext(const TraceContext& ctx)
    : previous_context_(getContext()),
      context_set_(true) {
    setContext(ctx);
}

TraceContextManager::ScopedContext::~ScopedContext() {
    if (context_set_) {
        setContext(previous_context_);
    }
}

}  // namespace access_model
}  // namespace themis
