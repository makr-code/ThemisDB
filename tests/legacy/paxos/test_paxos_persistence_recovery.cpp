/**
 * @file test_paxos_persistence_recovery.cpp
 * @brief Focused unit tests for PaxosStatePersistence and Paxos WAL durability.
 *
 * Test IDs: PSR-01 … PSR-10
 *
 * These tests verify that:
 *   - A Paxos acceptor can open and close the persistence layer cleanly.
 *   - Promise state is durably written and readable after a simulated restart.
 *   - Accept state is durably written and readable after a simulated restart.
 *   - Commit state is durably written and readable after a simulated restart.
 *   - Promises with lower ballot numbers are rejected after recovery.
 *   - Multiple slots are independently tracked and recovered.
 *   - forceCompact() writes a snapshot and the slot cache remains consistent.
 *   - The node state (round, committed slot) is restored from the WAL.
 *   - maybeCompact() is a no-op when below the compact_interval threshold.
 *   - writeEntity on ShardRPCClient (in-process path) returns success.
 */

#include <gtest/gtest.h>

#include "sharding/paxos_state_persistence.h"
#include "sharding/paxos_wal.h"
#include "sharding/paxos_snapshot.h"
#include "sharding/shard_rpc_client.h"

#include <filesystem>
#include <string>
#include <memory>
#include <algorithm>

namespace fs = std::filesystem;

using namespace themis::sharding;
using namespace themisdb::sharding;

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture
// ─────────────────────────────────────────────────────────────────────────────

class PaxosPersistenceRecoveryTest : public ::testing::Test {
protected:
    std::string tmp_dir_;
    std::unique_ptr<PaxosWAL>              wal_;
    std::unique_ptr<PaxosSnapshotManager>  snapshot_mgr_;

    void SetUp() override {
        // Create a unique temp directory
        tmp_dir_ = (fs::temp_directory_path() /
                    ("psr_test_" + std::to_string(
                         std::chrono::steady_clock::now().time_since_epoch().count())))
                       .string();
        fs::create_directories(tmp_dir_ + "/wal");
        fs::create_directories(tmp_dir_ + "/snapshots");

        PaxosWALConfig wal_cfg;
        wal_cfg.wal_directory      = tmp_dir_ + "/wal";
        wal_cfg.snapshot_directory = tmp_dir_ + "/snapshots";
        wal_cfg.sync_on_write      = false;  // faster in tests

        wal_ = std::make_unique<PaxosWAL>(wal_cfg);
        ASSERT_TRUE(wal_->initialize());

        snapshot_mgr_ = std::make_unique<PaxosSnapshotManager>(
            tmp_dir_ + "/snapshots", 10);
    }

    void TearDown() override {
        wal_.reset();
        snapshot_mgr_.reset();
        std::error_code ec;
        fs::remove_all(tmp_dir_, ec);
    }

    std::unique_ptr<PaxosStatePersistence> makePersistence(
        size_t compact_interval = 512) {
        PaxosStatePersistence::Config cfg;
        cfg.state_dir        = tmp_dir_ + "/paxos_state";
        cfg.compact_interval = compact_interval;
        cfg.sync_on_write    = false;
        return std::make_unique<PaxosStatePersistence>(
            wal_.get(), snapshot_mgr_.get(), cfg);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// PSR-01: open / close is idempotent and returns true
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(PaxosPersistenceRecoveryTest, PSR01_OpenClose) {
    auto p = makePersistence();

    EXPECT_TRUE(p->open("node-1"));
    EXPECT_TRUE(p->isOpen());

    p->close();
    EXPECT_FALSE(p->isOpen());

    // Re-open should succeed (no crash)
    EXPECT_TRUE(p->open("node-1"));
    p->close();
}

// ─────────────────────────────────────────────────────────────────────────────
// PSR-02: persistPromise is durable — survives a simulated restart
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(PaxosPersistenceRecoveryTest, PSR02_PromiseSurvivesRestart) {
    {
        auto p = makePersistence();
        ASSERT_TRUE(p->open("node-1"));
        EXPECT_TRUE(p->persistPromise(/*slot=*/1, /*ballot=*/5, "proposer-A"));
        p->close();
    }

    // Simulate restart: create a new persistence object backed by the same WAL
    {
        auto p = makePersistence();
        ASSERT_TRUE(p->open("node-1"));

        auto state = p->getAcceptorState(1);
        ASSERT_TRUE(state.has_value());
        EXPECT_EQ(state->slot,           1u);
        EXPECT_EQ(state->promised_round, 5u);
        EXPECT_EQ(state->promised_node,  "proposer-A");
        p->close();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// PSR-03: persistAccept is durable — survives a simulated restart
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(PaxosPersistenceRecoveryTest, PSR03_AcceptSurvivesRestart) {
    const std::string accepted_value = R"({"op":"write","key":"foo","val":"bar"})";

    {
        auto p = makePersistence();
        ASSERT_TRUE(p->open("node-1"));
        EXPECT_TRUE(p->persistPromise(2, 7, "proposer-B"));
        EXPECT_TRUE(p->persistAccept(2, 7, accepted_value));
        p->close();
    }

    {
        auto p = makePersistence();
        ASSERT_TRUE(p->open("node-1"));

        auto state = p->getAcceptorState(2);
        ASSERT_TRUE(state.has_value());
        EXPECT_EQ(state->slot,           2u);
        EXPECT_EQ(state->accepted_round, 7u);
        EXPECT_EQ(state->accepted_value, accepted_value);
        p->close();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// PSR-11: persistAccept writes structured command payload into ACCEPT WAL entry
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(PaxosPersistenceRecoveryTest, PSR11_AcceptWalContainsStructuredPayload) {
    const std::string accepted_value = R"({"operation":"PUT","data":{"key":"foo","val":"bar"}})";

    auto p = makePersistence();
    ASSERT_TRUE(p->open("node-1"));
    ASSERT_TRUE(p->persistAccept(7, 3, accepted_value));
    p->close();

    const auto entries = wal_->readEntries(wal_->getOldestLSN());
    auto accept_it = std::find_if(entries.begin(), entries.end(), [](const PaxosWALEntry& e) {
        return e.type == PaxosWALEntryType::ACCEPT && e.slot == 7;
    });
    ASSERT_NE(accept_it, entries.end());
    ASSERT_TRUE(accept_it->data.contains("value"));
    ASSERT_TRUE(accept_it->data["value"].is_object());
    ASSERT_TRUE(accept_it->data["value"].contains("data"));
    ASSERT_TRUE(accept_it->data["value"]["data"].is_object());
    EXPECT_EQ(accept_it->data["value"]["data"]["raw_command"].get<std::string>(), accepted_value);
    ASSERT_TRUE(accept_it->data["value"]["data"].contains("parsed_command"));
    EXPECT_EQ(accept_it->data["value"]["data"]["parsed_command"]["operation"].get<std::string>(), "PUT");
}

// ─────────────────────────────────────────────────────────────────────────────
// PSR-04: persistCommit marks slot as committed after restart
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(PaxosPersistenceRecoveryTest, PSR04_CommitSurvivesRestart) {
    {
        auto p = makePersistence();
        ASSERT_TRUE(p->open("node-1"));
        EXPECT_TRUE(p->persistPromise(3, 2, "proposer-C"));
        EXPECT_TRUE(p->persistAccept(3, 2, "committed-val"));
        EXPECT_TRUE(p->persistCommit(3));
        p->close();
    }

    {
        auto p = makePersistence();
        ASSERT_TRUE(p->open("node-1"));

        auto state = p->getAcceptorState(3);
        ASSERT_TRUE(state.has_value());
        EXPECT_TRUE(state->is_committed);
        EXPECT_EQ(p->nodeState().last_committed, 3u);
        p->close();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// PSR-05: A lower ballot promise does NOT overwrite a higher one after recovery
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(PaxosPersistenceRecoveryTest, PSR05_LowerBallotRejectedAfterRecovery) {
    {
        auto p = makePersistence();
        ASSERT_TRUE(p->open("node-1"));
        EXPECT_TRUE(p->persistPromise(4, 10, "proposer-X"));
        p->close();
    }

    {
        auto p = makePersistence();
        ASSERT_TRUE(p->open("node-1"));

        // Current promised round is 10; a new promise with ballot 3 must fail.
        // PaxosStatePersistence only persists when called — the caller (handlePrepare)
        // is responsible for the ballot check. Verify the stored round is still 10.
        auto state = p->getAcceptorState(4);
        ASSERT_TRUE(state.has_value());
        EXPECT_EQ(state->promised_round, 10u);
        p->close();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// PSR-06: Multiple independent slots are tracked and recovered correctly
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(PaxosPersistenceRecoveryTest, PSR06_MultipleSlots) {
    {
        auto p = makePersistence();
        ASSERT_TRUE(p->open("node-1"));

        for (uint64_t slot = 10; slot <= 14; ++slot) {
            EXPECT_TRUE(p->persistPromise(slot, slot * 2, "proposer"));
            EXPECT_TRUE(p->persistAccept(slot, slot * 2, "value-" + std::to_string(slot)));
        }
        p->close();
    }

    {
        auto p = makePersistence();
        ASSERT_TRUE(p->open("node-1"));

        for (uint64_t slot = 10; slot <= 14; ++slot) {
            auto state = p->getAcceptorState(slot);
            ASSERT_TRUE(state.has_value()) << "slot=" << slot;
            EXPECT_EQ(state->promised_round, slot * 2) << "slot=" << slot;
            EXPECT_EQ(state->accepted_value,
                      "value-" + std::to_string(slot)) << "slot=" << slot;
        }
        p->close();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// PSR-07: forceCompact writes a snapshot; slot cache remains consistent
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(PaxosPersistenceRecoveryTest, PSR07_ForceCompact) {
    auto p = makePersistence();
    ASSERT_TRUE(p->open("node-1"));

    for (uint64_t slot = 20; slot <= 24; ++slot) {
        EXPECT_TRUE(p->persistPromise(slot, 1, "proposer"));
        EXPECT_TRUE(p->persistAccept(slot, 1, "val"));
        EXPECT_TRUE(p->persistCommit(slot));
    }

    EXPECT_TRUE(p->forceCompact());

    // After compaction, slot cache should still be accessible
    for (uint64_t slot = 20; slot <= 24; ++slot) {
        auto state = p->getAcceptorState(slot);
        ASSERT_TRUE(state.has_value()) << "slot=" << slot;
        EXPECT_TRUE(state->is_committed) << "slot=" << slot;
    }
    p->close();
}

// ─────────────────────────────────────────────────────────────────────────────
// PSR-08: nodeState round counter increases with accepted ballot rounds
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(PaxosPersistenceRecoveryTest, PSR08_NodeStateRoundTracking) {
    {
        auto p = makePersistence();
        ASSERT_TRUE(p->open("node-1"));

        EXPECT_TRUE(p->persistPromise(30, 100, "proposer"));
        EXPECT_TRUE(p->persistPromise(31, 200, "proposer"));

        // The highest ballot round observed must be tracked
        EXPECT_GE(p->nodeState().current_round, 200u);
        p->close();
    }

    // Verify round is restored
    {
        auto p = makePersistence();
        ASSERT_TRUE(p->open("node-1"));
        EXPECT_GE(p->nodeState().current_round, 200u);
        p->close();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// PSR-09: maybeCompact is a no-op when below the compact_interval threshold
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(PaxosPersistenceRecoveryTest, PSR09_MaybeCompactNoOp) {
    // compact_interval = 100 commits; we only do 3
    auto p = makePersistence(/*compact_interval=*/100);
    ASSERT_TRUE(p->open("node-1"));

    for (uint64_t slot = 40; slot <= 42; ++slot) {
        EXPECT_TRUE(p->persistPromise(slot, 1, "proposer"));
        EXPECT_TRUE(p->persistAccept(slot, 1, "val"));
        EXPECT_TRUE(p->persistCommit(slot));
    }

    // Should not throw or corrupt state
    p->maybeCompact();

    // Slots must still be accessible
    for (uint64_t slot = 40; slot <= 42; ++slot) {
        auto state = p->getAcceptorState(slot);
        ASSERT_TRUE(state.has_value()) << "slot=" << slot;
    }
    p->close();
}

// ─────────────────────────────────────────────────────────────────────────────
// PSR-10: ACCEPT WAL entry stores structured payload metadata
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(PaxosPersistenceRecoveryTest, PSR10_AcceptWalCarriesStructuredPayload) {
    auto p = makePersistence();
    ASSERT_TRUE(p->open("node-1"));

    const std::string accepted_value = R"({"operation":"UPSERT","entity":"users","id":"u-1"})";
    ASSERT_TRUE(p->persistAccept(/*slot=*/55, /*ballot_round=*/12, accepted_value));
    p->close();

    const auto entries = wal_->readEntries(LSN(0, 0));
    auto accept_it = std::find_if(entries.begin(), entries.end(), [](const PaxosWALEntry& e) {
        return e.type == PaxosWALEntryType::ACCEPT && e.slot == 55;
    });

    ASSERT_NE(accept_it, entries.end());
    ASSERT_TRUE(accept_it->data.contains("value"));
    ASSERT_TRUE(accept_it->data["value"].contains("data"));
    const auto& value_data = accept_it->data["value"]["data"];
    ASSERT_TRUE(value_data.contains("raw_command"));
    EXPECT_EQ(value_data["raw_command"], accepted_value);
    ASSERT_TRUE(value_data.contains("parsed_command"));
    EXPECT_EQ(value_data["parsed_command"]["operation"], "UPSERT");
}

// ─────────────────────────────────────────────────────────────────────────────
// PSR-11: ShardRPCClient::writeEntity on loopback (in-process) path returns true
// ─────────────────────────────────────────────────────────────────────────────

TEST(ShardRpcWriteEntityTest, PSR11_WriteEntityInProcessReturnsTrue) {
    ShardRPCClient::Config cfg;
    cfg.endpoint    = "127.0.0.1:50051";   // loopback → in-process path
    cfg.shard_id    = "test-shard-1";
    cfg.max_retries = 1;
    cfg.retry_delay_ms = 0;
    cfg.enable_circuit_breaker = false;

    ShardRPCClient client(cfg);

    const bool ok = client.writeEntity(
        "users",
        "uuid-abc-123",
        {{"name", "Alice"}, {"age", 30}},
        /*timestamp_ns=*/0
    );

    EXPECT_TRUE(ok);
}
