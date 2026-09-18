/**
 * @file geo_topology_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "server/auth_middleware.h"
#include "sharding/shard_topology.h"
#include "sharding/redundancy_strategy.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace http = beast::http;

namespace themis {

namespace server {

class GeoTopologyApiHandler {
public:
    GeoTopologyApiHandler(
        std::shared_ptr<sharding::ShardTopology> shard_topology,
        std::shared_ptr<sharding::CollectionRedundancyManager> redundancy_manager,
        std::shared_ptr<AuthMiddleware> auth
    );

    /**
     * @brief Handle Topology Get.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleTopologyGet(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Regions Get.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleRegionsGet(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Health Get.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleHealthGet(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Topology Shard Post.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleTopologyShardPost(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Topology Shard Delete.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleTopologyShardDelete(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Config Get.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleConfigGet(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Config Put.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleConfigPut(
        const http::request<http::string_body>& req);

private:
    std::shared_ptr<sharding::ShardTopology> shard_topology_;
    std::shared_ptr<sharding::CollectionRedundancyManager> redundancy_manager_;
    std::shared_ptr<AuthMiddleware> auth_;

    /**
     * @brief Make Error Response.
     * @param[in] status Input parameter.
     * @param[in] message Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeErrorResponse(
        http::status status,
        const std::string& message,
        const http::request<http::string_body>& req);

    /**
     * @brief Make Response.
     * @param[in] status Input parameter.
     * @param[in] body Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeResponse(
        http::status status,
        const std::string& body,
        const http::request<http::string_body>& req);

    /**
     * @brief Extract Trailing Segment.
     * @param[in] path Input parameter.
     * @param[in] prefix Input parameter.
     * @return Return value.
     */
    std::string extractTrailingSegment(const std::string& path,
                                       const std::string& prefix) const;
};

} // namespace server
} // namespace themis
