/**
 * @file policy.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * GPU Policy — default-deny capability gate for GPU resource access.
 */

#include "themis/gpu/policy.h"

namespace themis {
namespace gpu {

// ============================================================================
// Construction
// ============================================================================

GPUPolicy::GPUPolicy(const std::vector<std::string> &pre_granted_callers) {
    for (const auto &id : pre_granted_callers) {
        grant(id, Capability::GPU_ANY);
    }
}

// ============================================================================
// Grant / revoke
// ============================================================================

void GPUPolicy::grant(const std::string &caller_id, Capability cap) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (cap == Capability::GPU_ANY) {
        // Grant every concrete capability.
        grants_[caller_id].insert(cap_to_int(Capability::GPU_ALLOCATE));
        grants_[caller_id].insert(cap_to_int(Capability::GPU_FREE));
        grants_[caller_id].insert(cap_to_int(Capability::GPU_ADMIN));
        grants_[caller_id].insert(cap_to_int(Capability::GPU_ANY));
    } else {
        grants_[caller_id].insert(cap_to_int(cap));
    }
}

void GPUPolicy::revoke(const std::string &caller_id, Capability cap) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = grants_.find(caller_id);
    if (it == grants_.end()) {
        return;
    }

    if (cap == Capability::GPU_ANY) {
        grants_.erase(it);
    } else {
        it->second.erase(cap_to_int(cap));
        // Also remove the wildcard entry so GPU_ANY stops covering this caller.
        it->second.erase(cap_to_int(Capability::GPU_ANY));
        if (it->second.empty()) {
            grants_.erase(it);
        }
    }
}

void GPUPolicy::revokeAll(const std::string &caller_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    grants_.erase(caller_id);
}

// ============================================================================
// Check
// ============================================================================

bool GPUPolicy::hasCapability(const std::string &id, Capability cap) const {
    auto it = grants_.find(id);
    if (it == grants_.end()) {
        return false;
    }
    // GPU_ANY wildcard covers every capability.
    if (it->second.count(cap_to_int(Capability::GPU_ANY))) {
        return true;
    }
    return it->second.count(cap_to_int(cap)) > 0;
}

GPUPolicy::PolicyDecision GPUPolicy::check(const std::string &caller_id, Capability cap) const {
    PolicyDecision d;
    d.caller_id  = caller_id;
    d.capability = cap;

    std::lock_guard<std::mutex> lock(mutex_);
    if (hasCapability(caller_id, cap)) {
        d.allowed = true;
        d.reason  = "granted";
    } else {
        d.allowed = false;
        d.reason  = std::string("caller '") + caller_id + "' does not hold capability " + capabilityName(cap)
                    + " (default-deny)";
    }
    return d;
}

bool GPUPolicy::isAllowed(const std::string &caller_id, Capability cap) const {
    return check(caller_id, cap).allowed;
}

// ============================================================================
// Queries
// ============================================================================

std::vector<std::string> GPUPolicy::grantedCallers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> result = {};

    result.reserve(grants_.size());
    for (const auto &kv : grants_) {
        result.push_back(kv.first);
    }
    return result;
}

std::vector<GPUPolicy::Capability> GPUPolicy::capabilitiesOf(const std::string &caller_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Capability> result;
    auto it = grants_.find(caller_id);
    if (it == grants_.end()) {
        return result;
    }
    for (int v : it->second) {
        result.push_back(static_cast<Capability>(v));
    }
    return result;
}

size_t GPUPolicy::grantedCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(grants_.size());
}

// ============================================================================
// capabilityName
// ============================================================================

const char *capabilityName(GPUPolicy::Capability cap) {
    switch (cap) {
        case GPUPolicy::Capability::GPU_ALLOCATE:
            return "GPU_ALLOCATE";
        case GPUPolicy::Capability::GPU_FREE:
            return "GPU_FREE";
        case GPUPolicy::Capability::GPU_ADMIN:
            return "GPU_ADMIN";
        case GPUPolicy::Capability::GPU_ANY:
            return "GPU_ANY";
        default:
            return "UNKNOWN";
    }
}

} // namespace gpu
} // namespace themis
