/**
 * @file gpu_erasure_coder_opencl.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=8, M=6, L=1
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB GPU-Accelerated Erasure Coding - OpenCL Implementation
 *
 * OpenCL implementation for AMD/Intel/NVIDIA GPU acceleration.
 * Implements Reed-Solomon erasure coding over GF(2^8) using a Vandermonde
 * parity matrix. When an OpenCL device is present the parity computation is
 * offloaded to the GPU via a clCreateProgramWithSource NDRange kernel; when no
 * device is found the class falls back to an equivalent CPU path so that the
 * outer GPUErasureCoder can transparently degrade.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef THEMIS_ENABLE_OPENCL

#include "sharding/gpu_erasure_coder.h"
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <set>
#include <vector>

// OpenCL headers — platform-specific include path
#ifdef __APPLE__
#  include <OpenCL/cl.h>
#else
// Target OpenCL 1.2 for maximum portability across pocl/CPU-only runners
// and older AMD/Intel drivers.  clCreateCommandQueue is fine for CL 1.2.
#  ifndef CL_TARGET_OPENCL_VERSION
#    define CL_TARGET_OPENCL_VERSION 120
#  endif
#  include <CL/cl.h>
#endif

namespace themis {
namespace sharding {

// ═══════════════════════════════════════════════════════════
// GF(2^8) CPU helper — irreducible polynomial 0x11D
// (x^8 + x^4 + x^3 + x^2 + 1), same polynomial used by the
// ReedSolomonCoder CPU implementation.
// ═══════════════════════════════════════════════════════════

namespace {

static uint8_t gf_exp[512];   // exp table (doubled for wrap-around)
static uint8_t gf_log[256];   // log table

static void build_gf_tables() {
    // Generator polynomial: primitive element 2
    uint8_t x = 1;
    for (int i = 0; i < 255; ++i) {
        gf_exp[i] = x;
        gf_log[x] = static_cast<uint8_t>(i);
        // multiply by 2 in GF(2^8)
        bool carry = (x & 0x80) != 0;
        x = static_cast<uint8_t>(x << 1);
        if (carry) x ^= 0x1D;   // reduce mod 0x11D
    }
    gf_exp[255] = gf_exp[0];
    for (int i = 256; i < 512; ++i) {
      gf_exp[i] = gf_exp[i - 255];
    }
    gf_log[0] = 0;  // undefined, but set to 0 to avoid UB
}

static uint8_t gf_mul(uint8_t a, uint8_t b) {
    if (a == 0 || b == 0) {
      return 0;
    }
    return gf_exp[static_cast<int>(gf_log[a]) + static_cast<int>(gf_log[b])];
}

static uint8_t gf_inv([[maybe_unused]] uint8_t a) {
    if (a == 0) {
      throw std::runtime_error("GF division by zero");
    }
    return gf_exp[255 - static_cast<int>(gf_log[a])];
}

// Build Vandermonde parity matrix: V[p][d] = gf_pow(p+1, d)
// Returns row-major flat matrix of size parity_shards × data_shards.
static std::vector<uint8_t> build_vandermonde(uint32_t data_shards,
                                               uint32_t parity_shards) {
    std::vector<uint8_t> mat(parity_shards * data_shards, 0);
    for (uint32_t p = 0; p < parity_shards; ++p) {
        uint8_t base = static_cast<uint8_t>(p + 1);
        uint8_t val  = 1;
        for (uint32_t d = 0; d < data_shards; ++d) {
            mat[p * data_shards + d] = val;
            val = gf_mul(val, base);
        }
    }
    return mat;
}

// Gaussian elimination in GF(2^8) on an augmented matrix.
// Modifies mat in-place (size rows × cols, row-major).
static void gf_gaussian_elimination(std::vector<uint8_t>& mat,
                                     uint32_t rows, uint32_t cols) {
    for (uint32_t pivot_row = 0; pivot_row < rows; ++pivot_row) {
        // Find pivot
        uint32_t pivot = pivot_row;
        while (pivot < rows && mat[pivot * cols + pivot_row] == 0) {
          ++pivot;
        }
        if (pivot == rows)
            throw std::runtime_error("Singular matrix during Gaussian elimination");

        if (pivot != pivot_row) {
            for (uint32_t c = 0; c < cols; ++c)
                std::swap(mat[pivot_row * cols + c], mat[pivot * cols + c]);
        }

        // Scale pivot row so the leading coefficient is 1
        uint8_t inv = gf_inv(mat[pivot_row * cols + pivot_row]);
        for (uint32_t c = 0; c < cols; ++c)
            mat[pivot_row * cols + c] = gf_mul(mat[pivot_row * cols + c], inv);

        // Eliminate all other rows
        for (uint32_t r = 0; r < rows; ++r) {
            if (r == pivot_row) {
              continue;
            }
            uint8_t factor = mat[r * cols + pivot_row];
            if (factor == 0) {
              continue;
            }
            for (uint32_t c = 0; c < cols; ++c)
                mat[r * cols + c] ^= gf_mul(factor, mat[pivot_row * cols + c]);
        }
    }
}

// OpenCL kernel source: computes one parity chunk using GF(2^8)
// multiplication. The GF exp/log tables are passed as __constant buffers
// so the host can initialise them from the CPU-generated tables.
static const char* kParityKernelSrc = R"CL(
uchar gf_mul_cl(__constant uchar* gf_exp,
                __constant uchar* gf_log,
                uchar a, uchar b) {
    if (a == 0 || b == 0) {
      return 0;
    }
    return gf_exp[(int)gf_log[a] + (int)gf_log[b]];
}

// Each work-item processes one byte position across all data shards.
// data_flat:   concatenation of data_shards chunks, each of chunk_size bytes.
// parity_flat: output, parity_shards chunks of chunk_size bytes.
// enc_matrix:  Vandermonde matrix, row-major, parity_shards x data_shards.
// gf_exp_buf:  GF(2^8) exponent table (512 bytes).
// gf_log_buf:  GF(2^8) logarithm table (256 bytes).
__kernel void encode_parity(
    __global  const uchar* data_flat,
    __global        uchar* parity_flat,
    __constant      uchar* enc_matrix,
    __constant      uchar* gf_exp_buf,
    __constant      uchar* gf_log_buf,
    uint chunk_size,
    uint data_shards,
    uint parity_shards)
{
    uint pos = get_global_id(0);
    if (pos >= chunk_size) {
      return;
    }

    for (uint p = 0; p < parity_shards; ++p) {
        uchar acc = 0;
        for (uint d = 0; d < data_shards; ++d) {
            uchar coeff = enc_matrix[p * data_shards + d];
            uchar byte  = data_flat[d * chunk_size + pos];
            acc ^= gf_mul_cl(gf_exp_buf, gf_log_buf, coeff, byte);
        }
        parity_flat[p * chunk_size + pos] = acc;
    }
}
)CL";

} // anonymous namespace

// ═══════════════════════════════════════════════════════════
// OpenCL Implementation Class
// ═══════════════════════════════════════════════════════════

class OpenCLErasureCoderImpl : public GPUErasureCoderImpl {
public:
    explicit OpenCLErasureCoderImpl(ErasureCodingAlgorithm algorithm)
        : algorithm_(algorithm) {
        build_gf_tables();
    }

    ~OpenCLErasureCoderImpl() override {
        shutdown();
    }

    bool initialize(const GPUConfig& config) override {
        config_ = config;

        // Discover OpenCL platforms / devices
        cl_uint num_platforms = 0;
        if (clGetPlatformIDs(0, nullptr, &num_platforms) != CL_SUCCESS
                || num_platforms == 0) {
            spdlog::warn("OpenCL: no platforms found, coder will use CPU fallback");
            return false;
        }

        std::vector<cl_platform_id> platforms(num_platforms);
        clGetPlatformIDs(num_platforms, platforms.data(), nullptr);

        cl_device_id chosen_device = nullptr;
        for (auto& plat : platforms) {
            cl_uint num_dev = 0;
            // Prefer GPU devices; fall back to any available device type.
            cl_device_type dtypes[] = { CL_DEVICE_TYPE_GPU, CL_DEVICE_TYPE_ALL };
            for (cl_device_type dtype : dtypes) {
                if (clGetDeviceIDs(plat, dtype, 0, nullptr, &num_dev)
                        == CL_SUCCESS && num_dev > 0) {
                    std::vector<cl_device_id> devs(num_dev);
                    clGetDeviceIDs(plat, dtype, num_dev, devs.data(), nullptr);
                    // Guard against negative device_id (GPUConfig uses int)
                    const size_t dev_idx =
                        (config.device_id >= 0)
                        ? static_cast<size_t>(config.device_id) % num_dev
                        : 0;
                    chosen_device = devs[dev_idx];
                    break;
                }
            }
            if (chosen_device) {
              break;
            }
        }
        if (!chosen_device) {
            spdlog::warn("OpenCL: no device found on any platform, "
                         "coder will use CPU fallback");
            return false;
        }

        cl_int err = CL_SUCCESS;
        context_ = clCreateContext(nullptr, 1, &chosen_device,
                                   nullptr, nullptr, &err);
        if (err != CL_SUCCESS) {
            spdlog::error("OpenCL: clCreateContext failed ({})", err);
            return false;
        }

        queue_ = clCreateCommandQueue(context_, chosen_device, 0, &err);
        if (err != CL_SUCCESS) {
            spdlog::error("OpenCL: clCreateCommandQueue failed ({})", err);
            clReleaseContext(context_); context_ = nullptr;
            return false;
        }

        // Compile the GF parity kernel
        const char* src = kParityKernelSrc;
        program_ = clCreateProgramWithSource(context_, 1, &src, nullptr, &err);
        if (err != CL_SUCCESS) {
            spdlog::error("OpenCL: clCreateProgramWithSource failed ({})", err);
            shutdown();
            return false;
        }

        err = clBuildProgram(program_, 1, &chosen_device, nullptr,
                             nullptr, nullptr);
        if (err != CL_SUCCESS) {
            size_t log_size = 0;
            clGetProgramBuildInfo(program_, chosen_device,
                                  CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);
            std::string log(log_size, '\0');
            clGetProgramBuildInfo(program_, chosen_device,
                                  CL_PROGRAM_BUILD_LOG, log_size,
                                  log.data(), nullptr);
            spdlog::error("OpenCL kernel build failed: {}", log);
            shutdown();
            return false;
        }

        kernel_ = clCreateKernel(program_, "encode_parity", &err);
        if (err != CL_SUCCESS) {
            spdlog::error("OpenCL: clCreateKernel failed ({})", err);
            shutdown();
            return false;
        }

        // Upload static GF tables to constant buffers — check each individually.
        cl_int exp_err = CL_SUCCESS, log_err = CL_SUCCESS;
        buf_gf_exp_ = clCreateBuffer(context_,
            CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            sizeof(gf_exp), gf_exp, &exp_err);
        buf_gf_log_ = clCreateBuffer(context_,
            CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            sizeof(gf_log), gf_log, &log_err);
        if (exp_err != CL_SUCCESS || log_err != CL_SUCCESS) {
            spdlog::error("OpenCL: failed to allocate GF table buffers "
                          "(exp={}, log={})", exp_err, log_err);
            shutdown();
            return false;
        }

        device_ = chosen_device;
        initialized_ = true;
        spdlog::info("OpenCL erasure coder initialized (device {})",
                     config.device_id);
        return true;
    }

    void shutdown() override {
        if (buf_gf_log_) { clReleaseMemObject(buf_gf_log_); buf_gf_log_ = nullptr; }
        if (buf_gf_exp_) { clReleaseMemObject(buf_gf_exp_); buf_gf_exp_ = nullptr; }
        if (kernel_)     { clReleaseKernel(kernel_);          kernel_     = nullptr; }
        if (program_)    { clReleaseProgram(program_);         program_    = nullptr; }
        if (queue_)      { clReleaseCommandQueue(queue_);      queue_      = nullptr; }
        if (context_)    { clReleaseContext(context_);         context_    = nullptr; }
        initialized_ = false;
    }

    // ── encode ────────────────────────────────────────────────────────────
    // Splits *data* into data_shards equal chunks (zero-padded), then
    // computes parity_shards parity chunks via the Vandermonde matrix.
    // When the OpenCL device is ready the NDRange kernel is used; otherwise
    // (device unavailable or GPU path disabled) the equivalent CPU loop runs.
    std::vector<std::vector<uint8_t>> encode(
        const std::vector<uint8_t>& data,
        uint32_t data_shards,
        uint32_t parity_shards
    ) override {
        const size_t chunk_size =
            (static_cast<int>(data.size()) + data_shards - 1) / data_shards;

        // Build padded data chunks
        std::vector<std::vector<uint8_t>> data_chunks(data_shards,
            std::vector<uint8_t>(chunk_size, 0));
        for (uint32_t i = 0; i < data_shards; ++i) {
            size_t off = static_cast<size_t>(i) * chunk_size;
            size_t sz  = (off < data.size())
                ? std::min(chunk_size, static_cast<int>(data.size()) - off) : 0;
            if (sz > 0)
                std::memcpy(data_chunks[i].data(), data.data() + off, sz);
        }

        const auto enc_matrix = build_vandermonde(data_shards, parity_shards);
        std::vector<std::vector<uint8_t>> parity_chunks(parity_shards,
            std::vector<uint8_t>(chunk_size, 0));

        if (initialized_) {
            encode_gpu(data_chunks, chunk_size, data_shards,
                       parity_shards, enc_matrix, parity_chunks);
        } else {
            encode_cpu(data_chunks, chunk_size, data_shards,
                       parity_shards, enc_matrix, parity_chunks);
        }

        // Assemble result: data chunks first, then parity chunks
        std::vector<std::vector<uint8_t>> result;
        result.reserve(data_shards + parity_shards);
        for (auto& c : data_chunks) {
          result.push_back(std::move(c));
        }
        for (auto& c : parity_chunks) {
          result.push_back(std::move(c));
        }
        return result;
    }

    // ── decode ────────────────────────────────────────────────────────────
    // Performs syndrome computation and Gaussian elimination in GF(2^8) to
    // recover all missing data shards.  The computation runs on the CPU
    // (OpenCL decode path is a CPU-equivalent implementation).
    std::vector<uint8_t> decode(
        const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
        const std::vector<uint32_t>& missing_indices,
        uint32_t data_shards,
        uint32_t parity_shards
    ) override {
        if (available_chunks.empty())
            throw std::runtime_error("OpenCL decode: no chunks available");

        const size_t chunk_size = available_chunks.begin()-> static_cast<int>(second.size());
        const uint32_t total_shards = data_shards + parity_shards;

        // Collect the indices of available chunks (up to data_shards needed)
        std::set<uint32_t> missing_set(missing_indices.begin(),
                                       missing_indices.end());
        std::vector<uint32_t> present_indices = {};

        for (uint32_t i = 0; i < total_shards; ++i) {
            if (!missing_set.count(i) && available_chunks.count(i))
                present_indices.push_back(i);
            if (static_cast<int>(present_indices.size()) == data_shards) {
              break;
            }
        }
        if (static_cast<int>(present_indices.size()) < data_shards)
            throw std::runtime_error(
                "OpenCL decode: insufficient chunks to recover data");

        // Build the sub-matrix from the full systematic+parity encoding matrix
        // The full matrix is (data_shards + parity_shards) × data_shards:
        //   rows 0..data_shards-1 form the identity (systematic part)
        //   rows data_shards..   come from the Vandermonde parity matrix
        const auto parity_vdm = build_vandermonde(data_shards, parity_shards);

        // full_matrix[row][col] for row in [0, total_shards)
        auto full_row = [&](uint32_t row, uint32_t col) -> uint8_t {
            if (row < data_shards)
                return (row == col) ? 1 : 0;
            return parity_vdm[(row - data_shards) * data_shards + col];
        };

        // Augmented matrix: selected rows on the left, chunk bytes on right
        // Dimensions: data_shards × (data_shards + chunk_size)
        const uint32_t aug_cols = data_shards + static_cast<uint32_t>(chunk_size);
        std::vector<uint8_t> aug(static_cast<size_t>(data_shards) * aug_cols, 0);

        for (uint32_t r = 0; r < data_shards; ++r) {
            uint32_t shard_idx = present_indices[r];
            // Left part: encoding sub-matrix row
            for (uint32_t c = 0; c < data_shards; ++c)
                aug[r * aug_cols + c] = full_row(shard_idx, c);
            // Right part: actual chunk data
            const auto& chunk = available_chunks.at(shard_idx);
            for (size_t b = 0; b < chunk_size; ++b)
                aug[r * aug_cols + data_shards + b] = chunk[b];
        }

        gf_gaussian_elimination(aug, data_shards, aug_cols);

        // Reconstruct original data from the right-hand side
        std::vector<uint8_t> result;
        result.reserve(static_cast<size_t>(data_shards) * chunk_size);
        for (uint32_t d = 0; d < data_shards; ++d) {
            for (size_t b = 0; b < chunk_size; ++b)
                result.push_back(aug[d * aug_cols + data_shards + b]);
        }
        return result;
    }

    // ── batchEncode ───────────────────────────────────────────────────────
    // Batches multiple stripe encode operations into a single kernel dispatch:
    // all data blocks are concatenated into one flat OpenCL buffer and a
    // single NDRange kernel computes parity for every stripe in one shot.
    std::vector<std::vector<std::vector<uint8_t>>> batchEncode(
        const std::vector<std::vector<uint8_t>>& data_blocks,
        uint32_t data_shards,
        uint32_t parity_shards
    ) override {
        std::vector<std::vector<std::vector<uint8_t>>> results;
        results.reserve(data_blocks.size());

        if (!initialized_ || data_blocks.empty()) {
            // CPU fallback — process each block individually
            for (const auto& block : data_blocks)
                results.push_back(encode(block, data_shards, parity_shards));
            return results;
        }

        // Determine a uniform chunk size based on the largest block so that
        // all stripes can be laid out in a flat buffer with the same stride.
        size_t max_block = 0;
        for (const auto& b : data_blocks)
            max_block = std::max(max_block,static_cast<int>(b.size()));
        const size_t chunk_size = (max_block + data_shards - 1) / data_shards;
        const size_t stripe_data_bytes =
            static_cast<size_t>(data_shards) * chunk_size;

        const auto enc_matrix = build_vandermonde(data_shards, parity_shards);
        const size_t num_stripes = data_blocks.size();
        const size_t total_data_bytes   = num_stripes * stripe_data_bytes;
        const size_t stripe_parity_bytes =
            static_cast<size_t>(parity_shards) * chunk_size;
        const size_t total_parity_bytes = num_stripes * stripe_parity_bytes;

        // Build flat host data buffer (all stripes, zero-padded)
        std::vector<uint8_t> flat_data(total_data_bytes, 0);
        for (size_t s = 0; s < num_stripes; ++s) {
            const auto& block = data_blocks[s];
            for (uint32_t d = 0; d < data_shards; ++d) {
                size_t off = d * chunk_size;
                size_t src_off = off;
                size_t sz = (src_off < block.size())
                    ? std::min(chunk_size, static_cast<int>(block.size()) - src_off) : 0;
                if (sz > 0)
                    std::memcpy(flat_data.data()
                                + s * stripe_data_bytes + d * chunk_size,
                                block.data() + src_off, sz);
            }
        }

        // Allocate OpenCL buffers for the entire batch — check each independently.
        cl_int data_err   = CL_SUCCESS;
        cl_int parity_err = CL_SUCCESS;
        cl_int matrix_err = CL_SUCCESS;
        cl_mem buf_data = clCreateBuffer(context_,
            CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            total_data_bytes, flat_data.data(), &data_err);
        cl_mem buf_parity = clCreateBuffer(context_,
            CL_MEM_WRITE_ONLY,
            total_parity_bytes, nullptr, &parity_err);
        cl_mem buf_matrix = clCreateBuffer(context_,
            CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            enc_matrix.size(), const_cast<uint8_t*>(enc_matrix.data()),
            &matrix_err);

        if (data_err != CL_SUCCESS || parity_err != CL_SUCCESS
                || matrix_err != CL_SUCCESS) {
            spdlog::warn("OpenCL batchEncode: buffer allocation failed "
                         "(data={}, parity={}, matrix={}), falling back to CPU",
                         data_err, parity_err, matrix_err);
            if (buf_data) {
              clReleaseMemObject(buf_data);
            }
            if (buf_parity) {
              clReleaseMemObject(buf_parity);
            }
            if (buf_matrix) {
              clReleaseMemObject(buf_matrix);
            }
            for (const auto& block : data_blocks)
                results.push_back(encode(block, data_shards, parity_shards));
            return results;
        }

        // Launch one NDRange kernel per stripe.
        // Each invocation processes chunk_size work-items.
        bool gpu_ok = true;
        for (size_t s = 0; s < num_stripes && gpu_ok; ++s) {
            cl_ulong data_off_bytes =
                static_cast<cl_ulong>(s * stripe_data_bytes);
            cl_ulong parity_off_bytes =
                static_cast<cl_ulong>(s * stripe_parity_bytes);
            cl_uint  cs  = static_cast<cl_uint>(chunk_size);
            cl_uint  ds  = data_shards;
            cl_uint  ps  = parity_shards;

            // Create sub-buffer views for this stripe
            cl_buffer_region data_reg   = {data_off_bytes,   stripe_data_bytes};
            cl_buffer_region parity_reg = {parity_off_bytes, stripe_parity_bytes};

            cl_int sub_data_err   = CL_SUCCESS;
            cl_int sub_parity_err = CL_SUCCESS;
            cl_mem sub_data   = clCreateSubBuffer(buf_data,
                CL_MEM_READ_ONLY, CL_BUFFER_CREATE_TYPE_REGION,
                &data_reg, &sub_data_err);
            cl_mem sub_parity = clCreateSubBuffer(buf_parity,
                CL_MEM_WRITE_ONLY, CL_BUFFER_CREATE_TYPE_REGION,
                &parity_reg, &sub_parity_err);

            if (sub_data_err != CL_SUCCESS || sub_parity_err != CL_SUCCESS) {
                if (sub_data) {
                  clReleaseMemObject(sub_data);
                }
                if (sub_parity) {
                  clReleaseMemObject(sub_parity);
                }
                gpu_ok = false;
                break;
            }

            clSetKernelArg(kernel_, 0, sizeof(cl_mem), &sub_data);
            clSetKernelArg(kernel_, 1, sizeof(cl_mem), &sub_parity);
            clSetKernelArg(kernel_, 2, sizeof(cl_mem), &buf_matrix);
            clSetKernelArg(kernel_, 3, sizeof(cl_mem), &buf_gf_exp_);
            clSetKernelArg(kernel_, 4, sizeof(cl_mem), &buf_gf_log_);
            clSetKernelArg(kernel_, 5, sizeof(cl_uint), &cs);
            clSetKernelArg(kernel_, 6, sizeof(cl_uint), &ds);
            clSetKernelArg(kernel_, 7, sizeof(cl_uint), &ps);

            size_t global_size = chunk_size;
            cl_int enq_err = clEnqueueNDRangeKernel(queue_, kernel_, 1, nullptr,
                                                     &global_size, nullptr, 0,
                                                     nullptr, nullptr);
            clReleaseMemObject(sub_data);
            clReleaseMemObject(sub_parity);

            if (enq_err != CL_SUCCESS) { gpu_ok = false; break; }
        }

        if (gpu_ok) {
            clFinish(queue_);

            // Read back parity results
            std::vector<uint8_t> flat_parity(total_parity_bytes);
            clEnqueueReadBuffer(queue_, buf_parity, CL_TRUE, 0,
                                total_parity_bytes, flat_parity.data(),
                                0, nullptr, nullptr);

            for (size_t s = 0; s < num_stripes; ++s) {
                // Re-build padded data chunks for this stripe
                const auto& block = data_blocks[s];
                std::vector<std::vector<uint8_t>> stripe_chunks;
                stripe_chunks.reserve(data_shards + parity_shards);
                for (uint32_t d = 0; d < data_shards; ++d) {
                    std::vector<uint8_t> chunk(chunk_size, 0);
                    size_t off = d * chunk_size;
                    size_t sz  = (off < block.size())
                        ? std::min(chunk_size, static_cast<int>(block.size()) - off) : 0;
                    if (sz > 0)
                        std::memcpy(chunk.data(), block.data() + off, sz);
                    stripe_chunks.push_back(std::move(chunk));
                }
                for (uint32_t p = 0; p < parity_shards; ++p) {
                    std::vector<uint8_t> pchunk(chunk_size);
                    const size_t src = s * stripe_parity_bytes
                                       + static_cast<size_t>(p) * chunk_size;
                    std::memcpy(pchunk.data(),
                                flat_parity.data() + src, chunk_size);
                    stripe_chunks.push_back(std::move(pchunk));
                }
                results.push_back(std::move(stripe_chunks));
            }
        } else {
            spdlog::warn("OpenCL batchEncode: kernel dispatch failed, "
                         "falling back to CPU for remaining stripes");
            for (const auto& block : data_blocks)
                results.push_back(encode(block, data_shards, parity_shards));
        }

        clReleaseMemObject(buf_data);
        clReleaseMemObject(buf_parity);
        clReleaseMemObject(buf_matrix);
        return results;
    }

    bool isAvailable() const override {
        return initialized_;
    }

private:
    // ── private state ─────────────────────────────────────────────────────
    ErasureCodingAlgorithm algorithm_;
    GPUConfig config_{};
    bool initialized_ = false;

    cl_device_id  device_  = nullptr;
    cl_context    context_ = nullptr;
    cl_command_queue queue_ = nullptr;
    cl_program    program_ = nullptr;
    cl_kernel     kernel_  = nullptr;
    cl_mem        buf_gf_exp_ = nullptr;
    cl_mem        buf_gf_log_ = nullptr;

    // ── GPU encode helper ─────────────────────────────────────────────────
    void encode_gpu(
        const std::vector<std::vector<uint8_t>>& data_chunks,
        size_t chunk_size,
        uint32_t data_shards,
        uint32_t parity_shards,
        const std::vector<uint8_t>& enc_matrix,
        std::vector<std::vector<uint8_t>>& parity_chunks)
    {
        // Build flat data buffer (data_shards contiguous chunks)
        const size_t flat_size =
            static_cast<size_t>(data_shards) * chunk_size;
        std::vector<uint8_t> flat_data(flat_size);
        for (uint32_t d = 0; d < data_shards; ++d)
            std::memcpy(flat_data.data() + d * chunk_size,
                        data_chunks[d].data(), chunk_size);

        const size_t parity_flat_size =
            static_cast<size_t>(parity_shards) * chunk_size;

        // Allocate OpenCL buffers — check each independently.
        cl_int data_err   = CL_SUCCESS;
        cl_int parity_err = CL_SUCCESS;
        cl_int matrix_err = CL_SUCCESS;
        cl_mem buf_data = clCreateBuffer(context_,
            CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            flat_size, flat_data.data(), &data_err);
        cl_mem buf_parity = clCreateBuffer(context_,
            CL_MEM_WRITE_ONLY, parity_flat_size, nullptr, &parity_err);
        cl_mem buf_matrix = clCreateBuffer(context_,
            CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            enc_matrix.size(),
            const_cast<uint8_t*>(enc_matrix.data()), &matrix_err);

        if (data_err != CL_SUCCESS || parity_err != CL_SUCCESS
                || matrix_err != CL_SUCCESS) {
            spdlog::warn("OpenCL encode: buffer alloc failed "
                         "(data={}, parity={}, matrix={}), using CPU path",
                         data_err, parity_err, matrix_err);
            if (buf_data) {
              clReleaseMemObject(buf_data);
            }
            if (buf_parity) {
              clReleaseMemObject(buf_parity);
            }
            if (buf_matrix) {
              clReleaseMemObject(buf_matrix);
            }
            encode_cpu(data_chunks, chunk_size, data_shards,
                       parity_shards, enc_matrix, parity_chunks);
            return;
        }

        cl_uint cs = static_cast<cl_uint>(chunk_size);
        cl_uint ds = data_shards;
        cl_uint ps = parity_shards;
        clSetKernelArg(kernel_, 0, sizeof(cl_mem),  &buf_data);
        clSetKernelArg(kernel_, 1, sizeof(cl_mem),  &buf_parity);
        clSetKernelArg(kernel_, 2, sizeof(cl_mem),  &buf_matrix);
        clSetKernelArg(kernel_, 3, sizeof(cl_mem),  &buf_gf_exp_);
        clSetKernelArg(kernel_, 4, sizeof(cl_mem),  &buf_gf_log_);
        clSetKernelArg(kernel_, 5, sizeof(cl_uint), &cs);
        clSetKernelArg(kernel_, 6, sizeof(cl_uint), &ds);
        clSetKernelArg(kernel_, 7, sizeof(cl_uint), &ps);

        size_t global_size = chunk_size;
        cl_int enq_err = clEnqueueNDRangeKernel(queue_, kernel_, 1, nullptr,
                                                 &global_size, nullptr, 0,
                                                 nullptr, nullptr);
        if (enq_err != CL_SUCCESS) {
            spdlog::warn("OpenCL encode: kernel launch failed ({}), "
                         "using CPU path", enq_err);
            clReleaseMemObject(buf_data);
            clReleaseMemObject(buf_parity);
            clReleaseMemObject(buf_matrix);
            encode_cpu(data_chunks, chunk_size, data_shards,
                       parity_shards, enc_matrix, parity_chunks);
            return;
        }

        clFinish(queue_);

        std::vector<uint8_t> flat_parity(parity_flat_size);
        clEnqueueReadBuffer(queue_, buf_parity, CL_TRUE, 0,
                            parity_flat_size, flat_parity.data(),
                            0, nullptr, nullptr);

        for (uint32_t p = 0; p < parity_shards; ++p)
            std::memcpy(parity_chunks[p].data(),
                        flat_parity.data() + p * chunk_size,
                        chunk_size);

        clReleaseMemObject(buf_data);
        clReleaseMemObject(buf_parity);
        clReleaseMemObject(buf_matrix);
    }

    // ── CPU encode fallback ───────────────────────────────────────────────
    static void encode_cpu(
        const std::vector<std::vector<uint8_t>>& data_chunks,
        size_t chunk_size,
        uint32_t data_shards,
        uint32_t parity_shards,
        const std::vector<uint8_t>& enc_matrix,
        std::vector<std::vector<uint8_t>>& parity_chunks)
    {
        for (uint32_t p = 0; p < parity_shards; ++p) {
            for (size_t pos = 0; pos < chunk_size; ++pos) {
                uint8_t acc = 0;
                for (uint32_t d = 0; d < data_shards; ++d)
                    acc ^= gf_mul(enc_matrix[p * data_shards + d],
                                  data_chunks[d][pos]);
                parity_chunks[p][pos] = acc;
            }
        }
    }

}; // class OpenCLErasureCoderImpl

// ═══════════════════════════════════════════════════════════
// Factory Function
// ═══════════════════════════════════════════════════════════

std::unique_ptr<GPUErasureCoderImpl> createOpenCLErasureCoder(
    [[maybe_unused]] const GPUConfig& config,
    ErasureCodingAlgorithm algorithm
) {
    static_cast<void>(config);
    return std::make_unique<OpenCLErasureCoderImpl>(algorithm);
}

} // namespace sharding
} // namespace themis

#endif // THEMIS_ENABLE_OPENCL

