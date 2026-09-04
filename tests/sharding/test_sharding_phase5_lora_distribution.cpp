// Copyright 2025 ThemisDB
// Licensed under MIT License

/**
 * @file test_sharding_phase5_lora_distribution.cpp
 * @brief Phase 5: Adapter-Distribution & Sharding-Kopplung — End-to-end tests
 *
 * Test IDs: P5-L01 … P5-L25
 * Scope:
 *   - LoRAPackageRef validation (P5-L01..P5-L03)
 *   - InMemoryAdapterDistributionStore CRUD (P5-L04..P5-L10)
 *   - DefaultMerkleProofEngine (P5-L11..P5-L16)
 *   - DefaultLoRADistributionManager: distribute / confirm / fail / recover (P5-L17..P5-L22)
 *   - ShardDistributionSnapshot & recovery ordering (P5-L23..P5-L24)
 *   - End-to-end artifact receipt chain (P5-L25)
 *
 * Issue: #5418 — phase5-adapter-distribution-sharding-2026
 */

#include <gtest/gtest.h>
#include "sharding/lora_artifact_distribution.h"

#include <algorithm>
#include <string>
#include <thread>
#include <vector>
#include <unordered_set>

using namespace themis::sharding;

namespace {

// ─────────────────────────────────────────────────────────────────────────────
// Test helpers
// ─────────────────────────────────────────────────────────────────────────────

LoRAPackageRef makeRef(const std::string& id,
                       const std::string& version = "1.0.0",
                       AdapterArtifactType type   = AdapterArtifactType::LoRAPackage) {
    LoRAPackageRef ref;
    ref.adapter_id    = id;
    ref.version       = version;
    ref.artifact_type = type;
    // Generate a plausible 64-char hex content_hash
    ref.content_hash  = std::string(64, '0');
    for (size_t i = 0; i < id.size() && i < 64; ++i) {
        ref.content_hash[i] = id[i];
    }
    ref.base_model  = "llama-2-7b";
    ref.size_bytes  = 1024 * 1024;
    return ref;
}

std::shared_ptr<ILoRADistributionManager> makeManager() {
    return makeLoRADistributionManager(
        makeInMemoryDistributionStore(),
        makeDefaultMerkleProofEngine());
}

} // anonymous namespace

// =============================================================================
// P5-L01..P5-L03 — LoRAPackageRef validation
// =============================================================================

TEST(P5LoRAPackageRefTest, L01_ValidRefPassesIsValid) {
    auto ref = makeRef("adapter-alpha");
    EXPECT_TRUE(ref.isValid());
}

TEST(P5LoRAPackageRefTest, L02_EmptyAdapterIdFailsIsValid) {
    auto ref = makeRef("adapter-alpha");
    ref.adapter_id = "";
    EXPECT_FALSE(ref.isValid());
}

TEST(P5LoRAPackageRefTest, L03_ShortContentHashFailsIsValid) {
    auto ref = makeRef("adapter-beta");
    ref.content_hash = "tooshort";
    EXPECT_FALSE(ref.isValid());
}

// =============================================================================
// P5-L04..P5-L10 — InMemoryAdapterDistributionStore CRUD
// =============================================================================

class P5DistributionStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        store_ = makeInMemoryDistributionStore();
    }

    /// Build a minimal receipt with all required fields set.
    AdapterDistributionReceipt makeReceipt(const std::string& event_id,
                                           const std::string& target_shard = "shard-A") {
        AdapterDistributionReceipt r;
        r.event_id          = event_id;
        r.artifact_ref      = makeRef("adapter-1");
        r.source_shard_id   = "shard-src";
        r.target_shard_id   = target_shard;
        r.initiated_at      = std::chrono::system_clock::now();
        r.status            = ArtifactDistributionStatus::Pending;
        r.receipt_hash      = std::string(64, 'a');
        r.batch_merkle_root = std::string(64, 'b');
        return r;
    }

    std::shared_ptr<IAdapterDistributionStore> store_;
};

TEST_F(P5DistributionStoreTest, L04_StoreAndRetrieveReceipt) {
    auto receipt = makeReceipt("evt-001");
    ASSERT_TRUE(store_->storeReceipt(receipt));
    auto retrieved = store_->getReceipt("evt-001");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->event_id, "evt-001");
    EXPECT_EQ(retrieved->status, ArtifactDistributionStatus::Pending);
}

TEST_F(P5DistributionStoreTest, L05_StoreReceiptIdempotent) {
    auto receipt = makeReceipt("evt-002");
    EXPECT_TRUE(store_->storeReceipt(receipt));
    EXPECT_FALSE(store_->storeReceipt(receipt)); // second insert is no-op
    // Still retrievable
    EXPECT_TRUE(store_->getReceipt("evt-002").has_value());
}

TEST_F(P5DistributionStoreTest, L06_GetReceiptMissingReturnsNullopt) {
    EXPECT_FALSE(store_->getReceipt("nonexistent").has_value());
}

TEST_F(P5DistributionStoreTest, L07_UpdateReceiptStatusToConfirmed) {
    auto receipt = makeReceipt("evt-003");
    store_->storeReceipt(receipt);
    ASSERT_TRUE(store_->updateReceiptStatus(
        "evt-003", ArtifactDistributionStatus::Confirmed, "sig-xyz"));
    auto updated = store_->getReceipt("evt-003");
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated->status, ArtifactDistributionStatus::Confirmed);
    EXPECT_TRUE(updated->confirmed_at.has_value());
    EXPECT_EQ(updated->target_signature, "sig-xyz");
}

TEST_F(P5DistributionStoreTest, L08_ListReceiptsForShardFiltered) {
    store_->storeReceipt(makeReceipt("evt-A1", "shard-X"));
    store_->storeReceipt(makeReceipt("evt-A2", "shard-X"));
    store_->storeReceipt(makeReceipt("evt-B1", "shard-Y"));

    // Confirm one of shard-X's receipts
    store_->updateReceiptStatus("evt-A1", ArtifactDistributionStatus::Confirmed);

    auto all_x = store_->listReceiptsForShard("shard-X");
    EXPECT_EQ(all_x.size(), 2u);

    auto confirmed_x = store_->listReceiptsForShard(
        "shard-X", ArtifactDistributionStatus::Confirmed);
    EXPECT_EQ(confirmed_x.size(), 1u);
    EXPECT_EQ(confirmed_x[0].event_id, "evt-A1");
}

TEST_F(P5DistributionStoreTest, L09_StoreAndRetrieveSnapshot) {
    ShardDistributionSnapshot snap;
    snap.snapshot_id             = "snap-001";
    snap.shard_id                = "shard-Z";
    snap.captured_at             = std::chrono::system_clock::now();
    snap.confirmed_receipt_count = 3;
    snap.receipts_merkle_root    = std::string(64, 'c');
    snap.snapshot_hash           = std::string(64, 'd');

    ASSERT_TRUE(store_->storeSnapshot(snap));

    auto latest = store_->getLatestSnapshot("shard-Z");
    ASSERT_TRUE(latest.has_value());
    EXPECT_EQ(latest->snapshot_id, "snap-001");
    EXPECT_EQ(latest->confirmed_receipt_count, 3u);
}

TEST_F(P5DistributionStoreTest, L10_CountReceiptsSinceSnapshot) {
    // 4 receipts: confirm 3 of them
    store_->storeReceipt(makeReceipt("e1", "shard-W"));
    store_->storeReceipt(makeReceipt("e2", "shard-W"));
    store_->storeReceipt(makeReceipt("e3", "shard-W"));
    store_->storeReceipt(makeReceipt("e4", "shard-W"));
    store_->updateReceiptStatus("e1", ArtifactDistributionStatus::Confirmed);
    store_->updateReceiptStatus("e2", ArtifactDistributionStatus::Confirmed);
    store_->updateReceiptStatus("e3", ArtifactDistributionStatus::Confirmed);

    // snapshot_count = 1 → 2 new confirmed receipts since then
    EXPECT_EQ(store_->countReceiptsSinceSnapshot("shard-W", 1), 2u);
    // snapshot_count = 3 → 0 new
    EXPECT_EQ(store_->countReceiptsSinceSnapshot("shard-W", 3), 0u);
}

// =============================================================================
// P5-L11..P5-L16 — DefaultMerkleProofEngine
// =============================================================================

class P5MerkleProofTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine_ = makeDefaultMerkleProofEngine();
    }
    std::shared_ptr<IArtifactMerkleProofEngine> engine_;
};

TEST_F(P5MerkleProofTest, L11_SingleArtifactRootIsDeterministic) {
    auto ref  = makeRef("ada-001");
    auto root1 = engine_->buildRoot({ref});
    auto root2 = engine_->buildRoot({ref});
    EXPECT_EQ(root1, root2);
    EXPECT_EQ(root1.size(), 64u);
}

TEST_F(P5MerkleProofTest, L12_MultipleArtifactRootDeterministic) {
    std::vector<LoRAPackageRef> batch = {
        makeRef("ada-A"), makeRef("ada-B"), makeRef("ada-C")
    };
    auto root1 = engine_->buildRoot(batch);
    // Shuffle and rebuild — same root (sorting is deterministic)
    std::vector<LoRAPackageRef> shuffled = {makeRef("ada-C"), makeRef("ada-A"), makeRef("ada-B")};
    auto root2 = engine_->buildRoot(shuffled);
    EXPECT_EQ(root1, root2);
}

TEST_F(P5MerkleProofTest, L13_DifferentArtifactsProduceDifferentRoots) {
    auto root1 = engine_->buildRoot({makeRef("ada-X")});
    auto root2 = engine_->buildRoot({makeRef("ada-Y")});
    EXPECT_NE(root1, root2);
}

TEST_F(P5MerkleProofTest, L14_GenerateProofForExistingArtifact) {
    std::vector<LoRAPackageRef> batch = {
        makeRef("ada-1"), makeRef("ada-2"), makeRef("ada-3")
    };
    auto proof = engine_->generateProof(batch, "ada-1", "1.0.0");
    ASSERT_TRUE(proof.has_value());
    EXPECT_EQ(proof->adapter_id, "ada-1");
    EXPECT_EQ(proof->version, "1.0.0");
    EXPECT_EQ(proof->merkle_root.size(), 64u);
    EXPECT_EQ(proof->leaf_hash.size(), 64u);
    EXPECT_EQ(proof->batch_size, 3u);
}

TEST_F(P5MerkleProofTest, L15_GenerateProofMissingAdapterReturnsNullopt) {
    std::vector<LoRAPackageRef> batch = {makeRef("ada-1"), makeRef("ada-2")};
    EXPECT_FALSE(engine_->generateProof(batch, "ada-nonexistent", "1.0.0").has_value());
}

TEST_F(P5MerkleProofTest, L16_VerifyProofRoundTrip) {
    std::vector<LoRAPackageRef> batch = {};

    for (int i = 0; i < 5; ++i) {
        batch.push_back(makeRef("ada-" + std::to_string(i)));
    }
    for (int i = 0; i < 5; ++i) {
        auto proof = engine_->generateProof(batch, "ada-" + std::to_string(i), "1.0.0");
        ASSERT_TRUE(proof.has_value()) << "proof missing for ada-" << i;
        EXPECT_TRUE(engine_->verifyProof(*proof))
            << "verification failed for ada-" << i;
    }
}

// =============================================================================
// P5-L17..P5-L22 — DefaultLoRADistributionManager
// =============================================================================

class P5DistributionManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager_ = makeManager();
    }
    std::shared_ptr<ILoRADistributionManager> manager_;
};

TEST_F(P5DistributionManagerTest, L17_DistributeArtifactReturnsPendingStatus) {
    auto ref = makeRef("ada-test", "1.0.0");
    auto event_id = manager_->distributeArtifact(ref, "shard-src", "shard-tgt");
    ASSERT_FALSE(event_id.empty());

    auto status = manager_->getDistributionStatus(event_id);
    ASSERT_TRUE(status.has_value());
    EXPECT_EQ(status->status, ArtifactDistributionStatus::Pending);
    EXPECT_EQ(status->artifact_ref.adapter_id, "ada-test");
    EXPECT_EQ(status->target_shard_id, "shard-tgt");
}

TEST_F(P5DistributionManagerTest, L18_ConfirmReceiptTransitionsToConfirmed) {
    auto ref      = makeRef("ada-confirm");
    auto event_id = manager_->distributeArtifact(ref, "src", "tgt");
    ASSERT_TRUE(manager_->confirmReceipt(event_id, "sig-confirm"));

    auto status = manager_->getDistributionStatus(event_id);
    ASSERT_TRUE(status.has_value());
    EXPECT_EQ(status->status, ArtifactDistributionStatus::Confirmed);
    EXPECT_TRUE(status->confirmed_at.has_value());
    EXPECT_EQ(status->target_signature, "sig-confirm");
}

TEST_F(P5DistributionManagerTest, L19_MarkFailedTransitionsToFailed) {
    auto ref      = makeRef("ada-fail");
    auto event_id = manager_->distributeArtifact(ref, "src", "tgt");
    ASSERT_TRUE(manager_->markFailed(event_id, "network timeout"));

    auto status = manager_->getDistributionStatus(event_id);
    ASSERT_TRUE(status.has_value());
    EXPECT_EQ(status->status, ArtifactDistributionStatus::Failed);
}

TEST_F(P5DistributionManagerTest, L20_RecoverDistributionCreatesNewEvent) {
    auto ref       = makeRef("ada-recover");
    auto event_id  = manager_->distributeArtifact(ref, "src", "tgt");
    manager_->markFailed(event_id, "timeout");

    auto recovery_id = manager_->recoverDistribution(event_id);
    ASSERT_TRUE(recovery_id.has_value());
    EXPECT_NE(*recovery_id, event_id);

    // Original event marked Recovered
    auto orig = manager_->getDistributionStatus(event_id);
    ASSERT_TRUE(orig.has_value());
    EXPECT_EQ(orig->status, ArtifactDistributionStatus::Recovered);

    // Recovery event is Pending
    auto recovery = manager_->getDistributionStatus(*recovery_id);
    ASSERT_TRUE(recovery.has_value());
    EXPECT_EQ(recovery->status, ArtifactDistributionStatus::Pending);
}

TEST_F(P5DistributionManagerTest, L21_RecoverNonFailedReturnNullopt) {
    auto ref      = makeRef("ada-no-fail");
    auto event_id = manager_->distributeArtifact(ref, "src", "tgt");
    // Event is Pending, not Failed → cannot recover
    EXPECT_FALSE(manager_->recoverDistribution(event_id).has_value());
}

TEST_F(P5DistributionManagerTest, L22_DistributeToAllShardsCreatesReceiptPerShard) {
    auto ref = makeRef("ada-multi");
    std::vector<DistributionShardId> targets = {"s1", "s2", "s3"};
    auto result = manager_->distributeToAllShards(ref, "src", targets);
    EXPECT_EQ(result.size(), 3u);
    for (const auto& [shard, eid] : result) {
        auto status = manager_->getDistributionStatus(eid);
        ASSERT_TRUE(status.has_value());
        EXPECT_EQ(status->target_shard_id, shard);
        EXPECT_EQ(status->status, ArtifactDistributionStatus::Pending);
    }
}

// =============================================================================
// P5-L23..P5-L24 — ShardDistributionSnapshot & recovery ordering
// =============================================================================

TEST(P5SnapshotRecoveryTest, L23_TakeSnapshotCapturesConfirmedReceipts) {
    auto manager = makeManager();
    auto ref     = makeRef("ada-snap");

    // Distribute to two shards and confirm one
    auto e1 = manager->distributeArtifact(ref, "src", "shard-snap");
    auto e2 = manager->distributeArtifact(ref, "src", "shard-snap");
    manager->confirmReceipt(e1);
    // e2 remains Pending

    auto snap = manager->takeDistributionSnapshot("shard-snap");
    EXPECT_EQ(snap.shard_id, "shard-snap");
    EXPECT_EQ(snap.confirmed_receipt_count, 1u);
    EXPECT_EQ(snap.included_event_ids.size(), 1u);
    EXPECT_EQ(snap.receipts_merkle_root.size(), 64u);
    EXPECT_EQ(snap.snapshot_hash.size(), 64u);
}

TEST(P5SnapshotRecoveryTest, L24_GetRecoveryOrderReturnsPostSnapshotReceipts) {
    auto manager = makeManager();
    auto ref     = makeRef("ada-rec");

    // Confirm 3 events, take snapshot, then confirm 2 more
    for (int i = 0; i < 3; ++i) {
        auto eid = manager->distributeArtifact(ref, "src", "shard-rec");
        manager->confirmReceipt(eid);
    }
    auto snap = manager->takeDistributionSnapshot("shard-rec");
    EXPECT_EQ(snap.confirmed_receipt_count, 3u);

    // Add 2 more confirmed events AFTER the snapshot
    for (int i = 0; i < 2; ++i) {
        auto eid = manager->distributeArtifact(ref, "src", "shard-rec");
        manager->confirmReceipt(eid);
    }

    auto recovery = manager->getRecoveryOrder("shard-rec");
    EXPECT_EQ(recovery.size(), 2u);
}

// =============================================================================
// P5-L25 — End-to-end artifact receipt chain: Send / Confirm / Recover
// =============================================================================

TEST(P5EndToEndTest, L25_FullArtifactReceiptChain) {
    auto store    = makeInMemoryDistributionStore();
    auto engine   = makeDefaultMerkleProofEngine();
    auto manager  = makeLoRADistributionManager(store, engine);

    // ── 1. Build a batch of 4 artifacts ────────────────────────────────────
    std::vector<LoRAPackageRef> batch = {
        makeRef("legal-v1",   "1.0.0", AdapterArtifactType::LoRAPackage),
        makeRef("medical-v2", "2.1.0", AdapterArtifactType::PortableAdapterProduct),
        makeRef("finance-v1", "1.0.0", AdapterArtifactType::CheckpointBundle),
        makeRef("general-v3", "3.0.0", AdapterArtifactType::LoRAPackage)
    };

    // ── 2. Compute batch Merkle root ────────────────────────────────────────
    const std::string batch_root = engine->buildRoot(batch);
    ASSERT_EQ(batch_root.size(), 64u);

    // ── 3. Distribute to 3 shards ───────────────────────────────────────────
    const std::vector<DistributionShardId> shards = {"shard-0", "shard-1", "shard-2"};
    std::vector<DistributionEventId> all_events = {};

    for (const auto& artifact : batch) {
        auto per_shard = manager->distributeToAllShards(artifact, "external", shards);
        for (const auto& [s, eid] : per_shard) {
            all_events.push_back(eid);
        }
    }
    // 4 artifacts × 3 shards = 12 events
    EXPECT_EQ(all_events.size(), 12u);

    // ── 4. Confirm all events except 1 (simulate a failure) ─────────────────
    const std::string failed_eid = all_events.back();
    for (size_t i = 0; i + 1 < all_events.size(); ++i) {
        ASSERT_TRUE(manager->confirmReceipt(all_events[i], "sig-" + std::to_string(i)));
    }
    ASSERT_TRUE(manager->markFailed(failed_eid, "simulated network error"));

    // ── 5. Verify statuses ──────────────────────────────────────────────────
    for (size_t i = 0; i + 1 < all_events.size(); ++i) {
        auto s = manager->getDistributionStatus(all_events[i]);
        ASSERT_TRUE(s.has_value());
        EXPECT_EQ(s->status, ArtifactDistributionStatus::Confirmed);
    }
    {
        auto fs = manager->getDistributionStatus(failed_eid);
        ASSERT_TRUE(fs.has_value());
        EXPECT_EQ(fs->status, ArtifactDistributionStatus::Failed);
    }

    // ── 6. Recover the failed event ─────────────────────────────────────────
    auto recovery_eid = manager->recoverDistribution(failed_eid);
    ASSERT_TRUE(recovery_eid.has_value());
    ASSERT_TRUE(manager->confirmReceipt(*recovery_eid, "sig-recovery"));
    {
        auto rs = manager->getDistributionStatus(*recovery_eid);
        ASSERT_TRUE(rs.has_value());
        EXPECT_EQ(rs->status, ArtifactDistributionStatus::Confirmed);
        // Recovery receipt must be chained from the failed receipt
        auto orig = manager->getDistributionStatus(failed_eid);
        ASSERT_TRUE(orig.has_value());
        EXPECT_EQ(rs->previous_receipt_hash, orig->receipt_hash);
    }

    // ── 7. Take per-shard snapshots ─────────────────────────────────────────
    for (const auto& shard : shards) {
        auto snap = manager->takeDistributionSnapshot(shard);
        EXPECT_EQ(snap.shard_id, shard);
        // Each shard received 4 artifacts confirmed, so confirmed_receipt_count ≥ 4
        EXPECT_GE(snap.confirmed_receipt_count, 4u);
        EXPECT_EQ(snap.receipts_merkle_root.size(), 64u);
        EXPECT_EQ(snap.snapshot_hash.size(), 64u);
    }

    // ── 8. Merkle proof verification for one artifact in batch ──────────────
    auto proof = engine->generateProof(batch, "legal-v1", "1.0.0");
    ASSERT_TRUE(proof.has_value());
    EXPECT_TRUE(engine->verifyProof(*proof));
    EXPECT_EQ(proof->merkle_root, batch_root);
    EXPECT_EQ(proof->adapter_id, "legal-v1");

    // ── 9. Verify receipt hash chain integrity on shard-0 ───────────────────
    auto shard0_receipts = store->listReceiptsForShard(
        "shard-0", ArtifactDistributionStatus::Confirmed);
    // Each receipt except the first should have a non-empty previous_receipt_hash
    for (size_t i = 1; i < shard0_receipts.size(); ++i) {
        EXPECT_FALSE(shard0_receipts[i].previous_receipt_hash.empty())
            << "chain broken at index " << i;
    }

    // ── 10. Recovery ordering: no events outstanding after snapshot ──────────
    for (const auto& shard : shards) {
        auto recovery_order = manager->getRecoveryOrder(shard);
        EXPECT_TRUE(recovery_order.empty())
            << "shard " << shard << " has unexpected recovery entries";
    }
}
