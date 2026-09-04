/**
 * @file api_key_mgmt_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/api_key_mgmt_handler.h"
#include "server/auth_middleware.h"
#include "utils/logger.h"

#include <openssl/rand.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace server {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

ApiKeyMgmtHandler::ApiKeyMgmtHandler(std::shared_ptr<AuthMiddleware> auth)
    : auth_(std::move(auth)) {
}

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

std::string ApiKeyMgmtHandler::generateToken() {
    // 32 random bytes → 64 hex chars; prefixed with "themis_"
    unsigned char buf[32];
    if (RAND_bytes(buf, static_cast<int>(sizeof(buf))) != 1) {
        throw std::runtime_error("Failed to generate secure random bytes for API token");
    }
    std::ostringstream oss;
    oss << "themis_";
    for (auto b : buf) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    }
    return oss.str();
}

std::string ApiKeyMgmtHandler::generateKeyId() {
    // 8 random bytes → 16 hex chars; prefixed with "key_"
    unsigned char buf[8];
    if (RAND_bytes(buf, static_cast<int>(sizeof(buf))) != 1) {
        throw std::runtime_error("Failed to generate secure random bytes for key ID");
    }
    std::ostringstream oss;
    oss << "key_";
    for (auto b : buf) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    }
    return oss.str();
}

std::string ApiKeyMgmtHandler::currentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
#ifdef _WIN32
    gmtime_s(&tm_buf, &t);
#else
    gmtime_r(&t, &tm_buf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

std::string ApiKeyMgmtHandler::expiryTimestamp([[maybe_unused]] int days) {
    if (days <= 0) return {};
    auto now = std::chrono::system_clock::now() + std::chrono::hours(24 * days);
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
#ifdef _WIN32
    gmtime_s(&tm_buf, &t);
#else
    gmtime_r(&t, &tm_buf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

nlohmann::json ApiKeyMgmtHandler::recordToJson([[maybe_unused]] const ApiKeyRecord& rec) {
    nlohmann::json obj = {
        {"id",         rec.id},
        {"name",       rec.name},
        {"permissions", rec.permissions},
        {"created_at", rec.created_at}
    };
    if (!rec.expires_at.empty()) {
        obj["expires_at"] = rec.expires_at;
    }
    return obj;
}

// ---------------------------------------------------------------------------
// CRUD operations
// ---------------------------------------------------------------------------

nlohmann::json ApiKeyMgmtHandler::createKey([[maybe_unused]] const nlohmann::json& body) {
    try {
        // Validate required field
        if (!body.contains("name") || !body["name"].is_string()) {
            return {{"error", "Bad Request"}, {"message", "Missing required field: name"}, {"status_code", 400}};
        }
        std::string name = body.value("name", std::string{});
        if (name.empty()) {
            return {{"error", "Bad Request"}, {"message", "Field 'name' must not be empty"}, {"status_code", 400}};
        }

        // Parse optional fields
        std::vector<std::string> permissions;
        if (body.contains("permissions") && body["permissions"].is_array()) {
            for (const auto& p : body["permissions"]) {
                if (p.is_string()) permissions.push_back(p.get<std::string>());
            }
        }

        int expires_in_days = body.value("expires_in_days", 0);

        // Generate key material
        std::string token   = generateToken();
        std::string key_id  = generateKeyId();
        std::string created = currentTimestamp();
        std::string expires = expiryTimestamp(expires_in_days);

        // Store metadata first so the key is always tracked before it becomes active in auth
        ApiKeyRecord rec{key_id, name, token, permissions, created, expires};
        {
            std::lock_guard<std::mutex> lock(mutex_);
            keys_[key_id] = rec;
        }

        // Register with AuthMiddleware so the key is immediately active for authentication
        // If registration fails we roll back to maintain consistency
        if (auth_) {
            try {
                AuthMiddleware::TokenConfig cfg;
                cfg.token   = token;
                cfg.user_id = key_id;
                for (const auto& p : permissions) {
                    cfg.scopes.insert(p);
                }
                auth_->addToken(cfg);
            } catch (...) {
                // Rollback: remove from keys_ so we don't track an unauthenticated key
                std::lock_guard<std::mutex> lock(mutex_);
                keys_.erase(key_id);
                throw;
            }
        }

        THEMIS_INFO("API Key created: id='{}' name='{}' permissions={}", key_id, name, permissions.size());

        // Return full record including the one-time secret
        nlohmann::json resp = recordToJson(rec);
        resp["secret"] = token;  // returned once only
        return resp;

    } catch (const std::exception& ex) {
        THEMIS_ERROR("ApiKeyMgmtHandler::createKey failed: {}", ex.what());
        return {{"error", "Internal Server Error"}, {"message", ex.what()}, {"status_code", 500}};
    }
}

nlohmann::json ApiKeyMgmtHandler::listKeys() {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        nlohmann::json items = nlohmann::json::array();
        for (const auto& [id, rec] : keys_) {
            items.push_back(recordToJson(rec));
        }
        THEMIS_INFO("API Key list: {} keys", items.size());
        return {{"items", items}, {"total", static_cast<int>(items.size())}};
    } catch (const std::exception& ex) {
        THEMIS_ERROR("ApiKeyMgmtHandler::listKeys failed: {}", ex.what());
        return {{"error", "Internal Server Error"}, {"message", ex.what()}, {"status_code", 500}};
    }
}

nlohmann::json ApiKeyMgmtHandler::getKey([[maybe_unused]] const std::string& key_id) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = keys_.find(key_id);
        if (it == keys_.end()) {
            return {{"error", "Not Found"}, {"message", "API key '" + key_id + "' not found"}, {"status_code", 404}};
        }
        return recordToJson(it->second);
    } catch (const std::exception& ex) {
        THEMIS_ERROR("ApiKeyMgmtHandler::getKey failed: {}", ex.what());
        return {{"error", "Internal Server Error"}, {"message", ex.what()}, {"status_code", 500}};
    }
}

nlohmann::json ApiKeyMgmtHandler::updateKey(const std::string& key_id, const nlohmann::json& body) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = keys_.find(key_id);
        if (it == keys_.end()) {
            return {{"error", "Not Found"}, {"message", "API key '" + key_id + "' not found"}, {"status_code", 404}};
        }

        ApiKeyRecord& rec = it->second;

        // Apply updates
        if (body.contains("name") && body["name"].is_string()) {
            std::string new_name = body["name"].get<std::string>();
            if (!new_name.empty()) rec.name = new_name;
        }
        if (body.contains("permissions") && body["permissions"].is_array()) {
            rec.permissions.clear();
            for (const auto& p : body["permissions"]) {
                if (p.is_string()) rec.permissions.push_back(p.get<std::string>());
            }
            // Re-register token with updated permissions
            if (auth_) {
                auth_->removeToken(rec.token);
                AuthMiddleware::TokenConfig cfg;
                cfg.token   = rec.token;
                cfg.user_id = rec.id;
                for (const auto& p : rec.permissions) {
                    cfg.scopes.insert(p);
                }
                auth_->addToken(cfg);
            }
        }

        THEMIS_INFO("API Key updated: id='{}'", key_id);
        return recordToJson(rec);

    } catch (const std::exception& ex) {
        THEMIS_ERROR("ApiKeyMgmtHandler::updateKey failed: {}", ex.what());
        return {{"error", "Internal Server Error"}, {"message", ex.what()}, {"status_code", 500}};
    }
}

nlohmann::json ApiKeyMgmtHandler::deleteKey([[maybe_unused]] const std::string& key_id) {
    try {
        std::string token_to_remove;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = keys_.find(key_id);
            if (it == keys_.end()) {
                return {{"error", "Not Found"}, {"message", "API key '" + key_id + "' not found"}, {"status_code", 404}};
            }
            token_to_remove = it->second.token;
            keys_.erase(it);
        }

        // Deregister from AuthMiddleware
        if (auth_ && !token_to_remove.empty()) {
            auth_->removeToken(token_to_remove);
        }

        THEMIS_INFO("API Key revoked: id='{}'", key_id);
        return {{"success", true}, {"id", key_id}};

    } catch (const std::exception& ex) {
        THEMIS_ERROR("ApiKeyMgmtHandler::deleteKey failed: {}", ex.what());
        return {{"error", "Internal Server Error"}, {"message", ex.what()}, {"status_code", 500}};
    }
}

}} // namespace themis::server


