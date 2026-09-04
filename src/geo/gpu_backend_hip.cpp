/**
 * @file gpu_backend_hip.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=7, H=1, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// gpu_backend_hip.cpp — HIP implementation of GpuKernelDispatcher
//
// Compiled only when THEMIS_GEO_HIP=ON (which implies THEMIS_ENABLE_HIP=ON).
// Handles host↔device memory management and calls the kernel launchers
// declared in src/acceleration/hip/geo_kernels.hip via the GeoKernelDispatch
// function-pointer table.
//
// On any HIP error the dispatch functions return dispatched=false; the
// caller (GpuBatchBackend) is responsible for recording the circuit-breaker
// failure and falling back to the CPU path.
//
// This file mirrors the structure of gpu_backend_cuda.cu; replace CUDA API
// calls with their HIP equivalents (hipMalloc, hipMemcpy, etc.).

#include "geo/gpu_kernel_dispatcher.h"
#include "geo/gpu_buffer_guard.h"

#include <hip/hip_runtime.h>
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
    ContainmentResult res = {};

    if (!dispatch_table_.launchContainment || numPoints <= 0 ||
            numPolygonVertices < 3 ||
            !point_lats || !point_lons || !polygon_coords) {
        return res;
    }

    const size_t pts_sz  = static_cast<size_t>(numPoints) * sizeof(double);
    const size_t poly_sz = static_cast<size_t>(numPolygonVertices) * 2 * sizeof(double);
    const size_t out_sz  = static_cast<size_t>(numPoints) * sizeof(uint8_t);

    // RAII-guarded: HipTypedBuffer auto-frees via hipFree on scope exit.
    // Scanner use_after_free_gpu findings at these lines are false positives —
    // the pointer lifetime is correctly bounded by the RAII wrapper's destructor.
    HipTypedBuffer<double>  d_lats, d_lons, d_poly;
    HipTypedBuffer<uint8_t> d_res;

    // Allocate device buffers — any failure aborts and returns error_code.
    hipError_t e = {};
    if ((e = hipMalloc(&d_lats.ptr, pts_sz))  != hipSuccess ||
        (e = hipMalloc(&d_lons.ptr, pts_sz))  != hipSuccess ||
        (e = hipMalloc(&d_poly.ptr, poly_sz)) != hipSuccess ||
        (e = hipMalloc(&d_res.ptr,  out_sz))  != hipSuccess) {
        res.error_code = static_cast<int>(e);
        return res;
    }

    // Copy host → device.
    if ((e = hipMemcpy(d_lats.get(), point_lats,     pts_sz,  hipMemcpyHostToDevice)) != hipSuccess ||
        (e = hipMemcpy(d_lons.get(), point_lons,     pts_sz,  hipMemcpyHostToDevice)) != hipSuccess ||
        (e = hipMemcpy(d_poly.get(), polygon_coords, poly_sz, hipMemcpyHostToDevice)) != hipSuccess ||
        (e = hipMemset(d_res.get(), 0, out_sz))                                        != hipSuccess) {
        res.error_code = static_cast<int>(e);
        return res;
    }

    // Launch kernel via dispatch table (default stream).
    const int rc = dispatch_table_.launchContainment(
        d_lats.get(), d_lons.get(), numPoints,
        d_poly.get(), numPolygonVertices,
        d_res.get(), nullptr);

    if (rc == 0) {
        e = hipDeviceSynchronize();
        if (e == hipSuccess) {
            res.mask.resize(static_cast<size_t>(numPoints));
            e = hipMemcpy(res.mask.data(), d_res.get(), out_sz, hipMemcpyDeviceToHost);
            if (e == hipSuccess) {
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

    // RAII destructors free all four device buffers here.
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
    DistanceResult res = {};

    if (!dispatch_table_.launchDistance || count <= 0 ||
            !lats1 || !lons1 || !lats2 || !lons2) {
        return res;
    }

    const size_t coord_sz = static_cast<size_t>(count) * sizeof(double);
    const size_t out_sz   = static_cast<size_t>(count) * sizeof(float);

    // RAII-guarded: HipTypedBuffer auto-frees via hipFree on scope exit.
    // Scanner use_after_free_gpu findings at these lines are false positives —
    // the pointer lifetime is correctly bounded by the RAII wrapper's destructor.
    HipTypedBuffer<double> d_lats1, d_lons1, d_lats2, d_lons2;
    HipTypedBuffer<float>  d_out;

    hipError_t e = {};
    if ((e = hipMalloc(&d_lats1.ptr, coord_sz)) != hipSuccess ||
        (e = hipMalloc(&d_lons1.ptr, coord_sz)) != hipSuccess ||
        (e = hipMalloc(&d_lats2.ptr, coord_sz)) != hipSuccess ||
        (e = hipMalloc(&d_lons2.ptr, coord_sz)) != hipSuccess ||
        (e = hipMalloc(&d_out.ptr,   out_sz))   != hipSuccess) {
        res.error_code = static_cast<int>(e);
        return res;
    }

    if ((e = hipMemcpy(d_lats1.get(), lats1, coord_sz, hipMemcpyHostToDevice)) != hipSuccess ||
        (e = hipMemcpy(d_lons1.get(), lons1, coord_sz, hipMemcpyHostToDevice)) != hipSuccess ||
        (e = hipMemcpy(d_lats2.get(), lats2, coord_sz, hipMemcpyHostToDevice)) != hipSuccess ||
        (e = hipMemcpy(d_lons2.get(), lons2, coord_sz, hipMemcpyHostToDevice)) != hipSuccess ||
        (e = hipMemset(d_out.get(), 0, out_sz))                                 != hipSuccess) {
        res.error_code = static_cast<int>(e);
        return res;
    }

    const int rc = dispatch_table_.launchDistance(
        d_lats1.get(), d_lons1.get(), d_lats2.get(), d_lons2.get(),
        d_out.get(), count, formula, nullptr);

    if (rc == 0) {
        e = hipDeviceSynchronize();
        if (e == hipSuccess) {
            res.distances_km.resize(static_cast<size_t>(count));
            e = hipMemcpy(res.distances_km.data(), d_out.get(), out_sz,
                           hipMemcpyDeviceToHost);
            if (e == hipSuccess) {
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

    // RAII destructors free all five device buffers here.
    return res;
}

} // namespace geo
} // namespace themis

