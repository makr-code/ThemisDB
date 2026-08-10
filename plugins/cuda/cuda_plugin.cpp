/**
 * @file cuda_plugin.cpp
 * @brief Standalone loadable CUDA acceleration plugin for ThemisDB.
 *
 * Implements the @c BackendPlugin interface and exposes the
 * @c CreateBackendPlugin() C entry-point required by @c PluginLoader.
 *
 * Feature matrix
 * ──────────────
 * | vectorMode  | precision | backend created                    |
 * |-------------|-----------|-------------------------------------|
 * | SINGLE_GPU  | FP32      | CUDAVectorBackend (plain)           |
 * | SINGLE_GPU  | FP16      | MixedPrecisionVectorBackend (FP16)  |
 * | SINGLE_GPU  | BF16      | MixedPrecisionVectorBackend (BF16)  |
 * | MULTI_GPU   | any       | MultiGPUVectorBackend               |
 * | FAISS_GPU   | any       | FaissGPUVectorBackend               |
 *
 * @version 2.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 */

#include "cuda_plugin_config.h"
#include "acceleration/cuda_backend.h"
#include "acceleration/multi_gpu_backend.h"
#include "acceleration/faiss_gpu_backend.h"
#include "acceleration/plugin_loader.h"
#include "acceleration/fp16_vector_kernels.h"

#include <memory>
#include <string>
#include <stdexcept>
#include <utility>
#include <vector>

namespace themis {
namespace acceleration {

// ─────────────────────────────────────────────────────────────────────────────
// MixedPrecisionVectorBackend — wraps CUDAVectorBackend and replaces the
// computeDistances() hot path with FP16 or BF16 CUDA kernels when the build
// includes CUDA support.
// ─────────────────────────────────────────────────────────────────────────────

/// @brief Wraps @c CUDAVectorBackend and routes distance computations through
///        FP16 or BF16 CUDA kernels declared in fp16_vector_kernels.h.
///
/// All methods that are not precision-sensitive are forwarded verbatim to the
/// inner @c CUDAVectorBackend.  This keeps the wrapper thin and ensures that
/// future changes to the base backend are automatically inherited.
class MixedPrecisionVectorBackend final : public IVectorBackend {
public:
    /// @brief Construct a mixed-precision wrapper.
    /// @param precision  @c CudaPluginPrecision::FP16 or @c BF16.
    /// @throws std::invalid_argument if @p precision is FP32 (use plain
    ///         CUDAVectorBackend for FP32).
    explicit MixedPrecisionVectorBackend(CudaPluginPrecision precision)
        : precision_(precision)
        , inner_(std::make_unique<CUDAVectorBackend>())
    {
        if (precision == CudaPluginPrecision::FP32) {
            throw std::invalid_argument(
                "MixedPrecisionVectorBackend requires FP16 or BF16 precision; "
                "use CUDAVectorBackend directly for FP32.");
        }
    }

    // ── IComputeBackend ────────────────────────────────────────────────────

    /// @brief Returns "CUDA FP16" or "CUDA BF16" depending on the precision.
    /// @return Null-terminated name string.
    [[nodiscard]] const char* name() const noexcept override
    {
        return (precision_ == CudaPluginPrecision::FP16) ? "CUDA FP16" : "CUDA BF16";
    }

    /// @brief Always @c BackendType::CUDA.
    /// @return @c BackendType::CUDA
    [[nodiscard]] BackendType type() const noexcept override
    {
        return BackendType::CUDA;
    }

    /// @brief Delegates to the wrapped CUDAVectorBackend.
    /// @return true when CUDA is available on this machine.
    [[nodiscard]] bool isAvailable() const noexcept override
    {
        return inner_->isAvailable();
    }

    /// @brief Delegates to the wrapped CUDAVectorBackend.
    /// @return Backend capability descriptor.
    [[nodiscard]] BackendCapabilities getCapabilities() const override
    {
        return inner_->getCapabilities();
    }

    /// @brief Delegates to the wrapped CUDAVectorBackend.
    /// @return true on success.
    [[nodiscard]] bool initialize() override
    {
        return inner_->initialize();
    }

    /// @brief Delegates to the wrapped CUDAVectorBackend.
    void shutdown() override
    {
        inner_->shutdown();
    }

    // ── IVectorBackend ─────────────────────────────────────────────────────

    /// @brief Compute pairwise distances using FP16 or BF16 GPU kernels.
    ///
    /// When @c THEMIS_ENABLE_CUDA is defined, the function:
    ///   1. Allocates device buffers and uploads the FP32 input data.
    ///   2. Dispatches @c launchFP16L2DistanceKernel or
    ///      @c launchBF16L2DistanceKernel.
    ///   3. Downloads and returns the FP32 result.
    ///
    /// Falls back to the wrapped CUDAVectorBackend when CUDA is not
    /// available at runtime (@c isAvailable() == false).
    ///
    /// @param queries    Host FP32 pointer [numQueries × dim]
    /// @param numQueries Number of query vectors
    /// @param dim        Vector dimensionality
    /// @param vectors    Host FP32 pointer [numVectors × dim]
    /// @param numVectors Number of database vectors
    /// @param useL2      True for L2 distance (the mixed-precision kernel
    ///                   always computes L2; the flag is passed through to
    ///                   the fallback path).
    /// @return Flat FP32 distance matrix of size [numQueries × numVectors].
    [[nodiscard]] std::vector<float> computeDistances(
        const float* queries,
        size_t       numQueries,
        size_t       dim,
        const float* vectors,
        size_t       numVectors,
        bool         useL2 = true) override
    {
#ifdef THEMIS_ENABLE_CUDA
        if (isAvailable() && useL2) {
            const size_t outElems = numQueries * numVectors;
            std::vector<float> result(outElems);

            // Allocate device memory
            float* d_q   = nullptr;
            float* d_v   = nullptr;
            float* d_out = nullptr;

            const size_t qBytes   = numQueries  * dim * sizeof(float);
            const size_t vBytes   = numVectors  * dim * sizeof(float);
            const size_t outBytes = outElems         * sizeof(float);

            if (cudaMalloc(reinterpret_cast<void**>(&d_q),   qBytes)   != cudaSuccess ||
                cudaMalloc(reinterpret_cast<void**>(&d_v),   vBytes)   != cudaSuccess ||
                cudaMalloc(reinterpret_cast<void**>(&d_out), outBytes) != cudaSuccess)
            {
                // Allocation failure — fall back gracefully
                cudaFree(d_q);
                cudaFree(d_v);
                cudaFree(d_out);
                return inner_->computeDistances(queries, numQueries, dim,
                                                vectors, numVectors, useL2);
            }

            cudaMemcpy(d_q, queries, qBytes, cudaMemcpyHostToDevice);
            cudaMemcpy(d_v, vectors, vBytes, cudaMemcpyHostToDevice);

            cudaError_t err;
            if (precision_ == CudaPluginPrecision::FP16) {
                err = launchFP16L2DistanceKernel(d_q, d_v, d_out,
                                                 static_cast<int>(numQueries),
                                                 static_cast<int>(numVectors),
                                                 static_cast<int>(dim),
                                                 /*stream=*/nullptr);
            } else {
                err = launchBF16L2DistanceKernel(d_q, d_v, d_out,
                                                 static_cast<int>(numQueries),
                                                 static_cast<int>(numVectors),
                                                 static_cast<int>(dim),
                                                 /*stream=*/nullptr);
            }

            if (err == cudaSuccess) {
                cudaMemcpy(result.data(), d_out, outBytes, cudaMemcpyDeviceToHost);
            }

            cudaFree(d_q);
            cudaFree(d_v);
            cudaFree(d_out);

            if (err != cudaSuccess) {
                // Kernel error — fall back to FP32 path
                return inner_->computeDistances(queries, numQueries, dim,
                                                vectors, numVectors, useL2);
            }

            return result;
        }
#endif
        // Non-CUDA build or runtime unavailable — delegate to base backend
        return inner_->computeDistances(queries, numQueries, dim,
                                        vectors, numVectors, useL2);
    }

    /// @brief Batched k-nearest-neighbour search using precision-selected distances.
    ///
    /// Calls @c computeDistances() (which may use FP16/BF16 kernels) and then
    /// performs a per-query top-k selection on the host.  For large k or when
    /// the GPU is unavailable, delegates entirely to the wrapped backend.
    ///
    /// @param queries    Host FP32 pointer [numQueries × dim]
    /// @param numQueries Number of query vectors
    /// @param dim        Vector dimensionality
    /// @param vectors    Host FP32 pointer [numVectors × dim]
    /// @param numVectors Number of database vectors
    /// @param k          Number of nearest neighbours to return
    /// @param useL2      True for L2 distance metric
    /// @return Per-query lists of (globalIndex, distance) pairs, size k each.
    [[nodiscard]] std::vector<std::vector<std::pair<uint32_t, float>>> batchKnnSearch(
        const float* queries,
        size_t       numQueries,
        size_t       dim,
        const float* vectors,
        size_t       numVectors,
        size_t       k,
        bool         useL2 = true) override
    {
#ifdef THEMIS_ENABLE_CUDA
        if (isAvailable() && useL2 && k <= numVectors) {
            // Compute all pairwise distances via the mixed-precision kernel
            std::vector<float> dists =
                computeDistances(queries, numQueries, dim, vectors, numVectors, useL2);

            std::vector<std::vector<std::pair<uint32_t, float>>> results(numQueries);
            for (size_t q = 0; q < numQueries; ++q) {
                // Build index array for this query row
                const float* row = dists.data() + q * numVectors;
                std::vector<std::pair<float, uint32_t>> indexed(numVectors);
                for (size_t v = 0; v < numVectors; ++v) {
                    indexed[v] = {row[v], static_cast<uint32_t>(v)};
                }
                // Partial sort to find k smallest distances
                std::partial_sort(indexed.begin(),
                                  indexed.begin() + static_cast<ptrdiff_t>(k),
                                  indexed.end(),
                                  [](const auto& a, const auto& b) {
                                      return a.first < b.first;
                                  });
                results[q].reserve(k);
                for (size_t i = 0; i < k; ++i) {
                    results[q].emplace_back(indexed[i].second, indexed[i].first);
                }
            }
            return results;
        }
#endif
        return inner_->batchKnnSearch(queries, numQueries, dim,
                                      vectors, numVectors, k, useL2);
    }

private:
    CudaPluginPrecision                  precision_;  ///< FP16 or BF16
    std::unique_ptr<CUDAVectorBackend>   inner_;      ///< Wrapped FP32 backend
};

// ─────────────────────────────────────────────────────────────────────────────
// CUDAAccelerationPlugin
// ─────────────────────────────────────────────────────────────────────────────

/// @brief Standalone loadable CUDA acceleration plugin for ThemisDB.
///
/// Implements @c BackendPlugin to provide factory methods for CUDA-backed
/// vector, graph, and geo backends.  The vector backend flavour is selected
/// via @c CudaPluginConfig::vectorMode; the precision of distance kernels is
/// selected via @c CudaPluginConfig::precision.
///
/// Load via @c PluginLoader::loadPlugin("themis_accel_cuda.plugin.so") or
/// directly via the @c CreateBackendPlugin() entry-point.
class CUDAAccelerationPlugin final : public BackendPlugin {
public:
    /// @brief Default constructor — single-GPU FP32 mode.
    CUDAAccelerationPlugin() = default;

    /// @brief Construct with an explicit configuration.
    /// @param config  Runtime configuration selecting backend mode and precision.
    explicit CUDAAccelerationPlugin(CudaPluginConfig config)
        : config_(std::move(config))
    {}

    ~CUDAAccelerationPlugin() override = default;

    // ── BackendPlugin metadata ─────────────────────────────────────────────

    /// @brief Human-readable plugin name.
    /// @return "ThemisDB CUDA Acceleration Plugin"
    [[nodiscard]] const char* pluginName() const noexcept override
    {
        return "ThemisDB CUDA Acceleration Plugin";
    }

    /// @brief Semantic version string of this plugin.
    /// @return "2.0.0"
    [[nodiscard]] const char* pluginVersion() const noexcept override
    {
        return "2.0.0";
    }

    /// @brief Backend type advertised by this plugin.
    ///
    /// Returns @c BackendType::MULTI_GPU when the config selects multi-GPU
    /// mode; @c BackendType::CUDA otherwise.
    ///
    /// @return @c BackendType::CUDA or @c BackendType::MULTI_GPU
    [[nodiscard]] BackendType backendType() const noexcept override
    {
        return (config_.vectorMode == CudaPluginVectorMode::MULTI_GPU)
                   ? BackendType::MULTI_GPU
                   : BackendType::CUDA;
    }

    // ── BackendPlugin factories ────────────────────────────────────────────

    /// @brief Create a vector backend according to the plugin configuration.
    ///
    /// Selection logic:
    ///  - SINGLE_GPU + FP32  → @c CUDAVectorBackend
    ///  - SINGLE_GPU + FP16  → @c MixedPrecisionVectorBackend(FP16)
    ///  - SINGLE_GPU + BF16  → @c MixedPrecisionVectorBackend(BF16)
    ///  - MULTI_GPU  + any   → @c MultiGPUVectorBackend(config_.multiGpuConfig)
    ///  - FAISS_GPU  + any   → @c FaissGPUVectorBackend, then initializeIndex()
    ///
    /// @return Unique pointer to the created @c IVectorBackend.
    /// @throws std::runtime_error if FAISS GPU index initialisation fails.
    [[nodiscard]] std::unique_ptr<IVectorBackend> createVectorBackend() override
    {
        switch (config_.vectorMode) {
        case CudaPluginVectorMode::MULTI_GPU:
            return std::make_unique<MultiGPUVectorBackend>(config_.multiGpuConfig);

        case CudaPluginVectorMode::FAISS_GPU: {
#ifdef THEMIS_ENABLE_CUDA
            auto backend = std::make_unique<FaissGPUVectorBackend>();
            if (!backend->initializeIndex(config_.faissConfig)) {
                throw std::runtime_error(
                    "CUDAAccelerationPlugin: FaissGPUVectorBackend::initializeIndex() "
                    "failed — check FAISS configuration and GPU availability.");
            }
            return backend;
#else
            throw std::runtime_error(
                "CUDAAccelerationPlugin: FAISS_GPU mode requires THEMIS_ENABLE_CUDA.");
#endif
        }

        case CudaPluginVectorMode::SINGLE_GPU:
        default:
            if (config_.precision == CudaPluginPrecision::FP32) {
                return std::make_unique<CUDAVectorBackend>();
            }
            return std::make_unique<MixedPrecisionVectorBackend>(config_.precision);
        }
    }

    /// @brief Create a CUDA graph backend.
    /// @return Unique pointer to @c CUDAGraphBackend.
    [[nodiscard]] std::unique_ptr<IGraphBackend> createGraphBackend() override
    {
        return std::make_unique<CUDAGraphBackend>();
    }

    /// @brief Create a CUDA geospatial backend.
    /// @return Unique pointer to @c CUDAGeoBackend.
    [[nodiscard]] std::unique_ptr<IGeoBackend> createGeoBackend() override
    {
        return std::make_unique<CUDAGeoBackend>();
    }

private:
    CudaPluginConfig config_;  ///< Resolved at construction time
};

} // namespace acceleration
} // namespace themis

// ─────────────────────────────────────────────────────────────────────────────
// Plugin entry-point — called by PluginLoader::loadPlugin()
// ─────────────────────────────────────────────────────────────────────────────

THEMIS_DEFINE_PLUGIN(themis::acceleration::CUDAAccelerationPlugin)
