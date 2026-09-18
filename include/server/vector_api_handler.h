/**
 * @file vector_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once
#include "server/auth_middleware.h"

#include <memory>
#include <string>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace themis {

// Forward declarations
class RocksDBWrapper;
class VectorIndexManager;
class FieldEncryption;
class KeyProvider;
class AuthMiddleware;

namespace server {

namespace beast = boost::beast;
namespace http = beast::http;

struct AuthContext {
    std::string user_id;
    std::string tenant_id;
    std::vector<std::string> roles;
    std::map<std::string, std::string> attributes;
};

class VectorApiHandler {
public:
    VectorApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<VectorIndexManager> vector_index,
        std::shared_ptr<::themis::AuthMiddleware> auth,
        std::shared_ptr<FieldEncryption> field_encryption = nullptr,
        std::shared_ptr<KeyProvider> key_provider = nullptr
    );

    /**
     * @brief Handle Search.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSearch(const http::request<http::string_body>& req);

    /**
     * @brief Handle Batch Insert.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleBatchInsert(const http::request<http::string_body>& req);

    /**
     * @brief Handle Delete By Filter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleDeleteByFilter(const http::request<http::string_body>& req);

    /**
     * @brief Handle Index Save.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleIndexSave(const http::request<http::string_body>& req);

    /**
     * @brief Handle Index Load.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleIndexLoad(const http::request<http::string_body>& req);

    /**
     * @brief Handle Index Config Get.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleIndexConfigGet(const http::request<http::string_body>& req);

    /**
     * @brief Handle Index Config Put.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleIndexConfigPut(const http::request<http::string_body>& req);

    /**
     * @brief Handle Index Stats.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleIndexStats(const http::request<http::string_body>& req);

    /**
     * @brief Handle Incremental Reindex.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleIncrementalReindex(const http::request<http::string_body>& req);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<VectorIndexManager> vector_index_;
    std::shared_ptr<::themis::AuthMiddleware> auth_;
    std::shared_ptr<FieldEncryption> field_encryption_;
    std::shared_ptr<KeyProvider> key_provider_;

    // Helper methods
    /**
     * @brief Make Error Response.
     * @param[in] status Input parameter.
     * @param[in] message Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeErrorResponse(
        http::status status, const std::string& message, const http::request<http::string_body>& req);
    /**
     * @brief Make Response.
     * @param[in] status Input parameter.
     * @param[in] body Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeResponse(
        http::status status, const std::string& body, const http::request<http::string_body>& req);
    
    /**
     * @brief Require Access.
     * @param[in] req Input parameter.
     * @param[in] permission Input parameter.
     * @param[in] resource Input parameter.
     * @param[in] path Input parameter.
     * @return Return value.
     */
    std::optional<http::response<http::string_body>> requireAccess(
        const http::request<http::string_body>& req,
        const std::string& permission,
        const std::string& resource,
        const std::string& path);
    
    /**
     * @brief Extract Auth Context.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    AuthContext extractAuthContext(const http::request<http::string_body>& req) const;
};

} // namespace server
} // namespace themis
