#include <benchmark/benchmark.h>

#include "geo/geo_json_geometry.h"

#include <nlohmann/json.hpp>

#include <sstream>
#include <string>
#include <vector>

using themis::Coordinate;
using themis::geo::CrsId;
using themis::geo::GeoMultiPolygon;
using themis::geo::GeoPolygon;

namespace {

std::string buildMultiPolygonGeoJson(std::size_t polygon_count) {
    std::ostringstream out;
    out << R"({"type":"MultiPolygon","coordinates":[)";
    for (std::size_t i = 0; i < polygon_count; ++i) {
        if (i != 0) {
            out << ",";
        }
        const double base_x = static_cast<double>(i % 1000) * 0.001;
        const double base_y = static_cast<double>(i / 1000) * 0.001;
        out << "[[["
            << base_x << "," << base_y << "],["
            << (base_x + 0.0005) << "," << base_y << "],["
            << (base_x + 0.0005) << "," << (base_y + 0.0005) << "],["
            << base_x << "," << (base_y + 0.0005) << "],["
            << base_x << "," << base_y << "]]]";
    }
    out << "]}";
    return out.str();
}

GeoMultiPolygon parseGeoMultiPolygon(const std::string& payload) {
    const auto parsed = nlohmann::json::parse(payload);
    const auto& coords = parsed.at("coordinates");

    std::vector<GeoPolygon> polygons;
    polygons.reserve(coords.size());
    for (const auto& polygon_json : coords) {
        std::vector<GeoPolygon::Ring> rings;
        rings.reserve(polygon_json.size());
        for (const auto& ring_json : polygon_json) {
            GeoPolygon::Ring ring;
            ring.reserve(ring_json.size());
            for (const auto& point_json : ring_json) {
                ring.emplace_back(Coordinate{
                    point_json.at(0).get<double>(),
                    point_json.at(1).get<double>(),
                });
            }
            rings.push_back(std::move(ring));
        }
        polygons.emplace_back(std::move(rings), CrsId::WGS84);
    }
    return GeoMultiPolygon(std::move(polygons), CrsId::WGS84);
}

} // namespace

static void BM_GeoJSONParse_MultiPolygon_100k(benchmark::State& state) {
    const auto polygon_count = static_cast<std::size_t>(state.range(0));
    const std::string payload = buildMultiPolygonGeoJson(polygon_count);

    for (auto _ : state) {
        auto geometry = parseGeoMultiPolygon(payload);
        const auto validation = geometry.validate();
        benchmark::DoNotOptimize(geometry);
        benchmark::DoNotOptimize(validation.ok());
    }

    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) *
        static_cast<int64_t>(polygon_count));
    state.counters["polygons_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations()) * static_cast<double>(polygon_count),
        benchmark::Counter::kIsRate);
}

BENCHMARK(BM_GeoJSONParse_MultiPolygon_100k)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000)
    ->Unit(benchmark::kMillisecond);

