/**
 * @file geo_faiss_knn.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.9
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "geo/geo_faiss_knn.h"
#include "utils/logger.h"

#include <algorithm>
#include <cmath>
#include <vector>
#include <numeric>
#include <stdexcept>

// FAISS headers (CPU path always available; GPU path gated on THEMIS_ENABLE_CUDA)
#if defined(THEMIS_HAS_FAISS) && THEMIS_HAS_FAISS
  #include <faiss/IndexFlat.h>
    #if defined(THEMIS_ENABLE_CUDA) && THEMIS_ENABLE_CUDA
    #include <faiss/gpu/StandardGpuResources.h>
    #include <faiss/gpu/GpuIndexFlat.h>
  #endif
#endif

namespace themis {
namespace geo {

// ---------------------------------------------------------------------------
// ECEF projection helpers
// ---------------------------------------------------------------------------

static constexpr double kPi       = 3.14159265358979323846;
static constexpr double kEarthR_m = 6371000.0; // mean spherical radius

/// Project WGS-84 (lon_deg, lat_deg) to unit-sphere ECEF float32.
static void wgs84ToEcef(double lon_deg, double lat_deg,
                        float& x, float& y, float& z) noexcept {
    const double lon = lon_deg * kPi / 180.0;
    const double lat = lat_deg * kPi / 180.0;
    const double cos_lat = std::cos(lat);
    x = static_cast<float>(cos_lat * std::cos(lon));
    y = static_cast<float>(cos_lat * std::sin(lon));
    z = static_cast<float>(std::sin(lat));
}

/// Convert ECEF chord distance (unit sphere) to approximate geodesic metres.
/// chord = 2 × sin(angle/2), so angle = 2 × arcsin(chord/2).
static double chordToDistanceM([[maybe_unused]] float chord) noexcept {
    const double half_chord = static_cast<double>(chord) * 0.5;
    const double clamped    = std::min(1.0, std::max(0.0, half_chord));
    return kEarthR_m * 2.0 * std::asin(clamped);
}

/// Convert a radius in metres to an ECEF unit-sphere chord distance squared.
static float radiusToChordSq([[maybe_unused]] double radius_m) noexcept {
    const double angle    = radius_m / kEarthR_m;
    const double chord    = 2.0 * std::sin(angle * 0.5);
    return static_cast<float>(chord * chord);
}

// ---------------------------------------------------------------------------
// GeoFaissKnn::Impl
// ---------------------------------------------------------------------------

struct GeoFaissKnn::Impl {
    static constexpr int kDim = 3; // ECEF unit-sphere dimension

    GeoFaissKnn::Config cfg;

    bool        built          = false;
    bool        use_gpu        = false;
    std::size_t indexed_count  = 0;

    // dataset_map[faiss_idx] = original GeometryInfo index (accounts for skipped non-Points)
    std::vector<std::size_t> dataset_map;
    // Parallel ECEF vectors for the indexed points (host copy)
    std::vector<float>       ecef_data{}; // flat [indexed_count × kDim]

#if defined(THEMIS_HAS_FAISS) && THEMIS_HAS_FAISS
    #if defined(THEMIS_ENABLE_CUDA) && THEMIS_ENABLE_CUDA
    std::unique_ptr<faiss::gpu::StandardGpuResources> gpu_res;
    std::unique_ptr<faiss::gpu::GpuIndexFlatL2>       gpu_index;
  #endif
    std::unique_ptr<faiss::IndexFlatL2> cpu_index;
#endif

    explicit Impl(const GeoFaissKnn::Config& c) : cfg(c) {}

    bool build(const std::vector<GeometryInfo>& dataset) {
        built         = false;
        indexed_count = 0;
        dataset_map.clear();
        ecef_data.clear();
        ecef_data.reserve(dataset.size() * static_cast<std::size_t>(kDim)); // upper bound; shrinks after filtering

        for (std::size_t i = 0; i < dataset.size(); ++i) {
            const auto& g = dataset[i];
            if (!g.isPoint() || g.coords.empty()) {
              continue;
            }
            float x, y, z;
            wgs84ToEcef(g.coords[0].x, g.coords[0].y, x, y, z);
            ecef_data.push_back(x);
            ecef_data.push_back(y);
            ecef_data.push_back(z);
            dataset_map.push_back(i);
            ++indexed_count;
        }

        if (indexed_count == 0) {
            THEMIS_WARN("GeoFaissKnn: no valid Point geometries to index");
            return false;
        }

#if defined(THEMIS_HAS_FAISS) && THEMIS_HAS_FAISS
        bool gpu_ok = false;
    #if defined(THEMIS_ENABLE_CUDA) && THEMIS_ENABLE_CUDA
        if (!cfg.force_cpu) {
            try {
                gpu_res   = std::make_unique<faiss::gpu::StandardGpuResources>();
                faiss::gpu::GpuIndexFlatConfig gpu_cfg;
                gpu_cfg.device = cfg.cuda_device_id;
                gpu_index = std::make_unique<faiss::gpu::GpuIndexFlatL2>(
                    gpu_res.get(), kDim, gpu_cfg);
                gpu_index->add(static_cast<faiss::idx_t>(indexed_count),
                               ecef_data.data());
                use_gpu = true;
                gpu_ok  = true;
                THEMIS_INFO("GeoFaissKnn: GPU FLAT_L2 index built ({} points)", indexed_count);
            } catch (const std::exception& e) {
                THEMIS_WARN("GeoFaissKnn: GPU index build failed ({}); falling back to CPU", e.what());
                gpu_index.reset();
                gpu_res.reset();
            }
        }
  #endif
        if (!gpu_ok) {
            cpu_index = std::make_unique<faiss::IndexFlatL2>(kDim);
            cpu_index->add(static_cast<faiss::idx_t>(indexed_count),
                           ecef_data.data());
            use_gpu = false;
            THEMIS_INFO("GeoFaissKnn: CPU FLAT_L2 index built ({} points)", indexed_count);
        }
#else
        // FAISS not available; store data for brute-force CPU fallback
        use_gpu = false;
        THEMIS_WARN("GeoFaissKnn: FAISS not compiled in; using brute-force CPU k-NN");
#endif
        built = true;
        return true;
    }

    std::vector<GeoKnnResult> knnSearch(const GeometryInfo& query,
                                        std::size_t k) const {
        std::vector<GeoKnnResult> results = {};

        if (!built || indexed_count == 0) {
          return results;
        }
        if (!query.isPoint() || query.coords.empty()) {
          return results;
        }

        float qx, qy, qz;
        wgs84ToEcef(query.coords[0].x, query.coords[0].y, qx, qy, qz);

        const std::size_t keff = std::min(k, indexed_count);

#if defined(THEMIS_HAS_FAISS) && THEMIS_HAS_FAISS
        std::vector<faiss::idx_t> idx(keff, -1);
        std::vector<float>        dists(keff, 0.0f);

        const float qvec[kDim] = {qx, qy, qz};
        try {
    #if defined(THEMIS_ENABLE_CUDA) && THEMIS_ENABLE_CUDA
            if (use_gpu && gpu_index) {
                gpu_index->search(1,
                                  qvec,
                                  static_cast<faiss::idx_t>(keff),
                                  dists.data(), idx.data());
            } else
  #endif
            if (cpu_index) {
                cpu_index->search(1,
                                  qvec,
                                  static_cast<faiss::idx_t>(keff),
                                  dists.data(), idx.data());
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("GeoFaissKnn::knnSearch: FAISS search failed: {}", e.what());
            return results;
        }

        results.reserve(keff);
        for (std::size_t i = 0; i < keff; ++i) {
            if (idx[i] < 0) {
              continue;
            }
            const auto fi = static_cast<std::size_t>(idx[i]);
            if (fi >= static_cast<int>(dataset_map.size())) {
              continue;
            }
            GeoKnnResult r;
            r.index  = dataset_map[fi];
            r.dist_m = chordToDistanceM(std::sqrt(dists[i]));
            results.push_back(r);
        }
#else
        // Brute-force fallback when FAISS is not compiled in.
        const float qvec[kDim] = {qx, qy, qz};
        std::vector<std::pair<float, std::size_t>> candidates;
        candidates.reserve(indexed_count);
        for (std::size_t fi = 0; fi < indexed_count; ++fi) {
            const float* pv = ecef_data.data() + fi * kDim;
            float d2 = 0.0f;
            for (int d = 0; d < kDim; ++d) {
                const float diff = qvec[d] - pv[d];
                d2 += diff * diff;
            }
            candidates.emplace_back(d2, fi);
        }
        std::partial_sort(candidates.begin(),
                          candidates.begin() + static_cast<long>(keff),
                          candidates.end());
        results.reserve(keff);
        for (std::size_t i = 0; i < keff; ++i) {
            GeoKnnResult r;
            r.index  = dataset_map[candidates[i].second];
            r.dist_m = chordToDistanceM(std::sqrt(candidates[i].first));
            results.push_back(r);
        }
#endif
        return results;
    }

    std::vector<GeoKnnResult> radiusSearch(const GeometryInfo& query,
                                           double radius_m,
                                           std::size_t max_results) const {
        if (!built || radius_m <= 0.0) return {};

        // Search at most indexed_count neighbours, then filter by chord threshold.
        const std::size_t search_k = (max_results == 0 || max_results > indexed_count)
                                     ? indexed_count : max_results * 4;
        auto candidates = knnSearch(query, search_k);

        std::vector<GeoKnnResult> results = {};

        results.reserve(candidates.size());
        for (const auto& c : candidates) {
            if (c.dist_m > radius_m) break; // sorted ascending
            results.push_back(c);
            if (max_results > 0 && results.size() >= max_results) {
              break;
            }
        }
        return results;
    }
};

// ---------------------------------------------------------------------------
// GeoFaissKnn public API
// ---------------------------------------------------------------------------

GeoFaissKnn::GeoFaissKnn(const Config& cfg)
    : impl_(std::make_unique<Impl>(cfg)) {}

GeoFaissKnn::~GeoFaissKnn() = default;
GeoFaissKnn::GeoFaissKnn(GeoFaissKnn&&) noexcept = default;
GeoFaissKnn& GeoFaissKnn::operator=(GeoFaissKnn&&) noexcept = default;

bool GeoFaissKnn::build(const std::vector<GeometryInfo>& dataset) {
    return impl_->build(dataset);
}

std::vector<GeoKnnResult> GeoFaissKnn::knnSearch(
    const GeometryInfo& query, std::size_t k) const {
    return impl_->knnSearch(query, k);
}

std::vector<GeoKnnResult> GeoFaissKnn::radiusSearch(
    const GeometryInfo& query, double radius_m, std::size_t max_results) const {
    return impl_->radiusSearch(query, radius_m, max_results);
}

bool GeoFaissKnn::isBuilt() const noexcept {
    return impl_ && impl_->built;
}

std::size_t GeoFaissKnn::size() const noexcept {
    return impl_ ? impl_->indexed_count : 0;
}

const char* GeoFaissKnn::getBackendName() const noexcept {
    if (!impl_ || !impl_->built) {
      return "not_built";
    }
    return impl_->use_gpu ? "faiss_gpu" : "faiss_cpu";
}

} // namespace geo
} // namespace themis
