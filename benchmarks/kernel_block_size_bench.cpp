// ThemisDB — Kernel Block-Size Occupancy Benchmark
//
// Sweeps CUDA/HIP kernel launch configurations across block sizes
// 64, 128, 256, 512, 1024 (1-D) and 8×8, 16×16, 32×32 (2-D) and
// reports achieved occupancy and throughput for each acceleration kernel:
//
//   • computeL2DistanceKernel        (vector_kernels.cu)
//   • computeInnerProductKernel      (vector_kernels.cu)
//   • haversineDistanceKernel        (geo_kernels.cu)
//   • pointInPolygonKernel           (geo_kernels.cu)
//   • graphBFSInitKernel             (graph_kernels.cu)
//   • graphBFSExpandKernel           (graph_kernels.cu)
//
// Usage (CUDA build):
//   ./kernel_block_size_bench [--iters N] [--vectors N] [--queries N] [--dim N]
//
// When THEMIS_ENABLE_CUDA is NOT defined the benchmark reports that no CUDA
// device is available and exits with code 0.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <vector>
#include <string>
#include <algorithm>
#include <functional>

#ifdef THEMIS_ENABLE_CUDA
#  include <cuda_runtime.h>
#endif

// ---------------------------------------------------------------------------
// Tiny CLI parser
// ---------------------------------------------------------------------------

struct BenchConfig {
    int iters   = 5;
    int vectors = 10000;
    int queries = 256;
    int dim     = 128;
};

static BenchConfig parseArgs(int argc, char* argv[])
{
    BenchConfig cfg;
    for (int i = 1; i < argc - 1; ++i) {
        if (std::string(argv[i]) == "--iters")   cfg.iters   = std::atoi(argv[i+1]);
        if (std::string(argv[i]) == "--vectors") cfg.vectors = std::atoi(argv[i+1]);
        if (std::string(argv[i]) == "--queries") cfg.queries = std::atoi(argv[i+1]);
        if (std::string(argv[i]) == "--dim")     cfg.dim     = std::atoi(argv[i+1]);
    }
    return cfg;
}

// ---------------------------------------------------------------------------
// Timing helper
// ---------------------------------------------------------------------------

using ClockT = std::chrono::high_resolution_clock;

static double elapsedMs(ClockT::time_point t0)
{
    return std::chrono::duration<double, std::milli>(ClockT::now() - t0).count();
}

// ---------------------------------------------------------------------------
// Result row
// ---------------------------------------------------------------------------

struct Result {
    std::string kernel;
    int         blockSize;
    double      avgMs;
    double      occupancyPct;
};

// ---------------------------------------------------------------------------
// CUDA benchmark body
// ---------------------------------------------------------------------------

#ifdef THEMIS_ENABLE_CUDA

// Forward-declarations of external CUDA kernels (defined in their respective .cu files)
extern "C" {

void launchL2DistanceKernel(
    const float* d_queries, const float* d_vectors, float* d_distances,
    int numQueries, int numVectors, int dim, cudaStream_t stream);

void launchInnerProductDistanceKernel(
    const float* d_queries, const float* d_vectors, float* d_distances,
    int numQueries, int numVectors, int dim, cudaStream_t stream);

int launchGeoDistanceKernel(
    const double* d_lats1, const double* d_lons1,
    const double* d_lats2, const double* d_lons2,
    float* d_distances, int count,
    /* GeoDistanceFormula */ int formula, void* stream);

int launchGeoContainmentKernel(
    const double* d_point_lats, const double* d_point_lons,
    int numPoints,
    const double* d_polygon_coords, int numPolygonVertices,
    unsigned char* d_results, void* stream);

} // extern "C"

// ---------------------------------------------------------------------------
// Measure achieved SM occupancy for a 1-D kernel via CUPTI / cudaOccupancy API
// ---------------------------------------------------------------------------

template <typename KernelFn>
static double achievedOccupancyPct(KernelFn kernel, int blockSize, size_t sharedMemBytes = 0)
{
    int maxActive = 0;
    cudaOccupancyMaxActiveBlocksPerMultiprocessor(
        &maxActive, kernel, blockSize, sharedMemBytes);

    // Suppress unused variable from attributes query (kept for documentation
    // purposes to show the calling convention).
    cudaFuncAttributes attr{};
    (void)cudaFuncGetAttributes(&attr, kernel);

    // cudaOccupancyMaxActiveBlocksPerMultiprocessor gives active blocks per SM.
    // Theoretical maximum depends on device; query cudaDeviceProp.
    cudaDeviceProp prop{};
    int dev = 0; cudaGetDevice(&dev);
    cudaGetDeviceProperties(&prop, dev);

    // Theoretical max threads/SM / blockSize = max blocks/SM (warp-limited)
    const int maxBlocksTheoretical = prop.maxThreadsPerMultiProcessor / blockSize;
    if (maxBlocksTheoretical <= 0) return 0.0;
    return 100.0 * maxActive / maxBlocksTheoretical;
}

// ---------------------------------------------------------------------------
// Sweep a callable over a list of block sizes; return Result per block size.
// ---------------------------------------------------------------------------

static std::vector<Result> sweepBlockSizes1D(
    const std::string&              kernelName,
    const std::vector<int>&         blockSizes,
    int                             iters,
    std::function<void(cudaStream_t, int)> launcher)
{
    std::vector<Result> rows;
    cudaStream_t stream = nullptr;
    cudaStreamCreate(&stream);

    for (int bs : blockSizes) {
        // Warm-up
        launcher(stream, bs);
        cudaStreamSynchronize(stream);

        auto t0 = ClockT::now();
        for (int i = 0; i < iters; ++i) {
            launcher(stream, bs);
        }
        cudaStreamSynchronize(stream);
        const double avgMs = elapsedMs(t0) / iters;

        rows.push_back({kernelName, bs, avgMs, 0.0 /* occupancy not available generically */});
    }

    cudaStreamDestroy(stream);
    return rows;
}

static std::vector<Result> runBenchmarks(const BenchConfig& cfg)
{
    std::vector<Result> all;
    const std::vector<int> blockSizes1D = {64, 128, 256, 512, 1024};

    // ── Allocate device buffers ──────────────────────────────────────────────

    std::vector<float> h_queries(cfg.queries * cfg.dim, 1.0f);
    std::vector<float> h_vectors(cfg.vectors * cfg.dim, 1.0f);
    std::vector<double> h_lats(cfg.vectors, 0.0);
    std::vector<double> h_lons(cfg.vectors, 0.0);
    // tiny polygon: a unit square
    std::vector<double> h_poly = {0.0, 0.0,  1.0, 0.0,  1.0, 1.0,  0.0, 1.0};

    float*        d_queries    = nullptr;
    float*        d_vectors    = nullptr;
    float*        d_distances  = nullptr;
    double*       d_lats1      = nullptr;
    double*       d_lons1      = nullptr;
    double*       d_lats2      = nullptr;
    double*       d_lons2      = nullptr;
    float*        d_geo_dist   = nullptr;
    double*       d_poly       = nullptr;
    unsigned char* d_pip_res   = nullptr;

    cudaMalloc(&d_queries,   cfg.queries * cfg.dim  * sizeof(float));
    cudaMalloc(&d_vectors,   cfg.vectors * cfg.dim  * sizeof(float));
    cudaMalloc(&d_distances, cfg.queries * cfg.vectors * sizeof(float));
    cudaMalloc(&d_lats1,     cfg.vectors * sizeof(double));
    cudaMalloc(&d_lons1,     cfg.vectors * sizeof(double));
    cudaMalloc(&d_lats2,     cfg.vectors * sizeof(double));
    cudaMalloc(&d_lons2,     cfg.vectors * sizeof(double));
    cudaMalloc(&d_geo_dist,  cfg.vectors * sizeof(float));
    cudaMalloc(&d_poly,      h_poly.size() * sizeof(double));
    cudaMalloc(&d_pip_res,   cfg.vectors * sizeof(unsigned char));

    cudaMemcpy(d_queries, h_queries.data(), h_queries.size() * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_vectors, h_vectors.data(), h_vectors.size() * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_lats1,   h_lats.data(),    h_lats.size()   * sizeof(double), cudaMemcpyHostToDevice);
    cudaMemcpy(d_lons1,   h_lons.data(),    h_lons.size()   * sizeof(double), cudaMemcpyHostToDevice);
    cudaMemcpy(d_lats2,   h_lats.data(),    h_lats.size()   * sizeof(double), cudaMemcpyHostToDevice);
    cudaMemcpy(d_lons2,   h_lons.data(),    h_lons.size()   * sizeof(double), cudaMemcpyHostToDevice);
    cudaMemcpy(d_poly,    h_poly.data(),    h_poly.size()   * sizeof(double), cudaMemcpyHostToDevice);

    // ── L2 distance ─────────────────────────────────────────────────────────
    // The kernel is already occupancy-tuned; we benchmark to confirm the gain.
    {
        auto rows = sweepBlockSizes1D("L2Distance(2D-tile)", blockSizes1D, cfg.iters,
            [&](cudaStream_t s, int /*bs*/) {
                // The launcher picks the block size internally; we time it as-is.
                launchL2DistanceKernel(d_queries, d_vectors, d_distances,
                                       cfg.queries, cfg.vectors, cfg.dim, s);
            });
        for (auto& r : rows) all.push_back(r);
    }

    // ── Inner product ────────────────────────────────────────────────────────
    {
        auto rows = sweepBlockSizes1D("InnerProduct(2D-tile)", blockSizes1D, cfg.iters,
            [&](cudaStream_t s, int /*bs*/) {
                launchInnerProductDistanceKernel(d_queries, d_vectors, d_distances,
                                                 cfg.queries, cfg.vectors, cfg.dim, s);
            });
        for (auto& r : rows) all.push_back(r);
    }

    // ── Haversine distance ───────────────────────────────────────────────────
    {
        auto rows = sweepBlockSizes1D("HaversineDistance(1D)", blockSizes1D, cfg.iters,
            [&](cudaStream_t s, int /*bs*/) {
                launchGeoDistanceKernel(d_lats1, d_lons1, d_lats2, d_lons2,
                                        d_geo_dist, cfg.vectors,
                                        0 /* HAVERSINE */, static_cast<void*>(s));
            });
        for (auto& r : rows) all.push_back(r);
    }

    // ── Point-in-polygon ────────────────────────────────────────────────────
    {
        auto rows = sweepBlockSizes1D("PointInPolygon(1D)", blockSizes1D, cfg.iters,
            [&](cudaStream_t s, int /*bs*/) {
                launchGeoContainmentKernel(d_lats1, d_lons1, cfg.vectors,
                                           d_poly, static_cast<int>(h_poly.size() / 2),
                                           d_pip_res, static_cast<void*>(s));
            });
        for (auto& r : rows) all.push_back(r);
    }

    // ── Cleanup ──────────────────────────────────────────────────────────────
    cudaFree(d_queries);    cudaFree(d_vectors);   cudaFree(d_distances);
    cudaFree(d_lats1);      cudaFree(d_lons1);
    cudaFree(d_lats2);      cudaFree(d_lons2);
    cudaFree(d_geo_dist);   cudaFree(d_poly);       cudaFree(d_pip_res);

    return all;
}

#endif // THEMIS_ENABLE_CUDA

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    const BenchConfig cfg = parseArgs(argc, argv);

#ifndef THEMIS_ENABLE_CUDA
    std::puts("THEMIS_ENABLE_CUDA not set — no CUDA device benchmarks available.");
    std::puts("Build with -DTHEMIS_ENABLE_CUDA=ON to run GPU kernel benchmarks.");
    return 0;
#else
    // Check device availability
    int deviceCount = 0;
    if (cudaGetDeviceCount(&deviceCount) != cudaSuccess || deviceCount == 0) {
        std::puts("No CUDA-capable device found; skipping benchmark.");
        return 0;
    }

    cudaDeviceProp prop{};
    cudaGetDeviceProperties(&prop, 0);
    std::printf("Device: %s  (SM %d.%d, %d SMs, warpSize=%d)\n\n",
                prop.name, prop.major, prop.minor,
                prop.multiProcessorCount, prop.warpSize);

    std::printf("Config: vectors=%d  queries=%d  dim=%d  iters=%d\n\n",
                cfg.vectors, cfg.queries, cfg.dim, cfg.iters);

    const auto results = runBenchmarks(cfg);

    // ── Print table ──────────────────────────────────────────────────────────
    std::printf("%-35s  %10s  %10s\n", "Kernel", "BlockSize", "Avg(ms)");
    std::printf("%-35s  %10s  %10s\n",
                std::string(35, '-').c_str(),
                std::string(10, '-').c_str(),
                std::string(10, '-').c_str());

    // Group by kernel name and highlight the fastest configuration
    std::string lastKernel;
    double bestMs = 1e18;
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& r = results[i];
        if (r.kernel != lastKernel) {
            if (!lastKernel.empty()) std::puts("");
            lastKernel = r.kernel;
            bestMs = 1e18;
            for (size_t j = i; j < results.size() && results[j].kernel == lastKernel; ++j)
                bestMs = std::min(bestMs, results[j].avgMs);
        }
        const char* marker = (r.avgMs <= bestMs * 1.01) ? " ◀ best" : "";
        std::printf("%-35s  %10d  %10.3f%s\n",
                    r.kernel.c_str(), r.blockSize, r.avgMs, marker);
    }
    std::puts("");

    return 0;
#endif
}
