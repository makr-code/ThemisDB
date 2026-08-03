// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_storage_phase3_error_handling_focused.cpp
 * @brief Phase 3 storage module error handling and edge case focused tests.
 *
 * Validates Phase 3 implementation:
 *   - Standardized fail-safe behavior for replay faults
 *   - Unified diagnostics across persistence and recovery layers
 *   - Storage pressure mitigation and error handling
 *   - Edge case coverage for all error paths
 *
 * ## Test Cases
 *
 * ### SED-01..SED-08 — Error Diagnostics Classification
 *   SED-01  classifyErrorIncident() maps error codes to incident types
 *   SED-02  mapErrorMessage() handles common error string patterns
 *   SED-03  buildErrorContext() populates all context fields correctly
 *   SED-04  isDurabilityThreat() classification is accurate
 *   SED-05  Error severity levels are correctly assigned
 *   SED-06  Recovery suggestions are populated for each error type
 *   SED-07  emitDiagnosticEvent() produces structured output
 *   SED-08  Error name and description utilities work correctly
 *
 * ### SRF-01..SRF-08 — Recovery Fault Handling
 *   SRF-01  handleTornWalEntry() emits RECOVERY_INCOMPLETE
 *   SRF-02  handleInvalidCheckpoint() suggests fallback checkpoint
 *   SRF-03  handleRecoveryTimeout() stops recovery with RECOVERY_TIMEOUT
 *   SRF-04  handleWalFileCorruption() emits CRITICAL severity
 *   SRF-05  handleWalReadError() respects retry limits
 *   SRF-06  handleReplayEntryFailure() classifies by criticality
 *   SRF-07  handleRecoveryRetryExhausted() triggers manual intervention
 *   SRF-08  Recovery fault reports contain actionable suggestions
 *
 * ### SPM-01..SPM-08 — Storage Pressure Management
 *   SPM-01  getCapacityMetrics() returns current usage snapshot
 *   SPM-02  canAcceptWrite() rejects writes when exhausted
 *   SPM-03  canStartBackup() respects concurrent backup limit
 *   SPM-04  updateCapacity() classifies pressure levels correctly
 *   SPM-05  reportStorageExhausted() emits CRITICAL diagnostic
 *   SPM-06  reportBackupLimitExceeded() emits HIGH diagnostic
 *   SPM-07  Pressure escalation triggers operator alerts
 *   SPM-08  Write rejection at configured utilization threshold
 *
 * @see include/storage/storage_error_diagnostics.h
 * @see include/storage/storage_recovery_fault_handler.h
 * @see include/storage/storage_pressure_manager.h
 * @see src/storage/ROADMAP.md — Phase 3 items
 */

#include <gtest/gtest.h>

#include "storage/storage_api_contract.h"
#include "storage/storage_error_diagnostics.h"
#include "storage/storage_recovery_fault_handler.h"
#include "storage/storage_pressure_manager.h"

#include <chrono>
#include <string>
#include <vector>

using namespace themis::storage;
using namespace std::chrono_literals;

// ============================================================================
// Test Seed
// ============================================================================
static constexpr std::uint64_t kPhase3TestSeed = 2026;

// ============================================================================
// § 1  Error Diagnostics Classification Tests (SED-01..SED-08)
// ============================================================================

class StorageErrorDiagnosticsTest : public ::testing::Test {};

TEST_F(StorageErrorDiagnosticsTest, SED01_ClassifyWalWriteFailed) {
    auto ctx = classifyErrorIncident(StorageErrorCode::WAL_WRITE_FAILED);
    EXPECT_EQ(ctx.error_code, StorageErrorCode::WAL_WRITE_FAILED);
    EXPECT_EQ(ctx.incident_type, ErrorIncidentType::RECOVERY_FAULT);
    EXPECT_EQ(ctx.severity, ErrorSeverity::CRITICAL);
    EXPECT_TRUE(ctx.is_durability_threat);
    EXPECT_FALSE(ctx.is_retryable);
}

TEST_F(StorageErrorDiagnosticsTest, SED02_ClassifyTransactionConflict) {
    auto ctx = classifyErrorIncident(StorageErrorCode::TRANSACTION_CONFLICT);
    EXPECT_EQ(ctx.incident_type, ErrorIncidentType::TRANSACTION_CONFLICT);
    EXPECT_EQ(ctx.severity, ErrorSeverity::MEDIUM);
    EXPECT_FALSE(ctx.is_durability_threat);
    EXPECT_TRUE(ctx.is_retryable);
    EXPECT_EQ(ctx.retry_count, 3);
}

TEST_F(StorageErrorDiagnosticsTest, SED03_MapErrorMessageStorageExhausted) {
    StorageErrorCode code1 = mapErrorMessage("Disk is full");
    EXPECT_EQ(code1, StorageErrorCode::STORAGE_EXHAUSTED);

    StorageErrorCode code2 = mapErrorMessage("NO SPACE LEFT");
    EXPECT_EQ(code2, StorageErrorCode::STORAGE_EXHAUSTED);

    StorageErrorCode code3 = mapErrorMessage("capacity exhausted");
    EXPECT_EQ(code3, StorageErrorCode::STORAGE_EXHAUSTED);
}

TEST_F(StorageErrorDiagnosticsTest, SED04_MapErrorMessageCorruption) {
    StorageErrorCode code1 = mapErrorMessage("File corrupted");
    EXPECT_EQ(code1, StorageErrorCode::WAL_CORRUPTED);

    StorageErrorCode code2 = mapErrorMessage("Checksum mismatch");
    EXPECT_EQ(code2, StorageErrorCode::WAL_CORRUPTED);
}

TEST_F(StorageErrorDiagnosticsTest, SED05_MapErrorMessageTimeout) {
    StorageErrorCode code = mapErrorMessage("Operation timed out");
    EXPECT_EQ(code, StorageErrorCode::RECOVERY_TIMEOUT);
}

TEST_F(StorageErrorDiagnosticsTest, SED06_BuildErrorContextWithSuggestions) {
    auto ctx = buildErrorContext(
        StorageErrorCode::STORAGE_EXHAUSTED,
        "checkpoint()",
        "/nvme/themis.wal"
    );
    EXPECT_EQ(ctx.error_code, StorageErrorCode::STORAGE_EXHAUSTED);
    EXPECT_EQ(ctx.operation_context, "checkpoint()");
    EXPECT_EQ(ctx.affected_resource, "/nvme/themis.wal");
    EXPECT_FALSE(ctx.recovery_suggestion.empty());
    EXPECT_TRUE(ctx.recovery_suggestion.find("Free disk space") != std::string::npos);
}

TEST_F(StorageErrorDiagnosticsTest, SED07_BuildErrorContextRecoveryIncomplete) {
    auto ctx = buildErrorContext(
        StorageErrorCode::RECOVERY_INCOMPLETE,
        "recover()"
    );
    EXPECT_EQ(ctx.severity, ErrorSeverity::HIGH);
    EXPECT_TRUE(ctx.recovery_suggestion.find("backup") != std::string::npos ||
                ctx.recovery_suggestion.find("Partial") != std::string::npos);
}

TEST_F(StorageErrorDiagnosticsTest, SED08_ErrorNameAndDescription) {
    EXPECT_EQ(errorCodeName(StorageErrorCode::WAL_WRITE_FAILED), "WAL_WRITE_FAILED");
    EXPECT_EQ(errorCodeName(StorageErrorCode::STORAGE_EXHAUSTED), "STORAGE_EXHAUSTED");
    EXPECT_FALSE(errorCodeDescription(StorageErrorCode::WAL_WRITE_FAILED).empty());
    EXPECT_EQ(incidentTypeName(ErrorIncidentType::RECOVERY_FAULT), "RECOVERY_FAULT");
    EXPECT_EQ(severityName(ErrorSeverity::CRITICAL), "CRITICAL");
}

// ============================================================================
// § 2  Recovery Fault Handler Tests (SRF-01..SRF-08)
// ============================================================================

class RecoveryFaultHandlerTest : public ::testing::Test {
protected:
    RecoveryFaultHandler handler;
};

TEST_F(RecoveryFaultHandlerTest, SRF01_HandleTornWalEntry) {
    auto report = handler.handleTornWalEntry(
        "/nvme/themis.wal",
        100,  // checkpoint_seq
        200,  // fault_seq
        99    // entries_replayed
    );
    EXPECT_EQ(report.fault_type, RecoveryFaultType::TORN_WAL_ENTRY);
    EXPECT_EQ(report.error_code, StorageErrorCode::RECOVERY_INCOMPLETE);
    EXPECT_EQ(report.entries_replayed, 99);
    EXPECT_FALSE(report.recovery_suggestion.empty());
}

TEST_F(RecoveryFaultHandlerTest, SRF02_HandleInvalidCheckpoint) {
    auto report = handler.handleInvalidCheckpoint(
        150,  // checkpoint_seq (invalid)
        120,  // wal_tail_seq
        100   // recovery_start_seq (fallback)
    );
    EXPECT_EQ(report.fault_type, RecoveryFaultType::CHECKPOINT_INVALID);
    EXPECT_EQ(report.error_code, StorageErrorCode::CHECKPOINT_FAILED);
    EXPECT_TRUE(report.should_stop_recovery);
    EXPECT_TRUE(report.is_retryable);
}

TEST_F(RecoveryFaultHandlerTest, SRF03_HandleRecoveryTimeout) {
    auto report = handler.handleRecoveryTimeout(
        600ms,  // elapsed_time exceeds timeout
        5000,   // entries_replayed
        0       // checkpoint_seq
    );
    EXPECT_EQ(report.error_code, StorageErrorCode::RECOVERY_TIMEOUT);
    EXPECT_TRUE(report.should_stop_recovery);
    EXPECT_TRUE(report.is_retryable);
}

TEST_F(RecoveryFaultHandlerTest, SRF04_HandleWalFileCorruption) {
    auto report = handler.handleWalFileCorruption(
        "/nvme/themis.wal",
        "invalid_magic",
        1000  // bytes_verified
    );
    EXPECT_EQ(report.error_code, StorageErrorCode::WAL_CORRUPTED);
    EXPECT_TRUE(report.should_stop_recovery);
    EXPECT_FALSE(report.is_retryable);
}

TEST_F(RecoveryFaultHandlerTest, SRF05_HandleWalReadErrorWithRetries) {
    // First retry should be accepted
    auto report1 = handler.handleWalReadError("/nvme/themis.wal", "permission denied", 0);
    EXPECT_TRUE(report1.is_retryable);

    // After max retries, should not be retryable
    auto report2 = handler.handleWalReadError("/nvme/themis.wal", "permission denied", 3);
    EXPECT_FALSE(report2.is_retryable);
}

TEST_F(RecoveryFaultHandlerTest, SRF06_HandleReplayEntryFailureNonCritical) {
    auto report = handler.handleReplayEntryFailure(
        500,
        "unknown key format",
        false  // non-critical
    );
    EXPECT_FALSE(report.should_stop_recovery);
    EXPECT_TRUE(report.is_retryable);
}

TEST_F(RecoveryFaultHandlerTest, SRF07_HandleReplayEntryFailureCritical) {
    auto report = handler.handleReplayEntryFailure(
        500,
        "transaction violates invariant",
        true  // critical
    );
    EXPECT_TRUE(report.should_stop_recovery);
    EXPECT_FALSE(report.is_retryable);
}

TEST_F(RecoveryFaultHandlerTest, SRF08_HandleRecoveryRetryExhausted) {
    auto report = handler.handleRecoveryRetryExhausted(
        100,
        3,
        "persistent I/O errors"
    );
    EXPECT_EQ(report.fault_type, RecoveryFaultType::RECOVERY_RETRY_EXHAUSTED);
    EXPECT_TRUE(report.should_stop_recovery);
    EXPECT_FALSE(report.is_retryable);
}

// ============================================================================
// § 3  Storage Pressure Manager Tests (SPM-01..SPM-08)
// ============================================================================

class StoragePressureManagerTest : public ::testing::Test {
protected:
    StoragePressureManager mgr;
};

TEST_F(StoragePressureManagerTest, SPM01_GetCapacityMetricsNormal) {
    mgr.updateCapacity(1000000, 500000);  // 50% utilization
    auto metrics = mgr.getCapacityMetrics();
    EXPECT_EQ(metrics.total_capacity_bytes, 1000000);
    EXPECT_EQ(metrics.used_bytes, 500000);
    EXPECT_EQ(metrics.escalation_level, PressureEscalationLevel::NORMAL);
}

TEST_F(StoragePressureManagerTest, SPM02_CanAcceptWriteNormal) {
    mgr.updateCapacity(1000000, 500000);  // 50% utilization
    EXPECT_TRUE(mgr.canAcceptWrite(100000));
}

TEST_F(StoragePressureManagerTest, SPM03_CanAcceptWriteWhenExhausted) {
    mgr.updateCapacity(1000000, 995000);  // 99.5% utilization
    EXPECT_FALSE(mgr.canAcceptWrite(1000));
}

TEST_F(StoragePressureManagerTest, SPM04_PressureEscalation) {
    mgr.updateCapacity(1000000, 700000);  // 70% — should be NORMAL
    EXPECT_EQ(mgr.getCapacityMetrics().escalation_level, PressureEscalationLevel::NORMAL);

    mgr.updateCapacity(1000000, 800000);  // 80% — should be WARNING
    EXPECT_EQ(mgr.getCapacityMetrics().escalation_level, PressureEscalationLevel::WARNING);

    mgr.updateCapacity(1000000, 900000);  // 90% — should be ALERT
    EXPECT_EQ(mgr.getCapacityMetrics().escalation_level, PressureEscalationLevel::ALERT);

    mgr.updateCapacity(1000000, 970000);  // 97% — should be CRITICAL
    EXPECT_EQ(mgr.getCapacityMetrics().escalation_level, PressureEscalationLevel::CRITICAL);

    mgr.updateCapacity(1000000, 995000);  // 99.5% — should be EXHAUSTED
    EXPECT_EQ(mgr.getCapacityMetrics().escalation_level, PressureEscalationLevel::EXHAUSTED);
}

TEST_F(StoragePressureManagerTest, SPM05_ReportStorageExhausted) {
    auto ctx = mgr.reportStorageExhausted("write()", 1000000, 995000);
    EXPECT_EQ(ctx.error_code, StorageErrorCode::STORAGE_EXHAUSTED);
    EXPECT_EQ(ctx.severity, ErrorSeverity::CRITICAL);
    EXPECT_TRUE(ctx.is_durability_threat);
    EXPECT_FALSE(ctx.recovery_suggestion.empty());
}

TEST_F(StoragePressureManagerTest, SPM06_CanStartBackupNormal) {
    mgr.updateCapacity(1000000, 500000);  // 50% utilization
    EXPECT_TRUE(mgr.canStartBackup(100000, 0));  // 0 active backups
}

TEST_F(StoragePressureManagerTest, SPM07_CanStartBackupLimitExceeded) {
    mgr.updateCapacity(1000000, 500000);
    EXPECT_FALSE(mgr.canStartBackup(100000, kMaxConcurrentBackups));
}

TEST_F(StoragePressureManagerTest, SPM08_ReportBackupLimitExceeded) {
    auto ctx = mgr.reportBackupLimitExceeded(
        kMaxConcurrentBackups,
        kMaxConcurrentBackups
    );
    EXPECT_EQ(ctx.error_code, StorageErrorCode::BACKUP_LIMIT_EXCEEDED);
    EXPECT_EQ(ctx.severity, ErrorSeverity::HIGH);
    EXPECT_TRUE(ctx.is_retryable);
}

// ============================================================================
// § 4  Integration Tests
// ============================================================================

class StoragePhase3IntegrationTest : public ::testing::Test {};

TEST_F(StoragePhase3IntegrationTest, E2E_RecoveryFaultDiagnosticsFlow) {
    RecoveryFaultHandler fault_handler;
    
    // Simulate torn WAL entry during recovery
    auto fault_report = fault_handler.handleTornWalEntry(
        "/data/wal",
        100,
        150,
        100
    );
    
    EXPECT_EQ(fault_report.error_code, StorageErrorCode::RECOVERY_INCOMPLETE);
    
    // Build diagnostic context
    auto error_ctx = buildErrorContext(
        fault_report.error_code,
        fault_report.fault_message,
        "/data/wal"
    );
    
    EXPECT_EQ(error_ctx.severity, ErrorSeverity::HIGH);
    EXPECT_TRUE(error_ctx.is_durability_threat);
}

TEST_F(StoragePhase3IntegrationTest, E2E_CapacityManagementFlow) {
    StoragePressureManager mgr;
    
    // Start with normal capacity
    mgr.updateCapacity(10000000, 5000000);  // 50%
    EXPECT_TRUE(mgr.canAcceptWrite(1000000));
    
    // Gradually increase utilization
    mgr.updateCapacity(10000000, 8000000);  // 80%
    auto metrics = mgr.getCapacityMetrics();
    EXPECT_EQ(metrics.escalation_level, PressureEscalationLevel::WARNING);
    EXPECT_TRUE(mgr.canAcceptWrite(100000));
    
    // Reach critical level
    mgr.updateCapacity(10000000, 9600000);  // 96%
    metrics = mgr.getCapacityMetrics();
    EXPECT_EQ(metrics.escalation_level, PressureEscalationLevel::CRITICAL);
    
    // Report storage exhausted
    auto ctx = mgr.reportStorageExhausted("flush()", 10000000, 9950000);
    EXPECT_EQ(ctx.error_code, StorageErrorCode::STORAGE_EXHAUSTED);
    EXPECT_EQ(ctx.severity, ErrorSeverity::CRITICAL);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
