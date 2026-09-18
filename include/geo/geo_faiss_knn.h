#pragma once

/**
 * @file geo_faiss_knn.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.9
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "utils/geo/ewkb.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace geo {

struct GeoKnnResult {
    std::size_t index;    ///< Index into the dataset passed to build().
    double      dist_m;   ///< Approximate geodesic distance in metres.
};

class GeoFaissKnn {
public:
    struct Config {
        int cuda_device_id = 0;
        bool force_cpu = false;
    };

    explicit GeoFaissKnn(const Config& cfg = Config{});
    ~GeoFaissKnn();

    GeoFaissKnn(const GeoFaissKnn&)            = delete;
    GeoFaissKnn& operator=(const GeoFaissKnn&) = delete;
    GeoFaissKnn(GeoFaissKnn&&) noexcept;
    GeoFaissKnn& operator=(GeoFaissKnn&&) noexcept;

    /**
     * @brief Build.
     * @param[in] dataset Input parameter.
     * @return True when the operation succeeds.
     */
    bool build(const std::vector<GeometryInfo>& dataset);

    /**
     * @brief Knn Search.
     * @param[in] query Input parameter.
     * @param[in] k Input parameter.
     * @return Return value.
     */
    std::vector<GeoKnnResult> knnSearch(
        const GeometryInfo& query, std::size_t k) const;

    std::vector<GeoKnnResult> radiusSearch(
        const GeometryInfo& query, double radius_m,
        std::size_t max_results = 0) const;

    /**
     * @brief Is Built.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool isBuilt() const noexcept;

    /**
     * @brief Size.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    std::size_t size() const noexcept;

    /**
     * @brief Get Backend Name.
     * @return Pointer to the result.
     * @note Exception safety: noexcept.
     */
    const char* getBackendName() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace geo
} // namespace themis
