/**
 * @file policy.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace themis {
namespace gpu {

/**
 * @brief Policy-gated GPU usage controller.
 *
 * Implements a default-deny access model for GPU resources:
 * - New callers and tenants are denied GPU access until explicitly granted.
 * - Each caller (identified by a string ID) may hold one or more
 *   `Capability` grants.
 * - A `PolicyDecision` carries the result and a human-readable reason,
 *   suitable for structured logging and audit trails.
 *
 * This is intentionally decoupled from the memory manager so that policy
 * enforcement can be composed with allocation without circular dependencies.
 *
 * Thread safety: all methods are protected by an internal mutex.
 */
class GPUPolicy {
public:
    // -----------------------------------------------------------------------
    // Capabilities
    // -----------------------------------------------------------------------
    enum class Capability {
        GPU_ALLOCATE,   ///< May call TryAllocateGPU / ValidateAllocation
        GPU_FREE,       ///< May call DeallocateGPU
        GPU_ADMIN,      ///< May access stats, admin endpoints, dry-run
        GPU_ANY,        ///< Wildcard — all GPU capabilities
    };

    // -----------------------------------------------------------------------
    // Policy decision
    // -----------------------------------------------------------------------
    struct PolicyDecision {
        bool        allowed = false;
        std::string reason;           ///< Why access was allowed or denied
        std::string caller_id;
        Capability  capability = Capability::GPU_ALLOCATE;
    };

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------
    GPUPolicy() = default;

    /**
     * @brief Construct with a set of pre-granted callers (for tests / bootstrap).
     */
    explicit GPUPolicy(const std::vector<std::string>& pre_granted_callers);

    // -----------------------------------------------------------------------
    // Grant / revoke
    // -----------------------------------------------------------------------

    /**
     * @brief Grant a specific capability to @p caller_id.
     *
     * If @p cap is `GPU_ANY`, all capabilities are granted.
     */
    void grant(const std::string& caller_id, Capability cap = Capability::GPU_ANY);

    /**
     * @brief Revoke a specific capability from @p caller_id.
     *
     * If @p cap is `GPU_ANY`, all capabilities are revoked.
     */
    void revoke(const std::string& caller_id, Capability cap = Capability::GPU_ANY);

    /**
     * @brief Revoke all capabilities from @p caller_id and remove the entry.
     */
    void revokeAll(const std::string& caller_id);

    // -----------------------------------------------------------------------
    // Check
    // -----------------------------------------------------------------------

    /**
     * @brief Check whether @p caller_id holds @p cap.
     *
     * Default-deny: returns denied decision if the caller is unknown or if
     * the specific capability has not been granted.
     */
    PolicyDecision check(const std::string& caller_id,
                         Capability cap = Capability::GPU_ALLOCATE) const;

    /**
     * @brief Returns true if @p caller_id is allowed @p cap (convenience wrapper).
     */
    bool isAllowed(const std::string& caller_id,
                   Capability cap = Capability::GPU_ALLOCATE) const;

    // -----------------------------------------------------------------------
    // Queries
    // -----------------------------------------------------------------------

    /**
     * @brief List all caller IDs that currently hold at least one capability.
     */
    std::vector<std::string> grantedCallers() const;

    /**
     * @brief List capabilities held by @p caller_id.
     */
    std::vector<Capability> capabilitiesOf(const std::string& caller_id) const;

    /**
     * @brief Return the number of callers with at least one capability.
     */
    size_t grantedCount() const;

private:
    mutable std::mutex mutex_;

    // caller_id → set of granted capabilities
    std::unordered_map<std::string,
                       std::unordered_set<int>>  grants_;  // int = enum cast

    static int cap_to_int(Capability c) { return static_cast<int>(c); }

    bool hasCapability(const std::string& id, Capability cap) const;
};

// ---------------------------------------------------------------------------
// Capability name helper (for messages and logs)
// ---------------------------------------------------------------------------
const char* capabilityName(GPUPolicy::Capability cap);

} // namespace gpu
} // namespace themis

