// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_projects_workflows.cpp
 * @brief Google Benchmark coverage for project versioning workflows.
 *
 * Benchmarks use an isolated RocksDB instance per case and exercise the
 * production project services. They report measurements only; no release
 * threshold is asserted here.
 */

#include <benchmark/benchmark.h>

#include "projects/collaboration_manager.h"
#include "projects/project_diff.h"
#include "projects/project_template.h"
#include "projects/project_versioning.h"

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>

namespace {

using themis::RocksDBWrapper;
using namespace themis::projects;

class ProjectBenchmarkDb {
public:
    ProjectBenchmarkDb() {
        const auto nonce = std::chrono::steady_clock::now()
                               .time_since_epoch()
                               .count();
        path_ = std::filesystem::temp_directory_path() /
            ("themis_project_bench_" + std::to_string(nonce));
        std::error_code error;
        std::filesystem::remove_all(path_, error);

        RocksDBWrapper::Config config;
        config.db_path = path_.string();
        storage_ = std::make_shared<RocksDBWrapper>(config);
        ready_ = storage_->open();
    }

    ~ProjectBenchmarkDb() {
        storage_.reset();
        std::error_code error;
        std::filesystem::remove_all(path_, error);
    }

    bool ready() const noexcept { return ready_; }

    const std::shared_ptr<RocksDBWrapper>& storage() const noexcept {
        return storage_;
    }

    bool seedProject(
        const std::string& project_id,
        int64_t document_count,
        int64_t version) {
        for (int64_t index = 0; index < document_count; ++index) {
            const auto doc_id = "doc-" + std::to_string(index);
            const json document = {
                {"id", doc_id},
                {"title", "Document " + std::to_string(index)},
                {"metadata", {
                    {"version", version},
                    {"payload", std::string(64, static_cast<char>('a' + version))},
                }},
            };
            const auto key = "doc_proj:" + project_id + ":" + doc_id;
            if (!storage_->put(key, document.dump())) {
                return false;
            }
        }
        return true;
    }

    bool createSnapshot(
        ProjectVersioning& versioning,
        const std::string& project_id,
        SnapshotId& snapshot_id) {
        auto result = versioning.createSnapshot(project_id);
        const auto* id = std::get_if<SnapshotId>(&result);
        if (id == nullptr) {
            return false;
        }
        snapshot_id = *id;
        return true;
    }

private:
    std::filesystem::path path_;
    std::shared_ptr<RocksDBWrapper> storage_;
    bool ready_ = false;
};

void BM_ProjectVersioning_CreateSnapshot(benchmark::State& state) {
    ProjectBenchmarkDb db;
    const auto document_count = state.range(0);
    constexpr char kProjectId[] = "snapshot-create";
    if (!db.ready() || !db.seedProject(kProjectId, document_count, 0)) {
        state.SkipWithError("Unable to initialize project snapshot benchmark database");
        return;
    }

    ProjectVersioning versioning(db.storage());
    for (auto _ : state) {
        const auto result = versioning.createSnapshot(kProjectId);
        benchmark::DoNotOptimize(result);
        if (!std::holds_alternative<SnapshotId>(result)) {
            state.SkipWithError("ProjectVersioning::createSnapshot failed");
            return;
        }
    }
    state.SetItemsProcessed(state.iterations());
    state.counters["documents"] = static_cast<double>(document_count);
}
BENCHMARK(BM_ProjectVersioning_CreateSnapshot)->Arg(100)->Arg(1000);

void BM_ProjectVersioning_GetSnapshot(benchmark::State& state) {
    ProjectBenchmarkDb db;
    constexpr int64_t kDocumentCount = 1000;
    constexpr char kProjectId[] = "snapshot-get";
    if (!db.ready() || !db.seedProject(kProjectId, kDocumentCount, 0)) {
        state.SkipWithError("Unable to initialize project snapshot benchmark database");
        return;
    }

    ProjectVersioning versioning(db.storage());
    SnapshotId snapshot_id;
    if (!db.createSnapshot(versioning, kProjectId, snapshot_id)) {
        state.SkipWithError("Unable to create setup snapshot");
        return;
    }

    for (auto _ : state) {
        const auto snapshot = versioning.getSnapshot(snapshot_id);
        benchmark::DoNotOptimize(snapshot);
        if (!snapshot.has_value()) {
            state.SkipWithError("ProjectVersioning::getSnapshot failed");
            return;
        }
    }
    state.SetItemsProcessed(state.iterations());
    state.counters["documents"] = static_cast<double>(kDocumentCount);
}
BENCHMARK(BM_ProjectVersioning_GetSnapshot);

void BM_ProjectVersioning_RestoreSnapshot(benchmark::State& state) {
    ProjectBenchmarkDb db;
    constexpr int64_t kDocumentCount = 1000;
    constexpr char kProjectId[] = "snapshot-restore";
    if (!db.ready() || !db.seedProject(kProjectId, kDocumentCount, 0)) {
        state.SkipWithError("Unable to initialize project snapshot benchmark database");
        return;
    }

    ProjectVersioning versioning(db.storage());
    SnapshotId snapshot_id;
    if (!db.createSnapshot(versioning, kProjectId, snapshot_id)) {
        state.SkipWithError("Unable to create setup snapshot");
        return;
    }

    for (auto _ : state) {
        const auto status = versioning.restoreSnapshot(snapshot_id, "restore-target");
        if (!status.ok) {
            state.SkipWithError("ProjectVersioning::restoreSnapshot failed");
            return;
        }
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations());
    state.counters["documents"] = static_cast<double>(kDocumentCount);
}
BENCHMARK(BM_ProjectVersioning_RestoreSnapshot);

bool prepareDiffSnapshots(
    ProjectBenchmarkDb& db,
    ProjectVersioning& versioning,
    int64_t document_count,
    SnapshotId& ancestor,
    SnapshotId& ours,
    SnapshotId& theirs) {
    return db.seedProject("diff-ancestor", document_count, 0) &&
        db.seedProject("diff-ours", document_count, 1) &&
        db.seedProject("diff-theirs", document_count, 2) &&
        db.createSnapshot(versioning, "diff-ancestor", ancestor) &&
        db.createSnapshot(versioning, "diff-ours", ours) &&
        db.createSnapshot(versioning, "diff-theirs", theirs);
}

void BM_ProjectDiff_Snapshots(benchmark::State& state) {
    ProjectBenchmarkDb db;
    const auto document_count = state.range(0);
    if (!db.ready()) {
        state.SkipWithError("Unable to open project diff benchmark database");
        return;
    }

    ProjectVersioning versioning(db.storage());
    SnapshotId ancestor;
    SnapshotId ours;
    SnapshotId theirs;
    if (!prepareDiffSnapshots(
            db, versioning, document_count, ancestor, ours, theirs)) {
        state.SkipWithError("Unable to prepare project diff snapshots");
        return;
    }

    ProjectDiff diff(db.storage());
    for (auto _ : state) {
        const auto delta = diff.diff(ancestor, ours);
        benchmark::DoNotOptimize(delta);
        if (delta.empty()) {
            state.SkipWithError("ProjectDiff produced no changes for changed snapshots");
            return;
        }
    }
    state.SetItemsProcessed(state.iterations());
    state.counters["documents"] = static_cast<double>(document_count);
}
BENCHMARK(BM_ProjectDiff_Snapshots)->Arg(100)->Arg(1000);

void BM_ProjectMerge_ThreeWay(benchmark::State& state) {
    ProjectBenchmarkDb db;
    constexpr int64_t kDocumentCount = 1000;
    if (!db.ready()) {
        state.SkipWithError("Unable to open project merge benchmark database");
        return;
    }

    ProjectVersioning versioning(db.storage());
    SnapshotId ancestor;
    SnapshotId ours;
    SnapshotId theirs;
    if (!prepareDiffSnapshots(
            db, versioning, kDocumentCount, ancestor, ours, theirs)) {
        state.SkipWithError("Unable to prepare project merge snapshots");
        return;
    }

    ProjectMerge merge(db.storage());
    for (auto _ : state) {
        const auto result = merge.merge(ancestor, ours, theirs);
        benchmark::DoNotOptimize(result);
        if (result.ok || result.conflicts.empty()) {
            state.SkipWithError("Three-way merge failed to detect expected conflicts");
            return;
        }
    }
    state.SetItemsProcessed(state.iterations());
    state.counters["documents"] = static_cast<double>(kDocumentCount);
}
BENCHMARK(BM_ProjectMerge_ThreeWay);

void BM_ProjectTemplate_Instantiate(benchmark::State& state) {
    ProjectBenchmarkDb db;
    if (!db.ready()) {
        state.SkipWithError("Unable to open project template benchmark database");
        return;
    }

    ProjectTemplate project_template(db.storage());
    TemplateOptions options;
    options.project_name = "Benchmark project";
    for (auto _ : state) {
        const auto result =
            project_template.instantiate(BuiltinTemplate::WEB_APPLICATION, options);
        benchmark::DoNotOptimize(result);
        if (!result.ok) {
            state.SkipWithError("ProjectTemplate::instantiate failed");
            return;
        }
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ProjectTemplate_Instantiate);

void BM_CollaborationManager_LockCycle(benchmark::State& state) {
    ProjectBenchmarkDb db;
    if (!db.ready()) {
        state.SkipWithError("Unable to open collaboration benchmark database");
        return;
    }

    CollaborationManager collaboration(db.storage());
    for (auto _ : state) {
        const auto locked =
            collaboration.lockObject("benchmark-project", "document", "benchmark-user");
        if (!locked.ok) {
            state.SkipWithError("CollaborationManager::lockObject failed");
            return;
        }
        const auto unlocked =
            collaboration.unlockObject("benchmark-project", "document", "benchmark-user");
        if (!unlocked.ok) {
            state.SkipWithError("CollaborationManager::unlockObject failed");
            return;
        }
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CollaborationManager_LockCycle);

void BM_CollaborationManager_ChangeNotification(benchmark::State& state) {
    ProjectBenchmarkDb db;
    if (!db.ready()) {
        state.SkipWithError("Unable to open collaboration benchmark database");
        return;
    }

    CollaborationManager collaboration(db.storage());
    Change change;
    change.project_id = "benchmark-project";
    change.object_name = "document";
    change.field_path = "/metadata/title";
    change.old_value = "before";
    change.new_value = "after";
    change.actor = "benchmark-user";
    for (auto _ : state) {
        change.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        collaboration.notifyChange(change);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CollaborationManager_ChangeNotification);

}  // namespace

BENCHMARK_MAIN();
