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

/**
 * @brief Lifecycle status of a LoRAPackage.
 *
 * Transitions:
 *   DRAFT → VALIDATED → DEPRECATED | REVOKED
 */
enum class LoRAPackageStatus {
    DRAFT,       ///< Under development; not cleared for deployment
    VALIDATED,   ///< Integrity and policy checks passed; eligible for product builds
    DEPRECATED,  ///< Superseded by a newer package; existing products still valid
    REVOKED      ///< Security event; all derived products must be re-evaluated
};

/**
 * @brief Lifecycle status of a PortableAdapterProduct.
 *
 * Transitions:
 *   BUILDING → READY → DEPLOYED → RETIRED | FAILED
 */
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

/**
 * @brief Policy metadata governing LoRA adapter usage and deployment.
 *
 * Captures data-governance and operational constraints declared at package
 * creation time.  A missing or empty field is treated as "no restriction".
 */
struct AdapterUsagePolicy {
    /**
     * @brief SPDX identifier or custom license tag (e.g. "MIT", "CC-BY-4.0").
     * Empty implies proprietary / all-rights-reserved.
     */
    std::string license;

    /**
     * @brief Human-readable description of usage restrictions.
     * Examples: "Research use only", "No commercial redistribution".
     */
    std::string restrictions;

    /**
     * @brief Allow-list of base model IDs this package may be compiled against.
     * Empty list means "no restriction on base model".
     */
    std::vector<std::string> allowed_base_models;

    /**
     * @brief Maximum number of simultaneous deployments permitted.
     * 0 means unlimited.
     */
    int max_concurrent_deployments = 0;

    /**
     * @brief ISO 8601 UTC expiry date ("YYYY-MM-DD").  Empty = no expiry.
     */
    std::string expiry_date;

    /**
     * @brief Serialize to JSON.
     * @return json representation
     */
    [[nodiscard]] json to_json() const;

    /**
     * @brief Deserialize from JSON.
     * @param j source JSON object
     * @return AdapterUsagePolicy
     * @throws std::invalid_argument if required fields are missing or malformed
     */
    [[nodiscard]] static AdapterUsagePolicy from_json(const json& j);
};

/**
 * @brief Provenance metadata embedded in a LoRAPackage manifest.
 *
 * Provides enough information to reconstruct the training environment and
 * audit the data / model lineage.  Cryptographic hashes use SHA-256.
 */
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

/**
 * @brief Cryptographic integrity record for an artifact.
 *
 * Covers the artifact's canonical JSON bytes (for manifests) and/or the
 * raw binary weights (for weight files).
 */
struct ArtifactIntegrity {
    std::string weights_hash;    ///< SHA-256 of the adapter weight file(s)
    std::string manifest_hash;   ///< SHA-256 of the canonical manifest JSON bytes

    /**
     * @brief Ed25519 or ECDSA P-256 signature over `manifest_hash`.
     * Base64-encoded DER blob.  Empty when the artifact has not been signed.
     */
    std::string signature = {};

    std::string signature_algorithm; ///< e.g. "Ed25519", "ECDSA-P256-SHA256"
    std::string signer_id;           ///< Key / identity used for signing
    std::string signed_at;           ///< ISO 8601 UTC timestamp of signing

    /**
     * @brief True when the signature has been verified against the public key.
     *
     * This field is **not** persisted and is **not** updated automatically by
     * the store.  Callers that want to cache the verification result must set
     * this field themselves after calling
     * `LoRAManifestStore::verifyPackageIntegrity()` /
     * `LoRAManifestStore::verifyProductIntegrity()`, which return the result
     * as a `bool` but do not mutate the stored artifact.
     */
    bool signature_verified = false;

    [[nodiscard]] json to_json() const;
    [[nodiscard]] static ArtifactIntegrity from_json(const json& j);
};

// ============================================================================
// LoRAPackage
// ============================================================================

/**
 * @brief Source-of-truth artifact for a LoRA adapter.
 *
 * A LoRAPackage:
 *   - is rebuildable (given the same dataset + base model + hyperparameters)
 *   - is not bound to any specific base model version at construction time
 *   - carries full lineage, policy, and compatibility metadata
 *   - serves as the root for one or more PortableAdapterProduct artifacts
 *
 * ### Lifecycle
 * ```
 * DRAFT ──► VALIDATED ──► DEPRECATED
 *                    ╰──► REVOKED
 * ```
 *
 * ### Serialization
 * `to_json()` produces a deterministic canonical representation.
 * `from_json()` validates required fields and throws `std::invalid_argument`
 * on structural errors.
 *
 * @see PortableAdapterProduct
 * @see LoRAManifestStore
 */
struct LoRAPackage {
    // ── Identity ──────────────────────────────────────────────────────────
    std::string package_id;   ///< Unique package identifier (UUID or hash-based)
    std::string name;         ///< Human-readable name (e.g. "legal-qa-lora-v2")
    std::string version;      ///< Semantic version string ("MAJOR.MINOR.PATCH")
    std::string description;  ///< Optional free-text description

    // ── Architecture compatibility ────────────────────────────────────────
    /**
     * @brief Supported model architecture families (e.g. "llama", "mistral").
     * Products may only be built for base models whose architecture is listed.
     * Empty means "architecture-agnostic" (rare; prefer explicit allow-listing).
     */
    std::vector<std::string> supported_architectures;

    int    lora_rank  = 8;    ///< LoRA rank (r); must be > 0
    float  lora_alpha = 16.0f; ///< LoRA scaling factor (α); must be > 0

    /**
     * @brief Layer / module targets for LoRA injection.
     * Typical values: "q_proj", "v_proj", "k_proj", "o_proj".
     */
    std::vector<std::string> target_modules;

    // ── Lineage ──────────────────────────────────────────────────────────
    /**
     * @brief ID of the parent package this one was fine-tuned from.
     * Empty for root packages.
     */
    std::string parent_package_id;

    /// Full provenance record
    LoRAPackageProvenance provenance;

    // ── Policy ───────────────────────────────────────────────────────────
    AdapterUsagePolicy policy;

    // ── Weight reference ─────────────────────────────────────────────────
    /**
     * @brief Logical storage path for the raw adapter weights.
     * Interpretation is storage-backend specific (file path, object key, etc.).
     */
    std::string weights_path;

    // ── Integrity ────────────────────────────────────────────────────────
    ArtifactIntegrity integrity;

    // ── Status & timestamps ───────────────────────────────────────────────
    LoRAPackageStatus status = LoRAPackageStatus::DRAFT;
    std::string created_at;  ///< ISO 8601 UTC creation timestamp
    std::string updated_at;  ///< ISO 8601 UTC last-modification timestamp

    // ── Serialization ────────────────────────────────────────────────────

    /**
     * @brief Serialize to canonical JSON.
     *
     * Field order is deterministic so that SHA-256 over `to_json().dump()`
     * produces a stable manifest hash.
     *
     * @return json object
     */
    [[nodiscard]] json to_json() const;

    /**
     * @brief Deserialize from JSON.
     *
     * @param j source JSON object (must contain at minimum `package_id`, `name`,
     *          `version`)
     * @return LoRAPackage
     * @throws std::invalid_argument if required fields are absent or have wrong types
     */
    [[nodiscard]] static LoRAPackage from_json(const json& j);

    // ── Helpers ──────────────────────────────────────────────────────────

    /**
     * @brief Compute and store the manifest hash from the current fields.
     *
     * Sets `integrity.manifest_hash` to the SHA-256 hex digest of the
     * canonical manifest JSON.  The canonical form is derived from `to_json()`
     * with `integrity.manifest_hash` and all signature fields
     * (`signature`, `signature_algorithm`, `signer_id`, `signed_at`)
     * set to empty strings so that the hash covers only the artifact content.
     *
     * Must be called before signing.  Re-calling after signing preserves the
     * signature but updates the hash if content has changed.
     */
    void computeManifestHash();

    /**
     * @brief Check whether the given architecture is supported by this package.
     *
     * @param arch Architecture string (case-insensitive exact match)
     * @return true if `supported_architectures` is empty OR contains `arch`
     */
    [[nodiscard]] bool supportsArchitecture(const std::string& arch) const;

    /**
     * @brief Return a string representation of `status` for logging / display.
     */
    [[nodiscard]] std::string statusToString() const;

    /**
     * @brief Parse a status string back to enum.
     *
     * @param s status string (case-sensitive; must match enum name exactly)
     * @throws std::invalid_argument for unknown values
     */
    static LoRAPackageStatus statusFromString(const std::string& s);
};

// ============================================================================
// PortableAdapterProduct
// ============================================================================

/**
 * @brief Deployable, model-bound adapter product derived from a LoRAPackage.
 *
 * A PortableAdapterProduct:
 *   - is **not** rebuildable (it is a compiled / quantized binary artefact)
 *   - is bound to a specific base model identifier and quantization level
 *   - carries a per-file content hash for integrity verification at load time
 *   - references its source LoRAPackage for audit / lineage
 *
 * ### Lifecycle
 * ```
 * BUILDING ──► READY ──► DEPLOYED ──► RETIRED
 *         ╰──► FAILED
 * ```
 *
 * @see LoRAPackage
 * @see LoRAManifestStore
 */
struct PortableAdapterProduct {
    // ── Identity ──────────────────────────────────────────────────────────
    std::string product_id;         ///< Unique product identifier (UUID or hash-based)
    std::string name;               ///< Human-readable product name
    std::string version;            ///< Semantic version string
    std::string source_package_id;  ///< ID of the originating LoRAPackage

    // ── Model binding ─────────────────────────────────────────────────────
    std::string target_base_model_id;       ///< Specific base model this product targets
    std::string target_model_architecture;  ///< Architecture family ("llama", "mistral", …)

    /**
     * @brief Quantization level of the product binary.
     * Examples: "none" (FP16), "Q4_K_M", "Q8_0", "INT8", "INT4".
     */
    std::string quantization = "none";

    // ── Binary descriptor ─────────────────────────────────────────────────
    /**
     * @brief File format of the compiled adapter weights.
     * Examples: "SafeTensors", "GGUF-ST", "GGUF", "ONNX".
     */
    std::string format = "SafeTensors";

    /**
     * @brief Logical storage path for the compiled adapter binary.
     */
    std::string file_path;

    size_t file_size_bytes = 0; ///< Byte size of the binary file(s)

    // ── Runtime resource envelope ─────────────────────────────────────────
    int    max_context_length      = 0;   ///< Supported maximum context tokens (0 = inherit from base)
    size_t memory_requirement_mb   = 0;   ///< Estimated peak VRAM / RAM footprint in MiB

    // ── Compatibility assertions ──────────────────────────────────────────
    /**
     * @brief Explicit list of compatible base model versions.
     * Empty means "any version of target_base_model_id" (not recommended for
     * production deployments; prefer explicit version pinning).
     */
    std::vector<std::string> compatible_model_versions;

    // ── Integrity ────────────────────────────────────────────────────────
    ArtifactIntegrity integrity;

    // ── Status & timestamps ───────────────────────────────────────────────
    AdapterProductStatus status = AdapterProductStatus::BUILDING;
    std::string created_at;        ///< ISO 8601 UTC creation timestamp
    std::string updated_at;        ///< ISO 8601 UTC last-modification timestamp
    std::string deployed_at;       ///< ISO 8601 UTC deployment timestamp (empty if not yet deployed)

    // ── Serialization ────────────────────────────────────────────────────

    /**
     * @brief Serialize to canonical JSON.
     *
     * The same stability guarantee as LoRAPackage::to_json() applies.
     *
     * @return json object
     */
    [[nodiscard]] json to_json() const;

    /**
     * @brief Deserialize from JSON.
     *
     * @param j source JSON object (must contain at minimum `product_id`,
     *          `source_package_id`, `target_base_model_id`)
     * @return PortableAdapterProduct
     * @throws std::invalid_argument if required fields are absent or malformed
     */
    [[nodiscard]] static PortableAdapterProduct from_json(const json& j);

    // ── Helpers ──────────────────────────────────────────────────────────

    /**
     * @brief Compute and store the manifest hash from the current fields.
     *
     * Sets `integrity.manifest_hash` to the SHA-256 hex digest of the
     * canonical manifest JSON.  The canonical form is derived from `to_json()`
     * with `integrity.manifest_hash` and all signature fields
     * (`signature`, `signature_algorithm`, `signer_id`, `signed_at`)
     * set to empty strings so that the hash covers only the artifact content.
     *
     * Must be called before signing.
     */
    void computeManifestHash();

    /**
     * @brief Return a string representation of `status`.
     */
    [[nodiscard]] std::string statusToString() const;

    /**
     * @brief Parse a status string back to enum.
     *
     * @throws std::invalid_argument for unknown values
     */
    static AdapterProductStatus statusFromString(const std::string& s);
};

// ============================================================================
// Integrity utilities
// ============================================================================

/**
 * @brief Lightweight SHA-256 and signature-verification helpers.
 *
 * These utilities operate on in-memory data only; no I/O is performed.
 * Key management is the caller's responsibility.
 */
namespace IntegrityHelper {

/**
 * @brief Compute a SHA-256 hex digest of the given byte buffer.
 *
 * Uses a portable, dependency-free implementation suitable for manifest
 * hashing.  For performance-critical bulk file hashing, prefer the platform
 * crypto library.
 *
 * @param data  Pointer to the data buffer.  Must not be null if size > 0.
 * @param size  Number of bytes to hash.
 * @return 64-character lowercase hex string of the SHA-256 digest
 */
[[nodiscard]] std::string sha256Hex(const uint8_t* data, size_t size);

/**
 * @brief Compute a SHA-256 hex digest of a UTF-8 string.
 *
 * Convenience overload — delegates to the byte-buffer variant.
 *
 * @param input  UTF-8 string to hash.
 * @return 64-character lowercase hex string of the SHA-256 digest
 */
[[nodiscard]] std::string sha256Hex(const std::string& input);

/**
 * @brief Verify a SHA-256 hash against a data buffer.
 *
 * @param data        Data buffer.
 * @param size        Buffer size in bytes.
 * @param expected_hex  Expected 64-char hex digest.
 * @return true if `sha256Hex(data, size) == expected_hex`
 */
[[nodiscard]] bool verifyHash(const uint8_t* data, size_t size,
                               const std::string& expected_hex);

/**
 * @brief Verify a SHA-256 hash against a string.
 *
 * @param input        UTF-8 string to verify.
 * @param expected_hex Expected 64-char hex digest.
 * @return true if hash matches
 */
[[nodiscard]] bool verifyHash(const std::string& input,
                               const std::string& expected_hex);

} // namespace IntegrityHelper

// ============================================================================
// LoRAManifestStore
// ============================================================================

/**
 * @brief In-memory manifest registry for LoRAPackage and PortableAdapterProduct.
 *
 * Provides CRUD operations and integrity-verification helpers for both artifact
 * classes.  The store is thread-safe; all public methods are protected by an
 * internal mutex.
 *
 * ### Persistence
 * The store does not directly perform I/O.  Callers must serialise / restore
 * the store state via `exportPackages()` / `importPackages()` and the
 * corresponding product variants.  Integration with a RocksDB or file-backed
 * store is done at the application layer.
 *
 * ### Integrity verification
 * `verifyPackageIntegrity()` and `verifyProductIntegrity()` recompute the
 * manifest hash and compare it to the stored value.  Cryptographic signature
 * verification requires the caller to supply a verify callback (see
 * `setSignatureVerifier()`).
 *
 * @see LoRAPackage
 * @see PortableAdapterProduct
 */
class LoRAManifestStore {
public:
    /**
     * @brief Signature-verification callback type.
     *
     * The callback receives:
     *   - `manifest_hash`   — 64-char SHA-256 hex digest of the manifest JSON
     *   - `signature`       — base64-encoded signature bytes
     *   - `signer_id`       — key / identity string from ArtifactIntegrity
     *
     * @return true when the signature is valid for the given hash + key
     */
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

    // ── Configuration ─────────────────────────────────────────────────────

    /**
     * @brief Register a runtime signature-verification callback.
     *
     * If no verifier is set, `verifyPackageIntegrity()` and
     * `verifyProductIntegrity()` will skip the signature check and only
     * validate the manifest hash.
     *
     * @param verifier Callable that validates an Ed25519/ECDSA signature.
     */
    void setSignatureVerifier(SignatureVerifier verifier);

    // ── LoRAPackage CRUD ──────────────────────────────────────────────────

    /**
     * @brief Store or update a LoRAPackage in the registry.
     *
     * If a package with the same `package_id` already exists it is replaced.
     *
     * @param pkg  Package to store.
     * @return true on success; false if the package_id is empty
     */
    bool storePackage(const LoRAPackage& pkg);

    /**
     * @brief Retrieve a LoRAPackage by ID.
     *
     * @param package_id  Unique package identifier.
     * @return Package if found, std::nullopt otherwise.
     */
    [[nodiscard]] std::optional<LoRAPackage> loadPackage(
        const std::string& package_id) const;

    /**
     * @brief Remove a LoRAPackage from the registry.
     *
     * @param package_id  Unique package identifier.
     * @return true if the package existed and was removed; false otherwise.
     */
    bool deletePackage(const std::string& package_id);

    /**
     * @brief Return all stored LoRAPackage identifiers.
     */
    [[nodiscard]] std::vector<std::string> listPackageIds() const;

    /**
     * @brief Return all packages whose status matches `status`.
     *
     * @param status  Filter status.
     * @return Matching packages (copy).
     */
    [[nodiscard]] std::vector<LoRAPackage> listPackagesByStatus(
        LoRAPackageStatus status) const;

    // ── PortableAdapterProduct CRUD ───────────────────────────────────────

    /**
     * @brief Store or update a PortableAdapterProduct in the registry.
     *
     * @param product  Product to store.
     * @return true on success; false if product_id is empty or source_package_id
     *         is missing.
     */
    bool storeProduct(const PortableAdapterProduct& product);

    /**
     * @brief Retrieve a PortableAdapterProduct by ID.
     *
     * @param product_id  Unique product identifier.
     * @return Product if found, std::nullopt otherwise.
     */
    [[nodiscard]] std::optional<PortableAdapterProduct> loadProduct(
        const std::string& product_id) const;

    /**
     * @brief Remove a PortableAdapterProduct from the registry.
     *
     * @param product_id  Unique product identifier.
     * @return true if the product existed and was removed; false otherwise.
     */
    bool deleteProduct(const std::string& product_id);

    /**
     * @brief Return all stored PortableAdapterProduct identifiers.
     */
    [[nodiscard]] std::vector<std::string> listProductIds() const;

    /**
     * @brief Return all products derived from a given LoRAPackage.
     *
     * @param package_id  Source package identifier.
     * @return Products whose `source_package_id` matches (copy).
     */
    [[nodiscard]] std::vector<PortableAdapterProduct> listProductsByPackage(
        const std::string& package_id) const;

    /**
     * @brief Return all products whose status matches `status`.
     *
     * @param status  Filter status.
     */
    [[nodiscard]] std::vector<PortableAdapterProduct> listProductsByStatus(
        AdapterProductStatus status) const;

    // ── Integrity verification ────────────────────────────────────────────

    /**
     * @brief Verify the manifest hash (and optionally the signature) of a package.
     *
     * Recomputes `pkg.to_json().dump()` → SHA-256 and compares against
     * `pkg.integrity.manifest_hash`.  If a `SignatureVerifier` has been set and
     * the integrity record contains a non-empty signature, the signature is also
     * validated.
     *
     * @param package_id  ID of the package to verify.
     * @return true when all enabled checks pass; false otherwise.
     */
    [[nodiscard]] bool verifyPackageIntegrity(const std::string& package_id) const;

    /**
     * @brief Verify the manifest hash (and optionally the signature) of a product.
     *
     * Same semantics as `verifyPackageIntegrity()` but for products.
     *
     * @param product_id  ID of the product to verify.
     * @return true when all enabled checks pass; false otherwise.
     */
    [[nodiscard]] bool verifyProductIntegrity(const std::string& product_id) const;

    // ── Bulk export / import ──────────────────────────────────────────────

    /**
     * @brief Export all packages to a JSON array.
     *
     * Each element is the result of `LoRAPackage::to_json()`.
     *
     * @return json array
     */
    [[nodiscard]] json exportPackages() const;

    /**
     * @brief Import packages from a JSON array.
     *
     * Existing packages with the same ID are overwritten.
     *
     * @param j  json array produced by `exportPackages()`
     * @return Number of packages successfully imported.
     */
    size_t importPackages(const json& j);

    /**
     * @brief Export all products to a JSON array.
     */
    [[nodiscard]] json exportProducts() const;

    /**
     * @brief Import products from a JSON array.
     *
     * @param j  json array produced by `exportProducts()`
     * @return Number of products successfully imported.
     */
    size_t importProducts(const json& j);

    // ── Statistics ────────────────────────────────────────────────────────

    /**
     * @brief Return the total number of stored packages.
     */
    [[nodiscard]] size_t packageCount() const;

    /**
     * @brief Return the total number of stored products.
     */
    [[nodiscard]] size_t productCount() const;

private:
    mutable std::mutex mutex_;

    std::unordered_map<std::string, LoRAPackage>           packages_;
    std::unordered_map<std::string, PortableAdapterProduct> products_;

    SignatureVerifier signature_verifier_;
};

} // namespace retrieval
} // namespace themis
