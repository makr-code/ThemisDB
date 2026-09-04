// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

/**
 * @file lora_artifact_distribution.h
 * @brief Phase 5: Adapter-Distribution & Sharding-Kopplung
 *
 * Interfaces and value types for distributing LoRA/Adapter artifacts
 * across ThemisDB shards with RAID/Merkle proof integrity, distribution
 * receipts, shard snapshots, and end-to-end recovery support.
 *
 * Issue: #5418 — phase5-adapter-distribution-sharding-2026
 * Dependencies: sub:epic-adalora-package-2026, phase4-hashchain-provenance-2026
 */

#include <chrono>
#include <cstdint>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>

namespace themis::sharding {

// ─────────────────────────────────────────────────────────────────────────────
// Primitive aliases
// ─────────────────────────────────────────────────────────────────────────────

/// Opaque identifier for an artifact distribution event.
using DistributionEventId = std::string;

/// Opaque identifier for a shard-distribution snapshot.
using DistributionSnapshotId = std::string;

/// Shard identifier for routing context.
using DistributionShardId = std::string;

// ─────────────────────────────────────────────────────────────────────────────
// Enumerations
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Artifact types that can be distributed across shards.
 *
 * Aligns with EPIC 1.4 LoRAPackage / PortableAdapterProduct model.
 */
enum class AdapterArtifactType : uint8_t {
    LoRAPackage           = 0, ///< Source-oriented rebuildable LoRA package
    PortableAdapterProduct = 1, ///< Model-bound deployable adapter product
    CheckpointBundle       = 2, ///< Training checkpoint with provenance metadata
    Unknown                = 255
};

/**
 * @brief Status of an artifact distribution event.
 */
enum class ArtifactDistributionStatus : uint8_t {
    Pending   = 0, ///< Queued but not yet started
    InTransit = 1, ///< Bytes are actively being transferred
    Confirmed = 2, ///< Target shard acknowledged receipt and integrity
    Failed    = 3, ///< Transfer failed; receipt records error
    Recovered = 4  ///< Failed distribution recovered via retry or alternate path
};

// ─────────────────────────────────────────────────────────────────────────────
// Value types
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Lightweight reference to a LoRA/Adapter artifact.
 *
 * Carries only enough information for routing and integrity checking;
 * does not include the artifact bytes themselves.
 */
struct LoRAPackageRef {
    /// Stable adapter identifier (must not be empty).
    std::string adapter_id;

    /// Semantic version string (e.g., "1.2.0").
    std::string version;

    /// Artifact kind.
    AdapterArtifactType artifact_type{AdapterArtifactType::LoRAPackage};

    /// SHA-256 hex digest of the artifact bytes (64 hex chars, lowercase).
    std::string content_hash;

    /// Base model this adapter was trained against.
    std::string base_model;

    /// Approximate artifact size in bytes (used for routing cost estimation).
    uint64_t size_bytes{0};

    /// @return true if all required fields are populated.
    [[nodiscard]] bool isValid() const noexcept {
        return !adapter_id.empty() && !version.empty() && content_hash.size() == 64;
    }

    [[nodiscard]] nlohmann::json toJson() const {
        return {
            {"adapter_id",    adapter_id},
            {"version",       version},
            {"artifact_type", static_cast<int>(artifact_type)},
            {"content_hash",  content_hash},
            {"base_model",    base_model},
            {"size_bytes",    size_bytes}
        };
    }
};

/**
 * @brief Immutable, revision-secure receipt of a single distribution event.
 *
 * A receipt is created by the source and counter-signed by the target shard
 * upon confirmed receipt. It carries the Merkle root of the artifact batch
 * it belongs to, enabling audit trail reconstruction.
 *
 * Chaining: @c previous_receipt_hash forms a hash-linked receipt chain
 * anchored to the initial distribution. Verifying the chain gives a
 * tamper-evident audit trail (compatible with Phase 4 hash-chain provenance).
 */
struct AdapterDistributionReceipt {
    /// Unique identifier for this distribution event.
    DistributionEventId event_id;

    /// Reference to the distributed artifact.
    LoRAPackageRef artifact_ref;

    /// Source shard (or "external" for cross-instance push).
    DistributionShardId source_shard_id;

    /// Target shard that received the artifact.
    DistributionShardId target_shard_id;

    /// When the distribution was initiated (UTC, system clock).
    std::chrono::system_clock::time_point initiated_at;

    /// When the target confirmed receipt (nullopt until confirmation).
    std::optional<std::chrono::system_clock::time_point> confirmed_at;

    /// Final outcome of this distribution event.
    ArtifactDistributionStatus status{ArtifactDistributionStatus::Pending};

    /// SHA-256 hash of the previous receipt in the chain (empty for first event).
    std::string previous_receipt_hash;

    /// SHA-256 hash of this receipt (computed over all fields except itself).
    std::string receipt_hash;

    /// Merkle root of the artifact batch this event belongs to.
    std::string batch_merkle_root;

    /// Human-readable error message if status == Failed.
    std::string error_message;

    /// Target-shard acknowledgment signature (empty until confirmed).
    std::string target_signature;

    /// @return true if the receipt has been confirmed by the target shard.
    [[nodiscard]] bool isConfirmed() const noexcept {
        return status == ArtifactDistributionStatus::Confirmed && confirmed_at.has_value();
    }

    [[nodiscard]] nlohmann::json toJson() const {
        auto initiated_ts = std::chrono::system_clock::to_time_t(initiated_at);
        nlohmann::json j = {
            {"event_id",              event_id},
            {"artifact_ref",          artifact_ref.toJson()},
            {"source_shard_id",       source_shard_id},
            {"target_shard_id",       target_shard_id},
            {"initiated_at",          initiated_ts},
            {"status",                static_cast<int>(status)},
            {"previous_receipt_hash", previous_receipt_hash},
            {"receipt_hash",          receipt_hash},
            {"batch_merkle_root",     batch_merkle_root},
            {"error_message",         error_message},
            {"target_signature",      target_signature}
        };
        if (confirmed_at.has_value()) {
            j["confirmed_at"] = std::chrono::system_clock::to_time_t(*confirmed_at);
        }
        return j;
    }
};

/**
 * @brief Point-in-time snapshot of all distribution receipts for a shard.
 *
 * Used for shard recovery ordering: given a snapshot, the recovery process
 * can replay only the receipts issued after snapshot_receipt_count, avoiding
 * duplicate re-application.
 */
struct ShardDistributionSnapshot {
    /// Unique identifier for this snapshot.
    DistributionSnapshotId snapshot_id;

    /// The shard this snapshot covers.
    DistributionShardId shard_id;

    /// Wall-clock time when the snapshot was taken.
    std::chrono::system_clock::time_point captured_at;

    /// Number of confirmed receipts at snapshot time (monotonically increasing).
    uint64_t confirmed_receipt_count{0};

    /// Merkle root computed over all confirmed receipt hashes at this point.
    std::string receipts_merkle_root;

    /// SHA-256 hash of this snapshot record for tamper detection.
    std::string snapshot_hash;

    /// Ordered list of receipt event IDs included in this snapshot.
    std::vector<DistributionEventId> included_event_ids;

    [[nodiscard]] nlohmann::json toJson() const {
        return {
            {"snapshot_id",              snapshot_id},
            {"shard_id",                 shard_id},
            {"captured_at",              std::chrono::system_clock::to_time_t(captured_at)},
            {"confirmed_receipt_count",  confirmed_receipt_count},
            {"receipts_merkle_root",     receipts_merkle_root},
            {"snapshot_hash",            snapshot_hash},
            {"included_event_ids",       included_event_ids}
        };
    }
};

/**
 * @brief RAID/Merkle proof of artifact integrity in a batch tree.
 *
 * Supports O(log N) membership proof: given @c leaf_hash and the
 * @c proof_path sibling hashes, a verifier can recompute the Merkle root
 * and compare with the authoritative @c merkle_root.
 *
 * Proof path format: each element is a pair
 *   { "hash": "<64-hex>", "position": "left"|"right" }
 * indicating whether the sibling is to the left or right of the current node.
 *
 * Leaf identity is (adapter_id, version) — both fields are required to
 * unambiguously locate a leaf when multiple versions of the same adapter
 * appear in one batch.
 */
struct ArtifactMerkleProof {
    /// Merkle root of the batch this proof belongs to.
    std::string merkle_root;

    /// SHA-256 hash of the artifact leaf node.
    std::string leaf_hash;

    /// Adapter identifier this proof covers.
    std::string adapter_id = {};

    /// Version of the adapter this proof covers (together with adapter_id
    /// forms the unique leaf key).
    std::string version;

    /// Ordered proof path from leaf to root (exclusive of root).
    /// Each entry: { "hash": "<hex>", "position": "left"|"right" }
    std::vector<nlohmann::json> proof_path;

    /// Total leaf count in the batch (for audit purposes).
    uint64_t batch_size{0};

    /// @return true if all required proof fields are populated.
    [[nodiscard]] bool isValid() const noexcept {
        return merkle_root.size() == 64
            && leaf_hash.size() == 64
            && !adapter_id.empty()
            && !version.empty();
    }

    [[nodiscard]] nlohmann::json toJson() const {
        return {
            {"merkle_root", merkle_root},
            {"leaf_hash",   leaf_hash},
            {"adapter_id",  adapter_id},
            {"version",     version},
            {"proof_path",  proof_path},
            {"batch_size",  batch_size}
        };
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Interface: IAdapterDistributionStore
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Persists and retrieves adapter distribution receipts and snapshots.
 *
 * Implementations may back this with RocksDB, in-memory storage, or a
 * remote metadata shard.  All write operations are idempotent with respect
 * to @c event_id / @c snapshot_id (re-inserting the same ID is a no-op).
 *
 * Thread-safety: all methods must be safe to call concurrently.
 *
 * Performance contract: storeReceipt() and getReceipt() ≤ 5 ms under no I/O
 * contention.  listReceiptsForShard() ≤ 20 ms for up to 10 000 receipts.
 */
class IAdapterDistributionStore {
public:
    virtual ~IAdapterDistributionStore() = default;

    /**
     * @brief Persist a distribution receipt.
     *
     * @param receipt  Fully populated receipt (receipt_hash must be set).
     * @return true on success; false if the event_id already exists (idempotent).
     * @throws std::invalid_argument if receipt.event_id is empty.
     */
    [[nodiscard]] virtual bool storeReceipt(const AdapterDistributionReceipt& receipt) = 0;

    /**
     * @brief Retrieve a receipt by its event identifier.
     *
     * @param event_id  The distribution event identifier.
     * @return The receipt, or nullopt if not found.
     */
    [[nodiscard]] virtual std::optional<AdapterDistributionReceipt> getReceipt(
        const DistributionEventId& event_id) const = 0;

    /**
     * @brief Update the status and confirmation fields of an existing receipt.
     *
     * @param event_id         Target event.
     * @param new_status       The new distribution status.
     * @param target_signature Optional signature provided by the target shard.
     * @param error_message    Human-readable error message (persisted when
     *                         @p new_status is Failed; ignored otherwise).
     * @return true if the receipt was found and updated; false otherwise.
     */
    [[nodiscard]] virtual bool updateReceiptStatus(
        const DistributionEventId& event_id,
        ArtifactDistributionStatus new_status,
        const std::string& target_signature = {},
        const std::string& error_message = {}) = 0;

    /**
     * @brief List all receipts for a given target shard, optionally filtered by status.
     *
     * @param shard_id  Target shard identifier.
     * @param status    If provided, return only receipts with this status.
     * @return Ordered list of matching receipts (oldest first).
     */
    [[nodiscard]] virtual std::vector<AdapterDistributionReceipt> listReceiptsForShard(
        const DistributionShardId& shard_id,
        std::optional<ArtifactDistributionStatus> status = std::nullopt) const = 0;

    /**
     * @brief Persist a shard distribution snapshot.
     *
     * @param snapshot  Fully populated snapshot (snapshot_hash must be set).
     * @return true on success; false if snapshot_id already exists.
     * @throws std::invalid_argument if snapshot.snapshot_id is empty.
     */
    [[nodiscard]] virtual bool storeSnapshot(const ShardDistributionSnapshot& snapshot) = 0;

    /**
     * @brief Retrieve the most recent snapshot for a shard.
     *
     * @param shard_id  Target shard identifier.
     * @return The latest snapshot, or nullopt if none exists.
     */
    [[nodiscard]] virtual std::optional<ShardDistributionSnapshot> getLatestSnapshot(
        const DistributionShardId& shard_id) const = 0;

    /**
     * @brief Count confirmed receipts for a shard since a given snapshot.
     *
     * Used during recovery to determine which receipts need replay.
     *
     * @param shard_id    Target shard.
     * @param after_count Start counting from receipts with index > after_count.
     * @return Number of confirmed receipts after the given count.
     */
    [[nodiscard]] virtual uint64_t countReceiptsSinceSnapshot(
        const DistributionShardId& shard_id,
        uint64_t after_count) const = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Interface: IArtifactMerkleProofEngine
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Constructs Merkle trees over artifact batches and produces/verifies proofs.
 *
 * The implementation uses SHA-256 and canonical JSON serialisation for
 * leaf hashing (compatible with the blockchain_integrity module).
 *
 * Leaf ordering is deterministic: artifacts are sorted by adapter_id then
 * version before tree construction, ensuring reproducible roots.
 *
 * Performance contract: buildRoot() for 1 000 artifacts ≤ 50 ms.
 *   verify() is O(log N) where N is batch_size.
 */
class IArtifactMerkleProofEngine {
public:
    virtual ~IArtifactMerkleProofEngine() = default;

    /**
     * @brief Compute the Merkle root for a batch of artifact references.
     *
     * Artifacts are sorted by (adapter_id, version) before hashing to ensure
     * a deterministic root regardless of insertion order.
     *
     * @param artifacts  Non-empty batch of artifact references.
     * @return 64-char lowercase hex SHA-256 Merkle root.
     * @throws std::invalid_argument if @p artifacts is empty.
     */
    [[nodiscard]] virtual std::string buildRoot(
        const std::vector<LoRAPackageRef>& artifacts) const = 0;

    /**
     * @brief Generate a membership proof for a specific artifact within a batch.
     *
     * Leaves are identified by the composite key (adapter_id, version) to
     * unambiguously locate the correct leaf when multiple versions of the same
     * adapter appear in a batch.
     *
     * @param artifacts   The complete batch used to build the tree.
     * @param adapter_id  Adapter identifier of the artifact to prove.
     * @param version     Version string of the artifact to prove.
     * @return Populated proof struct, or nullopt if (adapter_id, version) is
     *         not in the batch.
     */
    [[nodiscard]] virtual std::optional<ArtifactMerkleProof> generateProof(
        const std::vector<LoRAPackageRef>& artifacts,
        const std::string& adapter_id,
        const std::string& version) const = 0;

    /**
     * @brief Verify that a proof correctly links leaf_hash to merkle_root.
     *
     * @param proof  The proof to verify (must satisfy proof.isValid()).
     * @return true if the proof path reconstructs the declared Merkle root.
     */
    [[nodiscard]] virtual bool verifyProof(const ArtifactMerkleProof& proof) const = 0;

    /**
     * @brief Compute the SHA-256 leaf hash for a single artifact reference.
     *
     * Uses canonical JSON serialisation (sorted keys) matching the
     * blockchain_integrity module.
     *
     * @param artifact  The artifact reference to hash.
     * @return 64-char lowercase hex hash.
     */
    [[nodiscard]] virtual std::string leafHash(const LoRAPackageRef& artifact) const = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Interface: ILoRADistributionManager
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Orchestrates distribution of LoRA/Adapter artifacts to ThemisDB shards.
 *
 * Responsibilities:
 * - Accept push requests for LoRA packages and portable adapter products.
 * - Issue and persist AdapterDistributionReceipts for every distribution event.
 * - Trigger Merkle proof generation for each distribution batch.
 * - Support cross-instance push (external endpoint) for federation scenarios.
 * - Expose snapshot and recovery ordering for disaster-recovery workflows.
 *
 * Thread-safety: all methods must be safe to call concurrently.
 *
 * Performance contracts:
 *   distributeArtifact() scheduling latency (before network I/O): ≤ 10 ms.
 *   confirmReceipt() ≤ 5 ms.
 *   takeDistributionSnapshot() ≤ 20 ms for up to 10 000 receipts.
 */
class ILoRADistributionManager {
public:
    virtual ~ILoRADistributionManager() = default;

    /**
     * @brief Initiate distribution of an artifact to one target shard.
     *
     * Creates a Pending receipt, schedules the transfer, and returns the
     * event identifier.  Callers should poll getDistributionStatus() or
     * register a callback via onDistributionComplete().
     *
     * @param artifact        Reference to the artifact to distribute.
     * @param source_shard_id Originating shard (or "external").
     * @param target_shard_id Destination shard.
     * @param batch_merkle_root Merkle root of the batch this event belongs to.
     *                          Pass an empty string to compute a single-artifact root.
     * @return The DistributionEventId for tracking this event.
     * @throws std::invalid_argument if artifact.isValid() is false.
     */
    [[nodiscard]] virtual DistributionEventId distributeArtifact(
        const LoRAPackageRef& artifact,
        const DistributionShardId& source_shard_id,
        const DistributionShardId& target_shard_id,
        const std::string& batch_merkle_root = {}) = 0;

    /**
     * @brief Distribute an artifact to all registered shards in a single batch.
     *
     * A single Merkle tree is computed for all target shards' receipts in this
     * batch, ensuring they share a common batch_merkle_root.
     *
     * @param artifact         Reference to the artifact.
     * @param source_shard_id  Originating shard.
     * @param target_shard_ids Non-empty list of destination shards.
     * @return Map of target_shard_id → DistributionEventId.
     * @throws std::invalid_argument if target_shard_ids is empty or artifact invalid.
     */
    [[nodiscard]] virtual std::unordered_map<DistributionShardId, DistributionEventId>
    distributeToAllShards(
        const LoRAPackageRef& artifact,
        const DistributionShardId& source_shard_id,
        const std::vector<DistributionShardId>& target_shard_ids) = 0;

    /**
     * @brief Confirm receipt of a previously distributed artifact.
     *
     * Transitions the receipt from InTransit → Confirmed and records the
     * confirmation timestamp and optional target signature.
     *
     * @param event_id         The distribution event to confirm.
     * @param target_signature Optional acknowledgment signature from the target.
     * @return true if the event was found and successfully confirmed.
     */
    [[nodiscard]] virtual bool confirmReceipt(
        const DistributionEventId& event_id,
        const std::string& target_signature = {}) = 0;

    /**
     * @brief Mark a distribution event as failed with an error message.
     *
     * @param event_id     The distribution event that failed.
     * @param error_message Human-readable description of the failure.
     * @return true if the event was found and marked as failed.
     */
    [[nodiscard]] virtual bool markFailed(
        const DistributionEventId& event_id,
        const std::string& error_message) = 0;

    /**
     * @brief Recover a previously failed distribution by retrying.
     *
     * Creates a new distribution event for the same artifact and target,
     * linked to the original event via the receipt chain.
     *
     * @param failed_event_id  The event to recover from.
     * @return New DistributionEventId for the recovery attempt, or nullopt if
     *         the original event is not in Failed state.
     */
    [[nodiscard]] virtual std::optional<DistributionEventId> recoverDistribution(
        const DistributionEventId& failed_event_id) = 0;

    /**
     * @brief Retrieve the current status of a distribution event.
     *
     * @param event_id  Target event identifier.
     * @return The receipt, or nullopt if not found.
     */
    [[nodiscard]] virtual std::optional<AdapterDistributionReceipt> getDistributionStatus(
        const DistributionEventId& event_id) const = 0;

    /**
     * @brief Generate a Merkle proof for an artifact within a named batch.
     *
     * The batch is identified by the @c batch_merkle_root stored on each receipt.
     * Only confirmed receipts sharing that root are treated as the batch leaves.
     *
     * @param batch_merkle_root Merkle root identifying the batch.
     * @param adapter_id        Adapter identifier of the artifact to prove.
     * @param version           Version of the artifact to prove (together with
     *                          adapter_id forms the unique leaf key).
     * @return Proof struct, or nullopt if the artifact is not in the batch.
     */
    [[nodiscard]] virtual std::optional<ArtifactMerkleProof> generateBatchProof(
        const std::string& batch_merkle_root,
        const std::string& adapter_id,
        const std::string& version) const = 0;

    /**
     * @brief Capture a point-in-time snapshot of distribution state for a shard.
     *
     * The snapshot records all confirmed receipts at the time of the call,
     * computes their combined Merkle root, and persists the result.
     *
     * @param shard_id  Shard to snapshot.
     * @return The newly created snapshot.
     */
    [[nodiscard]] virtual ShardDistributionSnapshot takeDistributionSnapshot(
        const DistributionShardId& shard_id) = 0;

    /**
     * @brief Determine which receipts must be replayed to recover a shard.
     *
     * Compares the latest snapshot with the current confirmed receipts and
     * returns the ordered list of event IDs not yet covered by the snapshot.
     *
     * @param shard_id  Shard to recover.
     * @return Ordered list of DistributionEventIds to replay (oldest first).
     *         Empty if no recovery is needed.
     */
    [[nodiscard]] virtual std::vector<DistributionEventId> getRecoveryOrder(
        const DistributionShardId& shard_id) const = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Factory functions
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Create an in-memory distribution store suitable for tests and single-node use.
 */
[[nodiscard]] std::shared_ptr<IAdapterDistributionStore> makeInMemoryDistributionStore();

/**
 * @brief Create the default Merkle proof engine.
 */
[[nodiscard]] std::shared_ptr<IArtifactMerkleProofEngine> makeDefaultMerkleProofEngine();

/**
 * @brief Create a distribution manager backed by the given store and proof engine.
 *
 * @param store         Receipt/snapshot backend (nullptr → fresh in-memory store).
 * @param proof_engine  Merkle engine (nullptr → DefaultMerkleProofEngine).
 */
[[nodiscard]] std::shared_ptr<ILoRADistributionManager> makeLoRADistributionManager(
    std::shared_ptr<IAdapterDistributionStore> store         = nullptr,
    std::shared_ptr<IArtifactMerkleProofEngine> proof_engine = nullptr);

} // namespace themis::sharding
