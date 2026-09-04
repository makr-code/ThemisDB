/**
 * @file input_validator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=11, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "security/input_validator.hpp"
#include <algorithm>
#include <cctype>
#include <iomanip>

namespace themis::security {

// ============================================================================
// VALIDATION METHODS
// ============================================================================

ValidationResult InputValidator::validateUserInput(
    std::string_view input,
    ValidationContext context) {
  
  if (input.empty()) {
    return {true, "", ""};
  }
  
  switch (context) {
    case ValidationContext::SQL_QUERY:
      // Check for SQL injection patterns
      if (!validateSqlInjectionRisk(input)) {
        return {false, 
                "Input contains SQL injection patterns",
                "Use parameterized queries with placeholders (?) instead of string concatenation"};
      }
      if (input.size() > MAX_FIELD_SIZE) {
        return {false,
                "Input exceeds maximum field size (" + std::to_string(MAX_FIELD_SIZE) + " bytes)",
                "Truncate input or increase MAX_FIELD_SIZE if legitimate"};
      }
      return {true, "", ""};
    
    case ValidationContext::SQL_IDENTIFIER:
      // Strict whitelist: [a-zA-Z_][a-zA-Z0-9_]*
      if (!validateIdentifier(input)) {
        return {false,
                "Identifier contains invalid characters",
                "Use only letters, numbers, and underscores; start with letter or underscore"};
      }
      return {true, "", ""};
    
    case ValidationContext::JSON_PAYLOAD:
      return validateJsonPayload(input);
    
    case ValidationContext::FILE_UPLOAD: {
      // For file uploads, context should include name + size
      // This is a simplified check; validateFileUpload() is more complete
      if (input.size() > MAX_FILE_UPLOAD_SIZE) {
        return {false,
                "File size exceeds maximum (" + std::to_string(MAX_FILE_UPLOAD_SIZE) + " bytes)",
                "Upload a smaller file or contact admin to increase limit"};
      }
      return {true, "", ""};
    }
    
    case ValidationContext::URI_PARAMETER:
      return validateUriParameter(input);
    
    case ValidationContext::SEARCH_QUERY:
      return validateSearchQuery(input, false);
    
    case ValidationContext::CONFIG_PATH:
      if (!validatePathTraversal(input)) {
        return {false,
                "Path contains traversal sequences (../, ..\\)",
                "Use absolute paths or paths relative to safe directory"};
      }
      return {true, "", ""};
    
    case ValidationContext::REQUEST_HEADER:
      return validateRequestHeader("", input);
  }
  
  return {false, "Unknown validation context", ""};
}

ValidationResult InputValidator::validateJsonPayload(std::string_view payload) {
  // Check size limit
  if (payload.size() > MAX_JSON_SIZE) {
    return {false,
            "JSON payload exceeds maximum size (" + std::to_string(MAX_JSON_SIZE) + " bytes)",
            "Reduce payload size or increase MAX_JSON_SIZE"};
  }
  
  // Check for null bytes
  if (containsNullBytes(payload)) {
    return {false,
            "JSON payload contains null bytes",
            "Remove null bytes from payload"};
  }
  
  // Check for valid UTF-8
  if (!isValidUtf8(payload)) {
    return {false,
            "JSON payload is not valid UTF-8",
            "Ensure all characters are valid UTF-8 encoded"};
  }
  
  // Check nesting depth (simple heuristic: count braces)
  size_t depth = 0, max_depth = 0;
  for (char c : payload) {
    if (c == '{' || c == '[') {
      depth++;
      if (depth > max_depth) {
        max_depth = depth;
      }
    } else if (c == '}' || c == ']') {
      if (depth > 0) {
        depth--;
      }
    }
  }
  
  if (max_depth > MAX_JSON_NESTING_DEPTH) {
    return {false,
            "JSON nesting depth exceeds maximum (" + std::to_string(MAX_JSON_NESTING_DEPTH) + ")",
            "Flatten JSON structure or increase nesting limit"};
  }
  
  return {true, "", ""};
}

ValidationResult InputValidator::validateFileUpload(
    std::string_view filename,
    size_t file_size,
    std::string_view mime_type) {
  (void)mime_type;
  
  // Check filename for traversal attacks
  if (!validatePathTraversal(filename)) {
    return {false,
            "Filename contains path traversal sequences",
            "Use only safe filenames without ../ or ..\\"};
  }
  
  // Check filename contains only safe characters
  for (char c : filename) {
    if (!isalnum(c) && c != '.' && c != '_' && c != '-') {
      return {false,
              "Filename contains invalid character: '" + std::string(1, c) + "'",
              "Use only [a-zA-Z0-9._-]"};
    }
  }
  
  // Check extension is whitelisted
  size_t dot_pos = filename.rfind('.');
  if (dot_pos == std::string_view::npos) {
    return {false,
            "Filename has no extension",
            "Provide a file extension"};
  }
  
  std::string ext(filename.substr(dot_pos));
  // Convert to lowercase for comparison
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
  
  bool ext_allowed = false;
  for (auto allowed : ALLOWED_UPLOAD_EXTENSIONS) {
    if (ext == allowed) {
      ext_allowed = true;
      break;
    }
  }
  
  if (!ext_allowed) {
    return {false,
            "File extension '" + ext + "' is not allowed",
            "Upload a file with an allowed extension"};
  }
  
  // Check file size limit
  if (file_size > MAX_FILE_UPLOAD_SIZE) {
    return {false,
            "File size exceeds maximum (" + std::to_string(MAX_FILE_UPLOAD_SIZE) + " bytes)",
            "Upload a smaller file"};
  }
  
  return {true, "", ""};
}

ValidationResult InputValidator::validateUriParameter(std::string_view uri_param) {
  // Check size limit
  if (uri_param.size() > MAX_URI_PARAMETER_SIZE) {
    return {false,
            "URI parameter exceeds maximum size (" + std::to_string(MAX_URI_PARAMETER_SIZE) + " bytes)",
            "Reduce parameter size"};
  }
  
  // Check for null bytes
  if (containsNullBytes(uri_param)) {
    return {false,
            "URI parameter contains null bytes",
            "Remove null bytes"};
  }
  
  // Check for control characters
  if (containsControlCharacters(uri_param)) {
    return {false,
            "URI parameter contains control characters",
            "Use percent-encoding for special characters"};
  }
  
  return {true, "", ""};
}

ValidationResult InputValidator::validateRequestHeader(
    std::string_view header_name,
    std::string_view header_value) {
  (void)header_name;
  
  // Check value size limit
  if (header_value.size() > MAX_HEADER_VALUE_SIZE) {
    return {false,
            "Header value exceeds maximum size (" + std::to_string(MAX_HEADER_VALUE_SIZE) + " bytes)",
            "Reduce header value size"};
  }
  
  // Check for null bytes (header injection)
  if (containsNullBytes(header_value)) {
    return {false,
            "Header value contains null bytes",
            "Remove null bytes"};
  }
  
  // Check for newline/carriage return (CRLF injection)
  if (header_value.find('\n') != std::string_view::npos ||
      header_value.find('\r') != std::string_view::npos) {
    return {false,
            "Header value contains newline/carriage return (CRLF injection detected)",
            "Remove \\n and \\r from header value"};
  }
  
  return {true, "", ""};
}

ValidationResult InputValidator::validateSearchQuery(
    std::string_view query,
    bool allow_wildcards) {
  
  // Check length
  if (query.empty()) {
    return {false,
            "Search query is empty",
            "Provide a search term"};
  }
  
  if (query.size() > 256) {
    return {false,
            "Search query exceeds 256 characters",
            "Reduce query length"};
  }
  
  // Check for SQL injection if this is a SQL search
  if (!allow_wildcards && !validateSqlInjectionRisk(query)) {
    return {false,
            "Search query contains SQL injection patterns",
            "Use parameterized queries"};
  }
  
  return {true, "", ""};
}

ValidationResult InputValidator::validateIdentifier(std::string_view identifier) {
  // Check length
  if (identifier.empty() || identifier.size() > MAX_IDENTIFIER_LENGTH) {
    return {false,
            "Identifier length invalid (must be 1-" + std::to_string(MAX_IDENTIFIER_LENGTH) + ")",
            "Choose a name within the length limit"};
  }
  
  // First character must be letter or underscore
  if (!isalpha(identifier[0]) && identifier[0] != '_') {
    return {false,
            "Identifier must start with letter or underscore",
            "Start identifier with [a-zA-Z_]"};
  }
  
  // Remaining characters must be alphanumeric or underscore
  for (size_t i = 1; i < identifier.size(); ++i) {
    if (!isalnum(identifier[i]) && identifier[i] != '_') {
      return {false,
              "Identifier contains invalid character '" + std::string(1, identifier[i]) + "'",
              "Use only [a-zA-Z0-9_]"};
    }
  }
  
  return {true, "", ""};
}

ValidationResult InputValidator::validateConfigPath(
    std::string_view path,
    std::string_view allowed_extension) {
  
  // Check for path traversal
  if (!validatePathTraversal(path)) {
    return {false,
            "Path contains traversal sequences",
            "Use absolute paths or paths relative to safe directory"};
  }
  
  // Check extension if specified
  if (!allowed_extension.empty()) {
    if (path.find(allowed_extension) == std::string_view::npos) {
      return {false,
              "Path does not end with allowed extension '" + std::string(allowed_extension) + "'",
              "Use a file with the correct extension"};
    }
  }
  
  return {true, "", ""};
}

// ============================================================================
// SANITIZATION METHODS
// ============================================================================

std::string InputValidator::sanitizeForHtml(std::string_view input) {
  std::string output;
  output.reserve(input.size() + (input.size() / 5));  // Typical overhead ~20%
  
  for (char c : input) {
    switch (c) {
      case '<':  output += "&lt;"; break;
      case '>':  output += "&gt;"; break;
      case '&':  output += "&amp;"; break;
      case '"':  output += "&quot;"; break;
      case '\'': output += "&#39;"; break;
      default:   output += c;
    }
  }
  
  return output;
}

std::string InputValidator::sanitizeForSqlLogging(std::string_view input) {
  // WARNING: For logging only, not for actual SQL queries!
  std::string output;
  output.reserve(input.size());
  
  for (char c : input) {
    if (c == '\'' || c == '\\') {
      output += '\\';
    }
    output += c;
  }
  
  return output;
}

std::string InputValidator::sanitizeForShell(std::string_view input) {
  // WARNING: Avoid executing shell commands with user input
  // Wrap in single quotes and escape existing quotes
  std::string output = "'";
  
  for (char c : input) {
    if (c == '\'') {
      output += "'\\''";  // End quote, escaped quote, start quote
    } else {
      output += c;
    }
  }
  
  output += "'";
  return output;
}

std::string InputValidator::sanitizeForJson(std::string_view input) {
  std::string output;
  output.reserve(input.size() + (input.size() / 5));
  
  for (unsigned char c : input) {
    switch (c) {
      case '"':  output += "\\\""; break;
      case '\\': output += "\\\\"; break;
      case '\b': output += "\\b"; break;
      case '\f': output += "\\f"; break;
      case '\n': output += "\\n"; break;
      case '\r': output += "\\r"; break;
      case '\t': output += "\\t"; break;
      default:
        if (c < 0x20 || c == 0x7F) {
          // Control character — encode as \uXXXX
          char buf[7];
          snprintf(buf, sizeof(buf), "\\u%04X", (unsigned)c);
          output += buf;
        } else {
          output += c;
        }
    }
  }
  
  return output;
}

// ============================================================================
// UTILITY METHODS
// ============================================================================

bool InputValidator::isAlphanumericWithUnderscore(std::string_view input) {
  for (char c : input) {
    if (!isalnum(c) && c != '_') {
      return false;
    }
  }
  return true;
}

bool InputValidator::containsNullBytes(std::string_view input) {
  return input.find('\0') != std::string_view::npos;
}

bool InputValidator::containsControlCharacters(std::string_view input) {
  for (unsigned char c : input) {
    if (c <= 0x1F || c == 0x7F) {
      return true;
    }
  }
  return false;
}

bool InputValidator::isValidUtf8(std::string_view input) {
  size_t i = 0;
  while (i < input.size()) {
    unsigned char c = static_cast<unsigned char>(input[i]);
    
    if (c < 0x80) {
      // Single-byte character (ASCII)
      i++;
    } else if ((c & 0xE0) == 0xC0) {
      // Two-byte character
      if (i + 1 >= input.size() || (input[i + 1] & 0xC0) != 0x80) {
        return false;
      }
      i += 2;
    } else if ((c & 0xF0) == 0xE0) {
      // Three-byte character
      if (i + 2 >= input.size() ||
          (input[i + 1] & 0xC0) != 0x80 ||
          (input[i + 2] & 0xC0) != 0x80) {
        return false;
      }
      i += 3;
    } else if ((c & 0xF8) == 0xF0) {
      // Four-byte character
      if (i + 3 >= input.size() ||
          (input[i + 1] & 0xC0) != 0x80 ||
          (input[i + 2] & 0xC0) != 0x80 ||
          (input[i + 3] & 0xC0) != 0x80) {
        return false;
      }
      i += 4;
    } else {
      // Invalid UTF-8
      return false;
    }
  }
  return true;
}

// ============================================================================
// PRIVATE VALIDATION HELPERS
// ============================================================================

bool InputValidator::validateSqlInjectionRisk(std::string_view input) {
  // Check for common SQL injection patterns
  static constexpr std::string_view DANGEROUS_PATTERNS[] = {
    "' OR ",
    "'; DROP",
    "' AND ",
    "\" OR ",
    "\" AND ",
    "; DROP",
    "UNION SELECT",
    "INSERT INTO",
    "DELETE FROM",
    "UPDATE SET",
    "1=1",
    "1' OR '1'='1",
  };
  
  std::string upper_input(input);
  std::transform(upper_input.begin(), upper_input.end(), 
                 upper_input.begin(), ::toupper);
  
  for (auto pattern : DANGEROUS_PATTERNS) {
    if (upper_input.find(pattern) != std::string::npos) {
      return false;  // Dangerous pattern found
    }
  }
  
  return true;  // No dangerous patterns detected
}

bool InputValidator::validateXssRisk(std::string_view input) {
  // Check for common XSS patterns
  static constexpr std::string_view XSS_PATTERNS[] = {
    "<script",
    "javascript:",
    "onclick",
    "onerror",
    "onload",
    "<iframe",
    "<object",
    "<embed",
  };
  
  std::string lower_input(input);
  std::transform(lower_input.begin(), lower_input.end(),
                 lower_input.begin(), ::tolower);
  
  for (auto pattern : XSS_PATTERNS) {
    if (lower_input.find(pattern) != std::string::npos) {
      return false;  // XSS pattern found
    }
  }
  
  return true;  // No XSS patterns detected
}

bool InputValidator::validatePathTraversal(std::string_view path) {
  // Check for path traversal sequences
  if (path.find("../") != std::string_view::npos ||
      path.find("..\\") != std::string_view::npos ||
      path.find("..") == 0) {  // Starts with ..
    return false;
  }
  
  return true;
}

}  // namespace themis::security
