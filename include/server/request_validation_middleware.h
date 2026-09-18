/**
 * @file request_validation_middleware.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.34
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class RequestValidationMiddleware {
public:
    struct ValidationResult {
        bool valid = true;
        std::string error_message;

        /**
         * @brief OK.
         * @return Return value.
         * @details Implements OK without additional internal calls.
         */
        static ValidationResult OK() { return {true, ""}; }
        /**
         * @brief Error.
         * @param[in] msg Input parameter.
         * @return Return value.
         * @details Calls: std::move().
         */
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
     * @brief Register Schema.
     * @param[in] method Input parameter.
     * @param[in] path Input parameter.
     * @param[in] schema Input parameter.
     */
    void registerSchema(const std::string& method, const std::string& path, nlohmann::json schema);

    /**
     * @brief Validate.
     * @param[in] method Input parameter.
     * @param[in] path Input parameter.
     * @param[in] body Input parameter.
     * @return Return value.
     */
    ValidationResult validate(const std::string& method,
                              const std::string& path,
                              const std::string& body) const;

    /**
     * @brief Validate.
     * @param[in] method Input parameter.
     * @param[in] path Input parameter.
     * @param[in] body Input parameter.
     * @return Return value.
     */
    ValidationResult validate(const std::string& method,
                              const std::string& path,
                              const nlohmann::json& body) const;

    /**
     * @brief Has Schema.
     * @param[in] method Input parameter.
     * @param[in] path Input parameter.
     * @return True when the operation succeeds.
     */
    bool hasSchema(const std::string& method, const std::string& path) const;

    /**
     * @brief Remove Schema.
     * @param[in] method Input parameter.
     * @param[in] path Input parameter.
     * @return True when the operation succeeds.
     */
    bool removeSchema(const std::string& method, const std::string& path);

    /**
     * @brief Clear Schemas.
     */
    void clearSchemas();

    /**
     * @brief Schema Count.
     * @return Return value.
     */
    size_t schemaCount() const;

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
        std::string path = {};
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

    /**
     * @brief Normalize Method.
     * @param[in] method Input parameter.
     * @return Return value.
     */
    static std::string normalizeMethod(const std::string& method);

    /**
     * @brief Find Schema Locked.
     * @param[in] method Input parameter.
     * @param[in] path Input parameter.
     * @return Pointer to the result.
     */
    const nlohmann::json* findSchemaLocked(const std::string& method,
                                           const std::string& path) const;

    /**
     * @brief Apply Schema.
     * @param[in] body Input parameter.
     * @param[in] schema Input parameter.
     * @return Return value.
     */
    static ValidationResult applySchema(const nlohmann::json& body,
                                        const nlohmann::json& schema);
};

} // namespace server
} // namespace themis
