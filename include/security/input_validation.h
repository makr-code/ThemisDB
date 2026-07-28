/**
 * @file input_validation.h
 * @brief Strict allowlist-based input validation for Phase 4 security hardening
 * 
 * Implements reject-by-default input validation for:
 * - HTTP request parameters and headers
 * - SQL query parameters
 * - LLM prompt inputs
 * 
 * @version 1.0
 * @date 2026-07-28
 * @author ThemisDB Security Team
 */

#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <vector>

namespace themis::security::phase4_hardening {

/**
 * @class ValidationResult
 * @brief Result of input validation operation
 */
struct ValidationResult {
  /// Whether the input passed validation
  bool valid = false;
  
  /// Error message if validation failed (empty if valid)
  std::string error_message;
  
  /// Sanitized input value (if applicable)
  std::string sanitized_value;
  
  /// Additional details for logging/debugging
  std::string details;

  ValidationResult() = default;
  
  ValidationResult(bool v, std::string_view msg = "", 
                  std::string_view sanitized = "") 
      : valid(v), error_message(msg), sanitized_value(sanitized) {}
};

/**
 * @class InputValidator
 * @brief Strict allowlist-based input validator with reject-by-default semantics
 * 
 * All validation follows these principles:
 * 1. Reject by default (fail on ANY unexpected condition)
 * 2. Whitelist known-good patterns (not blacklist known-bad)
 * 3. Validate all dimensions (length, characters, format, semantics)
 * 4. Log all validation failures for audit trail
 * 5. Sanitize output where needed (but validate first)
 */
class InputValidator {
 public:
  // ========================================================================
  // HTTP Request Validation
  // ========================================================================

  /**
   * @brief Validates HTTP request parameter with strict allowlist
   * @param param_name Parameter name (e.g., "user_id", "page")
   * @param param_value Parameter value to validate
   * @param max_length Maximum allowable length (default: 8192 bytes)
   * @return ValidationResult with validity status
   * 
   * Validation Rules:
   * - Name and value must not be empty
   * - Length must not exceed max_length
   * - Characters must be in allowlist (alphanumeric, underscore, hyphen, dot, space)
   * - No control characters, quotes, or special shell characters
   * 
   * Examples:
   * ✓ ValidateHttpParameter("user_id", "12345") 
   * ✓ ValidateHttpParameter("page", "search-results")
   * ✗ ValidateHttpParameter("query", "'; DROP TABLE users; --")
   * ✗ ValidateHttpParameter("path", "../../etc/passwd")
   */
  static ValidationResult ValidateHttpParameter(
      std::string_view param_name,
      std::string_view param_value,
      size_t max_length = 8192);

  /**
   * @brief Validates HTTP request body size
   * @param body_size Size of request body in bytes
   * @param max_size Maximum allowable size (default: 100MB)
   * @return ValidationResult with validity status
   * 
   * Validation Rules:
   * - Body size must not be zero
   * - Body size must not exceed max_size
   * 
   * Examples:
   * ✓ ValidateRequestBodySize(1024 * 1024)      // 1MB
   * ✗ ValidateRequestBodySize(500 * 1024 * 1024) // 500MB (too large)
   * ✗ ValidateRequestBodySize(0)                  // Empty body
   */
  static ValidationResult ValidateRequestBodySize(
      size_t body_size,
      size_t max_size = 104857600); // 100MB default

  /**
   * @brief Validates HTTP header value
   * @param header_name Header name (case-insensitive)
   * @param header_value Header value to validate
   * @return ValidationResult with validity status
   * 
   * Validation Rules:
   * - Value must not be empty
   * - Value must not contain CRLF sequences (prevents header injection)
   * - Value must not contain suspicious patterns (script, onerror, etc.)
   * 
   * Examples:
   * ✓ ValidateHttpHeader("Authorization", "******")
   * ✗ ValidateHttpHeader("X-Custom", "value\r\nX-Injected: evil")
   */
  static ValidationResult ValidateHttpHeader(
      std::string_view header_name,
      std::string_view header_value);

  /**
   * @brief Validates HTTP request path
   * @param path Request path/URI
   * @return ValidationResult with validity status
   * 
   * Validation Rules:
   * - Path must start with "/"
   * - Path must not contain ".." (path traversal prevention)
   * - Path must not contain control characters
   * - Percent-encoded sequences are validated
   */
  static ValidationResult ValidateHttpPath(std::string_view path);

  // ========================================================================
  // SQL Query Validation
  // ========================================================================

  /**
   * @brief Validates SQL query parameter value
   * @param param_value Parameter value to validate
   * @param param_type Expected parameter type (e.g., "string", "int", "uuid")
   * @return ValidationResult with validity status
   * 
   * Validation Rules:
   * - Value must match expected type
   * - String lengths must be within bounds
   * - Numeric values must be within range
   * - UUID must be valid format
   * - No raw SQL keywords in string parameters
   * 
   * Examples:
   * ✓ ValidateSQLParameter("123", "int")
   * ✓ ValidateSQLParameter("john@example.com", "email")
   * ✗ ValidateSQLParameter("1' OR '1'='1", "string")
   * ✗ ValidateSQLParameter("DROP TABLE users", "string")
   */
  static ValidationResult ValidateSQLParameter(
      std::string_view param_value,
      std::string_view param_type = "string");

  /**
   * @brief Validates SQL literal value from query
   * @param literal_value Literal value from SQL query
   * @param expected_type Expected type (string, number, boolean, null)
   * @return ValidationResult with validity status
   * 
   * Validation Rules:
   * - Literal must parse correctly for its type
   * - Numeric literals must be within type range
   * - String literals must use correct quoting
   * - No unescaped quotes in string literals
   */
  static ValidationResult ValidateSQLLiteral(
      std::string_view literal_value,
      std::string_view expected_type = "string");

  // ========================================================================
  // LLM Prompt Validation
  // ========================================================================

  /**
   * @brief Validates LLM model prompt input
   * @param prompt Prompt text to validate
   * @param max_length Maximum prompt length (default: 4096)
   * @return ValidationResult with validity status
   * 
   * Validation Rules:
   * - Prompt must not be empty
   * - Length must not exceed max_length
   * - Encoding must be valid UTF-8
   * - No control characters except newline/tab
   * - No prompt injection patterns detected
   * - Token count must be within limits
   * 
   * Examples:
   * ✓ ValidateLLMPrompt("What is 2+2?")
   * ✗ ValidateLLMPrompt("")  // Empty
   * ✗ ValidateLLMPrompt("ignore instructions: ...")  // Injection attempt
   */
  static ValidationResult ValidateLLMPrompt(
      std::string_view prompt,
      size_t max_length = 4096);

  /**
   * @brief Validates LLM model configuration parameter
   * @param param_name Parameter name (e.g., "temperature", "max_tokens")
   * @param param_value Parameter value to validate
   * @return ValidationResult with validity status
   * 
   * Validation Rules:
   * - Parameter name must be known and approved
   * - Numeric parameters must be within valid range
   * - String parameters must match approved patterns
   * 
   * Examples:
   * ✓ ValidateLLMParameter("temperature", "0.7")
   * ✓ ValidateLLMParameter("max_tokens", "512")
   * ✗ ValidateLLMParameter("temperature", "999")   // Out of range
   * ✗ ValidateLLMParameter("unknown_param", "x")   // Unknown parameter
   */
  static ValidationResult ValidateLLMParameter(
      std::string_view param_name,
      std::string_view param_value);

  // ========================================================================
  // Utility Functions
  // ========================================================================

  /**
   * @brief Checks if string contains only whitelisted characters
   * @param value String to check
   * @param allowed_pattern Regex pattern of allowed characters
   * @return true if all characters match pattern, false otherwise
   */
  static bool IsWhitelistedCharacters(
      std::string_view value,
      std::string_view allowed_pattern = "^[a-zA-Z0-9_.-]+$");

  /**
   * @brief Validates UTF-8 encoding
   * @param value String to validate
   * @return true if valid UTF-8, false otherwise
   */
  static bool IsValidUTF8(std::string_view value);

  /**
   * @brief Checks for common injection patterns
   * @param value String to check
   * @return true if injection patterns found, false otherwise
   */
  static bool ContainsInjectionPatterns(std::string_view value);

  /**
   * @brief Safely decodes percent-encoded string (URL decoding)
   * @param encoded Percent-encoded string
   * @return Decoded string, or empty optional if invalid encoding
   */
  static std::optional<std::string> SafeUrlDecode(std::string_view encoded);

  /**
   * @brief Escapes string for safe SQL usage (not a replacement for parameterized queries)
   * @param value String to escape
   * @return Escaped string safe for literal embedding
   * 
   * @warning Use parameterized queries instead of escaping when possible
   */
  static std::string EscapeForSQL(std::string_view value);

 private:
  // Injection pattern definitions
  static constexpr std::array<std::string_view, 10> SQL_INJECTION_PATTERNS = {
      R"(\bor\b\s+.+\s*=\s*.+)", // OR conditions
      R"(\bdrop\b\s+\btable\b)", // DROP TABLE
      R"(\bdelete\b\s+\bfrom\b)", // DELETE FROM
      R"(\binsert\b\s+\binto\b)", // INSERT INTO
      R"(\bupdate\b\s+\bset\b)", // UPDATE SET
      R"(;\s*--)", // Comment syntax
      R"(\bunion\b\s+\bselect\b)", // UNION queries
      R"(\bexec(?:ute)?\s*\()", // Dynamic execution
      R"(\beval\s*\()", // Code evaluation
      R"(\bselect\b\s+.+\bfrom\b)", // SELECT FROM
  };

  static constexpr std::array<std::string_view, 8> PROMPT_INJECTION_PATTERNS = {
      R"(\bignore\b.+\binstruction)",
      R"(\bforget\b.+\bprevious)",
      R"(\boverride\b.+\bsystem)",
      R"(\bsystem\b.+\bprompt)",
      R"(\bdeveloper\b.+\bmode)",
      R"(\bbypass\b.+\bfilter)",
      R"(\bexecute\b.+\bcode)",
      R"(\brun\b.+\bcommand)",
  };

  // HTTP parameter allowlist
  static bool IsAllowedHttpParameterCharacter(unsigned char c);
  
  // Validation helper functions
  static bool ValidateSQLKeywords(std::string_view value);
  static bool CheckPromptForInjection(std::string_view prompt);
};

} // namespace themis::security::phase4_hardening
