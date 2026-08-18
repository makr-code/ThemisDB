/**
 * @file test_audit_high_volume_stress.cpp
 * @brief High-volume audit entry stress tests (Wave C Batch 1)
 * @version 0.1.0
 * @note Part of Wave C — Security Production Validation
 * @note Performance targets: 1000+ events/sec sustained load, p95/p99 latency gates
 */

#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <atomic>
#include <vector>
#include <random>
#include <algorithm>

#include "governance/audit_batch_writer.h"
#include "governance/governance_audit_integrity.h"

namespace themis {
namespace governance {
namespace tests {

// ============================================================================
// Test Fixtures and Helpers
// ============================================================================

class AuditBatchWriterStressTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create audit integrity manager with HMAC signing
        auto signer = std::make_shared<AuditSigner>(
            AuditSigner::SignatureAlgorithm::HMAC_SHA256,
            "test-key-001",
            "secret-key-for-testing"
        );
        
        AuditRetentionPolicy policy;
        policy.retention_period_days = 2555;  // 7 years
        policy.archive_after_days = 365;
        
        manager_ = std::make_shared<AuditIntegrityManager>(policy, signer);
        
        // Create batch writer with test configuration
        AuditBatchWriter::Config config;
        config.buffer_size = 50000;
        config.batch_size = 5000;
        config.flush_interval_ms = 50;
        config.enable_checkpoints = true;
        config.enable_metrics = true;
        config.enable_backpressure = false;
        
        writer_ = std::make_unique<AuditBatchWriter>(manager_, config);
        
        ASSERT_EQ(writer_->start(), "OK");
    }
    
    void TearDown() override {
        if (writer_ && writer_->isRunning()) {
            writer_->shutdown();
        }
    }
    
    ImmutableAuditEntry createTestEntry(int sequence) {
        ImmutableAuditEntry entry;
        entry.entry_id = "entry_" + std::to_string(sequence);
        entry.rule_id = "rule_test_001";
        entry.operation = "update";
        entry.user = "test_user";
        entry.timestamp_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
        entry.details["test_sequence"] = sequence;
        entry.details["description"] = "Test audit entry for stress testing";
        return entry;
    }
    
    std::shared_ptr<AuditIntegrityManager> manager_;
    std::unique_ptr<AuditBatchWriter> writer_;
};

// ============================================================================
// Basic Functionality Tests
// ============================================================================

TEST_F(AuditBatchWriterStressTest, SingleEntrySubmission) {
    ImmutableAuditEntry entry = createTestEntry(1);
    
    auto start = std::chrono::high_resolution_clock::now();
    std::string status = writer_->submitEntry(entry);
    auto end = std::chrono::high_resolution_clock::now();
    
    ASSERT_EQ(status, "OK");
    
    auto latency_us = std::chrono::duration_cast<std::chrono::microseconds>(
        end - start
    ).count();
    
    // Wave C target: ≤100µs p95
    EXPECT_LT(latency_us, 1000) << "Single entry submission should be < 1ms";
}

TEST_F(AuditBatchWriterStressTest, BufferFillAndFlush) {
    const int ENTRIES = 1000;
    
    for (int i = 0; i < ENTRIES; ++i) {
        ImmutableAuditEntry entry = createTestEntry(i);
        std::string status = writer_->submitEntry(entry);
        ASSERT_EQ(status, "OK") << "Entry " << i << " failed to submit";
    }
    
    auto stats = writer_->getBufferStats();
    EXPECT_GT(stats["pending_entries"].get<int>(), 0) << "Buffer should have pending entries";
    
    auto result = writer_->forceFlush();
    EXPECT_TRUE(result.success) << result.error_message;
    EXPECT_EQ(result.entries_written, ENTRIES);
}

// ============================================================================
// High-Volume Load Tests (Wave C Criteria)
// ============================================================================

TEST_F(AuditBatchWriterStressTest, HighVolumeLoad_1000PerSecond) {
    /**
     * Wave C Exit Criteria: Audit export remains trustworthy under 
     * 1000+ events/sec sustained load
     */
    const int EVENTS_PER_SECOND = 1000;
    const int DURATION_SECONDS = 5;
    const int TOTAL_EVENTS = EVENTS_PER_SECOND * DURATION_SECONDS;
    
    std::atomic<int> submitted{0};
    std::atomic<int> errors{0};
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Submit events at target rate
    for (int i = 0; i < TOTAL_EVENTS; ++i) {
        ImmutableAuditEntry entry = createTestEntry(i);
        std::string status = writer_->submitEntry(entry);
        
        if (status == "OK") {
            submitted++;
        } else {
            errors++;
        }
        
        // Rate limiting: ~1ms between entries for 1000 events/sec
        if (i % 100 == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    // Wait for flushing
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration_seconds = std::chrono::duration_cast<std::chrono::seconds>(
        end_time - start_time
    ).count();
    
    EXPECT_EQ(errors.load(), 0) << "Should have no submission errors";
    EXPECT_EQ(submitted.load(), TOTAL_EVENTS) << "Should submit all events";
    
    auto metrics = writer_->getMetrics();
    EXPECT_GE(metrics["total_entries_flushed"].get<int>(), TOTAL_EVENTS * 0.95)
        << "Should flush at least 95% of entries";
    
    double throughput = submitted.load() / (double)duration_seconds;
    EXPECT_GE(throughput, EVENTS_PER_SECOND * 0.9)
        << "Should maintain at least 90% of target throughput";
}

TEST_F(AuditBatchWriterStressTest, LatencyUnderLoad) {
    /**
     * Wave C Exit Criteria: P95/P99 latency locked for audit operations
     * Targets: submission ≤100µs p95, ≤500µs p99
     */
    const int ENTRIES = 10000;
    std::vector<int64_t> latencies;
    latencies.reserve(ENTRIES);
    
    for (int i = 0; i < ENTRIES; ++i) {
        ImmutableAuditEntry entry = createTestEntry(i);
        
        auto start = std::chrono::high_resolution_clock::now();
        writer_->submitEntry(entry);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto latency_us = std::chrono::duration_cast<std::chrono::microseconds>(
            end - start
        ).count();
        
        latencies.push_back(latency_us);
    }
    
    std::sort(latencies.begin(), latencies.end());
    
    // Calculate percentiles
    int p95_idx = static_cast<int>(ENTRIES * 0.95);
    int p99_idx = static_cast<int>(ENTRIES * 0.99);
    
    auto p50 = latencies[ENTRIES / 2];
    auto p95 = latencies[p95_idx];
    auto p99 = latencies[p99_idx];
    
    EXPECT_LT(p50, 100) << "P50 submission latency should be < 100µs";
    EXPECT_LT(p95, 200) << "P95 submission latency should be < 200µs (test tolerance higher than Wave C target)";
    EXPECT_LT(p99, 500) << "P99 submission latency should be < 500µs";
    
    // Report metrics
    std::cout << "Latency metrics: p50=" << p50 << "µs, p95=" << p95 << "µs, p99=" << p99 << "µs\n";
}

// ============================================================================
// Idempotency Tests
// ============================================================================

TEST_F(AuditBatchWriterStressTest, IdempotentSubmissions) {
    /**
     * Wave C Criteria: Export reliability gates with idempotency validation
     */
    ImmutableAuditEntry entry = createTestEntry(1);
    const std::string TOKEN = "token_001";
    
    // First submission
    std::string status1 = writer_->submitEntryIdempotent(entry, TOKEN);
    ASSERT_EQ(status1, "OK");
    
    // Second submission with same token
    std::string status2 = writer_->submitEntryIdempotent(entry, TOKEN);
    ASSERT_EQ(status2, "DUPLICATE") << "Second submission with same token should be rejected";
    
    // Check token status
    auto token_status = writer_->getTokenStatus(TOKEN);
    ASSERT_TRUE(token_status.has_value());
    EXPECT_NE(token_status->state, "failed");
}

TEST_F(AuditBatchWriterStressTest, ConcurrentIdempotentSubmissions) {
    /**
     * Ensure concurrent submissions with same idempotency token don't create duplicates
     */
    const std::string TOKEN = "concurrent_token_001";
    const int THREAD_COUNT = 10;
    
    std::vector<std::thread> threads;
    std::vector<std::string> results;
    results.resize(THREAD_COUNT);
    
    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back([this, i, &TOKEN, &results]() {
            ImmutableAuditEntry entry = createTestEntry(i);
            results[i] = writer_->submitEntryIdempotent(entry, TOKEN);
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Exactly one should succeed
    int success_count = 0;
    int duplicate_count = 0;
    
    for (const auto& result : results) {
        if (result == "OK") success_count++;
        else if (result == "DUPLICATE") duplicate_count++;
    }
    
    EXPECT_EQ(success_count, 1) << "Exactly one submission should succeed";
    EXPECT_EQ(duplicate_count, THREAD_COUNT - 1) << "Others should be duplicates";
}

// ============================================================================
// Crash Recovery Tests
// ============================================================================

TEST_F(AuditBatchWriterStressTest, CheckpointGeneration) {
    /**
     * Wave C Criteria: Crash recovery with atomicity validation
     */
    const int ENTRIES = 5000;
    
    for (int i = 0; i < ENTRIES; ++i) {
        ImmutableAuditEntry entry = createTestEntry(i);
        writer_->submitEntry(entry);
    }
    
    writer_->forceFlush();
    
    auto checkpoints = writer_->getCheckpoints();
    EXPECT_GT(checkpoints.size(), 0) << "Should have checkpoints";
    
    for (const auto& cp : checkpoints) {
        EXPECT_FALSE(cp.checkpoint_id.empty());
        EXPECT_GT(cp.entry_count, 0);
        EXPECT_GT(cp.checkpoint_time_ms, 0);
    }
}

TEST_F(AuditBatchWriterStressTest, CheckpointVerification) {
    /**
     * Verify checkpoints are valid and can be used for recovery
     */
    const int ENTRIES = 2000;
    
    for (int i = 0; i < ENTRIES; ++i) {
        ImmutableAuditEntry entry = createTestEntry(i);
        writer_->submitEntry(entry);
    }
    
    writer_->forceFlush();
    
    auto checkpoints = writer_->getCheckpoints();
    ASSERT_GT(checkpoints.size(), 0);
    
    for (const auto& cp : checkpoints) {
        EXPECT_TRUE(writer_->verifyCheckpoint(cp))
            << "Checkpoint " << cp.checkpoint_id << " should be valid";
    }
}

// ============================================================================
// Backpressure and Buffer Management Tests
// ============================================================================

TEST_F(AuditBatchWriterStressTest, BufferBackpressure) {
    /**
     * Test backpressure handling when buffer is full
     */
    AuditBatchWriter::Config config;
    config.buffer_size = 100;  // Small buffer for testing
    config.enable_backpressure = true;
    config.flush_interval_ms = 1000;  // Slow flush
    
    auto bp_writer = std::make_unique<AuditBatchWriter>(manager_, config);
    ASSERT_EQ(bp_writer->start(), "OK");
    
    // Fill buffer
    int submitted = 0;
    int backpressured = 0;
    
    for (int i = 0; i < 500; ++i) {
        ImmutableAuditEntry entry = createTestEntry(i);
        std::string status = bp_writer->submitEntry(entry);
        
        if (status == "OK") {
            submitted++;
        } else if (status == "BUFFER_FULL") {
            backpressured++;
        }
    }
    
    EXPECT_GT(backpressured, 0) << "Should experience backpressure";
    EXPECT_LE(submitted, 100) << "Should not exceed buffer size";
    
    bp_writer->shutdown();
}

// ============================================================================
// Metrics and Reporting Tests
// ============================================================================

TEST_F(AuditBatchWriterStressTest, MetricsReporting) {
    const int ENTRIES = 1000;
    
    for (int i = 0; i < ENTRIES; ++i) {
        ImmutableAuditEntry entry = createTestEntry(i);
        writer_->submitEntry(entry);
    }
    
    writer_->forceFlush();
    
    auto metrics = writer_->getMetrics();
    EXPECT_EQ(metrics["total_entries_submitted"].get<int>(), ENTRIES);
    EXPECT_GT(metrics["total_batches_flushed"].get<int>(), 0);
    EXPECT_GE(metrics["avg_submission_latency_us"].get<double>(), 0);
}

TEST_F(AuditBatchWriterStressTest, BufferStats) {
    const int ENTRIES = 500;
    
    for (int i = 0; i < ENTRIES; ++i) {
        ImmutableAuditEntry entry = createTestEntry(i);
        writer_->submitEntry(entry);
    }
    
    auto stats = writer_->getBufferStats();
    EXPECT_GT(stats["pending_entries"].get<int>(), 0);
    EXPECT_GT(stats["buffer_capacity"].get<int>(), 0);
    EXPECT_LT(stats["buffer_fill_percentage"].get<double>(), 100);
}

}  // namespace tests
}  // namespace governance
}  // namespace themis

// ============================================================================
// Wave C Test Configuration
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    // Enable verbose logging for Wave C validation
    ::testing::FLAGS_gtest_repeat = 1;
    ::testing::FLAGS_gtest_shuffle = false;
    
    return RUN_ALL_TESTS();
}
