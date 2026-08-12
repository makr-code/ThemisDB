/**
 * @file geo_clustering.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "utils/geo/ewkb.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace themis {
namespace geo {

/// Label assigned to a point that belongs to no cluster (DBSCAN noise).
static constexpr int kDbscanNoise = -1;

/// Label assigned to a point that has not yet been classified.
static constexpr int kDbscanUnclassified = -2;

/**
 * @brief Result of a DBSCAN or k-means clustering run.
 *
 * `labels[i]` holds the cluster index (0-based) for the i-th input point.
 * For DBSCAN, noise points receive label `kDbscanNoise` (-1).
 * `num_clusters` is the total number of discovered (non-noise) clusters.
 */
struct GeoClusterResult {
    /// Cluster label for each input point, parallel to the input vector.
    std::vector<int> labels;
    /// Number of distinct clusters found (excludes DBSCAN noise).
    int num_clusters{0};
};

/**
 * @brief Configuration for the DBSCAN geo clustering algorithm.
 *
 * DBSCAN groups nearby points into clusters based on density.
 * Points that are not reachable from any core point are labeled as noise.
 */
struct DbscanConfig {
    /// Neighbourhood radius in metres (ε).  Must be > 0.
    double epsilon_m{500.0};
    /// Minimum number of points (including the core point itself) required
    /// to form a dense neighbourhood.  Must be ≥ 1.
    std::size_t min_points{3};
};

/**
 * @brief Configuration for the k-means geo clustering algorithm.
 *
 * k-means partitions `n` points into exactly `k` clusters by minimising
 * the sum of squared geodesic distances to cluster centroids.
 */
struct KMeansConfig {
    /// Number of clusters to produce.  Must be ≥ 1 and ≤ number of points.
    std::size_t k{3};
    /// Maximum number of Lloyd iterations.
    std::size_t max_iterations{100};
    /// Convergence tolerance: stop early when all centroid shifts are ≤ this
    /// value in metres between two consecutive iterations.
    double tolerance_m{1.0};
    /// Seed for the initial centroid selection (k-means++ initialisation).
    /// Use 0 for a deterministic default based on the first k distinct points.
    uint64_t seed{0};
};

/**
 * @brief Configuration controlling GPU acceleration for clustering algorithms.
 *
 * When `use_gpu` is true and CUDA is available, clustering will use GPU kernels
 * for the distance computation phase:
 *  - k-Means: ECEF 3D projection + FAISS GPU FLAT_L2 for the assignment step.
 *  - DBSCAN: GPU Haversine batch kernel for precomputing the adjacency matrix;
 *            BFS expansion runs on CPU using the precomputed matrix.  Only
 *            datasets with `n ≤ gpu_dbscan_max_n` are eligible (larger datasets
 *            fall back to CPU to stay within VRAM limits).
 *
 * When `use_gpu` is false, or when no CUDA device is present, the CPU path is
 * used transparently — callers do not need to check this at the call site.
 */
struct GpuClusteringConfig {
    /// Allow GPU acceleration when available.  Default true.
    bool use_gpu{true};
    /// Maximum point count for GPU DBSCAN (adjacency matrix = n² bits).
    /// At 32768 points the matrix is 128 MiB on GPU.  Default: 32768.
    std::size_t gpu_dbscan_max_n{32768};
};

/**
 * @brief Cluster geo points using the DBSCAN algorithm.
 *
 * All input geometries must be of type `Point` (2D WGS84).  Non-Point
 * geometries are treated as noise (label `kDbscanNoise`) without raising
 * an error.
 *
 * Distance between points is computed with the Haversine formula on the
 * WGS-84 sphere (same as `haversineDistanceM` in `spatial_join.h`).
 *
 * Complexity: O(n²) in the worst case (no spatial index acceleration).
 * When CUDA is available and `n ≤ GpuClusteringConfig::gpu_dbscan_max_n`
 * the distance computation phase is GPU-accelerated, reducing wall-clock
 * time by up to GPU-parallelism factor.
 *
 * @param points   Input point geometries.
 * @param config   DBSCAN parameters (epsilon_m, min_points).
 * @param gpu_cfg  GPU acceleration configuration (default: GPU enabled).
 * @return GeoClusterResult with labels parallel to `points`.
 */
GeoClusterResult dbscanCluster(
    const std::vector<GeometryInfo>& points,
    const DbscanConfig& config = DbscanConfig{},
    const GpuClusteringConfig& gpu_cfg = GpuClusteringConfig{});

/**
 * @brief Cluster geo points using the k-means algorithm.
 *
 * All input geometries must be of type `Point` (2D WGS84).  Non-Point
 * geometries are skipped and receive label -1.
 *
 * Centroid positions are maintained in geographic (lon, lat) coordinates
 * and distance is computed with the Haversine formula.  Centroid updates
 * use an arithmetic mean of (lon, lat) coordinates which is a valid
 * approximation for clusters spanning less than a few hundred kilometres.
 *
 * When CUDA is available the assignment step (nearest centroid search)
 * uses ECEF 3D projection and FAISS GPU FLAT_L2, providing significant
 * speedup for large datasets (n ≥ 10000) with k ≤ 256.  The centroid
 * update step always runs on CPU.
 *
 * Initialisation: if `config.seed == 0` the first `k` distinct input points
 * are used as initial centroids.  Otherwise k-means++ probabilistic seeding
 * is performed using `config.seed` to seed a simple LCG PRNG.
 *
 * @param points   Input point geometries.
 * @param config   k-means parameters (k, max_iterations, tolerance_m, seed).
 * @param gpu_cfg  GPU acceleration configuration (default: GPU enabled).
 * @return GeoClusterResult with labels in [0, k) parallel to `points`.
 * @throws std::invalid_argument if k == 0 or k > number of valid points.
 */
GeoClusterResult kmeansCluster(
    const std::vector<GeometryInfo>& points,
    const KMeansConfig& config = KMeansConfig{},
    const GpuClusteringConfig& gpu_cfg = GpuClusteringConfig{});

} // namespace geo
} // namespace themis

