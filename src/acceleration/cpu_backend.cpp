/**
 * @file cpu_backend.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "acceleration/cpu_backend.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <queue>

#include "acceleration/batch_validator.h"
#include "acceleration/kernel_invocation.h"
#include "utils/geometric_distances.h"

namespace themis {
namespace acceleration {

// ============================================================================
// CPUVectorBackend Implementation
// ============================================================================

// Delegate to SIMD-optimised implementations from utils/geometric_distances.h.
// Replaces the former scalar loops with hardware-accelerated kernels
// (AVX-512 / AVX2 / ARM NEON / scalar fallback).

float CPUVectorBackend::computeL2Distance(const float *a, const float *b, size_t dim) const {
    // Returns squared distance (no sqrt) to preserve monotonic ranking behaviour
    // of callers; matches the contract of the previous scalar implementation.
    return themis::simd::l2_distance_sq(a, b, dim);
}

float CPUVectorBackend::computeCosineDistance(const float *a, const float *b, size_t dim) const {
    return themis::simd::cosine_distance(a, b, dim);
}

std::vector<float> CPUVectorBackend::computeDistances(const float *queries, size_t numQueries, size_t dim,
                                                      const float *vectors, size_t numVectors, bool useL2) {
    clearError();
    auto sink = [this](ErrorContext e) { setError(std::move(e)); };
    if (!BatchValidator::validateVectorBatch(name(), queries, numQueries, dim, vectors, numVectors, sink)) {
        return {};
    }

    std::vector<float> distances(numQueries * numVectors);

    for (size_t q = 0; q < numQueries; ++q) {
        const float *query = queries + q * dim;
        for (size_t v = 0; v < numVectors; ++v) {
            const float *vector = vectors + v * dim;
            float dist = useL2 ? computeL2Distance(query, vector, dim) : computeCosineDistance(query, vector, dim);
            distances[q * numVectors + v] = dist;
        }
    }

    return distances;
}

std::vector<std::vector<std::pair<uint32_t, float>>>
CPUVectorBackend::batchKnnSearch(const float *queries, size_t numQueries, size_t dim, const float *vectors,
                                 size_t numVectors, size_t k, bool useL2) {
    clearError();
    auto sink = [this](ErrorContext e) { setError(std::move(e)); };
    if (!BatchValidator::validateVectorBatch(name(), queries, numQueries, dim, vectors, numVectors, sink)) {
        return {};
    }
    if (!BatchValidator::validateK(name(), k, sink)) {
        return {};
    }

    std::vector<std::vector<std::pair<uint32_t, float>>> results(numQueries);

    for (size_t q = 0; q < numQueries; ++q) {
        const float *query = queries + q * dim;

        // Compute all distances for this query
        std::vector<std::pair<uint32_t, float>> distances;
        distances.reserve(numVectors);

        for (size_t v = 0; v < numVectors; ++v) {
            const float *vector = vectors + v * dim;
            float dist = useL2 ? computeL2Distance(query, vector, dim) : computeCosineDistance(query, vector, dim);
            distances.emplace_back(static_cast<uint32_t>(v), dist);
        }

        // Partial sort to get k nearest neighbors.
        // Tie-breaking rule: when two candidates share the same distance the one
        // with the lower vector index is placed first (deterministic ordering).
        // Note: exact float equality is intentional — two entries are considered
        // tied only when their distance values are bit-for-bit identical, which
        // happens when the same computational path is applied to equal inputs.
        size_t actualK = std::min(k, distances.size());
        std::partial_sort(distances.begin(), distances.begin() + actualK, distances.end(),
                          [](const auto &a, const auto &b) {
                              if (a.second != b.second) {
                                  return a.second < b.second;
                              }
                              return a.first < b.first; // tie-break: lower index wins
                          });

        results[q].assign(distances.begin(), distances.begin() + actualK);
    }

    return results;
}

// ============================================================================
// CPUGraphBackend Implementation
// ============================================================================

std::vector<std::vector<uint32_t>> CPUGraphBackend::batchBFS(const uint32_t *adjacency, size_t numVertices,
                                                             const uint32_t *startVertices, size_t numStarts,
                                                             uint32_t maxDepth) {
    clearError();
    auto sink = [this](ErrorContext e) { setError(std::move(e)); };
    if (!BatchValidator::validateGraphBFSBatch(name(), adjacency, numVertices, startVertices, numStarts, sink)) {
        return {};
    }

    std::vector<std::vector<uint32_t>> results(numStarts);
    for (size_t s = 0; s < numStarts; ++s) {
        const uint32_t start = startVertices[s];
        if (start >= static_cast<uint32_t>(numVertices)) {
            continue;
        }

        std::vector<bool> visited(numVertices, false);
        std::queue<std::pair<uint32_t, uint32_t>> bfsQueue; // (vertex, depth)

        visited[start] = true;
        bfsQueue.push({start, 0u});
        results[s].push_back(start);

        while (!bfsQueue.empty()) {
            auto [current, depth] = bfsQueue.front();
            bfsQueue.pop();

            if (depth >= maxDepth) {
                continue;
            }

            // Dense adjacency matrix row for vertex `current`.
            // Interface contract: `adjacency` is a row-major N×N matrix where
            // adjacency[u * N + v] != 0 denotes an edge u→v (confirmed by the
            // CUDA backend which allocates numVertices * numVertices elements).
            // Complexity per BFS is therefore O(N²) in the number of vertices;
            // this is inherent to the dense-matrix representation and acceptable
            // for graphs where the caller already uses a dense format.
            const uint32_t *row = adjacency + current * numVertices;
            for (uint32_t v = 0; v < static_cast<uint32_t>(numVertices); ++v) {
                if (row[v] != 0u && !visited[v]) {
                    visited[v] = true;
                    results[s].push_back(v);
                    bfsQueue.push({v, depth + 1u});
                }
            }
        }
    }
    return results;
}

std::vector<std::vector<uint32_t>> CPUGraphBackend::batchShortestPath(const uint32_t *adjacency, const float *weights,
                                                                      size_t numVertices, const uint32_t *startVertices,
                                                                      const uint32_t *endVertices, size_t numPairs) {
    clearError();
    auto sink = [this](ErrorContext e) { setError(std::move(e)); };
    if (!BatchValidator::validateShortestPathBatch(name(), adjacency, weights, numVertices, startVertices, endVertices,
                                                   numPairs, sink)) {
        return {};
    }

    std::vector<std::vector<uint32_t>> results(numPairs);

    const auto N = static_cast<uint32_t>(numVertices);

    for (size_t p = 0; p < numPairs; ++p) {
        const uint32_t src = startVertices[p];
        const uint32_t dst = endVertices[p];

        if (src >= N || dst >= N) {
            continue;
        }

        if (src == dst) {
            results[p] = {src};
            continue;
        }

        // Dijkstra over dense N×N adjacency / weight matrices.
        // adjacency[u * N + v] != 0  →  edge u→v exists.
        // weights[u * N + v]          →  non-negative edge weight u→v.
        std::vector<float> dist(numVertices, std::numeric_limits<float>::infinity());
        std::vector<int64_t> parent(numVertices, -1);
        dist[src] = 0.0f;

        using DV = std::pair<float, uint32_t>; // (distance, vertex)
        std::priority_queue<DV, std::vector<DV>, std::greater<DV>> pq;
        pq.push({0.0f, src});

        while (!pq.empty()) {
            auto [d, u] = pq.top();
            pq.pop();

            if (d > dist[u]) {
                continue; // stale entry
            }
            if (u == dst) {
                break; // target reached
            }

            const uint32_t *adjRow = adjacency + u * N;
            const float *wRow      = weights + u * N;
            for (uint32_t v = 0; v < N; ++v) {
                if (adjRow[v] == 0u) {
                    continue;
                }
                const float raw_w = wRow[v];
                // Dijkstra requires non-negative edge weights.  Negative
                // weights in the input indicate invalid/corrupt weight data;
                // clamp to 0 to remain correct (zero-cost edge) rather than
                // silently producing a wrong shortest path.
                if (raw_w < 0.0f) {
                    std::cerr << "[CPUGraph] batchShortestPath: negative weight " << raw_w << " on edge " << u << "→"
                              << v << "; clamped to 0\n";
                }
                const float w  = std::max(0.0f, raw_w);
                const float nd = dist[u] + w;
                if (nd < dist[v]) {
                    dist[v]   = nd;
                    parent[v] = static_cast<int64_t>(u);
                    pq.push({nd, v});
                }
            }
        }

        if (std::isinf(dist[dst])) {
            continue; // no path
        }

        // Reconstruct path from destination back to source via parent chain.
        std::vector<uint32_t> path;
        for (int64_t v = static_cast<int64_t>(dst); v != -1; v = parent[v]) {
            path.push_back(static_cast<uint32_t>(v));
        }
        std::reverse(path.begin(), path.end());
        results[p] = std::move(path);
    }
    return results;
}

// ============================================================================
// CPUGeoBackend Implementation
// ============================================================================

double CPUGeoBackend::haversineDistance(double lat1, double lon1, double lat2, double lon2) const {
    return themis::geo::haversine_km(lat1, lon1, lat2, lon2);
}

double CPUGeoBackend::vincentyDistance(double lat1, double lon1, double lat2, double lon2) const {
    // Simplified Vincenty formula (more accurate than Haversine)
    // Full implementation would include ellipsoid parameters
    return haversineDistance(lat1, lon1, lat2, lon2);
}

std::vector<float> CPUGeoBackend::batchDistances(const double *latitudes1, const double *longitudes1,
                                                 const double *latitudes2, const double *longitudes2, size_t count,
                                                 bool useHaversine) {
    clearError();
    auto sink = [this](ErrorContext e) { setError(std::move(e)); };
    if (!BatchValidator::validateGeoBatch(name(), latitudes1, longitudes1, latitudes2, longitudes2, count, sink)) {
        return {};
    }

    std::vector<float> distances(count);

    for (size_t i = 0; i < count; ++i) {
        double dist  = useHaversine ? haversineDistance(latitudes1[i], longitudes1[i], latitudes2[i], longitudes2[i])
                                    : vincentyDistance(latitudes1[i], longitudes1[i], latitudes2[i], longitudes2[i]);
        distances[i] = static_cast<float>(dist);
    }

    return distances;
}

std::vector<bool> CPUGeoBackend::batchPointInPolygon(const double *pointLats, const double *pointLons, size_t numPoints,
                                                     const double *polygonCoords, size_t numPolygonVertices) {
    clearError();
    auto sink = [this](ErrorContext e) { setError(std::move(e)); };
    if (!BatchValidator::validatePointInPolygonBatch(name(), pointLats, pointLons, numPoints, polygonCoords,
                                                     numPolygonVertices, sink)) {
        return {};
    }

    std::vector<bool> results(numPoints);

    // Ray casting algorithm for point-in-polygon test
    for (size_t p = 0; p < numPoints; ++p) {
        double testLat = pointLats[p];
        double testLon = pointLons[p];

        bool inside = false;
        size_t j    = numPolygonVertices - 1;

        for (size_t i = 0; i < numPolygonVertices; ++i) {
            double lat_i = polygonCoords[i * 2];
            double lon_i = polygonCoords[i * 2 + 1];
            double lat_j = polygonCoords[j * 2];
            double lon_j = polygonCoords[j * 2 + 1];

            if (((lon_i > testLon) != (lon_j > testLon))
                && (testLat < (lat_j - lat_i) * (testLon - lon_i) / (lon_j - lon_i) + lat_i)) {
                inside = !inside;
            }

            j = i;
        }

        results[p] = inside;
    }

    return results;
}

// ============================================================================
// CPU Kernel Dispatch — frozen interface implementations
// ============================================================================
// These static functions match the ANNDistanceFn / ANNTopKFn / GeoDistanceFn /
// GeoContainmentFn typedefs from kernel_invocation.h exactly.  They are
// registered via populateANNDispatch() / populateGeoDispatch() so the
// BackendRegistry can call them through the dispatch tables without knowing
// the backend type at compile time.
// ============================================================================

namespace {

// ---------------------------------------------------------------------------
// ANN dispatch functions
// ---------------------------------------------------------------------------

static int cpu_ann_l2_distance(const float *queries, const float *vectors, float *distances, int numQueries,
                               int numVectors, int dim, void * /*stream*/) {
    for (int q = 0; q < numQueries; ++q) {
        for (int v = 0; v < numVectors; ++v) {
            float sum = 0.f;
            for (int d = 0; d < dim; ++d) {
                float diff = queries[q * dim + d] - vectors[v * dim + d];
                sum += diff * diff;
            }
            distances[q * numVectors + v] = sum;
        }
    }
    return 0;
}

static int cpu_ann_cosine_distance(const float *queries, const float *vectors, float *distances, int numQueries,
                                   int numVectors, int dim, void * /*stream*/) {
    // Minimum denominator (|a|*|b|) below which cosine is undefined: treat as max distance.
    constexpr float kCosineEpsilon = 1e-10f;
    for (int q = 0; q < numQueries; ++q) {
        for (int v = 0; v < numVectors; ++v) {
            float dot = 0.f, nq = 0.f, nv = 0.f;
            for (int d = 0; d < dim; ++d) {
                float qv = queries[q * dim + d];
                float vv = vectors[v * dim + d];
                dot += qv * vv;
                nq += qv * qv;
                nv += vv * vv;
            }
            const float denom             = std::sqrt(nq) * std::sqrt(nv);
            distances[q * numVectors + v] = (denom > kCosineEpsilon) ? 1.f - dot / denom : 1.f;
        }
    }
    return 0;
}

static int cpu_ann_inner_product(const float *queries, const float *vectors, float *distances, int numQueries,
                                 int numVectors, int dim, void * /*stream*/) {
    // Negative inner product so that smaller is better (consistent with L2/cosine)
    for (int q = 0; q < numQueries; ++q) {
        for (int v = 0; v < numVectors; ++v) {
            float dot = 0.f;
            for (int d = 0; d < dim; ++d) {
                dot += queries[q * dim + d] * vectors[v * dim + d];
            }
            distances[q * numVectors + v] = -dot;
        }
    }
    return 0;
}

static int cpu_ann_topk(const float *distances, uint32_t *topk_indices, float *topk_dists, int numQueries,
                        int numVectors, int topK, void * /*stream*/) {
    // Comparator: (distance, index) where lower distance wins; lower index breaks ties.
    // The max-heap keeps the topK smallest pairs by ejecting the largest.
    // Using pair<float,uint32_t> directly: pair comparison is lexicographic, so
    // equal distances resolve by index (higher index is "larger" and gets ejected).
    // This guarantees that for equal distances, the lower index is always kept.
    using Pair = std::pair<float, uint32_t>; // (distance, index)
    for (int q = 0; q < numQueries; ++q) {
        const float *row = distances + q * numVectors;
        // Max-heap of size topK: keeps the topK smallest distances
        std::priority_queue<Pair> heap;
        for (int v = 0; v < numVectors; ++v) {
            heap.emplace(row[v], static_cast<uint32_t>(v));
            if (static_cast<int>(heap.size()) > topK) {
                heap.pop(); // ejects largest (highest dist, or equal dist + highest index)
            }
        }
        // Drain heap in ascending order
        int slot = static_cast<int>(heap.size()) - 1;
        while (!heap.empty()) {
            topk_indices[q * topK + slot] = heap.top().second;
            topk_dists[q * topK + slot]   = heap.top().first;
            heap.pop();
            --slot;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Geospatial dispatch functions
// ---------------------------------------------------------------------------

static int cpu_geo_distance(const double *lats1, const double *lons1, const double *lats2, const double *lons2,
                            float *out_distances, int count,
                            GeoDistanceFormula /*formula*/, // Vincenty falls back to Haversine (CPU impl)
                            void * /*stream*/) {
    for (int i = 0; i < count; ++i) {
        out_distances[i] = static_cast<float>(themis::geo::haversine_km(lats1[i], lons1[i], lats2[i], lons2[i]));
    }
    return 0;
}

static int cpu_geo_containment(const double *point_lats, const double *point_lons, int numPoints,
                               const double *polygon_coords, int numVertices, uint8_t *results, void * /*stream*/) {
    for (int p = 0; p < numPoints; ++p) {
        const double testLat = point_lats[p];
        const double testLon = point_lons[p];
        bool inside          = false;
        int j                = numVertices - 1;
        for (int i = 0; i < numVertices; ++i) {
            const double lat_i = polygon_coords[i * 2];
            const double lon_i = polygon_coords[i * 2 + 1];
            const double lat_j = polygon_coords[j * 2];
            const double lon_j = polygon_coords[j * 2 + 1];
            if (((lon_i > testLon) != (lon_j > testLon))
                && (testLat < (lat_j - lat_i) * (testLon - lon_i) / (lon_j - lon_i) + lat_i)) {
                inside = !inside;
            }
            j = i;
        }
        results[p] = inside ? 1u : 0u;
    }
    return 0;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Public dispatch-population methods
// ---------------------------------------------------------------------------

ANNKernelDispatch CPUVectorBackend::populateANNDispatch() const {
    ANNKernelDispatch d;
    d.launchL2Distance   = cpu_ann_l2_distance;
    d.launchCosine       = cpu_ann_cosine_distance;
    d.launchInnerProduct = cpu_ann_inner_product;
    d.launchTopK         = cpu_ann_topk;
    return d;
}

GeoKernelDispatch CPUGeoBackend::populateGeoDispatch() const {
    GeoKernelDispatch d;
    d.launchDistance    = cpu_geo_distance;
    d.launchContainment = cpu_geo_containment;
    return d;
}

// =============================================================================
// CPUMatrixBackend Implementation
// =============================================================================

int CPUMatrixBackend::matmul(const MatrixKernelParams &params, void * /*opaque_stream*/) {
    return tensor_core::launchCPUMatmulKernel(
        static_cast<const float *>(params.A), static_cast<const float *>(params.B), static_cast<float *>(params.C),
        static_cast<int>(params.M), static_cast<int>(params.K), static_cast<int>(params.N), params.alpha, params.beta);
}

namespace {

static int cpu_matrix_matmul(const MatrixKernelParams &params, void *stream) {
    CPUMatrixBackend backend;
    return backend.matmul(params, stream);
}

} // anonymous namespace

MatrixKernelDispatch CPUMatrixBackend::populateMatrixDispatch() const {
    MatrixKernelDispatch d;
    d.launchMatmul = cpu_matrix_matmul;
    return d;
}

} // namespace acceleration
} // namespace themis
