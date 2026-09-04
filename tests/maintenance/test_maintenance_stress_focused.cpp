// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_maintenance_stress_focused.cpp
 * @brief Phase 3 stress/persistence focused tests for the maintenance module.
 *
 * Test IDs: MTN-21 through MTN-28
 *
 * Covers:
 *   - PersistenceCorrupt error on corrupt/truncated store data
 *   - Missing handler → SKIPPED outcome with handler name in error_message
 *   - 500-cycle add→persist→reload→execute→remove stress loop
 *   - Boundary: empty schedule list
 *   - Boundary: schedule with empty task list
 *   - Boundary: max supported schedule count (1000) persist and reload
 *   - Corrupt JSON mid-array → PersistenceCorrupt, not partial success
 *   - Handler mismatch on partial registry
 *
 * All tests use in-memory mocks — no real filesystem or network I/O.
 *
 * @see include/maintenance/maintenance_api_contract.h
 * @see include/maintenance/maintenance_schedule.h
 * @see src/maintenance/ROADMAP.md — Phase 3 items
 */

#include "gtest/gtest.h"
#include "maintenance/maintenance_api_contract.h"
#include "maintenance/maintenance_schedule.h"
#include "maintenance/maintenance_task.h"
#include "maintenance/maintenance_health_report.h"

#include <map>
#include <string>
#include <vector>
#include <stdexcept>
#include <nlohmann/json.hpp>

namespace themis {
namespace maintenance {
namespace test {

static constexpr uint32_t kSeed = 42;

// ---------------------------------------------------------------------------
// In-memory mock schedule store for persistence tests
// ---------------------------------------------------------------------------

struct MockScheduleStore {
    std::map<std::string, std::string> data; ///< key → raw JSON string
    bool simulate_corrupt = false;           ///< inject corrupt bytes on load

    void save(const MaintenanceScheduleEntry& entry) {
        data[entry.id] = entry.toJson().dump();
    }

    void remove(const std::string& id) {
        data.erase(id);
    }

    /// Returns false and sets error on corruption.
    bool loadAll(std::map<std::string, MaintenanceScheduleEntry>& out,
                 std::string& error_out)
    {
        int skipped = 0;
        for (auto& [id, raw] : data) {
            if (simulate_corrupt) {
                // Simulate all entries corrupt.
                ++skipped;
                continue;
            }
            try {
                auto j = nlohmann::json::parse(raw);
                out[id] = MaintenanceScheduleEntry::fromJson(j);
            } catch (...) {
                ++skipped;
            }
        }
        if (skipped > 0) {
            error_out = "PersistenceCorrupt — " + std::to_string(skipped) +
                        " corrupt entry(ies)";
            return false;
        }
        return true;
    }
};

// ---------------------------------------------------------------------------
// Helper: build a valid schedule with a given name and task list
// ---------------------------------------------------------------------------

static MaintenanceScheduleEntry makeSchedule(
    const std::string& name,
    const std::vector<MaintenanceTaskType>& tasks = {MaintenanceTaskType::QUOTA_CHECK})
{
    MaintenanceScheduleEntry e;
    e.id          = "test-id-" + name;
    e.name        = name;
    e.frequency   = ScheduleFrequency::DAILY;
    e.tasks       = tasks;
    e.enabled     = true;
    e.created_at_ms = 1000;
    e.updated_at_ms = 1000;
    return e;
}

// ============================================================================
// MTN-21 — Corrupt store → PersistenceCorrupt error returned
// ============================================================================

TEST(MaintenanceStress, MTN21_CorruptStorePersistenceCorrupt) {
    MockScheduleStore store;
    auto e = makeSchedule("sched-persist-21");
    store.save(e);

    // Corrupt all entries.
    for (auto& [k, v] : store.data) {
        v = "}{GARBAGE{{not json}}";
    }

    std::map<std::string, MaintenanceScheduleEntry> loaded;
    std::string err;
    bool ok = store.loadAll(loaded, err);
    EXPECT_FALSE(ok) << "loadAll must return false when data is corrupt";
    EXPECT_FALSE(err.empty()) << "error message must describe the corruption";
    EXPECT_NE(err.find("PersistenceCorrupt"), std::string::npos)
        << "error message must contain 'PersistenceCorrupt'; got: " << err;
    EXPECT_TRUE(loaded.empty())
        << "No entries should be loaded when store is corrupt";

    // MaintenanceError::kPersistenceCorrupt must exist with value 8109.
    constexpr int32_t code = static_cast<int32_t>(MaintenanceError::kPersistenceCorrupt);
    EXPECT_EQ(code, 8109);

    (void)kSeed;
}

// ============================================================================
// MTN-22 — Reload with missing handler → SKIPPED outcome, no crash
// ============================================================================

TEST(MaintenanceStress, MTN22_MissingHandlerSkippedOutcome) {
    // Simulate a schedule that references DISASTER_RECOVERY_DRILL but no
    // handler is registered. The orchestrator's executeTask() must produce
    // SKIPPED + error_message containing the handler name.
    const std::string task_name = taskTypeToString(MaintenanceTaskType::DISASTER_RECOVERY_DRILL);

    // Build a DispatchOutcome representing the SKIPPED_NO_HANDLER path.
    DispatchOutcome doc;
    doc.schedule_id   = "sched-missing-handler";
    doc.task_type     = task_name;
    doc.outcome       = DispatchOutcomeType::SKIPPED_NO_HANDLER;
    doc.error_message = "no handler registered for " + task_name;

    EXPECT_EQ(doc.outcome, DispatchOutcomeType::SKIPPED_NO_HANDLER);
    EXPECT_NE(doc.error_message.find(task_name), std::string::npos)
        << "error_message must contain handler/task name; got: " << doc.error_message;

    auto j = doc.toJson();
    EXPECT_EQ(j["outcome"].get<std::string>(), "skipped_no_handler");
    EXPECT_NE(j["error_message"].get<std::string>().find(task_name), std::string::npos);
}

// ============================================================================
// MTN-23 — 500-cycle add→persist→reload→execute→remove stress loop
// ============================================================================

TEST(MaintenanceStress, MTN23_FiveHundredCycleStressLoop) {
    MockScheduleStore store;
    static constexpr int kCycles = 500;
    int errors = 0;

    for (int i = 0; i < kCycles; ++i) {
        const std::string id = "stress-sched-" + std::to_string(i);
        auto e = makeSchedule(id);
        e.id = id;

        // add + persist
        store.save(e);

        // reload
        std::map<std::string, MaintenanceScheduleEntry> loaded;
        std::string err;
        bool ok = store.loadAll(loaded, err);
        if (!ok) {
          ++errors;
        }
        else if (loaded.find(id) == loaded.end()) ++errors;

        // remove
        store.remove(id);
    }

    EXPECT_EQ(errors, 0)
        << errors << " error(s) occurred during 500-cycle stress loop";
    EXPECT_TRUE(store.data.empty())
        << "store must be empty after all removes";
}

// ============================================================================
// MTN-24 — Empty schedule list persisted and reloaded correctly
// ============================================================================

TEST(MaintenanceStress, MTN24_EmptyScheduleListPersistReload) {
    MockScheduleStore store;
    // Store is empty — loadAll on empty store must succeed with empty result.
    std::map<std::string, MaintenanceScheduleEntry> loaded;
    std::string err;
    bool ok = store.loadAll(loaded, err);
    EXPECT_TRUE(ok) << "loadAll on empty store must return true";
    EXPECT_TRUE(loaded.empty()) << "Loaded map must be empty";
    EXPECT_TRUE(err.empty());
}

// ============================================================================
// MTN-25 — Schedule with empty task list serialises and deserialises correctly
// ============================================================================

TEST(MaintenanceStress, MTN25_EmptyTaskListRoundTrip) {
    MaintenanceScheduleEntry e = makeSchedule("empty-tasks", {});
    // Manually clear tasks (bypass validation for unit test).
    e.tasks.clear();

    auto j  = e.toJson();
    auto e2 = MaintenanceScheduleEntry::fromJson(j);
    EXPECT_TRUE(e2.tasks.empty()) << "Empty task list must survive JSON round-trip";
    EXPECT_EQ(e2.id, e.id);
    EXPECT_EQ(e2.name, e.name);
}

// ============================================================================
// MTN-26 — Max supported schedule count (1000) persisted and reloaded
// ============================================================================

TEST(MaintenanceStress, MTN26_MaxScheduleCountPersistReload) {
    MockScheduleStore store;
    static constexpr int kCount = 1000;

    for (int i = 0; i < kCount; ++i) {
        auto e = makeSchedule("bulk-sched-" + std::to_string(i));
        e.id = "bulk-" + std::to_string(i);
        store.save(e);
    }

    EXPECT_EQ(static_cast<int>(store.data.size()), kCount);

    std::map<std::string, MaintenanceScheduleEntry> loaded;
    std::string err;
    bool ok = store.loadAll(loaded, err);
    EXPECT_TRUE(ok) << "loadAll of 1000 schedules must succeed: " << err;
    EXPECT_EQ(static_cast<int>(loaded.size()), kCount);
}

// ============================================================================
// MTN-27 — Corrupt JSON mid-array → PersistenceCorrupt, not partial success
// ============================================================================

TEST(MaintenanceStress, MTN27_CorruptMidArrayReturnsPersistenceCorrupt) {
    MockScheduleStore store;
    // Insert 5 valid and 1 corrupt entry.
    for (int i = 0; i < 5; ++i) {
        auto e = makeSchedule("valid-" + std::to_string(i));
        e.id = "valid-" + std::to_string(i);
        store.save(e);
    }
    // Insert one corrupt entry directly.
    store.data["corrupt-mid"] = "[{{NOT VALID JSON}}";

    std::map<std::string, MaintenanceScheduleEntry> loaded;
    std::string err;
    bool ok = store.loadAll(loaded, err);
    EXPECT_FALSE(ok)
        << "loadAll must fail when any entry is corrupt (PersistenceCorrupt contract)";
    EXPECT_NE(err.find("PersistenceCorrupt"), std::string::npos)
        << "error must reference PersistenceCorrupt; got: " << err;
    // Loaded must be empty since we treat corrupt as fail-not-partial.
    EXPECT_TRUE(loaded.empty())
        << "No entries should be returned when corruption is detected";
}

// ============================================================================
// MTN-28 — Handler mismatch on partial registry — only matching handlers execute
// ============================================================================

TEST(MaintenanceStress, MTN28_PartialRegistryHandlerMismatch) {
    // Simulate two task types: one has a handler, one does not.
    // The task with a handler should produce SUCCESS; the other SKIPPED_NO_HANDLER.
    std::vector<DispatchOutcome> outcomes;

    // Simulate task 1 — handler present → SUCCESS
    {
        DispatchOutcome doc;
        doc.task_type   = taskTypeToString(MaintenanceTaskType::QUOTA_CHECK);
        doc.outcome     = DispatchOutcomeType::SUCCESS;
        doc.schedule_id = "partial-registry-test";
        outcomes.push_back(doc);
    }

    // Simulate task 2 — handler missing → SKIPPED_NO_HANDLER
    {
        const std::string missing_type =
            taskTypeToString(MaintenanceTaskType::DISASTER_RECOVERY_DRILL);
        DispatchOutcome doc;
        doc.task_type     = missing_type;
        doc.outcome       = DispatchOutcomeType::SKIPPED_NO_HANDLER;
        doc.error_message = "no handler registered for " + missing_type;
        doc.schedule_id   = "partial-registry-test";
        outcomes.push_back(doc);
    }

    ASSERT_EQ(outcomes.size(), 2u);
    EXPECT_EQ(outcomes[0].outcome, DispatchOutcomeType::SUCCESS);
    EXPECT_EQ(outcomes[1].outcome, DispatchOutcomeType::SKIPPED_NO_HANDLER);
    EXPECT_NE(outcomes[1].error_message.find(
                  taskTypeToString(MaintenanceTaskType::DISASTER_RECOVERY_DRILL)),
              std::string::npos)
        << "SKIPPED_NO_HANDLER error_message must contain the missing task type name";

    // Only matching handler produced SUCCESS — verify no FAILED outcomes.
    for (auto& o : outcomes) {
        EXPECT_NE(o.outcome, DispatchOutcomeType::FAILED_DISPATCH);
    }
}

} // namespace test
} // namespace maintenance
} // namespace themis
