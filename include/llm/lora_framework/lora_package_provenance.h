// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

/**
 * @file lora_package_provenance.h
 * @brief Phase 4 HashChain & Provenance Layer for LoRAPackage and AdapterProduct.
 *
 * Implements the unambiguous, auditable provenance chain required by issue #5417:
 *   - Hash-based ProvenanceHashLedger with parent-hash linkage
 *   - LoRAPackage and AdapterProduct artifact models
 *   - DistributionReceipt and ReceiptChain (tamper-evident receipt chain)
 *   - ReceiptManifest per distribution event (with Merkle root)
 *   - ShardLedgerEntry for RAID-Merkle-Proof integration
 *
 * Design principles:
 *   - All hash chains use SHA-256.
 *   - Every artifact carries its own content hash and a parent_hash that
 *     links it to the previous version in the chain (empty string = genesis).
 *   - Tamper detection is achieved by re-computing hashes and comparing
 *     against stored values; the entire chain is invalid if any link breaks.
 *   - Thread-safe: all ProvenanceHashLedger methods are guarded by a mutex.
 */

#include <nlohmann/json.hpp>

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace llm {
namespace lora {

using json = nlohmann::json;

// ============================================================================
// LoRAPackage — source-oriented, rebuildable adapter artifact
// ============================================================================

/**
 * @brief Source-oriented artifact package for a LoRA adapter.
 *
 * A LoRAPackage is the rebuildable, source-linked representation of an
 * adapter artifact.  It carries the adapter weights hash, the format
 * descriptor, and a parent_hash that links it to the previous version so
 * that the full version history forms a tamper-evident chain.
 *
 * @note package_hash covers all content fields (excluding package_hash
 *       itself) and is computed by LoRAPackage::computeContentHash().
 */
struct LoRAPackage {
    std::string package_id;        ///< Unique package identifier (UUID-like hex)
    std::string adapter_id;        ///< Reference to the originating LoRA adapter
    std::string version;           ///< Semantic version string of this package
    std::string format;            ///< Serialisation format: "safetensors" | "gguf" | "onnx"
    std::string weights_hash;      ///< SHA-256 of the packaged adapter weights
    std::string package_hash;      ///< SHA-256 over all content fields (set by ledger)
    std::string parent_hash;       ///< SHA-256 of the previous package version; empty = genesis
    std::string created_at;        ///< ISO 8601 UTC creation timestamp
    json        metadata;          ///< Application-specific extensions (JSON object)

    /**
     * @brief Serialise to JSON.
     * @return JSON representation of this package.
     */
    [[nodiscard]] json toJSON() const;

    /**
     * @brief Deserialise from JSON.
     * @param j Source JSON object.
     * @return Populated LoRAPackage.
     */
    [[nodiscard]] static LoRAPackage fromJSON(const json& j);

    /**
     * @brief Compute the canonical SHA-256 hash of all content fields.
     *
     * Covers: adapter_id, version, format, weights_hash, parent_hash, created_at,
     * and the serialised metadata.  Does NOT cover package_hash itself to
     * avoid a circular dependency.
     *
     * @return 64-character lowercase hex string.
     */
    [[nodiscard]] std::string computeContentHash() const;
};

// ============================================================================
// AdapterProduct — model-bound deployable adapter product
// ============================================================================

/**
 * @brief Model-bound deployable product derived from a LoRAPackage.
 *
 * An AdapterProduct is the deployable form of a LoRA adapter, bound to a
 * specific base model.  It records the compatibility commitment (expressed
 * as a hash of the compatibility matrix) and links back to the originating
 * LoRAPackage via package_id.
 *
 * @note product_hash covers all content fields (excluding product_hash itself)
 *       and is computed by AdapterProduct::computeContentHash().
 */
struct AdapterProduct {
    std::string product_id;            ///< Unique product identifier (UUID-like hex)
    std::string package_id;            ///< Reference to the originating LoRAPackage
    std::string base_model_id;         ///< Base model this product is bound to
    std::string base_model_hash;       ///< SHA-256 of base model weights
    std::string product_hash;          ///< SHA-256 over all content fields (set by ledger)
    std::string parent_hash;           ///< SHA-256 of the previous product version; empty = genesis
    std::string compatibility_hash;    ///< SHA-256 of the compatibility matrix snapshot
    std::string created_at;            ///< ISO 8601 UTC creation timestamp
    json        deployment_metadata;   ///< Deployment-specific extensions (JSON object)

    /**
     * @brief Serialise to JSON.
     * @return JSON representation.
     */
    [[nodiscard]] json toJSON() const;

    /**
     * @brief Deserialise from JSON.
     * @param j Source JSON object.
     * @return Populated AdapterProduct.
     */
    [[nodiscard]] static AdapterProduct fromJSON(const json& j);

    /**
     * @brief Compute the canonical SHA-256 hash of all content fields.
     *
     * Covers: package_id, base_model_id, base_model_hash, parent_hash,
     * compatibility_hash, created_at, and the serialised deployment_metadata.
     *
     * @return 64-character lowercase hex string.
     */
    [[nodiscard]] std::string computeContentHash() const;
};

// ============================================================================
// DistributionReceipt — per-event receipt
// ============================================================================

/**
 * @brief Immutable receipt for a single distribution event.
 *
 * Each time an artifact (LoRAPackage or AdapterProduct) is distributed,
 * exported, or placed on a shard, a DistributionReceipt is generated.
 * Receipts are linked in a tamper-evident chain via parent_receipt_hash.
 *
 * Supported event types: "deploy", "export", "shard_distribute",
 * "federated_sync", "checkpoint_transfer".
 *
 * @note receipt_hash covers all content fields and links (excluding
 *       receipt_hash itself).
 */
struct DistributionReceipt {
    std::string receipt_id;             ///< Unique receipt identifier
    std::string event_type;             ///< Kind of distribution event
    std::string artifact_id;            ///< package_id or product_id of the artifact
    std::string artifact_hash;          ///< Hash of the artifact at distribution time
    std::string recipient_id;           ///< Target node, endpoint, or shard identifier
    std::string distribution_timestamp; ///< ISO 8601 UTC timestamp
    std::string operator_signature;     ///< Optional operator signature over the receipt fields
    std::string parent_receipt_hash;    ///< SHA-256 of previous receipt; for the first receipt in a chain this is sha256(artifact_id) (the chain's genesis seed)
    std::string receipt_hash;           ///< SHA-256 of this receipt's canonical form (set by ledger)
    json        extra_metadata;         ///< Application-specific extensions

    /**
     * @brief Serialise to JSON.
     * @return JSON representation.
     */
    [[nodiscard]] json toJSON() const;

    /**
     * @brief Deserialise from JSON.
     * @param j Source JSON object.
     * @return Populated DistributionReceipt.
     */
    [[nodiscard]] static DistributionReceipt fromJSON(const json& j);

    /**
     * @brief Compute the canonical SHA-256 hash of all content fields.
     *
     * Covers: event_type, artifact_id, artifact_hash, recipient_id,
     * distribution_timestamp, operator_signature, parent_receipt_hash,
     * and the serialised extra_metadata.
     *
     * @return 64-character lowercase hex string.
     */
    [[nodiscard]] std::string computeContentHash() const;
};

// ============================================================================
// ReceiptManifest — bundle of receipts for one distribution event
// ============================================================================

/**
 * @brief Manifest bundling all receipts for a single distribution event.
 *
 * A ReceiptManifest captures the full picture of one distribution round:
 * which artifact was distributed, to which recipients, and the Merkle root
 * that summarises all per-recipient receipts for cryptographic integrity.
 *
 * The manifest_hash covers manifest_id, event_type, artifact_id, created_at,
 * all receipt_hashes in stable order, and the merkle_root.
 */
struct ReceiptManifest {
    std::string manifest_id;        ///< Unique manifest identifier
    std::string event_type;         ///< Distribution event type (same as receipts)
    std::string artifact_id;        ///< Artifact covered by this manifest
    std::string created_at;         ///< ISO 8601 UTC creation timestamp
    std::vector<DistributionReceipt> receipts; ///< All receipts for this event
    std::string merkle_root;        ///< Merkle root of all receipt_hashes
    std::string manifest_hash;      ///< SHA-256 of canonical manifest content

    /**
     * @brief Serialise to JSON.
     * @return JSON representation.
     */
    [[nodiscard]] json toJSON() const;

    /**
     * @brief Deserialise from JSON.
     * @param j Source JSON object.
     * @return Populated ReceiptManifest.
     */
    [[nodiscard]] static ReceiptManifest fromJSON(const json& j);

    /**
     * @brief Compute the Merkle root of all receipt hashes in `receipts`.
     *
     * Uses a standard binary Merkle tree with SHA-256 internal nodes.
     * An empty receipts list returns the SHA-256 of the empty string.
     *
     * @return 64-character lowercase hex Merkle root.
     */
    [[nodiscard]] std::string computeMerkleRoot() const;

    /**
     * @brief Compute the canonical SHA-256 hash of all manifest fields.
     *
     * Covers: manifest_id, event_type, artifact_id, created_at,
     * all receipt_hashes in order, and merkle_root.
     *
     * @return 64-character lowercase hex string.
     */
    [[nodiscard]] std::string computeContentHash() const;
};

// ============================================================================
// ShardLedgerEntry — per-shard RAID-Merkle-Proof record
// ============================================================================

/**
 * @brief Shard-level integrity record for RAID-Merkle-Proof integration.
 *
 * When an artifact is placed on a shard, a ShardLedgerEntry records the
 * shard assignment alongside the Merkle proof required to verify that the
 * shard fragment is part of the original artifact.
 *
 * Shard ledger entries are chained via prev_entry_hash so the full history
 * of shard placements for an artifact is tamper-evident.
 */
struct ShardLedgerEntry {
    std::string entry_id;           ///< Unique ledger entry identifier
    std::string artifact_id;        ///< package_id or product_id
    std::string shard_id;           ///< Target shard node / partition identifier
    uint32_t    shard_index  = 0;   ///< Ordinal of this shard in the distribution (0-based)
    uint32_t    total_shards = 0;   ///< Total number of shards in this distribution
    std::string shard_hash;         ///< SHA-256 of this shard's content bytes
    std::vector<std::string> merkle_proof; ///< Sibling hashes for Merkle inclusion proof
    std::string placement_timestamp;       ///< ISO 8601 UTC placement timestamp
    std::string prev_entry_hash;    ///< SHA-256 of previous shard entry; for the first entry in a ledger this is sha256(artifact_id+"_shard_genesis") (deterministic genesis seed)
    std::string entry_hash;         ///< SHA-256 of this entry's canonical form (set by ledger)

    /**
     * @brief Serialise to JSON.
     * @return JSON representation.
     */
    [[nodiscard]] json toJSON() const;

    /**
     * @brief Deserialise from JSON.
     * @param j Source JSON object.
     * @return Populated ShardLedgerEntry.
     */
    [[nodiscard]] static ShardLedgerEntry fromJSON(const json& j);

    /**
     * @brief Compute the canonical SHA-256 hash of all content fields.
     *
     * Covers: artifact_id, shard_id, shard_index, total_shards, shard_hash,
     * merkle_proof entries in order, placement_timestamp, and prev_entry_hash.
     *
     * @return 64-character lowercase hex string.
     */
    [[nodiscard]] std::string computeContentHash() const;
};

// ============================================================================
// ReceiptChain — tamper-evident chain of DistributionReceipts
// ============================================================================

/**
 * @brief Tamper-evident chain of DistributionReceipts for one artifact.
 *
 * A ReceiptChain is the ordered, linked sequence of all DistributionReceipts
 * ever issued for one artifact (identified by artifact_id).  Each receipt in
 * the chain carries the parent_receipt_hash of its predecessor.
 *
 * @note This is a value type.  The ProvenanceHashLedger is the authoritative
 *       store for live receipt chains.
 */
struct ReceiptChain {
    std::string chain_id;          ///< Unique chain identifier (matches artifact_id in most cases)
    std::string artifact_id;       ///< Artifact whose receipts this chain covers
    std::string genesis_hash;      ///< Seed hash used to initialise the chain (SHA-256 of chain_id)
    std::vector<DistributionReceipt> entries; ///< Ordered entries, oldest first
    std::string head_hash;         ///< Current head hash (= receipt_hash of last entry)

    /**
     * @brief Serialise to JSON.
     * @return JSON representation.
     */
    [[nodiscard]] json toJSON() const;

    /**
     * @brief Deserialise from JSON.
     * @param j Source JSON object.
     * @return Populated ReceiptChain.
     */
    [[nodiscard]] static ReceiptChain fromJSON(const json& j);
};

// ============================================================================
// ProvenanceHashLedger
// ============================================================================

/**
 * @brief Hash-based provenance ledger for LoRAPackage and AdapterProduct.
 *
 * The ProvenanceHashLedger is the central authority for all Phase-4 provenance
 * artefacts.  It:
 *   1. Maintains the hash-chained version history of every LoRAPackage and
 *      AdapterProduct registered with it.
 *   2. Builds and stores ReceiptChains (one per artifact) from appended
 *      DistributionReceipts.
 *   3. Creates and validates ReceiptManifests for batch distribution events.
 *   4. Maintains a per-artifact ShardLedger whose entries are chained by
 *      prev_entry_hash for RAID-Merkle-Proof integration.
 *   5. Exports full audit paths as JSON for CLI / REST consumption.
 *
 * Thread safety: all public methods are protected by a single shared mutex.
 * Reads and writes from multiple threads are safe.
 */
class ProvenanceHashLedger {
public:
    ProvenanceHashLedger();
    ~ProvenanceHashLedger();

    ProvenanceHashLedger(const ProvenanceHashLedger&)            = delete;
    ProvenanceHashLedger& operator=(const ProvenanceHashLedger&) = delete;

    // -----------------------------------------------------------------------
    // LoRAPackage lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Register a new LoRAPackage version in the ledger.
     *
     * Sets package_id (if empty), created_at (if empty), parent_hash (to the
     * current head for this adapter_id), and package_hash.  The package is
     * appended to the per-adapter package chain.
     *
     * @param pkg  Package to register.  Fields package_id, parent_hash,
     *             created_at, and package_hash may be empty — they will be
     *             populated by this method.
     * @return Populated LoRAPackage with all hash fields set.
     */
    [[nodiscard]] LoRAPackage appendPackage(LoRAPackage pkg);

    /**
     * @brief Retrieve all LoRAPackage versions for an adapter, oldest first.
     *
     * @param adapter_id  Adapter identifier.
     * @return Ordered vector of packages; empty if none registered.
     */
    [[nodiscard]] std::vector<LoRAPackage> getPackageChain(
        const std::string& adapter_id) const;

    /**
     * @brief Retrieve a specific LoRAPackage by its package_id.
     *
     * @param package_id  Package identifier.
     * @return Package if found; nullopt otherwise.
     */
    [[nodiscard]] std::optional<LoRAPackage> getPackage(
        const std::string& package_id) const;

    // -----------------------------------------------------------------------
    // AdapterProduct lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Register a new AdapterProduct version in the ledger.
     *
     * Sets product_id (if empty), created_at (if empty), parent_hash (to the
     * current head for this package_id), and product_hash.
     *
     * @param product  Product to register.  Fields product_id, parent_hash,
     *                 created_at, and product_hash may be empty.
     * @return Populated AdapterProduct with all hash fields set.
     */
    [[nodiscard]] AdapterProduct appendProduct(AdapterProduct product);

    /**
     * @brief Retrieve all AdapterProduct versions for a package, oldest first.
     *
     * @param package_id  Package identifier.
     * @return Ordered vector of products; empty if none registered.
     */
    [[nodiscard]] std::vector<AdapterProduct> getProductChain(
        const std::string& package_id) const;

    /**
     * @brief Retrieve a specific AdapterProduct by its product_id.
     *
     * @param product_id  Product identifier.
     * @return Product if found; nullopt otherwise.
     */
    [[nodiscard]] std::optional<AdapterProduct> getProduct(
        const std::string& product_id) const;

    // -----------------------------------------------------------------------
    // DistributionReceipt and ReceiptChain
    // -----------------------------------------------------------------------

    /**
     * @brief Append a DistributionReceipt to the artifact's receipt chain.
     *
     * Populates receipt_id (if empty), distribution_timestamp (if empty),
     * parent_receipt_hash (to the current chain head for this artifact_id),
     * and receipt_hash.  Creates the receipt chain if it does not yet exist.
     *
     * @param receipt  Receipt to append.  artifact_id must be non-empty.
     * @return Populated DistributionReceipt with all hash fields set.
     * @throws std::invalid_argument when receipt.artifact_id is empty.
     */
    [[nodiscard]] DistributionReceipt appendReceipt(DistributionReceipt receipt);

    /**
     * @brief Retrieve the full ReceiptChain for an artifact.
     *
     * @param artifact_id  Package or product identifier.
     * @return ReceiptChain; entries is empty if none registered.
     */
    [[nodiscard]] ReceiptChain getReceiptChain(
        const std::string& artifact_id) const;

    /**
     * @brief Verify the integrity of the receipt chain for an artifact.
     *
     * Recomputes each receipt's content hash and verifies that
     * parent_receipt_hash links match.  Returns false at the first broken
     * link.
     *
     * @param artifact_id  Artifact whose chain to verify.
     * @return true when the entire chain is intact; false on any violation.
     */
    [[nodiscard]] bool verifyReceiptChain(const std::string& artifact_id) const;

    // -----------------------------------------------------------------------
    // ReceiptManifest
    // -----------------------------------------------------------------------

    /**
     * @brief Create a ReceiptManifest for a batch distribution event.
     *
     * Appends each receipt in @p receipts to the artifact's chain (via
     * appendReceipt()), computes the Merkle root and manifest hash, and
     * stores the resulting manifest.
     *
     * @param event_type   Distribution event type string.  Must be non-empty.
     * @param artifact_id  Artifact being distributed.  Must be non-empty.
     * @param receipts     Per-recipient receipts (artifact_id should match).
     * @return Populated ReceiptManifest with merkle_root and manifest_hash set.
     * @throws std::invalid_argument when event_type or artifact_id is empty.
     */
    [[nodiscard]] ReceiptManifest createManifest(
        const std::string&              event_type,
        const std::string&              artifact_id,
        std::vector<DistributionReceipt> receipts);

    /**
     * @brief Retrieve a ReceiptManifest by its manifest_id.
     *
     * @param manifest_id  Manifest identifier.
     * @return Manifest if found; nullopt otherwise.
     */
    [[nodiscard]] std::optional<ReceiptManifest> getManifest(
        const std::string& manifest_id) const;

    /**
     * @brief Validate a ReceiptManifest: verify Merkle root and manifest hash.
     *
     * Recomputes the Merkle root from the receipts' receipt_hash values and
     * recomputes the manifest_hash.  Both must match the stored values.
     *
     * @param manifest  Manifest to validate.
     * @return true when both checks pass; false otherwise.
     */
    [[nodiscard]] bool validateManifest(const ReceiptManifest& manifest) const;

    // -----------------------------------------------------------------------
    // ShardLedger (RAID-Merkle-Proof integration)
    // -----------------------------------------------------------------------

    /**
     * @brief Append a ShardLedgerEntry to the artifact's shard ledger.
     *
     * Populates entry_id (if empty), placement_timestamp (if empty),
     * prev_entry_hash (to the current shard-ledger head for this artifact_id),
     * and entry_hash.
     *
     * @param entry  Entry to append.  artifact_id must be non-empty.
     * @return Populated ShardLedgerEntry with all hash fields set.
     * @throws std::invalid_argument when entry.artifact_id is empty.
     */
    [[nodiscard]] ShardLedgerEntry appendShardEntry(ShardLedgerEntry entry);

    /**
     * @brief Retrieve all ShardLedgerEntries for an artifact, oldest first.
     *
     * @param artifact_id  Artifact identifier.
     * @return Ordered vector of entries; empty if none registered.
     */
    [[nodiscard]] std::vector<ShardLedgerEntry> getShardLedger(
        const std::string& artifact_id) const;

    /**
     * @brief Verify the integrity of the shard ledger chain for an artifact.
     *
     * Recomputes each entry's content hash and verifies prev_entry_hash
     * linkage.  Returns false at the first broken link.
     *
     * @param artifact_id  Artifact whose ledger to verify.
     * @return true when the full shard ledger chain is intact.
     */
    [[nodiscard]] bool verifyShardLedger(const std::string& artifact_id) const;

    // -----------------------------------------------------------------------
    // Audit path export
    // -----------------------------------------------------------------------

    /**
     * @brief Export the full audit path for an artifact as JSON.
     *
     * Accepts an @p artifact_id that may be an adapter_id, package_id, or
     * product_id.  The method resolves the identity to its canonical adapter
     * and package context:
     *   - adapter_id  → all packages for that adapter; products for every
     *                   package in the adapter's chain.
     *   - package_id  → the full package chain for the owning adapter; products
     *                   scoped to the given package.
     *   - product_id  → the full package chain for the owning adapter; products
     *                   scoped to the owning package.
     *
     * The receipt chain, manifests, and shard ledger sections are always
     * looked up directly by the supplied @p artifact_id.
     *
     * @param artifact_id  Adapter, package, or product identifier.
     * @return JSON document with keys: "artifact_id", "packages", "products",
     *         "receipt_chain", "manifests", "shard_ledger".
     */
    [[nodiscard]] json exportAuditPath(const std::string& artifact_id) const;

    // -----------------------------------------------------------------------
    // SHA-256 utility
    // -----------------------------------------------------------------------

    /**
     * @brief Compute the SHA-256 hash of a string and return as lowercase hex.
     * @param data  Input data.
     * @return 64-character lowercase hex string.
     */
    [[nodiscard]] static std::string sha256Hex(const std::string& data);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace lora
} // namespace llm
} // namespace themis
