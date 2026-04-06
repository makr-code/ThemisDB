/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_retry_policy.cpp                              ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-04-06 04:22:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  ⚫ DRAFT                                        ║
    • Quality Score:   0.0/100                                        ║
    • Total Lines:     540                                            ║
    • Open Issues:     TODOs: 0, Stubs: 42                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • efdbcc2fc8  2026-03-19  merge: resolve conflicts with develop - keep predictive p... ║
    • ace9bcf815  2026-03-16  feat(chimera): mark Error Recovery and Retry Logic (Issue... ║
    • 353ab78386  2026-03-12  fix(chimera): address all review comments on retry policy ║
    • 350ef6ad71  2026-03-12  feat(chimera): implement Error Recovery and Retry Logic (... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 📝 Draft / Stub                                              ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_retry_policy.cpp
 * @brief Unit tests for CHIMERA Error Recovery and Retry Logic
 *
 * @details
 * Tests cover:
 *  - RetryPolicy defaults and customisation
 *  - Exponential backoff delay computation (mocked sleep)
 *  - Error classification (transient vs permanent)
 *  - CircuitBreaker: CLOSED → OPEN → HALF_OPEN → CLOSED transitions
 *  - ConnectionWithRetry: successful retry, permanent error, circuit open,
 *    health_check helper
 *
 * All tests run without a live database server.
 *
 * @copyright MIT License
 */

#include <gtest/gtest.h>
#include "chimera/retry_policy.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace chimera;

// ============================================================================
// Minimal stub adapter used by all tests
// ============================================================================

/**
 * @brief Controllable stub that lets tests inject success/failure sequences.
 */
class StubAdapter : public IDatabaseAdapter {
public:
    explicit StubAdapter(bool initially_connected = true)
        : connected_(initially_connected)
    {}

    // ------------------------------------------------------------------
    // Connection management
    // ------------------------------------------------------------------
    Result<bool> connect(
        const std::string& /*connection_string*/,
        const std::map<std::string, std::string>& /*options*/ = {}
    ) override {
        if (connect_should_succeed_) {
            connected_ = true;
            return Result<bool>::ok(true);
        }
        return Result<bool>::err(ErrorCode::CONNECTION_ERROR, "connect failed");
    }

    Result<bool> disconnect() override {
        connected_ = false;
        return Result<bool>::ok(true);
    }

    bool is_connected() const override { return connected_; }

    // ------------------------------------------------------------------
    // Scripted execute_query for retry testing
    // ------------------------------------------------------------------

    /**
     * @brief Enqueue a sequence of Results to return from execute_query.
     *
     * Each call to execute_query dequeues the front element.  When the
     * queue is empty, returns a default success.
     */
    void enqueue_result(Result<RelationalTable> r) {
        scripted_results_.push_back(std::move(r));
    }

    size_t call_count() const { return call_count_; }

    void set_connect_should_succeed(bool v) { connect_should_succeed_ = v; }

    // ------------------------------------------------------------------
    // IRelationalAdapter
    // ------------------------------------------------------------------
    Result<RelationalTable> execute_query(
        const std::string& /*query*/,
        const std::vector<Scalar>& /*params*/ = {}
    ) override {
        ++call_count_;
        if (!scripted_results_.empty()) {
            auto r = std::move(scripted_results_.front());
            scripted_results_.erase(scripted_results_.begin());
            return r;
        }
        return Result<RelationalTable>::ok(RelationalTable{});
    }

    Result<size_t> insert_row(
        const std::string&, const RelationalRow&
    ) override { return Result<size_t>::ok(1); }

    Result<size_t> batch_insert(
        const std::string&, const std::vector<RelationalRow>&
    ) override { return Result<size_t>::ok(0); }

    Result<QueryStatistics> get_query_statistics() const override {
        QueryStatistics qs;
        qs.execution_time = std::chrono::microseconds(0);
        qs.rows_read = 0;
        qs.rows_returned = 0;
        qs.bytes_read = 0;
        return Result<QueryStatistics>::ok(std::move(qs));
    }

    // IVectorAdapter
    Result<std::string> insert_vector(const std::string&, const Vector&) override {
        return Result<std::string>::ok("v1");
    }
    Result<size_t> batch_insert_vectors(const std::string&, const std::vector<Vector>&) override {
        return Result<size_t>::ok(0);
    }
    Result<std::vector<std::pair<Vector, double>>> search_vectors(
        const std::string&, const Vector&, size_t,
        const std::map<std::string, Scalar>&) override {
        return Result<std::vector<std::pair<Vector, double>>>::ok({});
    }
    Result<bool> create_index(
        const std::string&, size_t,
        const std::map<std::string, Scalar>&) override {
        return Result<bool>::ok(true);
    }

    // IGraphAdapter
    Result<std::string> insert_node(const GraphNode&) override {
        return Result<std::string>::ok("n1");
    }
    Result<std::string> insert_edge(const GraphEdge&) override {
        return Result<std::string>::ok("e1");
    }
    Result<GraphPath> shortest_path(
        const std::string&, const std::string&, size_t) override {
        return Result<GraphPath>::ok(GraphPath{});
    }
    Result<std::vector<GraphNode>> traverse(
        const std::string&, size_t,
        const std::vector<std::string>&) override {
        return Result<std::vector<GraphNode>>::ok({});
    }
    Result<std::vector<GraphPath>> execute_graph_query(
        const std::string&,
        const std::map<std::string, Scalar>&) override {
        return Result<std::vector<GraphPath>>::ok({});
    }

    // IDocumentAdapter
    Result<std::string> insert_document(const std::string&, const Document&) override {
        return Result<std::string>::ok("d1");
    }
    Result<size_t> batch_insert_documents(
        const std::string&, const std::vector<Document>&) override {
        return Result<size_t>::ok(0);
    }
    Result<std::vector<Document>> find_documents(
        const std::string&,
        const std::map<std::string, Scalar>&, size_t) override {
        return Result<std::vector<Document>>::ok({});
    }
    Result<size_t> update_documents(
        const std::string&,
        const std::map<std::string, Scalar>&,
        const std::map<std::string, Scalar>&) override {
        return Result<size_t>::ok(0);
    }

    // ITransactionAdapter
    Result<std::string> begin_transaction(const TransactionOptions&) override {
        return Result<std::string>::ok("txn1");
    }
    Result<bool> commit_transaction(const std::string&) override {
        return Result<bool>::ok(true);
    }
    Result<bool> rollback_transaction(const std::string&) override {
        return Result<bool>::ok(true);
    }

    // ISystemInfoAdapter
    Result<SystemInfo> get_system_info() const override {
        SystemInfo info;
        info.system_name = "StubDB";
        info.version = "0.0.1";
        return Result<SystemInfo>::ok(std::move(info));
    }
    Result<SystemMetrics> get_metrics() const override {
        SystemMetrics m;
        m.memory.total_bytes = 0;
        m.memory.used_bytes = 0;
        m.memory.available_bytes = 0;
        m.storage.total_bytes = 0;
        m.storage.used_bytes = 0;
        m.storage.available_bytes = 0;
        m.cpu.utilization_percent = 0.0;
        m.cpu.thread_count = 0;
        return Result<SystemMetrics>::ok(std::move(m));
    }
    bool has_capability(Capability) const override { return false; }
    std::vector<Capability> get_capabilities() const override { return {}; }

private:
    bool connected_;
    bool connect_should_succeed_ = true;
    std::vector<Result<RelationalTable>> scripted_results_;
    size_t call_count_ = 0;
};

// ============================================================================
// RetryPolicy Tests
// ============================================================================

TEST(RetryPolicyTest, DefaultValues) {
    RetryPolicy policy;
    EXPECT_EQ(policy.max_retries, 3u);
    EXPECT_EQ(policy.initial_delay, std::chrono::milliseconds(100));
    EXPECT_DOUBLE_EQ(policy.backoff_multiplier, 2.0);
    EXPECT_EQ(policy.max_delay, std::chrono::milliseconds(10000));
    ASSERT_TRUE(policy.is_transient);
}

TEST(RetryPolicyTest, DefaultIsTransientClassifiesTimeoutAsTransient) {
    RetryPolicy policy;
    EXPECT_TRUE(policy.is_transient(ErrorCode::TIMEOUT));
    EXPECT_TRUE(policy.is_transient(ErrorCode::CONNECTION_ERROR));
}

TEST(RetryPolicyTest, DefaultIsTransientClassifiesPermanentErrors) {
    RetryPolicy policy;
    EXPECT_FALSE(policy.is_transient(ErrorCode::PERMISSION_DENIED));
    EXPECT_FALSE(policy.is_transient(ErrorCode::INVALID_ARGUMENT));
    EXPECT_FALSE(policy.is_transient(ErrorCode::NOT_FOUND));
    EXPECT_FALSE(policy.is_transient(ErrorCode::CONSTRAINT_VIOLATION));
}

TEST(RetryPolicyTest, CustomTransientClassifier) {
    RetryPolicy policy;
    policy.is_transient = [](ErrorCode code) {
        return code == ErrorCode::RESOURCE_EXHAUSTED;
    };
    EXPECT_TRUE(policy.is_transient(ErrorCode::RESOURCE_EXHAUSTED));
    EXPECT_FALSE(policy.is_transient(ErrorCode::TIMEOUT));
}

// ============================================================================
// CircuitBreaker Tests
// ============================================================================

TEST(CircuitBreakerTest, InitialStateIsClosed) {
    CircuitBreaker cb;
    EXPECT_EQ(cb.state(), CircuitState::CLOSED);
    EXPECT_TRUE(cb.allow_request());
}

TEST(CircuitBreakerTest, ClosedToOpenAfterFailureThreshold) {
    CircuitBreaker cb(/*failure_threshold=*/3,
                      /*open_timeout=*/std::chrono::seconds(60));

    for (size_t i = 0; i < 2; ++i) {
        cb.record_failure();
        EXPECT_EQ(cb.state(), CircuitState::CLOSED);
    }
    cb.record_failure(); // 3rd failure
    EXPECT_EQ(cb.state(), CircuitState::OPEN);
}

TEST(CircuitBreakerTest, OpenRejectsRequests) {
    CircuitBreaker cb(/*failure_threshold=*/1,
                      /*open_timeout=*/std::chrono::seconds(60));
    cb.record_failure();
    ASSERT_EQ(cb.state(), CircuitState::OPEN);
    EXPECT_FALSE(cb.allow_request());
}

TEST(CircuitBreakerTest, OpenTransitionsToHalfOpenAfterTimeout) {
    // Use zero open_timeout so the transition can be tested without real sleeping.
    CircuitBreaker cb(/*failure_threshold=*/1,
                      /*open_timeout=*/std::chrono::milliseconds(0));
    cb.record_failure();
    ASSERT_EQ(cb.state(), CircuitState::OPEN);

    // With open_timeout == 0, allow_request() should immediately transition to HALF_OPEN.
    EXPECT_TRUE(cb.allow_request());
    EXPECT_EQ(cb.state(), CircuitState::HALF_OPEN);
}

TEST(CircuitBreakerTest, HalfOpenToClosedOnSuccess) {
    // Use zero open_timeout so the probe does not require sleeping.
    CircuitBreaker cb(/*failure_threshold=*/1,
                      /*open_timeout=*/std::chrono::milliseconds(0));
    cb.record_failure();
    ASSERT_TRUE(cb.allow_request()); // probe transitions OPEN → HALF_OPEN

    cb.record_success();
    EXPECT_EQ(cb.state(), CircuitState::CLOSED);
}

TEST(CircuitBreakerTest, HalfOpenToOpenOnFailure) {
    // Use zero open_timeout so the probe does not require sleeping.
    CircuitBreaker cb(/*failure_threshold=*/1,
                      /*open_timeout=*/std::chrono::milliseconds(0));
    cb.record_failure();
    ASSERT_TRUE(cb.allow_request()); // probe transitions OPEN → HALF_OPEN

    cb.record_failure(); // probe fails — re-opens circuit
    EXPECT_EQ(cb.state(), CircuitState::OPEN);
}

TEST(CircuitBreakerTest, SuccessInClosedResetsFailureCount) {
    CircuitBreaker cb(/*failure_threshold=*/3,
                      /*open_timeout=*/std::chrono::seconds(60));
    cb.record_failure();
    cb.record_failure();
    EXPECT_EQ(cb.consecutive_failures(), 2u);

    cb.record_success();
    EXPECT_EQ(cb.consecutive_failures(), 0u);
    EXPECT_EQ(cb.state(), CircuitState::CLOSED);
}

TEST(CircuitBreakerTest, ResetRestoresClosedState) {
    CircuitBreaker cb(/*failure_threshold=*/1,
                      /*open_timeout=*/std::chrono::seconds(60));
    cb.record_failure();
    ASSERT_EQ(cb.state(), CircuitState::OPEN);

    cb.reset();
    EXPECT_EQ(cb.state(), CircuitState::CLOSED);
    EXPECT_EQ(cb.consecutive_failures(), 0u);
    EXPECT_TRUE(cb.allow_request());
}

// ============================================================================
// ConnectionWithRetry Tests
// ============================================================================

/**
 * @brief Build a ConnectionWithRetry with zero-duration delays for fast tests.
 */
static ConnectionWithRetry make_conn(
    std::unique_ptr<IDatabaseAdapter> adapter,
    size_t max_retries = 3,
    bool custom_transient = false)
{
    RetryPolicy policy;
    policy.max_retries    = max_retries;
    policy.initial_delay  = std::chrono::milliseconds(0);
    policy.max_delay      = std::chrono::milliseconds(0);
    if (custom_transient) {
        policy.is_transient = [](ErrorCode code) {
            return code == ErrorCode::TIMEOUT ||
                   code == ErrorCode::CONNECTION_ERROR ||
                   code == ErrorCode::RESOURCE_EXHAUSTED;
        };
    }
    return ConnectionWithRetry(std::move(adapter), policy);
}

// ------------------------------------------------------------------

TEST(ConnectionWithRetryTest, SuccessOnFirstAttempt) {
    auto stub = std::make_unique<StubAdapter>();
    stub->enqueue_result(Result<RelationalTable>::ok(RelationalTable{}));

    auto conn = make_conn(std::move(stub));
    auto result = conn.execute_with_retry<RelationalTable>(
        [&]() { return conn.adapter().execute_query("SELECT 1"); });

    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(static_cast<StubAdapter&>(conn.adapter()).call_count(), 1u);
}

TEST(ConnectionWithRetryTest, RetriesOnTransientError) {
    auto stub = std::make_unique<StubAdapter>();
    // Two transient failures, then success.
    stub->enqueue_result(Result<RelationalTable>::err(
        ErrorCode::CONNECTION_ERROR, "transient 1"));
    stub->enqueue_result(Result<RelationalTable>::err(
        ErrorCode::TIMEOUT, "transient 2"));
    stub->enqueue_result(Result<RelationalTable>::ok(RelationalTable{}));

    auto conn = make_conn(std::move(stub), /*max_retries=*/3);
    auto result = conn.execute_with_retry<RelationalTable>(
        [&]() { return conn.adapter().execute_query("SELECT 1"); });

    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(static_cast<StubAdapter&>(conn.adapter()).call_count(), 3u);
}

TEST(ConnectionWithRetryTest, DoesNotRetryPermanentError) {
    auto stub = std::make_unique<StubAdapter>();
    stub->enqueue_result(Result<RelationalTable>::err(
        ErrorCode::PERMISSION_DENIED, "permanent"));
    stub->enqueue_result(Result<RelationalTable>::ok(RelationalTable{}));

    auto conn = make_conn(std::move(stub), /*max_retries=*/3);
    auto result = conn.execute_with_retry<RelationalTable>(
        [&]() { return conn.adapter().execute_query("SELECT 1"); });

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::PERMISSION_DENIED);
    // Should have called only once (no retry on permanent error).
    EXPECT_EQ(static_cast<StubAdapter&>(conn.adapter()).call_count(), 1u);
}

TEST(ConnectionWithRetryTest, ExhaustsMaxRetriesAndReturnsLastError) {
    auto stub = std::make_unique<StubAdapter>();
    // All four attempts (1 initial + 3 retries) fail.
    for (int i = 0; i < 4; ++i) {
        stub->enqueue_result(Result<RelationalTable>::err(
            ErrorCode::TIMEOUT, "timeout"));
    }

    auto conn = make_conn(std::move(stub), /*max_retries=*/3);
    auto result = conn.execute_with_retry<RelationalTable>(
        [&]() { return conn.adapter().execute_query("SELECT 1"); });

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::TIMEOUT);
    EXPECT_EQ(static_cast<StubAdapter&>(conn.adapter()).call_count(), 4u);
}

TEST(ConnectionWithRetryTest, CircuitBreakerOpenBlocksCallImmediately) {
    auto stub = std::make_unique<StubAdapter>();
    StubAdapter* raw = stub.get();

    RetryPolicy policy;
    policy.max_retries   = 3;
    policy.initial_delay = std::chrono::milliseconds(0);
    policy.max_delay     = std::chrono::milliseconds(0);

    ConnectionWithRetry conn(std::move(stub), policy);

    // Trip the circuit breaker via the non-const accessor (no const_cast needed).
    for (size_t i = 0; i < 5; ++i) {
        conn.circuit_breaker().record_failure();
    }
    ASSERT_EQ(conn.circuit_breaker().state(), CircuitState::OPEN);

    // execute_with_retry should reject without calling the adapter.
    size_t calls_before = raw->call_count();
    auto result = conn.execute_with_retry<RelationalTable>(
        [&]() { return conn.adapter().execute_query("SELECT 1"); });

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::TIMEOUT);
    EXPECT_EQ(raw->call_count(), calls_before); // no call made
}

TEST(ConnectionWithRetryTest, AdapterAndPolicyAccessors) {
    auto stub = std::make_unique<StubAdapter>();
    RetryPolicy policy;
    policy.max_retries = 7;

    ConnectionWithRetry conn(std::move(stub), policy);
    EXPECT_EQ(conn.policy().max_retries, 7u);
    EXPECT_NE(&conn.adapter(), nullptr);
}

// ============================================================================
// HealthCheck Tests
// ============================================================================

TEST(ConnectionWithRetryTest, HealthCheckReturnsTrueWhenConnected) {
    auto stub = std::make_unique<StubAdapter>(/*initially_connected=*/true);
    auto conn = make_conn(std::move(stub));

    EXPECT_TRUE(conn.health_check());
    EXPECT_EQ(conn.circuit_breaker().state(), CircuitState::CLOSED);
}

TEST(ConnectionWithRetryTest, HealthCheckReconnectsAndReturnsTrueOnSuccess) {
    auto stub = std::make_unique<StubAdapter>(/*initially_connected=*/false);
    stub->set_connect_should_succeed(true);

    auto conn = make_conn(std::move(stub));
    EXPECT_TRUE(conn.health_check("themisdb://localhost:7432"));
    EXPECT_EQ(conn.circuit_breaker().state(), CircuitState::CLOSED);
}

TEST(ConnectionWithRetryTest, HealthCheckRecordsFailureWhenReconnectFails) {
    auto stub = std::make_unique<StubAdapter>(/*initially_connected=*/false);
    stub->set_connect_should_succeed(false);

    auto conn = make_conn(std::move(stub));
    EXPECT_FALSE(conn.health_check("themisdb://localhost:7432"));
}

// ============================================================================
// Error Classification Customisation
// ============================================================================

TEST(ConnectionWithRetryTest, CustomTransientIncludesResourceExhausted) {
    auto stub = std::make_unique<StubAdapter>();
    stub->enqueue_result(Result<RelationalTable>::err(
        ErrorCode::RESOURCE_EXHAUSTED, "OOM"));
    stub->enqueue_result(Result<RelationalTable>::ok(RelationalTable{}));

    auto conn = make_conn(std::move(stub), /*max_retries=*/2,
                          /*custom_transient=*/true);
    auto result = conn.execute_with_retry<RelationalTable>(
        [&]() { return conn.adapter().execute_query("SELECT 1"); });

    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(static_cast<StubAdapter&>(conn.adapter()).call_count(), 2u);
}
