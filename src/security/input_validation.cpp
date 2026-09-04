/**
 * @file input_validation.cpp
 * @brief Implementation of input validation for Phase 4 security hardening
 * 
 * @version 1.0
 * @date 2026-07-28
 */

#include "security/input_validation.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <map>
#include <regex>
#include <sstream>

namespace themis::security::phase4_hardening {

// ============================================================================
// HTTP Request Validation Implementation
// ============================================================================

ValidationResult InputValidator::ValidateHttpParameter(
    std::string_view param_name,
    std::string_view param_value,
    size_t max_length) {
  // Rule 1: Parameter name and value must not be empty
  if (param_name.empty()) {
    return {false, "Parameter name is empty"};
  }
  if (param_value.empty()) {
    return {false, "Parameter value is empty"};
  }

  // Rule 2: Length must not exceed max_length
  if (param_value.length() > max_length) {
    return {false, "Parameter value exceeds maximum length of " + 
                  std::to_string(max_length)};
  }

  // Rule 3: Characters must be whitelisted
  for (unsigned char c : param_value) {
    if (!IsAllowedHttpParameterCharacter(c)) {
      return {false, "Parameter contains invalid character: " + 
                    std::string(1, c)};
    }
  }

  // Rule 4: Check for injection patterns
  if (ContainsInjectionPatterns(param_value)) {
    return {false, "Parameter contains suspicious patterns"};
  }

  // All validation passed
  return {true, "", std::string(param_value)};
}

ValidationResult InputValidator::ValidateRequestBodySize(
    size_t body_size,
    size_t max_size) {
  // Rule 1: Body size must not be zero
  if (body_size == 0) {
    return {false, "Request body is empty"};
  }

  // Rule 2: Body size must not exceed max_size
  if (body_size > max_size) {
    return {false, 
            "Request body size (" + std::to_string(body_size) + 
            " bytes) exceeds maximum of " + std::to_string(max_size)};
  }

  return {true, ""};
}

ValidationResult InputValidator::ValidateHttpHeader(
    std::string_view header_name,
    std::string_view header_value) {
  // Rule 1: Value must not be empty
  if (header_value.empty()) {
    return {false, "Header value is empty"};
  }

  // Rule 2: Check for CRLF injection (header injection prevention)
  static constexpr std::array<std::string_view, 3> CRLF_PATTERNS = {
      "\r\n", "\n\r", "\r"
  };

  for (const auto& pattern : CRLF_PATTERNS) {
    if (header_value.find(pattern) != std::string::npos) {
      return {false, "Header value contains CRLF sequence (header injection)"};
    }
  }

  // Rule 3: Check for suspicious patterns
  static constexpr std::array<std::string_view, 6> SUSPICIOUS_PATTERNS = {
      "://", "eval(", "script", "onerror", "onload", "javascript:"
  };

  for (const auto& pattern : SUSPICIOUS_PATTERNS) {
    if (header_value.find(pattern) != std::string::npos) {
      return {false, 
              "Header value contains suspicious pattern: " + 
              std::string(pattern)};
    }
  }

  // Rule 4: Check for control characters
  for (unsigned char c : header_value) {
    if (c < 32 && c != '\t') { // Allow tab (0x09) but not other control chars
      return {false, "Header value contains control character"};
    }
  }

  return {true, "", std::string(header_value)};
}

ValidationResult InputValidator::ValidateHttpPath(std::string_view path) {
  // Rule 1: Path must start with "/"
  if (path.empty() || path[0] != '/') {
    return {false, "Path must start with '/'"};
  }

  // Rule 2: Check for path traversal (../ sequences)
  if (path.find("..") != std::string::npos) {
    return {false, "Path contains '..' (path traversal attempt)"};
  }

  // Rule 3: Check for control characters
  for (unsigned char c : path) {
    if (c < 32) {
      return {false, "Path contains control character"};
    }
  }

  // Rule 4: Validate percent-encoded sequences
  for (size_t i = 0; i < path.length(); ++i) {
    if (path[i] == '%') {
      // Must be followed by exactly 2 hex digits
      if (i + 2 >= path.length() ||
          !std::isxdigit(static_cast<unsigned char>(path[i + 1])) ||
          !std::isxdigit(static_cast<unsigned char>(path[i + 2]))) {
        return {false, "Invalid percent-encoding in path"};
      }
      i += 2; // Skip the hex digits
    }
  }

  return {true, "", std::string(path)};
}

// ============================================================================
// SQL Query Validation Implementation
// ============================================================================

ValidationResult InputValidator::ValidateSQLParameter(
    std::string_view param_value,
    std::string_view param_type) {
  // Rule 1: Value must not be empty
  if (param_value.empty()) {
    return {false, "SQL parameter value is empty"};
  }

  // Rule 2: Validate based on type
  if (param_type == "int") {
    // Must be valid integer
    try {
      std::stoll(std::string(param_value));
    } catch (...) {
      return {false, "Parameter is not a valid integer"};
    }
  } else if (param_type == "float") {
    // Must be valid float
    try {
      std::stod(std::string(param_value));
    } catch (...) {
      return {false, "Parameter is not a valid floating-point number"};
    }
  } else if (param_type == "uuid") {
    // Must be valid UUID format (36 chars: 8-4-4-4-12)
    std::regex uuid_pattern(
        "^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-"
        "[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$");
    if (!std::regex_match(std::string(param_value), uuid_pattern)) {
      return {false, "Parameter is not a valid UUID"};
    }
  } else if (param_type == "email") {
    // Basic email validation
    std::regex email_pattern("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
    if (!std::regex_match(std::string(param_value), email_pattern)) {
      return {false, "Parameter is not a valid email address"};
    }
  } else if (param_type == "string") {
    // Check for SQL injection patterns in strings
    if (ValidateSQLKeywords(param_value)) {
      return {false, "Parameter contains SQL keywords (possible injection)"};
    }
    if (ContainsInjectionPatterns(param_value)) {
      return {false, "Parameter contains injection patterns"};
    }
  }

  return {true, "", std::string(param_value)};
}

ValidationResult InputValidator::ValidateSQLLiteral(
    std::string_view literal_value,
    std::string_view expected_type) {
  // Rule 1: Value must not be empty
  if (literal_value.empty()) {
    return {false, "SQL literal value is empty"};
  }

  // Rule 2: Validate based on type
  if (expected_type == "number") {
    try {
      std::stod(std::string(literal_value));
    } catch (...) {
      return {false, "Literal is not a valid number"};
    }
  } else if (expected_type == "string") {
    // String literals must be properly quoted
    if ((literal_value.front() != '\'' || literal_value.back() != '\'') &&
        (literal_value.front() != '"' || literal_value.back() != '"')) {
      return {false, "String literal must be properly quoted"};
    }

    // Check for unescaped quotes
    auto quote_char = literal_value.front();
    for (size_t i = 1; i < literal_value.length() - 1; ++i) {
      if (literal_value[i] == quote_char) {
        // In SQL, doubled quotes are the escape mechanism
        if (i + 1 < literal_value.length() && literal_value[i + 1] == quote_char) {
          i++; // Skip the paired quote
        } else {
          return {false, "String literal contains unescaped quote"};
        }
      }
    }
  } else if (expected_type == "boolean") {
    // Must be TRUE, FALSE, true, or false
    if (literal_value != "TRUE" && literal_value != "FALSE" &&
        literal_value != "true" && literal_value != "false") {
      return {false, "Boolean literal must be TRUE or FALSE"};
    }
  } else if (expected_type == "null") {
    // Must be NULL
    if (literal_value != "NULL" && literal_value != "null") {
      return {false, "Null literal must be NULL"};
    }
  }

  return {true, "", std::string(literal_value)};
}

// ============================================================================
// LLM Prompt Validation Implementation
// ============================================================================

ValidationResult InputValidator::ValidateLLMPrompt(
    std::string_view prompt,
    size_t max_length) {
  // Rule 1: Prompt must not be empty
  if (prompt.empty()) {
    return {false, "Prompt is empty"};
  }

  // Rule 2: Length must not exceed max_length
  if (prompt.length() > max_length) {
    return {false, 
            "Prompt exceeds maximum length of " + std::to_string(max_length)};
  }

  // Rule 3: Encoding must be valid UTF-8
  if (!IsValidUTF8(prompt)) {
    return {false, "Prompt contains invalid UTF-8 encoding"};
  }

  // Rule 4: Check for invalid control characters (except newline/tab)
  for (unsigned char c : prompt) {
    if (c < 32 && c != '\n' && c != '\t' && c != '\r') {
      return {false, "Prompt contains invalid control character"};
    }
  }

  // Rule 5: Check for prompt injection patterns
  if (CheckPromptForInjection(prompt)) {
    return {false, "Prompt contains injection patterns"};
  }

  return {true, "", std::string(prompt)};
}

ValidationResult InputValidator::ValidateLLMParameter(
    std::string_view param_name,
    std::string_view param_value) {
  // Approved parameters and their valid ranges
  static const std::map<std::string_view, std::pair<double, double>> 
      NUMERIC_PARAMS = {
    {"temperature", {0.0, 2.0}},
    {"top_p", {0.0, 1.0}},
    {"top_k", {0.0, 100.0}},
    {"max_tokens", {1.0, 4096.0}},
    {"frequency_penalty", {-2.0, 2.0}},
    {"presence_penalty", {-2.0, 2.0}},
  };

  static const std::vector<std::string_view> STRING_PARAMS = {
    "stop",
    "model",
    "system_prompt",
  };

  // Rule 1: Parameter name must be known
  bool param_known = false;
  bool is_numeric = NUMERIC_PARAMS.find(param_name) != NUMERIC_PARAMS.end();
  bool is_string = std::find(STRING_PARAMS.begin(), STRING_PARAMS.end(),
                             param_name) != STRING_PARAMS.end();

  if (!is_numeric && !is_string) {
    return {false, 
            "Unknown LLM parameter: " + std::string(param_name)};
  }

  // Rule 2: Validate value based on parameter type
  if (is_numeric) {
    try {
      double value = std::stod(std::string(param_value));
      auto [min, max] = NUMERIC_PARAMS.at(param_name);
      
      if (value < min || value > max) {
        return {false, 
                "Parameter value out of range [" + std::to_string(min) + 
                ", " + std::to_string(max) + "]"};
      }
    } catch (...) {
      return {false, 
              "Parameter value is not a valid number"};
    }
  } else if (is_string) {
    // String parameters must not be empty
    if (param_value.empty()) {
      return {false, "String parameter value is empty"};
    }
    
    // Check for injection patterns
    if (ContainsInjectionPatterns(param_value)) {
      return {false, "Parameter contains injection patterns"};
    }
  }

  return {true, "", std::string(param_value)};
}

// ============================================================================
// Utility Function Implementation
// ============================================================================

bool InputValidator::IsWhitelistedCharacters(
    std::string_view value,
    std::string_view allowed_pattern) {
  try {
    std::regex pattern{std::string(allowed_pattern)};
    return std::regex_match(std::string(value), pattern);
  } catch (...) {
    return false; // Invalid regex or no match
  }
}

bool InputValidator::IsValidUTF8(std::string_view value) {
  for (size_t i = 0; i < value.length(); ++i) {
    unsigned char byte = static_cast<unsigned char>(value[i]);
    
    if ((byte & 0x80) == 0) {
      // ASCII character
      continue;
    } else if ((byte & 0xE0) == 0xC0) {
      // 2-byte sequence
      if (i + 1 >= value.length()) {
        return false;
      }
      unsigned char next = static_cast<unsigned char>(value[i + 1]);
      if ((next & 0xC0) != 0x80) {
        return false;
      }
      i++;
    } else if ((byte & 0xF0) == 0xE0) {
      // 3-byte sequence
      if (i + 2 >= value.length()) {
        return false;
      }
      for (int j = 1; j <= 2; j++) {
        unsigned char next = static_cast<unsigned char>(value[i + j]);
        if ((next & 0xC0) != 0x80) {
          return false;
        }
      }
      i += 2;
    } else if ((byte & 0xF8) == 0xF0) {
      // 4-byte sequence
      if (i + 3 >= value.length()) {
        return false;
      }
      for (int j = 1; j <= 3; j++) {
        unsigned char next = static_cast<unsigned char>(value[i + j]);
        if ((next & 0xC0) != 0x80) {
          return false;
        }
      }
      i += 3;
    } else {
      return false; // Invalid UTF-8
    }
  }
  return true;
}

bool InputValidator::ContainsInjectionPatterns(std::string_view value) {
  try {
    for (const auto& pattern : SQL_INJECTION_PATTERNS) {
      if (std::regex_search(value.begin(), value.end(),
                            std::regex(std::string(pattern),
                                       std::regex_constants::icase))) {
        return true;
      }
    }

    for (const auto& pattern : PROMPT_INJECTION_PATTERNS) {
      if (std::regex_search(value.begin(), value.end(),
                            std::regex(std::string(pattern),
                                       std::regex_constants::icase))) {
        return true;
      }
    }
  } catch (const std::regex_error&) {
    return true;
  }

  return false;
}

std::optional<std::string> InputValidator::SafeUrlDecode(
    std::string_view encoded) {
  std::string decoded = {};
  
  for (size_t i = 0; i < encoded.length(); ++i) {
    if (encoded[i] == '%') {
      // Must have 2 hex digits following
      if (i + 2 >= encoded.length()) {
        return std::nullopt; // Invalid encoding
      }
      
      char hex[3] = {static_cast<char>(encoded[i + 1]), 
                     static_cast<char>(encoded[i + 2]), '\0'};
      
      if (!std::isxdigit(hex[0]) || !std::isxdigit(hex[1])) {
        return std::nullopt; // Invalid hex digits
      }
      
      // Convert hex to character
      int char_code = std::stoi(hex, nullptr, 16);
      decoded += static_cast<char>(char_code);
      i += 2;
    } else if (encoded[i] == '+') {
      decoded += ' '; // Plus means space in URL encoding
    } else {
      decoded += encoded[i];
    }
  }
  
  return decoded;
}

std::string InputValidator::EscapeForSQL(std::string_view value) {
  std::string escaped = {};
  
  for (char c : value) {
    if (c == '\'') {
      escaped += "''"; // SQL escape: double the quote
    } else if (c == '"') {
      escaped += "\"\""; // Escape double quote too
    } else if (c == '\\') {
      escaped += "\\\\"; // Escape backslash
    } else if (c < 32) {
      // Control characters are not allowed
      escaped += " ";
    } else {
      escaped += c;
    }
  }
  
  return escaped;
}

// ============================================================================
// Private Helper Functions
// ============================================================================

bool InputValidator::IsAllowedHttpParameterCharacter(unsigned char c) {
  // Whitelist: alphanumeric, underscore, hyphen, dot, space
  return std::isalnum(c) || c == '_' || c == '-' || c == '.' || c == ' ';
}

bool InputValidator::ValidateSQLKeywords(std::string_view value) {
  std::string lower_value(value);
  std::transform(lower_value.begin(), lower_value.end(),
                 lower_value.begin(), ::tolower);

  static constexpr std::array<std::string_view, 15> SQL_KEYWORDS = {
      "select", "insert", "update", "delete", "drop", "create",
      "alter", "exec", "execute", "union", "grant", "revoke",
      "truncate", "declare", "or",
  };

  for (const auto& keyword : SQL_KEYWORDS) {
    // Look for keyword boundaries (word-wrapped)
    if (lower_value.find(keyword) != std::string::npos) {
      return true; // Found a SQL keyword
    }
  }

  return false;
}

bool InputValidator::CheckPromptForInjection(std::string_view prompt) {
  try {
    for (const auto& pattern : PROMPT_INJECTION_PATTERNS) {
      if (std::regex_search(prompt.begin(), prompt.end(),
                            std::regex(std::string(pattern),
                                       std::regex_constants::icase))) {
        return true;
      }
    }
  } catch (const std::regex_error&) {
    return true;
  }

  return false;
}

} // namespace themis::security::phase4_hardening
