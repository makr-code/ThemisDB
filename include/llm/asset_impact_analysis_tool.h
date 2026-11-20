#pragma once

#include "geo/geo_math.h"
#include "index/graph_index.h"
#include "llm/ai_orchestrator.h"
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"
#include "timeseries/tsstore.h"

#include <cmath>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace themis::llm {
namespace detail {

inline bool validAssetImpactKey(std::string_view value, std::size_t maxBytes) {
    return !value.empty() && value.size() <= maxBytes &&
           value.find(':') == std::string_view::npos &&
           value.find('\0') == std::string_view::npos;
}

inline bool readAssetLocation(RocksDBWrapper& db,
                              const std::string& graphId,
                              const GraphIndexManager::BoundedTraversalNode& node,
                              bool includeLocation,
                              std::optional<std::pair<double, double>>& location,
                              std::string& error,
                              const SkillDelegationContext* delegationContext) {
    if (delegationContext &&
        !delegationContext->consumeBudget(SkillBudgetUsage{0, 1, 0})) {
        error = "Delegated candidate budget exhausted";
        return false;
    }
    const auto key = "node:" + graphId + ":" + node.vertexId;
    const auto blob = db.get(key);
    if (!blob) {
        error = "Graph traversal references a missing node document";
        return false;
    }
    if (delegationContext &&
        !delegationContext->consumeBudget(
            SkillBudgetUsage{0, 0, blob->size()})) {
        error = "Delegated content-byte budget exhausted";
        return false;
    }
    if (blob->size() > 64 * 1024) {
        error = "Node document exceeds the 64 KiB analysis limit";
        return false;
    }
    if (!includeLocation) {
        location.reset();
        return true;
    }

    BaseEntity entity(node.vertexId);
    try {
        entity = BaseEntity::deserialize(node.vertexId, *blob);
    } catch (const std::exception&) {
        error = "Node document is invalid";
        return false;
    }

    const auto document = json::parse(entity.toJson(), nullptr, false);
    if (!document.is_object()) {
        error = "Node document must be a JSON object";
        return false;
    }
    const auto point = document.find("location");
    if (point == document.end()) {
        location.reset();
        return true;
    }
    if (!point->is_object() || !point->contains("type") ||
        point->at("type") != "Point" || !point->contains("coordinates")) {
        error = "Node location is not a GeoJSON Point";
        return false;
    }
    const auto& coordinates = point->at("coordinates");
    if (!coordinates.is_array() || coordinates.size() != 2 ||
        !coordinates[0].is_number() || !coordinates[1].is_number()) {
        error = "Invalid GeoJSON Point coordinates";
        return false;
    }
    const auto lon = coordinates[0].get<double>();
    const auto lat = coordinates[1].get<double>();
    if (!std::isfinite(lon) || !std::isfinite(lat) ||
        std::abs(lon) > 180 || std::abs(lat) > 90) {
        error = "Invalid GeoJSON Point coordinates";
        return false;
    }
    location = std::pair{lon, lat};
    return true;
}

inline json analyzeAssetImpact(RocksDBWrapper& db,
                              const GraphIndexManager& graph,
                              const TSStore& store,
                              const std::string& graphId,
                              const std::string& edgeType,
                              const std::string& metric,
                              const json& args,
                              const SkillDelegationContext* delegationContext = nullptr) {
    constexpr int64_t max_window_ms = 31LL * 24 * 60 * 60 * 1000;
    constexpr std::size_t max_vertices = 64;
    constexpr std::size_t max_edges = 256;
    constexpr std::size_t max_samples = 4096;
    const auto error = [](const char* message) { return json{{"error", message}}; };

    if (delegationContext &&
        (!delegationContext->allows("graph.read", graphId) ||
         !delegationContext->allows("timeseries.read", metric))) {
        return error("Delegated context does not permit the bound graph and metric");
    }

    if (!args.is_object() || args.size() != 5 + args.contains("geo")) {
        return error("Expected start_asset_id, max_depth, baseline_start_ms, current_start_ms, duration_ms and optional geo");
    }
    const auto assetIt = args.find("start_asset_id");
    if (assetIt == args.end() || !assetIt->is_string()) {
        return error("start_asset_id must be a string");
    }
    const auto startAsset = assetIt->get<std::string>();
    if (!validAssetImpactKey(startAsset, 1024)) {
        return error("start_asset_id is not a valid graph key");
    }

    const auto parseInteger = [&args](const char* key) -> int64_t {
        const auto it = args.find(key);
        if (it == args.end() || !it->is_number_integer() ||
            (it->is_number_unsigned() && it->get<uint64_t>() > INT64_MAX)) {
            return -1;
        }
        return it->get<int64_t>();
    };
    const auto maxDepth = parseInteger("max_depth");
    const auto baseline = parseInteger("baseline_start_ms");
    const auto current = parseInteger("current_start_ms");
    const auto duration = parseInteger("duration_ms");
    if (maxDepth < 0 || maxDepth > 3 || baseline < 0 || current < baseline ||
        duration < 1 || duration > max_window_ms || current - baseline < duration ||
        current > INT64_MAX - duration) {
        return error("Expected max_depth in [0, 3] and non-overlapping windows of 1 ms to 31 days");
    }

    const auto area = args.find("geo");
    if (area != args.end() &&
        (!area->is_object() || area->size() != 3 || !area->contains("lon") ||
         !area->contains("lat") || !area->contains("radius_m") ||
         !area->at("lon").is_number() || !area->at("lat").is_number() ||
         !area->at("radius_m").is_number() ||
         !std::isfinite(area->at("lon").get<double>()) ||
         !std::isfinite(area->at("lat").get<double>()) ||
         std::abs(area->at("lon").get<double>()) > 180 ||
         std::abs(area->at("lat").get<double>()) > 90 ||
         !(area->at("radius_m").get<double>() > 0 &&
           area->at("radius_m").get<double>() <= 1'000'000))) {
        return error("Expected valid geo coordinates and radius_m in (0, 1000000]");
    }

    GraphIndexManager::BoundedTraversalWorkMeter workMeter;
    if (delegationContext) {
        workMeter = [delegationContext](std::size_t candidates,
                                        std::size_t contentBytes) {
            return delegationContext->consumeBudget(
                SkillBudgetUsage{0, candidates, contentBytes});
        };
    }
    auto [traversalStatus, traversal] = graph.boundedBfs(
        startAsset, graphId, static_cast<int>(maxDepth), max_vertices, max_edges,
        std::chrono::milliseconds(500), edgeType, workMeter);
    if (!traversalStatus.ok) {
        return json{{"error", traversalStatus.message}};
    }

    json rows = json::array();
    std::size_t missingLocation = 0;
    std::optional<std::pair<double, double>> center;
    if (area != args.end()) {
        center = std::pair{
            area->at("lon").get<double>(), area->at("lat").get<double>()};
    }

    for (const auto& node : traversal.nodes) {
        std::optional<std::pair<double, double>> location;
        std::string nodeError;
        if (!readAssetLocation(
                db, graphId, node, center.has_value(), location, nodeError,
                delegationContext)) {
            return json{{"error", nodeError}};
        }

        double distance = 0;
        if (center) {
            if (!location) {
                ++missingLocation;
                continue;
            }
            distance = geo::haversineDistanceM(
                center->first, center->second, location->first, location->second);
            if (!std::isfinite(distance) || distance > area->at("radius_m").get<double>()) {
                continue;
            }
        }

        TSStore::QueryOptions options;
        options.metric = metric;
        options.entity = node.vertexId;
        options.limit = max_samples + 1;
        if (delegationContext) {
            options.work_meter = [delegationContext](
                                     std::size_t candidates,
                                     std::size_t contentBytes) {
                return delegationContext->consumeBudget(
                    SkillBudgetUsage{0, candidates, contentBytes});
            };
        }
        options.from_timestamp_ms = baseline;
        options.to_timestamp_ms = baseline + duration - 1;
        const auto baselinePoints = store.queryStrict(options);
        options.from_timestamp_ms = current;
        options.to_timestamp_ms = current + duration - 1;
        const auto currentPoints = store.queryStrict(options);
        if (!baselinePoints || !currentPoints) {
            return error(delegationContext
                             ? "Time series read failed or delegated budget exhausted"
                             : "Time series read failed");
        }
        if (baselinePoints->size() > max_samples || currentPoints->size() > max_samples) {
            return error("Time window exceeds the 4096-sample analysis limit");
        }

        const auto mean = [&](const auto& points, int64_t windowStart,
                              std::optional<double>& value) {
            if (points.empty()) {
                value.reset();
                return true;
            }
            double result = 0;
            for (const auto& point : points) {
                if (point.entity != node.vertexId || point.metric != metric ||
                    point.timestamp_ms < windowStart ||
                    point.timestamp_ms >= windowStart + duration ||
                    !std::isfinite(point.value)) {
                    return false;
                }
                result += point.value / static_cast<double>(points.size());
            }
            if (!std::isfinite(result)) {
                return false;
            }
            value = result;
            return true;
        };
        std::optional<double> baselineMean;
        std::optional<double> currentMean;
        if (!mean(*baselinePoints, baseline, baselineMean) ||
            !mean(*currentPoints, current, currentMean)) {
            return error("Time series contains invalid or unrepresentable samples");
        }

        json row = {
            {"asset_id", node.vertexId},
            {"depth", node.depth},
            {"baseline_count", baselinePoints->size()},
            {"current_count", currentPoints->size()},
            {"baseline_mean", baselineMean ? json(*baselineMean) : json(nullptr)},
            {"current_mean", currentMean ? json(*currentMean) : json(nullptr)},
            {"delta", nullptr}
        };
        if (baselineMean && currentMean) {
            const auto delta = *currentMean - *baselineMean;
            if (!std::isfinite(delta)) {
                return error("Difference between window means is not representable");
            }
            row["delta"] = delta;
        }
        if (center) {
            row["distance_m"] = distance;
        }
        rows.push_back(std::move(row));
    }

    return {
        {"metric", metric},
        {"graph_id", graphId},
        {"edge_type", edgeType},
        {"start_asset_id", startAsset},
        {"edges_examined", traversal.edgesExamined},
        {"missing_location", missingLocation},
        {"assets", std::move(rows)}
    };
}

} // namespace detail

namespace detail {

inline ToolSpec makeAssetImpactToolSpec() {
    const json ms = {{"type", "integer"}, {"minimum", 0}};
    const json number = {{"type", "number"}};
    ToolSpec spec;
    spec.name = "analyze_asset_impact";
    spec.description =
        "Compare time-series windows for assets reached through an authorized graph relation.";
    spec.args_schema = {
        {"type", "object"}, {"additionalProperties", false},
        {"required", {"start_asset_id", "max_depth", "baseline_start_ms",
                      "current_start_ms", "duration_ms"}},
        {"properties", {
            {"start_asset_id", {{"type", "string"}, {"minLength", 1}, {"maxLength", 1024}}},
            {"max_depth", {{"type", "integer"}, {"minimum", 0}, {"maximum", 3}}},
            {"baseline_start_ms", ms}, {"current_start_ms", ms}, {"duration_ms", ms},
            {"geo", {{"type", "object"}, {"additionalProperties", false},
                     {"required", {"lon", "lat", "radius_m"}},
                     {"properties", {{"lon", number}, {"lat", number}, {"radius_m", number}}}}}
        }}
    };
    return spec;
}

inline void registerAssetImpactAnalysisToolImpl(
    ToolRegistry& tools,
    std::shared_ptr<RocksDBWrapper> db,
    std::shared_ptr<GraphIndexManager> graph,
    std::shared_ptr<const TSStore> store,
    const std::string& graphId,
    const std::string& edgeType,
    const std::string& metric,
    bool delegated) {
    constexpr auto name = "analyze_asset_impact";
    if (!db || !db->getRawDB() || !graph || !store ||
        !validAssetImpactKey(graphId, 1024) ||
        !validAssetImpactKey(edgeType, 128) ||
        !validAssetImpactKey(metric, 128) ||
        tools.getSpec(name).has_value()) {
        throw std::invalid_argument("Invalid or duplicate asset impact tool binding");
    }

    auto spec = makeAssetImpactToolSpec();
    struct Sources {
        std::shared_ptr<RocksDBWrapper> db;
        std::shared_ptr<GraphIndexManager> graph;
        std::shared_ptr<const TSStore> store;
    };
    const auto sources =
        Sources{std::move(db), std::move(graph), std::move(store)};
    const auto boundGraphId = graphId;
    const auto boundEdgeType = edgeType;
    const auto boundMetric = metric;
    if (delegated) {
        tools.registerDelegatedTool(
            spec,
            [sources, boundGraphId, boundEdgeType, boundMetric](
                const json& args, const ModeSpec&,
                const SkillDelegationContext& delegationContext) {
                return analyzeAssetImpact(
                    *sources.db, *sources.graph, *sources.store, boundGraphId,
                    boundEdgeType, boundMetric, args, &delegationContext);
            });
    } else {
        tools.registerTool(
            spec,
            [sources, boundGraphId, boundEdgeType, boundMetric](
                const json& args, const ModeSpec&) {
                return analyzeAssetImpact(
                    *sources.db, *sources.graph, *sources.store, boundGraphId,
                    boundEdgeType, boundMetric, args);
            });
    }
}

} // namespace detail

/**
 * @brief Register the opt-in, read-only `analyze_asset_impact` tool.
 * @param tools Caller-scoped registry; its mode must explicitly allow the tool.
 * @param db Open database containing the bound graph and node documents.
 * @param graph Graph index bound to the same database wrapper.
 * @param store Time series store bound to the authorized metric and entities.
 * @param graphId Trusted property-graph scope; never selectable by tool input.
 * @param edgeType Trusted relationship type used for outgoing impact propagation.
 * @param metric Trusted TSStore metric; never selectable by tool input.
 * @throws std::invalid_argument On missing dependencies, invalid bindings or duplicate registration.
 * @note Authorize the complete graph scope and corresponding series before registration.
 *       Keep the registry principal-scoped and the database open while calls run.
 *       Graph traversal fails rather than returning partial results when limits or storage
 *       checks fail. Reads are synchronous and not atomic across graph and time-series data.
 *       Registering this tool does not automatically grant it to any mode.
 */
inline void registerAssetImpactAnalysisTool(
    ToolRegistry& tools,
    std::shared_ptr<RocksDBWrapper> db,
    std::shared_ptr<GraphIndexManager> graph,
    std::shared_ptr<const TSStore> store,
    const std::string& graphId,
    const std::string& edgeType,
    const std::string& metric) {
    detail::registerAssetImpactAnalysisToolImpl(
        tools, std::move(db), std::move(graph), std::move(store), graphId,
        edgeType, metric, false);
}

/**
 * @brief Register `analyze_asset_impact` with host-issued delegated authorization.
 * @param tools Caller-scoped registry; its mode must also allow the tool.
 * @param db Open database containing the bound graph and node documents.
 * @param graph Graph index bound to the same database.
 * @param store TSStore containing only caller-authorized series.
 * @param graphId Exact resource required for the `graph.read` capability.
 * @param edgeType Bound outgoing relationship type; not selectable by arguments.
 * @param metric Exact resource required for the `timeseries.read` capability.
 * @throws std::invalid_argument If dependencies/bindings are invalid or the
 *         tool name is already registered.
 * @note Dispatch also requires `tool.invoke` for `analyze_asset_impact`.
 *       Candidates include adjacency entries, node documents, and time-series
 *       points. Content bytes count serialized edge/node documents and
 *       time-series records/chunks plus decrypted Gorilla payload bytes;
 *       decoded points are charged as candidates. Payload bytes are charged
 *       after storage retrieval and before parsing/decoding, not as physical
 *       storage-I/O admission. Charges are shared across descendants and
 *       failures return an error without partial results. The database and
 *       registry must remain scoped to the same authorized principal.
 */
inline void registerDelegatedAssetImpactAnalysisTool(
    ToolRegistry& tools,
    std::shared_ptr<RocksDBWrapper> db,
    std::shared_ptr<GraphIndexManager> graph,
    std::shared_ptr<const TSStore> store,
    const std::string& graphId,
    const std::string& edgeType,
    const std::string& metric) {
    detail::registerAssetImpactAnalysisToolImpl(
        tools, std::move(db), std::move(graph), std::move(store), graphId,
        edgeType, metric, true);
}

} // namespace themis::llm
