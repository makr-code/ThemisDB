/**
 * @file input_validator.hpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>
#include <string_view>
#include <regex>
#include <optional>
#include <vector>

namespace themis::security {

/**
 * @brief Validation context — indicates what type of validation to apply
 */
enum class ValidationContext {
  /// User input for SQL queries (parameterized, bounded)
  SQL_QUERY,
  
  /// User input for database table/column names (strict whitelist)
  SQL_IDENTIFIER,
  
  /// User-supplied JSON payloads (size limit, no malicious nesting)
  JSON_PAYLOAD,
  
  /// File upload names and paths (no traversal, safe extensions)
  FILE_UPLOAD,
  
  /// URI/URL parameters (no control characters, bounded)
  URI_PARAMETER,
  
  /// Plain text search input (size limit, no wildcards unless intended)
  SEARCH_QUERY,
  
  /// Configuration file paths (no traversal, must be under safe directory)
  CONFIG_PATH,
  
  /// API request headers (bounded, no null bytes)
  REQUEST_HEADER,
};

/**
 * @brief Input validation result with detailed error information
 */
struct ValidationResult {
  bool is_valid = 0;
  std::string error_message;
  std::string remediation_hint;
  
  explicit operator bool() const { return is_valid; }
};

/**
 * @brief InputValidator — Static validation methods for all input types
 */
class InputValidator {
 public:
  InputValidator() = delete;  // Static class
  
  // ========== USER INPUT VALIDATION ==========
  
  /**
   * @brief Validates user-supplied strings against injection attacks
   * 
   * @param input User input to validate
   * @param context Type of validation to apply (SQL, JSON, file, etc.)
   * @return ValidationResult with is_valid flag and error details
   */
  static ValidationResult validateUserInput(
      std::string_view input,
      ValidationContext context);
  
  /**
   * @brief Validates JSON payload structure and size
   * 
   * Checks:
   *   - Total size < 100MB
   *   - Nesting depth < 20 levels
   *   - No null bytes in strings
   *   - Valid UTF-8 encoding
   *
   * @param payload JSON string to validate
   * @return ValidationResult
   */
  static ValidationResult validateJsonPayload(std::string_view payload);
  
  /**
   * @brief Validates file upload: name, size, MIME type
   * 
   * Checks:
   *   - Filename contains only safe characters [a-zA-Z0-9._-]
   *   - No path traversal sequences (../, ..\, etc.)
   *   - Extension is whitelisted
   *   - Total size < configured maximum (default 500MB)
   *
   * @param filename Original filename from upload
   * @param file_size Size in bytes
   * @param mime_type Content-Type from HTTP header
   * @return ValidationResult
   */
  static ValidationResult validateFileUpload(
      std::string_view filename,
      size_t file_size,
      std::string_view mime_type);
  
  /**
   * @brief Validates URI/URL parameters
   * 
   * Checks:
   *   - No control characters (0x00-0x1F, 0x7F)
   *   - No null bytes
   *   - Valid percent-encoding if present
   *   - Length < 8192 bytes
   *
   * @param uri_param Parameter value from URL query string
   * @return ValidationResult
   */
  static ValidationResult validateUriParameter(std::string_view uri_param);
  
  /**
   * @brief Validates HTTP request headers
   * 
   * Checks:
   *   - No null bytes
   *   - No newline/carriage return (header injection)
   *   - Length < 8KB
   *   - Valid characters for header values
   *
   * @param header_name Header name (e.g., "User-Agent")
   * @param header_value Header value
   * @return ValidationResult
   */
  static ValidationResult validateRequestHeader(
      std::string_view header_name,
      std::string_view header_value);
  
  /**
   * @brief Validates search query input
   * 
   * Checks:
   *   - Length between 1-256 characters
   *   - No SQL wildcards unless explicitly allowed
   *   - No regex control characters unless in regex mode
   *
   * @param query Search string
   * @param allow_wildcards If true, allow * and % wildcards
   * @return ValidationResult
   */
  static ValidationResult validateSearchQuery(
      std::string_view query,
      bool allow_wildcards = false);
  
  // ========== OUTPUT SANITIZATION ==========
  
  /**
   * @brief Sanitizes string for safe display in HTML context
   * 
   * Escapes:
   *   - < → &lt;
   *   - > → &gt;
   *   - & → &amp;
   *   - " → &quot;
   *   - ' → &#39;
   *
   * @param input String to sanitize
   * @return HTML-safe string
   */
  static std::string sanitizeForHtml(std::string_view input);
  
  /**
   * @brief Sanitizes string for safe use in SQL (for logging only, not queries!)
   * 
   * WARNING: This is for LOGGING ONLY. Never use this for SQL queries!
   * Always use parameterized queries with placeholders (?).
   *
   * Escapes single quotes and backslashes.
   *
   * @param input String to sanitize
   * @return SQL-safe string (for logging)
   */
  static std::string sanitizeForSqlLogging(std::string_view input);
  
  /**
   * @brief Sanitizes string for safe use in shell commands (not recommended!)
   * 
   * WARNING: Avoid executing shell commands with user input.
   * Prefer native APIs when available.
   *
   * Wraps string in single quotes and escapes existing quotes.
   *
   * @param input String to sanitize
   * @return Shell-safe string
   */
  static std::string sanitizeForShell(std::string_view input);
  
  /**
   * @brief Sanitizes string for safe display in JSON
   * 
   * Escapes:
   *   - Backslash → \\
   *   - Newline → \n
   *   - Tab → \t
   *   - Quote → \"
   *   - Control characters → \uXXXX
   *
   * @param input String to sanitize
   * @return JSON-safe string
   */
  static std::string sanitizeForJson(std::string_view input);
  
  // ========== IDENTIFIER VALIDATION (whitelist-based) ==========
  
  /**
   * @brief Validates identifier (table/column/database name)
   * 
   * Uses strict whitelist: [a-zA-Z_][a-zA-Z0-9_]*
   * Maximum length: 128 characters
   *
   * @param identifier Name to validate
   * @return ValidationResult
   */
  static ValidationResult validateIdentifier(std::string_view identifier);
  
  /**
   * @brief Validates configuration file path
   * 
   * Checks:
   *   - Must be absolute or relative to safe directory
   *   - No path traversal (../, ..\)
   *   - Must end in allowed extension
   *   - Must exist and be readable
   *
   * @param path Path to validate
   * @param allowed_extension Allowed file extension (e.g., ".conf")
   * @return ValidationResult
   */
  static ValidationResult validateConfigPath(
      std::string_view path,
      std::string_view allowed_extension = ".conf");
  
  // ========== UTILITY METHODS ==========
  
  /**
   * @brief Checks if string contains only alphanumeric characters and underscores
   * @param input String to check
   * @return true if safe identifier, false otherwise
   */
  static bool isAlphanumericWithUnderscore(std::string_view input);
  
  /**
   * @brief Checks if string contains null bytes
   * @param input String to check
   * @return true if null bytes found, false otherwise
   */
  static bool containsNullBytes(std::string_view input);
  
  /**
   * @brief Checks if string contains control characters (0x00-0x1F, 0x7F)
   * @param input String to check
   * @return true if control characters found, false otherwise
   */
  static bool containsControlCharacters(std::string_view input);
  
  /**
   * @brief Checks if string is valid UTF-8
   * @param input String to check
   * @return true if valid UTF-8, false otherwise
   */
  static bool isValidUtf8(std::string_view input);

 private:
  // Configuration constants
  static constexpr size_t MAX_FIELD_SIZE = 10'000;          // 10 KB per field
  static constexpr size_t MAX_JSON_SIZE = 100'000'000;      // 100 MB
  static constexpr size_t MAX_IDENTIFIER_LENGTH = 128;
  static constexpr size_t MAX_URI_PARAMETER_SIZE = 8'192;
  static constexpr size_t MAX_HEADER_VALUE_SIZE = 8'192;
  static constexpr size_t MAX_JSON_NESTING_DEPTH = 20;
  static constexpr size_t MAX_FILE_UPLOAD_SIZE = 500'000'000;  // 500 MB
  
  // Allowed file extensions for upload (whitelist)
  static constexpr std::string_view ALLOWED_UPLOAD_EXTENSIONS[] = {
    ".txt", ".csv", ".json", ".xml",
    ".pdf", ".doc", ".docx",
    ".jpg", ".jpeg", ".png", ".gif",
    ".zip", ".tar", ".gz"
  };
  
  // Validation helper methods
  static bool validateSqlQueryPattern(std::string_view input);
  static bool validateSqlInjectionRisk(std::string_view input);
  static bool validateXssRisk(std::string_view input);
  static bool validatePathTraversal(std::string_view path);
};

}  // namespace themis::security
