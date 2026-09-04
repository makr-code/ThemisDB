/**
 * @file geo_topology_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * GeoTopologyApiHandler — REST API for GEO_MIRROR topology config & health
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "server/geo_topology_api_handler.h"

#include "utils/input_validator.h"

#include <algorithm>
#include <sstream>
#include "utils/tracing.h"

namespace themis {
namespace server {

using json = nlohmann::json;

namespace {

constexpr size_t kMaxGeoTopologyIdentifierLength = 256;

bool isValidGeoTopologyIdentifier(const std::string& value) {
    themis::utils::InputValidator validator;
    return !value.empty() &&
           validator.validateStringLength(value, kMaxGeoTopologyIdentifierLength) &&
           validator.validatePathSegment(value) &&
           validator.validateHeaderValue(value);
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

GeoTopologyApiHandler::GeoTopologyApiHandler(
    std::shared_ptr<sharding::ShardTopology> shard_topology,
    std::shared_ptr<sharding::CollectionRedundancyManager> redundancy_manager,
    std::shared_ptr<AuthMiddleware> auth)
    : shard_topology_(std::move(shard_topology))
    , redundancy_manager_(std::move(redundancy_manager))
    , auth_(std::move(auth))
{
}

// ─────────────────────────────────────────────────────────────────────────────
// GET /api/v1/geo/topology
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body> GeoTopologyApiHandler::handleTopologyGet(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("handleTopologyGet");
    if (!shard_topology_) {
        return makeErrorResponse(http::status::service_unavailable,
                                 "Shard topology not available", req);
    }

    try {
        const auto shards = shard_topology_->getAllShards();
        json result = json::array();

        for (const auto& s : shards) {
            json entry = {
                {"shard_id",   s.shard_id},
                {"region",     s.region},
                {"zone",       s.zone},
                {"datacenter", s.datacenter},
                {"rack",       s.rack},
                {"is_healthy", s.is_healthy},
                {"raft_role",  s.raft_role},
                {"raft_term",  s.raft_term},
                {"raft_leader_id", s.raft_leader_id},
                {"raft_has_quorum", s.raft_has_quorum},
                {"primary_endpoint", s.primary_endpoint},
                {"capabilities", s.capabilities}
            };
            result.push_back(std::move(entry));
        }

        json response_body = {
            {"shards", result},
            {"total",  shards.size()}
        };
        return makeResponse(http::status::ok, response_body.dump(), req);

    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error,
                                 std::string("Error: ") + e.what(), req);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// GET /api/v1/geo/regions
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body> GeoTopologyApiHandler::handleRegionsGet(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan([[maybe_unused]] "handleRegionsGet");
    if (!shard_topology_) {
        return makeErrorResponse(http::status::service_unavailable,
                                 "Shard topology not available", req);
    }

    try {
        const auto regions = shard_topology_->getRegions();
        json result = json::array();

        for (const auto& region : regions) {
            const auto all_shards     = shard_topology_->getShardsInRegion(region);
            const auto healthy_shards = shard_topology_->getHealthyShardsInRegion(region);
            const uint32_t majority   = static_cast<uint32_t>(all_shards.size() / 2 + 1);
            const bool has_quorum     = shard_topology_->regionHasQuorum(region, majority);

            json zones_arr = json::array();
            for (const auto& s : all_shards) {
                if (!s.zone.empty() &&
                    std::find(zones_arr.begin(), zones_arr.end(), s.zone) == zones_arr.end()) {
                    zones_arr.push_back(s.zone);
                }
            }

            json entry = {
                {"region",         region},
                {"total_shards",   all_shards.size()},
                {"healthy_shards", healthy_shards.size()},
                {"has_majority_quorum", has_quorum},
                {"zones",          zones_arr}
            };
            result.push_back(std::move(entry));
        }

        json response_body = {
            {"regions",       result},
            {"total_regions", regions.size()}
        };
        return makeResponse(http::status::ok, response_body.dump(), req);

    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error,
                                 std::string("Error: ") + e.what(), req);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// GET /api/v1/geo/health
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body> GeoTopologyApiHandler::handleHealthGet(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("handleHealthGet");
    if (!shard_topology_) {
        return makeErrorResponse(http::status::service_unavailable,
                                 "Shard topology not available", req);
    }

    try {
        const auto regions      = shard_topology_->getRegions();
        const auto all_shards   = shard_topology_->getAllShards();
        const auto healthy_all  = shard_topology_->getHealthyShards();

        json failed_regions  = json::array();
        json degraded_regions = json::array();
        json healthy_regions  = json::array();

        for (const auto& region : regions) {
            const auto all    = shard_topology_->getShardsInRegion(region);
            const auto health = shard_topology_->getHealthyShardsInRegion(region);

            if (all.empty()) {
              continue;
            }

            const double ratio = static_cast<double>(health.size()) /
                                 static_cast<double>(all.size());

            if (ratio == 0.0) {
                failed_regions.push_back(region);
            } else if (ratio < 1.0) {
                degraded_regions.push_back({
                    {"region", region},
                    {"healthy_fraction", ratio},
                    {"healthy_shards", health.size()},
                    {"total_shards", all.size()}
                });
            } else {
                healthy_regions.push_back(region);
            }
        }

        // Determine overall status
        std::string overall_status;
        if (!failed_regions.empty()) {
            overall_status = "degraded";
        } else if (!degraded_regions.empty()) {
            overall_status = "partial";
        } else {
            overall_status = "healthy";
        }

        json response_body = {
            {"overall_status",   overall_status},
            {"total_shards",     all_shards.size()},
            {"healthy_shards",   healthy_all.size()},
            {"total_regions",    regions.size()},
            {"healthy_regions",  healthy_regions},
            {"degraded_regions", degraded_regions},
            {"failed_regions",   failed_regions}
        };
        return makeResponse(http::status::ok, response_body.dump(), req);

    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error,
                                 std::string("Error: ") + e.what(), req);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// POST /api/v1/geo/topology/shard
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body> GeoTopologyApiHandler::handleTopologyShardPost(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("handleTopologyShardPost");
    if (!shard_topology_) {
        return makeErrorResponse(http::status::service_unavailable,
                                 "Shard topology not available", req);
    }

    try {
        auto j = json::parse(req.body());

        if (!j.contains("shard_id") || !j["shard_id"].is_string()) {
            return makeErrorResponse(http::status::bad_request,
                                     "Missing required field: shard_id", req);
        }

        const std::string shard_id = j["shard_id"].get<std::string>();
        if (shard_id.empty()) {
            return makeErrorResponse(http::status::bad_request,
                                     "Field 'shard_id' must not be empty", req);
        }
        if (!isValidGeoTopologyIdentifier(shard_id)) {
            return makeErrorResponse(http::status::bad_request,
                                     "Invalid shard_id", req);
        }

        // Load existing shard info or create new entry
        sharding::ShardInfo info;
        auto existing = shard_topology_->getShard(shard_id);
        if (existing) {
            info = *existing;
        } else {
            info.shard_id = shard_id;
            info.is_healthy = true;
        }

        // Apply provided fields
        if (j.contains("region") && j["region"].is_string())
            info.region = j["region"].get<std::string>();
        if (j.contains("zone") && j["zone"].is_string())
            info.zone = j["zone"].get<std::string>();
        if (j.contains("datacenter") && j["datacenter"].is_string())
            info.datacenter = j["datacenter"].get<std::string>();
        if (j.contains("rack") && j["rack"].is_string())
            info.rack = j["rack"].get<std::string>();
        if (j.contains("primary_endpoint") && j["primary_endpoint"].is_string())
            info.primary_endpoint = j["primary_endpoint"].get<std::string>();
        if (j.contains("is_healthy") && j["is_healthy"].is_boolean())
            info.is_healthy = j["is_healthy"].get<bool>();

        shard_topology_->addShard(info);

        json response_body = {
            {"ok",       true},
            {"shard_id", shard_id},
            {"region",   info.region},
            {"zone",     info.zone},
            {"action",   existing ? "updated" : "created"}
        };
        return makeResponse(http::status::ok, response_body.dump(), req);

    } catch (const json::parse_error& e) {
        return makeErrorResponse(http::status::bad_request,
                                 std::string("JSON parse error: ") + e.what(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error,
                                 std::string("Error: ") + e.what(), req);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// DELETE /api/v1/geo/topology/shard/{shard_id}
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body> GeoTopologyApiHandler::handleTopologyShardDelete(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("handleTopologyShardDelete");
    if (!shard_topology_) {
        return makeErrorResponse(http::status::service_unavailable,
                                 "Shard topology not available", req);
    }
    auto& shard_topology = *shard_topology_;

    const std::string target   = std::string(req.target());
    const std::string shard_id = extractTrailingSegment(target,
                                     "/api/v1/geo/topology/shard/");
    if (shard_id.empty()) {
        return makeErrorResponse(http::status::bad_request,
                                 "Missing shard_id in path", req);
    }
    if (!isValidGeoTopologyIdentifier(shard_id)) {
        return makeErrorResponse(http::status::bad_request,
                                 "Invalid shard_id in path", req);
    }

    const auto existing = shard_topology.getShard(shard_id);
    if (!existing) {
        return makeErrorResponse(http::status::not_found,
                                 "Shard not found: " + shard_id, req);
    }

    shard_topology.removeShard(shard_id);

    json response_body = {
        {"ok",       true},
        {"shard_id", shard_id},
        {"removed",  true}
    };
    return makeResponse(http::status::ok, response_body.dump(), req);
}

// ─────────────────────────────────────────────────────────────────────────────
// GET /api/v1/geo/config/{collection}
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body> GeoTopologyApiHandler::handleConfigGet(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("handleConfigGet");
    if (!redundancy_manager_) {
        return makeErrorResponse(http::status::service_unavailable,
                                 "Redundancy manager not available", req);
    }
    auto& redundancy_manager = *redundancy_manager_;

    const std::string target = std::string(req.target());
    const std::string collection = extractTrailingSegment(target, "/api/v1/geo/config/");
    if (collection.empty()) {
        return makeErrorResponse(http::status::bad_request,
                                 "Missing collection name in path", req);
    }
    if (!isValidGeoTopologyIdentifier(collection)) {
        return makeErrorResponse(http::status::bad_request,
                                 "Invalid collection name in path", req);
    }

    try {
        const auto config = redundancy_manager.getConfig(collection);
        const auto& geo   = config.geo_replication;

        // Replication mode string
        std::string repl_mode;
        switch (geo.replication_mode) {
            case sharding::GeoReplicationConfig::ReplicationMode::SYNC:
                repl_mode = "sync"; break;
            case sharding::GeoReplicationConfig::ReplicationMode::SEMI_SYNC:
                repl_mode = "semi_sync"; break;
            default:
                repl_mode = "async"; break;
        }

        // Redundancy mode string
        std::string mode_str;
        switch (config.mode) {
            case sharding::RedundancyMode::GEO_MIRROR:    mode_str = "geo_mirror";    break;
            case sharding::RedundancyMode::MIRROR:        mode_str = "mirror";        break;
            case sharding::RedundancyMode::STRIPE:        mode_str = "stripe";        break;
            case sharding::RedundancyMode::STRIPE_MIRROR: mode_str = "stripe_mirror"; break;
            case sharding::RedundancyMode::PARITY:        mode_str = "parity";        break;
            case sharding::RedundancyMode::RAID6:         mode_str = "raid6";         break;
            default:                                      mode_str = "none";          break;
        }

        json write_quorums = json::object();
        for (const auto& [r, q] : geo.region_write_quorums)
            write_quorums[r] = q;

        json read_quorums = json::object();
        for (const auto& [r, q] : geo.region_read_quorums)
            read_quorums[r] = q;

        json response_body = {
            {"collection",             collection},
            {"mode",                   mode_str},
            {"replication_factor",     config.replication_factor},
            {"replication_mode",       repl_mode},
            {"local_region",           geo.local_region},
            {"max_staleness_ms",       geo.max_staleness_ms},
            {"prefer_local_reads",     geo.prefer_local_reads},
            {"prefer_local_writes",    geo.prefer_local_writes},
            {"enable_geo_failover",    geo.enable_geo_failover},
            {"region_failure_threshold", geo.region_failure_threshold},
            {"max_lag_ms",             geo.max_lag_ms},
            {"region_write_quorums",   write_quorums},
            {"region_read_quorums",    read_quorums},
            {"failed_regions",         geo.failed_regions}
        };
        return makeResponse(http::status::ok, response_body.dump(), req);

    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error,
                                 std::string("Error: ") + e.what(), req);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// PUT /api/v1/geo/config/{collection}
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body> GeoTopologyApiHandler::handleConfigPut(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("handleConfigPut");
    if (!redundancy_manager_) {
        return makeErrorResponse(http::status::service_unavailable,
                                 "Redundancy manager not available", req);
    }
    auto& redundancy_manager = *redundancy_manager_;

    const std::string target     = std::string(req.target());
    const std::string collection = extractTrailingSegment(target, "/api/v1/geo/config/");
    if (collection.empty()) {
        return makeErrorResponse(http::status::bad_request,
                                 "Missing collection name in path", req);
    }
    if (!isValidGeoTopologyIdentifier(collection)) {
        return makeErrorResponse(http::status::bad_request,
                                 "Invalid collection name in path", req);
    }

    try {
        auto j = json::parse(req.body());

        // Start from the existing config so partial updates are respected
        auto config = redundancy_manager.getConfig(collection);
        config.mode = sharding::RedundancyMode::GEO_MIRROR;
        auto& geo   = config.geo_replication;

        if (j.contains("replication_factor") && j["replication_factor"].is_number_integer())
            config.replication_factor = j["replication_factor"].get<uint32_t>();

        if (j.contains("replication_mode") && j["replication_mode"].is_string()) {
            const std::string m = j["replication_mode"].get<std::string>();
            if (m == "sync")
                geo.replication_mode = sharding::GeoReplicationConfig::ReplicationMode::SYNC;
            else if (m == "semi_sync")
                geo.replication_mode = sharding::GeoReplicationConfig::ReplicationMode::SEMI_SYNC;
            else
                geo.replication_mode = sharding::GeoReplicationConfig::ReplicationMode::ASYNC;
        }

        if (j.contains("local_region") && j["local_region"].is_string())
            geo.local_region = j["local_region"].get<std::string>();

        if (j.contains("max_staleness_ms") && j["max_staleness_ms"].is_number_integer())
            geo.max_staleness_ms = j["max_staleness_ms"].get<uint32_t>();

        if (j.contains("prefer_local_reads") && j["prefer_local_reads"].is_boolean())
            geo.prefer_local_reads = j["prefer_local_reads"].get<bool>();

        if (j.contains("prefer_local_writes") && j["prefer_local_writes"].is_boolean())
            geo.prefer_local_writes = j["prefer_local_writes"].get<bool>();

        if (j.contains("enable_geo_failover") && j["enable_geo_failover"].is_boolean())
            geo.enable_geo_failover = j["enable_geo_failover"].get<bool>();

        if (j.contains("region_failure_threshold") && j["region_failure_threshold"].is_number())
            geo.region_failure_threshold = j["region_failure_threshold"].get<double>();

        if (j.contains("max_lag_ms") && j["max_lag_ms"].is_number_integer())
            geo.max_lag_ms = j["max_lag_ms"].get<uint32_t>();

        if (j.contains("region_write_quorums") && j["region_write_quorums"].is_object()) {
            geo.region_write_quorums.clear();
            for (auto& [k, v] : j["region_write_quorums"].items()) {
                if (v.is_number_integer())
                    geo.region_write_quorums[k] = v.get<uint32_t>();
            }
        }

        if (j.contains("region_read_quorums") && j["region_read_quorums"].is_object()) {
            geo.region_read_quorums.clear();
            for (auto& [k, v] : j["region_read_quorums"].items()) {
                if (v.is_number_integer())
                    geo.region_read_quorums[k] = v.get<uint32_t>();
            }
        }

        // Validate before applying
        if (!config.validate()) {
            return makeErrorResponse(http::status::bad_request,
                                     "Invalid geo configuration: check quorum values and "
                                     "region_failure_threshold", req);
        }

        redundancy_manager.setCollectionConfig(collection, config);

        json response_body = {
            {"ok",         true},
            {"collection", collection},
            {"applied",    true}
        };
        return makeResponse(http::status::ok, response_body.dump(), req);

    } catch (const json::parse_error& e) {
        return makeErrorResponse(http::status::bad_request,
                                 std::string("JSON parse error: ") + e.what(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error,
                                 std::string("Error: ") + e.what(), req);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

std::string GeoTopologyApiHandler::extractTrailingSegment(
    const std::string& path, const std::string& prefix) const
{
    // Strip query string first
    auto qpos = path.find('?');
    const std::string clean = (qpos != std::string::npos) ? path.substr(0, qpos) : path;

    if (clean.rfind(prefix, 0) != 0) {
      return "";
    }
    const std::string trailing = clean.substr(prefix.size());
    if (trailing.empty()) {
      return "";
    }
    return trailing;
}

http::response<http::string_body> GeoTopologyApiHandler::makeErrorResponse(
    http::status status,
    const std::string& message,
    const http::request<http::string_body>& req)
{
    json error_body = {
        {"error",       true},
        {"message",     message},
        {"status_code", static_cast<int>(status)}
    };
    return makeResponse(status, error_body.dump(), req);
}

http::response<http::string_body> GeoTopologyApiHandler::makeResponse(
    http::status status,
    const std::string& body,
    const http::request<http::string_body>& req)
{
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::server, "THEMIS/0.1.0");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = body;
    res.prepare_payload();
    return res;
}

} // namespace server
} // namespace themis

