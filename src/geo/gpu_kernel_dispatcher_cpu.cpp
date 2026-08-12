/**
 * @file gpu_kernel_dispatcher_cpu.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=6; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// STUB/SIMULATION NOTE:
// Purpose: Provide a CPU-only (non-CUDA) implementation of GpuKernelDispatcher
//   so that the geo module can be compiled and run without CUDA.  All dispatch()
//   calls return immediately with dispatched=false; the spatial backend falls
//   through to the CPU geometry engine for every request.
// Activation: THEMIS_GEO_CUDA is OFF (default on machines without a CUDA-
//   capable GPU or without the CUDA toolkit installed).
// Production Delta: All geospatial kernels (distance matrix, containment
//   bitset, spatial join) execute on the CPU instead of the GPU.  Throughput
//   is reduced by up to 10–100× for large batch requests (1 M+ points).
//   Latency SLOs for geo-heavy queries will not be met at scale.
// Removal Plan: Install the CUDA toolkit, ensure a CUDA-capable GPU is
//   present, and rebuild with -DTHEMIS_GEO_CUDA=ON.  The GPU dispatcher
//   implementation (gpu_kernel_dispatcher.cu / .cpp) is then compiled
//   instead of this file.
// Roadmap ref: src/geo/FUTURE_ENHANCEMENTS.md §"CUDA Geospatial Kernels"
// gpu_kernel_dispatcher_cpu.cpp — no-op stub for non-CUDA builds.
//
// Compiled when THEMIS_GEO_CUDA is OFF so that the rest of the geo module
// can unconditionally include "geo/gpu_kernel_dispatcher.h" and call
// GpuKernelDispatcher without depending on CUDA.  All dispatch() calls
// return immediately with dispatched=false.

#include "geo/gpu_kernel_dispatcher.h"
#include <stdexcept>

#include <mutex>

namespace themis {
namespace geo {

namespace {
std::mutex                              s_dispatch_mutex;
GpuKernelDispatcher::ContainmentDispatchFn s_containment_dispatch_fn;
GpuKernelDispatcher::DistanceDispatchFn    s_distance_dispatch_fn;
}

void GpuKernelDispatcher::setContainmentDispatchFn(ContainmentDispatchFn fn) {
    std::lock_guard<std::mutex> lk(s_dispatch_mutex);
    s_containment_dispatch_fn = std::move(fn);
}

void GpuKernelDispatcher::setDistanceDispatchFn(DistanceDispatchFn fn) {
    std::lock_guard<std::mutex> lk(s_dispatch_mutex);
    s_distance_dispatch_fn = std::move(fn);
}

GpuKernelDispatcher::GpuKernelDispatcher(
    const themis::acceleration::GeoKernelDispatch& dt) noexcept
    : dispatch_table_(dt)
{}

bool GpuKernelDispatcher::isAvailable() const noexcept {
    std::lock_guard<std::mutex> lk(s_dispatch_mutex);
    return static_cast<bool>(s_containment_dispatch_fn) ||
           static_cast<bool>(s_distance_dispatch_fn);
}

GpuKernelDispatcher::ContainmentResult GpuKernelDispatcher::dispatchContainment(
    const double* point_lats,
    const double* point_lons,
    int           numPoints,
    const double* polygon_coords,
    int           numPolygonVertices
) {
    ContainmentDispatchFn fn;
    {
        std::lock_guard<std::mutex> lk(s_dispatch_mutex);
        fn = s_containment_dispatch_fn;
    }
    if (fn && point_lats && point_lons && polygon_coords && numPoints > 0 &&
        numPolygonVertices >= 3) {
        try {
            return fn(point_lats, point_lons, numPoints, polygon_coords, numPolygonVertices);
        } catch (...) {
            auto result = ContainmentResult{};
            result.error_code = -1;
            return result;
        }
    }
    return ContainmentResult{};
}

GpuKernelDispatcher::DistanceResult GpuKernelDispatcher::dispatchDistance(
    const double* lats1,
    const double* lons1,
    const double* lats2,
    const double* lons2,
    int           count,
    themis::acceleration::GeoDistanceFormula formula
) {
    DistanceDispatchFn fn;
    {
        std::lock_guard<std::mutex> lk(s_dispatch_mutex);
        fn = s_distance_dispatch_fn;
    }
    if (fn && lats1 && lons1 && lats2 && lons2 && count > 0) {
        try {
            return fn(lats1, lons1, lats2, lons2, count, formula);
        } catch (...) {
            auto result = DistanceResult{};
            result.error_code = -1;
            return result;
        }
    }
    return DistanceResult{};
}

} // namespace geo
} // namespace themis

