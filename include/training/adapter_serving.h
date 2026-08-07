/**
 * @file adapter_serving.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 96/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=2, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>

namespace themis {
namespace training {

// ============================================================================
// ILLMRouter — abstract LLM router interface
// ============================================================================

/**
 * @brief Abstract interface for the LLM inference router.
 *
 * The training module calls this interface to propagate adapter version
 * weights to the live inference layer.  Production callers inject a
 * concrete implementation (e.g., backed by MultiLoRAManager); tests inject
 * a mock.
 *
 * Threading contract: implementations must be thread-safe; calls may arrive
 * from the deployment thread concurrently with ongoing inference requests.
 * The routing weight update must be atomic from the request handler's
 * perspective (no mid-request split change is observable).
 */
class ILLMRouter {
public:
    virtual ~ILLMRouter();

    /**
     * @brief Update the traffic weight for the named adapter version.
     *
     * Sets the fraction of incoming inference requests that should be
     * routed to @p version.  Implementations must normalise weights across
     * all registered versions so that the total sums to 1.0.
     *
     * @param version Adapter version identifier (e.g., "legal_v1.1").
     * @param weight  Desired traffic fraction in [0.0, 1.0].
     * @return true on success; false if the version is unknown to the router
     *         (the caller may retry after loading the adapter).
     */
    [[nodiscard]] virtual bool setAdapterWeight(const std::string& version, float weight) = 0;

    /**
     * @brief Whether the router is reachable and ready to accept weight updates.
     */
    [[nodiscard]] virtual bool isAvailable() const = 0;

    /**
     * @brief Return the version identifier that currently receives 100% of
     *        traffic, or an empty string if no version is fully active.
     */
    [[nodiscard]] virtual std::string activeVersion() const = 0;
};

// ============================================================================
// DeployResult
// ============================================================================

/**
 * @brief Result of a deployVersion() or rollbackVersion() operation.
 *
 * Returned by IncrementalLoRATrainer::deployVersionEx() and
 * IncrementalLoRATrainer::rollbackVersionEx().
 */
struct DeployResult {
    bool        success        = false;  ///< Whether the operation succeeded
    std::string active_version;          ///< Version that now receives traffic
    float       split_applied  = 0.0f;  ///< Traffic fraction routed to active_version
    std::string error;                   ///< Non-empty on failure ("version_not_found", etc.)

    DeployResult() = default;

    /// Convenience: create a successful result.
    static DeployResult ok(const std::string& version, float split) {
        DeployResult r;
        r.success        = true;
        r.active_version = version;
        r.split_applied  = split;
        return r;
    }

    /// Convenience: create a failed result.
    static DeployResult fail(const std::string& reason) {
        DeployResult r;
        r.success = false;
        r.error   = reason;
        return r;
    }
};

// ============================================================================
// Phase 2: Deployment Validation (determinism and readiness checks)
// ============================================================================

/**
 * @brief Validate an adapter before deployment to serving infrastructure.
 *
 * Phase 2 hardening: performs comprehensive deployment readiness checks
 * including version string validation, state consistency, and
 * pre-deployment health assessment.
 *
 * @param adapter_version Version identifier (must be non-empty, valid format)
 * @param checkpoint_path Path to the checkpoint file (must exist and be readable)
 * @param expected_sha256 Expected SHA-256 of checkpoint (if empty, skipped)
 * @return Empty string if valid; otherwise error message describing the issue
 */
std::string validateDeploymentReadiness(
    const std::string& adapter_version,
    const std::string& checkpoint_path,
    const std::string& expected_sha256 = "");

/**
 * @brief Generate deterministic deployment fingerprint for an adapter.
 *
 * Creates a stable identifier based on version, checkpoint hash, and
 * deployment timestamp. Used to ensure repeatable deployment decisions
 * and audit trail consistency.
 *
 * @param adapter_version Version identifier
 * @param checkpoint_sha256 SHA-256 of the checkpoint file
 * @return Deterministic fingerprint string (hex-encoded)
 */
std::string computeDeploymentFingerprint(
    const std::string& adapter_version,
    const std::string& checkpoint_sha256);

} // namespace training
} // namespace themis
