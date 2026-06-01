/**
 * @file artifact_manifest_test.cc
 * @brief Contract tests for IManifestStore (sub-issue #5430).
 *
 * Validates factory construction, commit/lookup cycle, partial-loss reporting,
 * status updates, snapshot/restore round-trip, and listByStatus.
 * Production distributed consensus store is tracked in sub-issue #5430.
 */

#include "distributed_tensor/include/artifact_manifest.h"

#include <gtest/gtest.h>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

using namespace themis::distributed_tensor;

namespace {

ManifestEntry makeEntry(const std::string& artifact_id) {
    ManifestEntry e;
    e.artifact_id     = artifact_id;
    e.status          = ManifestEntryStatus::Pending;
    e.global_checksum = "abc123";
    e.schema_version  = 1;
    e.coordinator_node = "node-0";
    return e;
}

} // namespace

class ManifestStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        store_ = makeManifestStore();
        ASSERT_NE(store_, nullptr);
    }

    std::unique_ptr<IManifestStore> store_;
};

TEST_F(ManifestStoreTest, FactoryReturnsNonNull) {
    EXPECT_NE(store_, nullptr);
}

TEST_F(ManifestStoreTest, LookupMissingReturnsNullopt) {
    auto entry = store_->lookup("nonexistent");
    EXPECT_FALSE(entry.has_value());
}

TEST_F(ManifestStoreTest, CommitReturnsTrueAndCanLookup) {
    bool ok = store_->commit(makeEntry("art-1"));
    EXPECT_TRUE(ok);
    auto entry = store_->lookup("art-1");
    EXPECT_TRUE(entry.has_value());
    EXPECT_EQ(entry->artifact_id, "art-1");
}

TEST_F(ManifestStoreTest, CommitSetsCommittedAt) {
    store_->commit(makeEntry("art-2"));
    auto entry = store_->lookup("art-2");
    ASSERT_TRUE(entry.has_value());
    // committed_at should be non-epoch after commit.
    EXPECT_GT(entry->committed_at.time_since_epoch().count(), 0);
}

TEST_F(ManifestStoreTest, UpdateStatusChangesEntry) {
    store_->commit(makeEntry("art-3"));
    bool ok = store_->updateStatus("art-3", ManifestEntryStatus::Committed);
    EXPECT_TRUE(ok);
    auto entry = store_->lookup("art-3");
    ASSERT_TRUE(entry.has_value());
    EXPECT_EQ(entry->status, ManifestEntryStatus::Committed);
}

TEST_F(ManifestStoreTest, UpdateStatusOnUnknownArtifactReturnsFalse) {
    bool ok = store_->updateStatus("nonexistent", ManifestEntryStatus::Committed);
    EXPECT_FALSE(ok);
}

TEST_F(ManifestStoreTest, ReportPartialLossChangesStatus) {
    store_->commit(makeEntry("art-4"));
    store_->updateStatus("art-4", ManifestEntryStatus::Committed);
    bool ok = store_->reportPartialLoss("art-4", {"shard-1"});
    EXPECT_TRUE(ok);
    auto entry = store_->lookup("art-4");
    ASSERT_TRUE(entry.has_value());
    EXPECT_EQ(entry->status, ManifestEntryStatus::PartialLoss);
}

TEST_F(ManifestStoreTest, ListByStatusReturnsCommittedEntries) {
    ManifestEntry e = makeEntry("art-5");
    store_->commit(e);
    store_->updateStatus("art-5", ManifestEntryStatus::Committed);
    auto ids = store_->listByStatus(ManifestEntryStatus::Committed);
    bool found = false;
    for (const auto& id : ids) {
        if (id == "art-5") found = true;
    }
    EXPECT_TRUE(found);
}

TEST_F(ManifestStoreTest, SnapshotThenRestoreRoundTrip) {
    store_->commit(makeEntry("art-6"));
    ManifestSnapshot snap = store_->snapshot();
    EXPECT_FALSE(snap.snapshot_id.empty());
    EXPECT_FALSE(snap.entries.empty());

    auto fresh = makeManifestStore();
    bool ok = fresh->restore(snap);
    EXPECT_TRUE(ok);
    auto entry = fresh->lookup("art-6");
    EXPECT_TRUE(entry.has_value());
}

TEST_F(ManifestStoreTest, SnapshotEpochIsNonNegative) {
    ManifestSnapshot snap = store_->snapshot();
    EXPECT_GE(snap.epoch, 0u);
}
