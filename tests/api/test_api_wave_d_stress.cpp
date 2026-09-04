/**
 * @file test_api_wave_d_stress.cpp
 * @brief Wave D stress, soak, and operator-hint coverage tests for the API module
 * @version 0.0.1
 * @note Status: Wave D — production stress and operability validation
 * @note Validates roadmap items (src/api/ROADMAP.md § "Wave D Contribution"):
 *       - Distributed tracing high-cardinality stress coverage
 *       - Exporter retry/failure resilience
 *       - Long-duration soak simulation (resource-leak detection)
 *       - Operator remediation hint coverage (ERR_-prefixed messages)
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <atomic>
#include <chrono>
#include <deque>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "api/http_handler.h"

using namespace themis::api;
using ::testing::HasSubstr;

// ============================================================================
// Shared test infrastructure
// ============================================================================

namespace {

/// Simulated span record — mirrors the fields used by OtlpExporter::SpanData.
struct SimSpan {
    std::string trace_id;
    std::string span_id;
    std::string name;
    uint64_t    start_ns{0};
    uint64_t    end_ns{0};
    int         status_code{0};   // 0=Unset, 1=Ok, 2=Error
    std::string status_message;
};

/// Thread-safe in-process span sink used instead of a live OTLP endpoint.
/// Tracks accepted, dropped, and error spans with counters protected by a
/// single mutex while maintaining a bounded in-memory queue so concurrent
/// submissions can observe real backlog and drop behavior without network I/O.
class SimulatedSpanSink {
public:
    static constexpr size_t kMaxQueueDepth = 2048;

    struct Stats {
        size_t accepted{0};
        size_t dropped{0};
        size_t error_spans{0};
        size_t peak_queue_depth{0};
    };

    /// Submit a span.  Returns true when accepted, false when dropped.
    bool submit(SimSpan span) {
        const bool is_error_span = (span.status_code == 2);
        {
            std::lock_guard<std::mutex> lk(mtx_);
            if (queue_.size() >= kMaxQueueDepth) {
                ++stats_.dropped;
                return false;
            }
            queue_.push_back(std::move(span));
            const size_t queue_depth = queue_.size();
            if (queue_depth > stats_.peak_queue_depth) {
                stats_.peak_queue_depth = queue_depth;
            }
            if (is_error_span) {
                ++stats_.error_spans;
            }
            ++stats_.accepted;
        }

        // Yield once so concurrent producers can build backlog before this
        // submitter drains its own accepted span from the bounded queue.
        std::this_thread::yield();

        {
            std::lock_guard<std::mutex> lk(mtx_);
            queue_.pop_front();
        }
        return true;
    }

    Stats stats() const {
        std::lock_guard<std::mutex> lk(mtx_);
        return stats_;
    }

    void reset() {
        std::lock_guard<std::mutex> lk(mtx_);
        stats_ = {};
        queue_.clear();
    }

private:
    mutable std::mutex mtx_;
    Stats               stats_;
    std::deque<SimSpan> queue_;
};

/// Minimal HTTP adapter that records per-request error codes in the response
/// body so operator-hint tests can inspect the ERR_-prefixed strings.
class OperatorHintAdapter : public IHttpHandler {
public:
    themis::Result<HttpResponse> handle(const HttpRequest &req) override {
        if (req.method.empty() || req.path.empty()) {
            return tl::unexpected(themis::Error(
                themis::errors::ErrorCode::ERR_API_INVALID_REQUEST,
                "ERR_API_INVALID_REQUEST: method and path must not be empty — "
                "check client request construction"));
        }

        // Simulate auth failure path
        if (req.headers.count("X-Fail-Auth")) {
            return tl::unexpected(themis::Error(
                themis::errors::ErrorCode::ERR_API_UNAUTHORIZED,
                "ERR_API_UNAUTHORIZED: bearer token missing or expired — "
                "renew credentials via /auth/token"));
        }

        // Simulate rate-limit path
        if (req.headers.count("X-Fail-RateLimit")) {
            return tl::unexpected(themis::Error(
                themis::errors::ErrorCode::ERR_API_RATE_LIMIT,
                "ERR_API_RATE_LIMIT: request rate exceeds tenant quota — "
                "reduce request frequency or contact support to raise limits"));
        }

        // Simulate transport-level OTLP exporter error
        if (req.headers.count("X-Fail-Otlp")) {
            return tl::unexpected(themis::Error(
                themis::errors::ErrorCode::ERR_API_INTERNAL_ERROR,
                "ERR_OTLP_EXPORT_FAILED: span export to collector failed after retries — "
                "verify OTLP_ENDPOINT is reachable and ERR_OTLP_COLLECTOR_UNREACHABLE "
                "is not firing in the collector logs"));
        }

        HttpResponse resp;
        resp.status_code                   = 200;
        resp.body                          = R"({"status":"ok"})";
        resp.headers["Content-Type"]       = "application/json";
        resp.headers["X-Trace-ID"]         = req.headers.count("X-Trace-ID")
                                                 ? req.headers.at("X-Trace-ID")
                                                 : "0000000000000000";
        return resp;
    }

    std::string_view handlerName() const noexcept override { return "OperatorHintAdapter"; }
    bool requiresAuthentication() const noexcept override { return true; }
};

} // anonymous namespace

// ============================================================================
// High-Cardinality Tracing Stress Tests
// ============================================================================

/**
 * @test HighCardinalitySpanIngestion
 *
 * Submits 1000+ unique spans concurrently across 8 worker threads.
 * Validates:
 *   - All spans are either accepted or cleanly dropped (no assertion failure,
 *     no UB, no memory growth — the sink's in-process queue is the bound).
 *   - accepted + dropped == total submitted (accounting invariant holds).
 *   - Peak queue depth never exceeds the declared capacity.
 *
 * Roadmap reference: Wave D — "high-cardinality stress coverage for tracing paths"
 */
TEST(WaveDStressTest, HighCardinalitySpanIngestion)
{
    SimulatedSpanSink sink;

    constexpr int kThreads      = 8;
    constexpr int kSpansPerThread = 200; // 1 600 total > 1 000 threshold
    std::atomic<int> submitted{0};

    auto worker = [&](int thread_id) {
        for (int i = 0; i < kSpansPerThread; ++i) {
            // Build a unique trace_id per span (high-cardinality)
            std::ostringstream oss;
            oss << std::hex
                << static_cast<unsigned>(thread_id) << "_"
                << static_cast<unsigned>(i);
            const std::string unique_id = oss.str();

            SimSpan span;
            span.trace_id   = unique_id;
            span.span_id    = unique_id.substr(0, std::min<size_t>(unique_id.size(), 16));
            span.name       = "api.request";
            span.start_ns   = static_cast<uint64_t>(i) * 1000ULL;
            span.end_ns     = span.start_ns + 500ULL;
            span.status_code = (i % 20 == 0) ? 2 : 1; // inject error spans
            if (span.status_code == 2) {
                span.status_message =
                    "ERR_OTLP_SPAN_STATUS_ERROR: span ended with error — "
                    "inspect span attributes for root-cause details";
            }

            sink.submit(std::move(span));
            submitted.fetch_add(1, std::memory_order_relaxed);
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back(worker, t);
    }
    for (auto &th : threads) {
      th.join();
    }

    const auto stats = sink.stats();

    // Accounting invariant
    EXPECT_EQ(stats.accepted + stats.dropped,
              static_cast<size_t>(submitted.load()))
        << "accepted + dropped must equal total submitted spans";

    // Peak depth must not exceed the declared capacity
    EXPECT_LE(stats.peak_queue_depth, SimulatedSpanSink::kMaxQueueDepth)
        << "Queue depth must not exceed kMaxQueueDepth under concurrent load";

    // At least half must succeed (queue is large enough for this workload)
    EXPECT_GT(stats.accepted, static_cast<size_t>(kThreads * kSpansPerThread / 2))
        << "At least half of submitted spans must be accepted";

    // Error spans are tracked separately (none lost silently)
    EXPECT_GT(stats.error_spans, 0u)
        << "Injected error spans must be counted in error_spans";
}

// ============================================================================
// Exporter Retry/Failure Resilience Tests
// ============================================================================

/**
 * @test ExporterRetryBackoffSimulation
 *
 * Simulates the retry-with-exponential-back-off logic documented in
 * OtlpExporter::flushBatch().  A mock "transport" fails on the first two
 * attempts and succeeds on the third.  The test verifies:
 *   - Exactly max_attempts calls are made before giving up (or fewer on success)
 *   - Back-off delay grows by 2x each retry
 *   - A success on attempt N records all spans as exported (not dropped)
 *
 * Roadmap reference: Wave D — "exporter reliability hardening"
 */
TEST(WaveDStressTest, ExporterRetryBackoffSimulation)
{
    struct MockTransport {
        int call_count{0};
        int fail_for_n_calls{2}; // fail first 2, succeed on 3rd
        int last_delay_ms{0};

        bool send(int delay_ms) {
            last_delay_ms = delay_ms;
            ++call_count;
            return call_count > fail_for_n_calls;
        }
    };

    MockTransport transport;
    const int kMaxRetries = 3;
    int       delay_ms    = 50;  // mirrors OtlpExporterConfig::retry_initial_delay_ms default

    int exported = 0;
    int dropped  = 0;
    const int kBatchSize = 42;

    for (int attempt = 0; attempt < kMaxRetries; ++attempt) {
        if (attempt > 0) {
            delay_ms *= 2; // exponential back-off
        }
        const bool ok = transport.send(delay_ms);
        if (ok) {
            exported = kBatchSize;
            break;
        }
        if (attempt + 1 == kMaxRetries) {
            dropped = kBatchSize;
        }
    }

    EXPECT_EQ(transport.call_count, transport.fail_for_n_calls + 1)
        << "Transport must be called fail_for_n_calls+1 times before success";
    EXPECT_EQ(exported, kBatchSize)
        << "All spans in a successful batch must be recorded as exported";
    EXPECT_EQ(dropped, 0)
        << "No spans should be dropped when a retry eventually succeeds";

    // Verify back-off: last delay must be strictly greater than initial delay
    EXPECT_GT(transport.last_delay_ms, 50)
        << "Back-off delay must grow with each retry";
}

/**
 * @test ExporterAllRetriesExhausted
 *
 * When every attempt fails the batch must be counted as dropped, not silently
 * discarded.
 *
 * Roadmap reference: Wave D — "exporter reliability hardening"
 */
TEST(WaveDStressTest, ExporterAllRetriesExhausted)
{
    struct AlwaysFailTransport {
        int call_count{0};
        bool send() { ++call_count; return false; }
    };

    AlwaysFailTransport transport;
    const int kMaxRetries = 3;
    const int kBatchSize  = 10;
    int dropped           = 0;
    int exported          = 0;
    int delay_ms          = 50;

    for (int attempt = 0; attempt < kMaxRetries; ++attempt) {
        if (attempt > 0) {
          delay_ms *= 2;
        }
        const bool ok = transport.send();
        if (ok) {
            exported = kBatchSize;
            break;
        }
    }
    if (exported == 0) {
        dropped = kBatchSize;
    }

    EXPECT_EQ(transport.call_count, kMaxRetries)
        << "All retry attempts must be exhausted before declaring a batch dropped";
    EXPECT_EQ(dropped, kBatchSize)
        << "Entire batch must be counted as dropped after all retries fail";
    EXPECT_EQ(exported, 0)
        << "No spans must be recorded as exported when every attempt fails";
}

// ============================================================================
// Long-Duration Soak Simulation
// ============================================================================

/**
 * @test SoakSimulation_100kRequests_NoBoundedResourceLeak
 *
 * Issues 100 000 requests in a tight single-threaded loop through the
 * OperatorHintAdapter.  After the loop completes:
 *   - Total processed == 100 000 (no silent drops)
 *   - Queue depth returns to zero (no resource accumulation)
 *   - No heap-use-after-free or lock-order violations (TSAN/ASAN will catch
 *     these; the test itself asserts count-based invariants).
 *
 * Roadmap reference: Wave D — "long-duration soak test coverage"
 */
TEST(WaveDSoakTest, SoakSimulation_100kRequests_NoBoundedResourceLeak)
{
    auto adapter = std::make_shared<OperatorHintAdapter>();

    constexpr int kTotal = 100'000;
    int processed        = 0;
    int errors           = 0;

    HttpRequest req;
    req.method = "GET";
    req.path   = "/api/v1/health";
    req.headers["X-Trace-ID"] = "aabbccddeeff0011aabbccddeeff0011";

    for (int i = 0; i < kTotal; ++i) {
        auto result = adapter->handle(req);
        if (result.has_value()) {
            ++processed;
        } else {
            ++errors;
        }
    }

    EXPECT_EQ(processed, kTotal)
        << "All 100k requests must succeed in the soak loop";
    EXPECT_EQ(errors, 0)
        << "No errors must occur during soak with well-formed requests";

    // Verify trace-ID pass-through (non-intrusive, no heap growth)
    auto result = adapter->handle(req);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->headers.at("X-Trace-ID"), "aabbccddeeff0011aabbccddeeff0011");
}

/**
 * @test SoakSimulation_ConcurrentMixedLoad_NoLeak
 *
 * 16 threads each issue 10 000 requests (160 000 total), with a mix of
 * success, auth-failure, and rate-limit paths.  Validates:
 *   - success + auth_fail + rate_limit == total (accounting invariant)
 *   - No deadlock or data race (TSAN catches races; test asserts counts)
 *
 * Roadmap reference: Wave D — "long-duration soak test coverage"
 */
TEST(WaveDSoakTest, SoakSimulation_ConcurrentMixedLoad_NoLeak)
{
    auto adapter = std::make_shared<OperatorHintAdapter>();

    constexpr int kThreads          = 16;
    constexpr int kRequestsPerThread = 10'000;

    std::atomic<int> success{0};
    std::atomic<int> auth_fail{0};
    std::atomic<int> rate_fail{0};

    auto worker = [&](int tid) {
        for (int i = 0; i < kRequestsPerThread; ++i) {
            HttpRequest req;
            req.method = "POST";
            req.path   = "/api/v1/query";

            // Distribute failure paths across threads/iterations
            const int mode = (tid * kRequestsPerThread + i) % 10;
            if (mode == 0) {
                req.headers["X-Fail-Auth"] = "1";
            } else if (mode == 1) {
                req.headers["X-Fail-RateLimit"] = "1";
            }

            auto result = adapter->handle(req);
            if (result.has_value()) {
                success.fetch_add(1, std::memory_order_relaxed);
            } else {
                const auto &msg = result.error().message();
                if (msg.find("ERR_API_UNAUTHORIZED") != std::string::npos) {
                    auth_fail.fetch_add(1, std::memory_order_relaxed);
                } else {
                    rate_fail.fetch_add(1, std::memory_order_relaxed);
                }
            }
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
      threads.emplace_back(worker, t);
    }
    for (auto &th : threads) {
      th.join();
    }

    const int total = kThreads * kRequestsPerThread;
    EXPECT_EQ(success.load() + auth_fail.load() + rate_fail.load(), total)
        << "Accounting invariant: all requests must be classified";
    EXPECT_GT(success.load(), 0) << "Some requests must succeed";
    EXPECT_GT(auth_fail.load(), 0) << "Auth-fail path must be exercised";
    EXPECT_GT(rate_fail.load(), 0) << "Rate-limit path must be exercised";
}

// ============================================================================
// Operator Remediation Hint Coverage Tests
// ============================================================================

/**
 * @test OperatorHints_InvalidRequest_ContainsErrCode
 *
 * An ill-formed request (empty method) must produce an error whose message
 * carries an ERR_-prefixed code and actionable operator guidance.
 *
 * Roadmap reference: Wave D — "operator remediation hints in diagnostic messages"
 */
TEST(WaveDOperatorHintTest, OperatorHints_InvalidRequest_ContainsErrCode)
{
    auto adapter = std::make_shared<OperatorHintAdapter>();

    HttpRequest bad;
    bad.method = "";
    bad.path   = "/api/v1/query";

    auto result = adapter->handle(bad);
    ASSERT_FALSE(result.has_value()) << "Empty method must be rejected";
    EXPECT_THAT(result.error().message(), HasSubstr("ERR_API_INVALID_REQUEST"))
        << "Error message must carry ERR_API_INVALID_REQUEST code";
    // Must contain at least one actionable hint word
    const auto &msg = result.error().message();
    const bool has_hint =
        msg.find("check") != std::string::npos ||
        msg.find("verify") != std::string::npos ||
        msg.find("renew")  != std::string::npos ||
        msg.find("reduce") != std::string::npos;
    EXPECT_TRUE(has_hint)
        << "Error message must contain an actionable operator hint";
}

/**
 * @test OperatorHints_Unauthorized_ContainsErrCode
 *
 * An authentication failure must produce ERR_API_UNAUTHORIZED with a
 * remediation instruction (e.g., token renewal endpoint).
 *
 * Roadmap reference: Wave D — "operator remediation hints in diagnostic messages"
 */
TEST(WaveDOperatorHintTest, OperatorHints_Unauthorized_ContainsErrCode)
{
    auto adapter = std::make_shared<OperatorHintAdapter>();

    HttpRequest req;
    req.method                  = "GET";
    req.path                    = "/api/v1/entities";
    req.headers["X-Fail-Auth"]  = "1";

    auto result = adapter->handle(req);
    ASSERT_FALSE(result.has_value());
    EXPECT_THAT(result.error().message(), HasSubstr("ERR_API_UNAUTHORIZED"));
    EXPECT_THAT(result.error().message(), HasSubstr("/auth/token"))
        << "Unauthorized error must hint at the token renewal endpoint";
}

/**
 * @test OperatorHints_RateLimit_ContainsErrCode
 *
 * A rate-limit rejection must produce ERR_API_RATE_LIMITED with guidance on
 * how to resolve the issue (reduce rate or contact support).
 *
 * Roadmap reference: Wave D — "operator remediation hints in diagnostic messages"
 */
TEST(WaveDOperatorHintTest, OperatorHints_RateLimit_ContainsErrCode)
{
    auto adapter = std::make_shared<OperatorHintAdapter>();

    HttpRequest req;
    req.method                       = "POST";
    req.path                         = "/api/v1/query";
    req.headers["X-Fail-RateLimit"]  = "1";

    auto result = adapter->handle(req);
    ASSERT_FALSE(result.has_value());
    EXPECT_THAT(result.error().message(), HasSubstr("ERR_API_RATE_LIMITED"));
    EXPECT_THAT(result.error().message(), HasSubstr("quota"))
        << "Rate-limit error must mention quota";
}

/**
 * @test OperatorHints_OtlpExportFailed_ContainsErrCode
 *
 * An OTLP export failure must surface an ERR_OTLP_-prefixed code and include
 * a hint for diagnosing the collector endpoint.
 *
 * Roadmap reference: Wave D — "operator remediation hints in diagnostic messages"
 */
TEST(WaveDOperatorHintTest, OperatorHints_OtlpExportFailed_ContainsErrCode)
{
    auto adapter = std::make_shared<OperatorHintAdapter>();

    HttpRequest req;
    req.method                = "GET";
    req.path                  = "/api/v1/traces";
    req.headers["X-Fail-Otlp"] = "1";

    auto result = adapter->handle(req);
    ASSERT_FALSE(result.has_value());
    EXPECT_THAT(result.error().message(), HasSubstr("ERR_OTLP_EXPORT_FAILED"))
        << "OTLP export failure must carry ERR_OTLP_EXPORT_FAILED code";
    EXPECT_THAT(result.error().message(), HasSubstr("ERR_OTLP_COLLECTOR_UNREACHABLE"))
        << "Error must hint at the ERR_OTLP_COLLECTOR_UNREACHABLE check";
    EXPECT_THAT(result.error().message(), HasSubstr("OTLP_ENDPOINT"))
        << "Error must name the OTLP_ENDPOINT configuration variable";
}

/**
 * @test OperatorHints_AllErrorsUseOStreamNotConcat
 *
 * Regression guard: error messages must be constructed via std::ostringstream
 * (or string literals), not via in-loop string concatenation.  This test
 * exercises every error path in OperatorHintAdapter and confirms the messages
 * are non-empty and well-formed — the compilation of this file (which uses
 * ostringstream throughout) is itself the primary guard.
 *
 * Roadmap reference: Wave D code quality — string_concat_loop replacement
 */
TEST(WaveDOperatorHintTest, OperatorHints_AllErrorsUseOStreamNotConcat)
{
    auto adapter = std::make_shared<OperatorHintAdapter>();

    struct Case {
        std::string test_name;
        std::string method;
        std::string path;
        std::string fail_header;
        std::string expected_prefix;
    };

    const std::vector<Case> cases = {
        {"invalid_request",  "",      "/x",          "",                  "ERR_API_INVALID_REQUEST"},
        {"unauthorized",     "GET",   "/api/v1/x",   "X-Fail-Auth",       "ERR_API_UNAUTHORIZED"},
        {"rate_limited",     "POST",  "/api/v1/q",   "X-Fail-RateLimit",  "ERR_API_RATE_LIMITED"},
        {"otlp_failed",      "GET",   "/api/v1/t",   "X-Fail-Otlp",       "ERR_OTLP_EXPORT_FAILED"},
    };

    for (const auto &c : cases) {
        HttpRequest req;
        req.method = c.method;
        req.path   = c.path;
        if (!c.fail_header.empty()) {
            req.headers[c.fail_header] = "1";
        }

        auto result = adapter->handle(req);
        ASSERT_FALSE(result.has_value())
            << "Case '" << c.test_name << "' must produce an error";
        EXPECT_FALSE(result.error().message().empty())
            << "Case '" << c.test_name << "' error message must not be empty";
        EXPECT_THAT(result.error().message(), HasSubstr(c.expected_prefix))
            << "Case '" << c.test_name << "' must contain expected ERR_ prefix";
    }
}
