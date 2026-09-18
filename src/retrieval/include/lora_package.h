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
 * Persistent manifest registry backed by an in-memory map (real storage
 * integration via RocksDB/file is wired by the caller).  Provides
 * CRUD and integrity-check helpers for both artifact classes.
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

    LoRAManifestStore() = default;
    ~LoRAManifestStore() = default;

    // Non-copyable; movable
    LoRAManifestStore(const LoRAManifestStore&) = delete;
    LoRAManifestStore& operator=(const LoRAManifestStore&) = delete;
    LoRAManifestStore(LoRAManifestStore&&) = default;
    LoRAManifestStore& operator=(LoRAManifestStore&&) = default;

    /**
     * @brief ── Configuration ─────────────────────────────────────────────────────
     * @param[in] verifier Input parameter.
     */

    void setSignatureVerifier(SignatureVerifier verifier);

    /**
     * @brief ── LoRAPackage CRUD ──────────────────────────────────────────────────
     * @param[in] pkg Input parameter.
     * @return True when the operation succeeds.
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

    [[nodiscard]] std::vector<std::string> listPackageIds() const;

    [[nodiscard]] std::vector<LoRAPackage> listPackagesByStatus(
        LoRAPackageStatus status) const;

    /**
     * @brief ── PortableAdapterProduct CRUD ───────────────────────────────────────
     * @param[in] product Input parameter.
     * @return True when the operation succeeds.
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

    [[nodiscard]] std::vector<std::string> listProductIds() const;

    [[nodiscard]] std::vector<PortableAdapterProduct> listProductsByPackage(
        const std::string& package_id) const;

    [[nodiscard]] std::vector<PortableAdapterProduct> listProductsByStatus(
        AdapterProductStatus status) const;

    // ── Integrity verification ────────────────────────────────────────────

    [[nodiscard]] bool verifyPackageIntegrity(const std::string& package_id) const;

    [[nodiscard]] bool verifyProductIntegrity(const std::string& product_id) const;

    // ── Bulk export / import ──────────────────────────────────────────────

    [[nodiscard]] json exportPackages() const;

    /**
     * @brief Import Packages.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    size_t importPackages(const json& j);

    [[nodiscard]] json exportProducts() const;

    /**
     * @brief Import Products.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    size_t importProducts(const json& j);

    // ── Statistics ────────────────────────────────────────────────────────

    [[nodiscard]] size_t packageCount() const;

    [[nodiscard]] size_t productCount() const;

private:
    mutable std::mutex mutex_;

    std::unordered_map<std::string, LoRAPackage>           packages_;
    std::unordered_map<std::string, PortableAdapterProduct> products_;

    SignatureVerifier signature_verifier_;
};

} // namespace retrieval
} // namespace themis
