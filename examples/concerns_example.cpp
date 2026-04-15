/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            concerns_example.cpp                               ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:32:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   87.0/100                                       ║
    • Total Lines:     478                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * Core Concerns Example
 *
 * Demonstrates all five cross-cutting concern interfaces introduced in v1.6.0:
 *
 *   ILogger       — structured, context-aware logging
 *   ITracer       — distributed tracing with scoped spans
 *   IMetrics      — counters, gauges, histograms and latency tracking
 *   ICache        — key/value caching with TTL and eviction
 *   IContext      — request-scoped context propagation
 *   IAsyncLogger  — non-blocking async log dispatch
 *   IAsyncCache   — non-blocking async cache operations
 *
 * Additional v1.6.0 helpers demonstrated:
 *   MetricLabels  — type-safe fluent label builder
 *   labels::k*    — predefined label-name constants
 *   context_keys::k* — predefined context attribute constants
 *   lifecycle hooks (flush / shutdown / isHealthy)
 *
 * This example uses no-op and in-memory implementations so it can be built
 * and run without any external services (spdlog, OTLP collector, etc.).
 *
 * To adapt for production, replace the no-op/in-memory implementations
 * with the real adapters:
 *
 *   #include "core/concerns/spdlog_logger_adapter.h"
 *   #include "core/concerns/otel_tracer_adapter.h"
 *   #include "core/concerns/prometheus_metrics_adapter.h"
 */

#include "core/concerns/concerns_context.h"
#include "core/concerns/noop_implementations.h"
#include "core/concerns/inmemory_cache_impl.h"
#include "core/concerns/metric_labels.h"
#include "core/concerns/i_context.h"
#include "core/concerns/i_async_logger.h"
#include "core/concerns/i_async_cache.h"

#include <iostream>
#include <memory>
#include <string>
#include <cassert>

using namespace themis::core::concerns;

// ---------------------------------------------------------------------------
// Helper: print a section header
// ---------------------------------------------------------------------------
static void section(const std::string& title) {
    std::cout << "\n=== " << title << " ===" << std::endl;
}

// ---------------------------------------------------------------------------
// 1. ILogger — basic and structured logging
// ---------------------------------------------------------------------------
static void demo_logger(ILogger& logger) {
    section("ILogger — basic logging");

    logger.setLevel(ILogger::Level::DEBUG);
    logger.trace("This is a TRACE message (usually suppressed in production)");
    logger.debug("Debugging: value=42");
    logger.info("Service started successfully");
    logger.warn("Cache hit rate is below threshold");
    logger.error("Failed to connect to replica; retrying...");
    logger.critical("Primary storage unreachable — immediate action required");

    std::cout << "Active log level: "
              << ILogger::levelToString(logger.getLevel()) << std::endl;

    section("ILogger — structured logging");

    ILogger::Fields fields = {
        {"component",  "query_engine"},
        {"query_id",   "q-8821"},
        {"elapsed_ms", "14"},
    };
    logger.logStructured(ILogger::Level::INFO, "Query completed", fields);

    section("ILogger — logWithContext (trace correlation)");

    TraceContext tc;
    tc.trace_id   = "a1b2c3d4e5f60001";
    tc.request_id = "req-0042";
    logger.logWithContext(ILogger::Level::INFO, "Fetching user record", tc,
                          {{"user_id", "u-7"}, {"table", "users"}});

    section("ILogger — lifecycle");

    logger.flush();                 // ensure all buffered records are flushed
    auto probe = logger.isHealthy();
    std::cout << "Logger healthy: " << (probe.ok ? "yes" : "no")
              << " (" << probe.message << ")" << std::endl;
}

// ---------------------------------------------------------------------------
// 2. ITracer — span creation and attributes
// ---------------------------------------------------------------------------
static void demo_tracer(ITracer& tracer) {
    section("ITracer — root span");

    auto root = tracer.startSpan("http.request");
    root->setAttribute("http.method",  "POST");
    root->setAttribute("http.url",     "/api/v1/query");
    root->setAttribute("http.status",  200LL);
    root->setAttribute("cache.hit",    false);

    {
        section("ITracer — child span (db.query)");

        auto child = tracer.startChildSpan("db.query", *root);
        child->setAttribute("db.table",     "documents");
        child->setAttribute("db.rows",      128LL);
        child->setAttribute("db.latency_ms", 6.3);
        child->setStatus(true, "OK");
        child->end();
    }

    {
        section("ITracer — child span with error");

        auto err_span = tracer.startChildSpan("cache.get", *root);
        err_span->setAttribute("cache.key", "doc:0099");
        err_span->recordError("cache backend timeout after 250ms");
        err_span->setStatus(false, "TIMEOUT");
        err_span->end();
    }

    root->setStatus(true, "OK");
    root->end();

    section("ITracer — ScopedSpan (RAII)");

    {
        ScopedSpan scoped(tracer, "background.flush");
        scoped.setAttribute("component", "write_buffer");
        scoped.setAttribute("bytes",     4096LL);
        // span automatically ends when `scoped` goes out of scope
    }

    section("ITracer — lifecycle");

    tracer.flush();
    auto probe = tracer.isHealthy();
    std::cout << "Tracer healthy: " << (probe.ok ? "yes" : "no")
              << " (" << probe.message << ")" << std::endl;
}

// ---------------------------------------------------------------------------
// 3. IMetrics — counters, gauges, histograms, and MetricLabels
// ---------------------------------------------------------------------------
static void demo_metrics(IMetrics& metrics) {
    section("IMetrics — raw string labels (backward compatible)");

    metrics.incrementCounter("http_requests_total", 1,
        {{"method", "GET"}, {"status", "200"}, {"endpoint", "/health"}});
    metrics.setGauge("active_connections", 42.0, {{"node", "primary"}});
    metrics.observeHistogram("request_duration_seconds", 0.014,
        {{"handler", "query"}});

    section("IMetrics — MetricLabels fluent builder");

    // MetricLabels is implicitly convertible to IMetrics::Labels.
    metrics.incrementCounter("http_requests_total", 1,
        MetricLabels()
            .add(labels::kMethod,   "POST")
            .add(labels::kStatus,   "201")
            .add(labels::kEndpoint, "/api/v1/doc"));

    metrics.recordLatency("db.query", 6.3,
        MetricLabels()
            .add(labels::kOperation, "INSERT")
            .add(labels::kTable,     "documents"));

    metrics.recordError("cache.get",
        MetricLabels()
            .add(labels::kCacheResult, "miss")
            .add(labels::kCacheName,   "semantic"));

    metrics.recordSuccess("cache.get",
        MetricLabels()
            .add(labels::kCacheResult, "hit")
            .add(labels::kCacheName,   "semantic"));

    section("IMetrics — gauge manipulation");

    metrics.setGauge("memory_bytes", 1024.0 * 1024.0 * 128.0); // 128 MB
    metrics.incrementGauge("active_queries", 1.0);
    metrics.incrementGauge("active_queries", 2.0);
    metrics.decrementGauge("active_queries", 1.0); // now 2

    section("IMetrics — LatencyTimer RAII");

    {
        LatencyTimer timer(metrics, "batch_insert",
            MetricLabels().add(labels::kOperation, "bulk_write"));
        // ... simulate some work ...
        std::cout << "Elapsed so far: " << timer.elapsedMs() << " ms" << std::endl;
        // recordLatency() is called automatically in ~LatencyTimer
    }

    section("IMetrics — export and lifecycle");

    std::string exported = metrics.exportMetrics();
    std::cout << "Exported metrics length: " << exported.size() << " bytes"
              << " (empty for NoOpMetrics)" << std::endl;

    metrics.flush();
    auto probe = metrics.isHealthy();
    std::cout << "Metrics healthy: " << (probe.ok ? "yes" : "no")
              << " (" << probe.message << ")" << std::endl;
}

// ---------------------------------------------------------------------------
// 4. ICache — put / get / invalidate / stats
// ---------------------------------------------------------------------------
static void demo_cache(ICache& cache) {
    section("ICache — basic put / get");

    CacheEntry doc1{"{ \"id\": 1, \"title\": \"ThemisDB v1.6 release notes\" }",
                    /*version=*/1, /*timestamp_ms=*/1708300000000ULL};

    bool stored = cache.put("doc:1", doc1, /*ttl_ms=*/30000);
    std::cout << "Stored doc:1: " << (stored ? "yes" : "no") << std::endl;

    auto result = cache.get("doc:1");
    if (result) {
        std::cout << "Cache hit  — payload: " << result->payload
                  << ", version: "  << result->version << std::endl;
    } else {
        std::cout << "Cache miss — doc:1 not found" << std::endl;
    }

    auto miss = cache.get("doc:999");
    std::cout << "Cache miss — doc:999 present: "
              << (miss.has_value() ? "yes" : "no") << std::endl;

    section("ICache — statistics");

    std::cout << "Size:      " << cache.size()     << std::endl;
    std::cout << "Hit count: " << cache.hitCount() << std::endl;
    std::cout << "Miss count: " << cache.missCount()<< std::endl;
    std::cout << "Hit rate:  " << cache.hitRate()  << std::endl;

    section("ICache — invalidation");

    cache.put("doc:2", {"payload_2", 1, 0});
    cache.put("doc:3", {"payload_3", 1, 0});
    std::cout << "Before invalidate: size=" << cache.size() << std::endl;

    cache.invalidate("doc:2");
    std::cout << "After invalidate doc:2: size=" << cache.size() << std::endl;

    cache.invalidatePattern("doc:*");
    std::cout << "After invalidatePattern doc:*: size=" << cache.size() << std::endl;

    section("ICache — lifecycle");

    cache.put("temp:1", {"temporary", 1, 0});
    cache.flush();  // flush pending writes to backing store (no-op for in-memory)
    auto probe = cache.isHealthy();
    std::cout << "Cache healthy: " << (probe.ok ? "yes" : "no")
              << " (" << probe.message << ")" << std::endl;
}

// ---------------------------------------------------------------------------
// 5. IContext — context propagation and logWithContext bridge
// ---------------------------------------------------------------------------
static void demo_context(ILogger& logger) {
    section("IContext — root context creation");

    auto root = SimpleContext::create("trace-abc123", "req-0042");
    root->set(context_keys::kService,  "query-worker");
    root->set(context_keys::kUserId,   "u-7");
    root->set(context_keys::kTenantId, "acme-corp");

    std::cout << "trace_id:  " << root->get(context_keys::kTraceId).value_or("(none)") << std::endl;
    std::cout << "request_id: " << root->get(context_keys::kRequestId).value_or("(none)") << std::endl;
    std::cout << "service:   " << root->get(context_keys::kService).value_or("(none)") << std::endl;

    section("IContext — child context (sub-operation)");

    auto child = root->createChild();
    child->set(context_keys::kOperation, "db.query");

    // Inherited attributes visible through parent chain:
    std::cout << "Child trace_id (inherited): "
              << child->get(context_keys::kTraceId).value_or("(none)") << std::endl;
    std::cout << "Child operation (own):      "
              << child->get(context_keys::kOperation).value_or("(none)") << std::endl;
    std::cout << "Parent has operation: "
              << (root->has(context_keys::kOperation) ? "yes" : "no") << std::endl;

    section("IContext — shadow parent attribute");

    auto child2 = root->createChild();
    child2->set(context_keys::kRequestId, "req-sub-001"); // shadows parent
    std::cout << "Parent request_id: "
              << root->get(context_keys::kRequestId).value_or("(none)") << std::endl;
    std::cout << "Child2  request_id: "
              << child2->get(context_keys::kRequestId).value_or("(none)") << std::endl;

    section("IContext — toTraceContext() bridge");

    TraceContext tc = child->toTraceContext();
    std::cout << "TraceContext.trace_id:   " << tc.trace_id   << std::endl;
    std::cout << "TraceContext.request_id: " << tc.request_id << std::endl;
    std::cout << "TraceContext.empty():    " << (tc.empty() ? "yes" : "no") << std::endl;

    section("IContext — logWithContext() integration");

    logger.logWithContext(ILogger::Level::INFO, "Query executed successfully",
                          child->toTraceContext(),
                          {{"rows", "128"}, {"latency_ms", "6"}});

    section("IContext — context_keys constants");

    std::cout << "kTraceId:   " << context_keys::kTraceId   << std::endl;
    std::cout << "kRequestId: " << context_keys::kRequestId << std::endl;
    std::cout << "kUserId:    " << context_keys::kUserId    << std::endl;
    std::cout << "kTenantId:  " << context_keys::kTenantId  << std::endl;
    std::cout << "kOperation: " << context_keys::kOperation << std::endl;
    std::cout << "kService:   " << context_keys::kService   << std::endl;
    std::cout << "kSessionId: " << context_keys::kSessionId << std::endl;
}

// ---------------------------------------------------------------------------
// 6. IAsyncLogger and IAsyncCache — non-blocking async dispatch
// ---------------------------------------------------------------------------
static void demo_async_interfaces() {
    section("IAsyncLogger — fire-and-forget logging");

    NoOpAsyncLogger async_logger;

    // Fire-and-forget: the returned future is discarded;
    // the log record is dispatched asynchronously.
    async_logger.infoAsync("Service started (async)");
    async_logger.warnAsync("Cache miss rate elevated (async)");

    // Await completion (useful before shutdown or in tests):
    auto f = async_logger.errorAsync("Timeout connecting to replica");
    f.get();  // blocks until the async log call completes

    // All standard level shortcuts are available:
    async_logger.traceAsync("trace (async)").get();
    async_logger.debugAsync("debug (async)").get();
    async_logger.criticalAsync("critical (async)").get();

    // Structured async log:
    async_logger.logStructuredAsync(ILogger::Level::INFO, "Query completed",
        {{"rows", "42"}, {"latency_ms", "7"}}).get();

    section("IAsyncLogger — lifecycle");

    async_logger.flush();
    std::cout << "Async logger healthy: "
              << (async_logger.isHealthy().ok ? "yes" : "no") << std::endl;

    section("IAsyncCache — non-blocking cache operations");

    NoOpAsyncCache async_cache;

    // Async put — fire and forget write
    async_cache.putAsync("session:001", CacheEntry{"token-data", 1, 0}, 60000);

    // Async get — start the I/O then do other work
    auto get_future = async_cache.getAsync("session:001");
    // ... simulate other work while I/O is in-flight ...
    auto result = get_future.get();
    std::cout << "Async get session:001: "
              << (result.has_value() ? "hit" : "miss (NoOp always misses)")
              << std::endl;

    // Async invalidate
    async_cache.invalidateAsync("session:001").get();
    std::cout << "Async invalidate completed." << std::endl;

    section("IAsyncCache — lifecycle");

    async_cache.flush();
    std::cout << "Async cache healthy: "
              << (async_cache.isHealthy().ok ? "yes" : "no") << std::endl;
}

// ---------------------------------------------------------------------------
// 7. ConcernsContext — aggregate lifecycle and health probes
// ---------------------------------------------------------------------------
static void demo_concerns_context() {
    section("ConcernsContext — createNoOp");

    auto ctx = ConcernsContext::createNoOp();

    ctx->logInfo("Service initialised");
    ctx->logDebug("Debug: concerns context ready");

    auto span = ctx->startSpan("batch_job");
    span->setAttribute("job_id", "batch-7");
    span->setAttribute("items",  500LL);
    span->end();

    ctx->metrics().incrementCounter("batch_jobs_total", 1,
        MetricLabels().add(labels::kResult, "success"));
    ctx->recordMetric("batch_duration_ms", 250.0);

    CacheEntry entry{"serialized_result", 1, 0};
    ctx->cache().put("batch:7:result", entry, 60000);
    auto cached = ctx->cache().get("batch:7:result");
    assert(cached.has_value());

    section("ConcernsContext — healthCheck / readinessCheck");

    auto health    = ctx->healthCheck();
    auto readiness = ctx->readinessCheck();

    std::cout << "Health  — overall:  " << (health.isHealthy()    ? "healthy" : "unhealthy") << std::endl;
    std::cout << "Health  — logger:   " << (health.logger.ok      ? "ok" : health.logger.message)  << std::endl;
    std::cout << "Health  — tracer:   " << (health.tracer.ok      ? "ok" : health.tracer.message)  << std::endl;
    std::cout << "Health  — metrics:  " << (health.metrics.ok     ? "ok" : health.metrics.message) << std::endl;
    std::cout << "Health  — cache:    " << (health.cache.ok       ? "ok" : health.cache.message)   << std::endl;
    std::cout << "Readiness overall:  " << (readiness.isHealthy() ? "ready" : "not_ready")        << std::endl;

    section("ConcernsContext — flush and shutdown");

    ctx->flush();       // drain all sinks; context remains usable
    ctx->shutdown();    // teardown order: tracer → metrics → cache → logger
    std::cout << "Context shut down cleanly." << std::endl;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "  ThemisDB Core Concerns — v1.6.0 Demo" << std::endl;
    std::cout << "==================================================" << std::endl;

    // Construct individual no-op / in-memory implementations for the per-
    // interface demos so each section is self-contained.
    NoOpLogger  logger;
    NoOpTracer  tracer;
    NoOpMetrics metrics;
    InMemoryCacheImpl cache(/*maxSize=*/1000, /*defaultTTL_ms=*/0);

    demo_logger(logger);
    demo_tracer(tracer);
    demo_metrics(metrics);
    demo_cache(cache);
    demo_context(logger);
    demo_async_interfaces();
    demo_concerns_context();

    std::cout << "\n==================================================" << std::endl;
    std::cout << "  All demos completed successfully." << std::endl;
    std::cout << "==================================================" << std::endl;
    return 0;
}
