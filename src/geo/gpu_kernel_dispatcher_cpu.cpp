/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            gpu_kernel_dispatcher_cpu.cpp                      ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-04-15 18:48:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     66                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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

namespace themis {
namespace geo {

GpuKernelDispatcher::GpuKernelDispatcher(
    const themis::acceleration::GeoKernelDispatch& dt) noexcept
    : dispatch_table_(dt)
{}

bool GpuKernelDispatcher::isAvailable() const noexcept {
    return false;
}

GpuKernelDispatcher::ContainmentResult GpuKernelDispatcher::dispatchContainment(
    const double* /*point_lats*/,
    const double* /*point_lons*/,
    int           /*numPoints*/,
    const double* /*polygon_coords*/,
    int           /*numPolygonVertices*/
) {
    return ContainmentResult{};
}

GpuKernelDispatcher::DistanceResult GpuKernelDispatcher::dispatchDistance(
    const double* /*lats1*/,
    const double* /*lons1*/,
    const double* /*lats2*/,
    const double* /*lons2*/,
    int           /*count*/,
    themis::acceleration::GeoDistanceFormula /*formula*/
) {
    return DistanceResult{};
}

} // namespace geo
} // namespace themis
