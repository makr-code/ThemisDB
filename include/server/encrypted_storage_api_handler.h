/**
 * @file encrypted_storage_api_handler.h
 * @brief HTTP handler for the User Encrypted Storage API.
 *
 * Exposes `themis::plugins::MultiLevelEncryptedStorage` as production HTTP
 * endpoints under `/user/storage/encrypted/`. This header is the production
 * consumer route for the `user_storage_encrypted` module — previously the
 * module had only test consumers.
 *
 * ### Routes
 *  - `POST   /user/storage/encrypted/store`        — Store an encrypted value.
 *  - `GET    /user/storage/encrypted/retrieve/{key}`— Retrieve and decrypt a value.
 *  - `DELETE /user/storage/encrypted/{key}`         — Delete an encrypted entry.
 *  - `GET    /user/storage/encrypted/list`          — List stored key names (no values).
 *  - `POST   /user/storage/encrypted/rotate`        — Trigger key rotation for a scope.
 *
 * ### Plugin availability
 * The user_storage_encrypted module is built as an opt-in plugin target via
 * the CMake flag `THEMIS_PLUGIN_USER_STORAGE_ENCRYPTED`. When the flag is OFF
 * the plugin is not linked and this handler must not be instantiated.
 *
 * ### Authentication
 * All routes require a valid ****** validated by `AuthMiddleware`.
 * Key-rotation (`/rotate`) additionally requires `admin` role.
 *
 * @version 1.0.0
 * @note Maturity: 🟡 INTEGRATION-READY (wiring to HttpServer and plugin loading pending)
 * @note Compile gate: `THEMIS_PLUGIN_USER_STORAGE_ENCRYPTED`
 * @note Production consumer route for `src/user_storage_encrypted/` —
 *       see `src/user_storage_encrypted/ARCHITECTURE.md`
 */

#pragma once

#include "server/auth_middleware.h"
#include "user_storage_encrypted/multi_level_storage.hpp"

#include <memory>
#include <string>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace http  = beast::http;

namespace themis {

class RocksDBWrapper;

namespace server {

/**
 * @brief HTTP handler for user encrypted storage endpoints.
 *
 * Wraps `themis::plugins::MultiLevelEncryptedStorage` and exposes store/
 * retrieve/delete/list/rotate operations via an authenticated REST API.
 *
 * ### Thread safety
 * All public methods are thread-safe. The underlying storage implementation
 * provides its own internal synchronisation.
 */
class EncryptedStorageApiHandler {
public:
    /**
     * @brief Construct the encrypted storage handler.
     *
     * @param storage        RocksDB storage backend (for non-encrypted metadata).
     * @param auth           Authentication/authorisation middleware.
     * @param enc_storage    Shared encrypted storage plugin instance.
     */
    EncryptedStorageApiHandler(
        std::shared_ptr<RocksDBWrapper>                               storage,
        std::shared_ptr<themis::AuthMiddleware>                       auth,
        std::shared_ptr<themis::plugins::MultiLevelEncryptedStorage>  enc_storage);

    ~EncryptedStorageApiHandler();

    // Non-copyable, non-movable.
    EncryptedStorageApiHandler(const EncryptedStorageApiHandler&)            = delete;
    EncryptedStorageApiHandler& operator=(const EncryptedStorageApiHandler&) = delete;

    /**
     * @brief Dispatch a user encrypted storage request.
     *
     * @param req    Parsed HTTP request.
     * @param target URL target path.
     * @return       HTTP response.
     */
    http::response<http::string_body> handle(
        const http::request<http::string_body>& req,
        const std::string&                      target);

private:
    /// @name Route handlers
    /// @{
    http::response<http::string_body> handleStore(
        const http::request<http::string_body>& req);
    http::response<http::string_body> handleRetrieve(
        const http::request<http::string_body>& req,
        const std::string&                      key);
    http::response<http::string_body> handleDelete(
        const http::request<http::string_body>& req,
        const std::string&                      key);
    http::response<http::string_body> handleList(
        const http::request<http::string_body>& req);
    http::response<http::string_body> handleRotate(
        const http::request<http::string_body>& req);
    /// @}

    std::shared_ptr<RocksDBWrapper>                              storage_;
    std::shared_ptr<themis::AuthMiddleware>                      auth_;
    std::shared_ptr<themis::plugins::MultiLevelEncryptedStorage> enc_storage_;
};

}  // namespace server
}  // namespace themis
