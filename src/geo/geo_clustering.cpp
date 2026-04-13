/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            geo_clustering.cpp                                 ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-04-13 04:25:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     314                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 104da2fb8d  2026-02-25  fix(geo/audit): add explicit <limits>, remove unused test... ║
    • 3cd57ddfb3  2026-02-25  feat(geo): implement DBSCAN and k-means clustering for ge... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "geo/geo_clustering.h"
#include "geo/spatial_join.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace themis {
namespace geo {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

/// Extract (lon, lat) from a Point GeometryInfo.
/// Returns {0.0, 0.0} and sets `valid` = false for non-Point geometries.
struct LonLat {
    double lon{0.0};
    double lat{0.0};
    bool valid{false};
};

static LonLat extractLonLat(const GeometryInfo& g) noexcept {
    if (g.isPoint() && !g.coords.empty()) {
        return {g.coords[0].x, g.coords[0].y, true};
    }
    return {};
}

// ---------------------------------------------------------------------------
// DBSCAN implementation
// ---------------------------------------------------------------------------

GeoClusterResult dbscanCluster(
    const std::vector<GeometryInfo>& points,
    const DbscanConfig& config)
{
    const std::size_t n = points.size();
    GeoClusterResult result;
    result.labels.assign(n, kDbscanUnclassified);

    if (n == 0 || config.epsilon_m <= 0.0 || config.min_points == 0) {
        // Mark everything as noise for degenerate configs
        std::fill(result.labels.begin(), result.labels.end(), kDbscanNoise);
        result.num_clusters = 0;
        return result;
    }

    // Pre-extract coordinates; mark non-Point entries as noise immediately.
    std::vector<LonLat> coords(n);
    for (std::size_t i = 0; i < n; ++i) {
        coords[i] = extractLonLat(points[i]);
        if (!coords[i].valid) {
            result.labels[i] = kDbscanNoise;
        }
    }

    // Returns the indices of all valid points within epsilon_m of point i.
    auto regionQuery = [&](std::size_t i) -> std::vector<std::size_t> {
        std::vector<std::size_t> neighbours;
        for (std::size_t j = 0; j < n; ++j) {
            if (!coords[j].valid) continue;
            const double dist = haversineDistanceM(
                coords[i].lon, coords[i].lat,
                coords[j].lon, coords[j].lat);
            if (dist <= config.epsilon_m) {
                neighbours.push_back(j);
            }
        }
        return neighbours;
    };

    int cluster_id = 0;

    for (std::size_t i = 0; i < n; ++i) {
        // Skip already processed or invalid points.
        if (result.labels[i] != kDbscanUnclassified) continue;
        if (!coords[i].valid) continue;

        std::vector<std::size_t> neighbours = regionQuery(i);

        if (neighbours.size() < config.min_points) {
            // Mark as noise for now; may be density-reachable from another core.
            result.labels[i] = kDbscanNoise;
            continue;
        }

        // Start a new cluster.
        result.labels[i] = cluster_id;

        // Seed queue with neighbours (excluding i itself).
        std::vector<std::size_t> queue;
        queue.reserve(neighbours.size());
        for (std::size_t nb : neighbours) {
            if (nb != i) queue.push_back(nb);
        }

        std::size_t qi = 0;
        while (qi < queue.size()) {
            const std::size_t j = queue[qi++];

            if (result.labels[j] == kDbscanNoise) {
                // Border point: assign to current cluster but don't expand.
                result.labels[j] = cluster_id;
            }

            if (result.labels[j] != kDbscanUnclassified) continue;

            result.labels[j] = cluster_id;

            std::vector<std::size_t> j_neighbours = regionQuery(j);
            if (j_neighbours.size() >= config.min_points) {
                // j is a core point; add its unvisited neighbours.
                for (std::size_t nb : j_neighbours) {
                    if (result.labels[nb] == kDbscanUnclassified ||
                        result.labels[nb] == kDbscanNoise) {
                        queue.push_back(nb);
                    }
                }
            }
        }

        ++cluster_id;
    }

    result.num_clusters = cluster_id;
    return result;
}

// ---------------------------------------------------------------------------
// k-means implementation
// ---------------------------------------------------------------------------

GeoClusterResult kmeansCluster(
    const std::vector<GeometryInfo>& points,
    const KMeansConfig& config)
{
    const std::size_t n = points.size();
    GeoClusterResult result;
    result.labels.assign(n, -1);

    if (n == 0) {
        result.num_clusters = 0;
        return result;
    }

    // Pre-extract valid coordinates.
    std::vector<LonLat> coords(n);
    std::vector<std::size_t> valid_idx;
    valid_idx.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        coords[i] = extractLonLat(points[i]);
        if (coords[i].valid) valid_idx.push_back(i);
    }

    const std::size_t valid_n = valid_idx.size();

    if (config.k == 0) {
        throw std::invalid_argument("kmeansCluster: k must be >= 1");
    }
    if (config.k > valid_n) {
        throw std::invalid_argument(
            "kmeansCluster: k (" + std::to_string(config.k) +
            ") exceeds number of valid points (" +
            std::to_string(valid_n) + ")");
    }

    // -----------------------------------------------------------------
    // Centroid initialisation
    // -----------------------------------------------------------------

    struct Centroid { double lon; double lat; };
    std::vector<Centroid> centroids(config.k);

    if (config.seed == 0) {
        // Deterministic: pick the first k distinct valid points.
        for (std::size_t c = 0; c < config.k; ++c) {
            centroids[c] = {coords[valid_idx[c]].lon, coords[valid_idx[c]].lat};
        }
    } else {
        // k-means++ probabilistic seeding with a simple LCG PRNG.
        // LCG constants from Numerical Recipes.
        uint64_t rng = config.seed;
        auto nextDouble = [&]() -> double {
            rng = rng * 6364136223846793005ULL + 1442695040888963407ULL;
            return static_cast<double>(rng >> 33) / static_cast<double>(1ULL << 31);
        };

        // Choose the first centroid uniformly at random from valid points.
        std::size_t first = static_cast<std::size_t>(nextDouble() * valid_n) % valid_n;
        centroids[0] = {coords[valid_idx[first]].lon, coords[valid_idx[first]].lat};

        // For each subsequent centroid, choose proportionally to squared distance
        // from the nearest already-chosen centroid.
        std::vector<double> d2(valid_n, std::numeric_limits<double>::max());

        for (std::size_t c = 1; c < config.k; ++c) {
            // Update d2 for the newly added centroid c-1.
            double total = 0.0;
            for (std::size_t vi = 0; vi < valid_n; ++vi) {
                const double dist = haversineDistanceM(
                    coords[valid_idx[vi]].lon, coords[valid_idx[vi]].lat,
                    centroids[c - 1].lon, centroids[c - 1].lat);
                if (dist * dist < d2[vi]) d2[vi] = dist * dist;
                total += d2[vi];
            }

            // Sample next centroid proportionally to d2.
            double threshold = nextDouble() * total;
            double cumsum = 0.0;
            std::size_t chosen = valid_n - 1; // fallback
            for (std::size_t vi = 0; vi < valid_n; ++vi) {
                cumsum += d2[vi];
                if (cumsum >= threshold) {
                    chosen = vi;
                    break;
                }
            }
            centroids[c] = {coords[valid_idx[chosen]].lon, coords[valid_idx[chosen]].lat};
        }
    }

    // -----------------------------------------------------------------
    // Lloyd iterations
    // -----------------------------------------------------------------

    std::vector<int>    cluster_labels(valid_n, 0);
    std::vector<double> centroid_sum_lon(config.k, 0.0);
    std::vector<double> centroid_sum_lat(config.k, 0.0);
    std::vector<std::size_t> centroid_count(config.k, 0);

    for (std::size_t iter = 0; iter < config.max_iterations; ++iter) {

        // Assignment step: assign each valid point to the nearest centroid.
        for (std::size_t vi = 0; vi < valid_n; ++vi) {
            double best_dist = std::numeric_limits<double>::max();
            int    best_c    = 0;
            for (std::size_t c = 0; c < config.k; ++c) {
                const double dist = haversineDistanceM(
                    coords[valid_idx[vi]].lon, coords[valid_idx[vi]].lat,
                    centroids[c].lon, centroids[c].lat);
                if (dist < best_dist) {
                    best_dist = dist;
                    best_c    = static_cast<int>(c);
                }
            }
            cluster_labels[vi] = best_c;
        }

        // Update step: recompute centroids as arithmetic mean of (lon, lat).
        std::fill(centroid_sum_lon.begin(), centroid_sum_lon.end(), 0.0);
        std::fill(centroid_sum_lat.begin(), centroid_sum_lat.end(), 0.0);
        std::fill(centroid_count.begin(),   centroid_count.end(),   0);

        for (std::size_t vi = 0; vi < valid_n; ++vi) {
            const int c = cluster_labels[vi];
            centroid_sum_lon[c] += coords[valid_idx[vi]].lon;
            centroid_sum_lat[c] += coords[valid_idx[vi]].lat;
            ++centroid_count[c];
        }

        // Check convergence and update centroids.
        double max_shift = 0.0;
        for (std::size_t c = 0; c < config.k; ++c) {
            if (centroid_count[c] == 0) continue; // empty cluster; keep old centroid

            const double new_lon = centroid_sum_lon[c] / static_cast<double>(centroid_count[c]);
            const double new_lat = centroid_sum_lat[c] / static_cast<double>(centroid_count[c]);

            const double shift = haversineDistanceM(
                centroids[c].lon, centroids[c].lat, new_lon, new_lat);
            if (shift > max_shift) max_shift = shift;

            centroids[c] = {new_lon, new_lat};
        }

        if (max_shift <= config.tolerance_m) break;
    }

    // Write labels back to result (index into original `points` array).
    for (std::size_t vi = 0; vi < valid_n; ++vi) {
        result.labels[valid_idx[vi]] = cluster_labels[vi];
    }
    result.num_clusters = static_cast<int>(config.k);
    return result;
}

} // namespace geo
} // namespace themis
