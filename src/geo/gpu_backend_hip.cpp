/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            gpu_backend_hip.cpp                                ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-04-15 18:07:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     227                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
    hipError_t e;
    if ((e = hipMalloc(&d_lats, pts_sz))  != hipSuccess ||
        (e = hipMalloc(&d_lons, pts_sz))  != hipSuccess ||
        (e = hipMalloc(&d_poly, poly_sz)) != hipSuccess ||
        (e = hipMalloc(&d_res,  out_sz))  != hipSuccess) {
        res.error_code = static_cast<int>(e);
        hipFree(d_lats); hipFree(d_lons);
        hipFree(d_poly); hipFree(d_res);
        return res;
    }

    // Copy host → device.
    if ((e = hipMemcpy(d_lats, point_lats,     pts_sz,  hipMemcpyHostToDevice)) != hipSuccess ||
        (e = hipMemcpy(d_lons, point_lons,     pts_sz,  hipMemcpyHostToDevice)) != hipSuccess ||
        (e = hipMemcpy(d_poly, polygon_coords, poly_sz, hipMemcpyHostToDevice)) != hipSuccess ||
        (e = hipMemset(d_res, 0, out_sz))                                        != hipSuccess) {
        res.error_code = static_cast<int>(e);
        hipFree(d_lats); hipFree(d_lons);
        hipFree(d_poly); hipFree(d_res);
        return res;
    }

    // Launch kernel via dispatch table (default stream).
    const int rc = dispatch_table_.launchContainment(
        d_lats, d_lons, numPoints,
        d_poly, numPolygonVertices,
        d_res, nullptr);

    if (rc == 0) {
        e = hipDeviceSynchronize();
        if (e == hipSuccess) {
            res.mask.resize(static_cast<size_t>(numPoints));
            e = hipMemcpy(res.mask.data(), d_res, out_sz, hipMemcpyDeviceToHost);
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

    hipFree(d_lats); hipFree(d_lons);
    hipFree(d_poly); hipFree(d_res);
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

    hipError_t e;
    if ((e = hipMalloc(&d_lats1, coord_sz)) != hipSuccess ||
        (e = hipMalloc(&d_lons1, coord_sz)) != hipSuccess ||
        (e = hipMalloc(&d_lats2, coord_sz)) != hipSuccess ||
        (e = hipMalloc(&d_lons2, coord_sz)) != hipSuccess ||
        (e = hipMalloc(&d_out,   out_sz))   != hipSuccess) {
        res.error_code = static_cast<int>(e);
        hipFree(d_lats1); hipFree(d_lons1);
        hipFree(d_lats2); hipFree(d_lons2);
        hipFree(d_out);
        return res;
    }

    if ((e = hipMemcpy(d_lats1, lats1, coord_sz, hipMemcpyHostToDevice)) != hipSuccess ||
        (e = hipMemcpy(d_lons1, lons1, coord_sz, hipMemcpyHostToDevice)) != hipSuccess ||
        (e = hipMemcpy(d_lats2, lats2, coord_sz, hipMemcpyHostToDevice)) != hipSuccess ||
        (e = hipMemcpy(d_lons2, lons2, coord_sz, hipMemcpyHostToDevice)) != hipSuccess ||
        (e = hipMemset(d_out, 0, out_sz))                                 != hipSuccess) {
        res.error_code = static_cast<int>(e);
        hipFree(d_lats1); hipFree(d_lons1);
        hipFree(d_lats2); hipFree(d_lons2);
        hipFree(d_out);
        return res;
    }

    const int rc = dispatch_table_.launchDistance(
        d_lats1, d_lons1, d_lats2, d_lons2,
        d_out, count, formula, nullptr);

    if (rc == 0) {
        e = hipDeviceSynchronize();
        if (e == hipSuccess) {
            res.distances_km.resize(static_cast<size_t>(count));
            e = hipMemcpy(res.distances_km.data(), d_out, out_sz,
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

    hipFree(d_lats1); hipFree(d_lons1);
    hipFree(d_lats2); hipFree(d_lons2);
    hipFree(d_out);
    return res;
}

} // namespace geo
} // namespace themis
