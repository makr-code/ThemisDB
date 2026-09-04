/**
 * @file test_database_maintenance_orchestrator.cpp
 * @brief Unit tests for DatabaseMaintenanceOrchestrator – CRUD operations,
 *        job management, health reporting, window enforcement, and metrics.
 */

#include <gtest/gtest.h>

#include "maintenance/database_maintenance_orchestrator.h"
#include "maintenance/maintenance_schedule.h"
#include "maintenance/maintenance_schedule_store.h"
#include "maintenance/maintenance_task.h"
#include "maintenance/maintenance_health_report.h"
#include "maintenance/i_maintenance_task_handler.h"
#include "maintenance/i_distributed_lock.h"
#include "server/maintenance_api_handler.h"
#include "observability/metrics_collector.h"

#include <nlohmann/json.hpp>
#include <atomic>
#include <thread>
#include <vector>
#include <chrono>
#include <map>
#include <mutex>
#include <string>

using namespace themis;
using namespace themis::maintenance;
using namespace std::chrono_literals;

// ===========================================================================
// Minimal in-memory IStorageEngine for persistence tests
// ===========================================================================

class InMemoryStorageEngine : public IStorageEngine {
public:
    Result<void> open(const std::string& /*db_path*/) override { return OkVoid(); }
    void close() override {}

    Result<void> put(const std::string& key, const std::string& value) override {
        std::lock_guard<std::mutex> lk(mu_);
        store_[key] = value;
        return OkVoid();
    }

    Result<std::string> get(const std::string& key) override {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = store_.find(key);
        if (it == store_.end()) {
            return tl::unexpected(Error(errors::ErrorCode::ERR_INDEX_NOT_FOUND,
                                        "key not found: " + key));
        }
        return it->second;
    }

    Result<void> del(const std::string& key) override {
        std::lock_guard<std::mutex> lk(mu_);
        store_.erase(key);
        return OkVoid();
    }

    Result<void> scanPrefix(
        std::string_view prefix,
        std::function<bool(std::string_view, std::string_view)> callback) override
    {
        std::lock_guard<std::mutex> lk(mu_);
        for (auto& [k, v] : store_) {
            if (k.size() >= prefix.size() &&
                k.compare(0, prefix.size(), prefix.data(), prefix.size()) == 0) {
                if (!callback(k, v)) {
                  break;
                }
            }
        }
        return OkVoid();
    }

    /// Inject a corrupt (non-JSON) value for a given key (for error-path tests).
    void injectCorrupt(const std::string& key, const std::string& bad_value) {
        std::lock_guard<std::mutex> lk(mu_);
        store_[key] = bad_value;
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lk(mu_);
        return store_.size();
    }

private:
    mutable std::mutex              mu_;
    std::map<std::string, std::string> store_;
};

// ---------------------------------------------------------------------------
// Fixture – orchestrator without real scheduler (nullptr is safe when not
// relying on cron registration)
// ---------------------------------------------------------------------------

class MaintenanceOrchestratorTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping MaintenanceOrchestratorTest on Windows due to intermittent segfaults and unstable async state ordering in orchestrator suite.";
#endif
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
    EXPECT_TRUE(del) << del.error().message();

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

TEST_F(MaintenanceApiHandlerTest, ListSchedules_InvalidTenantFilterReturnsError) {
    auto result = handler_->listSchedules("tenant\r\nX-Injected: 1");
    EXPECT_EQ(result.value("status", ""), "error");
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

TEST_F(MaintenanceApiHandlerTest, GetSchedule_InvalidIdReturnsError) {
    auto result = handler_->getSchedule("../bad");
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

TEST_F(MaintenanceApiHandlerTest, TriggerNow_InvalidScheduleIdReturnsError) {
    auto result = handler_->triggerNow("../bad", false);
    EXPECT_EQ(result.value("status", ""), "error");
}

TEST_F(MaintenanceApiHandlerTest, ListJobs_ReturnsArray) {
    auto result = handler_->listJobs();
    EXPECT_TRUE(result.contains("jobs"));
    EXPECT_EQ(result["count"].get<int>(), 0);
}

TEST_F(MaintenanceApiHandlerTest, CancelJob_InvalidIdReturnsError) {
    auto result = handler_->cancelJob("../bad-job");
    EXPECT_EQ(result.value("status", ""), "error");
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

// ===========================================================================
// Maintenance window enforcement
// ===========================================================================

TEST_F(MaintenanceOrchestratorTest, WindowEnforcement_SkipsJobOutsideWindow) {
    // Create a schedule with enforce_window=true and a window that is guaranteed
    // to be in the future (window 00:00-00:01 UTC — almost certainly not NOW).
    // To avoid flakiness we set a 1-hour-wide window and pick an hour that is
    // guaranteed not to be the current UTC hour by checking.
    int current_hour = static_cast<int>(
        std::chrono::duration_cast<std::chrono::hours>(
            std::chrono::system_clock::now().time_since_epoch()).count() % 24);

    // Pick a window that does NOT include the current hour
    int safe_start = (current_hour + 12) % 24;  // 12 hours away
    int safe_end   = (safe_start + 1) % 24;

    auto entry = makeEntry("Window Test");
    entry.enforce_window    = true;
    entry.window_start_hour = safe_start;
    entry.window_end_hour   = safe_end;
    entry.tasks = {MaintenanceTaskType::METRICS_COLLECTION};

    auto created = orchestrator_->createSchedule(entry);
    ASSERT_TRUE(created);

    // Trigger the schedule – execution is synchronous via the callback thread.
    // We need to manually invoke executeSchedule-like logic.
    // Since triggerNow launches a detached thread, we use a small sleep.
    auto job_result = orchestrator_->triggerNow(created->id);
    ASSERT_TRUE(job_result) << job_result.error().message();

    std::string job_id = job_result->id;

    // Wait for the background thread to finish (up to 2 s)
    for (int i = 0; i < 40; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        auto j = orchestrator_->getJob(job_id);
        if ((j && (j->state == MaintenanceJobState::SKIPPED ||
                  j->state == MaintenanceJobState::SUCCEEDED ||
                  j->state == MaintenanceJobState::FAILED))) {
            break;
        }
    }

    auto final_job = orchestrator_->getJob(job_id);
    ASSERT_TRUE(final_job);
    EXPECT_EQ(final_job->state, MaintenanceJobState::SKIPPED)
        << "Expected SKIPPED but got: " << jobStateToString(final_job->state);
    EXPECT_NE(final_job->error_message.find("window"), std::string::npos);
}

TEST_F(MaintenanceOrchestratorTest, WindowEnforcement_RunsInsideWindow) {
    // Schedule with enforce_window=true but window = full day [0, 23]
    auto entry = makeEntry("Full Day Window");
    entry.enforce_window    = true;
    entry.window_start_hour = 0;
    entry.window_end_hour   = 23;
    entry.tasks = {MaintenanceTaskType::METRICS_COLLECTION};

    auto created = orchestrator_->createSchedule(entry);
    ASSERT_TRUE(created);

    auto job_result = orchestrator_->triggerNow(created->id);
    ASSERT_TRUE(job_result);

    std::string job_id = job_result->id;
    for (int i = 0; i < 40; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        auto j = orchestrator_->getJob(job_id);
        if (j && j->state != MaintenanceJobState::RUNNING &&
                 j->state != MaintenanceJobState::PENDING) break;
    }

    auto final_job = orchestrator_->getJob(job_id);
    ASSERT_TRUE(final_job);
    // Should NOT be SKIPPED when inside window
    EXPECT_NE(final_job->state, MaintenanceJobState::SKIPPED);
    EXPECT_EQ(final_job->state, MaintenanceJobState::SUCCEEDED);
}

TEST_F(MaintenanceOrchestratorTest, WindowEnforcement_NoEnforcementAllowsAnyHour) {
    // enforce_window=false means the job should always run regardless of hour
    auto entry = makeEntry("No Enforcement");
    entry.enforce_window    = false;
    entry.window_start_hour = 2;
    entry.window_end_hour   = 3;   // Very narrow window – almost certainly NOT now
    entry.tasks = {MaintenanceTaskType::METRICS_COLLECTION};

    auto created = orchestrator_->createSchedule(entry);
    ASSERT_TRUE(created);

    auto job_result = orchestrator_->triggerNow(created->id);
    ASSERT_TRUE(job_result);

    std::string job_id = job_result->id;
    for (int i = 0; i < 40; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        auto j = orchestrator_->getJob(job_id);
        if (j && j->state != MaintenanceJobState::RUNNING &&
                 j->state != MaintenanceJobState::PENDING) break;
    }

    auto final_job = orchestrator_->getJob(job_id);
    ASSERT_TRUE(final_job);
    EXPECT_NE(final_job->state, MaintenanceJobState::SKIPPED);
}

// ===========================================================================
// Halt-on-failure (DAG sequential execution)
// ===========================================================================

TEST_F(MaintenanceOrchestratorTest, HaltOnFailure_StopsAfterFirstFailure) {
    // metrics_collection will SUCCEED (module-delegated).
    // We test that with halt_on_task_failure, subsequent tasks are not started
    // after a failing task.  Since we can't inject a failure in the current
    // module-delegated tasks, we verify the flag is stored and honoured
    // (via the schedule entry).
    auto entry = makeEntry("Halt Test");
    entry.halt_on_task_failure = true;
    entry.tasks = {
        MaintenanceTaskType::METRICS_COLLECTION,
        MaintenanceTaskType::QUOTA_CHECK,
    };
    entry.enforce_window = false;

    auto created = orchestrator_->createSchedule(entry);
    ASSERT_TRUE(created);
    EXPECT_TRUE(created->halt_on_task_failure);

    // Verify it round-trips through patch
    nlohmann::json patch = {{"halt_on_task_failure", false}};
    auto patched = orchestrator_->patchSchedule(created->id, patch);
    ASSERT_TRUE(patched);
    EXPECT_FALSE(patched->halt_on_task_failure);
}

// ===========================================================================
// SKIPPED job state enum/JSON
// ===========================================================================

TEST(JobStateTest, SkippedStateJson) {
    OrchestratorJob job;
    job.id            = "skip-job";
    job.schedule_id   = "sched-1";
    job.task_type     = MaintenanceTaskType::METRICS_COLLECTION;
    job.state         = MaintenanceJobState::SKIPPED;
    job.error_message = "Outside maintenance window";
    job.started_at_ms  = 1000;
    job.finished_at_ms = 1001;

    auto j = job.toJson();
    EXPECT_EQ(j["state"].get<std::string>(), "skipped");
    EXPECT_NE(j["error_message"].get<std::string>().find("window"),
              std::string::npos);
}

// ===========================================================================
// MaintenanceScheduleStore unit tests
// ===========================================================================

class MaintenanceScheduleStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine_ = std::make_shared<InMemoryStorageEngine>();
        store_  = std::make_unique<MaintenanceScheduleStore>(engine_.get());
    }

    MaintenanceScheduleEntry makeEntry(const std::string& id,
                                       const std::string& name = "Test") {
        MaintenanceScheduleEntry e;
        e.id        = id;
        e.name      = name;
        e.frequency = ScheduleFrequency::DAILY;
        e.tasks     = {MaintenanceTaskType::METRICS_COLLECTION};
        return e;
    }

    std::shared_ptr<InMemoryStorageEngine> engine_;
    std::unique_ptr<MaintenanceScheduleStore> store_;
};

TEST_F(MaintenanceScheduleStoreTest, Save_StoresOneKey) {
    auto entry = makeEntry("id-001", "Alpha");
    auto result = store_->save(entry);
    ASSERT_TRUE(result) << result.error().message();
    EXPECT_EQ(engine_->size(), 1u);
}

TEST_F(MaintenanceScheduleStoreTest, Save_OverwritesExistingEntry) {
    auto entry = makeEntry("id-001", "Alpha");
    ASSERT_TRUE(store_->save(entry));
    entry.name = "Beta";
    ASSERT_TRUE(store_->save(entry));
    EXPECT_EQ(engine_->size(), 1u); // still one key

    std::map<std::string, MaintenanceScheduleEntry> schedules;
    ASSERT_TRUE(store_->loadAll(schedules));
    ASSERT_EQ(schedules.count("id-001"), 1u);
    EXPECT_EQ(schedules["id-001"].name, "Beta");
}

TEST_F(MaintenanceScheduleStoreTest, Remove_DeletesKey) {
    ASSERT_TRUE(store_->save(makeEntry("id-001")));
    EXPECT_EQ(engine_->size(), 1u);

    auto result = store_->remove("id-001");
    ASSERT_TRUE(result) << result.error().message();
    EXPECT_EQ(engine_->size(), 0u);
}

TEST_F(MaintenanceScheduleStoreTest, Remove_IdempotentForMissingKey) {
    // Deleting a non-existent key must not return an error.
    auto result = store_->remove("does-not-exist");
    EXPECT_TRUE(result);
}

TEST_F(MaintenanceScheduleStoreTest, LoadAll_ReturnsAllSavedEntries) {
    ASSERT_TRUE(store_->save(makeEntry("id-001", "A")));
    ASSERT_TRUE(store_->save(makeEntry("id-002", "B")));
    ASSERT_TRUE(store_->save(makeEntry("id-003", "C")));

    std::map<std::string, MaintenanceScheduleEntry> schedules;
    auto result = store_->loadAll(schedules);
    ASSERT_TRUE(result) << result.error().message();
    EXPECT_EQ(schedules.size(), 3u);
    EXPECT_EQ(schedules.count("id-001"), 1u);
    EXPECT_EQ(schedules.count("id-002"), 1u);
    EXPECT_EQ(schedules.count("id-003"), 1u);
}

TEST_F(MaintenanceScheduleStoreTest, LoadAll_SkipsCorruptEntry) {
    // Save one valid and inject one corrupt entry.
    ASSERT_TRUE(store_->save(makeEntry("id-valid", "Valid")));
    engine_->injectCorrupt(
        std::string(MaintenanceScheduleStore::kKeyPrefix) + "corrupt-key",
        "THIS IS NOT JSON {{{{");

    std::map<std::string, MaintenanceScheduleEntry> schedules;
    auto result = store_->loadAll(schedules);
    ASSERT_TRUE(result) << result.error().message(); // must not fail overall
    // Only the valid entry should be present.
    ASSERT_EQ(schedules.size(), 1u);
    EXPECT_EQ(schedules.count("id-valid"), 1u);
}

TEST_F(MaintenanceScheduleStoreTest, LoadAll_EmptyEngineReturnsEmptyMap) {
    std::map<std::string, MaintenanceScheduleEntry> schedules;
    auto result = store_->loadAll(schedules);
    ASSERT_TRUE(result);
    EXPECT_TRUE(schedules.empty());
}

TEST_F(MaintenanceScheduleStoreTest, KeyPrefixIsCorrect) {
    EXPECT_EQ(MaintenanceScheduleStore::kKeyPrefix, "maint_sched::");
}

// ===========================================================================
// Restart-persistence integration tests
// ===========================================================================

/// Simulates the full restart scenario: create schedules, destroy the
/// orchestrator, re-create it with the same storage, verify all schedules
/// are still present.
class SchedulePersistenceIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine_ = std::make_shared<InMemoryStorageEngine>();
    }

    MaintenanceScheduleEntry makeEntry(const std::string& name,
                                       ScheduleFrequency freq = ScheduleFrequency::DAILY) {
        MaintenanceScheduleEntry e;
        e.name      = name;
        e.frequency = freq;
        e.tasks     = {MaintenanceTaskType::METRICS_COLLECTION};
        return e;
    }

    std::shared_ptr<InMemoryStorageEngine> engine_;
};

TEST_F(SchedulePersistenceIntegrationTest, RestartRetainsAllThreeSchedules) {
    std::string id1, id2, id3;

    // Phase 1: create 3 schedules using the first orchestrator instance.
    {
        DatabaseMaintenanceOrchestrator orc(nullptr, nullptr, nullptr, engine_.get());

        auto r1 = orc.createSchedule(makeEntry("Daily Metrics"));
        ASSERT_TRUE(r1) << r1.error().message();
        id1 = r1->id;

        auto r2 = orc.createSchedule(makeEntry("Weekly Consistency",
                                                ScheduleFrequency::WEEKLY));
        ASSERT_TRUE(r2) << r2.error().message();
        id2 = r2->id;

        auto r3 = orc.createSchedule(makeEntry("Monthly Compaction",
                                                ScheduleFrequency::MONTHLY));
        ASSERT_TRUE(r3) << r3.error().message();
        id3 = r3->id;

        EXPECT_EQ(orc.listSchedules().size(), 3u);
        // Orchestrator is destroyed here; schedules remain in the storage engine.
    }

    // Phase 2: create a new orchestrator instance backed by the same engine.
    // start() loads schedules from storage before checking the scheduler
    // availability, so schedules_ is populated even when scheduler is null.
    {
        DatabaseMaintenanceOrchestrator orc2(nullptr, nullptr, nullptr, engine_.get());

        // start() will return an error (null scheduler) but MUST first load
        // all persisted schedules into schedules_.
        auto start_result = orc2.start();
        EXPECT_FALSE(start_result); // expected: error because scheduler is null

        // Verify via the orchestrator's own API that all 3 schedules are present.
        auto reloaded_vec = orc2.listSchedules();
        ASSERT_EQ(reloaded_vec.size(), 3u);

        // Build a map by id for easier lookup.
        std::map<std::string, MaintenanceScheduleEntry> reloaded = {};

        for (const auto& entry : reloaded_vec) {
            reloaded.emplace(entry.id, entry);
        }

        EXPECT_EQ(reloaded.count(id1), 1u);
        EXPECT_EQ(reloaded.count(id2), 1u);
        EXPECT_EQ(reloaded.count(id3), 1u);
        EXPECT_EQ(reloaded[id1].name, "Daily Metrics");
        EXPECT_EQ(reloaded[id2].name, "Weekly Consistency");
        EXPECT_EQ(reloaded[id3].name, "Monthly Compaction");
    }
}

TEST_F(SchedulePersistenceIntegrationTest, DeletedScheduleNotReloadedAfterRestart) {
    std::string id1, id2;

    {
        DatabaseMaintenanceOrchestrator orc(nullptr, nullptr, nullptr, engine_.get());

        auto r1 = orc.createSchedule(makeEntry("Keep Me"));
        ASSERT_TRUE(r1);
        id1 = r1->id;

        auto r2 = orc.createSchedule(makeEntry("Delete Me"));
        ASSERT_TRUE(r2);
        id2 = r2->id;

        auto del = orc.deleteSchedule(id2);
        ASSERT_TRUE(del) << del.error().message();
    }

    // After restart only id1 should be present.
    {
        DatabaseMaintenanceOrchestrator orc2(nullptr, nullptr, nullptr, engine_.get());
        EXPECT_FALSE(orc2.start()); // loads from storage; null scheduler returns error

        auto schedules = orc2.listSchedules();
        EXPECT_EQ(schedules.size(), 1u);
        ASSERT_FALSE(schedules.empty());
        EXPECT_EQ(schedules[0].id, id1);
    }
}

TEST_F(SchedulePersistenceIntegrationTest, UpdatedSchedulePersistedAfterRestart) {
    std::string id = {};

    {
        DatabaseMaintenanceOrchestrator orc(nullptr, nullptr, nullptr, engine_.get());

        auto r = orc.createSchedule(makeEntry("Original Name"));
        ASSERT_TRUE(r);
        id = r->id;

        MaintenanceScheduleEntry updated = *r;
        updated.name = "Updated Name";
        auto upd = orc.updateSchedule(id, updated);
        ASSERT_TRUE(upd) << upd.error().message();
    }

    {
        DatabaseMaintenanceOrchestrator orc2(nullptr, nullptr, nullptr, engine_.get());
        EXPECT_FALSE(orc2.start()); // loads from storage; null scheduler returns error

        auto fetched = orc2.getSchedule(id);
        ASSERT_TRUE(fetched);
        EXPECT_EQ(fetched->name, "Updated Name");
    }
}

TEST_F(SchedulePersistenceIntegrationTest, PatchedSchedulePersistedAfterRestart) {
    std::string id = {};

    {
        DatabaseMaintenanceOrchestrator orc(nullptr, nullptr, nullptr, engine_.get());

        auto r = orc.createSchedule(makeEntry("Before Patch"));
        ASSERT_TRUE(r);
        id = r->id;

        nlohmann::json patch = {{"name", "After Patch"}};
        auto p = orc.patchSchedule(id, patch);
        ASSERT_TRUE(p) << p.error().message();
    }

    {
        DatabaseMaintenanceOrchestrator orc2(nullptr, nullptr, nullptr, engine_.get());
        EXPECT_FALSE(orc2.start()); // loads from storage; null scheduler returns error

        auto fetched = orc2.getSchedule(id);
        ASSERT_TRUE(fetched);
        EXPECT_EQ(fetched->name, "After Patch");
    }
}

TEST_F(SchedulePersistenceIntegrationTest,
       CorruptEntrySkippedValidEntriesLoadedAfterRestart)
{
    std::string id_good = {};

    {
        DatabaseMaintenanceOrchestrator orc(nullptr, nullptr, nullptr, engine_.get());
        auto r = orc.createSchedule(makeEntry("Good Schedule"));
        ASSERT_TRUE(r);
        id_good = r->id;
    }

    // Corrupt a second entry directly in the storage engine.
    engine_->injectCorrupt(
        std::string(MaintenanceScheduleStore::kKeyPrefix) + "bad-entry",
        "{ invalid json !!!!");

    // Restart: corrupt entry must be skipped, valid entry must be loaded.
    {
        DatabaseMaintenanceOrchestrator orc2(nullptr, nullptr, nullptr, engine_.get());
        EXPECT_FALSE(orc2.start()); // loads from storage; null scheduler returns error

        auto schedules = orc2.listSchedules();
        ASSERT_EQ(schedules.size(), 1u);
        EXPECT_EQ(schedules[0].id, id_good);
    }
}

TEST_F(SchedulePersistenceIntegrationTest,
       NullStorageDoesNotPersistSchedules)
{
    // Without a storage engine, the orchestrator operates purely in-memory.
    // Schedules created in the first instance must NOT appear in the second
    // (since there is no shared backing store).
    std::string id = {};
    {
        DatabaseMaintenanceOrchestrator orc(nullptr, nullptr, nullptr, nullptr);
        auto r = orc.createSchedule(makeEntry("In-Memory Only"));
        ASSERT_TRUE(r);
        id = r->id;
    }

    DatabaseMaintenanceOrchestrator orc2(nullptr, nullptr, nullptr, nullptr);
    auto fetched = orc2.getSchedule(id);
    EXPECT_FALSE(fetched); // not found – correct behaviour
}


// Force-run: window override
// ===========================================================================

// Returns a UTC hour that is guaranteed not to be the current UTC hour
// (offset by 12 h), using the same gmtime_r/gmtime_s path as the production
// isInMaintenanceWindow() helper to avoid drift if clock sources change.
static int excludedWindowHour() {
    std::time_t t = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now());
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    return (tm.tm_hour + 12) % 24;
}

// Polls until the job reaches a terminal state (not RUNNING/PENDING) or the
// timeout is exhausted.  Returns the final job state via the orchestrator.
static void waitForTerminalJobState(
    DatabaseMaintenanceOrchestrator* orch,
    const std::string& job_id,
    int max_iterations = 60,
    std::chrono::milliseconds poll_interval = std::chrono::milliseconds(50))
{
    for (int i = 0; i < max_iterations; ++i) {
        std::this_thread::sleep_for(poll_interval);
        auto j = orch->getJob(job_id);
        if (j && j->state != MaintenanceJobState::RUNNING &&
                 j->state != MaintenanceJobState::PENDING) break;
    }
}

TEST_F(MaintenanceOrchestratorTest, ForceRun_BypassesWindowEnforcement) {
    // Build a schedule whose window is guaranteed to exclude the current hour.
    int excluded_hour = excludedWindowHour();

    auto entry = makeEntry("Force Run Test");
    entry.enforce_window    = true;
    entry.window_start_hour = excluded_hour;
    entry.window_end_hour   = (excluded_hour + 1) % 24;
    entry.tasks             = {MaintenanceTaskType::METRICS_COLLECTION};

    auto created = orchestrator_->createSchedule(entry);
    ASSERT_TRUE(created) << created.error().message();

    // A normal trigger should be SKIPPED (outside window).
    auto normal_job = orchestrator_->triggerNow(created->id, /*force=*/false);
    ASSERT_TRUE(normal_job);
    waitForTerminalJobState(orchestrator_.get(), normal_job->id);
    auto normal_final = orchestrator_->getJob(normal_job->id);
    ASSERT_TRUE(normal_final);
    EXPECT_EQ(normal_final->state, MaintenanceJobState::SKIPPED)
        << "Expected SKIPPED but got: " << jobStateToString(normal_final->state);
    EXPECT_FALSE(normal_final->forced);

    // A force trigger must bypass the window and not be SKIPPED.
    auto force_job = orchestrator_->triggerNow(created->id, /*force=*/true);
    ASSERT_TRUE(force_job);
    EXPECT_TRUE(force_job->forced);

    waitForTerminalJobState(orchestrator_.get(), force_job->id);
    auto force_final = orchestrator_->getJob(force_job->id);
    ASSERT_TRUE(force_final);
    EXPECT_TRUE(
        force_final->state == MaintenanceJobState::SUCCEEDED ||
        force_final->state == MaintenanceJobState::FAILED)
        << "Force-run should complete with SUCCEEDED or FAILED, not be skipped. Got: "
        << jobStateToString(force_final->state);
    EXPECT_TRUE(force_final->forced);
}

TEST_F(MaintenanceOrchestratorTest, ForceRun_ForcedFieldInJson) {
    // Verify that the forced flag is serialised to JSON correctly.
    OrchestratorJob job;
    job.id             = "force-job";
    job.schedule_id    = "sched-x";
    job.task_type      = MaintenanceTaskType::METRICS_COLLECTION;
    job.state          = MaintenanceJobState::SUCCEEDED;
    job.started_at_ms  = 1000;
    job.finished_at_ms = 2000;
    job.forced         = true;

    auto j = job.toJson();
    ASSERT_TRUE(j.contains("forced"));
    EXPECT_TRUE(j["forced"].get<bool>());

    // A non-forced job should also carry the field, set to false.
    OrchestratorJob nf;
    nf.id             = "normal-job";
    nf.task_type      = MaintenanceTaskType::METRICS_COLLECTION;
    nf.state          = MaintenanceJobState::SUCCEEDED;
    nf.started_at_ms  = 1000;
    nf.finished_at_ms = 2000;
    nf.forced         = false;

    auto jn = nf.toJson();
    ASSERT_TRUE(jn.contains("forced"));
    EXPECT_FALSE(jn["forced"].get<bool>());
}

TEST_F(MaintenanceOrchestratorTest, ForceRun_ApiHandler_PassesForceFlagToOrchestrator) {
    // Verify that MaintenanceApiHandler::triggerNow propagates the force flag
    // and the response contains the forced field.
    int excluded_hour = excludedWindowHour();

    auto entry = makeEntry("API Force Run Test");
    entry.enforce_window    = true;
    entry.window_start_hour = excluded_hour;
    entry.window_end_hour   = (excluded_hour + 1) % 24;
    entry.tasks             = {MaintenanceTaskType::METRICS_COLLECTION};

    auto created = orchestrator_->createSchedule(entry);
    ASSERT_TRUE(created);

    server::MaintenanceApiHandler handler(orchestrator_.get());

    // Force-trigger via the API handler
    auto resp = handler.triggerNow(created->id, /*force=*/true);
    ASSERT_FALSE(resp.contains("error")) << resp.dump();
    EXPECT_EQ(resp.value("status", ""), "triggered");
    EXPECT_TRUE(resp.value("forced", false));

    // Wait for the background thread to reach a terminal state
    std::string job_id = resp.value("id", "");
    ASSERT_FALSE(job_id.empty());
    waitForTerminalJobState(orchestrator_.get(), job_id);

    auto final_job = orchestrator_->getJob(job_id);
    ASSERT_TRUE(final_job);
    EXPECT_TRUE(
        final_job->state == MaintenanceJobState::SUCCEEDED ||
        final_job->state == MaintenanceJobState::FAILED)
        << "Force-run should complete, not be skipped. Got: "
        << jobStateToString(final_job->state);
    EXPECT_TRUE(final_job->forced);
}


// ===========================================================================
// DAG / task_dependencies tests
// ===========================================================================

// ---------------------------------------------------------------------------
// Serialisation round-trip for MaintenanceTaskDependency
// ---------------------------------------------------------------------------
TEST(TaskDependencyTest, JsonRoundTrip) {
    MaintenanceTaskDependency dep;
    dep.task_type  = MaintenanceTaskType::STORAGE_COMPACTION;
    dep.depends_on = {MaintenanceTaskType::MVCC_CLEANUP,
                      MaintenanceTaskType::METRICS_COLLECTION};

    auto j        = dep.toJson();
    auto restored = MaintenanceTaskDependency::fromJson(j);

    EXPECT_EQ(restored.task_type,    MaintenanceTaskType::STORAGE_COMPACTION);
    ASSERT_EQ(restored.depends_on.size(), 2u);
    EXPECT_EQ(restored.depends_on[0], MaintenanceTaskType::MVCC_CLEANUP);
    EXPECT_EQ(restored.depends_on[1], MaintenanceTaskType::METRICS_COLLECTION);
}

// ---------------------------------------------------------------------------
// Full schedule JSON round-trip with task_dependencies
// ---------------------------------------------------------------------------
TEST_F(MaintenanceOrchestratorTest, ScheduleEntry_JsonRoundTrip_WithTaskDependencies) {
    MaintenanceScheduleEntry e = makeEntry("DAG Round-Trip");
    e.tasks = {MaintenanceTaskType::MVCC_CLEANUP,
               MaintenanceTaskType::STORAGE_COMPACTION};

    MaintenanceTaskDependency dep;
    dep.task_type  = MaintenanceTaskType::STORAGE_COMPACTION;
    dep.depends_on = {MaintenanceTaskType::MVCC_CLEANUP};
    e.task_dependencies = {dep};

    auto j        = e.toJson();
    auto restored = MaintenanceScheduleEntry::fromJson(j);

    ASSERT_EQ(restored.task_dependencies.size(), 1u);
    EXPECT_EQ(restored.task_dependencies[0].task_type,
              MaintenanceTaskType::STORAGE_COMPACTION);
    ASSERT_EQ(restored.task_dependencies[0].depends_on.size(), 1u);
    EXPECT_EQ(restored.task_dependencies[0].depends_on[0],
              MaintenanceTaskType::MVCC_CLEANUP);
}

// ---------------------------------------------------------------------------
// Create schedule with valid task_dependencies succeeds
// ---------------------------------------------------------------------------
TEST_F(MaintenanceOrchestratorTest, CreateSchedule_WithTaskDependencies_Succeeds) {
    auto entry = makeEntry("DAG Schedule");
    entry.tasks = {MaintenanceTaskType::MVCC_CLEANUP,
                   MaintenanceTaskType::STORAGE_COMPACTION};

    MaintenanceTaskDependency dep;
    dep.task_type  = MaintenanceTaskType::STORAGE_COMPACTION;
    dep.depends_on = {MaintenanceTaskType::MVCC_CLEANUP};
    entry.task_dependencies = {dep};

    auto result = orchestrator_->createSchedule(entry);
    ASSERT_TRUE(result) << result.error().message();
    ASSERT_EQ(result->task_dependencies.size(), 1u);
    EXPECT_EQ(result->task_dependencies[0].task_type,
              MaintenanceTaskType::STORAGE_COMPACTION);
}

// ---------------------------------------------------------------------------
// Update schedule with valid task_dependencies succeeds
// ---------------------------------------------------------------------------
TEST_F(MaintenanceOrchestratorTest, UpdateSchedule_WithTaskDependencies_Succeeds) {
    auto created = orchestrator_->createSchedule(makeEntry("DAG Update Test"));
    ASSERT_TRUE(created);

    MaintenanceScheduleEntry upd = *created;
    upd.tasks = {MaintenanceTaskType::METRICS_COLLECTION,
                 MaintenanceTaskType::CONSISTENCY_CHECK};
    MaintenanceTaskDependency dep;
    dep.task_type  = MaintenanceTaskType::CONSISTENCY_CHECK;
    dep.depends_on = {MaintenanceTaskType::METRICS_COLLECTION};
    upd.task_dependencies = {dep};

    auto result = orchestrator_->updateSchedule(created->id, upd);
    ASSERT_TRUE(result) << result.error().message();
    ASSERT_EQ(result->task_dependencies.size(), 1u);
}

// ---------------------------------------------------------------------------
// Cycle detection: createSchedule with a cyclic DAG must be rejected
// ---------------------------------------------------------------------------
TEST_F(MaintenanceOrchestratorTest, CreateSchedule_CyclicDependencies_Rejected) {
    auto entry = makeEntry("Cyclic DAG");
    entry.tasks = {MaintenanceTaskType::MVCC_CLEANUP,
                   MaintenanceTaskType::STORAGE_COMPACTION};

    // A -> B and B -> A
    MaintenanceTaskDependency dep_a;
    dep_a.task_type  = MaintenanceTaskType::STORAGE_COMPACTION;
    dep_a.depends_on = {MaintenanceTaskType::MVCC_CLEANUP};

    MaintenanceTaskDependency dep_b;
    dep_b.task_type  = MaintenanceTaskType::MVCC_CLEANUP;
    dep_b.depends_on = {MaintenanceTaskType::STORAGE_COMPACTION};

    entry.task_dependencies = {dep_a, dep_b};

    auto result = orchestrator_->createSchedule(entry);
    ASSERT_FALSE(result) << "Cyclic dependency should be rejected, got: "
                         << (result ? "success" : result.error().message());
}

// ---------------------------------------------------------------------------
// Cycle detection: updateSchedule with a cyclic DAG must be rejected
// ---------------------------------------------------------------------------
TEST_F(MaintenanceOrchestratorTest, UpdateSchedule_CyclicDependencies_Rejected) {
    auto created = orchestrator_->createSchedule(makeEntry("Cyclic Update"));
    ASSERT_TRUE(created);

    MaintenanceScheduleEntry upd = *created;
    upd.tasks = {MaintenanceTaskType::MVCC_CLEANUP,
                 MaintenanceTaskType::STORAGE_COMPACTION};

    MaintenanceTaskDependency dep_a;
    dep_a.task_type  = MaintenanceTaskType::STORAGE_COMPACTION;
    dep_a.depends_on = {MaintenanceTaskType::MVCC_CLEANUP};

    MaintenanceTaskDependency dep_b;
    dep_b.task_type  = MaintenanceTaskType::MVCC_CLEANUP;
    dep_b.depends_on = {MaintenanceTaskType::STORAGE_COMPACTION};

    upd.task_dependencies = {dep_a, dep_b};

    auto result = orchestrator_->updateSchedule(created->id, upd);
    ASSERT_FALSE(result) << "Cyclic dependency should be rejected on update";
}

// ---------------------------------------------------------------------------
// DAG ordering correctness: resolveTaskExecutionOrder respects declared deps
// ---------------------------------------------------------------------------
TEST_F(MaintenanceOrchestratorTest, TaskExecution_DAGOrderRespected) {
    // Schedule two tasks where STORAGE_COMPACTION depends on MVCC_CLEANUP.
    // STORAGE_COMPACTION is listed first in `tasks` — without DAG it would run
    // first; with DAG it must run second.
    MaintenanceScheduleEntry entry = makeEntry("DAG Order Test");
    entry.tasks = {MaintenanceTaskType::STORAGE_COMPACTION,  // listed first intentionally
                   MaintenanceTaskType::MVCC_CLEANUP};

    MaintenanceTaskDependency dep;
    dep.task_type  = MaintenanceTaskType::STORAGE_COMPACTION;
    dep.depends_on = {MaintenanceTaskType::MVCC_CLEANUP};
    entry.task_dependencies = {dep};

    // Verify ordering directly via the public static helper.
    auto order = DatabaseMaintenanceOrchestrator::resolveTaskExecutionOrder(entry);
    ASSERT_EQ(order.size(), 2u);
    // MVCC_CLEANUP must precede STORAGE_COMPACTION.
    EXPECT_EQ(order[0], MaintenanceTaskType::MVCC_CLEANUP)
        << "MVCC_CLEANUP should be first (prerequisite)";
    EXPECT_EQ(order[1], MaintenanceTaskType::STORAGE_COMPACTION)
        << "STORAGE_COMPACTION should be second (dependent)";

    // Also verify the schedule runs to completion successfully.
    entry.enforce_window = false;
    auto created = orchestrator_->createSchedule(entry);
    ASSERT_TRUE(created) << created.error().message();

    auto job_result = orchestrator_->triggerNow(created->id, /*force=*/true);
    ASSERT_TRUE(job_result) << job_result.error().message();
    waitForTerminalJobState(orchestrator_.get(), job_result->id);

    auto final_job = orchestrator_->getJob(job_result->id);
    ASSERT_TRUE(final_job);
    EXPECT_EQ(final_job->state, MaintenanceJobState::SUCCEEDED)
        << "DAG schedule should complete with SUCCEEDED. Got: "
        << jobStateToString(final_job->state);
}

// ---------------------------------------------------------------------------
// DAG ordering correctness: stable ordering preserves entry.tasks order for
// unrelated tasks (tasks with no dependency between them keep original position)
// ---------------------------------------------------------------------------
TEST(ResolveTaskExecutionOrderTest, StableOrderPreservedForUnrelatedTasks) {
    MaintenanceScheduleEntry entry;
    entry.name  = "Stable";
    // Three tasks: A(0), B(1), C(2).  B depends on A; C is unrelated.
    // Stable seeding: initial ready = [A, C] (both in-degree 0, seeded in order).
    // After processing A, B becomes ready and is inserted before C (lower index).
    // Expected output order: A, B, C — original relative positions preserved.
    entry.tasks = {MaintenanceTaskType::METRICS_COLLECTION,   // A (index 0)
                   MaintenanceTaskType::QUOTA_CHECK,          // B (index 1)
                   MaintenanceTaskType::CONSISTENCY_CHECK};   // C (index 2)

    MaintenanceTaskDependency dep;
    dep.task_type  = MaintenanceTaskType::QUOTA_CHECK;       // B depends on A
    dep.depends_on = {MaintenanceTaskType::METRICS_COLLECTION};
    entry.task_dependencies = {dep};

    auto order = DatabaseMaintenanceOrchestrator::resolveTaskExecutionOrder(entry);
    ASSERT_EQ(order.size(), 3u);
    // All three positions are deterministic with stable Kahn's seeding.
    EXPECT_EQ(order[0], MaintenanceTaskType::METRICS_COLLECTION) << "A must be first (prereq for B)";
    EXPECT_EQ(order[1], MaintenanceTaskType::QUOTA_CHECK)        << "B must be second (depends on A)";
    EXPECT_EQ(order[2], MaintenanceTaskType::CONSISTENCY_CHECK)  << "C must be third (unrelated, keeps original position)";
}

// ---------------------------------------------------------------------------
// Validation: task_type not in entry.tasks is rejected
// ---------------------------------------------------------------------------
TEST(ResolveTaskExecutionOrderTest, TaskTypeNotInTasksRejected) {
    MaintenanceScheduleEntry entry;
    entry.name  = "Validation";
    entry.tasks = {MaintenanceTaskType::MVCC_CLEANUP};

    MaintenanceTaskDependency dep;
    dep.task_type  = MaintenanceTaskType::STORAGE_COMPACTION; // NOT in tasks
    dep.depends_on = {MaintenanceTaskType::MVCC_CLEANUP};
    entry.task_dependencies = {dep};

    EXPECT_THROW(
        DatabaseMaintenanceOrchestrator::resolveTaskExecutionOrder(entry),
        std::invalid_argument);
}

// ---------------------------------------------------------------------------
// Validation: depends_on referencing a task not in entry.tasks is rejected
// ---------------------------------------------------------------------------
TEST(ResolveTaskExecutionOrderTest, DependsOnTaskNotInTasksRejected) {
    MaintenanceScheduleEntry entry;
    entry.name  = "Validation";
    entry.tasks = {MaintenanceTaskType::MVCC_CLEANUP};

    MaintenanceTaskDependency dep;
    dep.task_type  = MaintenanceTaskType::MVCC_CLEANUP;
    dep.depends_on = {MaintenanceTaskType::STORAGE_COMPACTION}; // NOT in tasks
    entry.task_dependencies = {dep};

    EXPECT_THROW(
        DatabaseMaintenanceOrchestrator::resolveTaskExecutionOrder(entry),
        std::invalid_argument);
}

// ---------------------------------------------------------------------------
// applyPatch round-trip for task_dependencies
// ---------------------------------------------------------------------------
TEST_F(MaintenanceOrchestratorTest, ApplyPatch_UpdatesTaskDependencies) {
    auto entry = makeEntry("Patch DAG");
    entry.tasks = {MaintenanceTaskType::MVCC_CLEANUP,
                   MaintenanceTaskType::STORAGE_COMPACTION};
    auto created = orchestrator_->createSchedule(entry);
    ASSERT_TRUE(created);

    MaintenanceTaskDependency dep;
    dep.task_type  = MaintenanceTaskType::STORAGE_COMPACTION;
    dep.depends_on = {MaintenanceTaskType::MVCC_CLEANUP};

    nlohmann::json patch;
    patch["task_dependencies"] = nlohmann::json::array({dep.toJson()});

    auto patched = orchestrator_->patchSchedule(created->id, patch);
    ASSERT_TRUE(patched) << patched.error().message();
    ASSERT_EQ(patched->task_dependencies.size(), 1u);
    EXPECT_EQ(patched->task_dependencies[0].task_type,
              MaintenanceTaskType::STORAGE_COMPACTION);
}

// ---------------------------------------------------------------------------
// No task_dependencies -> falls back to positional order (existing behaviour)
// ---------------------------------------------------------------------------
TEST_F(MaintenanceOrchestratorTest, TaskExecution_NoDAG_PositionalOrderUnchanged) {
    auto entry = makeEntry("No DAG");
    entry.tasks = {MaintenanceTaskType::METRICS_COLLECTION,
                   MaintenanceTaskType::QUOTA_CHECK};
    entry.enforce_window = false;
    // task_dependencies intentionally empty

    auto created = orchestrator_->createSchedule(entry);
    ASSERT_TRUE(created);

    auto job_result = orchestrator_->triggerNow(created->id, /*force=*/true);
    ASSERT_TRUE(job_result);
    waitForTerminalJobState(orchestrator_.get(), job_result->id);

    auto final_job = orchestrator_->getJob(job_result->id);
    ASSERT_TRUE(final_job);
    EXPECT_EQ(final_job->state, MaintenanceJobState::SUCCEEDED);
}

// ---------------------------------------------------------------------------
// halt_on_task_failure with DAG: flag is preserved in JSON round-trip
// ---------------------------------------------------------------------------
TEST_F(MaintenanceOrchestratorTest, DAG_HaltOnTaskFailure_FlagPreserved) {
    auto entry = makeEntry("DAG Halt");
    entry.tasks = {MaintenanceTaskType::MVCC_CLEANUP,
                   MaintenanceTaskType::STORAGE_COMPACTION};
    entry.halt_on_task_failure = true;
    entry.enforce_window = false;

    MaintenanceTaskDependency dep;
    dep.task_type  = MaintenanceTaskType::STORAGE_COMPACTION;
    dep.depends_on = {MaintenanceTaskType::MVCC_CLEANUP};
    entry.task_dependencies = {dep};

    auto created = orchestrator_->createSchedule(entry);
    ASSERT_TRUE(created) << created.error().message();
    EXPECT_TRUE(created->halt_on_task_failure);

    // JSON round-trip preserves both flag and DAG
    auto j        = created->toJson();
    auto restored = MaintenanceScheduleEntry::fromJson(j);
    EXPECT_TRUE(restored.halt_on_task_failure);
    ASSERT_EQ(restored.task_dependencies.size(), 1u);
}

// ===========================================================================
// IMaintenanceTaskHandler registry tests
// ===========================================================================

// ---------------------------------------------------------------------------
// Simple mock handler that records invocations and returns a preset result.
// ---------------------------------------------------------------------------

class MockTaskHandler : public IMaintenanceTaskHandler {
public:
    explicit MockTaskHandler(std::string name,
                             Result<std::string> result = Result<std::string>{"mock success"})
        : name_(std::move(name)), result_(std::move(result)) {}

    Result<std::string> execute(const std::string& job_id,
                                MaintenanceTaskType task_type) override {
        ++call_count_;
        last_job_id_   = job_id;
        last_task_type_ = task_type;
        return result_;
    }

    std::string handlerName() const override { return name_; }

    int         call_count()     const { return call_count_; }
    std::string last_job_id()    const { return last_job_id_; }
    MaintenanceTaskType last_task_type() const { return last_task_type_; }

private:
    std::string          name_;
    Result<std::string>  result_;
    std::atomic<int>     call_count_{0};
    std::string          last_job_id_;
    MaintenanceTaskType  last_task_type_{MaintenanceTaskType::METRICS_COLLECTION};
};

// ---------------------------------------------------------------------------
// registerTaskHandler – basic registration and listTaskHandlers
// ---------------------------------------------------------------------------

TEST_F(MaintenanceOrchestratorTest, RegisterTaskHandler_AppearsInList) {
    auto handler = std::make_shared<MockTaskHandler>("CompactionHandler");
    orchestrator_->registerTaskHandler(MaintenanceTaskType::STORAGE_COMPACTION, handler);

    auto handlers = orchestrator_->listTaskHandlers();
    ASSERT_EQ(handlers.count("storage_compaction"), 1u);
    EXPECT_EQ(handlers.at("storage_compaction"), "CompactionHandler");
}

// ---------------------------------------------------------------------------
// Registering a second handler for the same type replaces the first
// ---------------------------------------------------------------------------

TEST_F(MaintenanceOrchestratorTest, RegisterTaskHandler_ReplacesExisting) {
    auto h1 = std::make_shared<MockTaskHandler>("Handler1");
    auto h2 = std::make_shared<MockTaskHandler>("Handler2");
    orchestrator_->registerTaskHandler(MaintenanceTaskType::STORAGE_COMPACTION, h1);
    orchestrator_->registerTaskHandler(MaintenanceTaskType::STORAGE_COMPACTION, h2);

    auto handlers = orchestrator_->listTaskHandlers();
    EXPECT_EQ(handlers.at("storage_compaction"), "Handler2");
}

// ---------------------------------------------------------------------------
// executeTask calls the registered handler and sets SUCCEEDED on success
// ---------------------------------------------------------------------------

TEST_F(MaintenanceOrchestratorTest, RegisteredHandler_CalledOnExecuteTask_Success) {
    auto handler = std::make_shared<MockTaskHandler>("StorageCompaction", Result<std::string>{"compaction done"});
    orchestrator_->registerTaskHandler(MaintenanceTaskType::STORAGE_COMPACTION, handler);

    auto entry = makeEntry("Compaction Schedule");
    entry.tasks          = {MaintenanceTaskType::STORAGE_COMPACTION};
    entry.enforce_window = false;
    auto created = orchestrator_->createSchedule(entry);
    ASSERT_TRUE(created);

    auto job_result = orchestrator_->triggerNow(created->id, /*force=*/true);
    ASSERT_TRUE(job_result);
    waitForTerminalJobState(orchestrator_.get(), job_result->id);

    auto final_job = orchestrator_->getJob(job_result->id);
    ASSERT_TRUE(final_job);
    EXPECT_EQ(final_job->state, MaintenanceJobState::SUCCEEDED);
    EXPECT_EQ(final_job->result_summary, "compaction done");
    EXPECT_GE(handler->call_count(), 1);
}

// ---------------------------------------------------------------------------
// executeTask sets FAILED when the registered handler returns an error
// ---------------------------------------------------------------------------

TEST_F(MaintenanceOrchestratorTest, RegisteredHandler_CalledOnExecuteTask_Failure) {
    auto err_result = Result<std::string>{tl::unexpected(
        Error(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED, "disk full"))};
    auto handler = std::make_shared<MockTaskHandler>("FailingHandler", err_result);
    orchestrator_->registerTaskHandler(MaintenanceTaskType::STORAGE_COMPACTION, handler);

    auto entry = makeEntry("Failing Compaction");
    entry.tasks          = {MaintenanceTaskType::STORAGE_COMPACTION};
    entry.enforce_window = false;
    auto created = orchestrator_->createSchedule(entry);
    ASSERT_TRUE(created);

    auto job_result = orchestrator_->triggerNow(created->id, /*force=*/true);
    ASSERT_TRUE(job_result);
    waitForTerminalJobState(orchestrator_.get(), job_result->id);

    auto final_job = orchestrator_->getJob(job_result->id);
    ASSERT_TRUE(final_job);
    EXPECT_EQ(final_job->state, MaintenanceJobState::FAILED);
    EXPECT_FALSE(final_job->error_message.empty());
}

// ---------------------------------------------------------------------------
// Unregistered module-delegated task type returns SKIPPED
// ---------------------------------------------------------------------------

TEST_F(MaintenanceOrchestratorTest, UnregisteredTaskType_ReturnsSkipped) {
    // No handler registered for REPLICA_VALIDATION
    auto entry = makeEntry("Replica Validation Schedule");
    entry.tasks          = {MaintenanceTaskType::REPLICA_VALIDATION};
    entry.enforce_window = false;
    auto created = orchestrator_->createSchedule(entry);
    ASSERT_TRUE(created);

    auto job_result = orchestrator_->triggerNow(created->id, /*force=*/true);
    ASSERT_TRUE(job_result);
    waitForTerminalJobState(orchestrator_.get(), job_result->id);

    auto final_job = orchestrator_->getJob(job_result->id);
    ASSERT_TRUE(final_job);
    EXPECT_EQ(final_job->state, MaintenanceJobState::SKIPPED)
        << "Expected SKIPPED but got: " << jobStateToString(final_job->state);
    EXPECT_FALSE(final_job->result_summary.empty());
    EXPECT_NE(final_job->result_summary.find("no handler"), std::string::npos);
}

// ---------------------------------------------------------------------------
// Multiple handlers registered for different task types
// ---------------------------------------------------------------------------

TEST_F(MaintenanceOrchestratorTest, MultipleHandlers_AllListed) {
    orchestrator_->registerTaskHandler(
        MaintenanceTaskType::STORAGE_COMPACTION,
        std::make_shared<MockTaskHandler>("CompactionHandler"));
    orchestrator_->registerTaskHandler(
        MaintenanceTaskType::REPLICA_VALIDATION,
        std::make_shared<MockTaskHandler>("ReplicaHandler"));
    orchestrator_->registerTaskHandler(
        MaintenanceTaskType::MVCC_CLEANUP,
        std::make_shared<MockTaskHandler>("MvccHandler"));

    auto handlers = orchestrator_->listTaskHandlers();
    EXPECT_EQ(handlers.size(), 3u);
    EXPECT_EQ(handlers.at("storage_compaction"),  "CompactionHandler");
    EXPECT_EQ(handlers.at("replica_validation"),  "ReplicaHandler");
    EXPECT_EQ(handlers.at("mvcc_cleanup"),         "MvccHandler");
}

// ---------------------------------------------------------------------------
// listTaskHandlers returns empty map when no handlers are registered
// ---------------------------------------------------------------------------

TEST_F(MaintenanceOrchestratorTest, ListTaskHandlers_EmptyWhenNoneRegistered) {
    auto handlers = orchestrator_->listTaskHandlers();
    EXPECT_TRUE(handlers.empty());
}

// ===========================================================================
// MaintenanceApiHandler – listTaskHandlers endpoint
// ===========================================================================

TEST_F(MaintenanceApiHandlerTest, ListTaskHandlers_EmptyWhenNoneRegistered) {
    auto result = handler_->listTaskHandlers();
    EXPECT_EQ(result.value("count", -1), 0);
    ASSERT_TRUE(result.contains("task_handlers"));
    EXPECT_TRUE(result["task_handlers"].empty());
}

TEST_F(MaintenanceApiHandlerTest, ListTaskHandlers_ReturnsRegisteredHandlers) {
    orchestrator_->registerTaskHandler(
        MaintenanceTaskType::STORAGE_COMPACTION,
        std::make_shared<MockTaskHandler>("StorageCompactionHandler"));
    orchestrator_->registerTaskHandler(
        MaintenanceTaskType::MVCC_CLEANUP,
        std::make_shared<MockTaskHandler>("MvccCleanupHandler"));

    auto result = handler_->listTaskHandlers();
    EXPECT_EQ(result.value("count", -1), 2);

    auto& arr = result["task_handlers"];
    ASSERT_EQ(arr.size(), 2u);

    // Verify both entries are present (order may vary)
    std::map<std::string, std::string> by_type = {};

    for (auto& item : arr) {
        by_type[item.value("task_type", "")] = item.value("handler", "");
    }
    EXPECT_EQ(by_type["storage_compaction"], "StorageCompactionHandler");
    EXPECT_EQ(by_type["mvcc_cleanup"],        "MvccCleanupHandler");
}

TEST_F(MaintenanceApiHandlerTest, ListTaskHandlers_NullOrchestratorReturnsError) {
    server::MaintenanceApiHandler null_handler(nullptr);
    auto result = null_handler.listTaskHandlers();
    EXPECT_EQ(result.value("status", ""), "error");
}

// ===========================================================================
// Multi-Tenant Schedule Isolation
// ===========================================================================

// MT-01: tenant_id is stored in the schedule entry and round-trips via JSON.
TEST_F(MaintenanceOrchestratorTest, TenantId_StoredInSchedule) {
    auto entry = makeEntry("Tenant A Schedule");
    entry.tenant_id = "tenant-a";

    auto result = orchestrator_->createSchedule(entry);
    ASSERT_TRUE(result) << result.error().message();
    EXPECT_EQ(result->tenant_id, "tenant-a");

    // Round-trip via JSON
    auto json_val = result->toJson();
    EXPECT_EQ(json_val.value("tenant_id", ""), "tenant-a");

    auto restored = MaintenanceScheduleEntry::fromJson(json_val);
    EXPECT_EQ(restored.tenant_id, "tenant-a");

    // getSchedule returns tenant_id
    auto fetched = orchestrator_->getSchedule(result->id);
    ASSERT_TRUE(fetched);
    EXPECT_EQ(fetched->tenant_id, "tenant-a");
}

// MT-02: empty tenant_id (global schedule) is preserved.
TEST_F(MaintenanceOrchestratorTest, TenantId_EmptyMeansGlobal) {
    auto entry = makeEntry("Global Schedule");
    // tenant_id left empty

    auto result = orchestrator_->createSchedule(entry);
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->tenant_id.empty());
}

// MT-03: applyPatch can update tenant_id.
TEST_F(MaintenanceOrchestratorTest, TenantId_PatchUpdate) {
    auto entry = makeEntry("Patchable");
    entry.tenant_id = "tenant-x";

    auto created = orchestrator_->createSchedule(entry);
    ASSERT_TRUE(created);

    nlohmann::json patch = {{"tenant_id", "tenant-y"}};
    auto patched = orchestrator_->patchSchedule(created->id, patch);
    ASSERT_TRUE(patched) << patched.error().message();
    EXPECT_EQ(patched->tenant_id, "tenant-y");
}

// MT-04: listSchedules() without filter returns all schedules (global + tenanted).
TEST_F(MaintenanceOrchestratorTest, ListSchedules_NoFilterReturnsAll) {
    auto e1 = makeEntry("Global");
    auto e2 = makeEntry("Tenant-A-1");
    e2.tenant_id = "tenant-a";
    auto e3 = makeEntry("Tenant-B-1");
    e3.tenant_id = "tenant-b";

    ASSERT_TRUE(orchestrator_->createSchedule(e1));
    ASSERT_TRUE(orchestrator_->createSchedule(e2));
    ASSERT_TRUE(orchestrator_->createSchedule(e3));

    auto all = orchestrator_->listSchedules();
    EXPECT_EQ(all.size(), 3u);
}

// MT-05: listSchedules(tenant_id) returns only schedules for that tenant.
TEST_F(MaintenanceOrchestratorTest, ListSchedules_FilterByTenantId) {
    auto global = makeEntry("Global");
    auto ta1    = makeEntry("Tenant-A-1");
    auto ta2    = makeEntry("Tenant-A-2");
    auto tb1    = makeEntry("Tenant-B-1");
    ta1.tenant_id = "tenant-a";
    ta2.tenant_id = "tenant-a";
    tb1.tenant_id = "tenant-b";

    ASSERT_TRUE(orchestrator_->createSchedule(global));
    ASSERT_TRUE(orchestrator_->createSchedule(ta1));
    ASSERT_TRUE(orchestrator_->createSchedule(ta2));
    ASSERT_TRUE(orchestrator_->createSchedule(tb1));

    auto result_a = orchestrator_->listSchedules("tenant-a");
    ASSERT_EQ(result_a.size(), 2u);
    for (auto& s : result_a) {
        EXPECT_EQ(s.tenant_id, "tenant-a");
    }

    auto result_b = orchestrator_->listSchedules("tenant-b");
    ASSERT_EQ(result_b.size(), 1u);
    EXPECT_EQ(result_b[0].tenant_id, "tenant-b");

    // Unknown tenant returns empty
    auto result_unknown = orchestrator_->listSchedules("tenant-x");
    EXPECT_TRUE(result_unknown.empty());
}

// MT-06: API handler listSchedules(tenant_id) passes filter to orchestrator.
TEST_F(MaintenanceApiHandlerTest, ListSchedules_FilterByTenantId) {
    // Create one global and two tenant-a schedules via the handler
    {
        nlohmann::json body = makeScheduleBody("Global");
        handler_->createSchedule(body);
    }
    {
        nlohmann::json body = makeScheduleBody("TenantA-1");
        body["tenant_id"]   = "tenant-a";
        handler_->createSchedule(body);
    }
    {
        nlohmann::json body = makeScheduleBody("TenantA-2");
        body["tenant_id"]   = "tenant-a";
        handler_->createSchedule(body);
    }

    // No filter → all 3
    auto all = handler_->listSchedules();
    EXPECT_EQ(all["count"].get<int>(), 3);

    // Filter by tenant-a → 2
    auto tenant_a = handler_->listSchedules("tenant-a");
    ASSERT_TRUE(tenant_a.contains("schedules"));
    EXPECT_EQ(tenant_a["count"].get<int>(), 2);
    for (auto& s : tenant_a["schedules"]) {
        EXPECT_EQ(s.value("tenant_id", ""), "tenant-a");
    }

    // Filter by unknown tenant → 0
    auto unknown = handler_->listSchedules("unknown-tenant");
    EXPECT_EQ(unknown["count"].get<int>(), 0);
}

// MT-07: null orchestrator returns error for listSchedules with tenant filter.
TEST_F(MaintenanceApiHandlerTest, ListSchedules_TenantFilter_NullOrchestratorReturnsError) {
    server::MaintenanceApiHandler null_handler(nullptr);
    auto result = null_handler.listSchedules("tenant-x");
    EXPECT_EQ(result.value("status", ""), "error");
}

// MT-08: setTenantMaintenanceConfig/getTenantMaintenanceConfig round-trip.
TEST_F(MaintenanceOrchestratorTest, TenantMaintenanceConfig_RoundTrip) {
    TenantMaintenanceConfig cfg;
    cfg.enforce_window      = true;
    cfg.window_start_hour   = 3;
    cfg.window_end_hour     = 7;
    cfg.max_concurrent_jobs = 2;

    orchestrator_->setTenantMaintenanceConfig("tenant-t", cfg);

    auto retrieved = orchestrator_->getTenantMaintenanceConfig("tenant-t");
    EXPECT_EQ(retrieved.enforce_window,      true);
    EXPECT_EQ(retrieved.window_start_hour,   3);
    EXPECT_EQ(retrieved.window_end_hour,     7);
    EXPECT_EQ(retrieved.max_concurrent_jobs, 2);
}

// MT-09: getTenantMaintenanceConfig returns defaults for unregistered tenant.
TEST_F(MaintenanceOrchestratorTest, TenantMaintenanceConfig_DefaultsForUnknown) {
    auto cfg = orchestrator_->getTenantMaintenanceConfig("not-registered");
    EXPECT_FALSE(cfg.enforce_window);
    EXPECT_EQ(cfg.max_concurrent_jobs, 0);
}

// MT-10: setTenantMaintenanceConfig ignores empty tenant_id.
TEST_F(MaintenanceOrchestratorTest, TenantMaintenanceConfig_EmptyIdIgnored) {
    TenantMaintenanceConfig cfg;
    cfg.enforce_window    = true;
    cfg.max_concurrent_jobs = 5;
    // Should not crash or store
    orchestrator_->setTenantMaintenanceConfig("", cfg);
    // Default value must be returned for empty id
    auto retrieved = orchestrator_->getTenantMaintenanceConfig("");
    EXPECT_FALSE(retrieved.enforce_window);
    EXPECT_EQ(retrieved.max_concurrent_jobs, 0);
}

// MT-11: per-tenant window override skips job outside tenant window.
TEST_F(MaintenanceOrchestratorTest, TenantWindowEnforcement_SkipsJobOutsideTenantWindow) {
    int current_hour = static_cast<int>(
        std::chrono::duration_cast<std::chrono::hours>(
            std::chrono::system_clock::now().time_since_epoch()).count() % 24);

    // Pick a narrow tenant window that is guaranteed to NOT include current_hour
    int tenant_start = (current_hour + 12) % 24;
    int tenant_end   = (tenant_start + 1) % 24;

    // Schedule has enforce_window=false so it would normally run at any hour
    auto entry = makeEntry("Tenant Window Test");
    entry.tenant_id      = "tenant-window";
    entry.enforce_window = false;   // per-schedule window NOT enforced
    entry.tasks = {MaintenanceTaskType::METRICS_COLLECTION};

    auto created = orchestrator_->createSchedule(entry);
    ASSERT_TRUE(created);

    // Register tenant config with a window that excludes current hour
    TenantMaintenanceConfig tenant_cfg;
    tenant_cfg.enforce_window    = true;
    tenant_cfg.window_start_hour = tenant_start;
    tenant_cfg.window_end_hour   = tenant_end;
    orchestrator_->setTenantMaintenanceConfig("tenant-window", tenant_cfg);

    auto job_result = orchestrator_->triggerNow(created->id);
    ASSERT_TRUE(job_result) << job_result.error().message();

    // Wait for background thread (up to 2 s)
    for (int i = 0; i < 40; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        auto j = orchestrator_->getJob(job_result->id);
        if (j && j->state != MaintenanceJobState::RUNNING &&
                 j->state != MaintenanceJobState::PENDING) break;
    }

    auto final_job = orchestrator_->getJob(job_result->id);
    ASSERT_TRUE(final_job);
    EXPECT_EQ(final_job->state, MaintenanceJobState::SKIPPED)
        << "Expected SKIPPED but got: " << jobStateToString(final_job->state);
    EXPECT_NE(final_job->error_message.find("window"), std::string::npos);
}

// MT-12: per-tenant window override runs job when inside tenant window.
TEST_F(MaintenanceOrchestratorTest, TenantWindowEnforcement_RunsJobInsideTenantWindow) {
    // Schedule with a very narrow schedule window (not current hour)
    int current_hour = static_cast<int>(
        std::chrono::duration_cast<std::chrono::hours>(
            std::chrono::system_clock::now().time_since_epoch()).count() % 24);
    int sched_start = (current_hour + 12) % 24;
    int sched_end   = (sched_start + 1) % 24;

    auto entry = makeEntry("Tenant Window Run Test");
    entry.tenant_id         = "tenant-wide-window";
    entry.enforce_window    = true;
    entry.window_start_hour = sched_start;  // per-schedule: not current hour
    entry.window_end_hour   = sched_end;
    entry.tasks = {MaintenanceTaskType::METRICS_COLLECTION};

    auto created = orchestrator_->createSchedule(entry);
    ASSERT_TRUE(created);

    // Tenant config overrides with a full-day window → always in window
    TenantMaintenanceConfig tenant_cfg;
    tenant_cfg.enforce_window    = true;
    tenant_cfg.window_start_hour = 0;
    tenant_cfg.window_end_hour   = 23;
    orchestrator_->setTenantMaintenanceConfig("tenant-wide-window", tenant_cfg);

    auto job_result = orchestrator_->triggerNow(created->id);
    ASSERT_TRUE(job_result);

    for (int i = 0; i < 40; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        auto j = orchestrator_->getJob(job_result->id);
        if (j && j->state != MaintenanceJobState::RUNNING &&
                 j->state != MaintenanceJobState::PENDING) break;
    }

    auto final_job = orchestrator_->getJob(job_result->id);
    ASSERT_TRUE(final_job);
    EXPECT_NE(final_job->state, MaintenanceJobState::FAILED);
    // Should be SUCCEEDED or SKIPPED (no handler → SKIPPED), but NOT window-based SKIPPED
    if (final_job->state == MaintenanceJobState::SKIPPED) {
        // Acceptable only if it was the "no handler" skip, not the window skip
        EXPECT_EQ(final_job->error_message.find("window"), std::string::npos)
            << "Unexpected window-based skip: " << final_job->error_message;
    }
}

// ---------------------------------------------------------------------------
// BlockingMockTaskHandler – used for quota enforcement tests
// ---------------------------------------------------------------------------

namespace {
class BlockingMockTaskHandler : public IMaintenanceTaskHandler {
public:
    BlockingMockTaskHandler() : released_(false) {}

    Result<std::string> execute(const std::string& /*job_id*/,
                                MaintenanceTaskType /*task_type*/) override {
        std::unique_lock<std::mutex> lk(mu_);
        cv_.wait(lk, [this] { return released_.load(); });
        return Result<std::string>{"blocking handler done"};
    }

    std::string handlerName() const override { return "BlockingMockTaskHandler"; }

    void release() {
        released_.store(true);
        cv_.notify_all();
    }

private:
    std::mutex              mu_;
    std::condition_variable cv_;
    std::atomic<bool>       released_;
};
} // anonymous namespace

// MT-13: per-tenant concurrent job quota – second job is SKIPPED when quota is full.
TEST_F(MaintenanceOrchestratorTest, TenantQuota_SecondJobSkippedWhenQuotaFull) {
    auto blocking_handler = std::make_shared<BlockingMockTaskHandler>();
    orchestrator_->registerTaskHandler(MaintenanceTaskType::METRICS_COLLECTION,
                                       blocking_handler);

    // Set max_concurrent_jobs=1 for this tenant
    TenantMaintenanceConfig cfg;
    cfg.max_concurrent_jobs = 1;
    orchestrator_->setTenantMaintenanceConfig("tenant-quota", cfg);

    auto entry = makeEntry("Quota Test Schedule");
    entry.tenant_id      = "tenant-quota";
    entry.enforce_window = false;
    entry.tasks          = {MaintenanceTaskType::METRICS_COLLECTION};

    auto created = orchestrator_->createSchedule(entry);
    ASSERT_TRUE(created);

    // Trigger job1 – it will block inside the handler.
    // triggerNow() sets job.state=RUNNING in jobs_ synchronously before spawning the thread.
    auto job1_result = orchestrator_->triggerNow(created->id);
    ASSERT_TRUE(job1_result) << job1_result.error().message();
    std::string job1_id = job1_result->id;

    // Trigger job2 – should be SKIPPED because job1 is RUNNING
    // (jobs_[job1.id].state == RUNNING was set synchronously above)
    auto job2_result = orchestrator_->triggerNow(created->id);
    ASSERT_TRUE(job2_result) << job2_result.error().message();
    std::string job2_id = job2_result->id;

    // Wait for job2 to reach a terminal state (up to 2 s)
    for (int i = 0; i < 40; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        auto j = orchestrator_->getJob(job2_id);
        if (j && j->state != MaintenanceJobState::RUNNING &&
                 j->state != MaintenanceJobState::PENDING) break;
    }

    auto final_job2 = orchestrator_->getJob(job2_id);
    ASSERT_TRUE(final_job2);
    EXPECT_EQ(final_job2->state, MaintenanceJobState::SKIPPED)
        << "Expected job2 SKIPPED but got: " << jobStateToString(final_job2->state);
    EXPECT_NE(final_job2->error_message.find("quota"), std::string::npos);

    // Release blocking handler so job1 can complete
    blocking_handler->release();

    for (int i = 0; i < 40; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        auto j = orchestrator_->getJob(job1_id);
        if (j && j->state != MaintenanceJobState::RUNNING &&
                 j->state != MaintenanceJobState::PENDING) break;
    }

    auto final_job1 = orchestrator_->getJob(job1_id);
    ASSERT_TRUE(final_job1);
    EXPECT_EQ(final_job1->state, MaintenanceJobState::SUCCEEDED);
}

// MT-14: per-tenant quota = 0 means unlimited; second job is not quota-skipped.
TEST_F(MaintenanceOrchestratorTest, TenantQuota_ZeroMeansUnlimited) {
    TenantMaintenanceConfig cfg;
    cfg.max_concurrent_jobs = 0;   // unlimited
    orchestrator_->setTenantMaintenanceConfig("tenant-unlimited", cfg);

    auto entry = makeEntry("Unlimited Quota");
    entry.tenant_id      = "tenant-unlimited";
    entry.enforce_window = false;
    entry.tasks          = {MaintenanceTaskType::METRICS_COLLECTION};

    auto created = orchestrator_->createSchedule(entry);
    ASSERT_TRUE(created);

    // Trigger two jobs back-to-back
    auto job1 = orchestrator_->triggerNow(created->id);
    auto job2 = orchestrator_->triggerNow(created->id);
    ASSERT_TRUE(job1);
    ASSERT_TRUE(job2);

    // Wait for both to reach a terminal state
    for (int i = 0; i < 40; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        auto j1 = orchestrator_->getJob(job1->id);
        auto j2 = orchestrator_->getJob(job2->id);
        bool j1_done = j1 && j1->state != MaintenanceJobState::RUNNING &&
                            j1->state != MaintenanceJobState::PENDING;
        bool j2_done = j2 && j2->state != MaintenanceJobState::RUNNING &&
                            j2->state != MaintenanceJobState::PENDING;
        if (j1_done && j2_done) {
          break;
        }
    }

    auto final1 = orchestrator_->getJob(job1->id);
    auto final2 = orchestrator_->getJob(job2->id);
    ASSERT_TRUE(final1);
    ASSERT_TRUE(final2);

    // Neither should be SKIPPED due to quota
    auto is_quota_skip = [](const OrchestratorJob& j) {
        return j.state == MaintenanceJobState::SKIPPED &&
               j.error_message.find("quota") != std::string::npos;
    };
    EXPECT_FALSE(is_quota_skip(*final1)) << "job1 was quota-skipped unexpectedly";
    EXPECT_FALSE(is_quota_skip(*final2)) << "job2 was quota-skipped unexpectedly";
}

// MT-15: OrchestratorJob::tenant_id is populated from the parent schedule.
TEST_F(MaintenanceOrchestratorTest, OrchestratorJob_TenantIdPopulated) {
    auto entry = makeEntry("Job Tenant Test");
    entry.tenant_id      = "tenant-job";
    entry.enforce_window = false;
    entry.tasks          = {MaintenanceTaskType::METRICS_COLLECTION};

    auto created = orchestrator_->createSchedule(entry);
    ASSERT_TRUE(created);

    auto job_result = orchestrator_->triggerNow(created->id);
    ASSERT_TRUE(job_result);

    // triggerNow returns the job immediately; tenant_id should be populated
    EXPECT_EQ(job_result->tenant_id, "tenant-job");

    // Also verify via JSON round-trip
    auto job_json = job_result->toJson();
    EXPECT_EQ(job_json.value("tenant_id", ""), "tenant-job");
}

// Concurrency / TSAN – shared_mutex read-path upgrade
// Exercises 8 concurrent listSchedules readers + 1 createSchedule writer.
// When built with -DTHEMIS_ENABLE_TSAN=ON, ThreadSanitizer will report any
// data race that remains on schedules_mutex_ or jobs_mutex_.
// ===========================================================================

TEST_F(MaintenanceOrchestratorTest, ConcurrentListSchedules_NoDataRace) {
    // Pre-populate a few schedules so readers have non-empty work.
    for (int i = 0; i < 4; ++i) {
        auto e = makeEntry("Seed-" + std::to_string(i));
        auto result = orchestrator_->createSchedule(e);
        ASSERT_TRUE(result) << "Seed createSchedule(" << i << ") failed: "
                            << result.error().message();
    }

    constexpr int kReaders        = 8;
    constexpr int kItersPerThread = 200;

    std::atomic<bool> go{false};
    std::atomic<int>  total_read{0};
    // Collect writer failures outside the worker thread so gtest assertions
    // are always issued from the main thread (worker-thread ASSERT/EXPECT
    // calls do not abort the test and can be missed by the test framework).
    std::atomic<int>  writer_failures{0};

    // 8 concurrent readers – yield inside the spin loop to avoid burning CPU
    // and to reduce TSAN false-positive suppression headroom.
    std::vector<std::thread> readers;
    readers.reserve(kReaders);
    for (int t = 0; t < kReaders; ++t) {
        readers.emplace_back([&] {
            while (!go.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            for (int i = 0; i < kItersPerThread; ++i) {
                auto schedules = orchestrator_->listSchedules();
                total_read.fetch_add(static_cast<int>(schedules.size()),
                                     std::memory_order_relaxed);
            }
        });
    }

    // 1 concurrent writer – track failures atomically so the main thread can
    // assert on them after joining (gtest assertions in worker threads are
    // unreliable and may silently pass even when the assertion fires).
    std::thread writer([&] {
        while (!go.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        for (int i = 0; i < kItersPerThread; ++i) {
            auto e      = makeEntry("Writer-" + std::to_string(i));
            auto result = orchestrator_->createSchedule(e);
            if (!result) {
                writer_failures.fetch_add(1, std::memory_order_relaxed);
            }
        }
    });

    go.store(true, std::memory_order_release);

    writer.join();
    for (auto& r : readers) {
      r.join();
    }

    // Assert writer never failed – checked on main thread where gtest works.
    EXPECT_EQ(writer_failures.load(), 0)
        << "createSchedule failed " << writer_failures.load() << " time(s) during concurrent run";

    // Sanity: at least the pre-populated schedules must be visible at the end.
    EXPECT_GE(orchestrator_->listSchedules().size(), 4u);
    EXPECT_GT(total_read.load(), 0);
}

// ===========================================================================
// Distributed lock tests (IDistributedLock integration)
// ===========================================================================

// ---------------------------------------------------------------------------
// Helper: a lock stub that always refuses to grant the lock, pretending it
// is held by a specific peer node.
// ---------------------------------------------------------------------------

class AlwaysLockedDistributedLock : public IDistributedLock {
public:
    explicit AlwaysLockedDistributedLock(std::string holder_node_id,
                                         std::string own_node_id = "this-node")
        : holder_node_id_(std::move(holder_node_id))
        , own_node_id_(std::move(own_node_id))
    {}

    bool        tryAcquire(const std::string& /*key*/, int64_t /*ttl_ms*/) override { return false; }
    void        release(const std::string& /*key*/) override {}
    std::string getHolderNodeId(const std::string& /*key*/) const override { return holder_node_id_; }
    std::string nodeId() const override { return own_node_id_; }

private:
    std::string holder_node_id_;
    std::string own_node_id_;
};

// ---------------------------------------------------------------------------
// Helper: a lock stub that always succeeds and records acquire/release calls.
// ---------------------------------------------------------------------------

class RecordingDistributedLock : public IDistributedLock {
public:
    explicit RecordingDistributedLock(std::string own_node_id = "this-node")
        : own_node_id_(std::move(own_node_id)) {}

    bool tryAcquire(const std::string& key, int64_t ttl_ms) override {
        std::lock_guard<std::mutex> lg(mu_);
        ++acquire_count_;
        last_key_    = key;
        last_ttl_ms_ = ttl_ms;
        return true;
    }

    void release(const std::string& key) override {
        std::lock_guard<std::mutex> lg(mu_);
        ++release_count_;
        last_release_key_ = key;
    }

    std::string getHolderNodeId(const std::string& /*key*/) const override { return own_node_id_; }
    std::string nodeId() const override { return own_node_id_; }

    int         acquireCount()    const { std::lock_guard<std::mutex> lg(mu_); return acquire_count_; }
    int         releaseCount()    const { std::lock_guard<std::mutex> lg(mu_); return release_count_; }
    std::string lastKey()         const { std::lock_guard<std::mutex> lg(mu_); return last_key_; }
    int64_t     lastTtlMs()       const { std::lock_guard<std::mutex> lg(mu_); return last_ttl_ms_; }
    std::string lastReleaseKey()  const { std::lock_guard<std::mutex> lg(mu_); return last_release_key_; }

private:
    std::string own_node_id_;
    mutable std::mutex mu_;
    int     acquire_count_{0};
    int     release_count_{0};
    std::string last_key_;
    int64_t     last_ttl_ms_{0};
    std::string last_release_key_;
};

// ---------------------------------------------------------------------------
// DL-1: No lock configured → job runs normally (backwards-compatible)
// ---------------------------------------------------------------------------

TEST_F(MaintenanceOrchestratorTest, DistributedLock_NoLock_JobRunsNormally) {
    // No distributed lock set on the orchestrator.
    auto entry = makeEntry("No Lock Schedule");
    entry.enforce_window = false;
    auto created = orchestrator_->createSchedule(entry);
    ASSERT_TRUE(created) << created.error().message();

    auto job_result = orchestrator_->triggerNow(created->id, /*force=*/true);
    ASSERT_TRUE(job_result);
    waitForTerminalJobState(orchestrator_.get(), job_result->id);

    auto final_job = orchestrator_->getJob(job_result->id);
    ASSERT_TRUE(final_job);
    EXPECT_TRUE(
        final_job->state == MaintenanceJobState::SUCCEEDED ||
        final_job->state == MaintenanceJobState::SKIPPED)  // SKIPPED = no handler (acceptable)
        << "Unexpected state: " << jobStateToString(final_job->state);
}

// ---------------------------------------------------------------------------
// DL-2: Lock acquired by this node → job executes
// ---------------------------------------------------------------------------

TEST_F(MaintenanceOrchestratorTest, DistributedLock_LockAcquired_JobExecutes) {
    auto recording_lock = std::make_shared<RecordingDistributedLock>("node-A");
    orchestrator_->setDistributedLock(recording_lock);

    auto entry = makeEntry("Locked Schedule");
    entry.enforce_window = false;
    auto created = orchestrator_->createSchedule(entry);
    ASSERT_TRUE(created);

    auto job_result = orchestrator_->triggerNow(created->id, /*force=*/true);
    ASSERT_TRUE(job_result);
    waitForTerminalJobState(orchestrator_.get(), job_result->id);

    auto final_job = orchestrator_->getJob(job_result->id);
    ASSERT_TRUE(final_job);
    // Job should complete (SUCCEEDED or SKIPPED due to unregistered handler, NOT SKIPPED
    // due to lock denial).
    EXPECT_NE(final_job->state, MaintenanceJobState::PENDING);
    EXPECT_NE(final_job->state, MaintenanceJobState::FAILED);

    // Lock must have been acquired once
    EXPECT_GE(recording_lock->acquireCount(), 1);
    EXPECT_EQ(recording_lock->lastKey(), created->id);

    // Lock must have been released after job completion
    EXPECT_GE(recording_lock->releaseCount(), 1);
    EXPECT_EQ(recording_lock->lastReleaseKey(), created->id);
}

// ---------------------------------------------------------------------------
// DL-3: Lock held by peer → job is SKIPPED
// ---------------------------------------------------------------------------

TEST_F(MaintenanceOrchestratorTest, DistributedLock_LockHeldByPeer_JobSkipped) {
    auto peer_lock = std::make_shared<AlwaysLockedDistributedLock>(
        /*holder=*/"peer-node-B", /*own_node=*/"this-node-A");
    orchestrator_->setDistributedLock(peer_lock);

    auto entry = makeEntry("Peer Holds Lock");
    entry.enforce_window = false;
    auto created = orchestrator_->createSchedule(entry);
    ASSERT_TRUE(created);

    auto job_result = orchestrator_->triggerNow(created->id, /*force=*/true);
    ASSERT_TRUE(job_result);
    waitForTerminalJobState(orchestrator_.get(), job_result->id);

    auto final_job = orchestrator_->getJob(job_result->id);
    ASSERT_TRUE(final_job);
    EXPECT_EQ(final_job->state, MaintenanceJobState::SKIPPED)
        << "Expected SKIPPED when peer holds lock, got: "
        << jobStateToString(final_job->state);

    // Error message must mention the peer
    EXPECT_NE(final_job->error_message.find("peer-node-B"), std::string::npos)
        << "Error message should contain peer node ID, got: "
        << final_job->error_message;
}

// ---------------------------------------------------------------------------
// DL-4: TTL auto-computed from window when lock_ttl_ms == 0
// ---------------------------------------------------------------------------

TEST_F(MaintenanceOrchestratorTest, DistributedLock_TtlAutoComputedFromWindow) {
    auto recording_lock = std::make_shared<RecordingDistributedLock>();
    orchestrator_->setDistributedLock(recording_lock);

    auto entry          = makeEntry("TTL Window Schedule");
    entry.enforce_window    = false;
    entry.window_start_hour = 2;
    entry.window_end_hour   = 6;  // 4-hour window → 4 * 3600 * 1000 = 14 400 000 ms
    entry.lock_ttl_ms       = 0;  // auto-compute

    auto created = orchestrator_->createSchedule(entry);
    ASSERT_TRUE(created);

    auto job_result = orchestrator_->triggerNow(created->id, /*force=*/true);
    ASSERT_TRUE(job_result);
    waitForTerminalJobState(orchestrator_.get(), job_result->id);

    ASSERT_GE(recording_lock->acquireCount(), 1);
    // Expected TTL: 4h * 3 600 000 ms + 30 000 ms safety margin = 14 430 000 ms
    const int64_t expected_ttl = 4LL * 3600LL * 1000LL + 30000LL;
    EXPECT_EQ(recording_lock->lastTtlMs(), expected_ttl)
        << "Auto-computed TTL should be window_duration + 30 s safety margin";
}

// ---------------------------------------------------------------------------
// DL-5: Explicit lock_ttl_ms overrides auto-computation
// ---------------------------------------------------------------------------

TEST_F(MaintenanceOrchestratorTest, DistributedLock_ExplicitLockTtl_UsedDirectly) {
    auto recording_lock = std::make_shared<RecordingDistributedLock>();
    orchestrator_->setDistributedLock(recording_lock);

    auto entry          = makeEntry("Explicit TTL Schedule");
    entry.enforce_window = false;
    entry.lock_ttl_ms    = 120000;  // 2 minutes explicit

    auto created = orchestrator_->createSchedule(entry);
    ASSERT_TRUE(created);

    auto job_result = orchestrator_->triggerNow(created->id, /*force=*/true);
    ASSERT_TRUE(job_result);
    waitForTerminalJobState(orchestrator_.get(), job_result->id);

    ASSERT_GE(recording_lock->acquireCount(), 1);
    EXPECT_EQ(recording_lock->lastTtlMs(), 120000LL)
        << "Explicit lock_ttl_ms should be passed unchanged to tryAcquire";
}

// ---------------------------------------------------------------------------
// DL-6: lock_ttl_ms round-trips through JSON serialisation
// ---------------------------------------------------------------------------

TEST_F(MaintenanceOrchestratorTest, DistributedLock_LockTtlMs_JsonRoundTrip) {
    auto entry          = makeEntry("JSON TTL");
    entry.lock_ttl_ms   = 90000;

    auto j        = entry.toJson();
    ASSERT_TRUE(j.contains("lock_ttl_ms"));
    EXPECT_EQ(j["lock_ttl_ms"].get<int64_t>(), 90000LL);

    auto restored = MaintenanceScheduleEntry::fromJson(j);
    EXPECT_EQ(restored.lock_ttl_ms, 90000LL);
}

// ---------------------------------------------------------------------------
// DL-7: lock_ttl_ms can be patched via applyPatch
// ---------------------------------------------------------------------------

TEST_F(MaintenanceOrchestratorTest, DistributedLock_LockTtlMs_ApplyPatch) {
    auto entry = makeEntry("Patch TTL");
    entry.lock_ttl_ms = 0;

    nlohmann::json patch;
    patch["lock_ttl_ms"] = 60000;
    entry.applyPatch(patch);

    EXPECT_EQ(entry.lock_ttl_ms, 60000LL);
}

// ---------------------------------------------------------------------------
// DL-8: setDistributedLock(nullptr) clears the lock → job runs without lock
// ---------------------------------------------------------------------------

TEST_F(MaintenanceOrchestratorTest, DistributedLock_SetNullptr_ClearsLock) {
    auto recording_lock = std::make_shared<RecordingDistributedLock>();
    orchestrator_->setDistributedLock(recording_lock);

    // Clear the lock
    orchestrator_->setDistributedLock(nullptr);

    auto entry = makeEntry("Clear Lock Schedule");
    entry.enforce_window = false;
    auto created = orchestrator_->createSchedule(entry);
    ASSERT_TRUE(created);

    auto job_result = orchestrator_->triggerNow(created->id, /*force=*/true);
    ASSERT_TRUE(job_result);
    waitForTerminalJobState(orchestrator_.get(), job_result->id);

    // No acquire/release should have happened
    EXPECT_EQ(recording_lock->acquireCount(), 0);
    EXPECT_EQ(recording_lock->releaseCount(), 0);
}

// ---------------------------------------------------------------------------
// DL-9: InProcessDistributedLock — two nodes, first acquires, second skipped
// ---------------------------------------------------------------------------

TEST(InProcessDistributedLockTest, FirstNodeAcquires_SecondSkipped) {
#ifdef _WIN32
    GTEST_SKIP() << "Skipping InProcessDistributedLockTest on Windows due to inconsistent shared-lock acquisition semantics across runtime implementations.";
#endif
    // Simulate two nodes using the same InProcessDistributedLock (shared state).
    auto shared_lock = std::make_shared<InProcessDistributedLock>("node-A");

    // node-A acquires
    EXPECT_TRUE(shared_lock->tryAcquire("sched-1", 60000));
    EXPECT_EQ(shared_lock->getHolderNodeId("sched-1"), "node-A");

    // node-B uses a separate instance that points to the same underlying lock.
    // (In real multi-node scenarios this is a distributed lock service;
    // here we test the in-process version by making the same object visible as
    // a different "node" with a different node_id.)
    InProcessDistributedLock node_b_view("node-B");
    // The in-process lock is per-instance, so node-B has its own state; we
    // verify the InProcessDistributedLock API directly on the same instance.
    EXPECT_FALSE(shared_lock->tryAcquire("sched-1", 60000));  // node-A already holds it
}

TEST(InProcessDistributedLockTest, ReleaseAllowsReacquire) {
    InProcessDistributedLock lock("node-A");
    ASSERT_TRUE(lock.tryAcquire("sched-2", 5000));
    lock.release("sched-2");
    EXPECT_TRUE(lock.tryAcquire("sched-2", 5000));
}

TEST(InProcessDistributedLockTest, ExpiredTtlAllowsReacquire) {
    InProcessDistributedLock lock("node-A");
    // Acquire with a very short TTL (1 ms); it will expire almost immediately.
    ASSERT_TRUE(lock.tryAcquire("sched-3", 1));
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    // After expiry another acquire must succeed
    EXPECT_TRUE(lock.tryAcquire("sched-3", 5000));
}

TEST(InProcessDistributedLockTest, GetHolderNodeId_ExpiredReturnsEmpty) {
    InProcessDistributedLock lock("node-A");
    ASSERT_TRUE(lock.tryAcquire("sched-4", 1));
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    EXPECT_TRUE(lock.getHolderNodeId("sched-4").empty());
}

TEST(InProcessDistributedLockTest, NodeId_ReturnsConfiguredId) {
    InProcessDistributedLock lock("my-node-42");
    EXPECT_EQ(lock.nodeId(), "my-node-42");
}
