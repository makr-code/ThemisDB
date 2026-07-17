#include <gtest/gtest.h>

#include "manifest_store.h"
#include "snapshot_update_worker.h"

#include <chrono>

using namespace themis::distributed_tensor;
using namespace std::chrono_literals;

namespace {

ArtifactManifest makeManifest() {
    ArtifactManifest manifest;
    manifest.artifact_id = "artifact-users";
    manifest.tensor_name = "users/embedding";
    manifest.kind = ArtifactKind::ADVISORY_SUMMARY;
    manifest.shard_id = 0;
    manifest.version = 1;
    manifest.created_at = std::chrono::system_clock::now() - 5s;
    manifest.integrity.crc32 = 0xABCD1234u;
    manifest.integrity.payload_bytes = 4096;
    manifest.residual = 0.10;
    manifest.rank_cap = 32;
    manifest.rank_status = 2;
    manifest.staleness_threshold_sec = 120;
    manifest.advisory_only = true;
    return manifest;
}

DeltaWindow makeWindow(std::initializer_list<DeltaMutationType> mutations,
                       uint32_t payload_size) {
    DeltaWindow window;
    window.artifact_id = "artifact-users";
    window.sequence_start = 1;
    window.sequence_end = static_cast<uint64_t>(mutations.size());
    window.extracted_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    uint64_t sequence = 1;
    for (const auto mutation : mutations) {
        DeltaLogEntry entry;
        entry.sequence_number = sequence++;
        entry.mutation_type = mutation;
        entry.affected_entity_id = "node-" + std::to_string(entry.sequence_number);
        entry.recorded_at_ms = window.extracted_at_ms;
        entry.source_transaction_id = "tx-" + std::to_string(entry.sequence_number);
        entry.shard_hint = "shard-a";
        entry.payload_size_bytes = payload_size;
        window.total_payload_size_bytes += payload_size;
        window.entries.push_back(entry);
    }

    return window;
}

TEST(SnapshotUpdateWorkerTest, PartialRefitPublishesManifestUpdate) {
    ManifestStore store;
    SnapshotBasedUpdateWorker worker(&store);
    ASSERT_TRUE(worker.start());

    const auto current_manifest = makeManifest();
    const auto delta_window = makeWindow(
        {DeltaMutationType::UPDATE, DeltaMutationType::UPDATE, DeltaMutationType::INSERT},
        256);

    UpdateMetrics metrics;
    const UpdateTask task{
        "artifact-users",
        delta_window,
        current_manifest,
        4096,
    };

    const auto decision = worker.processTask(task, metrics);
    EXPECT_EQ(decision, UpdateDecision::PARTIAL_REFIT);
    EXPECT_TRUE(metrics.success);

    const auto published = store.get("users/embedding", 0);
    ASSERT_TRUE(published.has_value());
    EXPECT_EQ(published->version, 2u);
    EXPECT_EQ(published->source_seq_end, delta_window.sequence_end);
    EXPECT_EQ(published->update_mode, UpdateMode::PARTIAL_REFIT);
    EXPECT_EQ(published->rebuild_state, RebuildState::PARTIAL_REFITTED);
}

TEST(SnapshotUpdateWorkerTest, RankCapBreachFallsBackToRebuild) {
    ManifestStore store;
    SnapshotBasedUpdateWorker worker(&store);
    ASSERT_TRUE(worker.start());

    auto current_manifest = makeManifest();
    current_manifest.rank_cap = 3;
    current_manifest.rank_status = 2;

    const auto delta_window = makeWindow(
        {DeltaMutationType::UPDATE, DeltaMutationType::UPDATE, DeltaMutationType::INSERT},
        256);

    UpdateMetrics metrics;
    const UpdateTask task{
        "artifact-users",
        delta_window,
        current_manifest,
        4096,
    };

    const auto decision = worker.processTask(task, metrics);
    EXPECT_EQ(decision, UpdateDecision::ERROR_FALLBACK_TO_REBUILD);
    EXPECT_TRUE(metrics.success);
    EXPECT_DOUBLE_EQ(metrics.resulting_residual, 0.0);

    const auto published = store.get("users/embedding", 0);
    ASSERT_TRUE(published.has_value());
    EXPECT_EQ(published->version, 2u);
    EXPECT_EQ(published->rebuild_state, RebuildState::REBUILT);
    EXPECT_EQ(published->update_mode, UpdateMode::REBUILD);
    EXPECT_EQ(published->rank_status, 0u);
}

}  // namespace
