/**
 * @file test_governance_phase2c_lineage_backpressure_focused.cpp
 * @brief Comprehensive tests for Phase 2C: Lineage Backpressure (circuit breaker, size limits, error semantics)
 * @version 1.0.0
 */

#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <memory>
#include <chrono>

#include "governance/data_lineage.h"
#include "governance/governance_diagnostics.h"
#include "utils/audit_logger.h"

namespace themis::governance::test {

// ─── Mock AuditLogger for Testing ───────────────────────────────────────────

class MockAuditLogger : public themis::utils::AuditLogger {
public:
    int32_t failure_count = 0;
    int32_t fail_after = -1;  // -1 means never fail, 0 means always fail
    int32_t call_count = 0;
    
    void logEvent(const nlohmann::json& event) override {
        call_count++;
        if (fail_after >= 0 && call_count > fail_after) {
            throw std::runtime_error("Mock audit logger failure for testing");
        }
    }
    
    nlohmann::json getEvents(int64_t start_ms = 0, int64_t end_ms = 0, 
                             const std::string& filter = "") const override {
        return nlohmann::json::array();
    }
    
    size_t getTotalEventCount() const override { return call_count; }
    
    void clear() override { call_count = 0; }
};

// ─── Test Fixture ───────────────────────────────────────────────────────────

class LineageBackpressureTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear global diagnostics for each test
        getGlobalDiagnosticAggregator().clear();
        tracker_ = std::make_shared<DataLineageTracker>();
    }
    
    void TearDown() override {
        tracker_.reset();
    }
    
    std::shared_ptr<DataLineageTracker> tracker_;
    
    // Helper to create a test event
    LineageEvent createTestEvent(const std::string& dataset_id, 
                                 const std::string& event_id = "",
                                 int64_t timestamp_ms = 0) {
        LineageEvent event;
        event.dataset_id = dataset_id;
        event.event_id = event_id;
        event.timestamp_ms = timestamp_ms;
        event.event_type = LineageEventType::INGESTION;
        event.performed_by = "test_user";
        event.operation = "test_operation";
        return event;
    }
};

// ─── LB-01: LineageRecordResult Error Semantics ───────────────────────────────

TEST_F(LineageBackpressureTest, LB_01_ErrorCodeValues) {
    // Test error code values are as expected
    EXPECT_EQ(static_cast<int32_t>(LineageError::kSuccess), 7360);
    EXPECT_EQ(static_cast<int32_t>(LineageError::kAuditLoggerFailure), 7361);
    EXPECT_EQ(static_cast<int32_t>(LineageError::kSizeLimitExceeded), 7362);
    EXPECT_EQ(static_cast<int32_t>(LineageError::kMemoryPressure), 7363);
    EXPECT_EQ(static_cast<int32_t>(LineageError::kCircuitBreakerOpen), 7364);
    EXPECT_EQ(static_cast<int32_t>(LineageError::kEventSequenceViolation), 7365);
}

TEST_F(LineageBackpressureTest, LB_01_IsSuccessCheck) {
    LineageRecordResult success_result;
    success_result.error = LineageError::kSuccess;
    EXPECT_TRUE(success_result.isSuccess());
    
    LineageRecordResult failure_result;
    failure_result.error = LineageError::kAuditLoggerFailure;
    EXPECT_FALSE(failure_result.isSuccess());
}

TEST_F(LineageBackpressureTest, LB_01_GetErrorNameCompleteness) {
    struct {
        LineageError error;
        std::string expected_name;
    } test_cases[] = {
        {LineageError::kSuccess, "SUCCESS"},
        {LineageError::kAuditLoggerFailure, "AUDIT_LOGGER_FAILURE"},
        {LineageError::kSizeLimitExceeded, "SIZE_LIMIT_EXCEEDED"},
        {LineageError::kMemoryPressure, "MEMORY_PRESSURE"},
        {LineageError::kCircuitBreakerOpen, "CIRCUIT_BREAKER_OPEN"},
        {LineageError::kEventSequenceViolation, "EVENT_SEQUENCE_VIOLATION"},
    };
    
    for (const auto& tc : test_cases) {
        LineageRecordResult result;
        result.error = tc.error;
        EXPECT_EQ(result.getErrorName(), tc.expected_name)
            << "Error name mismatch for code " << static_cast<int32_t>(tc.error);
    }
}

// ─── LB-02: Circuit Breaker State Transitions ────────────────────────────────

TEST_F(LineageBackpressureTest, LB_02_InitialStateIsClosed) {
    EXPECT_EQ(tracker_->getCircuitBreakerState(), CircuitBreakerState::CLOSED);
}

TEST_F(LineageBackpressureTest, LB_02_ClosedToOpenOnFailures) {
    auto mock_logger = std::make_shared<MockAuditLogger>();
    mock_logger->fail_after = 0;  // Fail immediately
    tracker_->setAuditLogger(mock_logger);
    tracker_->setCircuitBreakerThreshold(3);
    
    // Record 3 events to trigger 3 failures
    for (int i = 0; i < 3; ++i) {
        auto event = createTestEvent("dataset_1", "event_" + std::to_string(i));
        auto result = tracker_->recordEvent(event);
        EXPECT_FALSE(result.isSuccess());
    }
    
    // Circuit breaker should be OPEN now
    EXPECT_EQ(tracker_->getCircuitBreakerState(), CircuitBreakerState::OPEN);
}

TEST_F(LineageBackpressureTest, LB_02_OpenToHalfOpenAfterTimeout) {
    auto mock_logger = std::make_shared<MockAuditLogger>();
    mock_logger->fail_after = 0;
    tracker_->setAuditLogger(mock_logger);
    tracker_->setCircuitBreakerThreshold(1);
    tracker_->setCircuitBreakerRecoveryWindowMs(100);  // 100ms recovery window
    
    // Trigger failure to open circuit
    auto event = createTestEvent("dataset_1", "event_fail");
    tracker_->recordEvent(event);
    EXPECT_EQ(tracker_->getCircuitBreakerState(), CircuitBreakerState::OPEN);
    
    // Wait for recovery window to elapse
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    
    // Next event should transition to HALF_OPEN
    auto event2 = createTestEvent("dataset_1", "event_2");
    tracker_->recordEvent(event2);
    EXPECT_EQ(tracker_->getCircuitBreakerState(), CircuitBreakerState::HALF_OPEN);
}

TEST_F(LineageBackpressureTest, LB_02_HalfOpenToClosedOnSuccess) {
    auto mock_logger = std::make_shared<MockAuditLogger>();
    tracker_->setAuditLogger(mock_logger);
    tracker_->setCircuitBreakerThreshold(1);
    tracker_->setCircuitBreakerRecoveryWindowMs(100);
    
    // Force to HALF_OPEN state
    tracker_->recordAuditFailure();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    
    // Trigger state update to HALF_OPEN
    auto event = createTestEvent("dataset_1", "event_1");
    tracker_->recordEvent(event);
    EXPECT_EQ(tracker_->getCircuitBreakerState(), CircuitBreakerState::HALF_OPEN);
    
    // Record success to transition to CLOSED
    tracker_->recordAuditSuccess();
    EXPECT_EQ(tracker_->getCircuitBreakerState(), CircuitBreakerState::CLOSED);
}

// ─── LB-03: Audit Failure Recovery ──────────────────────────────────────────

TEST_F(LineageBackpressureTest, LB_03_EventsRecordedLocallyWhenCircuitOpen) {
    auto mock_logger = std::make_shared<MockAuditLogger>();
    mock_logger->fail_after = 0;
    tracker_->setAuditLogger(mock_logger);
    tracker_->setCircuitBreakerThreshold(1);
    
    // Trigger circuit breaker to open
    auto event1 = createTestEvent("dataset_1", "event_1");
    tracker_->recordEvent(event1);
    
    EXPECT_EQ(tracker_->getCircuitBreakerState(), CircuitBreakerState::OPEN);
    
    // Record another event (should succeed locally despite circuit open)
    auto event2 = createTestEvent("dataset_1", "event_2");
    auto result = tracker_->recordEvent(event2);
    
    // Event recorded but audit not forwarded
    EXPECT_EQ(result.error, LineageError::kCircuitBreakerOpen);
    EXPECT_EQ(tracker_->totalEventCount(), 2);
}

TEST_F(LineageBackpressureTest, LB_03_DiagnosticsEmittedOnFailure) {
    auto mock_logger = std::make_shared<MockAuditLogger>();
    mock_logger->fail_after = 0;
    tracker_->setAuditLogger(mock_logger);
    tracker_->setCircuitBreakerThreshold(1);
    
    auto& diag_agg = getGlobalDiagnosticAggregator();
    diag_agg.clear();
    
    auto event = createTestEvent("dataset_1", "event_1");
    tracker_->recordEvent(event);
    
    // Circuit breaker diagnostics should be recorded
    auto diagnostics = diag_agg.getDiagnosticsForCode(
        static_cast<GovDiagnosticCode>(LineageError::kCircuitBreakerOpen));
    EXPECT_GT(diagnostics.size(), 0);
    EXPECT_EQ(diagnostics[0].component, "lineage_tracker");
}

// ─── LB-04: Size Limit Enforcement ──────────────────────────────────────────

TEST_F(LineageBackpressureTest, LB_04_RecordUpToLimit) {
    tracker_->setMaxTotalEvents(100);
    
    // Record up to the limit
    for (int i = 0; i < 100; ++i) {
        auto event = createTestEvent("dataset_1", "event_" + std::to_string(i),
                                     static_cast<int64_t>(i * 1000));
        auto result = tracker_->recordEvent(event);
        EXPECT_TRUE(result.isSuccess()) << "Event " << i << " failed";
    }
    
    EXPECT_EQ(tracker_->totalEventCount(), 100);
}

TEST_F(LineageBackpressureTest, LB_04_SizeLimitEnforcedWithFIFOEviction) {
    tracker_->setMaxTotalEvents(50);
    
    // Record 100 events (should trigger FIFO eviction)
    for (int i = 0; i < 100; ++i) {
        auto event = createTestEvent("dataset_1", "event_" + std::to_string(i),
                                     static_cast<int64_t>(i * 1000));
        tracker_->recordEvent(event);
    }
    
    // Should only have 50 events (oldest 50 removed via FIFO)
    EXPECT_EQ(tracker_->totalEventCount(), 50);
    
    // Verify oldest events are gone
    auto lineage = tracker_->getLineage("dataset_1");
    EXPECT_FALSE(lineage.events.empty());
    
    // First remaining event should have been event_50 or later
    EXPECT_GE(lineage.events[0].event_id, "event_50");
}

// ─── LB-05: Concurrent Event Recording Under Backpressure ─────────────────

TEST_F(LineageBackpressureTest, LB_05_ConcurrentRecordingNoRaces) {
    tracker_->setMaxTotalEvents(1000);
    const int kThreadCount = 10;
    const int kEventsPerThread = 100;
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < kThreadCount; ++t) {
        threads.emplace_back([this, t]() {
            for (int i = 0; i < kEventsPerThread; ++i) {
                auto event = createTestEvent(
                    "dataset_" + std::to_string(t),
                    "event_t" + std::to_string(t) + "_" + std::to_string(i));
                auto result = tracker_->recordEvent(event);
                EXPECT_EQ(result.error, LineageError::kSuccess);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // All events should be recorded
    EXPECT_EQ(tracker_->totalEventCount(), kThreadCount * kEventsPerThread);
}

// ─── LB-06: Statistics Aggregation ──────────────────────────────────────────

TEST_F(LineageBackpressureTest, LB_06_StatisticsSnapshot) {
    auto mock_logger = std::make_shared<MockAuditLogger>();
    tracker_->setAuditLogger(mock_logger);
    
    // Record events to multiple datasets
    for (int d = 0; d < 5; ++d) {
        for (int i = 0; i < 10; ++i) {
            auto event = createTestEvent("dataset_" + std::to_string(d),
                                        "event_" + std::to_string(i));
            tracker_->recordEvent(event);
        }
    }
    
    auto stats = tracker_->getStatistics();
    EXPECT_EQ(stats.total_events, 50);
    EXPECT_EQ(stats.total_datasets, 5);
    EXPECT_EQ(stats.circuit_breaker_state, CircuitBreakerState::CLOSED);
    EXPECT_EQ(stats.last_error_code, static_cast<int32_t>(LineageError::kSuccess));
    EXPECT_GT(stats.timestamp_ms, 0);
}

TEST_F(LineageBackpressureTest, LB_06_StatisticsReflectErrors) {
    auto mock_logger = std::make_shared<MockAuditLogger>();
    mock_logger->fail_after = 0;
    tracker_->setAuditLogger(mock_logger);
    tracker_->setCircuitBreakerThreshold(1);
    
    // Trigger an error
    auto event = createTestEvent("dataset_1", "event_1");
    tracker_->recordEvent(event);
    
    auto stats = tracker_->getStatistics();
    EXPECT_EQ(stats.circuit_breaker_state, CircuitBreakerState::OPEN);
    EXPECT_EQ(stats.last_error_code, static_cast<int32_t>(LineageError::kAuditLoggerFailure));
}

// ─── LB-07: Manual Pruning and GC ───────────────────────────────────────────

TEST_F(LineageBackpressureTest, LB_07_PruneOldEventsFIFO) {
    // Record 100 events
    for (int i = 0; i < 100; ++i) {
        auto event = createTestEvent("dataset_1", "event_" + std::to_string(i),
                                     static_cast<int64_t>(i * 1000));
        tracker_->recordEvent(event);
    }
    
    EXPECT_EQ(tracker_->totalEventCount(), 100);
    
    // Prune to keep only 50
    auto result = tracker_->pruneOldEvents("dataset_1", 50);
    EXPECT_TRUE(result.isSuccess());
    EXPECT_EQ(result.event_count, 50);
    
    // Verify total events updated
    EXPECT_EQ(tracker_->totalEventCount(), 50);
}

TEST_F(LineageBackpressureTest, LB_07_PruneRecordsInAuditTrail) {
    // Record events
    for (int i = 0; i < 100; ++i) {
        auto event = createTestEvent("dataset_1", "event_" + std::to_string(i),
                                     static_cast<int64_t>(i * 1000));
        tracker_->recordEvent(event);
    }
    
    auto& diag_agg = getGlobalDiagnosticAggregator();
    diag_agg.clear();
    
    // Prune and check diagnostics
    tracker_->pruneOldEvents("dataset_1", 25);
    
    auto diagnostics = diag_agg.getDiagnosticsForCode(
        GovDiagnosticCode::kLineageBackpressure);
    EXPECT_GT(diagnostics.size(), 0);
    EXPECT_EQ(diagnostics[0].component, "lineage_tracker");
}

// ─── LB-08: Error Code Taxonomy Completeness ────────────────────────────────

TEST_F(LineageBackpressureTest, LB_08_AllErrorCodesMapped) {
    LineageError error_codes[] = {
        LineageError::kSuccess,
        LineageError::kAuditLoggerFailure,
        LineageError::kSizeLimitExceeded,
        LineageError::kMemoryPressure,
        LineageError::kCircuitBreakerOpen,
        LineageError::kEventSequenceViolation,
    };
    
    for (auto error : error_codes) {
        LineageRecordResult result;
        result.error = error;
        std::string name = result.getErrorName();
        EXPECT_FALSE(name.empty());
        EXPECT_NE(name, "UNKNOWN");
    }
}

TEST_F(LineageBackpressureTest, LB_08_NoDuplicateErrorCodes) {
    // Verify all codes are unique
    int32_t codes[] = {
        static_cast<int32_t>(LineageError::kSuccess),
        static_cast<int32_t>(LineageError::kAuditLoggerFailure),
        static_cast<int32_t>(LineageError::kSizeLimitExceeded),
        static_cast<int32_t>(LineageError::kMemoryPressure),
        static_cast<int32_t>(LineageError::kCircuitBreakerOpen),
        static_cast<int32_t>(LineageError::kEventSequenceViolation),
    };
    
    for (size_t i = 0; i < 6; ++i) {
        for (size_t j = i + 1; j < 6; ++j) {
            EXPECT_NE(codes[i], codes[j]);
        }
    }
}

// ─── LB-09: Diagnostics Emission Integration ────────────────────────────────

TEST_F(LineageBackpressureTest, LB_09_DiagnosticsRecordedInAggregator) {
    auto mock_logger = std::make_shared<MockAuditLogger>();
    mock_logger->fail_after = 0;
    tracker_->setAuditLogger(mock_logger);
    tracker_->setCircuitBreakerThreshold(1);
    
    auto& diag_agg = getGlobalDiagnosticAggregator();
    diag_agg.clear();
    
    // Record event with audit failure
    auto event = createTestEvent("dataset_1", "event_1");
    tracker_->recordEvent(event);
    
    // Verify diagnostic recorded
    auto total_diags = diag_agg.getTotalCount();
    EXPECT_GT(total_diags, 0);
}

TEST_F(LineageBackpressureTest, LB_09_DiagnosticCodesMatch) {
    auto mock_logger = std::make_shared<MockAuditLogger>();
    mock_logger->fail_after = 0;
    tracker_->setAuditLogger(mock_logger);
    tracker_->setCircuitBreakerThreshold(1);
    
    auto& diag_agg = getGlobalDiagnosticAggregator();
    diag_agg.clear();
    
    auto event = createTestEvent("dataset_1", "event_1");
    tracker_->recordEvent(event);
    
    auto diagnostics = diag_agg.getDiagnosticsForCode(
        static_cast<GovDiagnosticCode>(LineageError::kCircuitBreakerOpen));
    EXPECT_GT(diagnostics.size(), 0);
    
    // Verify remediation steps provided
    EXPECT_GT(diagnostics[0].remediation_steps.size(), 0);
}

TEST_F(LineageBackpressureTest, LB_09_RemediationStepsProvided) {
    auto mock_logger = std::make_shared<MockAuditLogger>();
    mock_logger->fail_after = 0;
    tracker_->setAuditLogger(mock_logger);
    tracker_->setCircuitBreakerThreshold(1);
    
    auto& diag_agg = getGlobalDiagnosticAggregator();
    diag_agg.clear();
    
    auto event = createTestEvent("dataset_1", "event_1");
    tracker_->recordEvent(event);
    
    auto diagnostics = diag_agg.getDiagnosticsForCode(
        static_cast<GovDiagnosticCode>(LineageError::kCircuitBreakerOpen));
    EXPECT_GT(diagnostics.size(), 0);
    EXPECT_GT(diagnostics[0].remediation_steps.size(), 0);
}

// ─── LB-10: FIFO Eviction Correctness Under High Volume ────────────────────

TEST_F(LineageBackpressureTest, LB_10_FIFOEvictionHighVolume) {
    tracker_->setMaxEventsPerDataset(1000);
    tracker_->setMaxTotalEvents(1000);
    
    // Record 5000 events
    for (int i = 0; i < 5000; ++i) {
        auto event = createTestEvent("dataset_1", "event_" + std::to_string(i),
                                     static_cast<int64_t>(i * 100));
        tracker_->recordEvent(event);
    }
    
    // Verify exactly 1000 remain (oldest 4000 removed)
    EXPECT_EQ(tracker_->totalEventCount(), 1000);
    
    // Verify oldest remaining event is event_4000 or later
    auto lineage = tracker_->getLineage("dataset_1");
    EXPECT_EQ(lineage.events.size(), 1000);
    
    // First event should be from the recent batch (event_4000+)
    EXPECT_GE(lineage.events.front().event_id, "event_4000");
}

TEST_F(LineageBackpressureTest, LB_10_VerifyNoMissingOrDuplicateEvents) {
    const int kTotalEvents = 500;
    tracker_->setMaxTotalEvents(kTotalEvents);
    
    // Record kTotalEvents + 200 (should keep last kTotalEvents)
    for (int i = 0; i < kTotalEvents + 200; ++i) {
        auto event = createTestEvent("dataset_1", "event_" + std::to_string(i),
                                     static_cast<int64_t>(i * 1000));
        tracker_->recordEvent(event);
    }
    
    auto lineage = tracker_->getLineage("dataset_1");
    
    // Should have exactly kTotalEvents
    EXPECT_EQ(lineage.events.size(), static_cast<size_t>(kTotalEvents));
    
    // Verify no duplicates (all event_ids should be unique)
    std::unordered_set<std::string> event_ids = {};

    for (const auto& e : lineage.events) {
        EXPECT_EQ(event_ids.count(e.event_id), 0) 
            << "Duplicate event_id: " << e.event_id;
        event_ids.insert(e.event_id);
    }
    
    // Verify chronological order
    for (size_t i = 1; i < lineage.events.size(); ++i) {
        EXPECT_LE(lineage.events[i - 1].timestamp_ms, lineage.events[i].timestamp_ms)
            << "Events not in chronological order";
    }
}

} // namespace themis::governance::test
