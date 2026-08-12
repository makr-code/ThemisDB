#include <gtest/gtest.h>
#include "utils/saga_logger.h"

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using namespace themis::utils;
namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Test fixture — writes a small synthetic WAL, then exercises the compactor.
// ---------------------------------------------------------------------------

class SAGACompactorTest : public ::testing::Test {
protected:
    static constexpr const char* kWalPath = "/tmp/test_saga_compactor_wal.jsonl";
    static constexpr const char* kSigPath = "/tmp/test_saga_compactor_sig.jsonl";

    SAGALoggerConfig cfg;

    void SetUp() override {
        cfg.log_path       = kWalPath;
        cfg.signature_path = kSigPath;
        // Remove any leftover files.
        fs::remove(kWalPath);
        fs::remove(kSigPath);
        fs::remove(std::string(kWalPath) + ".archive.jsonl");
    }

    void TearDown() override {
        fs::remove(kWalPath);
        fs::remove(kSigPath);
        fs::remove(std::string(kWalPath) + ".archive.jsonl");
        fs::remove(std::string(kWalPath) + ".compact.tmp");
    }

    // Helper: write a raw JSONL line to the WAL.
    void writeWalLine(const nlohmann::json& j) {
        std::ofstream ofs(kWalPath, std::ios::app);
        ofs << j.dump() << '\n';
    }
};

// ============================================================================
// SAGALogCompactor
// ============================================================================

TEST_F(SAGACompactorTest, CompactOnNonExistentWalReturnsZero) {
    SAGALogCompactor compactor(cfg);
    EXPECT_EQ(compactor.compact("saga-999"), 0u);
}

TEST_F(SAGACompactorTest, CompactArchivesCompletedEntries) {
    // Write 3 entries: 2 completed (saga_id < "saga-003"), 1 still pending.
    writeWalLine({{"saga_id","saga-001"},{"status","success"},{"step_name","s1"}});
    writeWalLine({{"saga_id","saga-002"},{"status","success"},{"step_name","s2"}});
    writeWalLine({{"saga_id","saga-004"},{"status","pending"},{"step_name","s3"}});

    SAGALogCompactor compactor(cfg);
    size_t archived = compactor.compact("saga-003");

    EXPECT_EQ(archived, 2u);
    EXPECT_TRUE(fs::exists(compactor.archivePath()));

    // The WAL must only contain the pending entry now.
    std::ifstream ifs(kWalPath);
    std::string line;
    size_t remaining = 0;
    while (std::getline(ifs, line)) {
        if (!line.empty()) ++remaining;
    }
    EXPECT_EQ(remaining, 1u);
}

TEST_F(SAGACompactorTest, CompactWithNothingToArchiveIsNoOp) {
    writeWalLine({{"saga_id","saga-010"},{"status","pending"},{"step_name","s1"}});

    SAGALogCompactor compactor(cfg);
    size_t archived = compactor.compact("saga-005"); // all entries are beyond threshold
    EXPECT_EQ(archived, 0u);
}

TEST_F(SAGACompactorTest, ArchivePathIsCorrect) {
    SAGALogCompactor compactor(cfg);
    EXPECT_EQ(compactor.archivePath(), std::string(kWalPath) + ".archive.jsonl");
}

// ============================================================================
// SAGALogReplayer
// ============================================================================

TEST_F(SAGACompactorTest, ReplayOnNonExistentWalReturnsZero) {
    SAGALogReplayer replayer(cfg);
    size_t count = replayer.replay_incomplete([](const SAGAStep&){});
    EXPECT_EQ(count, 0u);
}

TEST_F(SAGACompactorTest, ReplayCallsHandlerForPendingCompensations) {
    // Write a mix: one forward-success, one compensate-pending, one compensate-success.
    writeWalLine({{"saga_id","s1"},{"action","forward"},{"status","success"},{"step_name","A"}});
    writeWalLine({{"saga_id","s2"},{"action","compensate"},{"status","pending"},{"step_name","B"}});
    writeWalLine({{"saga_id","s3"},{"action","compensate"},{"status","success"},{"step_name","C"}});

    SAGALogReplayer replayer(cfg);

    std::vector<std::string> replayed_ids;
    size_t count = replayer.replay_incomplete([&](const SAGAStep& step){
        replayed_ids.push_back(step.saga_id);
    });

    EXPECT_EQ(count, 1u);
    ASSERT_EQ(replayed_ids.size(), 1u);
    EXPECT_EQ(replayed_ids[0], "s2");
}

TEST_F(SAGACompactorTest, ReplayReconstructsStepFields) {
    writeWalLine({
        {"saga_id","txn-42"},
        {"step_name","payment"},
        {"action","compensate"},
        {"status","pending"},
        {"entity_id","order-7"},
        {"payload",{{"amount",100}}}
    });

    SAGALogReplayer replayer(cfg);
    SAGAStep captured;
    replayer.replay_incomplete([&](const SAGAStep& step){ captured = step; });

    EXPECT_EQ(captured.saga_id,   "txn-42");
    EXPECT_EQ(captured.step_name, "payment");
    EXPECT_EQ(captured.entity_id, "order-7");
    EXPECT_EQ(captured.action,    "compensate");
    EXPECT_EQ(captured.status,    "pending");
}
