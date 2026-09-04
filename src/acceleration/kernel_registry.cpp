/**
 * @file kernel_registry.cpp
 * @brief KernelRegistry extended methods — validation, fallback resolution,
 *        enumeration, and summary formatting.
 *
 * Issue: #5382 — GPU/VRAM Layer: Refactor, vereinheitlichen und
 *                Shader-Vollständigkeit sichern
 *
 * Implements the extended API added to KernelRegistry (compute_backend.h) and
 * the convenience delegates on BackendRegistry:
 *   - KernelRegistry::lookupANNWithFallback()
 *   - KernelRegistry::lookupGeoWithFallback()
 *   - KernelRegistry::registeredBackends()
 *   - KernelRegistry::validate()
 *   - ValidationReport::summary()
 *   - BackendRegistry::validateKernels()
 *   - BackendRegistry::getKernelRegistry()
 */

#include "acceleration/compute_backend.h"

#include <sstream>

namespace themis {
namespace acceleration {

// ---------------------------------------------------------------------------
// Internal helper
// ---------------------------------------------------------------------------

namespace {

const char* backendTypeName(BackendType t) noexcept {
    switch (t) {
        case BackendType::CPU:       return "CPU";
        case BackendType::CUDA:      return "CUDA";
        case BackendType::ZLUDA:     return "ZLUDA";
        case BackendType::HIP:       return "HIP";
        case BackendType::ROCM:      return "ROCM";
        case BackendType::DIRECTX:   return "DIRECTX";
        case BackendType::VULKAN:    return "VULKAN";
        case BackendType::OPENGL:    return "OPENGL";
        case BackendType::METAL:     return "METAL";
        case BackendType::ONEAPI:    return "ONEAPI";
        case BackendType::OPENCL:    return "OPENCL";
        case BackendType::WEBGPU:    return "WEBGPU";
        case BackendType::MULTI_GPU: return "MULTI_GPU";
        case BackendType::AUTO:      return "AUTO";
        default:                     return "UNKNOWN";
    }
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// ValidationReport::summary()
// ---------------------------------------------------------------------------

std::string ValidationReport::summary() const {
    std::ostringstream oss;
    for (const auto& e : entries) {
        oss << backendTypeName(e.backend) << "  ";
        if (e.hasANN)    oss << "ANN="    << (e.annComplete    ? "\u2713" : "\u2717") << " ";
        if (e.hasGeo)    oss << "Geo="    << (e.geoComplete    ? "\u2713" : "\u2717") << " ";
        if (e.hasMatrix) oss << "Matrix=" << (e.matrixComplete ? "\u2713" : "\u2717") << " ";
        if (!e.missingSlots.empty()) {
            oss << "[";
            for (size_t i = 0; i < e.missingSlots.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << e.missingSlots[i];
            }
            oss << "]";
        }
        oss << "\n";
    }
    return oss.str();
}

// ---------------------------------------------------------------------------
// KernelRegistry::lookupANNWithFallback()
// ---------------------------------------------------------------------------

ANNKernelDispatch KernelRegistry::lookupANNWithFallback(BackendType primary) const noexcept {
    ANNKernelDispatch result = getANNDispatch(primary);

    if (primary != BackendType::CPU) {
        const ANNKernelDispatch cpu = getANNDispatch(BackendType::CPU);
        if (!result.launchL2Distance)   result.launchL2Distance   = cpu.launchL2Distance;
        if (!result.launchCosine)       result.launchCosine       = cpu.launchCosine;
        if (!result.launchInnerProduct) result.launchInnerProduct = cpu.launchInnerProduct;
        if (!result.launchTopK)         result.launchTopK         = cpu.launchTopK;
    }

    return result;
}

// ---------------------------------------------------------------------------
// KernelRegistry::lookupGeoWithFallback()
// ---------------------------------------------------------------------------

GeoKernelDispatch KernelRegistry::lookupGeoWithFallback(BackendType primary) const noexcept {
    GeoKernelDispatch result = getGeoDispatch(primary);

    if (primary != BackendType::CPU) {
        const GeoKernelDispatch cpu = getGeoDispatch(BackendType::CPU);
        if (!result.launchDistance)    result.launchDistance    = cpu.launchDistance;
        if (!result.launchContainment) result.launchContainment = cpu.launchContainment;
    }

    return result;
}

// ---------------------------------------------------------------------------
// KernelRegistry::registeredBackends()
// ---------------------------------------------------------------------------

std::vector<BackendType> KernelRegistry::registeredBackends() const {
    std::vector<BackendType> result;

    auto addIfAbsent = [&]([[maybe_unused]] BackendType t) {
        for (const auto b : result) {
            if (b == t) return;
        }
        result.push_back(t);
    };

    for (const auto& [bt, _] : annDispatch_)    addIfAbsent(bt);
    for (const auto& [bt, _] : geoDispatch_)    addIfAbsent(bt);
    for (const auto& [bt, _] : matrixDispatch_) addIfAbsent(bt);

    return result;
}

// ---------------------------------------------------------------------------
// KernelRegistry::validate()
// ---------------------------------------------------------------------------

ValidationReport KernelRegistry::validate() const {
    std::vector<BackendType> allBackends = registeredBackends();

    ValidationReport report;
    report.entries.reserve(allBackends.size());

    for (BackendType bt : allBackends) {
        KernelCoverage cov;
        cov.backend = bt;

        // --- ANN ---
        auto annIt = annDispatch_.find(bt);
        if (annIt != annDispatch_.end()) {
            cov.hasANN = true;
            const ANNKernelDispatch& d = annIt->second;
            bool ok = true;
            if (!d.launchL2Distance)   { cov.missingSlots.push_back("ANN::launchL2Distance");   ok = false; }
            if (!d.launchCosine)       { cov.missingSlots.push_back("ANN::launchCosine");        ok = false; }
            if (!d.launchInnerProduct) { cov.missingSlots.push_back("ANN::launchInnerProduct");  ok = false; }
            if (!d.launchTopK)         { cov.missingSlots.push_back("ANN::launchTopK");          ok = false; }
            cov.annComplete = ok;
        }

        // --- Geo ---
        auto geoIt = geoDispatch_.find(bt);
        if (geoIt != geoDispatch_.end()) {
            cov.hasGeo = true;
            const GeoKernelDispatch& d = geoIt->second;
            bool ok = true;
            if (!d.launchDistance)    { cov.missingSlots.push_back("Geo::launchDistance");    ok = false; }
            if (!d.launchContainment) { cov.missingSlots.push_back("Geo::launchContainment"); ok = false; }
            cov.geoComplete = ok;
        }

        // --- Matrix ---
        auto matIt = matrixDispatch_.find(bt);
        if (matIt != matrixDispatch_.end()) {
            cov.hasMatrix = true;
            bool ok = (matIt->second.launchMatmul != nullptr);
            if (!ok) cov.missingSlots.push_back("Matrix::launchMatmul");
            cov.matrixComplete = ok;
        }

        report.entries.push_back(std::move(cov));
    }

    return report;
}

// ---------------------------------------------------------------------------
// BackendRegistry::validateKernels()
// ---------------------------------------------------------------------------

ValidationReport BackendRegistry::validateKernels() const {
    std::shared_lock lock(registryMutex_);
    return kernelRegistry_.validate();
}

// ---------------------------------------------------------------------------
// BackendRegistry::getKernelRegistry()
// ---------------------------------------------------------------------------

const KernelRegistry& BackendRegistry::getKernelRegistry() const noexcept {
    return kernelRegistry_;
}

} // namespace acceleration
} // namespace themis
