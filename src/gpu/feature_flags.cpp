/**
 * @file feature_flags.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "themis/gpu/feature_flags.h"

namespace themis {
namespace gpu {

// ============================================================================
// Static helpers
// ============================================================================

const char* GPUFeatureFlags::featureName(Feature feature) noexcept {
    switch (feature) {
        case Feature::MEMORY_POOL:       return "MEMORY_POOL";
        case Feature::ASYNC_LAUNCHER:    return "ASYNC_LAUNCHER";
        case Feature::MULTI_GPU:         return "MULTI_GPU";
        case Feature::TENSOR_OPS:        return "TENSOR_OPS";
        case Feature::POLICY_GATE:       return "POLICY_GATE";
        case Feature::AUDIT_LOG:         return "AUDIT_LOG";
        case Feature::METRICS:           return "METRICS";
        case Feature::LOAD_BALANCER:     return "LOAD_BALANCER";
        case Feature::KERNEL_VALIDATOR:  return "KERNEL_VALIDATOR";
        case Feature::ALERTS:            return "ALERTS";
        case Feature::WASM_SANDBOX:      return "WASM_SANDBOX";
        case Feature::MIG_MANAGER:       return "MIG_MANAGER";
        case Feature::VULKAN_BACKEND:    return "VULKAN_BACKEND";
        case Feature::PEER_TO_PEER:      return "PEER_TO_PEER";
    }
    return "UNKNOWN";
}

std::string GPUFeatureFlags::editionName() {
    return std::string(edition::EDITION_STRING);
}

// ============================================================================
// Edition defaults
// ============================================================================

/*
 * Feature defaults per edition:
 *
 * | Feature          | COMMUNITY | ENTERPRISE | HYPERSCALER |
 * |------------------|-----------|------------|-------------|
 * | MEMORY_POOL      |    yes    |    yes     |    yes      |
 * | ASYNC_LAUNCHER   |    yes    |    yes     |    yes      |
 * | MULTI_GPU        |    no     |    yes     |    yes      |
 * | TENSOR_OPS       |    no     |    yes     |    yes      |
 * | POLICY_GATE      |    yes    |    yes     |    yes      |
 * | AUDIT_LOG        |    yes    |    yes     |    yes      |
 * | METRICS          |    yes    |    yes     |    yes      |
 * | LOAD_BALANCER    |    no     |    yes     |    yes      |
 * | KERNEL_VALIDATOR |    yes    |    yes     |    yes      |
 * | ALERTS           |    yes    |    yes     |    yes      |
 * | WASM_SANDBOX     |    no     |    yes     |    yes      |
 * | MIG_MANAGER      |    no     |    yes     |    yes      |
 * | VULKAN_BACKEND   |    yes    |    yes     |    yes      |
 * | PEER_TO_PEER     |    no     |    yes     |    yes      |
 */
bool GPUFeatureFlags::editionDefaultFor(Feature f) {
    const auto ed = edition::GetEditionType();

    // Multi-GPU features require Enterprise or above.
    if (f == Feature::MULTI_GPU || f == Feature::LOAD_BALANCER) {
        return ed == edition::EditionType::ENTERPRISE ||
               ed == edition::EditionType::HYPERSCALER;
    }

    // Tensor ops (GPU kernel execution) require Enterprise or above.
    if (f == Feature::TENSOR_OPS) {
        return ed == edition::EditionType::ENTERPRISE ||
               ed == edition::EditionType::HYPERSCALER;
    }

    // WASM sandbox requires Enterprise or above (third-party kernel isolation).
    if (f == Feature::WASM_SANDBOX) {
        return ed == edition::EditionType::ENTERPRISE ||
               ed == edition::EditionType::HYPERSCALER;
    }

    // MIG partitioning requires Enterprise or above (hardware partitioning).
    if (f == Feature::MIG_MANAGER) {
        return ed == edition::EditionType::ENTERPRISE ||
               ed == edition::EditionType::HYPERSCALER;
    }

    // Vulkan backend is available in all editions (cross-vendor, no CUDA/HIP needed).
    if (f == Feature::VULKAN_BACKEND) {
        return ed != edition::EditionType::UNKNOWN;
    }

    // Peer-to-peer GPU transfers require Enterprise or above.
    if (f == Feature::PEER_TO_PEER) {
        return ed == edition::EditionType::ENTERPRISE ||
               ed == edition::EditionType::HYPERSCALER;
    }

    // All other features are available in Community and above
    // (i.e. always true for any known edition).
    return ed != edition::EditionType::UNKNOWN;
}

void GPUFeatureFlags::initDefaults() {
    const Feature all[] = {
        Feature::MEMORY_POOL, Feature::ASYNC_LAUNCHER, Feature::MULTI_GPU,
        Feature::TENSOR_OPS,  Feature::POLICY_GATE,    Feature::AUDIT_LOG,
        Feature::METRICS,     Feature::LOAD_BALANCER,  Feature::KERNEL_VALIDATOR,
        Feature::ALERTS,      Feature::WASM_SANDBOX,   Feature::MIG_MANAGER,
        Feature::VULKAN_BACKEND, Feature::PEER_TO_PEER,
    };
    for (auto f : all) {
        defaults_[key(f)] = editionDefaultFor(f);
    }
}

// ============================================================================
// Construction
// ============================================================================

GPUFeatureFlags::GPUFeatureFlags() {
    initDefaults();
}

// ============================================================================
// Query
// ============================================================================

bool GPUFeatureFlags::isEnabled(Feature feature) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const int k = key(feature);
    // Check override first.
    auto ov = overrides_.find(k);
    if (ov != overrides_.end()) {
        return ov->second;
    }
    // Fall back to edition default.
    auto def = defaults_.find(k);
    if (def != defaults_.end()) {
        return def->second;
    }
    return false;
}

// ============================================================================
// Override
// ============================================================================

void GPUFeatureFlags::enable(Feature feature) {
    std::lock_guard<std::mutex> lock(mutex_);
    overrides_[key(feature)] = true;
}

void GPUFeatureFlags::disable(Feature feature) {
    std::lock_guard<std::mutex> lock(mutex_);
    overrides_[key(feature)] = false;
}

void GPUFeatureFlags::resetToDefaults() {
    std::lock_guard<std::mutex> lock(mutex_);
    overrides_.clear();
}

// ============================================================================
// Introspection
// ============================================================================

std::vector<GPUFeatureFlags::FeatureStatus> GPUFeatureFlags::getAll() const {
    std::lock_guard<std::mutex> lock(mutex_);
    const Feature all[] = {
        Feature::MEMORY_POOL, Feature::ASYNC_LAUNCHER, Feature::MULTI_GPU,
        Feature::TENSOR_OPS,  Feature::POLICY_GATE,    Feature::AUDIT_LOG,
        Feature::METRICS,     Feature::LOAD_BALANCER,  Feature::KERNEL_VALIDATOR,
        Feature::ALERTS,      Feature::WASM_SANDBOX,   Feature::MIG_MANAGER,
        Feature::VULKAN_BACKEND, Feature::PEER_TO_PEER,
    };
    std::vector<FeatureStatus> result;
    result.reserve(std::size(all));
    for (auto f : all) {
        FeatureStatus s;
        s.feature = f;
        s.name = featureName(f);
        const int k = key(f);
        auto ov = overrides_.find(k);
        if (ov != overrides_.end()) {
            s.enabled    = ov->second;
            s.overridden = true;
        } else {
            auto def = defaults_.find(k);
            s.enabled    = (def != defaults_.end()) ? def->second : false;
            s.overridden = false;
        }
        result.push_back(s);
    }
    return result;
}

} // namespace gpu
} // namespace themis
