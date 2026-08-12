/**
 * @file api_key_mgmt_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <nlohmann/json.hpp>

namespace themis {

class AuthMiddleware;

namespace server {

/**
 * @brief API Key Management Handler
 *
 * Provides REST endpoints for managing API authentication keys:
 * - POST   /api/keys           - Create a new API key
 * - GET    /api/keys           - List all API keys (metadata only, no secrets)
 * - GET    /api/keys/{id}      - Get a specific API key by ID
 * - PUT    /api/keys/{id}      - Update an existing API key
 * - DELETE /api/keys/{id}      - Revoke/delete an API key
 *
 * All endpoints require the "admin:all" scope.
 * The secret token is returned only at creation time.
 */
class ApiKeyMgmtHandler {
public:
    /**
     * @brief Metadata record for a managed API key.
     * The raw token is stored internally to allow deregistration from AuthMiddleware.
     */
    struct ApiKeyRecord {
        std::string id;                        ///< Unique key identifier (key_<hex>)
        std::string name;                      ///< Human-readable label
        std::string token;                     ///< The actual bearer token (hidden in responses)
        std::vector<std::string> permissions;  ///< Scopes/permissions granted
        std::string created_at;                ///< ISO-8601 creation timestamp
        std::string expires_at;                ///< ISO-8601 expiry; empty = never expires
    };

    /**
     * @param auth Shared pointer to the AuthMiddleware instance so that
     *             created keys are immediately active for authentication.
     */
    explicit ApiKeyMgmtHandler(std::shared_ptr<AuthMiddleware> auth);

    /**
     * @brief Create a new API key.
     * @param body JSON: { "name": "...", "permissions": [...], "expires_in_days": N }
     * @return JSON including the one-time secret, or an error object.
     */
    nlohmann::json createKey(const nlohmann::json& body);

    /**
     * @brief List all managed API keys (metadata only – no secret).
     * @return JSON: { "items": [...], "total": N }
     */
    nlohmann::json listKeys();

    /**
     * @brief Get metadata for a specific API key.
     * @param key_id Key identifier returned at creation time.
     * @return JSON metadata, or error with status_code 404.
     */
    nlohmann::json getKey(const std::string& key_id);

    /**
     * @brief Update the name or permissions of an existing API key.
     * @param key_id Key identifier.
     * @param body   JSON: { "name": "...", "permissions": [...] }
     * @return Updated metadata, or error.
     */
    nlohmann::json updateKey(const std::string& key_id, const nlohmann::json& body);

    /**
     * @brief Revoke and remove an API key.
     * @param key_id Key identifier.
     * @return JSON: { "success": true } or error.
     */
    nlohmann::json deleteKey(const std::string& key_id);

private:
    /// Generate a cryptographically-secure API token: "themis_" + 64 hex chars.
    static std::string generateToken();

    /// Generate a short, unique key identifier: "key_" + 16 hex chars.
    static std::string generateKeyId();

    /// Current UTC timestamp as ISO-8601 string.
    static std::string currentTimestamp();

    /// Compute ISO-8601 timestamp N days in the future (0 = never expires → returns "").
    static std::string expiryTimestamp(int days);

    /// Build a public-facing JSON object from a record (without the secret).
    static nlohmann::json recordToJson(const ApiKeyRecord& rec);

    std::shared_ptr<AuthMiddleware> auth_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, ApiKeyRecord> keys_; ///< key_id -> record
};

}} // namespace themis::server
