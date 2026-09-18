/**
 * @file input_validator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
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

    /**
     * @brief TBD: Describe InputValidator.
     * @param[in] schema_dir Input parameter.
     * @return Return value.
     */
    explicit InputValidator(std::string schema_dir);

    /**
     * @brief JSON schema validation against a JSON Schema Draft-7 file.
     * @param[in] payload Input parameter.
     * @param[in] schema_name Input parameter.
     * @return Return value.
     * @details Loads the schema from <schema_dir>/<schema_name>.json and validates `payload` against it. Supported keywords: Top-level: type ("object"), required, properties, additionalProperties Per property: type (string/object/number/integer/boolean/array/null), enum, minLength, maxLength, pattern, minimum, maximum, exclusiveMinimum, exclusiveMaximum Returns std::nullopt if valid (or if no schema file is found), otherwise an error message.
     */
    std::optional<std::string> validateJsonSchema(
        const nlohmann::json& payload,
        const std::string& schema_name
    ) const;

    /**
     * @brief Backward-compatible alias for older call sites/tests.
     * @param[in] payload Input parameter.
     * @param[in] schema_name Input parameter.
     * @return Return value.
     */
    std::optional<std::string> validateJsonStub(
        const nlohmann::json& payload,
        const std::string& schema_name
    ) const;

    /**
     * @brief JSON schema validation against an in-memory JSON Schema Draft-7 object.
     * @param[in] payload Input parameter.
     * @param[in] schema Input parameter.
     * @return Return value.
     * @details Same keyword support as validateJsonStub. Returns std::nullopt if valid, otherwise an error message.
     */
    static std::optional<std::string> validateJson(
        const nlohmann::json& payload,
        const nlohmann::json& schema
    );

    /**
     * @brief Validate AQL request payload (expects keys like {"query": ".
     * @param[in] payload Input parameter.
     * @return Return value.
     * @details ..", "bindVars": {...}}) Performs minimal checks: required fields, max length, disallowed characters/patterns
     */
    std::optional<std::string> validateAqlRequest(const nlohmann::json& payload) const;

    /**
     * @brief Validate path segment (e.
     * @param[in] segment Input parameter.
     * @return True on success.
     * @details g., entity key); rejects traversal and separators
     */
    bool validatePathSegment(const std::string& segment) const;

    // Sanitize strings for logs (strip control chars and truncate)
    std::string sanitizeForLogs(const std::string& input, size_t max_len = 512) const;

    // Configure/query schema directory
    const std::string& schemaDir() const { return schema_dir_; }

    // -------------------------------------------------------------------------
    // Security-focused validation methods
    // -------------------------------------------------------------------------

    /**
     * @brief Validate an AQL query string for injection patterns (boolean injection, comment markers, dangerous operations such as REMOVE/DROP/UNION).
     * @param[in] query Input parameter.
     * @return True on success.
     * @details Returns true if the query appears safe, false if suspicious patterns are found.
     */
    bool validateAQLQuery(const std::string& query) const;

    /**
     * @brief Validate a file path, rejecting directory traversal sequences, URL-encoded traversal, dangerous protocols (file://), and sensitive OS paths (/proc/).
     * @param[in] path Input parameter.
     * @return True on success.
     * @details Returns true if the path appears safe, false otherwise.
     */
    bool validateFilePath(const std::string& path) const;

    /**
     * @brief Sanitize a string for safe embedding in HTML: - strips javascript:/vbscript: protocol prefixes (case-insensitive) - removes on* event-handler attributes (e.
     * @param[in] input Input parameter.
     * @return Return value.
     * @details g. onerror=, onload=) - HTML-encodes remaining special characters (& < > " ' /)
     */
    std::string sanitizeForHTML(const std::string& input) const;

    /**
     * @brief Validate a filename, rejecting shell metacharacters that enable command injection (; | & ` $ ( ) { } [ ] < > !
     * @param[in] filename Input parameter.
     * @return True on success.
     * @details ? * " \). Returns true if the filename is safe, false otherwise.
     */
    bool validateFilename(const std::string& filename) const;

    /**
     * @brief Check a JSON string value for NoSQL/MongoDB operator injections (e.
     * @param[in] input Input parameter.
     * @return True on success.
     * @details g. $gt, $ne, $regex) and JavaScript injection patterns (return true, ||). Returns true if no injection patterns are detected, false otherwise.
     */
    bool validateJSON(const std::string& input) const;

    /**
     * @brief Check an XML string for XXE constructs: DOCTYPE declarations, ENTITY definitions, and SYSTEM/PUBLIC keyword references.
     * @param[in] input Input parameter.
     * @return True on success.
     * @details Returns true if no XXE patterns are found, false otherwise.
     */
    bool validateXML(const std::string& input) const;

    /**
     * @brief Validate an LDAP filter value, rejecting unescaped LDAP metacharacters (* ( ) \ NUL) that could enable filter injection.
     * @param[in] input Input parameter.
     * @return True on success.
     * @details Returns true if the value is safe for use in an LDAP filter, false otherwise.
     */
    bool validateLDAPFilter(const std::string& input) const;

    /**
     * @brief Validate an email address, rejecting CRLF injection sequences (\r, \n, %0a, %0d) that could enable header injection.
     * @param[in] email Input parameter.
     * @return True on success.
     * @details Returns true if the address is safe, false otherwise.
     */
    bool validateEmail(const std::string& email) const;

    /**
     * @brief Validate a URL against a whitelist of allowed schemes.
     * @param[in] url Input parameter.
     * @param[in] allowed_schemes Input parameter.
     * @return True on success.
     * @details Rejects protocol-relative URLs (//), user-info in the authority (@), non-whitelisted schemes, and query parameters that embed redirect URLs. Returns true if the URL is safe, false otherwise.
     */
    bool validateURL(const std::string& url,
                     const std::vector<std::string>& allowed_schemes) const;

    /**
     * @brief Check that input does not exceed max_len bytes.
     * @param[in] input Input parameter.
     * @param[in] max_len Input parameter.
     * @return True on success.
     * @details Returns true if input.size() <= max_len, false otherwise.
     */
    bool validateStringLength(const std::string& input, size_t max_len) const;

    /**
     * @brief Check that value is within [min_val, max_val] (inclusive).
     * @param[in] value Input parameter.
     * @param[in] min_val Input parameter.
     * @param[in] max_val Input parameter.
     * @return True on success.
     * @details Returns true if in range, false otherwise.
     */
    bool validateIntegerRange(int64_t value, int64_t min_val, int64_t max_val) const;

    /**
     * @brief Sanitize a log message by removing dangerous printf-style format specifiers (%n, %N) that could cause memory corruption if passed to a format function.
     * @param[in] input Input parameter.
     * @return Return value.
     */
    std::string sanitizeLogMessage(const std::string& input) const;

    /**
     * @brief Normalize a UTF-8 string by converting full-width Unicode characters (U+FF01.
     * @param[in] input Input parameter.
     * @return Return value.
     * @details .U+FF5E) to their ASCII equivalents, exposing hidden injection patterns such as full-width angle brackets (U+FF1C/U+FF1E -> < >).
     */
    std::string normalizeUnicode(const std::string& input) const;

    /**
     * @brief Validate an HTTP header value, rejecting CRLF injection sequences (\r, \n, %0d, %0a) and null bytes.
     * @param[in] value Input parameter.
     * @return True on success.
     * @details Returns true if the value is safe, false otherwise.
     */
    bool validateHeaderValue(const std::string& value) const;

private:
    std::string schema_dir_ = {};

    /**
     * @brief Helper to load a stub schema from schema_dir_/name.
     * @param[in] schema_name Input parameter.
     * @return Return value.
     * @details json
     */
    std::optional<nlohmann::json> loadSchema(const std::string& schema_name) const;
};

} // namespace utils
} // namespace themis
