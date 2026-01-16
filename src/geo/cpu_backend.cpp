#include "geo/spatial_backend.h"
#include "utils/geo/ewkb.h"

#include <iostream>
#include <stdexcept>

namespace themis { namespace geo {

class CpuExactBackend final : public ISpatialComputeBackend {
public:
    const char* name() const noexcept override { return "cpu_exact"; }
    bool isAvailable() const noexcept override { return true; }
    SpatialBatchResults batchIntersects(const SpatialBatchInputs& in) override {
        SpatialBatchResults out;
        out.mask.assign(in.count, 0u); // placeholder: no-ops
        return out;
    }
    
    // Stub exact check - falls back to MBR only
    bool exactIntersects(const GeometryInfo& geom1, const GeometryInfo& geom2) override {
        // Without Boost.Geometry, fall back to MBR checks only
        auto mbr1 = geom1.computeMBR();
        auto mbr2 = geom2.computeMBR();
        return mbr1.intersects(mbr2);
    }
};

// Simple internal registry stub (no global linkage yet)
struct NullRegistry : public IGeoRegistry {
    void registerBackend(std::unique_ptr<ISpatialComputeBackend>) override {}
};

static void register_builtin_cpu_backend() {
#ifdef THEMIS_GEO_ENABLED
    try {
        NullRegistry reg;
        reg.registerBackend(std::make_unique<CpuExactBackend>());
    } catch (const std::exception& ex) {
        std::cerr << "WARNING: CPU geometry backend registration failed: " << ex.what() << std::endl;
    } catch (...) {
        std::cerr << "WARNING: CPU geometry backend registration failed with unknown exception" << std::endl;
    }
#endif
}

// Ensure the object file isn't discarded
static int s_geo_cpu_backend_anchor = (register_builtin_cpu_backend(), 0);

} } // namespace themis::geo
