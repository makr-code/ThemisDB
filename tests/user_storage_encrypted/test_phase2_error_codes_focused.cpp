/**
 * @file test_phase2_error_codes_focused.cpp
 * @brief Focused test for Phase 2-3 error codes and diagnostic framework
 * 
 * Tests error code definitions, diagnostic events, and the foundation
 * for production-ready error handling and observability.
 */

#include <gtest/gtest.h>
#include "user_storage_encrypted/error_codes.hpp"
#include <iostream>
#include <vector>

namespace themis {
namespace plugins {
namespace user_storage {
namespace test {

class ErrorCodesTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Register a test handler that captures events
        registerDiagnosticEventHandler([this](const DiagnosticEvent& event) {
            captured_events.push_back(event);
        });
    }
    
    std::vector<DiagnosticEvent> captured_events;
};

// ============================================================================
// Test 1: Error Code Definitions
// ============================================================================

TEST_F(ErrorCodesTest, ErrorCodeEnumValuesAreValid) {
    // Verify all error codes have meaningful numeric values
    EXPECT_EQ(static_cast<uint16_t>(ErrorCode::SUCCESS), 0);
    
    // Backend errors (1xxx)
    EXPECT_GE(static_cast<uint16_t>(ErrorCode::BACKEND_NOT_AVAILABLE), 1000);
    EXPECT_LE(static_cast<uint16_t>(ErrorCode::BACKEND_NOT_AVAILABLE), 1999);
    
    // Key rotation errors (2xxx)
    EXPECT_GE(static_cast<uint16_t>(ErrorCode::ROTATION_CALLBACK_FAILED), 2000);
    EXPECT_LE(static_cast<uint16_t>(ErrorCode::ROTATION_CALLBACK_FAILED), 2999);
    
    // Multi-level storage errors (3xxx)
    EXPECT_GE(static_cast<uint16_t>(ErrorCode::LEVEL_INITIALIZATION_FAILED), 3000);
    EXPECT_LE(static_cast<uint16_t>(ErrorCode::LEVEL_INITIALIZATION_FAILED), 3999);
    
    // KDF errors (4xxx)
    EXPECT_GE(static_cast<uint16_t>(ErrorCode::KDF_INVALID_MASTER_KEY), 4000);
    EXPECT_LE(static_cast<uint16_t>(ErrorCode::KDF_INVALID_MASTER_KEY), 4999);
    
    // Path validation errors (5xxx)
    EXPECT_GE(static_cast<uint16_t>(ErrorCode::PATH_EMPTY), 5000);
    EXPECT_LE(static_cast<uint16_t>(ErrorCode::PATH_EMPTY), 5999);
}

TEST_F(ErrorCodesTest, ErrorCodeToStringMappingIsComplete) {
    // Verify all error codes have string representations
    std::vector<ErrorCode> all_codes = {
        ErrorCode::SUCCESS,
        ErrorCode::BACKEND_NOT_AVAILABLE,
        ErrorCode::FUSE_NOT_AVAILABLE,
        ErrorCode::MOUNT_TIMEOUT,
        ErrorCode::UNMOUNT_TIMEOUT,
        ErrorCode::MOUNT_FAILED,
        ErrorCode::UNMOUNT_FAILED,
        ErrorCode::STALE_MOUNT_DETECTED,
        ErrorCode::FUSE_PERMISSION_DENIED,
        ErrorCode::MOUNT_POINT_INVALID,
        ErrorCode::ENCRYPTED_DIR_INVALID,
        ErrorCode::STDIN_DELIVERY_TIMEOUT,
        ErrorCode::STDIN_DELIVERY_FAILED,
        ErrorCode::COMMAND_EXECUTION_TIMEOUT,
        ErrorCode::ROTATION_CALLBACK_FAILED,
        ErrorCode::ROTATION_CALLBACK_TIMEOUT,
        ErrorCode::ROTATION_CALLBACK_EXCEPTION,
        ErrorCode::ROTATION_STORE_FAILURE,
        ErrorCode::ROTATION_INTERVAL_INVALID,
        ErrorCode::ROTATION_LEVEL_NOT_SCHEDULED,
        ErrorCode::LEVEL_INITIALIZATION_FAILED,
        ErrorCode::LEVEL_MOUNT_FAILED,
        ErrorCode::LEVEL_UNMOUNT_FAILED,
        ErrorCode::LEVEL_CONFIG_INVALID,
        ErrorCode::KEY_PROVIDER_NOT_FOUND,
        ErrorCode::KEY_PROVIDER_ERROR,
        ErrorCode::KDF_INVALID_MASTER_KEY,
        ErrorCode::KDF_SALT_GENERATION_FAILED,
        ErrorCode::KDF_SALT_PERSISTENCE_FAILED,
        ErrorCode::KDF_DERIVATION_FAILED,
        ErrorCode::KDF_INVALID_PARAMETERS,
        ErrorCode::PATH_EMPTY,
        ErrorCode::PATH_NOT_ABSOLUTE_OR_RELATIVE,
        ErrorCode::PATH_TRAVERSAL_DETECTED,
        ErrorCode::PATH_INVALID_CHARACTERS,
        ErrorCode::PATH_SYMLINK_TO_PARENT,
        ErrorCode::PERMISSION_DENIED,
    };
    
    for (const auto& code : all_codes) {
        std::string str = errorCodeToString(code);
        EXPECT_NE(str, "UNKNOWN_ERROR_CODE") 
            << "Error code " << static_cast<uint16_t>(code) << " not mapped";
    }
}

// ============================================================================
// Test 2: Diagnostic Event Emission
// ============================================================================

TEST_F(ErrorCodesTest, DiagnosticEventEmissionWorks) {
    captured_events.clear();
    
    DiagnosticEvent event;
    event.type = DiagnosticEvent::Type::MOUNT_STARTED;
    event.component = "test_component";
    event.message = "Test message";
    event.level = "TEST_LEVEL";
    
    emitDiagnosticEvent(event);
    
    ASSERT_EQ(captured_events.size(), 1);
    EXPECT_EQ(captured_events[0].component, "test_component");
    EXPECT_EQ(captured_events[0].message, "Test message");
    EXPECT_NE(captured_events[0].timestamp_ms, 0);
}

TEST_F(ErrorCodesTest, MultipleEventsAreCapatured) {
    captured_events.clear();
    
    // Emit 3 events
    for (int i = 0; i < 3; ++i) {
        DiagnosticEvent event;
        event.type = DiagnosticEvent::Type::MOUNT_COMPLETED;
        event.component = "test";
        event.message = "Event " + std::to_string(i);
        emitDiagnosticEvent(event);
    }
    
    EXPECT_EQ(captured_events.size(), 3);
    EXPECT_EQ(captured_events[0].message, "Event 0");
    EXPECT_EQ(captured_events[1].message, "Event 1");
    EXPECT_EQ(captured_events[2].message, "Event 2");
}

TEST_F(ErrorCodesTest, ErrorEventWithRemediation) {
    captured_events.clear();
    
    DiagnosticEvent event;
    event.type = DiagnosticEvent::Type::ERROR_DETECTED;
    event.component = "test_component";
    event.error_code = ErrorCode::BACKEND_NOT_AVAILABLE;
    event.message = "Backend not available";
    event.system_errno_val = ENOENT;
    event.remediation = "Install gocryptfs via: apt-get install gocryptfs";
    
    emitDiagnosticEvent(event);
    
    ASSERT_EQ(captured_events.size(), 1);
    EXPECT_EQ(captured_events[0].error_code, ErrorCode::BACKEND_NOT_AVAILABLE);
    EXPECT_EQ(captured_events[0].system_errno_val, ENOENT);
    EXPECT_EQ(captured_events[0].remediation, "Install gocryptfs via: apt-get install gocryptfs");
}

// ============================================================================
// Test 3: Diagnostic Event JSON Serialization
// ============================================================================

TEST_F(ErrorCodesTest, DiagnosticEventJsonSerialization) {
    DiagnosticEvent event;
    event.type = DiagnosticEvent::Type::MOUNT_FAILED;
    event.timestamp_ms = 1691000000000;  // Fixed timestamp for deterministic test
    event.error_code = ErrorCode::MOUNT_TIMEOUT;
    event.component = "gocryptfs_backend";
    event.message = "Mount operation timed out";
    event.level = "VERTRAUT";
    event.system_errno_val = ETIMEDOUT;
    event.remediation = "Increase mount timeout or check system load";
    
    std::string json_str = event.toJsonString();
    
    // Verify JSON structure
    EXPECT_NE(json_str.find("\"timestamp_ms\":1691000000000"), std::string::npos);
    EXPECT_NE(json_str.find("\"error_code\":1003"), std::string::npos);  // MOUNT_TIMEOUT = 1003
    EXPECT_NE(json_str.find("\"component\":\"gocryptfs_backend\""), std::string::npos);
    EXPECT_NE(json_str.find("\"message\":\"Mount operation timed out\""), std::string::npos);
    EXPECT_NE(json_str.find("\"level\":\"VERTRAUT\""), std::string::npos);
    EXPECT_NE(json_str.find("\"errno\":"), std::string::npos);
    EXPECT_NE(json_str.find("\"remediation\":"), std::string::npos);
}

// ============================================================================
// Test 4: Diagnostic Handler Registration
// ============================================================================

TEST_F(ErrorCodesTest, CustomHandlerCanBeRegistered) {
    int handler_call_count = 0;
    
    registerDiagnosticEventHandler([&handler_call_count](const DiagnosticEvent& event) {
        handler_call_count++;
    });
    
    DiagnosticEvent event;
    event.type = DiagnosticEvent::Type::ROTATION_STARTED;
    event.component = "test";
    emitDiagnosticEvent(event);
    
    EXPECT_EQ(handler_call_count, 1);
}

// ============================================================================
// Test 5: Error Code Category Validation
// ============================================================================

TEST_F(ErrorCodesTest, ErrorCodeCategoriesAreCorrect) {
    // Verify error codes are in correct categories
    
    // Backend errors (1xxx)
    auto backend_codes = {
        ErrorCode::BACKEND_NOT_AVAILABLE,
        ErrorCode::FUSE_NOT_AVAILABLE,
        ErrorCode::MOUNT_TIMEOUT,
        ErrorCode::UNMOUNT_TIMEOUT,
    };
    for (auto code : backend_codes) {
        uint16_t num = static_cast<uint16_t>(code);
        EXPECT_GE(num, 1000) << "Error code not in 1xxx range";
        EXPECT_LT(num, 2000) << "Error code not in 1xxx range";
    }
    
    // Key rotation errors (2xxx)
    auto rotation_codes = {
        ErrorCode::ROTATION_CALLBACK_FAILED,
        ErrorCode::ROTATION_CALLBACK_TIMEOUT,
        ErrorCode::ROTATION_STORE_FAILURE,
    };
    for (auto code : rotation_codes) {
        uint16_t num = static_cast<uint16_t>(code);
        EXPECT_GE(num, 2000) << "Error code not in 2xxx range";
        EXPECT_LT(num, 3000) << "Error code not in 2xxx range";
    }
    
    // Path validation errors (5xxx)
    auto path_codes = {
        ErrorCode::PATH_EMPTY,
        ErrorCode::PATH_TRAVERSAL_DETECTED,
        ErrorCode::PERMISSION_DENIED,
    };
    for (auto code : path_codes) {
        uint16_t num = static_cast<uint16_t>(code);
        EXPECT_GE(num, 5000) << "Error code not in 5xxx range";
        EXPECT_LT(num, 6000) << "Error code not in 5xxx range";
    }
}

} // namespace test
} // namespace user_storage
} // namespace plugins
} // namespace themis
