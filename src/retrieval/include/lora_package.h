/**
 * @file lora_package.h
 * @brief LoRAPackage and PortableAdapterProduct artifact classes for the ThemisDB
 *        adapter lifecycle — Phase 3 manifest, serialization, and integrity APIs.
 *
 * ## Artifact Taxonomy
 *
 * | Class                  | Role               | Rebuildable | Model-bound |
 * |------------------------|--------------------|-------------|-------------|
 * | LoRAPackage            | Source of truth    | Yes         | No          |
 * | PortableAdapterProduct | Deployable product | No          | Yes         |
 *
 * ### LoRAPackage
 * Represents a LoRA adapter in its source/policy form.  It carries:
 *   - Full lineage and provenance metadata (dataset, base-model, trainer)
 *   - Usage policy and deployment constraints
 *   - Cryptographic integrity hash and Ed25519/ECDSA signature
 *   - Compatibility metadata (architecture family, rank, alpha, target modules)
 *
 * A LoRAPackage is the **source of truth** for rebuild and audit workflows.
 * It does not carry compiled/quantized weights — it references them.
 *
 * ### PortableAdapterProduct
 * A concrete, deployable artifact produced from a LoRAPackage for a specific
 * base model at a specific quantization level.  It carries:
 *   - Back-reference to the originating LoRAPackage
 *   - Target model binding (model ID, architecture, quantization)
 *   - Binary format descriptor and per-file checksums
 *   - Runtime resource envelope (context length, VRAM footprint)
 *   - Deployment lifecycle status
 *
 * ### LoRAManifestStore
 * Persistent manifest registry backed by RocksDB when constructed with
 * RuntimeConfig. A source-compatible default constructor is retained only as
 * an isolated test fixture; it has no verifier and rejects all artifact writes.
 *
 * ## Serialization
 * Both artifact classes serialise to/from `nlohmann::json`.  The canonical
 * JSON representation is deterministic (field order fixed by to_json) so that
 * SHA-256 over the canonical bytes is reproducible across platforms.
 *
 * ## Integrity & Signing
 * Signing is intentionally decoupled from artifact construction.  The
 * `IntegrityHelper` namespace provides hash and verification utilities.
 * Callers are responsible for providing key material; this header does not
 * embed key management logic.
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <map>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <chrono>
#include <functional>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include "plugins/signed_plugin_repository.h"
#include "rag/deployment_gate_controller.h"
#include "rag/quality_metrics_collector.h"

namespace rocksdb {
class DB;
}

namespace themis {
namespace retrieval {

using json = nlohmann::json;

// ============================================================================
// Forward declarations
// ============================================================================

struct LoRAPackage;
struct PortableAdapterProduct;
class  LoRAManifestStore;

// ============================================================================
// Shared enumerations
// ============================================================================

enum class LoRAPackageStatus {
    DRAFT,       ///< Under development; not cleared for deployment
    VALIDATED,   ///< Integrity and policy checks passed; eligible for product builds
    DEPRECATED,  ///< Superseded by a newer package; existing products still valid
    REVOKED      ///< Security event; all derived products must be re-evaluated
};

enum class AdapterProductStatus {
    BUILDING,  ///< Product binary is being assembled
    READY,     ///< Binary complete; integrity verified; deployable
    DEPLOYED,  ///< Active in at least one inference endpoint
    RETIRED,   ///< Gracefully withdrawn from service
    FAILED     ///< Build or deployment failure; not usable
};

// ============================================================================
// Shared value types
// ============================================================================

struct AdapterUsagePolicy {
    std::string license;

    std::string restrictions;

    std::vector<std::string> allowed_base_models;

    int max_concurrent_deployments = 0;

    std::string expiry_date;

    [[nodiscard]] json to_json() const;

    [[nodiscard]] static AdapterUsagePolicy from_json(const json& j);
};

struct LoRAPackageProvenance {
    std::string trainer_id;            ///< Identity of the entity that ran training
    std::string training_framework;    ///< e.g. "PEFT-0.14", "LoRA-custom-1.2"
    std::string dataset_id;            ///< Logical dataset identifier
    std::string dataset_hash;          ///< SHA-256 of the training dataset content
    std::string base_model_id;         ///< Identifier of the base model used
    std::string base_model_hash;       ///< SHA-256 of the base model weights
    std::string hyperparameter_hash;   ///< SHA-256 of serialised hyperparameters
    double      training_duration_secs = 0.0; ///< Wall-clock training time
    std::string created_at;            ///< ISO 8601 UTC creation timestamp
    json        hardware_info;         ///< GPU / CPU / VRAM details (free-form)
    json        custom_metadata;       ///< Application-specific extensions

    [[nodiscard]] json to_json() const;
    [[nodiscard]] static LoRAPackageProvenance from_json(const json& j);
};

struct ArtifactIntegrity {
    std::string weights_hash;    ///< SHA-256 of the adapter weight file(s)
    std::string manifest_hash;   ///< SHA-256 of the canonical manifest JSON bytes

    std::string signature = {};

    std::string signature_algorithm; ///< e.g. "Ed25519", "ECDSA-P256-SHA256"
    std::string signer_id;           ///< Key / identity used for signing
    std::string signed_at;           ///< ISO 8601 UTC timestamp of signing

    bool signature_verified = false;

    [[nodiscard]] json to_json() const;
    [[nodiscard]] static ArtifactIntegrity from_json(const json& j);
};

// ============================================================================
// LoRAPackage
// ============================================================================

struct LoRAPackage {
    // ── Identity ──────────────────────────────────────────────────────────
    std::string package_id;   ///< Unique package identifier (UUID or hash-based)
    std::string name;         ///< Human-readable name (e.g. "legal-qa-lora-v2")
    std::string version;      ///< Semantic version string ("MAJOR.MINOR.PATCH")
    std::string description;  ///< Optional free-text description

    // ── Architecture compatibility ────────────────────────────────────────
    std::vector<std::string> supported_architectures;

    int    lora_rank  = 8;    ///< LoRA rank (r); must be > 0
    float  lora_alpha = 16.0f; ///< LoRA scaling factor (α); must be > 0

    std::vector<std::string> target_modules;

    // ── Lineage ──────────────────────────────────────────────────────────
    std::string parent_package_id;

    LoRAPackageProvenance provenance;

    // ── Policy ───────────────────────────────────────────────────────────
    AdapterUsagePolicy policy;

    // ── Weight reference ─────────────────────────────────────────────────
    std::string weights_path;

    // ── Integrity ────────────────────────────────────────────────────────
    ArtifactIntegrity integrity;

    // ── Status & timestamps ───────────────────────────────────────────────
    LoRAPackageStatus status = LoRAPackageStatus::DRAFT;
    std::string created_at;  ///< ISO 8601 UTC creation timestamp
    std::string updated_at;  ///< ISO 8601 UTC last-modification timestamp

    // ── Serialization ────────────────────────────────────────────────────

    [[nodiscard]] json to_json() const;

    [[nodiscard]] static LoRAPackage from_json(const json& j);

    /**
     * @brief ── Helpers ──────────────────────────────────────────────────────────
     */

    /**
     * @brief Compute the canonical artifact-content hash.
     * @note Signature metadata and mutable lifecycle status/update timestamps
     *       are excluded; identity, provenance, policy, and artifact content
     *       remain covered.
     */
    void computeManifestHash();

    [[nodiscard]] bool supportsArchitecture(const std::string& arch) const;

    [[nodiscard]] std::string statusToString() const;

    /**
     * @brief Status From String.
     * @param[in] s Input parameter.
     * @return Return value.
     */
    static LoRAPackageStatus statusFromString(const std::string& s);
};

// ============================================================================
// PortableAdapterProduct
// ============================================================================

struct PortableAdapterProduct {
    // ── Identity ──────────────────────────────────────────────────────────
    std::string product_id;         ///< Unique product identifier (UUID or hash-based)
    std::string name;               ///< Human-readable product name
    std::string version;            ///< Semantic version string
    std::string source_package_id;  ///< ID of the originating LoRAPackage

    // ── Model binding ─────────────────────────────────────────────────────
    std::string target_base_model_id;       ///< Specific base model this product targets
    std::string target_model_architecture;  ///< Architecture family ("llama", "mistral", …)

    std::string quantization = "none";

    // ── Binary descriptor ─────────────────────────────────────────────────
    std::string format = "SafeTensors";

    std::string file_path;

    size_t file_size_bytes = 0; ///< Byte size of the binary file(s)

    // ── Runtime resource envelope ─────────────────────────────────────────
    int    max_context_length      = 0;   ///< Supported maximum context tokens (0 = inherit from base)
    size_t memory_requirement_mb   = 0;   ///< Estimated peak VRAM / RAM footprint in MiB

    // ── Compatibility assertions ──────────────────────────────────────────
    std::vector<std::string> compatible_model_versions;

    // ── Integrity ────────────────────────────────────────────────────────
    ArtifactIntegrity integrity;

    // ── Status & timestamps ───────────────────────────────────────────────
    AdapterProductStatus status = AdapterProductStatus::BUILDING;
    std::string created_at;        ///< ISO 8601 UTC creation timestamp
    std::string updated_at;        ///< ISO 8601 UTC last-modification timestamp
    std::string deployed_at;       ///< ISO 8601 UTC deployment timestamp (empty if not yet deployed)

    // ── Serialization ────────────────────────────────────────────────────

    [[nodiscard]] json to_json() const;

    [[nodiscard]] static PortableAdapterProduct from_json(const json& j);

    /**
     * @brief ── Helpers ──────────────────────────────────────────────────────────
     */

    /**
     * @brief Compute the canonical product-content hash.
     * @note Signature metadata and mutable lifecycle status/update/deployment
     *       timestamps are excluded; model binding and product content remain
     *       covered.
     */
    void computeManifestHash();

    [[nodiscard]] std::string statusToString() const;

    /**
     * @brief Status From String.
     * @param[in] s Input parameter.
     * @return Return value.
     */
    static AdapterProductStatus statusFromString(const std::string& s);
};

// ============================================================================
// Integrity utilities
// ============================================================================

namespace IntegrityHelper {

[[nodiscard]] std::string sha256Hex(const uint8_t* data, size_t size);

[[nodiscard]] std::string sha256Hex(const std::string& input);

[[nodiscard]] bool verifyHash(const uint8_t* data, size_t size,
                               const std::string& expected_hex);

[[nodiscard]] bool verifyHash(const std::string& input,
                               const std::string& expected_hex);

} // namespace IntegrityHelper

// ============================================================================
// LoRAManifestStore
// ============================================================================

class LoRAManifestStore {
public:
    using SignatureVerifier = std::function<bool(
        const std::string& manifest_hash,
        const std::string& signature,
        const std::string& signer_id)>;
    /**
     * @brief Owning deployment-service authorization boundary.
     * @param product Signed product snapshot under consideration.
     * @param source_package Signed, validated source-package snapshot.
     * @return true only when the owning service authorizes deployment; false
     *         or an exception denies the deployment.
     * @note This callback is not an owner/RBAC implementation. It must be
     *       supplied by the owning deployment service.
     */
    using DeploymentAuthorization = std::function<bool(
        const PortableAdapterProduct& product,
        const LoRAPackage& source_package)>;
    /**
     * @brief Validate that deployment evidence is authentic and bound to a product.
     * @param product Exact signed product snapshot selected for deployment.
     * @param candidate_model_version Candidate version asserted by the service.
     * @param baseline_model_version Baseline version asserted by the service.
     * @param candidate_metrics Candidate metrics asserted by the service.
     * @param baseline_metrics Baseline metrics asserted by the service.
     * @return true only when the owning service validates the evidence and its
     *         binding to this exact product; false or exceptions deny deployment.
     * @note This validates evidence only. It is not deployment authorization or
     *       a quality decision.
     */
    using DeploymentEvidenceValidator = std::function<bool(
        const PortableAdapterProduct& product,
        uint32_t candidate_model_version,
        uint32_t baseline_model_version,
        const themis::rag::quality::AggregatedMetrics& candidate_metrics,
        const themis::rag::quality::AggregatedMetrics& baseline_metrics)>;
    /**
     * @brief Owner-service decision for an explicit lifecycle transition.
     * @param artifact_type "package" or "product".
     * @param artifact Full current artifact JSON snapshot.
     * @param current_status Existing status string.
     * @param requested_status Requested status string.
     * @return true only when the owner service authorizes this exact transition.
     * @note The registry independently validates and persists the transition.
     */
    using LifecycleTransitionAuthorizer = std::function<bool(
        const std::string& artifact_type,
        const json& artifact,
        const std::string& current_status,
        const std::string& requested_status)>;
    /**
     * @brief Check persisted lifecycle state against the owner service's authority.
     * @param artifact_type "package" or "product".
     * @param artifact Full persisted artifact JSON snapshot.
     * @param status Persisted status string.
     * @return true only when the owner service recognizes this exact state.
     * @note Required when reopening any non-initial lifecycle state. It must
     *       consult authoritative owner state, not trust the RocksDB value alone.
     */
    using PersistedLifecycleVerifier = std::function<bool(
        const std::string& artifact_type,
        const json& artifact,
        const std::string& status)>;

    /**
     * @brief Runtime configuration for the persistent artifact registry.
     * @note Signature verification is bound internally to the shared
     *       `SignedPluginRepository` Ed25519 primitive over the supplied
     *       manifest_hash with the active key whose fingerprint equals
     *       signer_id; callers cannot replace it.
     *       Authorization is delegated to the owning deployment service; it
     *       is never inferred from artifact metadata.
     */
    struct RuntimeConfig {
        std::string database_path; ///< Dedicated persistent RocksDB directory.
        std::vector<themis::plugins::PinnedKey> pinned_keys; ///< Active Ed25519 public-key pins.
        DeploymentAuthorization deployment_authorizer; ///< Owning-service authorization boundary.
        DeploymentEvidenceValidator deployment_evidence_validator; ///< Authenticates product-bound model versions/metrics; not authorization.
        LifecycleTransitionAuthorizer lifecycle_transition_authorizer; ///< Owner-service approval for non-default lifecycle transitions.
        PersistedLifecycleVerifier persisted_lifecycle_verifier; ///< Owner-authoritative validation on transition and reopen.
        std::shared_ptr<themis::rag::quality::DeploymentGateController> quality_gate; ///< Quality only; not authorization.

        /**
         * @brief Load typed retrieval settings from a YAML file.
         * @param yaml_path Resolved ThemisDB YAML config path.
         * @return Validated persistent settings with active pinned-key material.
         * @throws std::invalid_argument for malformed, incomplete, unpinned, or
         *         unsupported settings; std::runtime_error for file access errors.
         */
        [[nodiscard]] static RuntimeConfig loadFromYaml(const std::string& yaml_path);

        /**
         * @brief Load the canonical retrieval settings through ConfigPathResolver.
         * @return Validated production runtime settings.
         * @throws std::exception when the config is absent or invalid.
         */
        [[nodiscard]] static RuntimeConfig loadFromThemisConfig();
    };

    /**
     * @brief Outcome of the product deployment decision.
     * @note Quality is evaluated independently from authorization. A quality
     *       warning is not an allow decision and therefore does not deploy.
     */
    struct DeploymentResult {
        bool deployed = false; ///< True only after all checks and durable write succeed.
        themis::rag::quality::GateDecision quality_decision; ///< Quality-only decision; authorization is separate.
        std::string reason; ///< Failure reason or successful completion summary.
    };

    /**
     * @brief Construct a production store from config/ai_ml/retrieval.yaml.
     * @return Owned persistent store; construction fails on missing/invalid config.
     * @throws std::exception if configuration or RocksDB initialization fails.
     */
    [[nodiscard]] static std::unique_ptr<LoRAManifestStore> openFromThemisConfig();

    /**
     * @brief Construct an isolated, non-production compatibility fixture.
     * @note NON-PRODUCTION: this instance has no persistent DB or pinned keys.
     *       All artifact writes/imports and deployments fail closed. Production
     *       callers must use openFromThemisConfig().
     */
    LoRAManifestStore();

    /**
     * @brief Open the persistent RocksDB artifact registry with validated settings.
     * @param config Runtime settings with a non-empty DB path and at least one
     *        valid pinned Ed25519 key. The constructor binds the required
     *        verifier internally to ThemisDB's shared pinned-key primitive.
     * @throws std::invalid_argument for missing/invalid required settings.
     * @throws std::runtime_error if RocksDB cannot be opened or persisted
     *         records are malformed, unsigned, or fail verification.
     * @note Production composition should use openFromThemisConfig(); this
     *       constructor is also available for controlled tests. Callers cannot
     *       replace or disable the cryptographic verifier.
     */
    explicit LoRAManifestStore(RuntimeConfig config);
    ~LoRAManifestStore();

    // RocksDB ownership and mutex lifetime make instances non-copyable/non-movable.
    LoRAManifestStore(const LoRAManifestStore&) = delete;
    LoRAManifestStore& operator=(const LoRAManifestStore&) = delete;
    LoRAManifestStore(LoRAManifestStore&&) = delete;
    LoRAManifestStore& operator=(LoRAManifestStore&&) = delete;

    /**
     * @brief Verify and persist a signed package record.
     * @param pkg Package with non-empty ID, matching content hash, Ed25519
     *        signature, and signer ID matching a configured active pin.
     * @return true if the synchronous RocksDB write succeeds; false for an
     *         invalid/unsigned artifact, empty ID, or write error.
     * @note Safe for concurrent calls; an error does not update the in-memory
     *       view.
     */
    bool storePackage(const LoRAPackage& pkg);

    [[nodiscard]] std::optional<LoRAPackage> loadPackage(
        const std::string& package_id) const;

    /**
     * @brief Delete Package.
     * @param[in] package_id Identifier of the package.
     * @return True when the operation succeeds.
     */
    bool deletePackage(const std::string& package_id);

    /**
     * @brief Apply an owner-authorized package lifecycle transition.
     * @param package_id Existing signed package.
     * @param expected_status Required current status; protects against stale callers.
     * @param requested_status Target status; only legal forward/revocation transitions are accepted.
     * @return true only when owner authorization, authoritative-state validation,
     *         and synchronous persistence all succeed.
     * @note Ordinary store/import APIs accept DRAFT only. Missing, denying, or
     *       throwing lifecycle callbacks fail closed.
     */
    [[nodiscard]] bool transitionPackageStatus(
        const std::string& package_id,
        LoRAPackageStatus expected_status,
        LoRAPackageStatus requested_status);

    [[nodiscard]] std::vector<std::string> listPackageIds() const;

    [[nodiscard]] std::vector<LoRAPackage> listPackagesByStatus(
        LoRAPackageStatus status) const;

    /**
     * @brief Verify and persist a signed product that is not marked DEPLOYED.
     * @param product Product with non-empty product_id and source_package_id.
     *        Must also have a matching content hash, Ed25519 signature, and
     *        signer ID matching a configured active pin. DEPLOYED is reserved
     *        for deployProduct().
     * @return true if the synchronous RocksDB write succeeds; false for invalid
     *         identity, a DEPLOYED status, or write error.
     * @note Safe for concurrent calls; an error does not update the in-memory
     *       view.
     */
    bool storeProduct(const PortableAdapterProduct& product);

    [[nodiscard]] std::optional<PortableAdapterProduct> loadProduct(
        const std::string& product_id) const;

    /**
     * @brief Delete Product.
     * @param[in] product_id Identifier of the product.
     * @return True when the operation succeeds.
     */
    bool deleteProduct(const std::string& product_id);

    /**
     * @brief Apply an owner-authorized product lifecycle transition.
     * @param product_id Existing signed product.
     * @param expected_status Required current status; protects against stale callers.
     * @param requested_status Target status; DEPLOYED is reserved for deployProduct().
     * @return true only when owner authorization, authoritative-state validation,
     *         and synchronous persistence all succeed.
     * @note Ordinary store/import APIs accept BUILDING only. Missing, denying, or
     *       throwing lifecycle callbacks fail closed.
     */
    [[nodiscard]] bool transitionProductStatus(
        const std::string& product_id,
        AdapterProductStatus expected_status,
        AdapterProductStatus requested_status);

    [[nodiscard]] std::vector<std::string> listProductIds() const;

    [[nodiscard]] std::vector<PortableAdapterProduct> listProductsByPackage(
        const std::string& package_id) const;

    [[nodiscard]] std::vector<PortableAdapterProduct> listProductsByStatus(
        AdapterProductStatus status) const;

    // ── Integrity verification ────────────────────────────────────────────

    /**
     * @brief Verify a package's content hash and detached signature.
     * @param package_id Existing package identifier.
     * @return true only for a matching content hash, Ed25519 signature,
     *         configured pinned signer ID, and successful cryptographic verification.
     *         Missing records/configuration or any verification error returns false.
     */
    [[nodiscard]] bool verifyPackageIntegrity(const std::string& package_id) const;

    /**
     * @brief Verify a product's content hash and detached signature.
     * @param product_id Existing product identifier.
     * @return true only for a matching content hash, Ed25519 signature,
     *         configured pinned signer ID, and successful cryptographic verification.
     *         Missing records/configuration or any verification error returns false.
     */
    [[nodiscard]] bool verifyProductIntegrity(const std::string& product_id) const;

    /**
     * @brief Deploy a ready product after independent quality and authorization checks.
     * @param product_id Existing product whose status must be READY.
     * @param candidate_model_version Candidate model version supplied by the
     *        deployment service.
     * @param baseline_model_version Baseline model version supplied by the
     *        deployment service.
     * @param candidate_metrics Non-empty quality evaluation for the candidate.
     * @param baseline_metrics Non-empty quality evaluation for the baseline.
     * @return deployed=true only if both artifact signatures are valid, the
     *         evidence validator confirms exact product/version/metric binding,
     *         the quality controller returns kAllow, the separate authorization
     *         callback approves, and the update is durably persisted.
     * @note Missing/throwing evidence validation, lifecycle-state validation,
     *       quality controller, metrics, signatures, or authorization fail
     *       closed. The authorization callback is an integration boundary, not
     *       an implementation of owner/RBAC policy.
     * @note External callbacks run without the store mutex. The artifact
     *       snapshots are rechecked before the deployment write to reject races.
     */
    [[nodiscard]] DeploymentResult deployProduct(
        const std::string& product_id,
        uint32_t candidate_model_version,
        uint32_t baseline_model_version,
        const themis::rag::quality::AggregatedMetrics& candidate_metrics,
        const themis::rag::quality::AggregatedMetrics& baseline_metrics);

    // ── Bulk export / import ──────────────────────────────────────────────

    [[nodiscard]] json exportPackages() const;

    /**
     * @brief Import only cryptographically valid, pinned-key signed packages.
     * @param j JSON array of package records; malformed or unsigned entries are
     *        skipped without altering the registry.
     * @return Number of entries successfully persisted.
     * @note Safe for concurrent calls; writes are synchronous to RocksDB.
     */
    size_t importPackages(const json& j);

    [[nodiscard]] json exportProducts() const;

    /**
     * @brief Import only cryptographically valid, pinned-key signed products.
     * @param j JSON array of product records; malformed, unsigned, or DEPLOYED
     *        entries are skipped without altering the registry.
     * @return Number of entries successfully persisted.
     * @note Safe for concurrent calls; writes are synchronous to RocksDB.
     */
    size_t importProducts(const json& j);

    // ── Statistics ────────────────────────────────────────────────────────

    [[nodiscard]] size_t packageCount() const;

    [[nodiscard]] size_t productCount() const;

private:
    mutable std::mutex mutex_;

    std::unordered_map<std::string, LoRAPackage>           packages_;
    std::unordered_map<std::string, PortableAdapterProduct> products_;

    RuntimeConfig config_;
    SignatureVerifier signature_verifier_;
    std::unique_ptr<rocksdb::DB> db_;
};

} // namespace retrieval
} // namespace themis
