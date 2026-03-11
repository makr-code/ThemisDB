/**
 * @file test_database_maintenance_orchestrator.cpp
 * @brief Unit tests for DatabaseMaintenanceOrchestrator – CRUD operations,
 *        job management, and health reporting.
 */

#include <gtest/gtest.h>

#include "maintenance/database_maintenance_orchestrator.h"
#include "maintenance/maintenance_schedule.h"
#include "maintenance/maintenance_task.h"
#include "maintenance/maintenance_health_report.h"
#include "server/maintenance_api_handler.h"

#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>

using namespace themis;
using namespace themis::maintenance;
using namespace std::chrono_literals;

// ---------------------------------------------------------------------------
// Fixture – orchestrator without real scheduler (nullptr is safe when not
// relying on cron registration)
// ---------------------------------------------------------------------------

class MaintenanceOrchestratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Pass nullptr scheduler – schedule persistence CRUD does not require it;
        // only actual cron-registration paths do.
        orchestrator_ = std::make_unique<DatabaseMaintenanceOrchestrator>(
            nullptr, nullptr, nullptr);
    }

    void TearDown() override {
        orchestrator_.reset();
    }

    // Helper: build a minimal valid schedule entry.
    MaintenanceScheduleEntry makeEntry(
        const std::string& name = "Test Schedule",
        ScheduleFrequency freq  = ScheduleFrequency::DAILY)
    {
        MaintenanceScheduleEntry e;
        e.name      = name;
        e.frequency = freq;
        e.tasks     = {MaintenanceTaskType::METRICS_COLLECTION};
        return e;
    }

    std::unique_ptr<DatabaseMaintenanceOrchestrator> orchestrator_;
};

// ===========================================================================
// Schedule CRUD – Create
// ===========================================================================

TEST_F(MaintenanceOrchestratorTest, CreateSchedule_AssignsId) {
    auto entry = makeEntry("Daily Metrics");
    auto result = orchestrator_->createSchedule(entry);
    ASSERT_TRUE(result) << result.error().message();
    EXPECT_FALSE(result->id.empty());
    EXPECT_EQ(result->name, "Daily Metrics");
}

TEST_F(MaintenanceOrchestratorTest, CreateSchedule_DerivesCronForDaily) {
    auto entry = makeEntry();
    entry.window_start_hour = 3;
    auto result = orchestrator_->createSchedule(entry);
    ASSERT_TRUE(result);
    EXPECT_EQ(result->cron_expression, "0 3 * * *");
}

TEST_F(MaintenanceOrchestratorTest, CreateSchedule_DerivesCronForWeekly) {
    auto entry = makeEntry("Weekly", ScheduleFrequency::WEEKLY);
    entry.window_start_hour = 2;
    auto result = orchestrator_->createSchedule(entry);
    ASSERT_TRUE(result);
    EXPECT_EQ(result->cron_expression, "0 2 * * 0");
}

TEST_F(MaintenanceOrchestratorTest, CreateSchedule_DerivesCronForMonthly) {
    auto entry = makeEntry("Monthly", ScheduleFrequency::MONTHLY);
    entry.window_start_hour = 1;
    auto result = orchestrator_->createSchedule(entry);
    ASSERT_TRUE(result);
    EXPECT_EQ(result->cron_expression, "0 1 1 * *");
}

TEST_F(MaintenanceOrchestratorTest, CreateSchedule_DerivesCronForQuarterly) {
    auto entry = makeEntry("Quarterly", ScheduleFrequency::QUARTERLY);
    entry.window_start_hour = 0;
    auto result = orchestrator_->createSchedule(entry);
    ASSERT_TRUE(result);
    EXPECT_EQ(result->cron_expression, "0 0 1 1,4,7,10 *");
}

TEST_F(MaintenanceOrchestratorTest, CreateSchedule_CustomCronPreserved) {
    auto entry = makeEntry("Custom");
    entry.frequency        = ScheduleFrequency::CUSTOM;
    entry.cron_expression  = "*/15 * * * *";
    auto result = orchestrator_->createSchedule(entry);
    ASSERT_TRUE(result);
    EXPECT_EQ(result->cron_expression, "*/15 * * * *");
}

TEST_F(MaintenanceOrchestratorTest, CreateSchedule_SetsTimestamps) {
    auto before = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    auto result = orchestrator_->createSchedule(makeEntry());
    auto after = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    ASSERT_TRUE(result);
    EXPECT_GE(result->created_at_ms, before);
    EXPECT_LE(result->created_at_ms, after);
    EXPECT_EQ(result->created_at_ms, result->updated_at_ms);
}

TEST_F(MaintenanceOrchestratorTest, CreateSchedule_RejectsEmptyName) {
    auto entry = makeEntry("");
    auto result = orchestrator_->createSchedule(entry);
    EXPECT_FALSE(result);
    EXPECT_NE(result.error().context().find("name"), std::string::npos);
}

TEST_F(MaintenanceOrchestratorTest, CreateSchedule_RejectsEmptyTaskList) {
    MaintenanceScheduleEntry e;
    e.name      = "No tasks";
    e.frequency = ScheduleFrequency::DAILY;
    // tasks intentionally left empty
    auto result = orchestrator_->createSchedule(e);
    EXPECT_FALSE(result);
    EXPECT_NE(result.error().context().find("task"), std::string::npos);
}

TEST_F(MaintenanceOrchestratorTest, CreateSchedule_RejectsCustomWithoutCron) {
    auto entry = makeEntry("Custom no cron");
    entry.frequency = ScheduleFrequency::CUSTOM;
    entry.cron_expression.clear();
    auto result = orchestrator_->createSchedule(entry);
    EXPECT_FALSE(result);
    EXPECT_NE(result.error().context().find("cron_expression"), std::string::npos);
}

// ===========================================================================
// Schedule CRUD – Read
// ===========================================================================

TEST_F(MaintenanceOrchestratorTest, GetSchedule_ReturnsCreatedEntry) {
    auto created = orchestrator_->createSchedule(makeEntry("Readable"));
    ASSERT_TRUE(created);
    auto fetched = orchestrator_->getSchedule(created->id);
    ASSERT_TRUE(fetched);
    EXPECT_EQ(fetched->id,   created->id);
    EXPECT_EQ(fetched->name, "Readable");
}

TEST_F(MaintenanceOrchestratorTest, GetSchedule_ReturnsErrorForUnknownId) {
    auto result = orchestrator_->getSchedule("does-not-exist");
    EXPECT_FALSE(result);
    EXPECT_NE(result.error().context().find("not found"), std::string::npos);
}

TEST_F(MaintenanceOrchestratorTest, ListSchedules_ReturnsAllCreated) {
    orchestrator_->createSchedule(makeEntry("A"));
    orchestrator_->createSchedule(makeEntry("B"));
    orchestrator_->createSchedule(makeEntry("C"));
    auto list = orchestrator_->listSchedules();
    EXPECT_EQ(list.size(), 3u);
}

TEST_F(MaintenanceOrchestratorTest, ListSchedules_EmptyWhenNone) {
    EXPECT_TRUE(orchestrator_->listSchedules().empty());
}

// ===========================================================================
// Schedule CRUD – Update (PUT)
// ===========================================================================

TEST_F(MaintenanceOrchestratorTest, UpdateSchedule_ReplacesFields) {
    auto created = orchestrator_->createSchedule(makeEntry("Original"));
    ASSERT_TRUE(created);

    MaintenanceScheduleEntry updated = *created;
    updated.name = "Updated";
    updated.tasks = {MaintenanceTaskType::CONSISTENCY_CHECK};
    updated.frequency = ScheduleFrequency::WEEKLY;

    auto result = orchestrator_->updateSchedule(created->id, updated);
    ASSERT_TRUE(result) << result.error().message();
    EXPECT_EQ(result->name, "Updated");
    EXPECT_EQ(result->tasks.size(), 1u);
    EXPECT_EQ(result->tasks[0], MaintenanceTaskType::CONSISTENCY_CHECK);
}

TEST_F(MaintenanceOrchestratorTest, UpdateSchedule_PreservesCreatedAt) {
    auto created = orchestrator_->createSchedule(makeEntry("Preserve"));
    ASSERT_TRUE(created);

    MaintenanceScheduleEntry upd = *created;
    upd.name = "New name";
    auto result = orchestrator_->updateSchedule(created->id, upd);
    ASSERT_TRUE(result);
    EXPECT_EQ(result->created_at_ms, created->created_at_ms);
}

TEST_F(MaintenanceOrchestratorTest, UpdateSchedule_UpdatesTimestamp) {
    auto created = orchestrator_->createSchedule(makeEntry("TS Test"));
    ASSERT_TRUE(created);

    std::this_thread::sleep_for(5ms);

    MaintenanceScheduleEntry upd = *created;
    upd.name = "Different name";
    auto result = orchestrator_->updateSchedule(created->id, upd);
    ASSERT_TRUE(result);
    EXPECT_GE(result->updated_at_ms, created->updated_at_ms);
}

TEST_F(MaintenanceOrchestratorTest, UpdateSchedule_ReturnsErrorForUnknownId) {
    auto entry = makeEntry("X");
    auto result = orchestrator_->updateSchedule("nonexistent", entry);
    EXPECT_FALSE(result);
}

// ===========================================================================
// Schedule CRUD – Patch (PATCH)
// ===========================================================================

TEST_F(MaintenanceOrchestratorTest, PatchSchedule_UpdatesOnlyProvidedFields) {
    auto created = orchestrator_->createSchedule(makeEntry("Before Patch"));
    ASSERT_TRUE(created);

    nlohmann::json patch;
    patch["name"] = "After Patch";

    auto result = orchestrator_->patchSchedule(created->id, patch);
    ASSERT_TRUE(result) << result.error().message();
    EXPECT_EQ(result->name, "After Patch");
    // Other fields unchanged
    EXPECT_EQ(result->frequency, created->frequency);
    EXPECT_EQ(result->tasks,     created->tasks);
}

TEST_F(MaintenanceOrchestratorTest, PatchSchedule_CanToggleEnabled) {
    auto created = orchestrator_->createSchedule(makeEntry("Toggle"));
    ASSERT_TRUE(created);
    EXPECT_TRUE(created->enabled);

    nlohmann::json patch;
    patch["enabled"] = false;
    auto result = orchestrator_->patchSchedule(created->id, patch);
    ASSERT_TRUE(result);
    EXPECT_FALSE(result->enabled);
}

TEST_F(MaintenanceOrchestratorTest, PatchSchedule_ReturnsErrorForUnknownId) {
    nlohmann::json patch;
    patch["name"] = "Ghost";
    auto result = orchestrator_->patchSchedule("ghost-id", patch);
    EXPECT_FALSE(result);
}

// ===========================================================================
// Schedule CRUD – Delete
// ===========================================================================

TEST_F(MaintenanceOrchestratorTest, DeleteSchedule_RemovesEntry) {
    auto created = orchestrator_->createSchedule(makeEntry("ToDelete"));
    ASSERT_TRUE(created);

    auto del = orchestrator_->deleteSchedule(created->id);
    EXPECT_TRUE(del) << del.error();

    // Should no longer be accessible
    auto fetched = orchestrator_->getSchedule(created->id);
    EXPECT_FALSE(fetched);
}

TEST_F(MaintenanceOrchestratorTest, DeleteSchedule_DecrementsListSize) {
    auto a = orchestrator_->createSchedule(makeEntry("A"));
    auto b = orchestrator_->createSchedule(makeEntry("B"));
    ASSERT_TRUE(a); ASSERT_TRUE(b);

    EXPECT_EQ(orchestrator_->listSchedules().size(), 2u);
    orchestrator_->deleteSchedule(a->id);
    EXPECT_EQ(orchestrator_->listSchedules().size(), 1u);
}

TEST_F(MaintenanceOrchestratorTest, DeleteSchedule_ReturnsErrorForUnknownId) {
    auto result = orchestrator_->deleteSchedule("no-such-id");
    EXPECT_FALSE(result);
}

// ===========================================================================
// Serialisation round-trip
// ===========================================================================

TEST_F(MaintenanceOrchestratorTest, ScheduleEntry_JsonRoundTrip) {
    MaintenanceScheduleEntry e;
    e.id               = "test-id";
    e.name             = "JSON Round-Trip";
    e.description      = "Tests serialisation";
    e.frequency        = ScheduleFrequency::WEEKLY;
    e.cron_expression  = "0 2 * * 0";
    e.tasks            = {MaintenanceTaskType::CONSISTENCY_CHECK,
                          MaintenanceTaskType::MVCC_CLEANUP};
    e.enabled          = false;
    e.enforce_window   = true;
    e.window_start_hour = 2;
    e.window_end_hour  = 6;

    nlohmann::json j = e.toJson();
    auto restored = MaintenanceScheduleEntry::fromJson(j);

    EXPECT_EQ(restored.id,               e.id);
    EXPECT_EQ(restored.name,             e.name);
    EXPECT_EQ(restored.description,      e.description);
    EXPECT_EQ(restored.frequency,        e.frequency);
    EXPECT_EQ(restored.cron_expression,  e.cron_expression);
    EXPECT_EQ(restored.tasks,            e.tasks);
    EXPECT_EQ(restored.enabled,          e.enabled);
    EXPECT_EQ(restored.window_start_hour, e.window_start_hour);
    EXPECT_EQ(restored.window_end_hour,  e.window_end_hour);
}

TEST_F(MaintenanceOrchestratorTest, ApplyPatch_PartialUpdate) {
    MaintenanceScheduleEntry e = makeEntry("Before");
    nlohmann::json patch;
    patch["name"] = "After";
    patch["enabled"] = false;
    e.applyPatch(patch);
    EXPECT_EQ(e.name, "After");
    EXPECT_FALSE(e.enabled);
    // tasks untouched
    EXPECT_EQ(e.tasks[0], MaintenanceTaskType::METRICS_COLLECTION);
}

// ===========================================================================
// Health report
// ===========================================================================

TEST_F(MaintenanceOrchestratorTest, HealthReport_EmptyOrchestrator) {
    auto report = orchestrator_->getHealthReport();
    EXPECT_EQ(report.total_schedules,   0);
    EXPECT_EQ(report.enabled_schedules, 0);
    EXPECT_EQ(report.active_jobs,       0);
    EXPECT_GT(report.generated_at_ms,   0LL);
}

TEST_F(MaintenanceOrchestratorTest, HealthReport_CountsEnabledSchedules) {
    auto e1 = makeEntry("Enabled");  e1.enabled = true;
    auto e2 = makeEntry("Disabled"); e2.enabled = false;
    orchestrator_->createSchedule(e1);
    orchestrator_->createSchedule(e2);

    auto report = orchestrator_->getHealthReport();
    EXPECT_EQ(report.total_schedules,   2);
    EXPECT_EQ(report.enabled_schedules, 1);
}

TEST_F(MaintenanceOrchestratorTest, HealthReport_IncludesModuleProbe) {
    orchestrator_->registerHealthProbe("test_module", []() -> ModuleHealthSignal {
        ModuleHealthSignal s;
        s.module_name   = "test_module";
        s.status        = ModuleHealthStatus::OK;
        s.message       = "all good";
        s.checked_at_ms = 1000;
        return s;
    });

    auto report = orchestrator_->getHealthReport();
    ASSERT_EQ(report.module_signals.size(), 1u);
    EXPECT_EQ(report.module_signals[0].module_name, "test_module");
    EXPECT_EQ(report.module_signals[0].status, ModuleHealthStatus::OK);
}

TEST_F(MaintenanceOrchestratorTest, HealthReport_WorstStatusPropagates) {
    orchestrator_->registerHealthProbe("ok_module", []() -> ModuleHealthSignal {
        ModuleHealthSignal s;
        s.module_name = "ok_module";
        s.status      = ModuleHealthStatus::OK;
        return s;
    });
    orchestrator_->registerHealthProbe("critical_module", []() -> ModuleHealthSignal {
        ModuleHealthSignal s;
        s.module_name = "critical_module";
        s.status      = ModuleHealthStatus::CRITICAL;
        return s;
    });

    auto report = orchestrator_->getHealthReport();
    EXPECT_EQ(report.overall_status, ModuleHealthStatus::CRITICAL);
}

// ===========================================================================
// Status
// ===========================================================================

TEST_F(MaintenanceOrchestratorTest, GetStatus_ContainsExpectedKeys) {
    auto status = orchestrator_->getStatus();
    EXPECT_TRUE(status.contains("running"));
    EXPECT_TRUE(status.contains("total_schedules"));
    EXPECT_TRUE(status.contains("enabled_schedules"));
    EXPECT_TRUE(status.contains("active_jobs"));
    EXPECT_TRUE(status.contains("generated_at_ms"));
}

TEST_F(MaintenanceOrchestratorTest, GetStatus_ReflectsScheduleCount) {
    orchestrator_->createSchedule(makeEntry("S1"));
    orchestrator_->createSchedule(makeEntry("S2"));
    auto status = orchestrator_->getStatus();
    EXPECT_EQ(status["total_schedules"].get<int>(), 2);
}

// ===========================================================================
// MaintenanceApiHandler tests
// ===========================================================================

class MaintenanceApiHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        orchestrator_ = std::make_unique<DatabaseMaintenanceOrchestrator>(
            nullptr, nullptr, nullptr);
        handler_ = std::make_unique<server::MaintenanceApiHandler>(orchestrator_.get());
    }

    nlohmann::json makeScheduleBody(
        const std::string& name = "Test",
        const std::string& freq = "daily")
    {
        return {
            {"name",      name},
            {"frequency", freq},
            {"tasks",     nlohmann::json::array({"metrics_collection"})},
        };
    }

    std::unique_ptr<DatabaseMaintenanceOrchestrator> orchestrator_;
    std::unique_ptr<server::MaintenanceApiHandler>   handler_;
};

TEST_F(MaintenanceApiHandlerTest, CreateSchedule_ReturnsCreatedStatus) {
    auto result = handler_->createSchedule(makeScheduleBody("Via API"));
    EXPECT_EQ(result.value("status", ""), "created");
    EXPECT_FALSE(result.value("id", "").empty());
}

TEST_F(MaintenanceApiHandlerTest, CreateSchedule_RejectsEmptyName) {
    auto result = handler_->createSchedule(makeScheduleBody(""));
    EXPECT_EQ(result.value("status", ""), "error");
}

TEST_F(MaintenanceApiHandlerTest, ListSchedules_ReturnsArray) {
    handler_->createSchedule(makeScheduleBody("A"));
    handler_->createSchedule(makeScheduleBody("B"));
    auto result = handler_->listSchedules();
    EXPECT_TRUE(result.contains("schedules"));
    EXPECT_EQ(result["count"].get<int>(), 2);
    EXPECT_EQ(result["schedules"].size(), 2u);
}

TEST_F(MaintenanceApiHandlerTest, GetSchedule_ReturnsEntry) {
    auto created = handler_->createSchedule(makeScheduleBody("Fetch Me"));
    std::string id = created["id"].get<std::string>();
    auto fetched = handler_->getSchedule(id);
    EXPECT_EQ(fetched.value("name", ""), "Fetch Me");
}

TEST_F(MaintenanceApiHandlerTest, GetSchedule_UnknownIdReturnsError) {
    auto result = handler_->getSchedule("nope");
    EXPECT_EQ(result.value("status", ""), "error");
}

TEST_F(MaintenanceApiHandlerTest, UpdateSchedule_FullReplace) {
    auto created = handler_->createSchedule(makeScheduleBody("Old"));
    std::string id = created["id"].get<std::string>();

    nlohmann::json body = makeScheduleBody("New");
    body["tasks"] = nlohmann::json::array({"consistency_check"});
    auto result = handler_->updateSchedule(id, body);
    EXPECT_EQ(result.value("name", ""), "New");
}

TEST_F(MaintenanceApiHandlerTest, PatchSchedule_PartialUpdate) {
    auto created = handler_->createSchedule(makeScheduleBody("Patch Test"));
    std::string id = created["id"].get<std::string>();

    nlohmann::json patch = {{"name", "Patched"}};
    auto result = handler_->patchSchedule(id, patch);
    EXPECT_EQ(result.value("name", ""), "Patched");
    // frequency should be unchanged (daily)
    EXPECT_EQ(result.value("frequency", ""), "daily");
}

TEST_F(MaintenanceApiHandlerTest, DeleteSchedule_RemovesEntry) {
    auto created = handler_->createSchedule(makeScheduleBody("To Remove"));
    std::string id = created["id"].get<std::string>();

    auto del = handler_->deleteSchedule(id);
    EXPECT_EQ(del.value("status", ""), "deleted");

    auto fetched = handler_->getSchedule(id);
    EXPECT_EQ(fetched.value("status", ""), "error");
}

TEST_F(MaintenanceApiHandlerTest, ListJobs_ReturnsArray) {
    auto result = handler_->listJobs();
    EXPECT_TRUE(result.contains("jobs"));
    EXPECT_EQ(result["count"].get<int>(), 0);
}

TEST_F(MaintenanceApiHandlerTest, GetStatus_IsJson) {
    auto status = handler_->getStatus();
    EXPECT_TRUE(status.contains("running"));
}

TEST_F(MaintenanceApiHandlerTest, GetHealth_IsJson) {
    auto health = handler_->getHealth();
    EXPECT_TRUE(health.contains("overall_status"));
    EXPECT_TRUE(health.contains("module_signals"));
}

// ===========================================================================
// Task type and frequency string conversion
// ===========================================================================

TEST(MaintenanceTaskTypeTest, StringRoundTrip) {
    auto types = {
        MaintenanceTaskType::METRICS_COLLECTION,
        MaintenanceTaskType::CONSISTENCY_CHECK,
        MaintenanceTaskType::INDEX_REBUILD,
        MaintenanceTaskType::STORAGE_COMPACTION,
        MaintenanceTaskType::VECTOR_REINDEX,
    };
    for (auto t : types) {
        EXPECT_EQ(taskTypeFromString(taskTypeToString(t)), t)
            << "Round-trip failed for: " << taskTypeToString(t);
    }
}

TEST(ScheduleFrequencyTest, StringRoundTrip) {
    auto freqs = {
        ScheduleFrequency::DAILY,
        ScheduleFrequency::WEEKLY,
        ScheduleFrequency::MONTHLY,
        ScheduleFrequency::QUARTERLY,
        ScheduleFrequency::CUSTOM,
    };
    for (auto f : freqs) {
        EXPECT_EQ(frequencyFromString(frequencyToString(f)), f)
            << "Round-trip failed for: " << frequencyToString(f);
    }
}

TEST(ScheduleFrequencyTest, CronDerivation) {
    EXPECT_EQ(frequencyToCron(ScheduleFrequency::DAILY,     2), "0 2 * * *");
    EXPECT_EQ(frequencyToCron(ScheduleFrequency::WEEKLY,    3), "0 3 * * 0");
    EXPECT_EQ(frequencyToCron(ScheduleFrequency::MONTHLY,   1), "0 1 1 * *");
    EXPECT_EQ(frequencyToCron(ScheduleFrequency::QUARTERLY, 0), "0 0 1 1,4,7,10 *");
}

TEST(OrchestratorJobTest, JsonContainsAllFields) {
    OrchestratorJob job;
    job.id            = "job-1";
    job.schedule_id   = "sched-1";
    job.task_type     = MaintenanceTaskType::CONSISTENCY_CHECK;
    job.state         = MaintenanceJobState::SUCCEEDED;
    job.result_summary = "ok";
    job.started_at_ms  = 1000;
    job.finished_at_ms = 2000;

    auto j = job.toJson();
    EXPECT_EQ(j["id"].get<std::string>(),          "job-1");
    EXPECT_EQ(j["schedule_id"].get<std::string>(),  "sched-1");
    EXPECT_EQ(j["task_type"].get<std::string>(),    "consistency_check");
    EXPECT_EQ(j["state"].get<std::string>(),        "succeeded");
    EXPECT_EQ(j["duration_ms"].get<int64_t>(),      1000);
}
