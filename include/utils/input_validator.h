/**
 * @file input_validator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <optional>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace themis {
namespace utils {

/** @brief Validator for input. */
class InputValidator {
public:
    // Default constructor: no schema directory (schema validation skipped).
    InputValidator();

    explicit InputValidator(std::string schema_dir);

    // JSON schema validation against a JSON Schema Draft-7 file.
    // Loads the schema from <schema_dir>/<schema_name>.json and validates `payload` against it.
    // Supported keywords:
    //   Top-level:   type ("object"), required, properties, additionalProperties
    //   Per property: type (string/object/number/integer/boolean/array/null),
    //                 enum, minLength, maxLength, pattern,
    //                 minimum, maximum, exclusiveMinimum, exclusiveMaximum
    // Returns std::nullopt if valid (or if no schema file is found), otherwise an error message.
    std::optional<std::string> validateJsonSchema(
        const nlohmann::json& payload,
        const std::string& schema_name
    ) const;

    // Backward-compatible alias for older call sites/tests.
    std::optional<std::string> validateJsonStub(
        const nlohmann::json& payload,
        const std::string& schema_name
    ) const;

    // JSON schema validation against an in-memory JSON Schema Draft-7 object.
    // Same keyword support as validateJsonStub.
    // Returns std::nullopt if valid, otherwise an error message.
    static std::optional<std::string> validateJson(
        const nlohmann::json& payload,
        const nlohmann::json& schema
    );

    // Validate AQL request payload (expects keys like {"query": "...", "bindVars": {...}})
    // Performs minimal checks: required fields, max length, disallowed characters/patterns
    std::optional<std::string> validateAqlRequest(const nlohmann::json& payload) const;

    // Validate path segment (e.g., entity key); rejects traversal and separators
    bool validatePathSegment(const std::string& segment) const;

    // Sanitize strings for logs (strip control chars and truncate)
    std::string sanitizeForLogs(const std::string& input, size_t max_len = 512) const;

    // Configure/query schema directory
    const std::string& schemaDir() const { return schema_dir_; }

    // -------------------------------------------------------------------------
    // Security-focused validation methods
    // -------------------------------------------------------------------------

    // Validate an AQL query string for injection patterns (boolean injection,
    // comment markers, dangerous operations such as REMOVE/DROP/UNION).
    // Returns true if the query appears safe, false if suspicious patterns are found.
    bool validateAQLQuery(const std::string& query) const;

    // Validate a file path, rejecting directory traversal sequences, URL-encoded
    // traversal, dangerous protocols (file://), and sensitive OS paths (/proc/).
    // Returns true if the path appears safe, false otherwise.
    bool validateFilePath(const std::string& path) const;

    // Sanitize a string for safe embedding in HTML:
    // - strips javascript:/vbscript: protocol prefixes (case-insensitive)
    // - removes on* event-handler attributes (e.g. onerror=, onload=)
    // - HTML-encodes remaining special characters (& < > " ' /)
    std::string sanitizeForHTML(const std::string& input) const;

    // Validate a filename, rejecting shell metacharacters that enable command
    // injection (;  |  &  `  $  (  )  {  }  [  ]  <  >  !  ?  *  " \).
    // Returns true if the filename is safe, false otherwise.
    bool validateFilename(const std::string& filename) const;

    // Check a JSON string value for NoSQL/MongoDB operator injections
    // (e.g. $gt, $ne, $regex) and JavaScript injection patterns (return true, ||).
    // Returns true if no injection patterns are detected, false otherwise.
    bool validateJSON(const std::string& input) const;

    // Check an XML string for XXE constructs: DOCTYPE declarations, ENTITY
    // definitions, and SYSTEM/PUBLIC keyword references.
    // Returns true if no XXE patterns are found, false otherwise.
    bool validateXML(const std::string& input) const;

    // Validate an LDAP filter value, rejecting unescaped LDAP metacharacters
    // (* ( ) \ NUL) that could enable filter injection.
    // Returns true if the value is safe for use in an LDAP filter, false otherwise.
    bool validateLDAPFilter(const std::string& input) const;

    // Validate an email address, rejecting CRLF injection sequences (\r, \n,
    // %0a, %0d) that could enable header injection.
    // Returns true if the address is safe, false otherwise.
    bool validateEmail(const std::string& email) const;

    // Validate a URL against a whitelist of allowed schemes.
    // Rejects protocol-relative URLs (//), user-info in the authority (@),
    // non-whitelisted schemes, and query parameters that embed redirect URLs.
    // Returns true if the URL is safe, false otherwise.
    bool validateURL(const std::string& url,
                     const std::vector<std::string>& allowed_schemes) const;

    // Check that input does not exceed max_len bytes.
    // Returns true if input.size() <= max_len, false otherwise.
    bool validateStringLength(const std::string& input, size_t max_len) const;

    // Check that value is within [min_val, max_val] (inclusive).
    // Returns true if in range, false otherwise.
    bool validateIntegerRange(int64_t value, int64_t min_val, int64_t max_val) const;

    // Sanitize a log message by removing dangerous printf-style format specifiers
    // (%n, %N) that could cause memory corruption if passed to a format function.
    std::string sanitizeLogMessage(const std::string& input) const;

    // Normalize a UTF-8 string by converting full-width Unicode characters
    // (U+FF01..U+FF5E) to their ASCII equivalents, exposing hidden injection
    // patterns such as full-width angle brackets (U+FF1C/U+FF1E -> < >).
    std::string normalizeUnicode(const std::string& input) const;

    // Validate an HTTP header value, rejecting CRLF injection sequences
    // (\r, \n, %0d, %0a) and null bytes.
    // Returns true if the value is safe, false otherwise.
    bool validateHeaderValue(const std::string& value) const;

private:
    std::string schema_dir_ = {};

    // Helper to load a stub schema from schema_dir_/name.json
    std::optional<nlohmann::json> loadSchema(const std::string& schema_name) const;
};

} // namespace utils
} // namespace themis
