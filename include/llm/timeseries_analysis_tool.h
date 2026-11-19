#pragma once

#include "geo/geo_math.h"
#include "llm/ai_orchestrator.h"
#include "storage/key_schema.h"
#include "storage/rocksdb_wrapper.h"
#include "timeseries/tsstore.h"

#include <cmath>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace themis::llm {
namespace detail {

inline bool validSeriesKey(std::string_view key) {
    return !key.empty() && key.size() <= 128 &&
           key.find(':') == std::string_view::npos &&
           key.find('\0') == std::string_view::npos;
}

inline bool validCoordinates(const json& lon, const json& lat) {
    return lon.is_number() && lat.is_number() &&
           std::abs(lon.get<double>()) <= 180 && std::abs(lat.get<double>()) <= 90;
}

inline json compareSeriesWindows(RocksDBWrapper& db, const TSStore& store,
                                 const std::string& prefix, const std::string& metric,
                                 const json& args) {
    constexpr int64_t max_window_ms = 31LL * 24 * 60 * 60 * 1000;
    constexpr std::size_t max_entities = 64;
    constexpr std::size_t max_samples = 4096;
    const auto error = [](const char* message) { return json{{"error", message}}; };
    if (!args.is_object() || args.size() != 3 + args.contains("geo")) {
        return error("Expected baseline_start_ms, current_start_ms, duration_ms and optional geo");
    }
    const auto parseMs = [&args](const char* key) -> int64_t {
        const auto it = args.find(key);
        if (it == args.end() || !it->is_number_integer() ||
            (it->is_number_unsigned() && it->get<uint64_t>() > INT64_MAX)) {
            return -1;
        }
        return it->get<int64_t>();
    };
    const auto baseline = parseMs("baseline_start_ms");
    const auto current = parseMs("current_start_ms");
    const auto duration = parseMs("duration_ms");
    if (baseline < 0 || current < baseline || duration < 1 ||
        duration > max_window_ms || current - baseline < duration ||
        current > INT64_MAX - duration) {
        return error("Expected non-overlapping baseline and current windows of 1 ms to 31 days");
    }
    const auto area = args.find("geo");
    if (area != args.end() &&
        (!area->is_object() || area->size() != 3 || !area->contains("lon") ||
         !area->contains("lat") || !area->contains("radius_m") ||
         !validCoordinates(area->at("lon"), area->at("lat")) ||
         !area->at("radius_m").is_number() ||
         !(area->at("radius_m").get<double>() > 0 &&
           area->at("radius_m").get<double>() <= 1'000'000))) {
        return error("Expected geo lon, lat and radius_m in (0, 1000000]");
    }

    auto iterator = db.newIterator();
    if (!iterator) {
        return error("Could not read the entity collection");
    }
    auto& cursor = **iterator;
    json rows = json::array();
    std::size_t scanned = 0;
    std::size_t missing_location = 0;
    for (cursor.Seek(prefix); cursor.Valid() && cursor.key().starts_with(prefix); cursor.Next()) {
        if (++scanned > max_entities) {
            return error("Entity collection exceeds the 64-document analysis limit");
        }
        const auto key = cursor.key();
        const std::string entity(key.data() + prefix.size(), key.size() - prefix.size());
        if (!validSeriesKey(entity)) {
            return error("Invalid entity key for time series lookup");
        }
        double distance = 0;
        if (area != args.end()) {
            const auto value = cursor.value();
            if (value.size() > 64 * 1024) {
                return error("Entity document exceeds the 64 KiB geometry-read limit");
            }
            const auto document = json::parse(value.data(), value.data() + value.size(), nullptr, false);
            if (!document.is_object()) {
                return error("Entity document must contain a JSON object");
            }
            const auto point = document.find("location");
            if (point == document.end()) {
                ++missing_location;
                continue;
            }
            if (!point->is_object() || !point->contains("type") ||
                point->at("type") != "Point" || !point->contains("coordinates")) {
                return error("Entity location is not a GeoJSON Point");
            }
            const auto& coordinates = point->at("coordinates");
            if (!coordinates.is_array() || coordinates.size() != 2 ||
                !validCoordinates(coordinates[0], coordinates[1])) {
                return error("Invalid GeoJSON Point coordinates");
            }
            distance = geo::haversineDistanceM(
                area->at("lon").get<double>(), area->at("lat").get<double>(),
                coordinates[0].get<double>(), coordinates[1].get<double>());
            if (!std::isfinite(distance) || distance > area->at("radius_m").get<double>()) {
                continue;
            }
        }
        TSStore::QueryOptions options;
        options.metric = metric;
        options.entity = entity;
        options.limit = max_samples + 1;
        const auto readWindow = [&](int64_t start) {
            options.from_timestamp_ms = start;
            options.to_timestamp_ms = start + duration - 1;
            return store.query(options);
        };
        const auto past = readWindow(baseline);
        const auto now = readWindow(current);
        if (!past || !now) {
            return error("Time series read failed");
        }
        if (past->size() > max_samples || now->size() > max_samples) {
            return error("Time window exceeds the 4096-sample analysis limit");
        }
        const auto mean = [&](const auto& points, int64_t start) -> json {
            if (points.empty()) {
                return nullptr;
            }
            double result = 0;
            for (const auto& point : points) {
                if (point.entity != entity || point.metric != metric ||
                    point.timestamp_ms < start || point.timestamp_ms >= start + duration ||
                    !std::isfinite(point.value)) {
                    throw std::runtime_error("Time series contains an invalid sample");
                }
                result += point.value / static_cast<double>(points.size());
            }
            if (!std::isfinite(result)) {
                throw std::runtime_error("Time series mean is not representable");
            }
            return result;
        };
        const auto baseline_mean = mean(*past, baseline);
        const auto current_mean = mean(*now, current);
        json row = {{"entity", entity}, {"baseline_count", past->size()},
                    {"current_count", now->size()}, {"baseline_mean", baseline_mean},
                    {"current_mean", current_mean}, {"delta", nullptr}};
        if (!past->empty() && !now->empty()) {
            const auto delta = current_mean.get<double>() - baseline_mean.get<double>();
            if (!std::isfinite(delta)) {
                return error("Difference between window means is not representable");
            }
            row["delta"] = delta;
        }
        if (area != args.end()) {
            row["distance_m"] = distance;
        }
        rows.push_back(std::move(row));
    }
    if (!cursor.status().ok()) {
        return error("Entity collection scan failed");
    }
    return {{"metric", metric}, {"baseline_start_ms", baseline}, {"current_start_ms", current},
            {"duration_ms", duration}, {"missing_location", missing_location},
            {"entities", std::move(rows)}};
}

} // namespace detail

/**
 * @brief Register the opt-in, read-only `compare_timeseries_windows` tool.
 * @param tools Caller-scoped registry; its mode must explicitly allow the tool.
 * @param db Open database containing the authorized entity collection.
 * @param store Time series store for that caller and metric. It is destroyed
 *        before the database handle retained by the callback.
 * @param collection Trusted collection containing only entities the caller
 *        may read, including their series. The primary key is the TSStore entity ID.
 * @param metric Trusted, authorized TSStore metric; not selectable by tool input.
 * @throws std::invalid_argument On missing dependencies, invalid names or duplicate registration.
 * @note Bind only after verifying caller scope for both data sources. Do not
 *       reuse the registry for other principals or close the database while in use.
 *       Register before publishing the registry. Reads are synchronous and not
 *       cancellable via this tool; the two windows do not share a snapshot.
 *       Invalid arguments, storage errors, and limit violations return an `error`
 *       object; missing measurements produce null means and delta, not zeros.
 */
inline void registerTimeseriesAnalysisTool(ToolRegistry& tools,
                                           std::shared_ptr<RocksDBWrapper> db,
                                           std::shared_ptr<const TSStore> store,
                                           const std::string& collection,
                                           const std::string& metric) {
    constexpr auto name = "compare_timeseries_windows";
    if (!db || !db->getRawDB() || !store || !detail::validSeriesKey(collection) ||
        !detail::validSeriesKey(metric) || tools.getSpec(name).has_value()) {
        throw std::invalid_argument("Invalid or duplicate time series tool binding");
    }
    const json ms = {{"type", "integer"}, {"minimum", 0}};
    const json number = {{"type", "number"}};
    ToolSpec spec;
    spec.name = name;
    spec.description = "Compare two equal time windows for authorized entities, optionally within a radius.";
    spec.args_schema = {
        {"type", "object"}, {"additionalProperties", false},
        {"required", {"baseline_start_ms", "current_start_ms", "duration_ms"}},
        {"properties", {{"baseline_start_ms", ms}, {"current_start_ms", ms}, {"duration_ms", ms},
                        {"geo", {{"type", "object"}, {"additionalProperties", false},
                                 {"required", {"lon", "lat", "radius_m"}},
                                 {"properties", {{"lon", number}, {"lat", number}, {"radius_m", number}}}}}}}
    };
    struct Sources {
        std::shared_ptr<RocksDBWrapper> db;
        std::shared_ptr<const TSStore> store;
    };
    tools.registerTool(spec, [sources = Sources{std::move(db), std::move(store)},
                              prefix = KeySchema::makeRelationalKey(collection, ""), metric](
                                 const json& args, const ModeSpec&) {
        return detail::compareSeriesWindows(*sources.db, *sources.store, prefix, metric, args);
    });
}
} // namespace themis::llm
