/**
 * @file content_api_handler.h
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

namespace beast = boost::beast;
namespace http = beast::http;
#include <nlohmann/json.hpp>

namespace themis {

// Forward declarations
class RocksDBWrapper;
class SecondaryIndexManager;
class VectorIndexManager;

namespace content {
class ContentManager;
class ContentProcessor;
}

namespace server {

class ContentApiHandler {
public:
    ContentApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<content::ContentManager> content_manager,
        std::shared_ptr<content::ContentProcessor> content_processor,
        std::shared_ptr<themis::AuthMiddleware> auth,
        std::shared_ptr<SecondaryIndexManager> secondary_index,
        std::shared_ptr<VectorIndexManager> vector_index
    );

    /**
     * @brief Handle Import.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleImport(const http::request<http::string_body>& req);

    /**
     * @brief Handle Get.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGet(const http::request<http::string_body>& req);

    /**
     * @brief Handle Get Blob.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetBlob(const http::request<http::string_body>& req);

    /**
     * @brief Handle Get Chunks.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetChunks(const http::request<http::string_body>& req);

    /**
     * @brief Handle Hybrid Search.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleHybridSearch(const http::request<http::string_body>& req);

    /**
     * @brief Handle Fusion Search.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleFusionSearch(const http::request<http::string_body>& req);

    /**
     * @brief Handle Fulltext Search.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleFulltextSearch(const http::request<http::string_body>& req);

    /**
     * @brief Handle Config Get.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleConfigGet(const http::request<http::string_body>& req);

    /**
     * @brief Handle Config Put.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleConfigPut(const http::request<http::string_body>& req);

    /**
     * @brief Handle Content Filter Schema Get.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleContentFilterSchemaGet(const http::request<http::string_body>& req);

    /**
     * @brief Handle Content Filter Schema Put.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleContentFilterSchemaPut(const http::request<http::string_body>& req);

    /**
     * @brief Handle Edge Weight Config Get.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleEdgeWeightConfigGet(const http::request<http::string_body>& req);

    /**
     * @brief Handle Edge Weight Config Put.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleEdgeWeightConfigPut(const http::request<http::string_body>& req);

    /**
     * @brief Handle Encryption Schema Get.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleEncryptionSchemaGet(const http::request<http::string_body>& req);

    /**
     * @brief Handle Encryption Schema Put.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleEncryptionSchemaPut(const http::request<http::string_body>& req);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<content::ContentManager> content_manager_;
    std::shared_ptr<content::ContentProcessor> content_processor_;
    std::shared_ptr<themis::AuthMiddleware> auth_;
    std::shared_ptr<SecondaryIndexManager> secondary_index_;
    std::shared_ptr<VectorIndexManager> vector_index_;

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
};

} // namespace server
} // namespace themis
