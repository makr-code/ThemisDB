// gpu_backend_cuda.cu — CUDA implementation of GpuKernelDispatcher
//
// Compiled only when THEMIS_GEO_CUDA=ON (which implies THEMIS_ENABLE_CUDA=ON).
// Handles host↔device memory management and calls the kernel launchers
// declared in src/acceleration/cuda/geo_kernels.cu via the GeoKernelDispatch
// function-pointer table.
//
// On any CUDA error the dispatch functions return dispatched=false; the
// caller (GpuBatchBackend) is responsible for recording the circuit-breaker
// failure and falling back to the CPU path.

#include "geo/gpu_kernel_dispatcher.h"

#include <cuda_runtime.h>
#include <cstdint>
#include <cstring>

namespace themis {
namespace geo {

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

GpuKernelDispatcher::GpuKernelDispatcher(
    const themis::acceleration::GeoKernelDispatch& dt) noexcept
    : dispatch_table_(dt)
{}

// ---------------------------------------------------------------------------
// isAvailable
// ---------------------------------------------------------------------------

bool GpuKernelDispatcher::isAvailable() const noexcept {
    return dispatch_table_.launchContainment != nullptr ||
           dispatch_table_.launchDistance    != nullptr;
}

// ---------------------------------------------------------------------------
// dispatchContainment
// ---------------------------------------------------------------------------

GpuKernelDispatcher::ContainmentResult GpuKernelDispatcher::dispatchContainment(
    const double* point_lats,
    const double* point_lons,
    int           numPoints,
    const double* polygon_coords,
    int           numPolygonVertices
) {
    ContainmentResult res;

    if (!dispatch_table_.launchContainment || numPoints <= 0 ||
            numPolygonVertices < 3 ||
            !point_lats || !point_lons || !polygon_coords) {
        return res;
    }

    const size_t pts_sz  = static_cast<size_t>(numPoints) * sizeof(double);
    const size_t poly_sz = static_cast<size_t>(numPolygonVertices) * 2 * sizeof(double);
    const size_t out_sz  = static_cast<size_t>(numPoints) * sizeof(uint8_t);

    double*  d_lats = nullptr;
    double*  d_lons = nullptr;
    double*  d_poly = nullptr;
    uint8_t* d_res  = nullptr;

    // Allocate device buffers — any failure aborts and returns error_code.
    cudaError_t e;
    if ((e = cudaMalloc(&d_lats, pts_sz))  != cudaSuccess ||
        (e = cudaMalloc(&d_lons, pts_sz))  != cudaSuccess ||
        (e = cudaMalloc(&d_poly, poly_sz)) != cudaSuccess ||
        (e = cudaMalloc(&d_res,  out_sz))  != cudaSuccess) {
        res.error_code = static_cast<int>(e);
        cudaFree(d_lats); cudaFree(d_lons);
        cudaFree(d_poly); cudaFree(d_res);
        return res;
    }

    // Copy host → device.
    if ((e = cudaMemcpy(d_lats, point_lats,     pts_sz,  cudaMemcpyHostToDevice)) != cudaSuccess ||
        (e = cudaMemcpy(d_lons, point_lons,     pts_sz,  cudaMemcpyHostToDevice)) != cudaSuccess ||
        (e = cudaMemcpy(d_poly, polygon_coords, poly_sz, cudaMemcpyHostToDevice)) != cudaSuccess ||
        (e = cudaMemset(d_res, 0, out_sz))                                        != cudaSuccess) {
        res.error_code = static_cast<int>(e);
        cudaFree(d_lats); cudaFree(d_lons);
        cudaFree(d_poly); cudaFree(d_res);
        return res;
    }

    // Launch kernel via dispatch table (default stream).
    const int rc = dispatch_table_.launchContainment(
        d_lats, d_lons, numPoints,
        d_poly, numPolygonVertices,
        d_res, nullptr);

    if (rc == 0) {
        e = cudaDeviceSynchronize();
        if (e == cudaSuccess) {
            res.mask.resize(static_cast<size_t>(numPoints));
            e = cudaMemcpy(res.mask.data(), d_res, out_sz, cudaMemcpyDeviceToHost);
            if (e == cudaSuccess) {
                res.dispatched = true;
            } else {
                res.error_code = static_cast<int>(e);
                res.mask.clear();
            }
        } else {
            res.error_code = static_cast<int>(e);
        }
    } else {
        res.error_code = rc;
    }

    cudaFree(d_lats); cudaFree(d_lons);
    cudaFree(d_poly); cudaFree(d_res);
    return res;
}

// ---------------------------------------------------------------------------
// dispatchDistance
// ---------------------------------------------------------------------------

GpuKernelDispatcher::DistanceResult GpuKernelDispatcher::dispatchDistance(
    const double* lats1,
    const double* lons1,
    const double* lats2,
    const double* lons2,
    int           count,
    themis::acceleration::GeoDistanceFormula formula
) {
    DistanceResult res;

    if (!dispatch_table_.launchDistance || count <= 0 ||
            !lats1 || !lons1 || !lats2 || !lons2) {
        return res;
    }

    const size_t coord_sz = static_cast<size_t>(count) * sizeof(double);
    const size_t out_sz   = static_cast<size_t>(count) * sizeof(float);

    double* d_lats1 = nullptr;
    double* d_lons1 = nullptr;
    double* d_lats2 = nullptr;
    double* d_lons2 = nullptr;
    float*  d_out   = nullptr;

    cudaError_t e;
    if ((e = cudaMalloc(&d_lats1, coord_sz)) != cudaSuccess ||
        (e = cudaMalloc(&d_lons1, coord_sz)) != cudaSuccess ||
        (e = cudaMalloc(&d_lats2, coord_sz)) != cudaSuccess ||
        (e = cudaMalloc(&d_lons2, coord_sz)) != cudaSuccess ||
        (e = cudaMalloc(&d_out,   out_sz))   != cudaSuccess) {
        res.error_code = static_cast<int>(e);
        cudaFree(d_lats1); cudaFree(d_lons1);
        cudaFree(d_lats2); cudaFree(d_lons2);
        cudaFree(d_out);
        return res;
    }

    if ((e = cudaMemcpy(d_lats1, lats1, coord_sz, cudaMemcpyHostToDevice)) != cudaSuccess ||
        (e = cudaMemcpy(d_lons1, lons1, coord_sz, cudaMemcpyHostToDevice)) != cudaSuccess ||
        (e = cudaMemcpy(d_lats2, lats2, coord_sz, cudaMemcpyHostToDevice)) != cudaSuccess ||
        (e = cudaMemcpy(d_lons2, lons2, coord_sz, cudaMemcpyHostToDevice)) != cudaSuccess ||
        (e = cudaMemset(d_out, 0, out_sz))                                 != cudaSuccess) {
        res.error_code = static_cast<int>(e);
        cudaFree(d_lats1); cudaFree(d_lons1);
        cudaFree(d_lats2); cudaFree(d_lons2);
        cudaFree(d_out);
        return res;
    }

    const int rc = dispatch_table_.launchDistance(
        d_lats1, d_lons1, d_lats2, d_lons2,
        d_out, count, formula, nullptr);

    if (rc == 0) {
        e = cudaDeviceSynchronize();
        if (e == cudaSuccess) {
            res.distances_km.resize(static_cast<size_t>(count));
            e = cudaMemcpy(res.distances_km.data(), d_out, out_sz,
                           cudaMemcpyDeviceToHost);
            if (e == cudaSuccess) {
                res.dispatched = true;
            } else {
                res.error_code = static_cast<int>(e);
                res.distances_km.clear();
            }
        } else {
            res.error_code = static_cast<int>(e);
        }
    } else {
        res.error_code = rc;
    }

    cudaFree(d_lats1); cudaFree(d_lons1);
    cudaFree(d_lats2); cudaFree(d_lons2);
    cudaFree(d_out);
    return res;
}

} // namespace geo
} // namespace themis
