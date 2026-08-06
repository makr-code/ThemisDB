/**
 * @file test_process_diagnostics_incident_focused.cpp
 * @brief Phase 4 Diagnostics Tests: Incident classification, context capture, and reporting
 * @note Test IDs: G-01..G-08
 */

#include <gtest/gtest.h>
#include "process/process_api_contract.h"
#include "process/process_diagnostics.h"

#include <chrono>
#include <cstdint>
#include <sstream>
#include <string>

using namespace themis::process;

// ─────────────────────────────────────────────────────────────────────────────
// Diagnostics Test Fixture
// ─────────────────────────────────────────────────────────────────────────────

class DiagnosticsIncidentTest : public ::testing::Test {
protected:
    // Capture current time in milliseconds for verification
    int64_t capture_time_ms() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// G-01: DiagnosticRecord captures incident type correctly
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DiagnosticsIncidentTest, G01_DiagnosticRecordIncidentType) {
    DiagnosticRecord rec(
        DiagnosticIncidentType::IMPORT_INCIDENT,
        ProcError::kDeserialiserFailed,
        "deserialize_bpmn",
        "model_v1.bpmn",
        "Invalid XML: unclosed tag at line 42"
    );

    EXPECT_EQ(rec.incident_type, DiagnosticIncidentType::IMPORT_INCIDENT);
    EXPECT_EQ(rec.error_code, ProcError::kDeserialiserFailed);
}

// ─────────────────────────────────────────────────────────────────────────────
// G-02: DiagnosticRecord captures operation and input identifier
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DiagnosticsIncidentTest, G02_DiagnosticRecordOperationAndInput) {
    DiagnosticRecord rec(
        DiagnosticIncidentType::VALIDATION_INCIDENT,
        ProcError::kValidationFailed,
        "validate_model",
        "proc_model_12345",
        "Gateway type not supported"
    );

    EXPECT_EQ(rec.operation, "validate_model");
    EXPECT_EQ(rec.input_identifier, "proc_model_12345");
}

// ─────────────────────────────────────────────────────────────────────────────
// G-03: DiagnosticRecord captures actionable message
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DiagnosticsIncidentTest, G03_DiagnosticRecordActionableMessage) {
    const char* message = "Check model syntax and review BPMN 2.0 compliance";
    DiagnosticRecord rec(
        DiagnosticIncidentType::IMPORT_INCIDENT,
        ProcError::kDeserialiserFailed,
        "deserialize",
        "model.bpmn",
        message
    );

    EXPECT_EQ(rec.actionable_message, message);
    EXPECT_FALSE(rec.actionable_message.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// G-04: DiagnosticRecord timestamp is populated and reasonable
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DiagnosticsIncidentTest, G04_DiagnosticRecordTimestamp) {
    int64_t before_ms = capture_time_ms();
    DiagnosticRecord rec(
        DiagnosticIncidentType::RETRIEVAL_INCIDENT,
        ProcError::kDeserialiserFailed,
        "retrieve_context",
        "instance_999",
        "Context assembly failed"
    );
    int64_t after_ms = capture_time_ms();

    // Timestamp should be within reasonable bounds
    EXPECT_GE(rec.timestamp_ms, before_ms - 1000);  // Allow 1 second clock skew
    EXPECT_LE(rec.timestamp_ms, after_ms + 1000);
}

// ─────────────────────────────────────────────────────────────────────────────
// G-05: Multiple incident types are distinct
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DiagnosticsIncidentTest, G05_DistinctIncidentTypes) {
    DiagnosticRecord import_rec(
        DiagnosticIncidentType::IMPORT_INCIDENT,
        ProcError::kDeserialiserFailed,
        "import", "f1", "msg1"
    );

    DiagnosticRecord validation_rec(
        DiagnosticIncidentType::VALIDATION_INCIDENT,
        ProcError::kValidationFailed,
        "validate", "f2", "msg2"
    );

    DiagnosticRecord retrieval_rec(
        DiagnosticIncidentType::RETRIEVAL_INCIDENT,
        ProcError::kMaxContextSizeExceeded,
        "retrieve", "f3", "msg3"
    );

    DiagnosticRecord linking_rec(
        DiagnosticIncidentType::LINKING_INCIDENT,
        ProcError::kLinkingFailed,
        "link", "f4", "msg4"
    );

    DiagnosticRecord resource_rec(
        DiagnosticIncidentType::RESOURCE_INCIDENT,
        ProcError::kMaxDepthExceeded,
        "parse", "f5", "msg5"
    );

    EXPECT_NE(static_cast<int32_t>(import_rec.incident_type),
              static_cast<int32_t>(validation_rec.incident_type));
    EXPECT_NE(static_cast<int32_t>(import_rec.incident_type),
              static_cast<int32_t>(retrieval_rec.incident_type));
    EXPECT_NE(static_cast<int32_t>(import_rec.incident_type),
              static_cast<int32_t>(linking_rec.incident_type));
    EXPECT_NE(static_cast<int32_t>(import_rec.incident_type),
              static_cast<int32_t>(resource_rec.incident_type));
}

// ─────────────────────────────────────────────────────────────────────────────
// G-06: Error codes and incident types are orthogonal
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DiagnosticsIncidentTest, G06_ErrorCodesOrthogonalToIncidentTypes) {
    // Same error code with different incident types
    DiagnosticRecord rec1(
        DiagnosticIncidentType::IMPORT_INCIDENT,
        ProcError::kDeserialiserFailed,
        "op1", "input1", "msg1"
    );

    DiagnosticRecord rec2(
        DiagnosticIncidentType::VALIDATION_INCIDENT,
        ProcError::kDeserialiserFailed,
        "op2", "input2", "msg2"
    );

    // Same incident type with different error codes
    DiagnosticRecord rec3(
        DiagnosticIncidentType::IMPORT_INCIDENT,
        ProcError::kSerialiserFailed,
        "op3", "input3", "msg3"
    );

    EXPECT_EQ(rec1.error_code, rec2.error_code);
    EXPECT_NE(rec1.incident_type, rec2.incident_type);

    EXPECT_EQ(rec1.incident_type, rec3.incident_type);
    EXPECT_NE(rec1.error_code, rec3.error_code);
}

// ─────────────────────────────────────────────────────────────────────────────
// G-07: Formatted message produces valid output (non-empty, contains key fields)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DiagnosticsIncidentTest, G07_FormattedMessageContainsKeyFields) {
    DiagnosticRecord rec(
        DiagnosticIncidentType::IMPORT_INCIDENT,
        ProcError::kDeserialiserFailed,
        "deserialize_bpmn",
        "test_model.bpmn",
        "XML parsing failed at line 50"
    );

    std::string formatted = rec.toFormattedMessage();
    EXPECT_FALSE(formatted.empty());

    // Formatted message should contain key diagnostic information
    EXPECT_TRUE(formatted.find("test_model.bpmn") != std::string::npos ||
                formatted.find("7603") != std::string::npos ||
                formatted.find("XML parsing") != std::string::npos);
}

// ─────────────────────────────────────────────────────────────────────────────
// G-08: Incident type classification covers all resource limits
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DiagnosticsIncidentTest, G08_ResourceIncidentCoverageAllLimits) {
    DiagnosticRecord depth_rec(
        DiagnosticIncidentType::RESOURCE_INCIDENT,
        ProcError::kMaxDepthExceeded,
        "parse_model",
        "model_deep.bpmn",
        "Max nesting depth (100) exceeded"
    );

    DiagnosticRecord elements_rec(
        DiagnosticIncidentType::RESOURCE_INCIDENT,
        ProcError::kMaxElementsExceeded,
        "parse_model",
        "model_large.bpmn",
        "Max element count (10000) exceeded"
    );

    DiagnosticRecord context_rec(
        DiagnosticIncidentType::RESOURCE_INCIDENT,
        ProcError::kMaxContextSizeExceeded,
        "retrieve_context",
        "instance_999",
        "Max retrieval context (1 MB) exceeded"
    );

    // All are RESOURCE_INCIDENT type
    EXPECT_EQ(depth_rec.incident_type, DiagnosticIncidentType::RESOURCE_INCIDENT);
    EXPECT_EQ(elements_rec.incident_type, DiagnosticIncidentType::RESOURCE_INCIDENT);
    EXPECT_EQ(context_rec.incident_type, DiagnosticIncidentType::RESOURCE_INCIDENT);

    // But have distinct error codes
    EXPECT_NE(static_cast<int32_t>(depth_rec.error_code),
              static_cast<int32_t>(elements_rec.error_code));
    EXPECT_NE(static_cast<int32_t>(elements_rec.error_code),
              static_cast<int32_t>(context_rec.error_code));
}
