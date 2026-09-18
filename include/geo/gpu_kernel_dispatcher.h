/**
 * @file gpu_kernel_dispatcher.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

#include "acceleration/kernel_invocation.h"

namespace themis {
namespace geo {

class GpuKernelDispatcher {
public:
    struct ContainmentResult {
        bool                 dispatched = false; ///< true when the GPU kernel ran
        std::vector<uint8_t> mask;               ///< 1 = inside, 0 = outside [numPoints]
        int                  error_code = 0;     ///< non-zero CUDA error on failure
    };

    struct DistanceResult {
        bool               dispatched = false; ///< true when the GPU kernel ran
        std::vector<float> distances_km;       ///< distances in km [count]
        int                error_code = 0;     ///< non-zero CUDA error on failure
    };

    /**
     * @brief Gpu Kernel Dispatcher.
     * @param[in] dispatch_table Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    explicit GpuKernelDispatcher(
        const themis::acceleration::GeoKernelDispatch& dispatch_table) noexcept;

    /**
     * @brief Dispatch Containment.
     * @param[in] point_lats Input parameter.
     * @param[in] point_lons Input parameter.
     * @param[in] numPoints Input parameter.
     * @param[in] polygon_coords Input parameter.
     * @param[in] numPolygonVertices Input parameter.
     * @return Return value.
     */
    ContainmentResult dispatchContainment(
        const double* point_lats,
        const double* point_lons,
        int           numPoints,
        const double* polygon_coords,
        int           numPolygonVertices
    );

    DistanceResult dispatchDistance(
        const double* lats1,
        const double* lons1,
        const double* lats2,
        const double* lons2,
        int           count,
        themis::acceleration::GeoDistanceFormula formula =
            themis::acceleration::GeoDistanceFormula::HAVERSINE
    );

    /**
     * @brief Is Available.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool isAvailable() const noexcept;

#ifndef THEMIS_GEO_CUDA
    using ContainmentDispatchFn = std::function<ContainmentResult(const double*,
                                                                  const double*,
                                                                  int,
                                                                  const double*,
                                                                  int)>;
    using DistanceDispatchFn = std::function<DistanceResult(const double*,
                                                            const double*,
                                                            const double*,
                                                            const double*,
                                                            int,
                                                            themis::acceleration::GeoDistanceFormula)>;

    /**
     * @brief Set Containment Dispatch Fn.
     * @param[in] fn Input parameter.
     */
    static void setContainmentDispatchFn(ContainmentDispatchFn fn);
    /**
     * @brief Set Distance Dispatch Fn.
     * @param[in] fn Input parameter.
     */
    static void setDistanceDispatchFn(DistanceDispatchFn fn);
#endif

private:
    themis::acceleration::GeoKernelDispatch dispatch_table_;
};

} // namespace geo
} // namespace themis
