/**
 * @file test_input_validation.cpp
 * @brief Input validation framework tests
 * @author Copilot Code Generation
 * @date 2026-05-19
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "security/input_validator.hpp"

using namespace themis::security;

class InputValidationTest : public ::testing::Test {
 protected:
  // InputValidator is a static utility class; no instance is required.
};

// ============================================================================
// SQL INJECTION TESTS
// ============================================================================

TEST_F(InputValidationTest, RejectsSqlInjectionOrPattern) {
  std::string sql_inject = "admin' OR 'x'='x";
  auto result = InputValidator::validateUserInput(sql_inject, ValidationContext::SQL_QUERY);
  EXPECT_FALSE(result.is_valid);
  EXPECT_THAT(result.error_message, testing::HasSubstr("injection"));
}

TEST_F(InputValidationTest, RejectsSqlInjectionDropTable) {
  std::string sql_inject = "'; DROP TABLE users; --";
  auto result = InputValidator::validateUserInput(sql_inject, ValidationContext::SQL_QUERY);
  EXPECT_FALSE(result.is_valid);
}

TEST_F(InputValidationTest, RejectsSqlInjectionUnionSelect) {
  std::string sql_inject = "1 UNION SELECT * FROM credentials";
  auto result = InputValidator::validateUserInput(sql_inject, ValidationContext::SQL_QUERY);
  EXPECT_FALSE(result.is_valid);
}

TEST_F(InputValidationTest, AcceptsSafeQueryInput) {
  std::string safe = "John Doe";
  auto result = InputValidator::validateUserInput(safe, ValidationContext::SQL_QUERY);
  EXPECT_TRUE(result.is_valid);
}

// ============================================================================
// IDENTIFIER VALIDATION TESTS
// ============================================================================

TEST_F(InputValidationTest, AcceptsValidIdentifier) {
  auto result = InputValidator::validateIdentifier("users_table");
  EXPECT_TRUE(result.is_valid);
}

TEST_F(InputValidationTest, RejectsIdentifierStartingWithNumber) {
  auto result = InputValidator::validateIdentifier("123users");
  EXPECT_FALSE(result.is_valid);
  EXPECT_THAT(result.error_message, testing::HasSubstr("start"));
}

TEST_F(InputValidationTest, RejectsIdentifierWithHyphen) {
  auto result = InputValidator::validateIdentifier("user-names");
  EXPECT_FALSE(result.is_valid);
}

TEST_F(InputValidationTest, RejectsIdentifierWithSpecialChars) {
  auto result = InputValidator::validateIdentifier("users$table");
  EXPECT_FALSE(result.is_valid);
}

TEST_F(InputValidationTest, AcceptsUnderscoreStartIdentifier) {
  auto result = InputValidator::validateIdentifier("_internal_table");
  EXPECT_TRUE(result.is_valid);
}

TEST_F(InputValidationTest, RejectsIdentifierWithSpace) {
  auto result = InputValidator::validateIdentifier("users table");
  EXPECT_FALSE(result.is_valid);
}

// ============================================================================
// JSON VALIDATION TESTS
// ============================================================================

TEST_F(InputValidationTest, AcceptsValidJsonObject) {
  std::string json = R"({"name": "John", "age": 30})";
  auto result = InputValidator::validateJsonPayload(json);
  EXPECT_TRUE(result.is_valid);
}

TEST_F(InputValidationTest, AcceptsValidJsonArray) {
  std::string json = R"([1, 2, 3, 4])";
  auto result = InputValidator::validateJsonPayload(json);
  EXPECT_TRUE(result.is_valid);
}

TEST_F(InputValidationTest, RejectsJsonWithNullBytes) {
  std::string json_with_null = "{\"name\": \"Jo";
  json_with_null += '\0';
  json_with_null += "hn\"}";
  auto result = InputValidator::validateJsonPayload(json_with_null);
  EXPECT_FALSE(result.is_valid);
  EXPECT_THAT(result.error_message, testing::HasSubstr("null bytes"));
}

TEST_F(InputValidationTest, RejectsJsonWithExcessiveNesting) {
  // Build JSON with 25 levels of nesting (exceeds limit of 20).
  std::string json = {};
  for (int i = 0; i < 25; ++i) {
    json += "{\"a\":";
  }
  json += "42";
  for (int i = 0; i < 25; ++i) {
    json += "}";
  }
  
  auto result = InputValidator::validateJsonPayload(json);
  EXPECT_FALSE(result.is_valid);
  EXPECT_THAT(result.error_message, testing::HasSubstr("nesting"));
}

// ============================================================================
// FILE UPLOAD TESTS
// ============================================================================

TEST_F(InputValidationTest, AcceptsValidFileUpload) {
  auto result = InputValidator::validateFileUpload("document.pdf", 1'000'000, "application/pdf");
  EXPECT_TRUE(result.is_valid);
}

TEST_F(InputValidationTest, RejectsFileUploadWithPathTraversal) {
  auto result = InputValidator::validateFileUpload("../../../etc/passwd", 1000, "text/plain");
  EXPECT_FALSE(result.is_valid);
}

TEST_F(InputValidationTest, RejectsFileUploadWithInvalidExtension) {
  auto result = InputValidator::validateFileUpload("malware.exe", 1000, "application/octet-stream");
  EXPECT_FALSE(result.is_valid);
  EXPECT_THAT(result.error_message, testing::HasSubstr("not allowed"));
}

TEST_F(InputValidationTest, RejectsFileUploadWithSpecialChars) {
  auto result = InputValidator::validateFileUpload("file@#$.txt", 1000, "text/plain");
  EXPECT_FALSE(result.is_valid);
}

TEST_F(InputValidationTest, RejectsFileUploadExceedingSize) {
  // 600 MB (exceeds 500 MB limit)
  auto result = InputValidator::validateFileUpload("large.zip", 600'000'000, "application/zip");
  EXPECT_FALSE(result.is_valid);
  EXPECT_THAT(result.error_message, testing::HasSubstr("exceeds maximum"));
}

// ============================================================================
// URI PARAMETER TESTS
// ============================================================================

TEST_F(InputValidationTest, AcceptsValidUriParameter) {
  auto result = InputValidator::validateUriParameter("John%20Doe");
  EXPECT_TRUE(result.is_valid);
}

TEST_F(InputValidationTest, RejectsUriParameterWithNullByte) {
  std::string uri_with_null = "safe";
  uri_with_null += '\0';
  uri_with_null += "text";
  auto result = InputValidator::validateUriParameter(uri_with_null);
  EXPECT_FALSE(result.is_valid);
}

TEST_F(InputValidationTest, RejectsUriParameterWithControlChar) {
  std::string uri_with_control = "safe\x1Ftext";  // Unit separator control char
  auto result = InputValidator::validateUriParameter(uri_with_control);
  EXPECT_FALSE(result.is_valid);
}

// ============================================================================
// REQUEST HEADER TESTS
// ============================================================================

TEST_F(InputValidationTest, AcceptsValidRequestHeader) {
  auto result = InputValidator::validateRequestHeader("X-Custom-Header", "valid-value");
  EXPECT_TRUE(result.is_valid);
}

TEST_F(InputValidationTest, RejectsHeaderWithCRLFInjection) {
  auto result = InputValidator::validateRequestHeader("User-Agent", "Mozilla\r\nSet-Cookie: admin=true");
  EXPECT_FALSE(result.is_valid);
  EXPECT_THAT(result.error_message, testing::HasSubstr("CRLF"));
}

TEST_F(InputValidationTest, RejectsHeaderWithNewline) {
  auto result = InputValidator::validateRequestHeader("X-Custom", "valid\ninjection");
  EXPECT_FALSE(result.is_valid);
}

// ============================================================================
// SEARCH QUERY TESTS
// ============================================================================

TEST_F(InputValidationTest, AcceptsValidSearchQuery) {
  auto result = InputValidator::validateSearchQuery("database", false);
  EXPECT_TRUE(result.is_valid);
}

TEST_F(InputValidationTest, RejectsEmptySearchQuery) {
  auto result = InputValidator::validateSearchQuery("", false);
  EXPECT_FALSE(result.is_valid);
}

TEST_F(InputValidationTest, RejectsSearchQueryExceedingLength) {
  std::string long_query(300, 'a');  // 300 characters
  auto result = InputValidator::validateSearchQuery(long_query, false);
  EXPECT_FALSE(result.is_valid);
}

// ============================================================================
// SANITIZATION TESTS
// ============================================================================

TEST_F(InputValidationTest, SanitizesHtmlCorrectly) {
  std::string input = "<script>alert('xss')</script>";
  auto sanitized = InputValidator::sanitizeForHtml(input);
  
  EXPECT_EQ(sanitized, "&lt;script&gt;alert(&#39;xss&#39;)&lt;/script&gt;");
  EXPECT_FALSE(sanitized.find("<script") != std::string::npos);  // No <script tag
}

TEST_F(InputValidationTest, SanitizesJsonCorrectly) {
  std::string input = "Line 1\nLine 2\tTab";
  auto sanitized = InputValidator::sanitizeForJson(input);
  
  EXPECT_EQ(sanitized, "Line 1\\nLine 2\\tTab");
  EXPECT_FALSE(sanitized.find('\n') != std::string::npos);  // No raw newlines
}

TEST_F(InputValidationTest, SanitizesSqlForLoggingCorrectly) {
  std::string input = "'; DROP TABLE--'";
  auto sanitized = InputValidator::sanitizeForSqlLogging(input);
  
  EXPECT_EQ(sanitized, "\\'; DROP TABLE--\\'");
}

// ============================================================================
// UTILITY TESTS
// ============================================================================

TEST_F(InputValidationTest, DetectsNullBytes) {
  std::string with_null = "safe";
  with_null += '\0';
  with_null += "data";
  EXPECT_TRUE(InputValidator::containsNullBytes(with_null));
}

TEST_F(InputValidationTest, DetectsControlCharacters) {
  std::string with_control = "text\x1Fmore";
  EXPECT_TRUE(InputValidator::containsControlCharacters(with_control));
}

TEST_F(InputValidationTest, ValidatesUtf8Correctly) {
  // Valid UTF-8
  EXPECT_TRUE(InputValidator::isValidUtf8("Hello"));
  EXPECT_TRUE(InputValidator::isValidUtf8("こんにちは"));  // Japanese
  
  // Invalid UTF-8 (malformed multi-byte)
  EXPECT_FALSE(InputValidator::isValidUtf8("\xC0\x80"));  // Overlong encoding
}

TEST_F(InputValidationTest, IdentifiesAlphanumericWithUnderscore) {
  EXPECT_TRUE(InputValidator::isAlphanumericWithUnderscore("valid_123"));
  EXPECT_FALSE(InputValidator::isAlphanumericWithUnderscore("invalid-name"));
  EXPECT_FALSE(InputValidator::isAlphanumericWithUnderscore("has space"));
}

// ============================================================================
// PATH TRAVERSAL TESTS
// ============================================================================

TEST_F(InputValidationTest, RejectsPathWithDoubleDotSlash) {
  auto result = InputValidator::validateConfigPath("../../../etc/passwd.conf");
  EXPECT_FALSE(result.is_valid);
}

TEST_F(InputValidationTest, RejectsPathWithDoubleDotBackslash) {
  auto result = InputValidator::validateConfigPath("..\\..\\windows\\system32");
  EXPECT_FALSE(result.is_valid);
}

TEST_F(InputValidationTest, AcceptsRelativePathWithoutTraversal) {
  auto result = InputValidator::validateConfigPath("config/settings.conf", ".conf");
  EXPECT_TRUE(result.is_valid);
}

// ============================================================================
// INTEGRATION TESTS
// ============================================================================

TEST_F(InputValidationTest, XssPreventionIntegration) {
  // Simulate malicious user input
  std::string user_input = "<img src=x onerror='alert(1)'>";
  auto result = InputValidator::validateUserInput(user_input, ValidationContext::JSON_PAYLOAD);
  
  // Sanitize for display
  auto safe_output = InputValidator::sanitizeForHtml(user_input);
  EXPECT_EQ(safe_output, "&lt;img src=x onerror=&#39;alert(1)&#39;&gt;");
}

TEST_F(InputValidationTest, SqlInjectionPreventionIntegration) {
  // Simulate malicious SQL query input
  std::string user_query = "1' OR '1'='1";
  auto result = InputValidator::validateUserInput(user_query, ValidationContext::SQL_QUERY);
  
  EXPECT_FALSE(result.is_valid);
  EXPECT_TRUE(result.remediation_hint.size() > 0);
}

TEST_F(InputValidationTest, FullUploadValidationFlow) {
  // Simulate file upload validation
  std::string filename = "document.pdf";
  size_t file_size = 5'000'000;  // 5 MB
  std::string mime_type = "application/pdf";
  
  auto result = InputValidator::validateFileUpload(filename, file_size, mime_type);
  EXPECT_TRUE(result.is_valid);
}

// ============================================================================
// EDGE CASE TESTS
// ============================================================================

TEST_F(InputValidationTest, HandlesEmptyInput) {
  auto result = InputValidator::validateUserInput("", ValidationContext::SQL_QUERY);
  EXPECT_TRUE(result.is_valid);  // Empty input is typically safe
}

TEST_F(InputValidationTest, HandlesVeryLongInput) {
  std::string long_input(50'000, 'a');
  auto result = InputValidator::validateUserInput(long_input, ValidationContext::SQL_QUERY);
  EXPECT_FALSE(result.is_valid);  // Should exceed MAX_FIELD_SIZE (10KB)
}

TEST_F(InputValidationTest, HandlesUnicodeInput) {
  std::string unicode = "你好世界";  // Chinese: "Hello World"
  auto result = InputValidator::validateUserInput(unicode, ValidationContext::JSON_PAYLOAD);
  EXPECT_TRUE(result.is_valid);
}
