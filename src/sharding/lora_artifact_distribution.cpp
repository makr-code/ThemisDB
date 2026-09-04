// Copyright 2025 ThemisDB
// Licensed under MIT License

/**
 * @file lora_artifact_distribution.cpp
 * @brief Concrete implementations for Phase 5 LoRA artifact distribution.
 *
 * Provides:
 *   - portableSha256Hex()      — OpenSSL EVP SHA-256 hex digest helper
 *   - InMemoryAdapterDistributionStore — thread-safe in-memory receipt/snapshot store
 *   - DefaultMerkleProofEngine — SHA-256 Merkle tree builder and proof verifier
 *   - DefaultLoRADistributionManager — orchestrates distribution, receipts, snapshots
 *
 * Issue: #5418 — phase5-adapter-distribution-sharding-2026
 */

#include "sharding/lora_artifact_distribution.h"

#include <algorithm>
#include <iomanip>
#include <mutex>
#include <optional>
#include <openssl/evp.h>
#include <random>
#include <shared_mutex>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace themis::sharding {
namespace {

// ─────────────────────────────────────────────────────────────────────────────
// Portable hash helper (matches blockchain_integrity module)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Compute a deterministic, cross-platform SHA-256 hex digest of @p input.
 *
 * Uses OpenSSL's EVP interface for a real SHA-256 computation, guaranteeing
 * stable, cryptographically-sound output across all platforms and builds.
 * Returns a 64-character lowercase hex string.
 *
 * @throws std::runtime_error if the OpenSSL context cannot be allocated.
 */
[[nodiscard]] static std::string portableSha256Hex(const std::string& input) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        throw std::runtime_error("portableSha256Hex: EVP_MD_CTX_new failed");
    }
    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1
        || EVP_DigestUpdate(ctx, input.data(),static_cast<int>(input.size())) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("portableSha256Hex: EVP_Digest init/update failed");
    }
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;
    if (EVP_DigestFinal_ex(ctx, hash, &hash_len) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("portableSha256Hex: EVP_DigestFinal_ex failed");
    }
    EVP_MD_CTX_free(ctx);
    std::ostringstream hex = {};
    for (unsigned int i = 0; i < hash_len; ++i) {
        hex << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(hash[i]);
    }
    return hex.str(); // 64 hex chars
}

[[nodiscard]] static std::string combineHashes(const std::string& left, const std::string& right) {
    return portableSha256Hex(left + right);
}

/// Generate a random UUID-like event identifier.
[[nodiscard]] static std::string generateEventId() {
    static std::mutex rng_mu;
    static std::mt19937_64 rng{std::random_device{}()};
    std::lock_guard<std::mutex> lock(rng_mu);
    const uint64_t a = rng();
    const uint64_t b = rng();
    std::ostringstream oss = {};
    oss << std::hex << std::setw(16) << std::setfill('0') << a
        << std::setw(16) << std::setfill('0') << b;
    return "evt_" + oss.str();
}

[[nodiscard]] static std::string generateSnapshotId() {
    return "snap_" + generateEventId().substr(5);
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// InMemoryAdapterDistributionStore
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Thread-safe in-memory implementation of IAdapterDistributionStore.
 *
 * Suitable for testing and single-node deployments.  For multi-node durability
 * this class should be replaced by a RocksDB-backed or metadata-shard-backed
 * implementation.
 */
class InMemoryAdapterDistributionStore final : public IAdapterDistributionStore {
public:
    [[nodiscard]] bool storeReceipt(const AdapterDistributionReceipt& receipt) override {
        if (receipt.event_id.empty()) {
            throw std::invalid_argument("storeReceipt: event_id must not be empty");
        }
        std::unique_lock lock(mu_);
        if (receipts_.count(receipt.event_id)) {
            return false; // idempotent: already stored
        }
        receipts_.emplace(receipt.event_id, receipt);
        shard_event_index_[receipt.target_shard_id].push_back(receipt.event_id);
        return true;
    }

    [[nodiscard]] std::optional<AdapterDistributionReceipt> getReceipt(
        const DistributionEventId& event_id) const override {
        std::shared_lock lock(mu_);
        auto it = receipts_.find(event_id);
        if (it == receipts_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    [[nodiscard]] bool updateReceiptStatus(
        const DistributionEventId& event_id,
        ArtifactDistributionStatus new_status,
        const std::string& target_signature = {},
        const std::string& error_message = {}) override {
        std::unique_lock lock(mu_);
        auto it = receipts_.find(event_id);
        if (it == receipts_.end()) {
            return false;
        }
        it->second.status = new_status;
        if (new_status == ArtifactDistributionStatus::Confirmed) {
            it->second.confirmed_at = std::chrono::system_clock::now();
        }
        if (!target_signature.empty()) {
            it->second.target_signature = target_signature;
        }
        if (!error_message.empty()) {
            it->second.error_message = error_message;
        }
        return true;
    }

    [[nodiscard]] std::vector<AdapterDistributionReceipt> listReceiptsForShard(
        const DistributionShardId& shard_id,
        std::optional<ArtifactDistributionStatus> status) const override {
        std::shared_lock lock(mu_);
        std::vector<AdapterDistributionReceipt> result;
        auto it = shard_event_index_.find(shard_id);
        if (it == shard_event_index_.end()) {
            return result;
        }
        for (const auto& eid : it->second) {
            auto rit = receipts_.find(eid);
            if (rit == receipts_.end()) {
              continue;
            }
            if (status.has_value() && rit->second.status != *status) {
              continue;
            }
            result.push_back(rit->second);
        }
        return result;
    }

    [[nodiscard]] bool storeSnapshot(const ShardDistributionSnapshot& snapshot) override {
        if (snapshot.snapshot_id.empty()) {
            throw std::invalid_argument("storeSnapshot: snapshot_id must not be empty");
        }
        std::unique_lock lock(mu_);
        if (snapshots_.count(snapshot.snapshot_id)) {
            return false; // idempotent
        }
        snapshots_.emplace(snapshot.snapshot_id, snapshot);
        // Keep only the latest snapshot per shard for O(1) latest lookup.
        auto& latest = latest_snapshot_per_shard_[snapshot.shard_id];
        if (!latest.has_value()
            || snapshot.captured_at > latest->captured_at) {
            latest = snapshot;
        }
        return true;
    }

    [[nodiscard]] std::optional<ShardDistributionSnapshot> getLatestSnapshot(
        const DistributionShardId& shard_id) const override {
        std::shared_lock lock(mu_);
        auto it = latest_snapshot_per_shard_.find(shard_id);
        if (it == latest_snapshot_per_shard_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    [[nodiscard]] uint64_t countReceiptsSinceSnapshot(
        const DistributionShardId& shard_id,
        uint64_t after_count) const override {
        std::shared_lock lock(mu_);
        auto it = shard_event_index_.find(shard_id);
        if (it == shard_event_index_.end()) {
            return 0;
        }
        uint64_t confirmed_total = 0;
        for (const auto& eid : it->second) {
            auto rit = receipts_.find(eid);
            if (rit != receipts_.end()
                && rit->second.status == ArtifactDistributionStatus::Confirmed) {
                ++confirmed_total;
            }
        }
        return confirmed_total > after_count ? confirmed_total - after_count : 0;
    }

private:
    mutable std::shared_mutex mu_;
    std::unordered_map<DistributionEventId, AdapterDistributionReceipt> receipts_;
    /// target_shard_id → ordered list of event_ids (insertion order)
    std::unordered_map<DistributionShardId, std::vector<DistributionEventId>> shard_event_index_;
    std::unordered_map<DistributionSnapshotId, ShardDistributionSnapshot> snapshots_;
    std::unordered_map<DistributionShardId, std::optional<ShardDistributionSnapshot>>
        latest_snapshot_per_shard_;
};

// ─────────────────────────────────────────────────────────────────────────────
// DefaultMerkleProofEngine
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief SHA-256 Merkle proof engine.
 *
 * Deterministic leaf ordering: artifacts are sorted by (adapter_id, version)
 * before tree construction to guarantee reproducible roots.
 */
class DefaultMerkleProofEngine final : public IArtifactMerkleProofEngine {
public:
    [[nodiscard]] std::string buildRoot(
        const std::vector<LoRAPackageRef>& artifacts) const override {
        if (artifacts.empty()) {
            throw std::invalid_argument("buildRoot: artifacts must not be empty");
        }
        auto sorted = sortedArtifacts(artifacts);
        std::vector<std::string> layer = {};

        layer.reserve(sorted.size());
        for (const auto& a : sorted) {
            layer.push_back(leafHash(a));
        }
        return buildTree(layer);
    }

    [[nodiscard]] std::optional<ArtifactMerkleProof> generateProof(
        const std::vector<LoRAPackageRef>& artifacts,
        const std::string& adapter_id,
        const std::string& version) const override {
        if (artifacts.empty()) {
          return std::nullopt;
        }
        auto sorted = sortedArtifacts(artifacts);

        // Find leaf index by composite key (adapter_id, version)
        size_t leaf_idx = sorted.size(); // sentinel
        for (size_t i = 0; i <static_cast<int>(sorted.size()); ++i) {
            if (sorted[i].adapter_id == adapter_id && sorted[i].version == version) {
                leaf_idx = i;
                break;
            }
        }
        if (leaf_idx == sorted.size()) {
            return std::nullopt; // not in batch
        }

        // Build all layers and collect proof path
        std::vector<std::string> layer = {};

        layer.reserve(sorted.size());
        for (const auto& a : sorted) {
            layer.push_back(leafHash(a));
        }

        const std::string the_leaf_hash = layer[leaf_idx];
        std::vector<nlohmann::json> proof_path;
        size_t idx = leaf_idx;

        while (layer.size() > 1) {
            if (layer.size() % 2 != 0) {
                layer.push_back(layer.back()); // duplicate last leaf
            }
            std::vector<std::string> next = {};

            next.reserve(layer.size() / 2);
            for (size_t i = 0; i <static_cast<int>(layer.size()); i += 2) {
                next.push_back(combineHashes(layer[i], layer[i + 1]));
                // Collect sibling for our proof path
                if (i == idx || i + 1 == idx) {
                    size_t sibling_i = (idx % 2 == 0) ? idx + 1 : idx - 1;
                    if (static_cast<int>(layer.size()) > sibling_i) {
                        std::string position = (sibling_i > idx) ? "right" : "left";
                        proof_path.push_back({
                            {"hash",     layer[sibling_i]},
                            {"position", position}
                        });
                    }
                }
            }
            layer = std::move(next);
            idx /= 2;
        }

        ArtifactMerkleProof proof;
        proof.merkle_root = layer[0];
        proof.leaf_hash   = the_leaf_hash;
        proof.adapter_id  = adapter_id;
        proof.version     = version;
        proof.proof_path  = std::move(proof_path);
        proof.batch_size  = static_cast<uint64_t>(sorted.size());
        return proof;
    }

    [[nodiscard]] bool verifyProof(const ArtifactMerkleProof& proof) const override {
        if (!proof.isValid()) {
          return false;
        }
        std::string current = proof.leaf_hash;
        for (const auto& step : proof.proof_path) {
            if (!step.contains("hash") || !step.contains("position")) {
              return false;
            }
            const std::string sibling  = step["hash"].get<std::string>();
            const std::string position = step["position"].get<std::string>();
            if (position == "right") {
                current = combineHashes(current, sibling);
            } else if (position == "left") {
                current = combineHashes(sibling, current);
            } else {
                return false; // reject malformed position strings
            }
        }
        return current == proof.merkle_root;
    }

    [[nodiscard]] std::string leafHash(const LoRAPackageRef& artifact) const override {
        // Canonical JSON with sorted keys matches blockchain_integrity module.
        nlohmann::json j = {
            {"adapter_id",    artifact.adapter_id},
            {"artifact_type", static_cast<int>(artifact.artifact_type)},
            {"base_model",    artifact.base_model},
            {"content_hash",  artifact.content_hash},
            {"size_bytes",    artifact.size_bytes},
            {"version",       artifact.version}
        };
        return portableSha256Hex(j.dump());
    }

private:
    [[nodiscard]] static std::vector<LoRAPackageRef> sortedArtifacts(
        const std::vector<LoRAPackageRef>& artifacts) {
        auto sorted = artifacts;
        std::sort(sorted.begin(), sorted.end(),
            [](const LoRAPackageRef& a, const LoRAPackageRef& b) {
                if (a.adapter_id != b.adapter_id) {
                  return a.adapter_id < b.adapter_id;
                }
                return a.version < b.version;
            });
        return sorted;
    }

    [[nodiscard]] static std::string buildTree(std::vector<std::string> layer) {
        while (layer.size() > 1) {
            if (layer.size() % 2 != 0) {
                layer.push_back(layer.back());
            }
            std::vector<std::string> next = {};

            next.reserve(layer.size() / 2);
            for (size_t i = 0; i <static_cast<int>(layer.size()); i += 2) {
                next.push_back(combineHashes(layer[i], layer[i + 1]));
            }
            layer = std::move(next);
        }
        return layer[0];
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// DefaultLoRADistributionManager
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Default orchestrator for LoRA artifact distribution.
 *
 * Uses an IAdapterDistributionStore for persistence and an
 * IArtifactMerkleProofEngine for proof generation.
 */
class DefaultLoRADistributionManager final : public ILoRADistributionManager {
public:
    /**
     * @param store         Receipt/snapshot persistence backend.
     * @param proof_engine  Merkle proof builder (optional; uses DefaultMerkleProofEngine
     *                      if nullptr is passed).
     */
    explicit DefaultLoRADistributionManager(
        std::shared_ptr<IAdapterDistributionStore> store,
        std::shared_ptr<IArtifactMerkleProofEngine> proof_engine = nullptr)
        : store_(std::move(store))
        , proof_engine_(proof_engine
              ? std::move(proof_engine)
              : std::make_shared<DefaultMerkleProofEngine>()) {}

    [[nodiscard]] DistributionEventId distributeArtifact(
        const LoRAPackageRef& artifact,
        const DistributionShardId& source_shard_id,
        const DistributionShardId& target_shard_id,
        const std::string& batch_merkle_root) override {
        if (!artifact.isValid()) {
            throw std::invalid_argument("distributeArtifact: invalid artifact reference");
        }

        const std::string event_id = generateEventId();
        const std::string root = batch_merkle_root.empty()
            ? proof_engine_->buildRoot({artifact})
            : batch_merkle_root;

        // Build receipt hash over all fields (previous hash chained)
        const std::string prev_hash = lastReceiptHash(target_shard_id);
        const std::string receipt_hash = computeReceiptHash(
            event_id, artifact.adapter_id, source_shard_id, target_shard_id, root, prev_hash);

        AdapterDistributionReceipt receipt;
        receipt.event_id              = event_id;
        receipt.artifact_ref          = artifact;
        receipt.source_shard_id       = source_shard_id;
        receipt.target_shard_id       = target_shard_id;
        receipt.initiated_at          = std::chrono::system_clock::now();
        receipt.status                = ArtifactDistributionStatus::Pending;
        receipt.previous_receipt_hash = prev_hash;
        receipt.receipt_hash          = receipt_hash;
        receipt.batch_merkle_root     = root;

        if (!store_->storeReceipt(receipt)) {
            throw std::runtime_error("distributeArtifact: failed to persist distribution receipt");
        }
        setLastReceiptHash(target_shard_id, receipt_hash);
        return event_id;
    }

    [[nodiscard]] std::unordered_map<DistributionShardId, DistributionEventId>
    distributeToAllShards(
        const LoRAPackageRef& artifact,
        const DistributionShardId& source_shard_id,
        const std::vector<DistributionShardId>& target_shard_ids) override {
        if (target_shard_ids.empty()) {
            throw std::invalid_argument("distributeToAllShards: target list must not be empty");
        }
        if (!artifact.isValid()) {
            throw std::invalid_argument("distributeToAllShards: invalid artifact reference");
        }

        // Use same batch Merkle root (single artifact → deterministic)
        const std::string batch_root = proof_engine_->buildRoot({artifact});

        std::unordered_map<DistributionShardId, DistributionEventId> result = {};

        for (const auto& target : target_shard_ids) {
            result[target] = distributeArtifact(artifact, source_shard_id, target, batch_root);
        }
        return result;
    }

    [[nodiscard]] bool confirmReceipt(
        const DistributionEventId& event_id,
        const std::string& target_signature) override {
        auto receipt = store_->getReceipt(event_id);
        if (!receipt.has_value()) {
          return false;
        }
        // Only Pending or InTransit receipts may be confirmed.
        if (receipt->status != ArtifactDistributionStatus::Pending
            && receipt->status != ArtifactDistributionStatus::InTransit) {
            return false;
        }
        if (receipt->status == ArtifactDistributionStatus::Pending) {
            const bool moved_to_in_transit =
                store_->updateReceiptStatus(event_id, ArtifactDistributionStatus::InTransit);
            if (!moved_to_in_transit) {
                return false;
            }
        }
        return store_->updateReceiptStatus(
            event_id,
            ArtifactDistributionStatus::Confirmed,
            target_signature);
    }

    [[nodiscard]] bool markFailed(
        const DistributionEventId& event_id,
        const std::string& error_message) override {
        auto receipt = store_->getReceipt(event_id);
        if (!receipt.has_value()) {
          return false;
        }
        return store_->updateReceiptStatus(
            event_id,
            ArtifactDistributionStatus::Failed,
            /*target_signature=*/{},
            error_message);
    }

    [[nodiscard]] std::optional<DistributionEventId> recoverDistribution(
        const DistributionEventId& failed_event_id) override {
        auto failed = store_->getReceipt(failed_event_id);
        if (!failed.has_value()) {
          return std::nullopt;
        }
        if (failed->status != ArtifactDistributionStatus::Failed) {
          return std::nullopt;
        }

        // Create a new distribution event linked via receipt chain
        const std::string recovery_event_id = generateEventId();
        const std::string batch_root = failed->batch_merkle_root;
        const std::string prev_hash  = failed->receipt_hash; // chain from failed receipt
        const std::string receipt_hash = computeReceiptHash(
            recovery_event_id,
            failed->artifact_ref.adapter_id,
            failed->source_shard_id,
            failed->target_shard_id,
            batch_root,
            prev_hash);

        AdapterDistributionReceipt recovery;
        recovery.event_id              = recovery_event_id;
        recovery.artifact_ref          = failed->artifact_ref;
        recovery.source_shard_id       = failed->source_shard_id;
        recovery.target_shard_id       = failed->target_shard_id;
        recovery.initiated_at          = std::chrono::system_clock::now();
        recovery.status                = ArtifactDistributionStatus::Pending;
        recovery.previous_receipt_hash = prev_hash;
        recovery.receipt_hash          = receipt_hash;
        recovery.batch_merkle_root     = batch_root;

        if (!store_->storeReceipt(recovery)) {
            throw std::runtime_error("recoverDistribution: failed to persist recovery receipt");
        }
        // Mark original as Recovered
        const bool marked_recovered =
            store_->updateReceiptStatus(failed_event_id, ArtifactDistributionStatus::Recovered);
        if (!marked_recovered) {
            return std::nullopt;
        }
        setLastReceiptHash(failed->target_shard_id, receipt_hash);
        return recovery_event_id;
    }

    [[nodiscard]] std::optional<AdapterDistributionReceipt> getDistributionStatus(
        const DistributionEventId& event_id) const override {
        return store_->getReceipt(event_id);
    }

    [[nodiscard]] std::optional<ArtifactMerkleProof> generateBatchProof(
        const std::string& batch_merkle_root,
        const std::string& adapter_id,
        const std::string& version) const override {
        // Collect unique artifacts from confirmed receipts sharing this batch_merkle_root.
        // NOTE: In production, an index on batch_merkle_root would replace this scan.
        std::vector<LoRAPackageRef> batch_artifacts;
        std::unordered_set<std::string> seen;

        for (const auto& shard_id : knownShardIds()) {
            auto receipts = store_->listReceiptsForShard(
                shard_id, ArtifactDistributionStatus::Confirmed);
            for (const auto& r : receipts) {
                if (r.batch_merkle_root != batch_merkle_root) {
                  continue;
                }
                const std::string key = r.artifact_ref.adapter_id + "@" + r.artifact_ref.version;
                if (seen.insert(key).second) {
                    batch_artifacts.push_back(r.artifact_ref);
                }
            }
        }
        if (batch_artifacts.empty()) {
          return std::nullopt;
        }
        return proof_engine_->generateProof(batch_artifacts, adapter_id, version);
    }

    [[nodiscard]] ShardDistributionSnapshot takeDistributionSnapshot(
        const DistributionShardId& shard_id) override {
        auto confirmed = store_->listReceiptsForShard(
            shard_id, ArtifactDistributionStatus::Confirmed);

        // Canonical ordering by event_id ensures snapshot roots are comparable
        // across different store implementations and call orderings.
        std::sort(confirmed.begin(), confirmed.end(),
            [](const AdapterDistributionReceipt& a, const AdapterDistributionReceipt& b) {
                return a.event_id < b.event_id;
            });

        std::vector<DistributionEventId> event_ids;
        std::vector<std::string> receipt_hashes = {};

        for (const auto& r : confirmed) {
            event_ids.push_back(r.event_id);
            receipt_hashes.push_back(r.receipt_hash);
        }

        // Build Merkle root over canonically ordered receipt hashes
        std::string receipts_root = {};
        if (!receipt_hashes.empty()) {
            std::vector<std::string> layer = receipt_hashes;
            while (layer.size() > 1) {
                if (layer.size() % 2 != 0) {
                  layer.push_back(layer.back());
                }
                std::vector<std::string> next = {};

                next.reserve(layer.size() / 2);
                for (size_t i = 0; i <static_cast<int>(layer.size()); i += 2) {
                    next.push_back(combineHashes(layer[i], layer[i + 1]));
                }
                layer = std::move(next);
            }
            receipts_root = layer[0];
        } else {
            receipts_root = portableSha256Hex("");
        }

        ShardDistributionSnapshot snap;
        snap.snapshot_id              = generateSnapshotId();
        snap.shard_id                 = shard_id;
        snap.captured_at              = std::chrono::system_clock::now();
        snap.confirmed_receipt_count  = static_cast<uint64_t>(confirmed.size());
        snap.receipts_merkle_root     = receipts_root;
        snap.included_event_ids       = std::move(event_ids);
        snap.snapshot_hash            = portableSha256Hex(
            snap.snapshot_id + shard_id + receipts_root
            + std::to_string(snap.confirmed_receipt_count));

        if (!store_->storeSnapshot(snap)) {
            throw std::runtime_error("takeDistributionSnapshot: failed to persist shard snapshot");
        }

        // Track known shard IDs for batch proof lookup
        std::lock_guard<std::mutex> lock(known_shards_mu_);
        known_shards_.insert(shard_id);

        return snap;
    }

    [[nodiscard]] std::vector<DistributionEventId> getRecoveryOrder(
        const DistributionShardId& shard_id) const override {
        auto snapshot = store_->getLatestSnapshot(shard_id);
        const uint64_t snapshot_count = snapshot.has_value()
            ? snapshot->confirmed_receipt_count : 0;

        const uint64_t remaining = store_->countReceiptsSinceSnapshot(shard_id, snapshot_count);
        if (remaining == 0) return {};

        // Return confirmed receipts beyond the snapshot count
        auto all_confirmed = store_->listReceiptsForShard(
            shard_id, ArtifactDistributionStatus::Confirmed);

        std::vector<DistributionEventId> recovery_order;
        recovery_order.reserve(static_cast<size_t>(remaining));
        uint64_t idx = 0;
        for (const auto& r : all_confirmed) {
            if (idx >= snapshot_count) {
                recovery_order.push_back(r.event_id);
            }
            ++idx;
        }
        return recovery_order;
    }

private:
    std::shared_ptr<IAdapterDistributionStore> store_;
    std::shared_ptr<IArtifactMerkleProofEngine> proof_engine_;

    // Receipt hash chain: last receipt hash per target shard
    mutable std::mutex chain_mu_;
    std::unordered_map<DistributionShardId, std::string> last_receipt_hash_;

    // Known shard IDs (populated by takeDistributionSnapshot and distribute*)
    mutable std::mutex known_shards_mu_;
    std::unordered_set<DistributionShardId> known_shards_;

    [[nodiscard]] std::string lastReceiptHash(const DistributionShardId& shard_id) const {
        std::lock_guard<std::mutex> lock(chain_mu_);
        auto it = last_receipt_hash_.find(shard_id);
        return it != last_receipt_hash_.end() ? it->second : std::string{};
    }

    void setLastReceiptHash(const DistributionShardId& shard_id, const std::string& hash) {
        std::lock_guard<std::mutex> lock(chain_mu_);
        last_receipt_hash_[shard_id] = hash;
        std::lock_guard<std::mutex> slock(known_shards_mu_);
        known_shards_.insert(shard_id);
    }

    [[nodiscard]] static std::string computeReceiptHash(
        const std::string& event_id,
        const std::string& adapter_id,
        const std::string& source,
        const std::string& target,
        const std::string& batch_root,
        const std::string& prev_hash) {
        return portableSha256Hex(event_id + adapter_id + source + target + batch_root + prev_hash);
    }

    [[nodiscard]] std::unordered_set<DistributionShardId> knownShardIds() const {
        std::lock_guard<std::mutex> lock(known_shards_mu_);
        return known_shards_;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Factory functions (public API)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Create an in-memory distribution store (suitable for tests and single-node use).
 */
std::shared_ptr<IAdapterDistributionStore> makeInMemoryDistributionStore() {
    return std::make_shared<InMemoryAdapterDistributionStore>();
}

/**
 * @brief Create the default Merkle proof engine.
 */
std::shared_ptr<IArtifactMerkleProofEngine> makeDefaultMerkleProofEngine() {
    return std::make_shared<DefaultMerkleProofEngine>();
}

/**
 * @brief Create a distribution manager backed by the given store and proof engine.
 *
 * @param store         Receipt/snapshot backend.  Pass nullptr to use a fresh
 *                      in-memory store.
 * @param proof_engine  Merkle proof engine.  Pass nullptr for the default.
 */
std::shared_ptr<ILoRADistributionManager> makeLoRADistributionManager(
    std::shared_ptr<IAdapterDistributionStore> store,
    std::shared_ptr<IArtifactMerkleProofEngine> proof_engine) {
    if (!store) {
        store = makeInMemoryDistributionStore();
    }
    return std::make_shared<DefaultLoRADistributionManager>(
        std::move(store), std::move(proof_engine));
}

} // namespace themis::sharding
