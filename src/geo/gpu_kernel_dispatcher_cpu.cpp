/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            gpu_kernel_dispatcher_cpu.cpp                      ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-04-15 07:11:55                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     67                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
