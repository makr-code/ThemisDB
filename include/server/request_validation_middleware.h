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

/*
 * ThemisDB | File: request_validation_middleware.h | Version: 0.0.34
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
     * @brief Register a JSON Schema Draft-7 object for an endpoint.
     *
     * @param method   HTTP method in any case ("GET", "POST", ...) or "*" for any method.
     * @param path     Exact path or prefix (e.g. "/api/v1/entities").
     *                 A trailing "/" is NOT added automatically.
     * @param schema   nlohmann::json object representing a JSON Schema Draft-7 document.
     *                 Replaces any previously registered schema for the same key.
     */
    void registerSchema(const std::string& method, const std::string& path, nlohmann::json schema);

    /**
     * @brief Validate the request body against the schema registered for (method, path).
     *
     * @param method   HTTP method string (e.g. "POST").
     * @param path     Request target path (e.g. "/api/v1/entities/42" or just "/api/v1/entities").
     * @param body     Raw request body string.  Empty body skips validation for
     *                 schemas that do not mark any field as required.
     * @return ValidationResult::OK() when valid or when no schema is registered;
     *         ValidationResult::Error(msg) otherwise.
     */
    ValidationResult validate(const std::string& method,
                              const std::string& path,
                              const std::string& body) const;

    /**
     * @brief Overload that accepts a pre-parsed JSON body.
     */
    ValidationResult validate(const std::string& method,
                              const std::string& path,
                              const nlohmann::json& body) const;

    /**
     * @brief Return true if at least one schema is registered for the given (method, path).
     *
     * Uses the same lookup logic as validate().
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
