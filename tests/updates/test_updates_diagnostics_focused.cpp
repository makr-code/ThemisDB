/**
 * @file test_updates_diagnostics_focused.cpp
 * @brief Phase A Item 2: Focused tests for diagnostics consistency and error taxonomy
 * @version 1.0.0
 * @since 1.8.1 (Q3 2026)
 *
 * Coverage:
 *  - Error code taxonomy [7400-7499]
 *  - Root cause classification
 *  - Severity level mapping
 *  - Structured error context with JSON serialization
 *  - DiagnosticEmitter thread-safe event emission
 *  - Listener pattern for diagnostic events
 *  - Human-readable message formatting
 *
 * Test suite: 15+ focused tests covering:
 *  - DIA-01 to DIA-15: Diagnostics and error taxonomy
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <gtest/gtest.h>
#include "updates/updates_diagnostics.h"
#include "updates/updates_diagnostic_emitter.h"

#include <thread>
#include <atomic>
#include <memory>

using namespace themis::updates;

// ============================================================================
// Test Fixtures
// ============================================================================

class DiagnosticsTest : public ::testing::Test {};

class DiagnosticEmitterTest : public ::testing::Test {
protected:
    // Test listener that collects events
    class TestListener : public DiagnosticListener {
    public:
        struct Event {
            ErrorContext context;
            bool is_error;
        };
        
        std::vector<Event> events;
        std::mutex mutex;
        
        void onDiagnosticEvent(const ErrorContext& context, bool is_error) override {
            std::lock_guard<std::mutex> lock(mutex);
            events.push_back({context, is_error});
        }
        
        size_t eventCount() const {
            std::lock_guard<std::mutex> lock(mutex);
            return events.size();
        }
    };
    
    DiagnosticEmitter emitter_;
    std::shared_ptr<TestListener> test_listener_;
    
    void SetUp() override {
        test_listener_ = std::make_shared<TestListener>();
        emitter_.addListener(test_listener_);
    }
};

// ============================================================================
// Item 2: Error Taxonomy Tests (DIA-01 to DIA-15)
// ============================================================================

// DIA-01: Error code range validation
TEST_F(DiagnosticsTest, DIA_01_ErrorCodeRangeValidation) {
    // All codes should be in [7400-7499]
    EXPECT_EQ(static_cast<uint16_t>(DiagnosticErrorCode::STATE_INVALID_TRANSITION), 7400);
    EXPECT_EQ(static_cast<uint16_t>(DiagnosticErrorCode::ROLLBACK_NO_CHECKPOINTS), 7421);
    EXPECT_EQ(static_cast<uint16_t>(DiagnosticErrorCode::PATCH_APPLY_FAILED), 7440);
    EXPECT_EQ(static_cast<uint16_t>(DiagnosticErrorCode::NETWORK_PARTITION), 7460);
    EXPECT_EQ(static_cast<uint16_t>(DiagnosticErrorCode::CASCADE_DETECTED), 7480);
    EXPECT_EQ(static_cast<uint16_t>(DiagnosticErrorCode::UNKNOWN_ERROR), 7499);
}

// DIA-02: Severity mapping for error codes
TEST_F(DiagnosticsTest, DIA_02_SeverityMappingForErrorCodes) {
    EXPECT_EQ(severityForErrorCode(DiagnosticErrorCode::STATE_INVALID_TRANSITION), 
              DiagnosticSeverity::ERROR);
    EXPECT_EQ(severityForErrorCode(DiagnosticErrorCode::PATCH_APPLY_FAILED), 
              DiagnosticSeverity::WARN);
    EXPECT_EQ(severityForErrorCode(DiagnosticErrorCode::CASCADE_DETECTED), 
              DiagnosticSeverity::CRITICAL);
    EXPECT_EQ(severityForErrorCode(DiagnosticErrorCode::NETWORK_PARTITION), 
              DiagnosticSeverity::ERROR);
}

// DIA-03: Root cause classification
TEST_F(DiagnosticsTest, DIA_03_RootCauseClassification) {
    EXPECT_EQ(rootCauseForErrorCode(DiagnosticErrorCode::PATCH_CHECKSUM_MISMATCH),
              RootCauseClass::CHECKSUM);
    EXPECT_EQ(rootCauseForErrorCode(DiagnosticErrorCode::NETWORK_PARTITION),
              RootCauseClass::NETWORK);
    EXPECT_EQ(rootCauseForErrorCode(DiagnosticErrorCode::CASCADE_DETECTED),
              RootCauseClass::CASCADE);
    EXPECT_EQ(rootCauseForErrorCode(DiagnosticErrorCode::STATE_INVALID_TRANSITION),
              RootCauseClass::STATE);
}

// DIA-04: ErrorContext JSON serialization
TEST_F(DiagnosticsTest, DIA_04_ErrorContextJsonSerialization) {
    ErrorContext ctx;
    ctx.timestamp = std::chrono::system_clock::now();
    ctx.error_code = DiagnosticErrorCode::PATCH_CHECKSUM_MISMATCH;
    ctx.severity = DiagnosticSeverity::ERROR;
    ctx.root_cause = RootCauseClass::CHECKSUM;
    ctx.message = "SHA256 mismatch: expected abc123, got def456";
    ctx.operation = "verify_patch";
    ctx.phase = "verifying";
    ctx.node_id = "";
    ctx.version = "1.7.0";
    
    auto j = ctx.toJson();
    ASSERT_TRUE(j.contains("error_code"));
    ASSERT_TRUE(j.contains("severity"));
    ASSERT_TRUE(j.contains("message"));
    
    EXPECT_EQ(j["error_code"], 7441);
    EXPECT_EQ(j["severity"], "ERROR");
    EXPECT_EQ(j["message"], "SHA256 mismatch: expected abc123, got def456");
}

// DIA-05: ErrorContext JSON deserialization
TEST_F(DiagnosticsTest, DIA_05_ErrorContextJsonDeserialization) {
    nlohmann::json j;
    j["error_code"] = 7441;
    j["severity"] = "ERROR";
    j["root_cause"] = 1;
    j["message"] = "Checksum mismatch";
    j["operation"] = "verify";
    j["phase"] = "verifying";
    j["timestamp"] = "2026-08-08T06:29:43Z";
    j["version"] = "1.7.0";
    
    auto ctx = ErrorContext::fromJson(j);
    ASSERT_TRUE(ctx.has_value());
    EXPECT_EQ(ctx->error_code, DiagnosticErrorCode::PATCH_CHECKSUM_MISMATCH);
    EXPECT_EQ(ctx->severity, DiagnosticSeverity::ERROR);
    EXPECT_EQ(ctx->message, "Checksum mismatch");
    EXPECT_EQ(ctx->operation, "verify");
}

// DIA-06: Error code name mapping
TEST_F(DiagnosticsTest, DIA_06_ErrorCodeNameMapping) {
    EXPECT_EQ(errorCodeName(DiagnosticErrorCode::STATE_INVALID_TRANSITION),
              "STATE_INVALID_TRANSITION");
    EXPECT_EQ(errorCodeName(DiagnosticErrorCode::ROLLBACK_CHECKPOINT_NOT_FOUND),
              "ROLLBACK_CHECKPOINT_NOT_FOUND");
    EXPECT_EQ(errorCodeName(DiagnosticErrorCode::PATCH_APPLY_FAILED),
              "PATCH_APPLY_FAILED");
    EXPECT_EQ(errorCodeName(DiagnosticErrorCode::NETWORK_PARTITION),
              "NETWORK_PARTITION");
    EXPECT_EQ(errorCodeName(DiagnosticErrorCode::CASCADE_DETECTED),
              "CASCADE_DETECTED");
}

// DIA-07: Severity name mapping
TEST_F(DiagnosticsTest, DIA_07_SeverityNameMapping) {
    EXPECT_EQ(severityName(DiagnosticSeverity::INFO), "INFO");
    EXPECT_EQ(severityName(DiagnosticSeverity::WARN), "WARN");
    EXPECT_EQ(severityName(DiagnosticSeverity::ERROR), "ERROR");
    EXPECT_EQ(severityName(DiagnosticSeverity::CRITICAL), "CRITICAL");
}

// DIA-08: DiagnosticEmitter listener registration
TEST_F(DiagnosticEmitterTest, DIA_08_ListenerRegistration) {
    EXPECT_EQ(emitter_.listenerCount(), 1);
    
    auto listener2 = std::make_shared<TestListener>();
    emitter_.addListener(listener2);
    EXPECT_EQ(emitter_.listenerCount(), 2);
}

// DIA-09: Emit error event
TEST_F(DiagnosticEmitterTest, DIA_09_EmitErrorEvent) {
    ErrorContext ctx;
    ctx.timestamp = std::chrono::system_clock::now();
    ctx.error_code = DiagnosticErrorCode::PATCH_APPLY_FAILED;
    ctx.severity = DiagnosticSeverity::ERROR;
    ctx.root_cause = RootCauseClass::ARTIFACT;
    ctx.message = "Failed to apply patch to bin/themis_server";
    ctx.operation = "apply_patch";
    ctx.phase = "applying";
    ctx.version = "1.7.0";
    
    emitter_.emitError(ctx);
    
    ASSERT_EQ(test_listener_->eventCount(), 1);
    auto& event = test_listener_->events[0];
    EXPECT_TRUE(event.is_error);
    EXPECT_EQ(event.context.error_code, DiagnosticErrorCode::PATCH_APPLY_FAILED);
}

// DIA-10: Emit info event
TEST_F(DiagnosticEmitterTest, DIA_10_EmitInfoEvent) {
    emitter_.emitInfo("state_transition", "applying", "Transitioned to applying state",
                      "", "1.7.0");
    
    ASSERT_EQ(test_listener_->eventCount(), 1);
    auto& event = test_listener_->events[0];
    EXPECT_FALSE(event.is_error);
    EXPECT_EQ(event.context.severity, DiagnosticSeverity::INFO);
}

// DIA-11: Emit state transition event
TEST_F(DiagnosticEmitterTest, DIA_11_EmitStateTransitionEvent) {
    emitter_.emitStateTransition("VERIFYING", "APPLYING", "1.7.0");
    
    ASSERT_EQ(test_listener_->eventCount(), 1);
    auto& event = test_listener_->events[0];
    EXPECT_EQ(event.context.operation, "state_transition");
    EXPECT_EQ(event.context.phase, "APPLYING");
}

// DIA-12: Emit checkpoint created event
TEST_F(DiagnosticEmitterTest, DIA_12_EmitCheckpointCreatedEvent) {
    emitter_.emitCheckpointCreated(1, "before_apply", "1.7.0");
    
    ASSERT_EQ(test_listener_->eventCount(), 1);
    auto& event = test_listener_->events[0];
    EXPECT_EQ(event.context.operation, "create_checkpoint");
    EXPECT_FALSE(event.is_error);
}

// DIA-13: Emit checkpoint rollback event
TEST_F(DiagnosticEmitterTest, DIA_13_EmitCheckpointRollbackEvent) {
    emitter_.emitCheckpointRollback(1, true, "rollback_reason");
    
    ASSERT_EQ(test_listener_->eventCount(), 1);
    auto& event = test_listener_->events[0];
    EXPECT_EQ(event.context.operation, "rollback_checkpoint");
    EXPECT_FALSE(event.is_error);  // Success
}

// DIA-14: Emit patch apply event
TEST_F(DiagnosticEmitterTest, DIA_14_EmitPatchApplyEvent) {
    emitter_.emitPatchApply("bin/themis_server", false, "checksum mismatch");
    
    ASSERT_EQ(test_listener_->eventCount(), 1);
    auto& event = test_listener_->events[0];
    EXPECT_EQ(event.context.operation, "apply_patch");
    EXPECT_TRUE(event.is_error);  // Failed
    EXPECT_EQ(event.context.error_code, DiagnosticErrorCode::PATCH_APPLY_FAILED);
}

// DIA-15: Format error message
TEST_F(DiagnosticEmitterTest, DIA_15_FormatErrorMessage) {
    ErrorContext ctx;
    ctx.error_code = DiagnosticErrorCode::PATCH_CHECKSUM_MISMATCH;
    ctx.severity = DiagnosticSeverity::ERROR;
    ctx.root_cause = RootCauseClass::CHECKSUM;
    ctx.message = "SHA256 mismatch";
    ctx.operation = "verify_patch";
    ctx.phase = "verifying";
    ctx.node_id = "node-a";
    ctx.version = "1.7.0";
    
    auto formatted = DiagnosticEmitter::formatErrorMessage(ctx);
    
    EXPECT_NE(formatted.find("PATCH_CHECKSUM_MISMATCH"), std::string::npos);
    EXPECT_NE(formatted.find("7441"), std::string::npos);  // Error code
    EXPECT_NE(formatted.find("verify_patch"), std::string::npos);
    EXPECT_NE(formatted.find("verifying"), std::string::npos);
    EXPECT_NE(formatted.find("node-a"), std::string::npos);
}

} // anonymous namespace
