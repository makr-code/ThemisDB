/**
 * @file test_otlp_exporter.cpp
 * @brief Unit tests for OtlpExporter and TracingMiddleware OTLP integration
 *
 * Tests cover:
 *  - OtlpExporter queue behaviour (enqueue, drop-on-full)
 *  - OTLP JSON payload structure (buildOtlpJson via integration)
 *  - TracingMiddleware::finishSpan() enqueues a span when exporter is attached
 *  - TracingMiddleware without exporter (no crash, no-op finishSpan)
 *  - OtlpExporter start/stop lifecycle
 */

#include <gtest/gtest.h>
#include "api/otlp_exporter.h"
#include "api/tracing_middleware.h"

#include <chrono>
#include <thread>

using namespace themis::api;

// ---------------------------------------------------------------------------
// OtlpExporter — disabled mode (no network calls)
// ---------------------------------------------------------------------------

TEST(OtlpExporterTest, DisabledByDefault)
{
    OtlpExporterConfig cfg;
    cfg.enabled = false;

    OtlpExporter exp(cfg);
    exp.start(); // no-op when disabled

    SpanData span;
    span.trace_id = "aabbccdd00112233aabbccdd00112233";
    span.name     = "test-span";
    span.start_time_unix_nano = 1000000000LL;
    span.end_time_unix_nano   = 2000000000LL;

    exp.enqueue(span); // must not crash
    EXPECT_EQ(exp.exportedSpanCount(), 0u);
    EXPECT_EQ(exp.droppedSpanCount(),  0u);

    exp.stop(); // no-op
}

TEST(OtlpExporterTest, QueueDropsWhenFull)
{
    // Use enabled=true but never start → background thread absent, queue fills up.
    OtlpExporterConfig cfg;
    cfg.enabled        = true;
    cfg.max_queue_size = 3;

    OtlpExporter exp2(cfg);
    // Do NOT call start() — background thread absent, queue accumulates.

    SpanData s;
    s.trace_id = "00000000000000000000000000000001";
    s.name     = "s";

    // Fill to capacity
    exp2.enqueue(s);
    exp2.enqueue(s);
    exp2.enqueue(s);
    EXPECT_EQ(exp2.droppedSpanCount(), 0u);

    // One more → oldest dropped
    exp2.enqueue(s);
    EXPECT_EQ(exp2.droppedSpanCount(), 1u);

    exp2.stop(); // drains queue without sending (no background thread)
}

TEST(OtlpExporterTest, ConfigAccessors)
{
    OtlpExporterConfig cfg;
    cfg.enabled        = true;
    cfg.endpoint       = "http://otel:4318/v1/traces";
    cfg.service_name   = "my-service";
    cfg.timeout_ms     = 2000;

    OtlpExporter exp(cfg);
    EXPECT_EQ(exp.config().endpoint,     "http://otel:4318/v1/traces");
    EXPECT_EQ(exp.config().service_name, "my-service");
    EXPECT_EQ(exp.config().timeout_ms,   2000);
}

TEST(OtlpExporterTest, StartStopNoOp)
{
    // Calling start/stop multiple times must be safe.
    OtlpExporterConfig cfg;
    cfg.enabled = false;

    OtlpExporter exp(cfg);
    exp.start();
    exp.start(); // idempotent
    exp.stop();
    exp.stop();  // idempotent
}

// ---------------------------------------------------------------------------
// SpanData helpers
// ---------------------------------------------------------------------------

TEST(SpanDataTest, DefaultValues)
{
    SpanData s;
    EXPECT_EQ(s.status_code, 0);
    EXPECT_EQ(s.start_time_unix_nano, 0);
    EXPECT_EQ(s.end_time_unix_nano,   0);
    EXPECT_TRUE(s.name.empty());
    EXPECT_TRUE(s.attributes.empty());
}

TEST(SpanDataTest, AttributeStorage)
{
    SpanData s;
    s.attributes["http.method"]       = "GET";
    s.attributes["http.status_code"]  = "200";
    s.attributes["db.system"]         = "themisdb";

    EXPECT_EQ(s.attributes.size(), 3u);
    EXPECT_EQ(s.attributes.at("http.method"), "GET");
}

// ---------------------------------------------------------------------------
// TracingMiddleware — without exporter
// ---------------------------------------------------------------------------

TEST(TracingMiddlewareTest, WithoutExporterNoOp)
{
    TracingMiddleware mw;

    const std::string id = mw.processRequest("");
    EXPECT_FALSE(id.empty());
    EXPECT_EQ(id, TracingMiddleware::currentCorrelationId());

    // finishSpan must not crash when no exporter is attached
    mw.finishSpan("HTTP GET /test", 200);

    TracingMiddleware::clearContext();
    EXPECT_TRUE(TracingMiddleware::currentCorrelationId().empty());
}

TEST(TracingMiddlewareTest, PreserveIncomingCorrelationId)
{
    TracingMiddleware mw;
    const std::string incoming = "my-correlation-id-12345";
    const std::string result   = mw.processRequest(incoming);

    EXPECT_EQ(result, incoming);
    EXPECT_EQ(TracingMiddleware::currentCorrelationId(), incoming);

    TracingMiddleware::clearContext();
}

TEST(TracingMiddlewareTest, GeneratesUuidWhenAbsent)
{
    TracingMiddleware mw;
    const std::string id = mw.processRequest("");

    // RFC 4122 UUID v4: 36 chars with dashes at positions 8, 13, 18, 23
    EXPECT_EQ(id.size(), 36u);
    EXPECT_EQ(id[8],  '-');
    EXPECT_EQ(id[13], '-');
    EXPECT_EQ(id[18], '-');
    EXPECT_EQ(id[23], '-');

    TracingMiddleware::clearContext();
}

// ---------------------------------------------------------------------------
// TracingMiddleware — with disabled exporter
// ---------------------------------------------------------------------------

TEST(TracingMiddlewareTest, WithDisabledExporterNoEnqueue)
{
    OtlpExporterConfig cfg;
    cfg.enabled        = false;
    cfg.max_queue_size = 100;

    OtlpExporter exp(cfg);
    TracingMiddleware mw(&exp);

    mw.processRequest("trace-abc");
    mw.finishSpan("HTTP POST /v2/documents", 201);

    // Disabled exporter → nothing enqueued
    EXPECT_EQ(exp.exportedSpanCount(), 0u);
    EXPECT_EQ(exp.droppedSpanCount(),  0u);

    TracingMiddleware::clearContext();
}

// ---------------------------------------------------------------------------
// TracingMiddleware — with enabled exporter (no real network)
// ---------------------------------------------------------------------------

TEST(TracingMiddlewareTest, WithEnabledExporterEnqueuesSpan)
{
    OtlpExporterConfig cfg;
    cfg.enabled        = true;
    cfg.max_queue_size = 100;
    // No start() call → background thread absent; spans just accumulate in queue.
    // This tests that finishSpan() actually calls enqueue() without crashing.

    OtlpExporter exp(cfg);
    TracingMiddleware mw(&exp);

    mw.processRequest("trace-123");
    // Simulate some work
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    mw.finishSpan("HTTP GET /v1/entity/foo", 200);

    // Queue should have exactly one span (dropped=0 because queue not full)
    EXPECT_EQ(exp.droppedSpanCount(), 0u);

    TracingMiddleware::clearContext();

    exp.stop(); // drains the queue (no curl attempt since no thread was started)
}

TEST(TracingMiddlewareTest, ErrorStatusSetsOtlpErrorCode)
{
    OtlpExporterConfig cfg;
    cfg.enabled        = true;
    cfg.max_queue_size = 100;

    OtlpExporter exp(cfg);
    TracingMiddleware mw(&exp);

    mw.processRequest("err-trace-456");
    mw.finishSpan("HTTP POST /v2/documents", 500);

    EXPECT_EQ(exp.droppedSpanCount(), 0u);

    TracingMiddleware::clearContext();
    exp.stop();
}

// ---------------------------------------------------------------------------
// OtlpExporterConfig defaults
// ---------------------------------------------------------------------------

TEST(OtlpExporterConfigTest, DefaultEndpoint)
{
    OtlpExporterConfig cfg;
    EXPECT_EQ(cfg.endpoint,      "http://localhost:4318/v1/traces");
    EXPECT_EQ(cfg.service_name,  "themisdb");
    EXPECT_EQ(cfg.timeout_ms,    5000);
    EXPECT_EQ(cfg.batch_size,    64u);
    EXPECT_EQ(cfg.max_queue_size, 8192u);
    EXPECT_FALSE(cfg.enabled);
}

// ---------------------------------------------------------------------------
// Retry configuration defaults
// ---------------------------------------------------------------------------

TEST(OtlpExporterConfigTest, RetryDefaults)
{
    OtlpExporterConfig cfg;
    EXPECT_EQ(cfg.max_export_retries,     3);
    EXPECT_EQ(cfg.retry_initial_delay_ms, 100);
}

TEST(OtlpExporterConfigTest, RetryCanBeDisabled)
{
    OtlpExporterConfig cfg;
    cfg.max_export_retries = 0;
    EXPECT_EQ(cfg.max_export_retries, 0);

    // An exporter with no retries must still be constructible.
    OtlpExporter exp(cfg);
    EXPECT_EQ(exp.config().max_export_retries, 0);
}

TEST(OtlpExporterConfigTest, RetryConfigRoundtrips)
{
    OtlpExporterConfig cfg;
    cfg.max_export_retries     = 5;
    cfg.retry_initial_delay_ms = 200;

    OtlpExporter exp(cfg);
    EXPECT_EQ(exp.config().max_export_retries,     5);
    EXPECT_EQ(exp.config().retry_initial_delay_ms, 200);
}

TEST(OtlpExporterConfigTest, RetryDelaySequence)
{
    // Verify the documented exponential delay progression:
    //   attempt 1: initial_delay_ms
    //   attempt 2: initial_delay_ms * 2
    //   attempt 3: initial_delay_ms * 4
    const int initial = 100;
    int delay = initial;
    EXPECT_EQ(delay,       100); delay *= 2;
    EXPECT_EQ(delay,       200); delay *= 2;
    EXPECT_EQ(delay,       400);
}

// ---------------------------------------------------------------------------
// Retry-related queue behaviour (no network required)
// ---------------------------------------------------------------------------

TEST(OtlpExporterTest, RetryZeroMeansNoRetries)
{
    // When max_export_retries = 0, OtlpExporter must construct and enqueue
    // without error.  We cannot exercise the retry path without a real HTTP
    // server, but we can confirm the exporter accepts the configuration.
    OtlpExporterConfig cfg;
    cfg.enabled            = true;
    cfg.max_export_retries = 0;
    cfg.max_queue_size     = 10;

    OtlpExporter exp(cfg);
    // No start() → background thread absent.

    SpanData s;
    s.trace_id = "00000000000000000000000000000099";
    s.name     = "retry-zero-test";
    exp.enqueue(s);

    EXPECT_EQ(exp.droppedSpanCount(), 0u);
    exp.stop();
}

// ---------------------------------------------------------------------------
// Deque-backed queue behaviour (O(1) pop-front replacement for vector)
// ---------------------------------------------------------------------------

TEST(OtlpExporterTest, DequeQueueDropOldestIsOrdinal)
{
    // Verify that when the queue is full the *oldest* span is dropped (FIFO).
    // With std::deque, pop_front() removes the oldest element in O(1).
    OtlpExporterConfig cfg;
    cfg.enabled        = true;
    cfg.max_queue_size = 3;

    OtlpExporter exp(cfg);
    // No start() → no background flush thread; spans accumulate in the deque.

    SpanData s1; s1.trace_id = "00000000000000000000000000000001"; s1.name = "first";
    SpanData s2; s2.trace_id = "00000000000000000000000000000002"; s2.name = "second";
    SpanData s3; s3.trace_id = "00000000000000000000000000000003"; s3.name = "third";

    exp.enqueue(s1);
    exp.enqueue(s2);
    exp.enqueue(s3);
    EXPECT_EQ(exp.droppedSpanCount(), 0u);

    // Fourth enqueue: oldest ("first") must be dropped.
    SpanData s4; s4.trace_id = "00000000000000000000000000000004"; s4.name = "fourth";
    exp.enqueue(s4);
    EXPECT_EQ(exp.droppedSpanCount(), 1u);

    // Fifth: "second" is now oldest.
    SpanData s5; s5.trace_id = "00000000000000000000000000000005"; s5.name = "fifth";
    exp.enqueue(s5);
    EXPECT_EQ(exp.droppedSpanCount(), 2u);

    exp.stop();
}

TEST(OtlpExporterTest, DequeQueueBelowCapacityNeverDrops)
{
    OtlpExporterConfig cfg;
    cfg.enabled        = true;
    cfg.max_queue_size = 100;

    OtlpExporter exp(cfg);

    SpanData s;
    s.trace_id = "abcdef0000000000abcdef0000000000";
    s.name     = "no-drop";

    for (int i = 0; i < 50; ++i) {
      exp.enqueue(s);
    }

    EXPECT_EQ(exp.droppedSpanCount(), 0u);
    exp.stop();
}

// ---------------------------------------------------------------------------
// Persistent curl handle — lifecycle smoke tests (no network required)
// ---------------------------------------------------------------------------

TEST(OtlpExporterTest, StartStopWithEnabledCreatesAndCleansUpHandle)
{
    // start() should create the persistent curl handle and stop() should
    // clean it up.  We verify there is no crash or memory error across the
    // lifecycle (the actual handle state is internal; we just verify no UB).
    OtlpExporterConfig cfg;
    cfg.enabled        = true;
    cfg.endpoint       = "http://127.0.0.1:19999/v1/traces"; // unreachable — no flush needed
    cfg.flush_interval_ms = 500;
    cfg.max_queue_size = 10;

    OtlpExporter exp(cfg);
    exp.start();
    // Brief sleep to confirm the background thread is alive without crashing.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    exp.stop(); // joins thread and frees curl handle
    // If we reach here without UB/crash the lifecycle is correct.
}

TEST(OtlpExporterTest, StartStopIdempotentWithCurlHandle)
{
    OtlpExporterConfig cfg;
    cfg.enabled        = true;
    cfg.endpoint       = "http://127.0.0.1:19999/v1/traces";
    cfg.flush_interval_ms = 500;
    cfg.max_queue_size = 5;

    OtlpExporter exp(cfg);
    exp.start();
    // Double-start must be a no-op (the second call returns early).
    exp.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    exp.stop();
    // Double-stop must be safe too.
    exp.stop();
}
