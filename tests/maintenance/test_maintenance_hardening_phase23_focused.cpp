// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_maintenance_hardening_phase23_focused.cpp
 * @brief Phase 2/3 hardening focused tests for the maintenance module.
 *
 * Test IDs: MTN-09 through MTN-16
 *
 * Covers:
 *   - Phase 2: schedule-entry serialization round-trips, frequency/cron mapping,
 *     task-type string dispatch, job-state string dispatch, default field constraints.
 *   - Phase 3: fail-safe field defaults, DAG dependency serialization round-trips,
 *     diagnostics structure (non-empty error messages on defined failure paths).
 *
 * No file I/O, no network, no real orchestrator instantiation — deterministic only.
 *
 * @see include/maintenance/maintenance_api_contract.h
 * @see include/maintenance/maintenance_schedule.h
 * @see include/maintenance/maintenance_task.h
 * @see src/maintenance/ROADMAP.md — Phase 2 and Phase 3 items
 */

#include "gtest/gtest.h"
#include "maintenance/maintenance_api_contract.h"
#include "maintenance/maintenance_schedule.h"
#include "maintenance/maintenance_task.h"

#include <cstdint>
#include <set>
#include <string>
#include <type_traits>
#include <vector>

namespace themis {
namespace maintenance {
namespace test {

// Canonical PRNG seed (deterministic, release-pinned).
static constexpr uint32_t kSeed = 42;

// ============================================================================
// MTN-09 — frequencyToCron produces non-empty cron expressions for all presets
// ============================================================================

/**
 * @brief Validates that every non-CUSTOM ScheduleFrequency maps to a
 * non-empty, syntactically plausible cron expression.
 *
 * Covers Phase 2 item: "align registry and execution behavior to bounded
 * runtime contracts" — the cron derivation is part of the schedule
 * activation contract.
 */
TEST(MaintenanceHardeningPhase23, MTN09_FrequencyToCronNonEmpty) {
    const ScheduleFrequency freqs[] = {
        ScheduleFrequency::DAILY,
        ScheduleFrequency::WEEKLY,
        ScheduleFrequency::MONTHLY,
        ScheduleFrequency::QUARTERLY,
    };
    for (auto f : freqs) {
        std::string cron = frequencyToCron(f, 2);
        EXPECT_FALSE(cron.empty())
            << "frequencyToCron returned empty for frequency="
            << frequencyToString(f);
        // A minimal cron expression has at least 9 characters ("0 2 * * *")
        EXPECT_GE(cron.size(), 9u)
            << "cron '" << cron << "' too short for frequency="
            << frequencyToString(f);
        // Must contain exactly four spaces (five fields).
        int spaces = 0;
        for (char c : cron) { if (c == ' ') ++spaces; }
        EXPECT_EQ(spaces, 4)
            << "cron '" << cron << "' does not have 5 fields; frequency="
            << frequencyToString(f);
    }
    (void)kSeed;
}

// ============================================================================
// MTN-10 — MaintenanceScheduleEntry toJson/fromJson round-trip
// ============================================================================

/**
 * @brief Validates that all key fields of MaintenanceScheduleEntry survive a
 * JSON serialization → deserialization round-trip without data loss.
 *
 * Covers Phase 2 item: "complete hardening for orchestrator and
 * schedule-store internals" — the schedule store relies on toJson/fromJson
 * for persistence.
 */
TEST(MaintenanceHardeningPhase23, MTN10_ScheduleEntryRoundTrip) {
    MaintenanceScheduleEntry orig;
    orig.id              = "rt-id-42";
    orig.name            = "Round-trip schedule";
    orig.description     = "Phase 2 persistence round-trip test";
    orig.tenant_id       = "tenant-42";
    orig.frequency       = ScheduleFrequency::WEEKLY;
    orig.cron_expression = "0 3 * * 0";
    orig.tasks           = { MaintenanceTaskType::MVCC_CLEANUP,
                              MaintenanceTaskType::CONSISTENCY_CHECK };
    orig.enabled              = true;
    orig.enforce_window       = true;
    orig.window_start_hour    = 3;
    orig.window_end_hour      = 7;
    orig.halt_on_task_failure = true;
    orig.lock_ttl_ms          = 5000;
    orig.created_at_ms        = 1000000;
    orig.updated_at_ms        = 2000000;
    orig.created_by           = "operator-1";
    orig.updated_by           = "operator-2";
    orig.last_run_ms          = 3000000;
    orig.next_run_ms          = 4000000;
    orig.last_run_state       = "success";
    orig.last_job_id          = "job-99";

    auto json_val = orig.toJson();
    auto restored = MaintenanceScheduleEntry::fromJson(json_val);

    EXPECT_EQ(restored.id,              orig.id);
    EXPECT_EQ(restored.name,            orig.name);
    EXPECT_EQ(restored.description,     orig.description);
    EXPECT_EQ(restored.tenant_id,       orig.tenant_id);
    EXPECT_EQ(restored.frequency,       orig.frequency);
    EXPECT_EQ(restored.cron_expression, orig.cron_expression);
    ASSERT_EQ(restored.tasks.size(),    orig.tasks.size());
    EXPECT_EQ(restored.tasks[0],        orig.tasks[0]);
    EXPECT_EQ(restored.tasks[1],        orig.tasks[1]);
    EXPECT_EQ(restored.enabled,              orig.enabled);
    EXPECT_EQ(restored.enforce_window,       orig.enforce_window);
    EXPECT_EQ(restored.window_start_hour,    orig.window_start_hour);
    EXPECT_EQ(restored.window_end_hour,      orig.window_end_hour);
    EXPECT_EQ(restored.halt_on_task_failure, orig.halt_on_task_failure);
    EXPECT_EQ(restored.lock_ttl_ms,          orig.lock_ttl_ms);
    EXPECT_EQ(restored.created_by,      orig.created_by);
    EXPECT_EQ(restored.updated_by,      orig.updated_by);
    // Timestamp and runtime state fields must survive persistence reload.
    EXPECT_EQ(restored.created_at_ms,   orig.created_at_ms);
    EXPECT_EQ(restored.updated_at_ms,   orig.updated_at_ms);
    EXPECT_EQ(restored.last_run_ms,     orig.last_run_ms);
    EXPECT_EQ(restored.next_run_ms,     orig.next_run_ms);
    EXPECT_EQ(restored.last_run_state,  orig.last_run_state);
    EXPECT_EQ(restored.last_job_id,     orig.last_job_id);
}

// ============================================================================
// MTN-11 — MaintenanceTaskDependency toJson/fromJson round-trip
// ============================================================================

/**
 * @brief Validates that MaintenanceTaskDependency survives a JSON
 * round-trip preserving all fields.
 *
 * Covers Phase 3 item: "unify diagnostics across scheduling/persistence/
 * execution incidents" — the dependency structure is part of the schedule's
 * durable state.
 */
TEST(MaintenanceHardeningPhase23, MTN11_TaskDependencyRoundTrip) {
    MaintenanceTaskDependency dep;
    dep.task_type  = MaintenanceTaskType::STORAGE_COMPACTION;
    dep.depends_on = { MaintenanceTaskType::MVCC_CLEANUP,
                       MaintenanceTaskType::CONSISTENCY_CHECK };

    auto json_val = dep.toJson();
    auto restored = MaintenanceTaskDependency::fromJson(json_val);

    EXPECT_EQ(restored.task_type,       dep.task_type);
    ASSERT_EQ(restored.depends_on.size(), dep.depends_on.size());
    EXPECT_EQ(restored.depends_on[0],   dep.depends_on[0]);
    EXPECT_EQ(restored.depends_on[1],   dep.depends_on[1]);
}

// ============================================================================
// MTN-12 — taskTypeToString/taskTypeFromString round-trip (all task types)
// ============================================================================

/**
 * @brief Validates that every MaintenanceTaskType survives a
 * string ↔ enum round-trip without loss.
 *
 * Covers Phase 2 item: "align registry and execution behavior to bounded
 * runtime contracts" — task type dispatch relies on deterministic string
 * conversion.
 */
TEST(MaintenanceHardeningPhase23, MTN12_TaskTypeStringRoundTrip) {
    const MaintenanceTaskType all_types[] = {
        MaintenanceTaskType::METRICS_COLLECTION,
        MaintenanceTaskType::FRAGMENTATION_MONITORING,
        MaintenanceTaskType::QUOTA_CHECK,
        MaintenanceTaskType::CONSISTENCY_CHECK,
        MaintenanceTaskType::REPLICA_VALIDATION,
        MaintenanceTaskType::PERFORMANCE_ANALYSIS,
        MaintenanceTaskType::MVCC_CLEANUP,
        MaintenanceTaskType::FULL_CHECKDB,
        MaintenanceTaskType::BACKUP_VERIFICATION,
        MaintenanceTaskType::CAPACITY_TREND_ANALYSIS,
        MaintenanceTaskType::INDEX_FRAGMENTATION_REPORT,
        MaintenanceTaskType::DISASTER_RECOVERY_DRILL,
        MaintenanceTaskType::BASELINE_UPDATE,
        MaintenanceTaskType::INDEX_REBUILD,
        MaintenanceTaskType::INDEX_REORGANIZE,
        MaintenanceTaskType::STATISTICS_UPDATE,
        MaintenanceTaskType::STORAGE_COMPACTION,
        MaintenanceTaskType::ORPHAN_CLEANUP,
        MaintenanceTaskType::VECTOR_REINDEX,
    };

    std::set<std::string> seen_strings;
    for (auto t : all_types) {
        const std::string s = taskTypeToString(t);
        EXPECT_NE(s, "unknown") << "Unmapped task type enum value";
        EXPECT_FALSE(s.empty());
        EXPECT_TRUE(seen_strings.insert(s).second)
            << "Duplicate task type string: " << s;

        // Round-trip: string → enum → string must be stable.
        auto restored = taskTypeFromString(s);
        EXPECT_EQ(taskTypeToString(restored), s)
            << "Round-trip failed for task type string '" << s << "'";
    }
}

// ============================================================================
// MTN-13 — jobStateToString coverage (all job states)
// ============================================================================

/**
 * @brief Validates that every MaintenanceJobState maps to a non-empty,
 * distinct string (full coverage of the operator-visible diagnostics path).
 *
 * Covers Phase 3 item: "unify diagnostics across scheduling/persistence/
 * execution incidents" — job state strings appear in operator logs and
 * REST responses.
 */
TEST(MaintenanceHardeningPhase23, MTN13_JobStateStringCoverage) {
    const MaintenanceJobState all_states[] = {
        MaintenanceJobState::PENDING,
        MaintenanceJobState::RUNNING,
        MaintenanceJobState::SUCCEEDED,
        MaintenanceJobState::FAILED,
        MaintenanceJobState::CANCELLED,
        MaintenanceJobState::SKIPPED,
    };

    std::set<std::string> seen;
    for (auto s : all_states) {
        const std::string str = jobStateToString(s);
        EXPECT_FALSE(str.empty());
        EXPECT_NE(str, "unknown")
            << "Unmapped job state enum value";
        EXPECT_TRUE(seen.insert(str).second)
            << "Duplicate job state string: " << str;
    }
    EXPECT_EQ(seen.size(), 6u);
}

// ============================================================================
// MTN-14 — MaintenanceScheduleEntry default field values (bounded/observable)
// ============================================================================

/**
 * @brief Validates that a default-constructed MaintenanceScheduleEntry has
 * well-defined, bounded default values consistent with the persistence and
 * execution contracts.
 *
 * Covers Phase 2 item: "complete hardening for orchestrator and schedule-store
 * internals" — default-constructed entries must never silently submit
 * invalid schedules.
 */
TEST(MaintenanceHardeningPhase23, MTN14_ScheduleEntryDefaults) {
    MaintenanceScheduleEntry e;
    EXPECT_TRUE(e.id.empty());
    EXPECT_TRUE(e.name.empty());
    EXPECT_TRUE(e.tenant_id.empty());
    EXPECT_EQ(e.frequency,         ScheduleFrequency::DAILY);
    EXPECT_TRUE(e.tasks.empty());
    EXPECT_TRUE(e.task_dependencies.empty());
    // Window defaults must be in valid [0, 23] range.
    EXPECT_GE(e.window_start_hour, 0);
    EXPECT_LE(e.window_start_hour, 23);
    EXPECT_GE(e.window_end_hour,   0);
    EXPECT_LE(e.window_end_hour,   23);
    // Timing defaults: no historical run state.
    EXPECT_EQ(e.last_run_ms,  0);
    EXPECT_EQ(e.next_run_ms,  0);
    EXPECT_TRUE(e.last_run_state.empty());
    EXPECT_TRUE(e.last_job_id.empty());
    // Lock TTL default: 0 means auto-computed by orchestrator.
    EXPECT_EQ(e.lock_ttl_ms, 0);
}

// ============================================================================
// MTN-15 — halt_on_task_failure field persists through JSON round-trip
// ============================================================================

/**
 * @brief Validates that the halt_on_task_failure field survives serialization
 * so that the fail-safe execution mode is preserved on persistence reload.
 *
 * Covers Phase 3 item: "standardize fail-safe behavior for missing handlers
 * and invalid persisted schedules."
 */
TEST(MaintenanceHardeningPhase23, MTN15_HaltOnTaskFailureRoundTrip) {
    // Test both true and false variants.
    for (bool halt : {true, false}) {
        MaintenanceScheduleEntry e;
        e.name               = "halt-test";
        e.tasks              = { MaintenanceTaskType::METRICS_COLLECTION };
        e.halt_on_task_failure = halt;

        auto restored = MaintenanceScheduleEntry::fromJson(e.toJson());
        EXPECT_EQ(restored.halt_on_task_failure, halt)
            << "halt_on_task_failure=" << halt << " not preserved by JSON round-trip";
    }
}

// ============================================================================
// MTN-16 — Task dependency list embedded in schedule survives round-trip
// ============================================================================

/**
 * @brief Validates that a MaintenanceScheduleEntry with a non-empty
 * task_dependencies list serializes and deserializes correctly.
 *
 * Covers Phase 3 item: "standardize fail-safe behavior for missing handlers
 * and invalid persisted schedules" — a corrupt dependency list after reload
 * would produce an unexpected execution order or a cycle detection error.
 */
TEST(MaintenanceHardeningPhase23, MTN16_ScheduleWithDependenciesRoundTrip) {
    MaintenanceScheduleEntry e;
    e.id   = "dag-sched-42";
    e.name = "DAG schedule";
    e.tasks = {
        MaintenanceTaskType::MVCC_CLEANUP,
        MaintenanceTaskType::STORAGE_COMPACTION,
        MaintenanceTaskType::CONSISTENCY_CHECK,
    };

    // STORAGE_COMPACTION depends on MVCC_CLEANUP;
    // CONSISTENCY_CHECK depends on STORAGE_COMPACTION.
    {
        MaintenanceTaskDependency d1;
        d1.task_type  = MaintenanceTaskType::STORAGE_COMPACTION;
        d1.depends_on = { MaintenanceTaskType::MVCC_CLEANUP };
        e.task_dependencies.push_back(d1);
    }
    {
        MaintenanceTaskDependency d2;
        d2.task_type  = MaintenanceTaskType::CONSISTENCY_CHECK;
        d2.depends_on = { MaintenanceTaskType::STORAGE_COMPACTION };
        e.task_dependencies.push_back(d2);
    }

    auto restored = MaintenanceScheduleEntry::fromJson(e.toJson());

    ASSERT_EQ(restored.tasks.size(),            e.tasks.size());
    ASSERT_EQ(restored.task_dependencies.size(), e.task_dependencies.size());

    // Verify each dependency was preserved in order and with correct edges.
    EXPECT_EQ(restored.task_dependencies[0].task_type, e.task_dependencies[0].task_type);
    ASSERT_EQ(restored.task_dependencies[0].depends_on.size(),
               e.task_dependencies[0].depends_on.size());
    EXPECT_EQ(restored.task_dependencies[0].depends_on[0],
               e.task_dependencies[0].depends_on[0]);

    EXPECT_EQ(restored.task_dependencies[1].task_type, e.task_dependencies[1].task_type);
    ASSERT_EQ(restored.task_dependencies[1].depends_on.size(),
               e.task_dependencies[1].depends_on.size());
    EXPECT_EQ(restored.task_dependencies[1].depends_on[0],
               e.task_dependencies[1].depends_on[0]);
}

} // namespace test
} // namespace maintenance
} // namespace themis
