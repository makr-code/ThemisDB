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

/**
 * @brief Handler for Geo-Topology Configuration and Health Observability
 *
 * Exposes a REST API for managing and monitoring the GEO_MIRROR topology:
 *
 *   GET  /api/v1/geo/topology               - List all shards with region/zone/health
 *   GET  /api/v1/geo/regions                - Per-region health summary
 *   GET  /api/v1/geo/health                 - Overall geo health (failed regions, quorum states)
 *   POST /api/v1/geo/topology/shard         - Add/update a shard's region and zone metadata
 *   GET  /api/v1/geo/config/{collection}    - Get geo-replication config for a collection
 *   PUT  /api/v1/geo/config/{collection}    - Update geo-replication config for a collection
 *
 * All endpoints return JSON. Authentication is enforced when an AuthMiddleware
 * is configured.
 */
class GeoTopologyApiHandler {
public:
    GeoTopologyApiHandler(
        std::shared_ptr<sharding::ShardTopology> shard_topology,
        std::shared_ptr<sharding::CollectionRedundancyManager> redundancy_manager,
        std::shared_ptr<AuthMiddleware> auth
    );

     * @brief TBD: Describe handleTopologyGet.
     * @param[in] req Input parameter.
     * @return Return value.
    /** GET /api/v1/geo/topology — all shards with region/zone/health/raft_role */
    http::response<http::string_body> handleTopologyGet(
        const http::request<http::string_body>& req);

     * @brief TBD: Describe handleRegionsGet.
     * @param[in] req Input parameter.
     * @return Return value.
    /** GET /api/v1/geo/regions — per-region aggregated health */
    http::response<http::string_body> handleRegionsGet(
        const http::request<http::string_body>& req);

     * @brief TBD: Describe handleHealthGet.
     * @param[in] req Input parameter.
     * @return Return value.
    /** GET /api/v1/geo/health — overall geo-failover / quorum health */
    http::response<http::string_body> handleHealthGet(
        const http::request<http::string_body>& req);

     * @brief TBD: Describe handleTopologyShardPost.
     * @param[in] req Input parameter.
     * @return Return value.
    /** POST /api/v1/geo/topology/shard — add or update shard region/zone */
    http::response<http::string_body> handleTopologyShardPost(
        const http::request<http::string_body>& req);

     * @brief TBD: Describe handleTopologyShardDelete.
     * @param[in] req Input parameter.
     * @return Return value.
    /** DELETE /api/v1/geo/topology/shard/{shard_id} — remove a shard from the topology */
    http::response<http::string_body> handleTopologyShardDelete(
        const http::request<http::string_body>& req);

     * @brief TBD: Describe handleConfigGet.
     * @param[in] req Input parameter.
     * @return Return value.
    /** GET /api/v1/geo/config/{collection} — get GeoReplicationConfig for a collection */
    http::response<http::string_body> handleConfigGet(
        const http::request<http::string_body>& req);

     * @brief TBD: Describe handleConfigPut.
     * @param[in] req Input parameter.
     * @return Return value.
    /** PUT /api/v1/geo/config/{collection} — update GeoReplicationConfig for a collection */
    http::response<http::string_body> handleConfigPut(
        const http::request<http::string_body>& req);

private:
    std::shared_ptr<sharding::ShardTopology> shard_topology_;
    std::shared_ptr<sharding::CollectionRedundancyManager> redundancy_manager_;
    std::shared_ptr<AuthMiddleware> auth_;

    /**
     * @brief TBD: Describe makeErrorResponse.
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
     * @brief TBD: Describe makeResponse.
     * @param[in] status Input parameter.
     * @param[in] body Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeResponse(
        http::status status,
        const std::string& body,
        const http::request<http::string_body>& req);

     * @brief TBD: Describe extractTrailingSegment.
     * @param[in] path Input parameter.
     * @param[in] prefix Input parameter.
     * @return Return value.
    /** Extract the last path segment after a prefix, e.g. "/api/v1/geo/config/mycoll" → "mycoll" */
    std::string extractTrailingSegment(const std::string& path,
                                       const std::string& prefix) const;
};

} // namespace server
} // namespace themis
