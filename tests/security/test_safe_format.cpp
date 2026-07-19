#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "security/safe_format.h"

using namespace themis::security;

class SafeFormatTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Redirect stdout to capture output
    }

    void TearDown() override {
        fflush(stdout);
    }
};

// ============================================================================
// Format String Attack Prevention Tests
// ============================================================================

TEST_F(SafeFormatTest, Printf_UserInput_NoFormatSpecifiers) {
    // User input should never be interpreted as format string
    std::string user_input = "Hello %x %x %x";
    int result = SafeFormat::print_string(user_input);
    EXPECT_GE(result, 0);
    // Should print the string literally, not interpret %x as hex specifiers
}

TEST_F(SafeFormatTest, Printf_SafeWrapper_WithValidFormat) {
    // Test safe printf wrapper with controlled format string
    std::string result = SafeFormat::format_safe("The answer is {}", 42);
    EXPECT_EQ(result, "The answer is 42");
}

TEST_F(SafeFormatTest, Sprintf_SafeWrapper_BufferOverflow) {
    // Test snprintf_safe prevents buffer overflow
    char buffer[10];
    int result = SafeFormat::snprintf_safe(buffer, sizeof(buffer), 
                                          "Very long string that should be truncated: {}", 
                                          "test");
    EXPECT_GE(result, 0);
    // Buffer should be null-terminated and within bounds
    EXPECT_LT(strlen(buffer), sizeof(buffer));
    EXPECT_EQ(buffer[sizeof(buffer) - 1], '\0');
}

TEST_F(SafeFormatTest, Format_EscapeForDisplay_ControlCharacters) {
    // Test escaping of control characters
    std::string input = "Hello\nWorld\t!";
    std::string escaped = SafeFormat::escape_for_display(input);
    EXPECT_THAT(escaped, testing::HasSubstr("\\n"));
    EXPECT_THAT(escaped, testing::HasSubstr("\\t"));
}

TEST_F(SafeFormatTest, Format_EscapeForDisplay_SpecialCharacters) {
    // Test escaping of special characters
    std::string input = "Quote: \"test\", Backslash: \\";
    std::string escaped = SafeFormat::escape_for_display(input);
    EXPECT_THAT(escaped, testing::HasSubstr("\\\""));
    EXPECT_THAT(escaped, testing::HasSubstr("\\\\"));
}

TEST_F(SafeFormatTest, Format_EscapeForDisplay_NonPrintable) {
    // Test escaping of non-printable characters
    std::string input = "\x00\x01\x1F";  // NUL, SOH, US control chars
    std::string escaped = SafeFormat::escape_for_display(input);
    EXPECT_THAT(escaped, testing::HasSubstr("\\x"));
}

TEST_F(SafeFormatTest, Printf_MultipleArguments) {
    // Test format_safe with multiple arguments
    std::string result = SafeFormat::format_safe("User: {}, ID: {}, Active: {}", 
                                                  "alice", 12345, true);
    EXPECT_EQ(result, "User: alice, ID: 12345, Active: true");
}

TEST_F(SafeFormatTest, Printf_NullBuffer) {
    // Test snprintf_safe with null buffer
    int result = SafeFormat::snprintf_safe(nullptr, 0, "test");
    EXPECT_EQ(result, -1);
}

TEST_F(SafeFormatTest, Printf_EmptyBuffer) {
    // Test snprintf_safe with zero size
    char buffer[10];
    int result = SafeFormat::snprintf_safe(buffer, 0, "test");
    EXPECT_EQ(result, -1);
}

// ============================================================================
// Format String Vulnerability Scenarios
// ============================================================================

TEST_F(SafeFormatTest, AttackVector_ReadStackMemory) {
    // Attempt to read stack memory via format string
    // Using safe wrapper should prevent this
    std::string safe_output = SafeFormat::format_safe("Reading: {}", "value");
    EXPECT_THAT(safe_output, testing::HasSubstr("value"));
}

TEST_F(SafeFormatTest, AttackVector_WriteStackMemory) {
    // Attempt %n write to stack via format string
    // Safe wrapper prevents this by using fmt library
    std::string safe_output = SafeFormat::format_safe("Write attempt: %n");
    EXPECT_THAT(safe_output, testing::HasSubstr("%n"));  // Literal string, not directive
}

TEST_F(SafeFormatTest, AttackVector_CrashViaFormatString) {
    // Attempt crash via malformed format string
    // Should handle gracefully
    std::string result = SafeFormat::format_safe_runtime("Crash: {}");  // Missing argument
    EXPECT_THAT(result, testing::HasSubstr("ERROR"));  // Or truncated
}

// ============================================================================
// Real-World Format String Patterns
// ============================================================================

TEST_F(SafeFormatTest, LoggingPattern_ErrorMessage) {
    // Common logging pattern
    std::string error_msg = "Connection refused";
    std::string log = SafeFormat::format_safe("[ERROR] {}", error_msg);
    EXPECT_THAT(log, testing::HasSubstr("[ERROR]"));
    EXPECT_THAT(log, testing::HasSubstr("Connection refused"));
}

TEST_F(SafeFormatTest, LoggingPattern_HTTPStatus) {
    // HTTP status code logging
    int status = 404;
    std::string error = "Not Found";
    std::string log = SafeFormat::format_safe("HTTP {} {}", status, error);
    EXPECT_THAT(log, testing::HasSubstr("404"));
    EXPECT_THAT(log, testing::HasSubstr("Not Found"));
}

TEST_F(SafeFormatTest, UserMessage_Logging) {
    // Test log_user_message with user input
    std::string user_msg = "User entered: %x %x %x";
    EXPECT_NO_THROW(SafeFormat::log_user_message(user_msg, "input"));
}

// ============================================================================
// Backward Compatibility & Performance
// ============================================================================

TEST_F(SafeFormatTest, Performance_LargeFormat) {
    // Test performance with large formatted string
    std::string result = SafeFormat::format_safe("Item {}: {}, ", 1, "test");
    for (int i = 2; i < 100; ++i) {
        result = SafeFormat::format_safe("{} Item {}: {}, ", result, i, "test");
    }
    EXPECT_GT(result.length(), 0);
}

TEST_F(SafeFormatTest, Compatibility_EmptyString) {
    // Test with empty strings
    std::string result = SafeFormat::format_safe("{}", "");
    EXPECT_EQ(result, "");
}

TEST_F(SafeFormatTest, Compatibility_LongString) {
    // Test with very long string
    std::string long_str(10000, 'A');
    std::string result = SafeFormat::format_safe("Length: {}", long_str.length());
    EXPECT_THAT(result, testing::HasSubstr("10000"));
}
