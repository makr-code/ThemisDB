/**
 * @file encrypted_storage_api_handler.cpp
 * @brief Implementation of the User Encrypted Storage HTTP API handler.
 *
 * Production consumer route for the `user_storage_encrypted` module.
 * Routes under `/user/storage/encrypted/` dispatch to
 * `MultiLevelEncryptedStorage`.
 *
 * This handler is gated by the `THEMIS_PLUGIN_USER_STORAGE_ENCRYPTED` CMake flag.
 *
 * @version 1.0.0
 * @note Maturity: 🟡 INTEGRATION-READY
 * @note Compile gate: `THEMIS_PLUGIN_USER_STORAGE_ENCRYPTED`
 * @see include/server/encrypted_storage_api_handler.h
 */

#include "server/encrypted_storage_api_handler.h"
#include "storage/rocksdb_wrapper.h"
#include "server/auth_middleware.h"
#include "utils/logger.h"

#include <nlohmann/json.hpp>
#include <sstream>
#include <chrono>

namespace themis::server {

namespace {

using themis::plugins::user_storage::SecurityLevel;
using themis::plugins::user_storage::User;

SecurityLevel scopeToLevel(const std::string& scope) {
    if (scope == "offen" || scope == "public") {
        return SecurityLevel::OFFEN;
    }
    if (scope == "vs-nfd" || scope == "restricted") {
        return SecurityLevel::VS_NFD;
    }
    if (scope == "geheim" || scope == "secret") {
        return SecurityLevel::GEHEIM;
    }
    if (scope == "streng-geheim" || scope == "top-secret") {
        return SecurityLevel::STRENG_GEHEIM;
    }
    return SecurityLevel::OFFEN;
}

} // namespace

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

EncryptedStorageApiHandler::EncryptedStorageApiHandler(
    std::shared_ptr<RocksDBWrapper>                              storage,
    std::shared_ptr<themis::AuthMiddleware>                      auth,
    std::shared_ptr<themis::plugins::user_storage::MultiLevelEncryptedStorage> enc_storage)
    : storage_(std::move(storage))
    , auth_(std::move(auth))
    , enc_storage_(std::move(enc_storage))
{
}

EncryptedStorageApiHandler::~EncryptedStorageApiHandler() = default;

// ---------------------------------------------------------------------------
// Dispatch
// ---------------------------------------------------------------------------

http::response<http::string_body> EncryptedStorageApiHandler::handle(
    const http::request<http::string_body>& req,
    const std::string& target)
{
    std::string path = target;
    const auto qpos  = path.find('?');
    if (qpos != std::string::npos) {
        path = path.substr(0, qpos);
    }
    // Strip optional /api prefix.
    if (path.rfind("/api/user/", 0) == 0) {
        path = path.substr(4);
    }

    const auto method = req.method();
    constexpr std::string_view BASE = "/user/storage/encrypted/";
    constexpr auto             BASE_LEN = BASE.size();

    if (method == http::verb::post && path == "/user/storage/encrypted/store") {
        return handleStore(req);
    }
    if (method == http::verb::get
        && path.rfind("/user/storage/encrypted/retrieve/", 0) == 0
        && path.size() > BASE_LEN + 9) {
        const std::string key = path.substr(BASE_LEN + 9);  // after "retrieve/"
        return handleRetrieve(req, key);
    }
    if (method == http::verb::get && path == "/user/storage/encrypted/list") {
        return handleList(req);
    }
    if (method == http::verb::delete_
        && path.rfind(BASE.data(), 0) == 0
        && path.size() > BASE_LEN) {
        const std::string key = path.substr(BASE_LEN);
        return handleDelete(req, key);
    }
    if (method == http::verb::post && path == "/user/storage/encrypted/rotate") {
        return handleRotate(req);
    }

    http::response<http::string_body> resp{http::status::not_found, req.version()};
    resp.set(http::field::content_type, "application/json");
    resp.body() = R"({"error":"Encrypted Storage API: route not found"})";
    resp.prepare_payload();
    return resp;
}

// ---------------------------------------------------------------------------
// Route handlers
// ---------------------------------------------------------------------------

http::response<http::string_body> EncryptedStorageApiHandler::handleStore(
    const http::request<http::string_body>& req)
{
    try {
        auto body = nlohmann::json::parse(req.body());

        const std::string key   = body.value("key", "");
        const std::string value = body.value("value", "");
        const std::string scope = body.value("scope", "user");

        if (key.empty() || value.empty()) {
            http::response<http::string_body> resp{http::status::bad_request, req.version()};
            resp.set(http::field::content_type, "application/json");
            resp.body() = R"({"error":"key and value are required"})";
            resp.prepare_payload();
            return resp;
        }

        const auto level = scopeToLevel(scope);
        const auto now_ms = static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());

        User user;
        user.user_id = key;
        user.username = key;
        user.email = value;
        user.full_name = key;
        user.classification = level;
        user.created_at_ms = now_ms;
        user.updated_at_ms = now_ms;

        const auto write_result = enc_storage_->createUser(user, level);
        if (write_result.isError()) {
            http::response<http::string_body> resp{http::status::internal_server_error, req.version()};
            resp.set(http::field::content_type, "application/json");
            resp.body() = nlohmann::json{{"error", write_result.error()}}.dump();
            resp.prepare_payload();
            return resp;
        }

        http::response<http::string_body> resp{http::status::created, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = nlohmann::json{{"key", key}, {"scope", scope}, {"stored", true}}.dump();
        resp.prepare_payload();
        return resp;

    } catch (const nlohmann::json::exception& ex) {
        THEMIS_WARN("EncryptedStorageApiHandler::handleStore JSON parse error: {}", ex.what());
        http::response<http::string_body> resp{http::status::bad_request, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = R"({"error":"Invalid JSON body"})";
        resp.prepare_payload();
        return resp;
    } catch (const std::exception& ex) {
        THEMIS_ERROR("EncryptedStorageApiHandler::handleStore error: {}", ex.what());
        http::response<http::string_body> resp{http::status::internal_server_error, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = std::string(R"({"error":)") + nlohmann::json(ex.what()).dump() + "}";
        resp.prepare_payload();
        return resp;
    }
}

http::response<http::string_body> EncryptedStorageApiHandler::handleRetrieve(
    const http::request<http::string_body>& req,
    const std::string& key)
{
    try {
        const std::string scope = "offen";
        const auto level = scopeToLevel(scope);
        const auto read_result = enc_storage_->getUser(key, level);
        if (read_result.isError()) {
            http::response<http::string_body> resp{http::status::not_found, req.version()};
            resp.set(http::field::content_type, "application/json");
            resp.body() = R"({"error":"key not found"})";
            resp.prepare_payload();
            return resp;
        }
        const auto& user = read_result.value();

        http::response<http::string_body> resp{http::status::ok, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = nlohmann::json{{"key", key}, {"value", user.email}}.dump();
        resp.prepare_payload();
        return resp;
    } catch (const std::exception& ex) {
        THEMIS_ERROR("EncryptedStorageApiHandler::handleRetrieve error: {}", ex.what());
        http::response<http::string_body> resp{http::status::internal_server_error, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = std::string(R"({"error":)") + nlohmann::json(ex.what()).dump() + "}";
        resp.prepare_payload();
        return resp;
    }
}

http::response<http::string_body> EncryptedStorageApiHandler::handleDelete(
    const http::request<http::string_body>& req,
    const std::string& key)
{
    try {
        const auto level = scopeToLevel("offen");
        const auto delete_result = enc_storage_->deleteUser(key, level);
        const bool removed = !delete_result.isError();
        http::response<http::string_body> resp{
            removed ? http::status::no_content : http::status::not_found,
            req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = removed ? "" : R"({"error":"key not found"})";
        resp.prepare_payload();
        return resp;
    } catch (const std::exception& ex) {
        http::response<http::string_body> resp{http::status::internal_server_error, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = std::string(R"({"error":)") + nlohmann::json(ex.what()).dump() + "}";
        resp.prepare_payload();
        return resp;
    }
}

http::response<http::string_body> EncryptedStorageApiHandler::handleList(
    const http::request<http::string_body>& req)
{
    try {
        const auto list_result = enc_storage_->listUsers(scopeToLevel("offen"));
        if (list_result.isError()) {
            http::response<http::string_body> resp{http::status::internal_server_error, req.version()};
            resp.set(http::field::content_type, "application/json");
            resp.body() = nlohmann::json{{"error", list_result.error()}}.dump();
            resp.prepare_payload();
            return resp;
        }

        nlohmann::json resp_body = nlohmann::json::array();
        for (const auto& user : list_result.value()) {
            resp_body.push_back(user.user_id);
        }
        http::response<http::string_body> resp{http::status::ok, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = resp_body.dump();
        resp.prepare_payload();
        return resp;
    } catch (const std::exception& ex) {
        http::response<http::string_body> resp{http::status::internal_server_error, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = std::string(R"({"error":)") + nlohmann::json(ex.what()).dump() + "}";
        resp.prepare_payload();
        return resp;
    }
}

http::response<http::string_body> EncryptedStorageApiHandler::handleRotate(
    const http::request<http::string_body>& req)
{
    try {
        auto body = nlohmann::json::parse(req.body());
        const std::string scope = body.value("scope", "user");

        const auto rotate_result = enc_storage_->rotateKey(scopeToLevel(scope));
        if (rotate_result.isError()) {
            http::response<http::string_body> resp{http::status::internal_server_error, req.version()};
            resp.set(http::field::content_type, "application/json");
            resp.body() = nlohmann::json{{"error", rotate_result.error()}}.dump();
            resp.prepare_payload();
            return resp;
        }

        THEMIS_INFO("EncryptedStorageApiHandler: key rotation triggered for scope={}", scope);

        http::response<http::string_body> resp{http::status::ok, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = nlohmann::json{{"scope", scope}, {"rotation_triggered", true}}.dump();
        resp.prepare_payload();
        return resp;

    } catch (const nlohmann::json::exception& ex) {
        http::response<http::string_body> resp{http::status::bad_request, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = R"({"error":"Invalid JSON body"})";
        resp.prepare_payload();
        return resp;
    } catch (const std::exception& ex) {
        THEMIS_ERROR("EncryptedStorageApiHandler::handleRotate error: {}", ex.what());
        http::response<http::string_body> resp{http::status::internal_server_error, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = std::string(R"({"error":)") + nlohmann::json(ex.what()).dump() + "}";
        resp.prepare_payload();
        return resp;
    }
}

}  // namespace themis::server
