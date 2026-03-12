/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            gpu_compression.cpp                                ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-12 20:37:03                                ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * ThemisDB GPU-Accelerated Compression Implementation
 *
 * GPU paths (CUDA/HIP) are compiled only when the corresponding CMake
 * feature flag is enabled:
 *   - THEMIS_ENABLE_CUDA → NVIDIA CUDA + nvCOMP
 *   - THEMIS_ENABLE_HIP  → AMD HIP / ROCm
 *
 * When neither flag is set (the default) every operation transparently
 * falls through to the CPU implementation so the module compiles and
 * functions correctly on any host.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/gpu_compression.h"
#include "utils/zstd_codec.h"

#include <lz4.h>
#include <snappy.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <functional>
#include <limits>
#include <stdexcept>

// ============================================================================
// CUDA / nvCOMP headers (only compiled when CUDA is enabled)
// ============================================================================
#ifdef THEMIS_ENABLE_CUDA
#  include <cuda_runtime.h>
#  include <nvcomp/lz4.h>
#  include <nvcomp/snappy.h>
#  include <nvcomp/zstd.h>
#endif

// ============================================================================
// HIP / ROCm headers (only compiled when HIP is enabled)
// ============================================================================
#ifdef THEMIS_ENABLE_HIP
#  include <hip/hip_runtime.h>
// ROCm does not yet ship a unified compression library analogous to nvCOMP;
// the HIP path therefore delegates to the CPU implementations until a
// first-party ROCm compression library becomes available.
#endif

namespace themis {
namespace storage {

// ============================================================================
// GpuCompressionImpl  — abstract platform-specific backend
// ============================================================================

/**
 * @brief Abstract interface implemented by each platform-specific backend.
 *
 * The concrete types are defined further below inside anonymous namespaces
 * and instantiated by GpuCompressionManager::init_gpu().
 */
class GpuCompressionImpl {
public:
    virtual ~GpuCompressionImpl() = default;

    virtual bool initialize(const GpuCompressionConfig& cfg) = 0;
    virtual void shutdown() = 0;
    virtual bool is_available() const = 0;

    virtual GpuCompressionResult compress(
        const uint8_t* data, size_t size,
        GpuCompressionAlgorithm algorithm,
        const GpuCompressionConfig& cfg) = 0;

    virtual std::vector<uint8_t> decompress(
        const std::vector<uint8_t>& data,
        GpuCompressionAlgorithm algorithm,
        size_t original_size,
        const GpuCompressionConfig& cfg) = 0;
};

// ============================================================================
// Helpers
// ============================================================================

namespace {

// ---------------------------------------------------------------------------
// Timing helper (returns elapsed ms since 'start')
// ---------------------------------------------------------------------------
inline double elapsed_ms(
    const std::chrono::high_resolution_clock::time_point& start)
{
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(now - start).count();
}

// ---------------------------------------------------------------------------
// Update a running average in place
// ---------------------------------------------------------------------------
inline void update_avg(double& avg, uint64_t n, double sample_ms)
{
    if (n == 0) {
        avg = sample_ms;
    } else {
        avg = (avg * static_cast<double>(n - 1) + sample_ms)
              / static_cast<double>(n);
    }
}

// ============================================================================
// CUDA (nvCOMP) implementation
// ============================================================================
#ifdef THEMIS_ENABLE_CUDA

class CudaNvcompImpl final : public GpuCompressionImpl {
public:
    ~CudaNvcompImpl() override { shutdown(); }

    bool initialize(const GpuCompressionConfig& cfg) override {
        cudaError_t err = cudaSetDevice(cfg.device_id);
        if (err != cudaSuccess) {
            spdlog::error("[gpu_compress] cudaSetDevice({}) failed: {}",
                          cfg.device_id, cudaGetErrorString(err));
            return false;
        }
        device_id_ = cfg.device_id;

        err = cudaStreamCreate(&stream_);
        if (err != cudaSuccess) {
            spdlog::error("[gpu_compress] cudaStreamCreate failed: {}",
                          cudaGetErrorString(err));
            return false;
        }

        available_ = true;
        spdlog::info("[gpu_compress] CUDA/nvCOMP backend initialised (device {})",
                     device_id_);
        return true;
    }

    void shutdown() override {
        if (stream_) {
            cudaStreamSynchronize(stream_);
            cudaStreamDestroy(stream_);
            stream_ = nullptr;
        }
        available_ = false;
    }

    bool is_available() const override { return available_; }

    // ------------------------------------------------------------------
    // compress
    // ------------------------------------------------------------------
    GpuCompressionResult compress(
        const uint8_t* data, size_t size,
        GpuCompressionAlgorithm algorithm,
        const GpuCompressionConfig& cfg) override
    {
        GpuCompressionResult result;
        result.algorithm     = algorithm;
        result.original_size = size;

        // Allocate device input buffer
        void* d_in = nullptr;
        if (cudaMalloc(&d_in, size) != cudaSuccess) {
            result.error_message = "cudaMalloc failed for input buffer";
            return result;
        }
        cudaMemcpyAsync(d_in, data, size,
                        cudaMemcpyHostToDevice, stream_);

        bool ok = false;
        switch (algorithm) {
            case GpuCompressionAlgorithm::ZSTD:
                ok = nvcomp_compress_zstd(
                    static_cast<uint8_t*>(d_in), size, cfg, result);
                break;
            case GpuCompressionAlgorithm::SNAPPY:
                ok = nvcomp_compress_snappy(
                    static_cast<uint8_t*>(d_in), size, cfg, result);
                break;
            case GpuCompressionAlgorithm::LZ4:
                ok = nvcomp_compress_lz4(
                    static_cast<uint8_t*>(d_in), size, cfg, result);
                break;
        }

        cudaFree(d_in);

        if (ok) {
            result.compression_ratio =
                static_cast<float>(size) /
                static_cast<float>(result.data.size());
            result.used_gpu = true;
            result.success  = true;
        }
        return result;
    }

    // ------------------------------------------------------------------
    // decompress
    // ------------------------------------------------------------------
    std::vector<uint8_t> decompress(
        const std::vector<uint8_t>& compressed,
        GpuCompressionAlgorithm algorithm,
        size_t original_size,
        const GpuCompressionConfig& /*cfg*/) override
    {
        switch (algorithm) {
            case GpuCompressionAlgorithm::ZSTD:
                return nvcomp_decompress_zstd(compressed, original_size);
            case GpuCompressionAlgorithm::SNAPPY:
                return nvcomp_decompress_snappy(compressed, original_size);
            case GpuCompressionAlgorithm::LZ4:
                return nvcomp_decompress_lz4(compressed, original_size);
        }
        return {};
    }

private:
    int            device_id_  = 0;
    cudaStream_t   stream_     = nullptr;
    bool           available_  = false;

    // ------------------------------------------------------------------
    // nvCOMP Zstd compress
    // ------------------------------------------------------------------
    bool nvcomp_compress_zstd(
        const uint8_t* d_in, size_t in_size,
        const GpuCompressionConfig& cfg,
        GpuCompressionResult& result)
    {
        nvcompBatchedZstdOpts_t opts{cfg.zstd_level};
        const size_t chunk   = cfg.chunk_size;
        size_t n_chunks      = (in_size + chunk - 1) / chunk;

        // Build host-side chunk pointer arrays
        std::vector<void*>   h_in_ptrs(n_chunks);
        std::vector<size_t>  h_in_sizes(n_chunks);
        for (size_t i = 0; i < n_chunks; ++i) {
            h_in_ptrs[i]  = const_cast<uint8_t*>(d_in) + i * chunk;
            h_in_sizes[i] = std::min(chunk, in_size - i * chunk);
        }

        size_t max_out_per_chunk = 0;
        nvcompBatchedZstdCompressGetMaxOutputChunkSize(
            chunk, opts, &max_out_per_chunk);

        // Device arrays
        void** d_in_ptrs   = nullptr;
        size_t* d_in_sizes = nullptr;
        void** d_out_ptrs  = nullptr;
        size_t* d_out_sizes = nullptr;
        void* d_workspace   = nullptr;
        size_t workspace_sz = 0;

        cudaMalloc(&d_in_ptrs,   n_chunks * sizeof(void*));
        cudaMalloc(&d_in_sizes,  n_chunks * sizeof(size_t));
        cudaMalloc(&d_out_ptrs,  n_chunks * sizeof(void*));
        cudaMalloc(&d_out_sizes, n_chunks * sizeof(size_t));

        // Allocate per-chunk output on device
        std::vector<void*> h_out_ptrs(n_chunks);
        for (size_t i = 0; i < n_chunks; ++i) {
            cudaMalloc(&h_out_ptrs[i], max_out_per_chunk);
        }

        cudaMemcpyAsync(d_in_ptrs,  h_in_ptrs.data(),
                        n_chunks * sizeof(void*),  cudaMemcpyHostToDevice, stream_);
        cudaMemcpyAsync(d_in_sizes, h_in_sizes.data(),
                        n_chunks * sizeof(size_t), cudaMemcpyHostToDevice, stream_);
        cudaMemcpyAsync(d_out_ptrs, h_out_ptrs.data(),
                        n_chunks * sizeof(void*),  cudaMemcpyHostToDevice, stream_);

        nvcompBatchedZstdCompressGetWorkspaceSize(
            n_chunks, chunk, opts, &workspace_sz);
        cudaMalloc(&d_workspace, workspace_sz);

        nvcompStatus_t status = nvcompBatchedZstdCompressAsync(
            (const void* const*)d_in_ptrs, d_in_sizes,
            chunk, n_chunks,
            d_workspace, workspace_sz,
            d_out_ptrs, d_out_sizes,
            opts, stream_);
        cudaStreamSynchronize(stream_);

        bool ok = (status == nvcompSuccess);
        if (ok) {
            std::vector<size_t> h_out_sizes(n_chunks);
            cudaMemcpy(h_out_sizes.data(), d_out_sizes,
                       n_chunks * sizeof(size_t), cudaMemcpyDeviceToHost);

            // Assemble result: [n_chunks:8][chunk_size:8][sizes...][data...]
            size_t total_out = 0;
            for (size_t s : h_out_sizes) total_out += s;

            result.data.resize(8 + 8 + n_chunks * 8 + total_out);
            uint8_t* p = result.data.data();
            memcpy(p, &n_chunks,  8); p += 8;
            memcpy(p, &in_size,   8); p += 8;
            for (size_t i = 0; i < n_chunks; ++i) {
                memcpy(p, &h_out_sizes[i], 8); p += 8;
            }
            for (size_t i = 0; i < n_chunks; ++i) {
                cudaMemcpy(p, h_out_ptrs[i], h_out_sizes[i],
                           cudaMemcpyDeviceToHost);
                p += h_out_sizes[i];
            }
        }

        // Cleanup
        for (auto* ptr : h_out_ptrs) cudaFree(ptr);
        cudaFree(d_in_ptrs);
        cudaFree(d_in_sizes);
        cudaFree(d_out_ptrs);
        cudaFree(d_out_sizes);
        cudaFree(d_workspace);

        return ok;
    }

    // ------------------------------------------------------------------
    // nvCOMP Snappy compress
    // ------------------------------------------------------------------
    bool nvcomp_compress_snappy(
        const uint8_t* d_in, size_t in_size,
        const GpuCompressionConfig& cfg,
        GpuCompressionResult& result)
    {
        const size_t chunk  = cfg.chunk_size;
        size_t n_chunks     = (in_size + chunk - 1) / chunk;

        std::vector<void*>  h_in_ptrs(n_chunks);
        std::vector<size_t> h_in_sizes(n_chunks);
        for (size_t i = 0; i < n_chunks; ++i) {
            h_in_ptrs[i]  = const_cast<uint8_t*>(d_in) + i * chunk;
            h_in_sizes[i] = std::min(chunk, in_size - i * chunk);
        }

        size_t max_out_per_chunk = 0;
        nvcompBatchedSnappyCompressGetMaxOutputChunkSize(
            chunk, nvcompBatchedSnappyDefaultOpts, &max_out_per_chunk);

        void** d_in_ptrs    = nullptr;
        size_t* d_in_sizes  = nullptr;
        void** d_out_ptrs   = nullptr;
        size_t* d_out_sizes = nullptr;
        void* d_workspace   = nullptr;
        size_t workspace_sz = 0;

        cudaMalloc(&d_in_ptrs,   n_chunks * sizeof(void*));
        cudaMalloc(&d_in_sizes,  n_chunks * sizeof(size_t));
        cudaMalloc(&d_out_ptrs,  n_chunks * sizeof(void*));
        cudaMalloc(&d_out_sizes, n_chunks * sizeof(size_t));

        std::vector<void*> h_out_ptrs(n_chunks);
        for (size_t i = 0; i < n_chunks; ++i)
            cudaMalloc(&h_out_ptrs[i], max_out_per_chunk);

        cudaMemcpyAsync(d_in_ptrs,  h_in_ptrs.data(),
                        n_chunks * sizeof(void*),  cudaMemcpyHostToDevice, stream_);
        cudaMemcpyAsync(d_in_sizes, h_in_sizes.data(),
                        n_chunks * sizeof(size_t), cudaMemcpyHostToDevice, stream_);
        cudaMemcpyAsync(d_out_ptrs, h_out_ptrs.data(),
                        n_chunks * sizeof(void*),  cudaMemcpyHostToDevice, stream_);

        nvcompBatchedSnappyCompressGetWorkspaceSize(
            n_chunks, chunk, nvcompBatchedSnappyDefaultOpts, &workspace_sz);
        cudaMalloc(&d_workspace, workspace_sz);

        nvcompStatus_t status = nvcompBatchedSnappyCompressAsync(
            (const void* const*)d_in_ptrs, d_in_sizes,
            chunk, n_chunks,
            d_workspace, workspace_sz,
            d_out_ptrs, d_out_sizes,
            nvcompBatchedSnappyDefaultOpts, stream_);
        cudaStreamSynchronize(stream_);

        bool ok = (status == nvcompSuccess);
        if (ok) {
            std::vector<size_t> h_out_sizes(n_chunks);
            cudaMemcpy(h_out_sizes.data(), d_out_sizes,
                       n_chunks * sizeof(size_t), cudaMemcpyDeviceToHost);

            size_t total_out = 0;
            for (size_t s : h_out_sizes) total_out += s;

            result.data.resize(8 + 8 + n_chunks * 8 + total_out);
            uint8_t* p = result.data.data();
            memcpy(p, &n_chunks, 8); p += 8;
            memcpy(p, &in_size,  8); p += 8;
            for (size_t i = 0; i < n_chunks; ++i) {
                memcpy(p, &h_out_sizes[i], 8); p += 8;
            }
            for (size_t i = 0; i < n_chunks; ++i) {
                cudaMemcpy(p, h_out_ptrs[i], h_out_sizes[i],
                           cudaMemcpyDeviceToHost);
                p += h_out_sizes[i];
            }
        }

        for (auto* ptr : h_out_ptrs) cudaFree(ptr);
        cudaFree(d_in_ptrs);
        cudaFree(d_in_sizes);
        cudaFree(d_out_ptrs);
        cudaFree(d_out_sizes);
        cudaFree(d_workspace);

        return ok;
    }

    // ------------------------------------------------------------------
    // nvCOMP LZ4 compress
    // ------------------------------------------------------------------
    bool nvcomp_compress_lz4(
        const uint8_t* d_in, size_t in_size,
        const GpuCompressionConfig& cfg,
        GpuCompressionResult& result)
    {
        const size_t chunk  = cfg.chunk_size;
        size_t n_chunks     = (in_size + chunk - 1) / chunk;

        std::vector<void*>  h_in_ptrs(n_chunks);
        std::vector<size_t> h_in_sizes(n_chunks);
        for (size_t i = 0; i < n_chunks; ++i) {
            h_in_ptrs[i]  = const_cast<uint8_t*>(d_in) + i * chunk;
            h_in_sizes[i] = std::min(chunk, in_size - i * chunk);
        }

        size_t max_out_per_chunk = 0;
        nvcompBatchedLZ4CompressGetMaxOutputChunkSize(
            chunk, nvcompBatchedLZ4DefaultOpts, &max_out_per_chunk);

        void** d_in_ptrs    = nullptr;
        size_t* d_in_sizes  = nullptr;
        void** d_out_ptrs   = nullptr;
        size_t* d_out_sizes = nullptr;
        void* d_workspace   = nullptr;
        size_t workspace_sz = 0;

        cudaMalloc(&d_in_ptrs,   n_chunks * sizeof(void*));
        cudaMalloc(&d_in_sizes,  n_chunks * sizeof(size_t));
        cudaMalloc(&d_out_ptrs,  n_chunks * sizeof(void*));
        cudaMalloc(&d_out_sizes, n_chunks * sizeof(size_t));

        std::vector<void*> h_out_ptrs(n_chunks);
        for (size_t i = 0; i < n_chunks; ++i)
            cudaMalloc(&h_out_ptrs[i], max_out_per_chunk);

        cudaMemcpyAsync(d_in_ptrs,  h_in_ptrs.data(),
                        n_chunks * sizeof(void*),  cudaMemcpyHostToDevice, stream_);
        cudaMemcpyAsync(d_in_sizes, h_in_sizes.data(),
                        n_chunks * sizeof(size_t), cudaMemcpyHostToDevice, stream_);
        cudaMemcpyAsync(d_out_ptrs, h_out_ptrs.data(),
                        n_chunks * sizeof(void*),  cudaMemcpyHostToDevice, stream_);

        nvcompBatchedLZ4CompressGetWorkspaceSize(
            n_chunks, chunk, nvcompBatchedLZ4DefaultOpts, &workspace_sz);
        cudaMalloc(&d_workspace, workspace_sz);

        nvcompStatus_t status = nvcompBatchedLZ4CompressAsync(
            (const void* const*)d_in_ptrs, d_in_sizes,
            chunk, n_chunks,
            d_workspace, workspace_sz,
            d_out_ptrs, d_out_sizes,
            nvcompBatchedLZ4DefaultOpts, stream_);
        cudaStreamSynchronize(stream_);

        bool ok = (status == nvcompSuccess);
        if (ok) {
            std::vector<size_t> h_out_sizes(n_chunks);
            cudaMemcpy(h_out_sizes.data(), d_out_sizes,
                       n_chunks * sizeof(size_t), cudaMemcpyDeviceToHost);

            size_t total_out = 0;
            for (size_t s : h_out_sizes) total_out += s;

            result.data.resize(8 + 8 + n_chunks * 8 + total_out);
            uint8_t* p = result.data.data();
            memcpy(p, &n_chunks, 8); p += 8;
            memcpy(p, &in_size,  8); p += 8;
            for (size_t i = 0; i < n_chunks; ++i) {
                memcpy(p, &h_out_sizes[i], 8); p += 8;
            }
            for (size_t i = 0; i < n_chunks; ++i) {
                cudaMemcpy(p, h_out_ptrs[i], h_out_sizes[i],
                           cudaMemcpyDeviceToHost);
                p += h_out_sizes[i];
            }
        }

        for (auto* ptr : h_out_ptrs) cudaFree(ptr);
        cudaFree(d_in_ptrs);
        cudaFree(d_in_sizes);
        cudaFree(d_out_ptrs);
        cudaFree(d_out_sizes);
        cudaFree(d_workspace);

        return ok;
    }

    // ------------------------------------------------------------------
    // nvCOMP decompression helpers (generic chunked format)
    // ------------------------------------------------------------------
    std::vector<uint8_t> nvcomp_decompress_impl(
        const std::vector<uint8_t>& compressed,
        size_t /*original_size*/,
        size_t workspace_sz_per_chunk,
        std::function<nvcompStatus_t(
            const void* const*, const size_t*, size_t,
            void*, size_t,
            void* const*, const size_t*,
            size_t, cudaStream_t)> decompress_fn)
    {
        if (compressed.size() < 16) return {};

        const uint8_t* p = compressed.data();
        size_t n_chunks  = 0;
        size_t orig_size = 0;
        memcpy(&n_chunks,  p, 8); p += 8;
        memcpy(&orig_size, p, 8); p += 8;

        if (compressed.size() < 16 + n_chunks * 8) return {};

        std::vector<size_t> chunk_sizes(n_chunks);
        for (size_t i = 0; i < n_chunks; ++i) {
            memcpy(&chunk_sizes[i], p, 8); p += 8;
        }

        // Allocate device buffers
        std::vector<void*> h_in_ptrs(n_chunks), h_out_ptrs(n_chunks);
        size_t per_chunk_out = (orig_size + n_chunks - 1) / n_chunks + 512;

        for (size_t i = 0; i < n_chunks; ++i) {
            cudaMalloc(&h_in_ptrs[i],  chunk_sizes[i]);
            cudaMalloc(&h_out_ptrs[i], per_chunk_out);
            cudaMemcpyAsync(h_in_ptrs[i], p, chunk_sizes[i],
                            cudaMemcpyHostToDevice, stream_);
            p += chunk_sizes[i];
        }

        void** d_in_ptrs    = nullptr;
        size_t* d_in_sizes  = nullptr;
        void** d_out_ptrs   = nullptr;
        size_t* d_out_sizes = nullptr;
        void* d_workspace   = nullptr;
        size_t workspace_sz = workspace_sz_per_chunk * n_chunks;

        cudaMalloc(&d_in_ptrs,   n_chunks * sizeof(void*));
        cudaMalloc(&d_in_sizes,  n_chunks * sizeof(size_t));
        cudaMalloc(&d_out_ptrs,  n_chunks * sizeof(void*));
        cudaMalloc(&d_out_sizes, n_chunks * sizeof(size_t));
        cudaMalloc(&d_workspace, workspace_sz);

        cudaMemcpyAsync(d_in_ptrs,  h_in_ptrs.data(),
                        n_chunks * sizeof(void*),  cudaMemcpyHostToDevice, stream_);
        cudaMemcpyAsync(d_in_sizes, chunk_sizes.data(),
                        n_chunks * sizeof(size_t), cudaMemcpyHostToDevice, stream_);

        std::vector<size_t> h_out_sizes(n_chunks, per_chunk_out);
        cudaMemcpyAsync(d_out_ptrs,  h_out_ptrs.data(),
                        n_chunks * sizeof(void*),  cudaMemcpyHostToDevice, stream_);
        cudaMemcpyAsync(d_out_sizes, h_out_sizes.data(),
                        n_chunks * sizeof(size_t), cudaMemcpyHostToDevice, stream_);

        // Note: actual decompressed sizes are written to d_out_sizes
        decompress_fn(
            (const void* const*)d_in_ptrs, (const size_t*)d_in_sizes,
            per_chunk_out,
            d_workspace, workspace_sz,
            (void* const*)d_out_ptrs, (size_t*)d_out_sizes,
            n_chunks, stream_);
        cudaStreamSynchronize(stream_);

        cudaMemcpy(h_out_sizes.data(), d_out_sizes,
                   n_chunks * sizeof(size_t), cudaMemcpyDeviceToHost);

        std::vector<uint8_t> result;
        result.reserve(orig_size);
        for (size_t i = 0; i < n_chunks; ++i) {
            size_t chunk_decompressed = h_out_sizes[i];
            size_t off = result.size();
            result.resize(off + chunk_decompressed);
            cudaMemcpy(result.data() + off, h_out_ptrs[i],
                       chunk_decompressed, cudaMemcpyDeviceToHost);
        }
        result.resize(orig_size);

        for (size_t i = 0; i < n_chunks; ++i) {
            cudaFree(h_in_ptrs[i]);
            cudaFree(h_out_ptrs[i]);
        }
        cudaFree(d_in_ptrs);
        cudaFree(d_in_sizes);
        cudaFree(d_out_ptrs);
        cudaFree(d_out_sizes);
        cudaFree(d_workspace);

        return result;
    }

    std::vector<uint8_t> nvcomp_decompress_zstd(
        const std::vector<uint8_t>& data, size_t orig)
    {
        size_t ws_sz = 0;
        nvcompBatchedZstdDecompressGetTempSize(1, 64 * 1024, &ws_sz);
        return nvcomp_decompress_impl(data, orig, ws_sz,
            [](const void* const* in, const size_t* in_sz, size_t unc_sz,
               void* ws, size_t ws_sz, void* const* out, size_t* out_sz,
               size_t n, cudaStream_t s) {
                return nvcompBatchedZstdDecompressAsync(
                    in, in_sz, unc_sz, nullptr, ws, ws_sz,
                    out, out_sz, n, s);
            });
    }

    std::vector<uint8_t> nvcomp_decompress_snappy(
        const std::vector<uint8_t>& data, size_t orig)
    {
        size_t ws_sz = 0;
        nvcompBatchedSnappyDecompressGetTempSize(1, 64 * 1024, &ws_sz);
        return nvcomp_decompress_impl(data, orig, ws_sz,
            [](const void* const* in, const size_t* in_sz, size_t unc_sz,
               void* ws, size_t ws_sz, void* const* out, size_t* out_sz,
               size_t n, cudaStream_t s) {
                return nvcompBatchedSnappyDecompressAsync(
                    in, in_sz, unc_sz, nullptr, ws, ws_sz,
                    out, out_sz, n, s);
            });
    }

    std::vector<uint8_t> nvcomp_decompress_lz4(
        const std::vector<uint8_t>& data, size_t orig)
    {
        size_t ws_sz = 0;
        nvcompBatchedLZ4DecompressGetTempSize(1, 64 * 1024, &ws_sz);
        return nvcomp_decompress_impl(data, orig, ws_sz,
            [](const void* const* in, const size_t* in_sz, size_t unc_sz,
               void* ws, size_t ws_sz, void* const* out, size_t* out_sz,
               size_t n, cudaStream_t s) {
                return nvcompBatchedLZ4DecompressAsync(
                    in, in_sz, unc_sz, nullptr, ws, ws_sz,
                    out, out_sz, n, s);
            });
    }
};

#endif // THEMIS_ENABLE_CUDA

} // anonymous namespace

// ============================================================================
// GpuCompressionManager — construction / destruction
// ============================================================================

GpuCompressionManager::GpuCompressionManager(const GpuCompressionConfig& cfg)
    : config_(cfg)
{
    if (config_.accel_type != GpuAccelerationType::CPU_ONLY) {
        if (!init_gpu()) {
            spdlog::warn("[gpu_compress] GPU initialisation failed; "
                         "falling back to CPU-only mode");
            active_accel_ = GpuAccelerationType::CPU_ONLY;
        }
    }
}

GpuCompressionManager::~GpuCompressionManager() = default;
GpuCompressionManager::GpuCompressionManager(GpuCompressionManager&&) noexcept = default;
GpuCompressionManager& GpuCompressionManager::operator=(GpuCompressionManager&&) noexcept = default;

// ============================================================================
// GPU initialisation
// ============================================================================

bool GpuCompressionManager::init_gpu()
{
    GpuAccelerationType requested = config_.accel_type;

    // AUTO: pick the best available backend at compile-time
    if (requested == GpuAccelerationType::AUTO) {
#ifdef THEMIS_ENABLE_CUDA
        requested = GpuAccelerationType::CUDA;
        spdlog::info("[gpu_compress] Auto-detected CUDA support");
#elif defined(THEMIS_ENABLE_HIP)
        requested = GpuAccelerationType::HIP;
        spdlog::info("[gpu_compress] Auto-detected HIP/ROCm support; "
                     "using CPU path until ROCm compression library is available");
        // HIP currently delegates to CPU — treat as CPU_ONLY
        active_accel_ = GpuAccelerationType::CPU_ONLY;
        return false;
#else
        spdlog::warn("[gpu_compress] No GPU support compiled in "
                     "(set THEMIS_ENABLE_CUDA or THEMIS_ENABLE_HIP)");
        active_accel_ = GpuAccelerationType::CPU_ONLY;
        return false;
#endif
    }

    switch (requested) {
#ifdef THEMIS_ENABLE_CUDA
        case GpuAccelerationType::CUDA: {
            auto cuda_impl = std::make_unique<CudaNvcompImpl>();
            if (!cuda_impl->initialize(config_)) {
                return false;
            }
            impl_        = std::move(cuda_impl);
            active_accel_ = GpuAccelerationType::CUDA;
            return true;
        }
#endif
        default:
            spdlog::warn("[gpu_compress] Requested backend ({}) not compiled in",
                         accel_type_to_string(requested));
            active_accel_ = GpuAccelerationType::CPU_ONLY;
            return false;
    }
}

bool GpuCompressionManager::should_use_gpu(size_t data_size) const
{
    if (force_cpu_) return false;
    if (!impl_ || !impl_->is_available()) return false;
    return data_size >= config_.min_size_for_gpu;
}

// ============================================================================
// compress
// ============================================================================

GpuCompressionResult GpuCompressionManager::compress(
    const uint8_t* data, size_t size, GpuCompressionAlgorithm algorithm)
{
    auto t_start = std::chrono::high_resolution_clock::now();

    ++stats_.total_compress_ops;
    stats_.bytes_in += size;

    GpuCompressionResult result;

    // ----------------------------------------------------------------
    // GPU path
    // ----------------------------------------------------------------
    if (should_use_gpu(size)) {
        try {
            result = impl_->compress(data, size, algorithm, config_);
            if (result.success) {
                ++stats_.gpu_compress_ops;
                stats_.bytes_out += result.data.size();
                double ms = elapsed_ms(t_start);
                update_avg(stats_.avg_gpu_compress_ms,
                           stats_.gpu_compress_ops, ms);
                return result;
            }
            // GPU compress returned failure — fall through to CPU
            spdlog::warn("[gpu_compress] GPU compress failed for {}, "
                         "falling back to CPU",
                         algorithm_to_string(algorithm));
            ++stats_.cpu_fallbacks;
        } catch (const std::exception& e) {
            spdlog::warn("[gpu_compress] GPU compress threw: {}; "
                         "falling back to CPU", e.what());
            ++stats_.cpu_fallbacks;
        }
    }

    // ----------------------------------------------------------------
    // CPU fallback path
    // ----------------------------------------------------------------
    switch (algorithm) {
        case GpuCompressionAlgorithm::ZSTD:
            result = cpu_compress_zstd(data, size);
            break;
        case GpuCompressionAlgorithm::SNAPPY:
            result = cpu_compress_snappy(data, size);
            break;
        case GpuCompressionAlgorithm::LZ4:
            result = cpu_compress_lz4(data, size);
            break;
    }

    stats_.bytes_out += result.data.size();
    double ms = elapsed_ms(t_start);
    uint64_t cpu_ops = stats_.total_compress_ops - stats_.gpu_compress_ops;
    update_avg(stats_.avg_cpu_compress_ms, cpu_ops, ms);

    return result;
}

GpuCompressionResult GpuCompressionManager::compress(
    const std::vector<uint8_t>& data, GpuCompressionAlgorithm algorithm)
{
    return compress(data.data(), data.size(), algorithm);
}

// ============================================================================
// decompress
// ============================================================================

std::vector<uint8_t> GpuCompressionManager::decompress(
    const std::vector<uint8_t>& compressed,
    GpuCompressionAlgorithm algorithm,
    size_t original_size)
{
    if (compressed.empty()) return {};

    auto t_start = std::chrono::high_resolution_clock::now();

    ++stats_.total_decompress_ops;

    std::vector<uint8_t> result;

    // ----------------------------------------------------------------
    // GPU path
    // ----------------------------------------------------------------
    if (should_use_gpu(compressed.size())) {
        try {
            result = impl_->decompress(compressed, algorithm,
                                       original_size, config_);
            if (!result.empty()) {
                ++stats_.gpu_decompress_ops;
                double ms = elapsed_ms(t_start);
                update_avg(stats_.avg_gpu_decompress_ms,
                           stats_.gpu_decompress_ops, ms);
                return result;
            }
            spdlog::warn("[gpu_compress] GPU decompress returned empty for {}, "
                         "falling back to CPU",
                         algorithm_to_string(algorithm));
            ++stats_.cpu_fallbacks;
        } catch (const std::exception& e) {
            spdlog::warn("[gpu_compress] GPU decompress threw: {}; "
                         "falling back to CPU", e.what());
            ++stats_.cpu_fallbacks;
        }
    }

    // ----------------------------------------------------------------
    // CPU fallback path
    // ----------------------------------------------------------------
    switch (algorithm) {
        case GpuCompressionAlgorithm::ZSTD:
            result = cpu_decompress_zstd(compressed, original_size);
            break;
        case GpuCompressionAlgorithm::SNAPPY:
            result = cpu_decompress_snappy(compressed, original_size);
            break;
        case GpuCompressionAlgorithm::LZ4:
            result = cpu_decompress_lz4(compressed, original_size);
            break;
    }

    double ms = elapsed_ms(t_start);
    uint64_t cpu_ops = stats_.total_decompress_ops - stats_.gpu_decompress_ops;
    update_avg(stats_.avg_cpu_decompress_ms, cpu_ops, ms);

    return result;
}

// ============================================================================
// Batch compress / decompress
// ============================================================================

std::vector<GpuCompressionResult> GpuCompressionManager::compress_batch(
    const std::vector<std::vector<uint8_t>>& buffers,
    GpuCompressionAlgorithm algorithm)
{
    std::vector<GpuCompressionResult> results;
    results.reserve(buffers.size());
    for (const auto& buf : buffers) {
        results.push_back(compress(buf, algorithm));
    }
    return results;
}

std::vector<std::vector<uint8_t>> GpuCompressionManager::decompress_batch(
    const std::vector<std::vector<uint8_t>>& compressed_buffers,
    GpuCompressionAlgorithm algorithm,
    const std::vector<size_t>& original_sizes)
{
    std::vector<std::vector<uint8_t>> results;
    results.reserve(compressed_buffers.size());
    for (size_t i = 0; i < compressed_buffers.size(); ++i) {
        size_t orig = (i < original_sizes.size()) ? original_sizes[i] : 0;
        results.push_back(decompress(compressed_buffers[i], algorithm, orig));
    }
    return results;
}

// ============================================================================
// CPU fallback implementations
// ============================================================================

// ------------------------------------------------------------------
// Zstd (reuses existing zstd_codec utility)
// ------------------------------------------------------------------

GpuCompressionResult GpuCompressionManager::cpu_compress_zstd(
    const uint8_t* data, size_t size)
{
    GpuCompressionResult res;
    res.algorithm     = GpuCompressionAlgorithm::ZSTD;
    res.original_size = size;
    res.used_gpu      = false;

    res.data = utils::zstd_compress(data, size, config_.zstd_level);
    if (!res.data.empty()) {
        res.compression_ratio =
            static_cast<float>(size) / static_cast<float>(res.data.size());
        res.success = true;
    } else {
        res.data.assign(data, data + size);
        res.compression_ratio = 1.0f;
        res.error_message     = "zstd_compress returned empty";
    }
    return res;
}

std::vector<uint8_t> GpuCompressionManager::cpu_decompress_zstd(
    const std::vector<uint8_t>& data, size_t /*original_size*/)
{
    return utils::zstd_decompress(data);
}

// ------------------------------------------------------------------
// Snappy (Google Snappy library)
// ------------------------------------------------------------------

GpuCompressionResult GpuCompressionManager::cpu_compress_snappy(
    const uint8_t* data, size_t size)
{
    GpuCompressionResult res;
    res.algorithm     = GpuCompressionAlgorithm::SNAPPY;
    res.original_size = size;
    res.used_gpu      = false;

    std::string compressed_str;
    snappy::Compress(
        reinterpret_cast<const char*>(data), size, &compressed_str);

    if (!compressed_str.empty()) {
        res.data.assign(
            reinterpret_cast<const uint8_t*>(compressed_str.data()),
            reinterpret_cast<const uint8_t*>(
                compressed_str.data() + compressed_str.size()));
        res.compression_ratio =
            static_cast<float>(size) / static_cast<float>(res.data.size());
        res.success = true;
    } else {
        res.data.assign(data, data + size);
        res.compression_ratio = 1.0f;
        res.error_message     = "snappy::Compress returned empty";
    }
    return res;
}

std::vector<uint8_t> GpuCompressionManager::cpu_decompress_snappy(
    const std::vector<uint8_t>& data, size_t /*original_size*/)
{
    std::string decompressed;
    bool ok = snappy::Uncompress(
        reinterpret_cast<const char*>(data.data()), data.size(),
        &decompressed);
    if (!ok) {
        spdlog::error("[gpu_compress] snappy::Uncompress failed");
        return {};
    }
    return std::vector<uint8_t>(
        reinterpret_cast<const uint8_t*>(decompressed.data()),
        reinterpret_cast<const uint8_t*>(
            decompressed.data() + decompressed.size()));
}

// ------------------------------------------------------------------
// LZ4 (liblz4)
// The compressed format stores the original size as an 8-byte header
// so that decompression can allocate the correct output buffer without
// the caller needing to track it separately.
// ------------------------------------------------------------------

static constexpr size_t kLz4HeaderSize = sizeof(uint64_t);

GpuCompressionResult GpuCompressionManager::cpu_compress_lz4(
    const uint8_t* data, size_t size)
{
    GpuCompressionResult res;
    res.algorithm     = GpuCompressionAlgorithm::LZ4;
    res.original_size = size;
    res.used_gpu      = false;

    if (size > static_cast<size_t>(std::numeric_limits<int>::max())) {
        res.error_message = "LZ4: input too large";
        res.data.assign(data, data + size);
        res.compression_ratio = 1.0f;
        return res;
    }

    int max_compressed = LZ4_compressBound(static_cast<int>(size));
    if (max_compressed <= 0) {
        res.error_message = "LZ4_compressBound returned <= 0";
        res.data.assign(data, data + size);
        res.compression_ratio = 1.0f;
        return res;
    }

    res.data.resize(kLz4HeaderSize + static_cast<size_t>(max_compressed));

    // Write original size header (little-endian)
    uint64_t orig64 = static_cast<uint64_t>(size);
    memcpy(res.data.data(), &orig64, kLz4HeaderSize);

    int compressed_bytes = LZ4_compress_default(
        reinterpret_cast<const char*>(data),
        reinterpret_cast<char*>(res.data.data() + kLz4HeaderSize),
        static_cast<int>(size),
        max_compressed);

    if (compressed_bytes > 0) {
        res.data.resize(kLz4HeaderSize + static_cast<size_t>(compressed_bytes));
        res.compression_ratio =
            static_cast<float>(size) / static_cast<float>(res.data.size());
        res.success = true;
    } else {
        res.data.assign(data, data + size);
        res.compression_ratio = 1.0f;
        res.error_message     = "LZ4_compress_default returned 0";
    }
    return res;
}

std::vector<uint8_t> GpuCompressionManager::cpu_decompress_lz4(
    const std::vector<uint8_t>& data, size_t original_size)
{
    if (data.size() < kLz4HeaderSize) {
        spdlog::error("[gpu_compress] LZ4 decompression: data too short for header");
        return {};
    }

    // Read original size from header
    uint64_t orig64 = 0;
    memcpy(&orig64, data.data(), kLz4HeaderSize);
    size_t expected_size = (original_size > 0) ? original_size
                                               : static_cast<size_t>(orig64);

    if (expected_size == 0) {
        spdlog::error("[gpu_compress] LZ4 decompression: unknown original size");
        return {};
    }

    std::vector<uint8_t> result(expected_size);
    int decompressed = LZ4_decompress_safe(
        reinterpret_cast<const char*>(data.data() + kLz4HeaderSize),
        reinterpret_cast<char*>(result.data()),
        static_cast<int>(data.size() - kLz4HeaderSize),
        static_cast<int>(expected_size));

    if (decompressed < 0) {
        spdlog::error("[gpu_compress] LZ4_decompress_safe failed ({})", decompressed);
        return {};
    }

    result.resize(static_cast<size_t>(decompressed));
    return result;
}

// ============================================================================
// Runtime introspection
// ============================================================================

bool GpuCompressionManager::is_gpu_available() const
{
    return !force_cpu_ && impl_ && impl_->is_available();
}

GpuAccelerationType GpuCompressionManager::active_accel_type() const
{
    if (force_cpu_ || !impl_ || !impl_->is_available()) {
        return GpuAccelerationType::CPU_ONLY;
    }
    return active_accel_;
}

void GpuCompressionManager::force_cpu_fallback(bool enable)
{
    force_cpu_ = enable;
    if (enable) {
        spdlog::info("[gpu_compress] CPU-only mode forced");
    }
}

void GpuCompressionManager::set_config(const GpuCompressionConfig& cfg)
{
    config_ = cfg;
}

void GpuCompressionManager::reset_stats()
{
    stats_ = Stats{};
}

// ============================================================================
// String helpers
// ============================================================================

std::string GpuCompressionManager::algorithm_to_string(
    GpuCompressionAlgorithm algorithm)
{
    switch (algorithm) {
        case GpuCompressionAlgorithm::ZSTD:   return "gpu_zstd";
        case GpuCompressionAlgorithm::SNAPPY: return "gpu_snappy";
        case GpuCompressionAlgorithm::LZ4:    return "gpu_lz4";
    }
    return "unknown";
}

std::string GpuCompressionManager::accel_type_to_string(
    GpuAccelerationType type)
{
    switch (type) {
        case GpuAccelerationType::CPU_ONLY: return "cpu_only";
        case GpuAccelerationType::CUDA:     return "cuda";
        case GpuAccelerationType::HIP:      return "hip";
        case GpuAccelerationType::AUTO:     return "auto";
    }
    return "unknown";
}

// ============================================================================
// Factory
// ============================================================================

std::unique_ptr<GpuCompressionManager> create_gpu_compression_manager(
    const GpuCompressionConfig& config)
{
    return std::make_unique<GpuCompressionManager>(config);
}

} // namespace storage
} // namespace themis
