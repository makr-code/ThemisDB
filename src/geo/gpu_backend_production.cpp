/**
 * @file gpu_backend_production.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=14, H=30, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include <algorithm>
#include <chrono>
#include <cmath>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "geo/gpu_buffer_guard.h"
#include "geo/spatial_backend.h"
#include "utils/geo/ewkb.h"
#include "utils/logger.h"

#ifdef THEMIS_ENABLE_CUDA
#include <cuda_runtime.h>
#endif

#ifdef THEMIS_ENABLE_OPENCL
#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#endif

namespace themis {
namespace geo {

// CPU-parallel backend using threading for batch spatial operations
/** @brief CPU-parallel backend using threading for batch spatial operations. */
class CpuParallelBackend final : public ISpatialComputeBackend {
  public:
    CpuParallelBackend() {
        thread_count_ = std::max(1u, std::thread::hardware_concurrency());
    }

    const char *name() const noexcept override {
        return "cpu_parallel";
    }

    bool isAvailable() const noexcept override {
        return true;
    }

    SpatialBatchResults batchIntersects(cons[[maybe_unused]] t SpatialBatchInputs &[[maybe_unused]] in) override {
        SpatialBatchResults out;
        out.mask.resize(in.count, 0u);

        if (in.count == 0) {
            return out;
        }

        // When no geometry data is provided return zero mask immediately.
        if (in.geoms_a.size() < in.count || in.geoms_b.size() < in.count) {
            return out;
        }

        // Parallel processing using multiple threads; each thread owns a disjoint
        // index range of `out.mask` so no synchronisation is required on writes.
        // Guard against thread_count_ == 0 (should never happen given the constructor,
        // but protects against integer division-by-zero if invariant is violated).
        if (thread_count_ == 0) {
            THEMIS_WARN("CpuParallelBackend: thread_count_ is 0; falling back to single-threaded");
            for (size_t i = 0; i < in.count; ++i) {
                out.mask[i] = exactIntersects(in.geoms_a[i], in.geoms_b[i]) ? 1u : 0u;
            }
            return out;
        }
        const size_t batch_size = (in.count + thread_count_ - 1) / thread_count_;

        // Use std::async / std::future so we can detect long-running workers
        // and emit a diagnostic before blocking on join.
        static constexpr auto kWorkerTimeout = std::chrono::seconds(30);
        std::vector<std::future<void>> futures;
        futures.reserve(thread_count_);

        for (size_t t = 0; t < thread_count_; ++t) {
            const size_t start_idx = t * batch_size;
            const size_t end_idx   = std::min(start_idx + batch_size, in.count);

            if (start_idx >= in.count) {
                break;
            }

            futures.emplace_back(std::async(std::launch::async,
                [this, &in, &out, start_idx, end_idx]() {
                    for (size_t i = start_idx; i < end_idx; ++i) {
                        out.mask[i] = exactIntersects(in.geoms_a[i], in.geoms_b[i]) ? 1u : 0u;
                    }
                }));
        }

        for (auto &fut : futures) {
            if (fut.wait_for(kWorkerTimeout) != std::future_status::ready) {
                THEMIS_WARN("CpuParallelBackend: worker did not complete within {}s; "
                            "still waiting — possible infinite loop in exactIntersects",
                            kWorkerTimeout.count());
            }
            fut.get(); // synchronise and propagate any exceptions
        }

        return out;
    }

    bool exactIntersects(cons[[maybe_unused]] t GeometryInfo &[[maybe_unused]] geom1, cons[[maybe_unused]] t GeometryInfo &[[maybe_unused]] geom2) override {
        // Use MBR as fast pre-check
        auto mbr1 = geom1.computeMBR();
        auto mbr2 = geom2.computeMBR();

        if (!mbr1.intersects(mbr2)) {
            return false;
        }

        // For exact check, perform detailed geometry intersection
        if (geom1.isPoint() && geom2.isPolygon()) {
            return pointInPolygon(geom1.coords[0], geom2);
        } else if (geom1.isPolygon() && geom2.isPoint()) {
            return pointInPolygon(geom2.coords[0], geom1);
        } else if (geom1.isPolygon() && geom2.isPolygon()) {
            return polygonIntersectsPolygon(geom1, geom2);
        }

        // Fallback to MBR check for unsupported types
        return true;
    }

    // Delegate geometry-manipulation operations to the CPU exact backend.
    // Previously the default base-class implementations returned an empty
    // GeometryInfo; now they are properly wired to the CPU exact path.

    GeometryInfo stBuffer(cons[[maybe_unused]] t GeometryInfo &[[maybe_unused]] geom, doubl[[maybe_unused]] e distance_[[maybe_unused]] m, in[[maybe_unused]] t arc_point[[maybe_unused]] s = 36) override {
        auto *b = getCpuExactBackend();
        return b ? b->stBuffer(geom, distance_m, arc_points) : GeometryInfo{};
    }

    GeometryInfo stUnion(cons[[maybe_unused]] t GeometryInfo &[[maybe_unused]] g1, cons[[maybe_unused]] t GeometryInfo &[[maybe_unused]] g2) override {
        auto *b = getCpuExactBackend();
        return b ? b->stUnion(g1, g2) : GeometryInfo{};
    }

    GeometryInfo stDifference(cons[[maybe_unused]] t GeometryInfo &[[maybe_unused]] g1, cons[[maybe_unused]] t GeometryInfo &[[maybe_unused]] g2) override {
        auto *b = getCpuExactBackend();
        return b ? b->stDifference(g1, g2) : GeometryInfo{};
    }

    double geodesicDistance(doubl[[maybe_unused]] e la[[maybe_unused]] t1, doubl[[maybe_unused]] e lo[[maybe_unused]] n1, doubl[[maybe_unused]] e la[[maybe_unused]] t2, doubl[[maybe_unused]] e lo[[maybe_unused]] n2) const override {
        auto *b = getCpuExactBackend();
        return b ? b->geodesicDistance(lat1, lon1, lat2, lon2) : 0.0;
    }

  private:
    unsigned int thread_count_;

    // Point-in-polygon test using ray-casting algorithm
    bool pointInPolygon(const Coordinate &point, const GeometryInfo &polygon) const {
        const auto &ring = polygon.rings.empty() ? polygon.coords : polygon.rings[0];
        if (ring.size() < 3) {
            return false;
        }

        bool inside = false;
        size_t j    = ring.size() - 1;

        for (size_t i = 0; i < ring.size(); j = i++) {
            if (((ring[i].y > point.y) != (ring[j].y > point.y))
                && (point.x < (ring[j].x - ring[i].x) * (point.y - ring[i].y) / (ring[j].y - ring[i].y) + ring[i].x)) {
                inside = !inside;
            }
        }

        return inside;
    }

    // Simplified polygon-polygon intersection check
    bool polygonIntersectsPolygon(const GeometryInfo &poly1, const GeometryInfo &poly2) const {
        // Check if any vertex of poly1 is inside poly2
        const auto &ring1 = poly1.rings.empty() ? poly1.coords : poly1.rings[0];
        for (const auto &coord : ring1) {
            if (pointInPolygon(coord, poly2)) {
                return true;
            }
        }

        // Check if any vertex of poly2 is inside poly1
        const auto &ring2 = poly2.rings.empty() ? poly2.coords : poly2.rings[0];
        for (const auto &coord : ring2) {
            if (pointInPolygon(coord, poly1)) {
                return true;
            }
        }

        // Check for edge-edge intersections
        return checkEdgeIntersections(ring1, ring2);
    }

    bool checkEdgeIntersections(const std::vector<Coordinate> &ring1, const std::vector<Coordinate> &ring2) const {
        for (size_t i = 0, j = ring1.size() - 1; i < ring1.size(); j = i++) {
            for (size_t k = 0, l = ring2.size() - 1; k < ring2.size(); l = k++) {
                if (segmentsIntersect(ring1[j], ring1[i], ring2[l], ring2[k])) {
                    return true;
                }
            }
        }
        return false;
    }

    bool segmentsIntersect(const Coordinate &p1, const Coordinate &p2, const Coordinate &p3,
                           const Coordinate &p4) const {
        auto ccw = [](const Coordinate &A, const Coordinate &B, const Coordinate &C) {
            return (C.y - A.y) * (B.x - A.x) > (B.y - A.y) * (C.x - A.x);
        };

        return ccw(p1, p3, p4) != ccw(p2, p3, p4) && ccw(p1, p2, p3) != ccw(p1, p2, p4);
    }
};

#ifdef THEMIS_ENABLE_CUDA

// CUDA kernels for GPU-accelerated spatial operations
__device__ bool cuda_point_in_polygon(double px, double py, const double *ring_x, const double *ring_y, int ring_size) {
    bool inside = false;
    int j       = ring_size - 1;

    for (int i = 0; i < ring_size; j = i++) {
        if (((ring_y[i] > py) != (ring_y[j] > py))
            && (px < (ring_x[j] - ring_x[i]) * (py - ring_y[i]) / (ring_y[j] - ring_y[i]) + ring_x[i])) {
            inside = !inside;
        }
    }

    return inside;
}

__global__ void cuda_batch_intersects_kernel(const double *query_mbr, const double *candidate_mbrs, uint8_t *results,
                                             int count) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= count)
        return;

    // Load query MBR (minx, miny, maxx, maxy)
    double q_minx = query_mbr[0];
    double q_miny = query_mbr[1];
    double q_maxx = query_mbr[2];
    double q_maxy = query_mbr[3];

    // Load candidate MBR
    int offset    = idx * 4;
    double c_minx = candidate_mbrs[offset + 0];
    double c_miny = candidate_mbrs[offset + 1];
    double c_maxx = candidate_mbrs[offset + 2];
    double c_maxy = candidate_mbrs[offset + 3];

    // MBR intersection test
    bool intersects = !(q_minx > c_maxx || q_maxx < c_minx || q_miny > c_maxy || q_maxy < c_miny);

    results[idx] = intersects ? 1 : 0;
}

/// Pairwise MBR intersection: each thread tests one geometry pair (a[idx], b[idx]).
/// mbrs_a and mbrs_b are flat arrays: [minx, miny, maxx, maxy] per entry.
__global__ void cuda_pairwise_intersects_kernel(const double *mbrs_a, const double *mbrs_b, uint8_t *results,
                                                int count) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= count)
        return;

    const int off       = idx * 4;
    const double a_minx = mbrs_a[off];
    const double a_miny = mbrs_a[off + 1];
    const double a_maxx = mbrs_a[off + 2];
    const double a_maxy = mbrs_a[off + 3];

    const double b_minx = mbrs_b[off];
    const double b_miny = mbrs_b[off + 1];
    const double b_maxx = mbrs_b[off + 2];
    const double b_maxy = mbrs_b[off + 3];

    results[idx] = (a_minx <= b_maxx && a_maxx >= b_minx && a_miny <= b_maxy && a_maxy >= b_miny) ? 1u : 0u;
}

/// Batch ST_BUFFER kernel for Point geometries.
/// Each thread computes one vertex of one buffer polygon.
/// Layout: gridDim.x = n, blockDim.x = arc_points+1 (clamped to 1024).
/// ring_x/ring_y are flat [n * (arc_points+1)] arrays.
__global__ void cuda_batch_point_buffer_kernel(const double *lons, ///< [n] centre longitudes
                                               const double *lats, ///< [n] centre latitudes
                                               double *ring_x,     ///< [n * (arc_points+1)] output longitudes
                                               double *ring_y,     ///< [n * (arc_points+1)] output latitudes
                                               int arc_points,
                                               double d_lat, ///< angular radius in degrees (latitude)
                                               double d_lon, ///< angular radius in degrees (longitude)
                                               int n) {
    const int pt  = blockIdx.x;  // point index
    const int vtx = threadIdx.x; // vertex index within the polygon
    if (pt >= n || vtx > arc_points)
        return;

    const double lon   = lons[pt];
    const double lat   = lats[pt];
    const double angle = 2.0 * 3.14159265358979323846 * vtx / arc_points;
    const int base     = pt * (arc_points + 1);
    ring_x[base + vtx] = lon + d_lon * cos(angle);
    ring_y[base + vtx] = lat + d_lat * sin(angle);
}

class CudaBackend final : public ISpatialComputeBackend {
  public:
    CudaBackend() : device_id_(0) {
        int device_count = 0;
        cudaGetDeviceCount(&device_count);
        is_available_ = (device_count > 0);

        if (is_available_) {
            cudaSetDevice(device_id_);
            cudaDeviceProp props;
            cudaGetDeviceProperties(&props, device_id_);
            THEMIS_INFO("CUDA backend initialized on device: {}", props.name);
        }
    }

    ~CudaBackend() {
        // d_cached_mbrs_a_, d_cached_mbrs_b_, and d_cached_results_ are RAII
        // wrappers — their destructors call cudaFree automatically.
        cudaDeviceReset();
    }

    const char *name() const noexcept override {
        return "cuda_gpu";
    }

    bool isAvailable() const noexcept override {
        return is_available_;
    }

    // Two-phase exact batch intersection:
    //   Phase 1 — GPU MBR filter (conservative, no false negatives).
    //   Phase 2 — CPU exact verification for MBR-positive candidates only.
    // Device buffers are cached and grown on demand to amortise cudaMalloc cost.
    SpatialBatchResults batchIntersects(cons[[maybe_unused]] t SpatialBatchInputs &[[maybe_unused]] in) override {
        SpatialBatchResults out;
        out.mask.resize(in.count);

        if (!is_available_ || in.count == 0) {
            return out;
        }

        // When no geometry data is provided, return zero-filled mask.
        if (in.geoms_a.size() < in.count || in.geoms_b.size() < in.count) {
            return out;
        }

        const int n = static_cast<int>(in.count);

        // Build flat MBR arrays: [minx, miny, maxx, maxy] per geometry.
        std::vector<double> mbrs_a(static_cast<size_t>(n) * 4);
        std::vector<double> mbrs_b(static_cast<size_t>(n) * 4);
        for (int i = 0; i < n; ++i) {
            auto ma           = in.geoms_a[i].computeMBR();
            mbrs_a[i * 4 + 0] = ma.minx;
            mbrs_a[i * 4 + 1] = ma.miny;
            mbrs_a[i * 4 + 2] = ma.maxx;
            mbrs_a[i * 4 + 3] = ma.maxy;
            auto mb           = in.geoms_b[i].computeMBR();
            mbrs_b[i * 4 + 0] = mb.minx;
            mbrs_b[i * 4 + 1] = mb.miny;
            mbrs_b[i * 4 + 2] = mb.maxx;
            mbrs_b[i * 4 + 3] = mb.maxy;
        }

        // Grow cached device buffers if needed (amortises alloc cost).
        const size_t mbr_sz = static_cast<size_t>(n) * 4 * sizeof(double);
        const size_t res_sz = static_cast<size_t>(n) * sizeof(uint8_t);

        if (!ensureCachedBuffers(n, mbr_sz, res_sz)) {
            THEMIS_WARN("CUDA buffer cache failed, falling back to CPU-parallel");
            CpuParallelBackend cpu_fallback;
            return cpu_fallback.batchIntersects(in);
        }

        // Upload geometry MBRs to device memory.
        cudaError_t e;
        if ((e = cudaMemcpy(d_cached_mbrs_a_.get(), mbrs_a.data(), mbr_sz, cudaMemcpyHostToDevice)) != cudaSuccess
            || (e = cudaMemcpy(d_cached_mbrs_b_.get(), mbrs_b.data(), mbr_sz, cudaMemcpyHostToDevice)) != cudaSuccess
            || (e = cudaMemset(d_cached_results_.get(), 0, res_sz)) != cudaSuccess) {
            THEMIS_WARN("CUDA upload failed ({}), falling back to CPU-parallel", static_cast<int>(e));
            CpuParallelBackend cpu_fallback;
            return cpu_fallback.batchIntersects(in);
        }

        // Phase 1: dispatch pairwise MBR intersection kernel.
        // n must be positive and fit in an int before computing the grid size.
        if (n <= 0 || static_cast<size_t>(n) > static_cast<size_t>(INT_MAX)) {
            THEMIS_WARN("CudaBackend: n={} out of valid range, falling back to CPU-parallel", n);
            CpuParallelBackend cpu_fallback;
            return cpu_fallback.batchIntersects(in);
        }
        const int blockSize = 256;
        const int gridSize  = (n + blockSize - 1) / blockSize;
        cuda_pairwise_intersects_kernel<<<gridSize, blockSize>>>(d_cached_mbrs_a_.get(), d_cached_mbrs_b_.get(), d_cached_results_.get(),
                                                                 n);

        e = cudaDeviceSynchronize();
        if (e == cudaSuccess) {
            e = cudaMemcpy(out.mask.data(), d_cached_results_.get(), res_sz, cudaMemcpyDeviceToHost);
        }

        if (e != cudaSuccess) {
            THEMIS_WARN("CUDA execution failed ({}), falling back to CPU-parallel", static_cast<int>(e));
            CpuParallelBackend cpu_fallback;
            return cpu_fallback.batchIntersects(in);
        }

        // Phase 2: CPU exact verification for MBR-positive candidates.
        // Eliminates false positives from the conservative MBR filter.
        // Build a sub-batch of only the candidates to leverage parallel threading.
        SpatialBatchInputs candidates;
        std::vector<size_t> candidate_indices;
        for (int i = 0; i < n; ++i) {
            if (out.mask[i]) {
                candidate_indices.push_back(static_cast<size_t>(i));
                candidates.geoms_a.push_back(in.geoms_a[i]);
                candidates.geoms_b.push_back(in.geoms_b[i]);
            }
        }
        if (!candidate_indices.empty()) {
            candidates.count   = candidate_indices.size();
            auto exact_results = cpu_exact_.batchIntersects(candidates);
            for (size_t j = 0; j < candidate_indices.size(); ++j) {
                out.mask[candidate_indices[j]] = (j < exact_results.mask.size()) ? exact_results.mask[j] : 0u;
            }
        }

        return out;
    }

    bool exactIntersects(cons[[maybe_unused]] t GeometryInfo &[[maybe_unused]] geom1, cons[[maybe_unused]] t GeometryInfo &[[maybe_unused]] geom2) override {
        // For single geometry checks, CPU is often faster due to transfer overhead
        return cpu_exact_.exactIntersects(geom1, geom2);
    }

    /// GPU-accelerated ST_BUFFER for Point geometries using the batch kernel.
    /// Falls back to cpu_exact_ for non-Point types or on any CUDA error.
    GeometryInfo stBuffer(cons[[maybe_unused]] t GeometryInfo &[[maybe_unused]] geom, doubl[[maybe_unused]] e distance_[[maybe_unused]] m, in[[maybe_unused]] t arc_point[[maybe_unused]] s = 36) override {
        if (!is_available_ || !geom.isPoint() || geom.coords.empty() || distance_m <= 0.0 || arc_points < 3) {
            return cpu_exact_.stBuffer(geom, distance_m, arc_points);
        }

        const double lon          = geom.coords[0].x;
        const double lat          = geom.coords[0].y;
        constexpr double kPiLocal = 3.14159265358979323846;
        const double lat_rad      = lat * kPiLocal / 180.0;
        const double cos_lat      = std::cos(lat_rad);
        const double d_lat        = distance_m / 111320.0;
        const double d_lon        = distance_m / (111320.0 * (cos_lat > 1e-6 ? cos_lat : 1e-6));

        const int    n_verts = arc_points + 1; // +1 to close the ring
        const size_t buf_sz  = static_cast<size_t>(n_verts) * sizeof(double);

        // RAII guards own all device allocations.
        // Any early return frees all allocated device buffers automatically.
        CudaTypedBuffer<double> d_lon_in, d_lat_in, d_ring_x, d_ring_y;

        double h_lon = lon, h_lat = lat;

        if (d_lon_in.alloc(1) != cudaSuccess)
            return cpu_exact_.stBuffer(geom, distance_m, arc_points);
        if (d_lat_in.alloc(1) != cudaSuccess)
            return cpu_exact_.stBuffer(geom, distance_m, arc_points);
        if (d_ring_x.alloc(static_cast<size_t>(n_verts)) != cudaSuccess)
            return cpu_exact_.stBuffer(geom, distance_m, arc_points);
        if (d_ring_y.alloc(static_cast<size_t>(n_verts)) != cudaSuccess)
            return cpu_exact_.stBuffer(geom, distance_m, arc_points);

        cudaError_t e;
        e = cudaMemcpy(d_lon_in.get(), &h_lon, sizeof(double), cudaMemcpyHostToDevice);
        if (e != cudaSuccess)
            return cpu_exact_.stBuffer(geom, distance_m, arc_points);
        e = cudaMemcpy(d_lat_in.get(), &h_lat, sizeof(double), cudaMemcpyHostToDevice);
        if (e != cudaSuccess)
            return cpu_exact_.stBuffer(geom, distance_m, arc_points);

        cuda_batch_point_buffer_kernel<<<1, n_verts>>>(
            d_lon_in.get(), d_lat_in.get(), d_ring_x.get(), d_ring_y.get(),
            arc_points, d_lat, d_lon, 1);

        e = cudaDeviceSynchronize();
        if (e != cudaSuccess)
            return cpu_exact_.stBuffer(geom, distance_m, arc_points);

        std::vector<double> h_ring_x(static_cast<size_t>(n_verts));
        std::vector<double> h_ring_y(static_cast<size_t>(n_verts));
        e = cudaMemcpy(h_ring_x.data(), d_ring_x.get(), buf_sz, cudaMemcpyDeviceToHost);
        if (e != cudaSuccess)
            return cpu_exact_.stBuffer(geom, distance_m, arc_points);
        e = cudaMemcpy(h_ring_y.data(), d_ring_y.get(), buf_sz, cudaMemcpyDeviceToHost);
        if (e != cudaSuccess)
            return cpu_exact_.stBuffer(geom, distance_m, arc_points);

        // RAII destructors free all device buffers on scope exit.
        GeometryInfo result(GeometryType::Polygon);
        std::vector<Coordinate> ring;
        ring.reserve(static_cast<size_t>(n_verts));
        for (int i = 0; i < n_verts; ++i) {
            ring.emplace_back(h_ring_x[static_cast<size_t>(i)], h_ring_y[static_cast<size_t>(i)]);
        }
        result.rings.push_back(std::move(ring));
        return result;
    }

    GeometryInfo stUnion(cons[[maybe_unused]] t GeometryInfo &[[maybe_unused]] g1, cons[[maybe_unused]] t GeometryInfo &[[maybe_unused]] g2) override {
        return cpu_exact_.stUnion(g1, g2);
    }

    GeometryInfo stDifference(cons[[maybe_unused]] t GeometryInfo &[[maybe_unused]] g1, cons[[maybe_unused]] t GeometryInfo &[[maybe_unused]] g2) override {
        return cpu_exact_.stDifference(g1, g2);
    }

    double geodesicDistance(doubl[[maybe_unused]] e la[[maybe_unused]] t1, doubl[[maybe_unused]] e lo[[maybe_unused]] n1, doubl[[maybe_unused]] e la[[maybe_unused]] t2, doubl[[maybe_unused]] e lo[[maybe_unused]] n2) const override {
        return cpu_exact_.geodesicDistance(lat1, lon1, lat2, lon2);
    }

  private:
    int device_id_;
    bool is_available_;
    CpuParallelBackend cpu_exact_; // reused across calls for Phase 2 verification

    // Cached device buffers — grown on demand, freed automatically via RAII.
    int cached_n_ = 0;
    CudaTypedBuffer<double>  d_cached_mbrs_a_;
    CudaTypedBuffer<double>  d_cached_mbrs_b_;
    CudaTypedBuffer<uint8_t> d_cached_results_;

    /// Ensure the cached device buffers are large enough for `n` pairs.
    /// Returns false on allocation failure (caller falls back to CPU).
    bool ensureCachedBuffers(int n, size_t mbr_sz, size_t res_sz) {
        if (n <= cached_n_)
            return true; // already large enough
        // Release current allocations before growing (RAII handles the free).
        d_cached_mbrs_a_.free();
        d_cached_mbrs_b_.free();
        d_cached_results_.free();
        cached_n_ = 0;
        cudaError_t e;
        if ((e = d_cached_mbrs_a_.alloc(mbr_sz / sizeof(double))) != cudaSuccess) {
            THEMIS_WARN("CUDA cudaMalloc failed for d_cached_mbrs_a_ ({})", static_cast<int>(e));
            return false;
        }
        if ((e = d_cached_mbrs_b_.alloc(mbr_sz / sizeof(double))) != cudaSuccess) {
            THEMIS_WARN("CUDA cudaMalloc failed for d_cached_mbrs_b_ ({})", static_cast<int>(e));
            d_cached_mbrs_a_.free();
            return false;
        }
        if ([[maybe_unused]] (e = d_cached_results_.alloc(res_sz / sizeof(uint8_t))) != cudaSuccess) {
            THEMIS_WARN("CUDA cudaMalloc failed for d_cached_results_ ({})", static_cast<int>(e));
            d_cached_mbrs_a_.free();
            d_cached_mbrs_b_.free();
            return false;
        }
        cached_n_ = n;
        return true;
    }
};

#endif // THEMIS_ENABLE_CUDA

#ifdef THEMIS_ENABLE_OPENCL

/// OpenCL kernel source for pairwise MBR intersection.
/// Requires cl_khr_fp64 for double-precision coordinates.
static const char *kOpenCLGeoIntersectsKernelSrc = R"(
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

__kernel void pairwise_mbr_intersects(
    __global const double* mbrs_a,
    __global const double* mbrs_b,
    __global uchar* results,
    const int count)
{
    int idx = get_global_id(0);
    if (idx >= count) {
      return;
    }
    int off = idx * 4;
    double a_minx = mbrs_a[off];
    double a_miny = mbrs_a[off + 1];
    double a_maxx = mbrs_a[off + 2];
    double a_maxy = mbrs_a[off + 3];
    double b_minx = mbrs_b[off];
    double b_miny = mbrs_b[off + 1];
    double b_maxx = mbrs_b[off + 2];
    double b_maxy = mbrs_b[off + 3];
    results[idx] = (a_minx <= b_maxx && a_maxx >= b_minx &&
                    a_miny <= b_maxy && a_maxy >= b_miny) ? 1 : 0;
}
)";

class OpenCLBackend final : public ISpatialComputeBackend {
  public:
    OpenCLBackend() : context_(nullptr), queue_(nullptr), program_(nullptr) {
        cl_int err;

        // Get platform
        cl_platform_id platform;
        err = clGetPlatformIDs(1, &platform, nullptr);
        if (err != CL_SUCCESS) {
            THEMIS_WARN("OpenCL platform not available");
            return;
        }

        // Get device (prefer GPU)
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device_, nullptr);
        if (err != CL_SUCCESS) {
            // Fall back to CPU
            err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device_, nullptr);
            if (err != CL_SUCCESS) {
                THEMIS_WARN("OpenCL device not available");
                return;
            }
        }

        // Create context
        context_ = clCreateContext(nullptr, 1, &device_, nullptr, nullptr, &err);
        if (err != CL_SUCCESS) {
            THEMIS_WARN("OpenCL context creation failed");
            return;
        }

        // Create command queue using the OpenCL 2.0+ API.
        // clCreateCommandQueue is deprecated since OpenCL 2.0; use
        // clCreateCommandQueueWithProperties with a null-terminated properties array.
        cl_queue_properties queue_props[] = {0};
        queue_ = clCreateCommandQueueWithProperties(context_, device_, queue_props, &err);
        if (err != CL_SUCCESS) {
            THEMIS_WARN("OpenCL command queue creation failed");
            clReleaseContext(context_);
            context_ = nullptr;
            return;
        }

        // Compile geo intersection kernels at startup.
        if (!compileKernels()) {
            THEMIS_WARN("OpenCL kernel compilation failed — OpenCL backend disabled");
            clReleaseCommandQueue(queue_);
            queue_ = nullptr;
            clReleaseContext(context_);
            context_ = nullptr;
            return;
        }

        is_available_ = true;
        THEMIS_INFO("OpenCL backend initialized");
    }

    ~OpenCLBackend() {
        if (program_)
            clReleaseProgram(program_);
        if (queue_)
            clReleaseCommandQueue(queue_);
        if (context_)
            clReleaseContext(context_);
    }

    const char *name() const noexcept override {
        return "opencl_gpu";
    }

    bool isAvailable() const noexcept override {
        return is_available_;
    }

    SpatialBatchResults batchIntersects(cons[[maybe_unused]] t SpatialBatchInputs &[[maybe_unused]] in) override {
        SpatialBatchResults out;
        out.mask.resize(in.count);

        if (!is_available_ || in.count == 0) {
            return out;
        }

        // When no geometry data is provided, return zero-filled mask.
        if (in.geoms_a.size() < in.count || in.geoms_b.size() < in.count) {
            return out;
        }

        const int n = static_cast<int>(in.count);

        // Build flat MBR arrays: [minx, miny, maxx, maxy] per geometry.
        std::vector<double> mbrs_a(static_cast<size_t>(n) * 4);
        std::vector<double> mbrs_b(static_cast<size_t>(n) * 4);
        for (int i = 0; i < n; ++i) {
            auto ma           = in.geoms_a[i].computeMBR();
            mbrs_a[i * 4 + 0] = ma.minx;
            mbrs_a[i * 4 + 1] = ma.miny;
            mbrs_a[i * 4 + 2] = ma.maxx;
            mbrs_a[i * 4 + 3] = ma.maxy;
            auto mb           = in.geoms_b[i].computeMBR();
            mbrs_b[i * 4 + 0] = mb.minx;
            mbrs_b[i * 4 + 1] = mb.miny;
            mbrs_b[i * 4 + 2] = mb.maxx;
            mbrs_b[i * 4 + 3] = mb.maxy;
        }

        const size_t mbr_sz = static_cast<size_t>(n) * 4 * sizeof(double);
        const size_t res_sz = static_cast<size_t>(n) * sizeof(uint8_t);

        // Create device buffers and upload host data.
        cl_int err;
        cl_mem d_mbrs_a
            = clCreateBuffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, mbr_sz, mbrs_a.data(), &err);
        if (err != CL_SUCCESS) {
            THEMIS_WARN("OpenCL buffer creation failed (mbrs_a), falling back to CPU");
            CpuParallelBackend cpu_fallback;
            return cpu_fallback.batchIntersects(in);
        }
        cl_mem d_mbrs_b
            = clCreateBuffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, mbr_sz, mbrs_b.data(), &err);
        if (err != CL_SUCCESS) {
            clReleaseMemObject(d_mbrs_a);
            THEMIS_WARN("OpenCL buffer creation failed (mbrs_b), falling back to CPU");
            CpuParallelBackend cpu_fallback;
            return cpu_fallback.batchIntersects(in);
        }
        cl_mem d_results = clCreateBuffer(context_, CL_MEM_WRITE_ONLY, res_sz, nullptr, &err);
        if (err != CL_SUCCESS) {
            clReleaseMemObject(d_mbrs_a);
            clReleaseMemObject(d_mbrs_b);
            THEMIS_WARN("OpenCL buffer creation failed (results), falling back to CPU");
            CpuParallelBackend cpu_fallback;
            return cpu_fallback.batchIntersects(in);
        }

        // Create and configure kernel instance.
        cl_kernel kernel = clCreateKernel(program_, "pairwise_mbr_intersects", &err);
        if (err != CL_SUCCESS) {
            clReleaseMemObject(d_mbrs_a);
            clReleaseMemObject(d_mbrs_b);
            clReleaseMemObject(d_results);
            THEMIS_WARN("OpenCL kernel creation failed, falling back to CPU");
            CpuParallelBackend cpu_fallback;
            return cpu_fallback.batchIntersects(in);
        }

        err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_mbrs_a);
        if (err != CL_SUCCESS) {
            clReleaseKernel(kernel);
            clReleaseMemObject(d_mbrs_a);
            clReleaseMemObject(d_mbrs_b);
            clReleaseMemObject(d_results);
            THEMIS_WARN("OpenCL clSetKernelArg failed for arg 0, falling back to CPU");
            CpuParallelBackend cpu_fallback;
            return cpu_fallback.batchIntersects(in);
        }
        err = clSetKernelArg(kernel, 1, sizeof(cl_mem), &d_mbrs_b);
        if (err != CL_SUCCESS) {
            clReleaseKernel(kernel);
            clReleaseMemObject(d_mbrs_a);
            clReleaseMemObject(d_mbrs_b);
            clReleaseMemObject(d_results);
            THEMIS_WARN("OpenCL clSetKernelArg failed for arg 1, falling back to CPU");
            CpuParallelBackend cpu_fallback;
            return cpu_fallback.batchIntersects(in);
        }
        err = clSetKernelArg(kernel, 2, sizeof(cl_mem), &d_results);
        if (err != CL_SUCCESS) {
            clReleaseKernel(kernel);
            clReleaseMemObject(d_mbrs_a);
            clReleaseMemObject(d_mbrs_b);
            clReleaseMemObject(d_results);
            THEMIS_WARN("OpenCL clSetKernelArg failed for arg 2, falling back to CPU");
            CpuParallelBackend cpu_fallback;
            return cpu_fallback.batchIntersects(in);
        }
        err = clSetKernelArg(kernel, 3, sizeof(cl_int), &n);
        if (err != CL_SUCCESS) {
            clReleaseKernel(kernel);
            clReleaseMemObject(d_mbrs_a);
            clReleaseMemObject(d_mbrs_b);
            clReleaseMemObject(d_results);
            THEMIS_WARN("OpenCL clSetKernelArg failed for arg 3, falling back to CPU");
            CpuParallelBackend cpu_fallback;
            return cpu_fallback.batchIntersects(in);
        }

        // Phase 1: enqueue NDRange kernel — one work-item per geometry pair.
        const size_t global_work_size = static_cast<size_t>(n);
        err     = clEnqueueNDRangeKernel(queue_, kernel, 1, nullptr, &global_work_size, nullptr, 0, nullptr, nullptr);
        bool ok = (err == CL_SUCCESS);
        if (ok) {
            clFinish(queue_);
            err = clEnqueueReadBuffer(queue_, d_results, CL_TRUE, 0, res_sz, out.mask.data(), 0, nullptr, nullptr);
            ok  = (err == CL_SUCCESS);
        }

        clReleaseKernel(kernel);
        clReleaseMemObject(d_mbrs_a);
        clReleaseMemObject(d_mbrs_b);
        clReleaseMemObject(d_results);

        if (!ok) {
            THEMIS_WARN("OpenCL execution failed, falling back to CPU-parallel");
            CpuParallelBackend cpu_fallback;
            return cpu_fallback.batchIntersects(in);
        }

        // Phase 2: CPU exact verification for MBR-positive candidates.
        // Eliminates false positives from the conservative MBR filter.
        // Build a sub-batch of only the candidates to leverage parallel threading.
        SpatialBatchInputs candidates;
        std::vector<size_t> candidate_indices;
        for (int i = 0; i < n; ++i) {
            if (out.mask[i]) {
                candidate_indices.push_back(static_cast<size_t>(i));
                candidates.geoms_a.push_back(in.geoms_a[i]);
                candidates.geoms_b.push_back(in.geoms_b[i]);
            }
        }
        if (!candidate_indices.empty()) {
            candidates.count   = candidate_indices.size();
            auto exact_results = cpu_exact_.batchIntersects(candidates);
            for (size_t j = 0; j < candidate_indices.size(); ++j) {
                out.mask[candidate_indices[j]] = (j < exact_results.mask.size()) ? exact_results.mask[j] : 0u;
            }
        }

        return out;
    }

    bool exactIntersects(cons[[maybe_unused]] t GeometryInfo &[[maybe_unused]] geom1, cons[[maybe_unused]] t GeometryInfo &[[maybe_unused]] geom2) override {
        return cpu_exact_.exactIntersects(geom1, geom2);
    }

    GeometryInfo stBuffer(cons[[maybe_unused]] t GeometryInfo &[[maybe_unused]] geom, doubl[[maybe_unused]] e distance_[[maybe_unused]] m, in[[maybe_unused]] t arc_point[[maybe_unused]] s = 36) override {
        return cpu_exact_.stBuffer(geom, distance_m, arc_points);
    }

    GeometryInfo stUnion(cons[[maybe_unused]] t GeometryInfo &[[maybe_unused]] g1, cons[[maybe_unused]] t GeometryInfo &[[maybe_unused]] g2) override {
        return cpu_exact_.stUnion(g1, g2);
    }

    GeometryInfo stDifference(cons[[maybe_unused]] t GeometryInfo &[[maybe_unused]] g1, cons[[maybe_unused]] t GeometryInfo &[[maybe_unused]] g2) override {
        return cpu_exact_.stDifference(g1, g2);
    }

    double geodesicDistance(doubl[[maybe_unused]] e la[[maybe_unused]] t1, doubl[[maybe_unused]] e lo[[maybe_unused]] n1, doubl[[maybe_unused]] e la[[maybe_unused]] t2, doubl[[maybe_unused]] e lo[[maybe_unused]] n2) const override {
        return cpu_exact_.geodesicDistance(lat1, lon1, lat2, lon2);
    }

  private:
    cl_device_id device_;
    cl_context context_;
    cl_command_queue queue_;
    cl_program program_;
    bool is_available_ = false;
    CpuParallelBackend cpu_exact_; // reused across calls for Phase 2 verification

    /// Compile geo intersection kernels from source; called once in the constructor.
    bool compileKernels() {
        cl_int err;
        program_ = clCreateProgramWithSource(context_, 1, &kOpenCLGeoIntersectsKernelSrc, nullptr, &err);
        if (err != CL_SUCCESS) {
            THEMIS_WARN("OpenCL clCreateProgramWithSource failed ({})", static_cast<int>(err));
            return false;
        }
        err = clBuildProgram(program_, 1, &device_, nullptr, nullptr, nullptr);
        if (err != CL_SUCCESS) {
            size_t log_sz = 0;
            clGetProgramBuildInfo(program_, device_, CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_sz);
            std::string log;
            if (log_sz > 0) {
                log.assign(log_sz, '\0');
                clGetProgramBuildInfo(program_, device_, CL_PROGRAM_BUILD_LOG, log_sz, &log[0], nullptr);
            }
            THEMIS_WARN("OpenCL kernel build failed: {}", log);
            clReleaseProgram(program_);
            program_ = nullptr;
            return false;
        }
        return true;
    }
};

#endif // THEMIS_ENABLE_OPENCL

// Production GPU backend with automatic fallback
/** @brief Production GPU backend with automatic fallback. */
class ProductionGpuBackend final : public ISpatialComputeBackend {
  public:
    ProductionGpuBackend() {
// Try to initialize backends in order of preference
#ifdef THEMIS_ENABLE_CUDA
        cuda_backend_ = std::make_unique<CudaBackend>();
        if (cuda_backend_->isAvailable()) {
            active_backend_ = cuda_backend_.get();
            THEMIS_INFO("Using CUDA backend for GPU acceleration");
            return;
        }
#endif

#ifdef THEMIS_ENABLE_OPENCL
        opencl_backend_ = std::make_unique<OpenCLBackend>();
        if (opencl_backend_->isAvailable()) {
            active_backend_ = opencl_backend_.get();
            THEMIS_INFO("Using OpenCL backend for GPU acceleration");
            return;
        }
#endif

        // Fall back to CPU-parallel
        cpu_backend_    = std::make_unique<CpuParallelBackend>();
        active_backend_ = cpu_backend_.get();
        THEMIS_INFO("Using CPU-parallel backend (GPU not available)");
    }

    const char *name() const noexcept override {
        return active_backend_ ? active_backend_->name() : "none";
    }

    bool isAvailable() const noexcept override {
        return active_backend_ != nullptr;
    }

    SpatialBatchResults batchIntersects(cons[[maybe_unused]] t SpatialBatchInputs &[[maybe_unused]] in) override {
        if (active_backend_) {
            return active_backend_->batchIntersects(in);
        }

        SpatialBatchResults out;
        out.mask.assign(in.count, 0u);
        return out;
    }

    bool exactIntersects(cons[[maybe_unused]] t GeometryInfo &[[maybe_unused]] geom1, cons[[maybe_unused]] t GeometryInfo &[[maybe_unused]] geom2) override {
        if (active_backend_) {
            return active_backend_->exactIntersects(geom1, geom2);
        }
        auto mbr1 = geom1.computeMBR();
        auto mbr2 = geom2.computeMBR();
        return mbr1.intersects(mbr2);
    }

    GeometryInfo stBuffer(cons[[maybe_unused]] t GeometryInfo &[[maybe_unused]] geom, doubl[[maybe_unused]] e distance_[[maybe_unused]] m, in[[maybe_unused]] t arc_point[[maybe_unused]] s = 36) override {
        return active_backend_ ? active_backend_->stBuffer(geom, distance_m, arc_points) : GeometryInfo{};
    }

    GeometryInfo stUnion(cons[[maybe_unused]] t GeometryInfo &[[maybe_unused]] g1, cons[[maybe_unused]] t GeometryInfo &[[maybe_unused]] g2) override {
        return active_backend_ ? active_backend_->stUnion(g1, g2) : GeometryInfo{};
    }

    GeometryInfo stDifference(cons[[maybe_unused]] t GeometryInfo &[[maybe_unused]] g1, cons[[maybe_unused]] t GeometryInfo &[[maybe_unused]] g2) override {
        return active_backend_ ? active_backend_->stDifference(g1, g2) : GeometryInfo{};
    }

    double geodesicDistance(doubl[[maybe_unused]] e la[[maybe_unused]] t1, doubl[[maybe_unused]] e lo[[maybe_unused]] n1, doubl[[maybe_unused]] e la[[maybe_unused]] t2, doubl[[maybe_unused]] e lo[[maybe_unused]] n2) const override {
        return active_backend_ ? active_backend_->geodesicDistance(lat1, lon1, lat2, lon2) : 0.0;
    }

  private:
#ifdef THEMIS_ENABLE_CUDA
    std::unique_ptr<CudaBackend> cuda_backend_;
#endif

#ifdef THEMIS_ENABLE_OPENCL
    std::unique_ptr<OpenCLBackend> opencl_backend_;
#endif

    std::unique_ptr<CpuParallelBackend> cpu_backend_;
    ISpatialComputeBackend *active_backend_ = nullptr;
};

// Global production backend instance
static std::unique_ptr<ProductionGpuBackend> g_production_backend;

/// Lightweight proxy registered in the GeoBackendRegistry so the production
/// GPU backend is discoverable at runtime without creating a second GPU instance.
class ProductionGpuRegistryProxy final : public ISpatialComputeBackend {
  public:
    const char *name() const noexcept override {
        return "production_gpu";
    }
    bool isAvailable() const noexcept override {
        auto *b = getProductionGpuBackend();
        return b && b->isAvailable();
    }
    SpatialBatchResults batchIntersects(cons[[maybe_unused]] t SpatialBatchInputs &[[maybe_unused]] in) override {
        auto *b = getProductionGpuBackend();
        if (b) {
            return b->batchIntersects(in);
        }
        SpatialBatchResults out;
        out.mask.assign(in.count, 0u);
        return out;
    }
    bool exactIntersects(cons[[maybe_unused]] t GeometryInfo &[[maybe_unused]] g1, cons[[maybe_unused]] t GeometryInfo &[[maybe_unused]] g2) override {
        auto *b = getProductionGpuBackend();
        return b ? b->exactIntersects(g1, g2) : false;
    }
};

static void register_production_backend() {
    static std::once_flag s_once;
    std::call_once(s_once, []() {
        g_production_backend = std::make_unique<ProductionGpuBackend>();
        // Register in the global geo backend registry for runtime discoverability.
        if (auto *reg = getGeoBackendRegistry()) {
            reg->registerBackend(std::make_unique<ProductionGpuRegistryProxy>());
        }
    });
}

// Auto-register on module load
static int s_production_backend_anchor = (register_production_backend(), 0);

// Public API to get the production backend
ISpatialComputeBackend *getProductionGpuBackend() {
    return g_production_backend.get();
}

} // namespace geo
} // namespace themis
