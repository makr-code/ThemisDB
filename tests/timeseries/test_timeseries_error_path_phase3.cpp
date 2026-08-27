/**
 * @file test_timeseries_error_path_phase3.cpp
 * @brief Phase 3 Error Handling Tests - Incident taxonomy validation
 * @version 0.3.0
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "timeseries/timeseries_incident_taxonomy.h"
#include "timeseries/timeseries_api_contract.h"

#include <atomic>
#include <mutex>
#include <vector>
#include <memory>
#include <thread>

using namespace themis;
using namespace themis::timeseries;

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture
//
// IncidentHandler is a plain function pointer (void(*)(const Incident&) noexcept),
// so capturing lambdas cannot be used directly.  The fixture stores a static
// pointer to the active instance and routes through a non-capturing free function.
// ─────────────────────────────────────────────────────────────────────────────

class IncidentTaxonomyTest : public ::testing::Test {
public:
    // Static pointer to the currently-active test fixture instance.
    // Protected by the incident handler mechanism: only one test runs at a time.
    static IncidentTaxonomyTest* s_active;

    // Non-capturing handler forwarded to the active fixture.
    static void staticHandler(const Incident& incident) noexcept {
        auto* self = s_active;
        if (!self) return;
        {
            std::lock_guard<std::mutex> lk(self->incidents_mutex_);
            self->captured_incidents.push_back(incident);
        }
        self->handler_call_count.fetch_add(1, std::memory_order_relaxed);
    }

protected:
    std::vector<Incident> captured_incidents;
    std::atomic<size_t>   handler_call_count{0};
    std::mutex            incidents_mutex_;

    void SetUp() override {
        captured_incidents.clear();
        handler_call_count.store(0, std::memory_order_relaxed);
        s_active = this;
        setIncidentHandler(&IncidentTaxonomyTest::staticHandler);
    }

    void TearDown() override {
        setIncidentHandler(nullptr);
        s_active = nullptr;
    }

    // Returns a snapshot copy of the last captured incident (thread-safe).
    Incident lastIncident() const {
        std::lock_guard<std::mutex> lk(const_cast<std::mutex&>(incidents_mutex_));
        EXPECT_FALSE(captured_incidents.empty());
        return captured_incidents.back();
    }
};

IncidentTaxonomyTest* IncidentTaxonomyTest::s_active = nullptr;

// ─────────────────────────────────────────────────────────────────────────────
// Ingest Incident Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IncidentTaxonomyTest, IngestIncidentBufferPressure) {
    auto incident = Incident::warnIngest(
        IngestIncidentCode::BUFFER_PRESSURE_HIGH,
        IncidentContext{.caller_tag = "producer_123"}
    );
    emitIncident(incident);
    
    EXPECT_EQ(handler_call_count.load(std::memory_order_relaxed), 1);
    EXPECT_EQ(lastIncident().severity, IncidentSeverity::WARN);
    EXPECT_EQ(lastIncident().code.ingest_code, IngestIncidentCode::BUFFER_PRESSURE_HIGH);
}

TEST_F(IncidentTaxonomyTest, IngestIncidentBufferOverflow) {
    auto incident = Incident::errorIngest(
        IngestIncidentCode::BUFFER_OVERFLOW_IMMINENT,
        IncidentContext{.series_id = "metric_1", .recovery_hint = "increase_buffer_size"}
    );
    emitIncident(incident);
    
    EXPECT_EQ(handler_call_count.load(std::memory_order_relaxed), 1);
    EXPECT_EQ(lastIncident().severity, IncidentSeverity::ERROR);
    EXPECT_EQ(lastIncident().code.ingest_code, IngestIncidentCode::BUFFER_OVERFLOW_IMMINENT);
    EXPECT_EQ(lastIncident().context.series_id, "metric_1");
}

TEST_F(IncidentTaxonomyTest, IngestIncidentFlushTimeout) {
    auto incident = Incident::errorIngest(
        IngestIncidentCode::FLUSH_TIMEOUT
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().code.ingest_code, IngestIncidentCode::FLUSH_TIMEOUT);
}

TEST_F(IncidentTaxonomyTest, IngestIncidentTimestampOutOfOrder) {
    auto incident = Incident::errorIngest(
        IngestIncidentCode::TIMESTAMP_OUT_OF_ORDER,
        IncidentContext{.recovery_hint = "check_client_clock"}
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().code.ingest_code, IngestIncidentCode::TIMESTAMP_OUT_OF_ORDER);
    EXPECT_EQ(lastIncident().context.recovery_hint, "check_client_clock");
}

TEST_F(IncidentTaxonomyTest, IngestIncidentTimestampInvalid) {
    auto incident = Incident::warnIngest(
        IngestIncidentCode::TIMESTAMP_INVALID
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().code.ingest_code, IngestIncidentCode::TIMESTAMP_INVALID);
}

TEST_F(IncidentTaxonomyTest, IngestIncidentSeriesQuotaExceeded) {
    auto incident = Incident::criticalIngest(
        IngestIncidentCode::SERIES_QUOTA_EXCEEDED,
        IncidentContext{.caller_tag = "batch_ingest_worker"}
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().severity, IncidentSeverity::CRITICAL);
    EXPECT_EQ(lastIncident().code.ingest_code, IngestIncidentCode::SERIES_QUOTA_EXCEEDED);
}

TEST_F(IncidentTaxonomyTest, IngestIncidentSeriesCapacityExceeded) {
    auto incident = Incident::errorIngest(
        IngestIncidentCode::SERIES_CAPACITY_EXCEEDED
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().code.ingest_code, IngestIncidentCode::SERIES_CAPACITY_EXCEEDED);
}

TEST_F(IncidentTaxonomyTest, IngestIncidentInternalError) {
    auto incident = Incident::errorIngest(
        IngestIncidentCode::INGEST_INTERNAL_ERROR
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().code.ingest_code, IngestIncidentCode::INGEST_INTERNAL_ERROR);
}

// ─────────────────────────────────────────────────────────────────────────────
// Query Incident Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IncidentTaxonomyTest, QueryIncidentRangeInvalid) {
    auto incident = Incident::errorQuery(
        QueryIncidentCode::RANGE_INVALID
    );
    emitIncident(incident);
    
    EXPECT_EQ(handler_call_count.load(std::memory_order_relaxed), 1);
    EXPECT_EQ(lastIncident().severity, IncidentSeverity::ERROR);
    EXPECT_EQ(lastIncident().code.query_code, QueryIncidentCode::RANGE_INVALID);
}

TEST_F(IncidentTaxonomyTest, QueryIncidentSeriesNotFound) {
    auto incident = Incident::warnQuery(
        QueryIncidentCode::SERIES_NOT_FOUND
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().code.query_code, QueryIncidentCode::SERIES_NOT_FOUND);
}

TEST_F(IncidentTaxonomyTest, QueryIncidentTimeout) {
    auto incident = Incident::errorQuery(
        QueryIncidentCode::QUERY_TIMEOUT,
        IncidentContext{.recovery_hint = "increase_timeout_or_narrow_range"}
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().code.query_code, QueryIncidentCode::QUERY_TIMEOUT);
    EXPECT_EQ(lastIncident().severity, IncidentSeverity::ERROR);
}

TEST_F(IncidentTaxonomyTest, QueryIncidentRetentionBoundaryCrossed) {
    auto incident = Incident::warnQuery(
        QueryIncidentCode::RETENTION_BOUNDARY_CROSSED
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().code.query_code, QueryIncidentCode::RETENTION_BOUNDARY_CROSSED);
}

TEST_F(IncidentTaxonomyTest, QueryIncidentConsistencyCheckFailed) {
    auto incident = Incident::criticalQuery(
        QueryIncidentCode::CONSISTENCY_CHECK_FAILED
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().severity, IncidentSeverity::CRITICAL);
    EXPECT_EQ(lastIncident().code.query_code, QueryIncidentCode::CONSISTENCY_CHECK_FAILED);
}

TEST_F(IncidentTaxonomyTest, QueryIncidentDownsamplingInvalid) {
    auto incident = Incident::errorQuery(
        QueryIncidentCode::DOWNSAMPLING_INVALID
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().code.query_code, QueryIncidentCode::DOWNSAMPLING_INVALID);
}

TEST_F(IncidentTaxonomyTest, QueryIncidentInternalError) {
    auto incident = Incident::errorQuery(
        QueryIncidentCode::QUERY_INTERNAL_ERROR,
        IncidentContext{.series_id = "cpu_usage", .recovery_hint = "check_logs"}
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().code.query_code, QueryIncidentCode::QUERY_INTERNAL_ERROR);
    EXPECT_EQ(lastIncident().context.series_id, "cpu_usage");
}

// ─────────────────────────────────────────────────────────────────────────────
// Lifecycle Incident Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IncidentTaxonomyTest, LifecycleIncidentRetentionExpired) {
    auto incident = Incident::infoLifecycle(
        LifecycleIncidentCode::RETENTION_EXPIRED
    );
    emitIncident(incident);
    
    EXPECT_EQ(handler_call_count.load(std::memory_order_relaxed), 1);
    EXPECT_EQ(lastIncident().severity, IncidentSeverity::INFO);
    EXPECT_EQ(lastIncident().code.lifecycle_code, LifecycleIncidentCode::RETENTION_EXPIRED);
}

TEST_F(IncidentTaxonomyTest, LifecycleIncidentRetentionPolicyViolation) {
    auto incident = Incident::errorLifecycle(
        LifecycleIncidentCode::RETENTION_POLICY_VIOLATION
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().code.lifecycle_code, LifecycleIncidentCode::RETENTION_POLICY_VIOLATION);
}

TEST_F(IncidentTaxonomyTest, LifecycleIncidentDeletionFailed) {
    auto incident = Incident::errorLifecycle(
        LifecycleIncidentCode::DELETION_FAILED,
        IncidentContext{.recovery_hint = "check_disk_permissions"}
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().code.lifecycle_code, LifecycleIncidentCode::DELETION_FAILED);
    EXPECT_EQ(lastIncident().context.recovery_hint, "check_disk_permissions");
}

TEST_F(IncidentTaxonomyTest, LifecycleIncidentEncryptionRotationFailure) {
    auto incident = Incident::criticalLifecycle(
        LifecycleIncidentCode::ENCRYPTION_ROTATION_FAILURE,
        IncidentContext{.series_id = "secret_data", .caller_tag = "rotation_daemon"}
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().severity, IncidentSeverity::CRITICAL);
    EXPECT_EQ(lastIncident().code.lifecycle_code, LifecycleIncidentCode::ENCRYPTION_ROTATION_FAILURE);
}

TEST_F(IncidentTaxonomyTest, LifecycleIncidentEncryptionKeyNotFound) {
    auto incident = Incident::errorLifecycle(
        LifecycleIncidentCode::ENCRYPTION_KEY_NOT_FOUND
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().code.lifecycle_code, LifecycleIncidentCode::ENCRYPTION_KEY_NOT_FOUND);
}

TEST_F(IncidentTaxonomyTest, LifecycleIncidentEncryptionStateInvalid) {
    auto incident = Incident::errorLifecycle(
        LifecycleIncidentCode::ENCRYPTION_STATE_INVALID
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().code.lifecycle_code, LifecycleIncidentCode::ENCRYPTION_STATE_INVALID);
}

TEST_F(IncidentTaxonomyTest, LifecycleIncidentGCFailed) {
    auto incident = Incident::warnLifecycle(
        LifecycleIncidentCode::GC_FAILED
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().code.lifecycle_code, LifecycleIncidentCode::GC_FAILED);
    EXPECT_EQ(lastIncident().severity, IncidentSeverity::WARN);
}

TEST_F(IncidentTaxonomyTest, LifecycleIncidentInternalError) {
    auto incident = Incident::errorLifecycle(
        LifecycleIncidentCode::LIFECYCLE_INTERNAL_ERROR
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().code.lifecycle_code, LifecycleIncidentCode::LIFECYCLE_INTERNAL_ERROR);
}

// ─────────────────────────────────────────────────────────────────────────────
// Integration Incident Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IncidentTaxonomyTest, IntegrationIncidentRemoteWriteClientError) {
    auto incident = Incident::errorIntegration(
        IntegrationIncidentCode::REMOTE_WRITE_CLIENT_ERROR
    );
    emitIncident(incident);
    
    EXPECT_EQ(handler_call_count.load(std::memory_order_relaxed), 1);
    EXPECT_EQ(lastIncident().code.integration_code, IntegrationIncidentCode::REMOTE_WRITE_CLIENT_ERROR);
}

TEST_F(IncidentTaxonomyTest, IntegrationIncidentRemoteWriteServerError) {
    auto incident = Incident::errorIntegration(
        IntegrationIncidentCode::REMOTE_WRITE_SERVER_ERROR
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().code.integration_code, IntegrationIncidentCode::REMOTE_WRITE_SERVER_ERROR);
}

TEST_F(IncidentTaxonomyTest, IntegrationIncidentRemoteWriteNetworkError) {
    auto incident = Incident::errorIntegration(
        IntegrationIncidentCode::REMOTE_WRITE_NETWORK_ERROR
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().code.integration_code, IntegrationIncidentCode::REMOTE_WRITE_NETWORK_ERROR);
}

TEST_F(IncidentTaxonomyTest, IntegrationIncidentRemoteWriteValidationError) {
    auto incident = Incident::errorIntegration(
        IntegrationIncidentCode::REMOTE_WRITE_VALIDATION_ERROR,
        IncidentContext{.recovery_hint = "check_schema"}
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().code.integration_code, IntegrationIncidentCode::REMOTE_WRITE_VALIDATION_ERROR);
}

TEST_F(IncidentTaxonomyTest, IntegrationIncidentRemoteWriteRetriesExhausted) {
    auto incident = Incident::errorIntegration(
        IntegrationIncidentCode::REMOTE_WRITE_RETRIES_EXHAUSTED,
        IncidentContext{.recovery_hint = "check_remote_service_health"}
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().code.integration_code, IntegrationIncidentCode::REMOTE_WRITE_RETRIES_EXHAUSTED);
}

TEST_F(IncidentTaxonomyTest, IntegrationIncidentMetricsExportFailed) {
    auto incident = Incident::warnIntegration(
        IntegrationIncidentCode::METRICS_EXPORT_FAILED
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().code.integration_code, IntegrationIncidentCode::METRICS_EXPORT_FAILED);
}

TEST_F(IncidentTaxonomyTest, IntegrationIncidentCriticalRemoteWriteFailure) {
    auto incident = Incident::criticalIntegration(
        IntegrationIncidentCode::REMOTE_WRITE_CLIENT_ERROR,
        IncidentContext{.caller_tag = "remote_write_buffer"}
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().severity, IncidentSeverity::CRITICAL);
}

// ─────────────────────────────────────────────────────────────────────────────
// Error Classification Tests (Helpers)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IncidentTaxonomyTest, IsBackpressureErrorClassification) {
    EXPECT_TRUE(isBackpressureError(IngestIncidentCode::BUFFER_PRESSURE_HIGH));
    EXPECT_TRUE(isBackpressureError(IngestIncidentCode::BUFFER_OVERFLOW_IMMINENT));
    EXPECT_FALSE(isBackpressureError(IngestIncidentCode::FLUSH_TIMEOUT));
}

TEST_F(IncidentTaxonomyTest, IsHardIngestErrorClassification) {
    EXPECT_TRUE(isHardIngestError(IngestIncidentCode::TIMESTAMP_OUT_OF_ORDER));
    EXPECT_TRUE(isHardIngestError(IngestIncidentCode::TIMESTAMP_INVALID));
    EXPECT_FALSE(isHardIngestError(IngestIncidentCode::FLUSH_TIMEOUT));
}

TEST_F(IncidentTaxonomyTest, IsHardQueryErrorClassification) {
    EXPECT_TRUE(isHardQueryError(QueryIncidentCode::SERIES_NOT_FOUND));
    EXPECT_TRUE(isHardQueryError(QueryIncidentCode::RANGE_INVALID));
    EXPECT_FALSE(isHardQueryError(QueryIncidentCode::QUERY_TIMEOUT));
}

TEST_F(IncidentTaxonomyTest, IsRetryableIntegrationError) {
    EXPECT_TRUE(isRetryableIntegrationError(IntegrationIncidentCode::REMOTE_WRITE_SERVER_ERROR));
    EXPECT_TRUE(isRetryableIntegrationError(IntegrationIncidentCode::REMOTE_WRITE_NETWORK_ERROR));
    EXPECT_FALSE(isRetryableIntegrationError(IntegrationIncidentCode::REMOTE_WRITE_CLIENT_ERROR));
}

TEST_F(IncidentTaxonomyTest, IsPermanentIntegrationError) {
    EXPECT_TRUE(isPermanentIntegrationError(IntegrationIncidentCode::REMOTE_WRITE_CLIENT_ERROR));
    EXPECT_TRUE(isPermanentIntegrationError(IntegrationIncidentCode::REMOTE_WRITE_VALIDATION_ERROR));
    EXPECT_FALSE(isPermanentIntegrationError(IntegrationIncidentCode::REMOTE_WRITE_SERVER_ERROR));
}

// ─────────────────────────────────────────────────────────────────────────────
// Handler Registration Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IncidentTaxonomyTest, HandlerRegistrationAndInvocation) {
    // The fixture already registered staticHandler in SetUp().
    // Verify that emitIncident() routes through the registered handler.
    auto incident = Incident::warnIngest(IngestIncidentCode::BUFFER_PRESSURE_HIGH);
    emitIncident(incident);

    EXPECT_EQ(handler_call_count.load(std::memory_order_relaxed), 1u);
    setIncidentHandler(nullptr);
}

TEST_F(IncidentTaxonomyTest, HandlerDeregistration) {
    setIncidentHandler(nullptr);
    
    auto incident = Incident::warnIngest(IngestIncidentCode::BUFFER_PRESSURE_HIGH);
    emitIncident(incident);
    
    // Should not call handler, but should still log
    EXPECT_EQ(handler_call_count.load(std::memory_order_relaxed), 0);
}

TEST_F(IncidentTaxonomyTest, ConcurrentIncidentEmission) {
    std::vector<std::thread> threads;
    constexpr size_t num_threads = 10;
    constexpr size_t incidents_per_thread = 10;
    
    for (size_t t = 0; t < num_threads; ++t) {
        threads.emplace_back([this]() {
            for (size_t i = 0; i < incidents_per_thread; ++i) {
                auto incident = Incident::warnIngest(IngestIncidentCode::BUFFER_PRESSURE_HIGH);
                emitIncident(incident);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(handler_call_count.load(std::memory_order_relaxed), 
              num_threads * incidents_per_thread);
}

// ─────────────────────────────────────────────────────────────────────────────
// Severity Levels Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IncidentTaxonomyTest, SeverityLevelCritical) {
    auto incident = Incident::criticalIngest(IngestIncidentCode::BUFFER_OVERFLOW_IMMINENT);
    EXPECT_EQ(incident.severity, IncidentSeverity::CRITICAL);
}

TEST_F(IncidentTaxonomyTest, SeverityLevelError) {
    auto incident = Incident::errorIngest(IngestIncidentCode::BUFFER_OVERFLOW_IMMINENT);
    EXPECT_EQ(incident.severity, IncidentSeverity::ERROR);
}

TEST_F(IncidentTaxonomyTest, SeverityLevelWarn) {
    auto incident = Incident::warnIngest(IngestIncidentCode::TIMESTAMP_INVALID);
    EXPECT_EQ(incident.severity, IncidentSeverity::WARN);
}

TEST_F(IncidentTaxonomyTest, SeverityLevelInfo) {
    auto incident = Incident::infoIngest(IngestIncidentCode::FLUSH_TIMEOUT);
    EXPECT_EQ(incident.severity, IncidentSeverity::INFO);
}

// ─────────────────────────────────────────────────────────────────────────────
// Performance Tests (Bounded Latency)
//
// These tests measure wall-clock time and are environment-dependent; they are
// intentionally disabled for CI.  Latency gate validation belongs in the
// dedicated benchmark suite at benchmarks/timeseries/bench_timeseries_release_gates.cpp
// (TSRG-01..TSRG-06).  Re-enable manually when profiling locally.
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IncidentTaxonomyTest, DISABLED_IncidentEmissionLatencyBounded) {
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; ++i) {
        auto incident = Incident::warnIngest(IngestIncidentCode::BUFFER_PRESSURE_HIGH);
        emitIncident(incident);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // 1000 emissions should be < 100ms total (~100µs per emission)
    EXPECT_LT(duration_us.count(), 100000);
}

TEST_F(IncidentTaxonomyTest, DISABLED_IncidentCreationLatencyBounded) {
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 10000; ++i) {
        auto incident = Incident::warnIngest(IngestIncidentCode::BUFFER_PRESSURE_HIGH);
        (void)incident;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // 10000 creations should be < 50ms (~5µs per creation)
    EXPECT_LT(duration_us.count(), 50000);
}
