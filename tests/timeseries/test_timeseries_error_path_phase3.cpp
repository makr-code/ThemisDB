/**
 * @file test_timeseries_error_path_phase3.cpp
 * @brief Phase 3 Error Handling Tests - Comprehensive error path coverage
 * @version 0.3.0
 * @note Phase 3 deliverable: >90% error path code coverage
 * 
 * Tests incident taxonomy:
 * - IngestIncident: buffer pressure, validation failures, flush timeouts
 * - QueryIncident: range errors, timeout, consistency failures
 * - LifecycleIncident: retention violations, key rotation failures
 * - IntegrationIncident: remote-write validation, metrics export failures
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "timeseries/timeseries_incident_taxonomy.h"
#include "timeseries/timeseries_api_contract.h"
#include "timeseries/adaptive_flush_controller.h"
#include "timeseries/retention.h"
#include "timeseries/prometheus_remote_write.h"
#include "timeseries/encrypted_chunk_store.h"

#include <atomic>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>

using namespace themis;
using namespace themis::timeseries;

// ─────────────────────────────────────────────────────────────────────────────
// Test Fixtures
// ─────────────────────────────────────────────────────────────────────────────

class IncidentTaxonomyTest : public ::testing::Test {
protected:
    std::vector<Incident> captured_incidents;
    std::atomic<size_t> handler_call_count{0};
    
    void SetUp() override {
        captured_incidents.clear();
        handler_call_count.store(0, std::memory_order_relaxed);
        
        // Register test handler
        setIncidentHandler([this](const Incident& incident) {
            captured_incidents.push_back(incident);
            handler_call_count.fetch_add(1, std::memory_order_relaxed);
        });
    }
    
    void TearDown() override {
        setIncidentHandler(nullptr);
    }
    
    const Incident& lastIncident() const {
        EXPECT_FALSE(captured_incidents.empty());
        return captured_incidents.back();
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Ingest Incident Tests (10 tests)
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
        IngestIncidentCode::BUFFER_OVERFLOW,
        "queue_exhausted",
        IncidentContext{.series_id = "metric_1", .recovery_hint = "increase_buffer_size"}
    );
    emitIncident(incident);
    
    EXPECT_EQ(handler_call_count.load(std::memory_order_relaxed), 1);
    EXPECT_EQ(lastIncident().severity, IncidentSeverity::ERROR);
    EXPECT_EQ(lastIncident().context.series_id, "metric_1");
}

TEST_F(IncidentTaxonomyTest, IngestIncidentFlushTimeout) {
    auto incident = Incident::errorIngest(
        IngestIncidentCode::FLUSH_TIMEOUT,
        "flush_took_>100ms"
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().ingest_code, IngestIncidentCode::FLUSH_TIMEOUT);
}

TEST_F(IncidentTaxonomyTest, IngestIncidentTimestampOutOfOrder) {
    auto incident = Incident::errorIngest(
        IngestIncidentCode::TIMESTAMP_OUT_OF_ORDER,
        "ts=1000_after_ts=2000",
        IncidentContext{.recovery_hint = "check_client_clock"}
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().ingest_code, IngestIncidentCode::TIMESTAMP_OUT_OF_ORDER);
    EXPECT_EQ(lastIncident().context.recovery_hint, "check_client_clock");
}

TEST_F(IncidentTaxonomyTest, IngestIncidentValidationFailure) {
    auto incident = Incident::warnIngest(
        IngestIncidentCode::VALIDATION_FAILURE,
        "metric_name_empty"
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().ingest_code, IngestIncidentCode::VALIDATION_FAILURE);
}

TEST_F(IncidentTaxonomyTest, IngestIncidentCriticalBackpressure) {
    auto incident = Incident::criticalIngest(
        IngestIncidentCode::BUFFER_PRESSURE,
        "continuous_backpressure_5min",
        IncidentContext{.caller_tag = "batch_ingest_worker"}
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().severity, IncidentSeverity::CRITICAL);
    EXPECT_EQ(lastIncident().context.caller_tag, "batch_ingest_worker");
}

TEST_F(IncidentTaxonomyTest, IngestIncidentSeriesNotFound) {
    auto incident = Incident::errorIngest(
        IngestIncidentCode::SERIES_NOT_FOUND,
        "series_123_not_in_store"
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().ingest_code, IngestIncidentCode::SERIES_NOT_FOUND);
}

TEST_F(IncidentTaxonomyTest, IngestIncidentMetricMalformed) {
    auto incident = Incident::warnIngest(
        IngestIncidentCode::METRIC_MALFORMED,
        "invalid_utf8_in_label"
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().ingest_code, IngestIncidentCode::METRIC_MALFORMED);
}

TEST_F(IncidentTaxonomyTest, IngestIncidentEncodingError) {
    auto incident = Incident::errorIngest(
        IngestIncidentCode::ENCODING_ERROR,
        "protobuf_decoding_failed"
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().ingest_code, IngestIncidentCode::ENCODING_ERROR);
}

TEST_F(IncidentTaxonomyTest, IngestIncidentCheckpointFailure) {
    auto incident = Incident::errorIngest(
        IngestIncidentCode::CHECKPOINT_FAILURE,
        "checkpoint_write_io_error",
        IncidentContext{.recovery_hint = "check_disk_space"}
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().ingest_code, IngestIncidentCode::CHECKPOINT_FAILURE);
}

// ─────────────────────────────────────────────────────────────────────────────
// Query Incident Tests (10 tests)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IncidentTaxonomyTest, QueryIncidentRangeInvalid) {
    auto incident = Incident::errorQuery(
        QueryIncidentCode::RANGE_INVALID,
        "start_ts >= end_ts"
    );
    emitIncident(incident);
    
    EXPECT_EQ(handler_call_count.load(std::memory_order_relaxed), 1);
    EXPECT_EQ(lastIncident().severity, IncidentSeverity::ERROR);
    EXPECT_EQ(lastIncident().query_code, QueryIncidentCode::RANGE_INVALID);
}

TEST_F(IncidentTaxonomyTest, QueryIncidentSeriesNotFound) {
    auto incident = Incident::warnQuery(
        QueryIncidentCode::SERIES_NOT_FOUND,
        "series_456_not_in_database"
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().query_code, QueryIncidentCode::SERIES_NOT_FOUND);
}

TEST_F(IncidentTaxonomyTest, QueryIncidentTimeout) {
    auto incident = Incident::errorQuery(
        QueryIncidentCode::QUERY_TIMEOUT,
        "query_exceeded_30s",
        IncidentContext{.recovery_hint = "increase_timeout_or_narrow_range"}
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().query_code, QueryIncidentCode::QUERY_TIMEOUT);
    EXPECT_EQ(lastIncident().severity, IncidentSeverity::ERROR);
}

TEST_F(IncidentTaxonomyTest, QueryIncidentRetentionBoundaryCrossed) {
    auto incident = Incident::warnQuery(
        QueryIncidentCode::RETENTION_BOUNDARY_CROSSED,
        "query_start_before_retention_cutoff"
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().query_code, QueryIncidentCode::RETENTION_BOUNDARY_CROSSED);
}

TEST_F(IncidentTaxonomyTest, QueryIncidentInconsistency) {
    auto incident = Incident::criticalQuery(
        QueryIncidentCode::INCONSISTENCY,
        "federated_shard_returned_different_data"
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().severity, IncidentSeverity::CRITICAL);
    EXPECT_EQ(lastIncident().query_code, QueryIncidentCode::INCONSISTENCY);
}

TEST_F(IncidentTaxonomyTest, QueryIncidentAggregationFailure) {
    auto incident = Incident::errorQuery(
        QueryIncidentCode::AGGREGATION_FAILURE,
        "downsampling_computation_overflow"
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().query_code, QueryIncidentCode::AGGREGATION_FAILURE);
}

TEST_F(IncidentTaxonomyTest, QueryIncidentDecompressionFailure) {
    auto incident = Incident::errorQuery(
        QueryIncidentCode::DECOMPRESSION_FAILURE,
        "gorilla_decompression_crc_mismatch",
        IncidentContext{.series_id = "cpu_usage", .recovery_hint = "check_chunk_integrity"}
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().query_code, QueryIncidentCode::DECOMPRESSION_FAILURE);
    EXPECT_EQ(lastIncident().context.series_id, "cpu_usage");
}

TEST_F(IncidentTaxonomyTest, QueryIncidentCacheHitFailure) {
    auto incident = Incident::warnQuery(
        QueryIncidentCode::CACHE_HIT_FAILURE,
        "cache_entry_evicted_during_query"
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().query_code, QueryIncidentCode::CACHE_HIT_FAILURE);
}

TEST_F(IncidentTaxonomyTest, QueryIncidentIndexNotFound) {
    auto incident = Incident::errorQuery(
        QueryIncidentCode::INDEX_NOT_FOUND,
        "index_file_missing_for_partition"
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().query_code, QueryIncidentCode::INDEX_NOT_FOUND);
}

// ─────────────────────────────────────────────────────────────────────────────
// Lifecycle Incident Tests (10 tests)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IncidentTaxonomyTest, LifecycleIncidentRetentionPolicyViolation) {
    auto incident = Incident::errorLifecycle(
        LifecycleIncidentCode::RETENTION_POLICY_VIOLATION,
        "data_retained_beyond_max_age"
    );
    emitIncident(incident);
    
    EXPECT_EQ(handler_call_count.load(std::memory_order_relaxed), 1);
    EXPECT_EQ(lastIncident().lifecycle_code, LifecycleIncidentCode::RETENTION_POLICY_VIOLATION);
}

TEST_F(IncidentTaxonomyTest, LifecycleIncidentDeletionFailed) {
    auto incident = Incident::errorLifecycle(
        LifecycleIncidentCode::DELETION_FAILED,
        "disk_io_error_deleting_chunk_file",
        IncidentContext{.recovery_hint = "check_disk_permissions"}
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().lifecycle_code, LifecycleIncidentCode::DELETION_FAILED);
    EXPECT_EQ(lastIncident().context.recovery_hint, "check_disk_permissions");
}

TEST_F(IncidentTaxonomyTest, LifecycleIncidentEncryptionRotationFailure) {
    auto incident = Incident::criticalLifecycle(
        LifecycleIncidentCode::ENCRYPTION_ROTATION_FAILURE,
        "key_rotation_unable_to_reach_kms",
        IncidentContext{.series_id = "secret_data", .caller_tag = "rotation_daemon"}
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().severity, IncidentSeverity::CRITICAL);
    EXPECT_EQ(lastIncident().lifecycle_code, LifecycleIncidentCode::ENCRYPTION_ROTATION_FAILURE);
}

TEST_F(IncidentTaxonomyTest, LifecycleIncidentEncryptionKeyNotFound) {
    auto incident = Incident::errorLifecycle(
        LifecycleIncidentCode::ENCRYPTION_KEY_NOT_FOUND,
        "key_version_2_not_in_kms"
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().lifecycle_code, LifecycleIncidentCode::ENCRYPTION_KEY_NOT_FOUND);
}

TEST_F(IncidentTaxonomyTest, LifecycleIncidentEncryptionStateInvalid) {
    auto incident = Incident::errorLifecycle(
        LifecycleIncidentCode::ENCRYPTION_STATE_INVALID,
        "chunk_partially_encrypted"
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().lifecycle_code, LifecycleIncidentCode::ENCRYPTION_STATE_INVALID);
}

TEST_F(IncidentTaxonomyTest, LifecycleIncidentCompactionFailed) {
    auto incident = Incident::warnLifecycle(
        LifecycleIncidentCode::COMPACTION_FAILED,
        "compaction_aborted_due_to_memory_pressure"
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().lifecycle_code, LifecycleIncidentCode::COMPACTION_FAILED);
    EXPECT_EQ(lastIncident().severity, IncidentSeverity::WARN);
}

TEST_F(IncidentTaxonomyTest, LifecycleIncidentTieredStorageTransferFailed) {
    auto incident = Incident::errorLifecycle(
        LifecycleIncidentCode::TIERED_STORAGE_TRANSFER_FAILED,
        "s3_transfer_timeout_after_3_retries"
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().lifecycle_code, LifecycleIncidentCode::TIERED_STORAGE_TRANSFER_FAILED);
}

TEST_F(IncidentTaxonomyTest, LifecycleIncidentIntegrityCheckFailed) {
    auto incident = Incident::criticalLifecycle(
        LifecycleIncidentCode::INTEGRITY_CHECK_FAILED,
        "chunk_checksum_mismatch_after_tiered_transfer"
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().severity, IncidentSeverity::CRITICAL);
}

TEST_F(IncidentTaxonomyTest, LifecycleIncidentRebalanceFailed) {
    auto incident = Incident::errorLifecycle(
        LifecycleIncidentCode::REBALANCE_FAILED,
        "unable_to_move_shard_replica"
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().lifecycle_code, LifecycleIncidentCode::REBALANCE_FAILED);
}

TEST_F(IncidentTaxonomyTest, LifecycleIncidentMaintenanceWindDownFailed) {
    auto incident = Incident::warnLifecycle(
        LifecycleIncidentCode::MAINTENANCE_WINDDOWN_FAILED,
        "unable_to_drain_all_tenants_before_shutdown"
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().lifecycle_code, LifecycleIncidentCode::MAINTENANCE_WINDDOWN_FAILED);
}


// ─────────────────────────────────────────────────────────────────────────────
// Integration Incident Tests (8 tests)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IncidentTaxonomyTest, IntegrationIncidentRemoteWriteValidationError) {
    auto incident = Incident::errorIntegration(
        IntegrationIncidentCode::REMOTE_WRITE_VALIDATION_ERROR,
        "prometheus_auth_token_expired"
    );
    emitIncident(incident);
    
    EXPECT_EQ(handler_call_count.load(std::memory_order_relaxed), 1);
    EXPECT_EQ(lastIncident().integration_code, IntegrationIncidentCode::REMOTE_WRITE_VALIDATION_ERROR);
}

TEST_F(IncidentTaxonomyTest, IntegrationIncidentRemoteWriteRetriesExhausted) {
    auto incident = Incident::errorIntegration(
        IntegrationIncidentCode::REMOTE_WRITE_RETRIES_EXHAUSTED,
        "remote_write_failed_10_consecutive_times",
        IncidentContext{.recovery_hint = "check_remote_service_health"}
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().integration_code, IntegrationIncidentCode::REMOTE_WRITE_RETRIES_EXHAUSTED);
}

TEST_F(IncidentTaxonomyTest, IntegrationIncidentMetricsExportFailure) {
    auto incident = Incident::warnIntegration(
        IntegrationIncidentCode::METRICS_EXPORT_FAILURE,
        "prometheus_scrape_timeout_5s"
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().integration_code, IntegrationIncidentCode::METRICS_EXPORT_FAILURE);
}

TEST_F(IncidentTaxonomyTest, IntegrationIncidentFederationShardFailure) {
    auto incident = Incident::errorIntegration(
        IntegrationIncidentCode::FEDERATION_SHARD_FAILURE,
        "remote_shard_unreachable"
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().integration_code, IntegrationIncidentCode::FEDERATION_SHARD_FAILURE);
}

TEST_F(IncidentTaxonomyTest, IntegrationIncidentCriticalRemoteWriteFailure) {
    auto incident = Incident::criticalIntegration(
        IntegrationIncidentCode::REMOTE_WRITE_VALIDATION_ERROR,
        "all_remote_backends_offline",
        IncidentContext{.caller_tag = "remote_write_buffer"}
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().severity, IncidentSeverity::CRITICAL);
}

TEST_F(IncidentTaxonomyTest, IntegrationIncidentWebhookDeliveryFailure) {
    auto incident = Incident::warnIntegration(
        IntegrationIncidentCode::WEBHOOK_DELIVERY_FAILURE,
        "webhook_http_503_retrying"
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().integration_code, IntegrationIncidentCode::WEBHOOK_DELIVERY_FAILURE);
}

TEST_F(IncidentTaxonomyTest, IntegrationIncidentReplicationLagExceeded) {
    auto incident = Incident::errorIntegration(
        IntegrationIncidentCode::REPLICATION_LAG_EXCEEDED,
        "replica_lagging_30s_behind_primary"
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().integration_code, IntegrationIncidentCode::REPLICATION_LAG_EXCEEDED);
}

TEST_F(IncidentTaxonomyTest, IntegrationIncidentServiceDiscoveryFailure) {
    auto incident = Incident::errorIntegration(
        IntegrationIncidentCode::SERVICE_DISCOVERY_FAILURE,
        "etcd_connection_lost"
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().integration_code, IntegrationIncidentCode::SERVICE_DISCOVERY_FAILURE);
}

// ─────────────────────────────────────────────────────────────────────────────
// Error Classification Tests (Helpers)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IncidentTaxonomyTest, IsBackpressureErrorClassification) {
    EXPECT_TRUE(isBackpressureError(TimeseriesErrorCode::BUFFER_PRESSURE));
    EXPECT_TRUE(isBackpressureError(TimeseriesErrorCode::BUFFER_OVERFLOW));
    EXPECT_FALSE(isBackpressureError(TimeseriesErrorCode::ENCRYPTION_KEY_NOT_FOUND));
}

TEST_F(IncidentTaxonomyTest, IsTransientErrorClassification) {
    EXPECT_TRUE(isTransientError(TimeseriesErrorCode::FLUSH_TIMEOUT));
    EXPECT_TRUE(isTransientError(TimeseriesErrorCode::REMOTE_WRITE_RETRIES_EXHAUSTED));
    EXPECT_FALSE(isTransientError(TimeseriesErrorCode::RETENTION_POLICY_VIOLATION));
}

TEST_F(IncidentTaxonomyTest, IsHardIngestErrorClassification) {
    EXPECT_TRUE(isHardIngestError(TimeseriesErrorCode::TIMESTAMP_OUT_OF_ORDER));
    EXPECT_TRUE(isHardIngestError(TimeseriesErrorCode::ENCODING_ERROR));
    EXPECT_FALSE(isHardIngestError(TimeseriesErrorCode::FLUSH_TIMEOUT));
}

TEST_F(IncidentTaxonomyTest, IsPermanentIntegrationErrorClassification) {
    EXPECT_TRUE(isPermanentIntegrationError(TimeseriesErrorCode::REMOTE_WRITE_VALIDATION_ERROR));
    EXPECT_FALSE(isPermanentIntegrationError(TimeseriesErrorCode::REMOTE_WRITE_RETRIES_EXHAUSTED));
}

// ─────────────────────────────────────────────────────────────────────────────
// Handler Registration Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IncidentTaxonomyTest, HandlerRegistrationAndInvocation) {
    size_t call_count = 0;
    setIncidentHandler([&](const Incident& incident) {
        ++call_count;
    });
    
    auto incident = Incident::warnIngest(IngestIncidentCode::BUFFER_PRESSURE, "test");
    emitIncident(incident);
    
    EXPECT_EQ(call_count, 1);
}

TEST_F(IncidentTaxonomyTest, HandlerDeregistration) {
    setIncidentHandler(nullptr);
    
    auto incident = Incident::warnIngest(IngestIncidentCode::BUFFER_PRESSURE, "test");
    emitIncident(incident);
    
    // Should not call handler, but should still log
    EXPECT_EQ(handler_call_count.load(std::memory_order_relaxed), 0);
}

TEST_F(IncidentTaxonomyTest, ConcurrentIncidentEmission) {
    std::vector<std::thread> threads;
    constexpr size_t num_threads = 10;
    constexpr size_t incidents_per_thread = 10;
    
    for (size_t t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, t]() {
            for (size_t i = 0; i < incidents_per_thread; ++i) {
                auto code = static_cast<IngestIncidentCode>(i % 4);
                auto incident = Incident::warnIngest(code, "concurrent_test");
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
// Context Propagation Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IncidentTaxonomyTest, IncidentContextSerierId) {
    IncidentContext ctx;
    ctx.series_id = "temperature_sensor_42";
    
    auto incident = Incident::errorIngest(
        IngestIncidentCode::SERIES_NOT_FOUND,
        "series_lookup_failed",
        ctx
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().context.series_id, "temperature_sensor_42");
}

TEST_F(IncidentTaxonomyTest, IncidentContextRecoveryHint) {
    IncidentContext ctx;
    ctx.recovery_hint = "increase_buffer_size_or_reduce_ingest_rate";
    
    auto incident = Incident::criticalIngest(
        IngestIncidentCode::BUFFER_OVERFLOW,
        "buffer_exceeded_max_capacity",
        ctx
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().context.recovery_hint, "increase_buffer_size_or_reduce_ingest_rate");
}

TEST_F(IncidentTaxonomyTest, IncidentContextCallerTag) {
    IncidentContext ctx;
    ctx.caller_tag = "batch_ingest_handler_thread_5";
    
    auto incident = Incident::errorQuery(
        QueryIncidentCode::QUERY_TIMEOUT,
        "aggregation_exceeded_time_limit",
        ctx
    );
    emitIncident(incident);
    
    EXPECT_EQ(lastIncident().context.caller_tag, "batch_ingest_handler_thread_5");
}

// ─────────────────────────────────────────────────────────────────────────────
// Severity Levels Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IncidentTaxonomyTest, SeverityLevelCritical) {
    auto incident = Incident::criticalIngest(IngestIncidentCode::BUFFER_OVERFLOW, "test");
    EXPECT_EQ(incident.severity, IncidentSeverity::CRITICAL);
}

TEST_F(IncidentTaxonomyTest, SeverityLevelError) {
    auto incident = Incident::errorIngest(IngestIncidentCode::BUFFER_OVERFLOW, "test");
    EXPECT_EQ(incident.severity, IncidentSeverity::ERROR);
}

TEST_F(IncidentTaxonomyTest, SeverityLevelWarn) {
    auto incident = Incident::warnIngest(IngestIncidentCode::VALIDATION_FAILURE, "test");
    EXPECT_EQ(incident.severity, IncidentSeverity::WARN);
}

TEST_F(IncidentTaxonomyTest, SeverityLevelInfo) {
    auto incident = Incident::infoIngest(IngestIncidentCode::CHECKPOINT_FAILURE, "test");
    EXPECT_EQ(incident.severity, IncidentSeverity::INFO);
}

// ─────────────────────────────────────────────────────────────────────────────
// Factory Method Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IncidentTaxonomyTest, FactoryMethodIngestBoundaryConditions) {
    // Empty message should still work
    auto i1 = Incident::warnIngest(IngestIncidentCode::BUFFER_PRESSURE, "");
    EXPECT_EQ(i1.severity, IncidentSeverity::WARN);
    
    // Long message should work
    std::string long_msg(1000, 'x');
    auto i2 = Incident::warnIngest(IngestIncidentCode::BUFFER_PRESSURE, long_msg);
    EXPECT_EQ(i2.severity, IncidentSeverity::WARN);
}

TEST_F(IncidentTaxonomyTest, FactoryMethodQueryAllSeverities) {
    auto i_crit = Incident::criticalQuery(QueryIncidentCode::INCONSISTENCY, "test");
    auto i_err = Incident::errorQuery(QueryIncidentCode::RANGE_INVALID, "test");
    auto i_warn = Incident::warnQuery(QueryIncidentCode::CACHE_HIT_FAILURE, "test");
    auto i_info = Incident::infoQuery(QueryIncidentCode::DECOMPRESSION_FAILURE, "test");
    
    EXPECT_EQ(i_crit.severity, IncidentSeverity::CRITICAL);
    EXPECT_EQ(i_err.severity, IncidentSeverity::ERROR);
    EXPECT_EQ(i_warn.severity, IncidentSeverity::WARN);
    EXPECT_EQ(i_info.severity, IncidentSeverity::INFO);
}

TEST_F(IncidentTaxonomyTest, FactoryMethodLifecycleAllSeverities) {
    auto i_crit = Incident::criticalLifecycle(LifecycleIncidentCode::ENCRYPTION_ROTATION_FAILURE, "test");
    auto i_err = Incident::errorLifecycle(LifecycleIncidentCode::DELETION_FAILED, "test");
    auto i_warn = Incident::warnLifecycle(LifecycleIncidentCode::COMPACTION_FAILED, "test");
    auto i_info = Incident::infoLifecycle(LifecycleIncidentCode::REBALANCE_FAILED, "test");
    
    EXPECT_EQ(i_crit.severity, IncidentSeverity::CRITICAL);
    EXPECT_EQ(i_err.severity, IncidentSeverity::ERROR);
    EXPECT_EQ(i_warn.severity, IncidentSeverity::WARN);
    EXPECT_EQ(i_info.severity, IncidentSeverity::INFO);
}

TEST_F(IncidentTaxonomyTest, FactoryMethodIntegrationAllSeverities) {
    auto i_crit = Incident::criticalIntegration(IntegrationIncidentCode::FEDERATION_SHARD_FAILURE, "test");
    auto i_err = Incident::errorIntegration(IntegrationIncidentCode::REMOTE_WRITE_RETRIES_EXHAUSTED, "test");
    auto i_warn = Incident::warnIntegration(IntegrationIncidentCode::WEBHOOK_DELIVERY_FAILURE, "test");
    auto i_info = Incident::infoIntegration(IntegrationIncidentCode::SERVICE_DISCOVERY_FAILURE, "test");
    
    EXPECT_EQ(i_crit.severity, IncidentSeverity::CRITICAL);
    EXPECT_EQ(i_err.severity, IncidentSeverity::ERROR);
    EXPECT_EQ(i_warn.severity, IncidentSeverity::WARN);
    EXPECT_EQ(i_info.severity, IncidentSeverity::INFO);
}

// ─────────────────────────────────────────────────────────────────────────────
// Emission Latency Tests (Performance Gates)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IncidentTaxonomyTest, IncidentEmissionLatencyBounded) {
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; ++i) {
        auto incident = Incident::warnIngest(IngestIncidentCode::BUFFER_PRESSURE, "perf_test");
        emitIncident(incident);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Latency should be bounded: 1000 emissions should be < 100ms total (~100µs per emission)
    EXPECT_LT(duration_us.count(), 100000);
}

TEST_F(IncidentTaxonomyTest, IncidentCreationLatencyBounded) {
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 10000; ++i) {
        auto incident = Incident::warnIngest(IngestIncidentCode::BUFFER_PRESSURE, "perf_test");
        (void)incident;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Creation should be very fast: 10000 creations should be < 50ms (~5µs per creation)
    EXPECT_LT(duration_us.count(), 50000);
}

// ─────────────────────────────────────────────────────────────────────────────
// Error Code Validation Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IncidentTaxonomyTest, AllIngestCodesRepresented) {
    // Verify all ingest incident codes can be created
    std::vector<IngestIncidentCode> codes = {
        IngestIncidentCode::BUFFER_PRESSURE,
        IngestIncidentCode::BUFFER_OVERFLOW,
        IngestIncidentCode::FLUSH_TIMEOUT,
        IngestIncidentCode::TIMESTAMP_OUT_OF_ORDER,
        IngestIncidentCode::VALIDATION_FAILURE,
        IngestIncidentCode::SERIES_NOT_FOUND,
        IngestIncidentCode::METRIC_MALFORMED,
        IngestIncidentCode::ENCODING_ERROR,
        IngestIncidentCode::CHECKPOINT_FAILURE
    };
    
    for (auto code : codes) {
        auto incident = Incident::warnIngest(code, "test");
        EXPECT_EQ(incident.ingest_code, code);
    }
}

TEST_F(IncidentTaxonomyTest, AllQueryCodesRepresented) {
    // Verify all query incident codes can be created
    std::vector<QueryIncidentCode> codes = {
        QueryIncidentCode::RANGE_INVALID,
        QueryIncidentCode::SERIES_NOT_FOUND,
        QueryIncidentCode::QUERY_TIMEOUT,
        QueryIncidentCode::RETENTION_BOUNDARY_CROSSED,
        QueryIncidentCode::INCONSISTENCY,
        QueryIncidentCode::AGGREGATION_FAILURE,
        QueryIncidentCode::DECOMPRESSION_FAILURE,
        QueryIncidentCode::CACHE_HIT_FAILURE,
        QueryIncidentCode::INDEX_NOT_FOUND
    };
    
    for (auto code : codes) {
        auto incident = Incident::warnQuery(code, "test");
        EXPECT_EQ(incident.query_code, code);
    }
}

TEST_F(IncidentTaxonomyTest, AllLifecycleCodesRepresented) {
    // Verify all lifecycle incident codes can be created
    std::vector<LifecycleIncidentCode> codes = {
        LifecycleIncidentCode::RETENTION_POLICY_VIOLATION,
        LifecycleIncidentCode::DELETION_FAILED,
        LifecycleIncidentCode::ENCRYPTION_ROTATION_FAILURE,
        LifecycleIncidentCode::ENCRYPTION_KEY_NOT_FOUND,
        LifecycleIncidentCode::ENCRYPTION_STATE_INVALID,
        LifecycleIncidentCode::COMPACTION_FAILED,
        LifecycleIncidentCode::TIERED_STORAGE_TRANSFER_FAILED,
        LifecycleIncidentCode::INTEGRITY_CHECK_FAILED,
        LifecycleIncidentCode::REBALANCE_FAILED,
        LifecycleIncidentCode::MAINTENANCE_WINDDOWN_FAILED
    };
    
    for (auto code : codes) {
        auto incident = Incident::warnLifecycle(code, "test");
        EXPECT_EQ(incident.lifecycle_code, code);
    }
}

TEST_F(IncidentTaxonomyTest, AllIntegrationCodesRepresented) {
    // Verify all integration incident codes can be created
    std::vector<IntegrationIncidentCode> codes = {
        IntegrationIncidentCode::REMOTE_WRITE_VALIDATION_ERROR,
        IntegrationIncidentCode::REMOTE_WRITE_RETRIES_EXHAUSTED,
        IntegrationIncidentCode::METRICS_EXPORT_FAILURE,
        IntegrationIncidentCode::FEDERATION_SHARD_FAILURE,
        IntegrationIncidentCode::WEBHOOK_DELIVERY_FAILURE,
        IntegrationIncidentCode::REPLICATION_LAG_EXCEEDED,
        IntegrationIncidentCode::SERVICE_DISCOVERY_FAILURE
    };
    
    for (auto code : codes) {
        auto incident = Incident::warnIntegration(code, "test");
        EXPECT_EQ(incident.integration_code, code);
    }
}

}  // namespace themis
