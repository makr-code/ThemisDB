/**
 * @file request_validation_middleware.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.34
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <nlohmann/json.hpp>

namespace themis {
namespace server {

/**
 * @brief Request body validation middleware using JSON Schema Draft-7 per endpoint.
 *
 * Allows registering a JSON Schema for each (HTTP-method, path) pair.  Incoming
 * request bodies are validated before they reach the handler.  Uses the same
 * schema-validation engine as `utils::InputValidator` (Draft-7 subset:
 * type, required, properties, additionalProperties, enum, minLength, maxLength,
 * pattern, minimum, maximum, exclusiveMinimum, exclusiveMaximum).
 *
 * ### Typical usage
 * ```cpp
 * RequestValidationMiddleware validator;
 *
 * // Register schema for POST /api/v1/entities
 * validator.registerSchema("POST", "/api/v1/entities", {
 *     {"type", "object"},
 *     {"required", {"key", "data"}},
 *     {"properties", {
 *         {"key",  {{"type", "string"}, {"minLength", 1}}},
 *         {"data", {{"type", "object"}}}
 *     }}
 * });
 *
 * // In routeRequest()
 * if (auto result = validator.validate(method_str, path, req.body()); !result.valid) {
 *     return makeErrorResponse(http::status::bad_request, result.error_message, req);
 * }
 * ```
 *
 * ### Path matching (longest-prefix, most specific method first)
 * 1. Exact (method, path)
 * 2. Longest prefix (method, path_prefix)
 * 3. Exact ("*", path)
 * 4. Longest prefix ("*", path_prefix)
 *
 * If no schema is found for a request the validation is skipped (result.valid == true).
 */
class RequestValidationMiddleware {
public:
    /**
     * @brief Result of a validation call.
     */
    struct ValidationResult {
        bool valid = true;
        std::string error_message;

        static ValidationResult OK() { return {true, ""}; }
        static ValidationResult Error(std::string msg) { return {false, std::move(msg)}; }
    };

    RequestValidationMiddleware() = default;
    ~RequestValidationMiddleware() = default;

    // Non-copyable, non-movable (contains a mutex)
    RequestValidationMiddleware(const RequestValidationMiddleware&) = delete;
    RequestValidationMiddleware& operator=(const RequestValidationMiddleware&) = delete;
    RequestValidationMiddleware(RequestValidationMiddleware&&) = delete;
    RequestValidationMiddleware& operator=(RequestValidationMiddleware&&) = delete;

    /**
     * @brief Register a JSON Schema Draft-7 for an endpoint to enable request body validation.
     * 
     * Replaces any previously registered schema for the same (method, path) key.
     * Schemas are used during request validation (validate method) to enforce
     * request contract compliance before the handler is invoked.
     * 
     * @param method HTTP method ("GET", "POST", "PUT", "DELETE", etc., case-insensitive)
     *               or "*" to match any HTTP method
     * @param path Exact path or prefix (e.g. "/api/v1/entities" or "/api/v1/entities/").
     *             No automatic trailing-slash normalization; register both if needed.
     *             Longest-prefix matching applies; "/api/v1" will match "/api/v1/anything"
     * @param schema nlohmann::json object representing the JSON Schema Draft-7 document.
     *               Must be a JSON object (not null, array, etc.).
     *               Replaces any previously registered schema for the same key.
     * 
     * @note Thread-safe; safe to register schemas while validate() is running on other threads
     * @note Schema registration does NOT validate the schema document itself (best-effort parsing)
     * @note Large schemas may impact lookup performance
     * 
     * @see validate() to validate request bodies against registered schemas
     * @see hasSchema() to check if a schema exists
     * @see removeSchema() to unregister a schema
     */
    void registerSchema(const std::string& method, const std::string& path, nlohmann::json schema);

    /**
     * @brief Validate a raw request body string against the schema registered for an endpoint.
     * 
     * Performs JSON Schema Draft-7 validation before the request reaches the handler.
     * In HttpServer routing, body validation is executed during request intake before
     * final handler dispatch, but not globally after all auth checks for every path.
     * 
     * ### Path Matching Priority
     * Routes are matched in this order (first match wins):
     * 1. Exact (method, path)
     * 2. Longest prefix (method, path_prefix)
     * 3. Exact ("*", path) - matches any method
     * 4. Longest prefix ("*", path_prefix)
     * 
     * If no schema is registered for a request path, validation is skipped (returns OK).
     * 
     * ### Empty Body Handling
     * An empty body string is converted to an empty JSON object (`{}`) and then validated.
     * Whitespace-only bodies are parsed as-is and may fail with a JSON parse error.
     * 
     * ### Validation Features (JSON Schema Draft-7 subset)
     * - type: "object", "array", "string", "number", "boolean", "null"
     * - required: list of mandatory field names
     * - properties: field-level schema definitions
     * - additionalProperties: allow/disallow extra fields
     * - enum: list of allowed values
     * - minLength, maxLength: string length constraints
     * - pattern: regex pattern matching
     * - minimum, maximum: numeric range constraints
     * - exclusiveMinimum, exclusiveMaximum: exclusive range bounds
     * 
     * @param method HTTP method string (case-insensitive, e.g. "POST", "post", "Post")
     * @param path Request target path (e.g. "/api/v1/entities/42" or "/api/v1/entities").
     *             No automatic trailing-slash normalization; "/entities" and "/entities/" are different.
     * @param body Raw request body string. Empty string is converted to `{}` before validation.
     * 
     * @return ValidationResult::OK() when:
     *         - Body is valid against registered schema
     *         - No schema is registered for this (method, path)
     *         ValidationResult::Error(msg) when parsing fails or schema validation fails
     * 
     * @note Thread-safe; concurrent validate() calls allowed
     * @note Validation is best-effort; complex schemas may have limitations
     * @note No sensitive data leakage in error messages (uses JSON pointer paths)
     * 
     * @see registerSchema() to register a schema for an endpoint
     * @see hasSchema() to check if a schema exists without validating
     * @see validate(method, path, json) for pre-parsed JSON bodies
     */
    ValidationResult validate(const std::string& method,
                              const std::string& path,
                              const std::string& body) const;

    /**
     * @brief Validate a pre-parsed JSON body against the schema registered for an endpoint.
     * 
     * Identical to validate(method, path, string) except the body is already parsed as JSON.
     * Avoids re-parsing if the body has already been converted to JSON for other purposes.
     * 
     * @param method HTTP method string (case-insensitive)
     * @param path Request target path
     * @param body Pre-parsed JSON object (any JSON value: object, array, string, number, boolean, null)
     * 
     * @return ValidationResult::OK() or ValidationResult::Error(msg)
     * 
     * @note Thread-safe; concurrent calls allowed
     * @note See validate(method, path, string) for detailed path-matching and validation logic
     * 
     * @see validate(method, path, string) for raw body validation
     */
    ValidationResult validate(const std::string& method,
                              const std::string& path,
                              const nlohmann::json& body) const;

    /**
     * @brief Check if a schema is registered for an endpoint (method, path pair).
     * 
     * Uses the same path-matching logic as validate():
     * 1. Exact (method, path)
     * 2. Longest prefix (method, path_prefix)
     * 3. Exact ("*", path)
     * 4. Longest prefix ("*", path_prefix)
     * 
     * Useful for conditional validation or logging.
     * 
     * @param method HTTP method (case-insensitive) or "*" for any method
     * @param path Request target path
     * 
     * @return true if a schema is registered for the (method, path) pair; false otherwise
     * 
     * @note Thread-safe; returns snapshot
     * @note Does NOT perform validation; only checks schema existence
     * 
     * @see validate() to validate against the schema
     */
    bool hasSchema(const std::string& method, const std::string& path) const;

    /**
     * @brief Remove the schema registered for an exact (method, path) key.
     *
     * @return true if a schema was removed, false if no schema was registered.
     */
    bool removeSchema(const std::string& method, const std::string& path);

    /**
     * @brief Remove all registered schemas.
     */
    void clearSchemas();

    /**
     * @brief Return the number of registered schemas.
     */
    size_t schemaCount() const;

    /**
     * @brief Cumulative validation metrics.
     */
    struct Metrics {
        std::atomic<uint64_t> validation_pass_total{0};   ///< body validated and accepted
        std::atomic<uint64_t> validation_fail_total{0};   ///< body rejected by schema
        std::atomic<uint64_t> validation_skip_total{0};   ///< no schema registered
        std::atomic<uint64_t> parse_error_total{0};       ///< body could not be parsed as JSON
    };

    const Metrics& getMetrics() const { return metrics_; }

private:
    struct EndpointKey {
        std::string method; // always upper-case or "*"
        std::string path;
        bool operator==(const EndpointKey& o) const noexcept {
            return method == o.method && path == o.path;
        }
    };

    struct EndpointKeyHash {
        size_t operator()(const EndpointKey& k) const noexcept {
            // Simple but adequate combination
            size_t h1 = std::hash<std::string>{}(k.method);
            size_t h2 = std::hash<std::string>{}(k.path);
            return h1 ^ (h2 * 2654435761ULL);
        }
    };

    mutable std::mutex mutex_;
    std::unordered_map<EndpointKey, nlohmann::json, EndpointKeyHash> schemas_;
    mutable Metrics metrics_;

    /// Convert HTTP method string to upper-case.
    static std::string normalizeMethod(const std::string& method);

    /**
     * @brief Find the best-matching schema for (method, path).
     *
     * Precedence (first match wins):
     *   1. Exact match  (method, path)
     *   2. Longest-prefix match (method, prefix)
     *   3. Exact match  ("*", path)
     *   4. Longest-prefix match ("*", prefix)
     *
     * Caller must hold mutex_.
     */
    const nlohmann::json* findSchemaLocked(const std::string& method,
                                           const std::string& path) const;

    /**
     * @brief Core JSON Schema Draft-7 subset validation.
     *
     * Delegates to utils::InputValidator::validateJson which is the shared
     * schema-validation engine.  Declared here as a static helper so
     * request_validation_middleware.cpp keeps the dependency encapsulated.
     */
    static ValidationResult applySchema(const nlohmann::json& body,
                                        const nlohmann::json& schema);
};

} // namespace server
} // namespace themis
