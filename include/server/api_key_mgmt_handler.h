/**
 * @file api_key_mgmt_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class ApiKeyMgmtHandler {
public:
    struct ApiKeyRecord {
        std::string id;                        ///< Unique key identifier (key_<hex>)
        std::string name;                      ///< Human-readable label
        std::string token;                     ///< The actual bearer token (hidden in responses)
        std::vector<std::string> permissions;  ///< Scopes/permissions granted
        std::string created_at;                ///< ISO-8601 creation timestamp
        std::string expires_at;                ///< ISO-8601 expiry; empty = never expires
    };

    /**
     * @brief Api Key Mgmt Handler.
     * @param[in] auth Input parameter.
     * @return Return value.
     */
    explicit ApiKeyMgmtHandler(std::shared_ptr<AuthMiddleware> auth);

    /**
     * @brief Create Key.
     * @param[in] body Input parameter.
     * @return Return value.
     */
    nlohmann::json createKey(const nlohmann::json& body);

    /**
     * @brief List Keys.
     * @return Return value.
     */
    nlohmann::json listKeys();

    /**
     * @brief Get Key.
     * @param[in] key_id Identifier of the key.
     * @return Return value.
     */
    nlohmann::json getKey(const std::string& key_id);

    /**
     * @brief Update Key.
     * @param[in] key_id Identifier of the key.
     * @param[in] body Input parameter.
     * @return Return value.
     */
    nlohmann::json updateKey(const std::string& key_id, const nlohmann::json& body);

    /**
     * @brief Delete Key.
     * @param[in] key_id Identifier of the key.
     * @return Return value.
     */
    nlohmann::json deleteKey(const std::string& key_id);

private:
    /**
     * @brief Generate Token.
     * @return Return value.
     */
    static std::string generateToken();

    /**
     * @brief Generate Key Id.
     * @return Return value.
     */
    static std::string generateKeyId();

    /**
     * @brief Current Timestamp.
     * @return Return value.
     */
    static std::string currentTimestamp();

    /**
     * @brief Expiry Timestamp.
     * @param[in] days Input parameter.
     * @return Return value.
     */
    static std::string expiryTimestamp(int days);

    /**
     * @brief Record To Json.
     * @param[in] rec Input parameter.
     * @return Return value.
     */
    static nlohmann::json recordToJson(const ApiKeyRecord& rec);

    std::shared_ptr<AuthMiddleware> auth_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, ApiKeyRecord> keys_; ///< key_id -> record
};

}} // namespace themis::server
