/**
 * @file test_task_scheduler_api_handler.cpp
 * @brief Unit tests for TaskSchedulerApiHandler
 */

#include <gtest/gtest.h>
#include "server/task_scheduler_api_handler.h"
#include "scheduler/task_scheduler.h"
#include "scheduler/task_audit_event.h"
#include "scheduler/task_audit_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "query/query_engine.h"
#include <filesystem>
#include <chrono>
#include <set>

using namespace themis;
using namespace themis::server;
using namespace std::chrono_literals;

// ============================================================================
// Test fixture
// ============================================================================

class TaskSchedulerApiHandlerTest : public ::testing::Test {
protected:
    static std::string makeDbPath() {
        auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        return (std::filesystem::temp_directory_path() /
                std::filesystem::path("themis_api_test_" + std::to_string(now))).string();
    }

    void SetUp() override {
        TaskScheduler::clearRequestContext();
        ASSERT_EQ(TaskScheduler::currentUserId(), "system");
        db_path_ = makeDbPath();
        std::filesystem::create_directories(db_path_);

        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_ + "/db";
        cfg.enable_blobdb = false;
        storage_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(storage_->open());

        idx_     = std::make_unique<SecondaryIndexManager>(*storage_);
        engine_  = std::make_unique<QueryEngine>(*storage_, *idx_);

        TaskScheduler::Config sched_cfg;
        sched_cfg.max_concurrent_tasks    = 2;
        sched_cfg.check_interval          = 50ms;
        sched_cfg.persist_tasks           = false;
        sched_cfg.enable_audit_logging    = false;
        sched_cfg.enable_anomaly_detection = false;

        scheduler_ = std::make_unique<TaskScheduler>(engine_.get(), sched_cfg);
        handler_   = std::make_unique<TaskSchedulerApiHandler>(scheduler_.get());
    }

    void TearDown() override {
        TaskScheduler::clearRequestContext();
        handler_.reset();
        if (scheduler_) {
            scheduler_->stop();
            scheduler_.reset();
        }
        engine_.reset();
        idx_.reset();
        storage_->close();
        storage_.reset();
        std::filesystem::remove_all(db_path_);
    }

    // Build a minimal valid task JSON
    static nlohmann::json makeTaskJson(const std::string& name = "test_task",
                                       const std::string& aql  = "RETURN 1") {
        return nlohmann::json{
            {"name",         name},
            {"description",  "Unit test task"},
            {"type",         "aql_query"},
            {"aql_query",    aql},
            {"trigger_type", "interval"},
            {"interval_ms",  5000},
            {"timeout_ms",   30000},
            {"max_retries",  0},
            {"enabled",      true}
        };
    }

    std::string db_path_;
    std::unique_ptr<RocksDBWrapper>        storage_;
    std::unique_ptr<SecondaryIndexManager> idx_;
    std::unique_ptr<QueryEngine>           engine_;
    std::unique_ptr<TaskScheduler>         scheduler_;
    std::unique_ptr<TaskSchedulerApiHandler> handler_;
};

// ============================================================================
// registerTask
// ============================================================================

TEST_F(TaskSchedulerApiHandlerTest, RegisterTask_ReturnsCreated) {
    auto result = handler_->registerTask(makeTaskJson());
    EXPECT_EQ(result.value("status", ""), "created");
    EXPECT_FALSE(result.value("id", "").empty());
}

TEST_F(TaskSchedulerApiHandlerTest, RegisterTask_MissingName_ReturnsError) {
    nlohmann::json req = {{"aql_query", "RETURN 1"}, {"type", "aql_query"}};
    auto result = handler_->registerTask(req);
    EXPECT_EQ(result.value("status", ""), "error");
}

// ============================================================================
// listTasks
// ============================================================================

TEST_F(TaskSchedulerApiHandlerTest, ListTasks_EmptyScheduler) {
    auto result = handler_->listTasks();
    ASSERT_TRUE(result.contains("items"));
    EXPECT_TRUE(result["items"].is_array());
    EXPECT_EQ(result["items"].size(), 0u);
    EXPECT_EQ(result.value("total", -1), 0);
}

TEST_F(TaskSchedulerApiHandlerTest, ListTasks_AfterRegister) {
    handler_->registerTask(makeTaskJson("task_a"));
    handler_->registerTask(makeTaskJson("task_b"));
    auto result = handler_->listTasks();
    EXPECT_EQ(result.value("total", 0), 2);
    EXPECT_EQ(result["items"].size(), 2u);
}

// ============================================================================
// getTask
// ============================================================================

TEST_F(TaskSchedulerApiHandlerTest, GetTask_NotFound) {
    auto result = handler_->getTask("nonexistent_id");
    EXPECT_EQ(result.value("status", ""), "error");
}

TEST_F(TaskSchedulerApiHandlerTest, GetTask_Found) {
    auto reg = handler_->registerTask(makeTaskJson("my_task"));
    std::string id = reg.value("id", "");
    ASSERT_FALSE(id.empty());

    auto result = handler_->getTask(id);
    EXPECT_EQ(result.value("name", ""), "my_task");
    EXPECT_EQ(result.value("id", ""), id);
}

// ============================================================================
// updateTask
// ============================================================================

TEST_F(TaskSchedulerApiHandlerTest, UpdateTask_Found) {
    auto reg = handler_->registerTask(makeTaskJson("orig_name"));
    std::string id = reg.value("id", "");
    ASSERT_FALSE(id.empty());

    auto upd = makeTaskJson("updated_name");
    upd["id"] = id;
    auto result = handler_->updateTask(id, upd);
    EXPECT_EQ(result.value("status", ""), "updated");
}

TEST_F(TaskSchedulerApiHandlerTest, UpdateTask_NotFound_ReturnsError) {
    auto result = handler_->updateTask("no_such_id", makeTaskJson());
    EXPECT_EQ(result.value("status", ""), "error");
}

// ============================================================================
// enableTask / disableTask
// ============================================================================

TEST_F(TaskSchedulerApiHandlerTest, DisableAndEnableTask) {
    auto reg = handler_->registerTask(makeTaskJson());
    std::string id = reg.value("id", "");
    ASSERT_FALSE(id.empty());

    auto dis = handler_->disableTask(id);
    EXPECT_EQ(dis.value("status", ""), "disabled");

    auto task = handler_->getTask(id);
    EXPECT_EQ(task.value("enabled", true), false);

    auto en = handler_->enableTask(id);
    EXPECT_EQ(en.value("status", ""), "enabled");

    task = handler_->getTask(id);
    EXPECT_EQ(task.value("enabled", false), true);
}

TEST_F(TaskSchedulerApiHandlerTest, DisableTask_NotFound) {
    auto result = handler_->disableTask("bad_id");
    EXPECT_EQ(result.value("status", ""), "error");
}

TEST_F(TaskSchedulerApiHandlerTest, ExecuteTask_DeniedWithoutPermission) {
    scheduler_->registerFunction("api_exec_denied_fn",
        [](const nlohmann::json&) -> nlohmann::json { return {{"ok", true}}; });

    ScheduledTask task;
    task.id = "api_exec_denied_task";
    task.name = "api_exec_denied_task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "api_exec_denied_fn";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;
    scheduler_->registerTask(task);

    TaskScheduler::RequestContext ctx;
    ctx.user_id = "api-user";
    ctx.authorization_justification = "api-negative-execute-test";
    TaskScheduler::setRequestContext(ctx);

    auto result = handler_->executeTask(task.id);
    TaskScheduler::clearRequestContext();

    EXPECT_EQ(result.value("status", ""), "error");
    EXPECT_NE(result.value("error", "").find("Unauthorized"), std::string::npos);
}

TEST_F(TaskSchedulerApiHandlerTest, ExecuteTask_AllowedWithPermission) {
    scheduler_->registerFunction("api_exec_allowed_fn",
        [](const nlohmann::json&) -> nlohmann::json { return {{"ok", true}}; });

    ScheduledTask task;
    task.id = "api_exec_allowed_task";
    task.name = "api_exec_allowed_task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "api_exec_allowed_fn";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;
    scheduler_->registerTask(task);

    TaskScheduler::RequestContext ctx;
    ctx.user_id = "api-user";
    ctx.granted_permissions.insert("task:execute");
    ctx.authorization_justification = "api-positive-execute-test";
    TaskScheduler::setRequestContext(ctx);

    auto result = handler_->executeTask(task.id);
    TaskScheduler::clearRequestContext();

    EXPECT_EQ(result.value("status", ""), "executed");
    ASSERT_TRUE(result.contains("result"));
    EXPECT_EQ(result["result"].value("ok", false), true);
}

// ============================================================================
// unregisterTask
// ============================================================================

TEST_F(TaskSchedulerApiHandlerTest, UnregisterTask) {
    auto reg = handler_->registerTask(makeTaskJson());
    std::string id = reg.value("id", "");
    ASSERT_FALSE(id.empty());

    auto del = handler_->unregisterTask(id);
    EXPECT_EQ(del.value("status", ""), "deleted");

    // Task should be gone
    auto result = handler_->getTask(id);
    EXPECT_EQ(result.value("status", ""), "error");
}

TEST_F(TaskSchedulerApiHandlerTest, UnregisterTask_NotFound) {
    auto result = handler_->unregisterTask("nonexistent");
    EXPECT_EQ(result.value("status", ""), "error");
}

// ============================================================================
// getStats
// ============================================================================

TEST_F(TaskSchedulerApiHandlerTest, GetStats_BeforeStart) {
    auto stats = handler_->getStats();
    EXPECT_TRUE(stats.contains("registered_tasks"));
    EXPECT_TRUE(stats.contains("active_tasks"));
    EXPECT_TRUE(stats.contains("total_executions"));
    EXPECT_TRUE(stats.contains("scheduler_running"));
}

TEST_F(TaskSchedulerApiHandlerTest, GetStats_RegisteredCount) {
    handler_->registerTask(makeTaskJson("s1"));
    handler_->registerTask(makeTaskJson("s2"));
    auto stats = handler_->getStats();
    EXPECT_EQ(stats.value("registered_tasks", 0), 2);
}

// ============================================================================
// getWebUi
// ============================================================================

TEST_F(TaskSchedulerApiHandlerTest, GetWebUi_ReturnsHtml) {
    auto html = handler_->getWebUi();
    EXPECT_FALSE(html.empty());
    EXPECT_NE(html.find("<!DOCTYPE html>"), std::string::npos);
    EXPECT_NE(html.find("Task Scheduler"), std::string::npos);
    EXPECT_NE(html.find("/api/tasks"), std::string::npos);
}

TEST_F(TaskSchedulerApiHandlerTest, GetWebUi_HasCreateTaskButton) {
    auto html = handler_->getWebUi();
    // The "New Task" button must be present to allow task creation
    EXPECT_NE(html.find("openCreateDialog"), std::string::npos);
    EXPECT_NE(html.find("New Task"), std::string::npos);
}

TEST_F(TaskSchedulerApiHandlerTest, GetWebUi_HasMonitoringStats) {
    auto html = handler_->getWebUi();
    // Stats section must show registered, active, running, executions, failures
    EXPECT_NE(html.find("s-registered"), std::string::npos);
    EXPECT_NE(html.find("s-active"), std::string::npos);
    EXPECT_NE(html.find("s-running"), std::string::npos);
    EXPECT_NE(html.find("s-total"), std::string::npos);
    EXPECT_NE(html.find("s-failed"), std::string::npos);
    EXPECT_NE(html.find("loadStats"), std::string::npos);
}

TEST_F(TaskSchedulerApiHandlerTest, GetWebUi_HasPauseResumeActions) {
    auto html = handler_->getWebUi();
    // Pause/resume (disable/enable) actions must be present
    EXPECT_NE(html.find("disableTask"), std::string::npos);
    EXPECT_NE(html.find("enableTask"), std::string::npos);
    EXPECT_NE(html.find("Pause"), std::string::npos);
    EXPECT_NE(html.find("Resume"), std::string::npos);
}

TEST_F(TaskSchedulerApiHandlerTest, GetWebUi_HasDeleteAction) {
    auto html = handler_->getWebUi();
    // Delete action must be present
    EXPECT_NE(html.find("deleteTask"), std::string::npos);
    EXPECT_NE(html.find("DELETE"), std::string::npos);
}

TEST_F(TaskSchedulerApiHandlerTest, GetWebUi_HasTaskTable) {
    auto html = handler_->getWebUi();
    // Task table for monitoring must be present
    EXPECT_NE(html.find("task-table-body"), std::string::npos);
    EXPECT_NE(html.find("loadTasks"), std::string::npos);
}

TEST_F(TaskSchedulerApiHandlerTest, GetWebUi_HasCreateDialog) {
    auto html = handler_->getWebUi();
    // Modal dialog with task form fields must be present
    EXPECT_NE(html.find("task-dialog"), std::string::npos);
    EXPECT_NE(html.find("f-name"), std::string::npos);
    EXPECT_NE(html.find("f-aql"), std::string::npos);
    EXPECT_NE(html.find("f-trigger"), std::string::npos);
    EXPECT_NE(html.find("saveTask"), std::string::npos);
}

TEST_F(TaskSchedulerApiHandlerTest, GetWebUi_HasAutoRefresh) {
    auto html = handler_->getWebUi();
    // Auto-refresh must be present for continuous monitoring with 30-second interval
    EXPECT_NE(html.find("setInterval"), std::string::npos);
    EXPECT_NE(html.find("loadAll"), std::string::npos);
    EXPECT_NE(html.find("30000"), std::string::npos);
}

TEST_F(TaskSchedulerApiHandlerTest, GetWebUi_HasXssProtection) {
    auto html = handler_->getWebUi();
    // escHtml function must be present with correct character escaping logic
    EXPECT_NE(html.find("escHtml"), std::string::npos);
    // Verify escHtml escapes the four critical HTML special characters
    EXPECT_NE(html.find("&amp;"), std::string::npos);
    EXPECT_NE(html.find("&lt;"), std::string::npos);
    EXPECT_NE(html.find("&gt;"), std::string::npos);
    EXPECT_NE(html.find("&quot;"), std::string::npos);
    // escHtml must be applied to user-supplied data in the task table
    EXPECT_NE(html.find("escHtml(t.id)"), std::string::npos);
    EXPECT_NE(html.find("escHtml(t.name)"), std::string::npos);
}

TEST_F(TaskSchedulerApiHandlerTest, GetWebUi_IsIdempotent) {
    // getWebUi() must return identical output on repeated calls
    auto html1 = handler_->getWebUi();
    auto html2 = handler_->getWebUi();
    EXPECT_EQ(html1, html2);
}

// ============================================================================
// Null scheduler guard
// ============================================================================

TEST(TaskSchedulerApiHandlerNullTest, NullScheduler_AllMethodsReturnError) {
    TaskSchedulerApiHandler h(nullptr);

    EXPECT_EQ(h.listTasks().value("status", ""), "error");
    EXPECT_EQ(h.getTask("x").value("status", ""), "error");
    EXPECT_EQ(h.getStats().value("status", ""), "error");
    EXPECT_EQ(h.enableTask("x").value("status", ""), "error");
    EXPECT_EQ(h.disableTask("x").value("status", ""), "error");
    EXPECT_EQ(h.unregisterTask("x").value("status", ""), "error");
    EXPECT_EQ(h.executeTask("x").value("status", ""), "error");
    EXPECT_EQ(h.getTaskResults("x", 10).value("status", ""), "error");
    EXPECT_EQ(h.getLatestTaskResult("x").value("status", ""), "error");
    EXPECT_EQ(h.getExecutionHistory("x").value("status", ""), "error");
    EXPECT_EQ(h.executeDAG(nlohmann::json{{"task_ids", nlohmann::json::array()}}).value("status", ""), "error");

    nlohmann::json req{{"name", "t"}, {"type", "aql_query"}, {"aql_query", "RETURN 1"}};
    EXPECT_EQ(h.registerTask(req).value("status", ""), "error");

    nlohmann::json upd{{"name", "t2"}, {"type", "aql_query"}, {"aql_query", "RETURN 2"}};
    EXPECT_EQ(h.updateTask("x", upd).value("status", ""), "error");

    // getWebUi() should still return valid HTML
    EXPECT_FALSE(h.getWebUi().empty());
    EXPECT_NE(h.getWebUi().find("<!DOCTYPE html>"), std::string::npos);
}

// ============================================================================
// parseTaskFromJson round-trip
// ============================================================================

TEST_F(TaskSchedulerApiHandlerTest, RegisterTask_AllTriggerTypes) {
    for (const auto& trigger : std::vector<std::string>{"interval", "cron", "manual"}) {
        nlohmann::json req = makeTaskJson("t_" + trigger);
        req["trigger_type"] = trigger;
        if (trigger == "cron") {
          req["cron_expression"] = "*/5 * * * *";
        }
        auto result = handler_->registerTask(req);
        EXPECT_EQ(result.value("status", ""), "created") << "trigger=" << trigger;
    }
}

TEST_F(TaskSchedulerApiHandlerTest, RegisterTask_FunctionType) {
    nlohmann::json req = makeTaskJson("fn_task");
    req["type"]          = "function";
    req["function_name"] = "compress_data";
    req.erase("aql_query");
    auto result = handler_->registerTask(req);
    EXPECT_EQ(result.value("status", ""), "created");
}

// ============================================================================
// Input validation
// ============================================================================

TEST_F(TaskSchedulerApiHandlerTest, RegisterTask_ZeroIntervalMs_ReturnsError) {
    auto req = makeTaskJson("bad_interval");
    req["interval_ms"] = 0;
    auto result = handler_->registerTask(req);
    EXPECT_EQ(result.value("status", ""), "error");
    EXPECT_NE(result.value("error", "").find("interval_ms"), std::string::npos);
}

TEST_F(TaskSchedulerApiHandlerTest, RegisterTask_NegativeIntervalMs_ReturnsError) {
    auto req = makeTaskJson("neg_interval");
    req["interval_ms"] = -100;
    auto result = handler_->registerTask(req);
    EXPECT_EQ(result.value("status", ""), "error");
    EXPECT_NE(result.value("error", "").find("interval_ms"), std::string::npos);
}

TEST_F(TaskSchedulerApiHandlerTest, RegisterTask_ZeroTimeoutMs_ReturnsError) {
    auto req = makeTaskJson("bad_timeout");
    req["timeout_ms"] = 0;
    auto result = handler_->registerTask(req);
    EXPECT_EQ(result.value("status", ""), "error");
    EXPECT_NE(result.value("error", "").find("timeout_ms"), std::string::npos);
}

TEST_F(TaskSchedulerApiHandlerTest, RegisterTask_InvalidNamePathTraversal_ReturnsError) {
    auto req = makeTaskJson("../bad_task");
    auto result = handler_->registerTask(req);
    EXPECT_EQ(result.value("status", ""), "error");
    EXPECT_NE(result.value("error", "").find("name"), std::string::npos);
}

TEST_F(TaskSchedulerApiHandlerTest, RegisterTask_UnsafeAqlPattern_ReturnsError) {
    auto req = makeTaskJson("unsafe_aql");
    req["aql_query"] = "FOR d IN users FILTER 1==1 OR 1=1 RETURN d";
    auto result = handler_->registerTask(req);
    EXPECT_EQ(result.value("status", ""), "error");
}

TEST_F(TaskSchedulerApiHandlerTest, GetTask_InvalidTaskId_ReturnsError) {
    auto result = handler_->getTask("../invalid-id");
    EXPECT_EQ(result.value("status", ""), "error");
    EXPECT_NE(result.value("error", "").find("task_id"), std::string::npos);
}

TEST_F(TaskSchedulerApiHandlerTest, ExecuteDag_InvalidTaskId_ReturnsError) {
    nlohmann::json req{
        {"task_ids", nlohmann::json::array({"../invalid"})}
    };
    auto result = handler_->executeDAG(req);
    EXPECT_EQ(result.value("status", ""), "error");
}

TEST_F(TaskSchedulerApiHandlerTest, ListTasks_TotalIsInt) {
    handler_->registerTask(makeTaskJson("t1"));
    auto result = handler_->listTasks();
    ASSERT_TRUE(result.contains("total"));
    EXPECT_TRUE(result["total"].is_number_integer());
    EXPECT_EQ(result["total"].get<int64_t>(), 1);
}

// ============================================================================
// getExecutionHistory – searchable audit log
// ============================================================================

TEST_F(TaskSchedulerApiHandlerTest, GetExecutionHistory_NoAuditManager_ReturnsEmpty) {
    // Fixture disables audit logging (enable_audit_logging = false), so
    // getAuditManager() returns nullptr. Expect empty items array.
    auto result = handler_->getExecutionHistory("some_task_id");
    ASSERT_TRUE(result.contains("items"));
    EXPECT_TRUE(result["items"].is_array());
    ASSERT_TRUE(result.contains("total"));
    EXPECT_EQ(result["total"].get<int64_t>(), 0);
}

TEST_F(TaskSchedulerApiHandlerTest, GetExecutionHistory_NullScheduler_ReturnsError) {
    TaskSchedulerApiHandler h(nullptr);
    auto result = h.getExecutionHistory("x");
    EXPECT_EQ(result.value("status", ""), "error");
}

TEST_F(TaskSchedulerApiHandlerTest, GetExecutionHistory_WithQueryParams_NoAuditManager) {
    // Pagination and filter params should be gracefully ignored when audit is disabled.
    nlohmann::json params{{"limit", 10}, {"offset", 0}, {"success", true}};
    auto result = handler_->getExecutionHistory("task_id", params);
    ASSERT_TRUE(result.contains("items"));
    EXPECT_EQ(result["items"].size(), 0u);
}

// ============================================================================
// getExecutionHistory – with audit manager enabled
// ============================================================================

class TaskSchedulerApiHandlerAuditTest : public ::testing::Test {
protected:
    static std::string makeDbPath() {
        auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        return (std::filesystem::temp_directory_path() /
                std::filesystem::path("themis_audit_api_test_" + std::to_string(now))).string();
    }

    void SetUp() override {
        db_path_ = makeDbPath();
        std::filesystem::create_directories(db_path_);

        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_ + "/db";
        cfg.enable_blobdb = false;
        storage_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(storage_->open());

        idx_    = std::make_unique<SecondaryIndexManager>(*storage_);
        engine_ = std::make_unique<QueryEngine>(*storage_, *idx_);

        TaskScheduler::Config sched_cfg;
        sched_cfg.max_concurrent_tasks    = 2;
        sched_cfg.check_interval          = 50ms;
        sched_cfg.persist_tasks           = false;
        sched_cfg.enable_audit_logging    = false;
        sched_cfg.enable_anomaly_detection = false;
        sched_cfg.enable_result_store     = true;
        sched_cfg.result_store_max_results_per_task = 10;

        scheduler_ = std::make_unique<TaskScheduler>(
            engine_.get(), sched_cfg,
            /*changefeed=*/nullptr,
            /*audit_logger=*/nullptr,
            storage_.get());

        handler_ = std::make_unique<TaskSchedulerApiHandler>(scheduler_.get());

        // Register a simple function used across tests.
        scheduler_->registerFunction("api_result_fn",
            [](const nlohmann::json&) -> nlohmann::json {
                return {{"status", "ok"}, {"value", 1}};
            });
    }

    void TearDown() override {
        handler_.reset();
        if (scheduler_) {
            scheduler_->stop();
            scheduler_.reset();
        }
        engine_.reset();
        idx_.reset();
        storage_->close();
        storage_.reset();
        std::filesystem::remove_all(db_path_);
    }

    // Register + execute a function task, return its generated task ID.
    std::string registerAndExecute(const std::string& name) {
        nlohmann::json req{
            {"name",         name},
            {"type",         "function"},
            {"function_name","api_result_fn"},
            {"trigger_type", "manual"},
            {"timeout_ms",   5000},
            {"max_retries",  0}
        };
        auto reg = handler_->registerTask(req);
        EXPECT_EQ(reg.value("status", ""), "created");
        std::string id = reg.value("id", "");
        EXPECT_FALSE(id.empty());
        handler_->executeTask(id);
        return id;
    }

    std::string db_path_;
    std::unique_ptr<RocksDBWrapper>        storage_;
    std::unique_ptr<SecondaryIndexManager> idx_;
    std::unique_ptr<QueryEngine>           engine_;
    std::unique_ptr<TaskScheduler>         scheduler_;
    std::unique_ptr<TaskSchedulerApiHandler> handler_;
};

// Alias for backward compatibility with existing tests
using TaskResultApiTest = TaskSchedulerApiHandlerAuditTest;

TEST_F(TaskResultApiTest, GetLatestTaskResult_AfterExecution) {
    std::string id = registerAndExecute("latest_result_task");

    auto result = handler_->getLatestTaskResult(id);
    EXPECT_EQ(result.value("task_id", ""), id);
    EXPECT_EQ(result.value("success", false), true);
    EXPECT_TRUE(result.value("error", "").empty());
    EXPECT_GT(result.value("duration_ms", 0.0), 0.0);
}

TEST_F(TaskResultApiTest, GetLatestTaskResult_NotFound_ReturnsNotFound) {
    auto result = handler_->getLatestTaskResult("nonexistent_task_xyz");
    EXPECT_EQ(result.value("status", ""), "not_found");
    EXPECT_EQ(result.value("task_id", ""), "nonexistent_task_xyz");
}

TEST_F(TaskResultApiTest, GetTaskResults_ReturnsItems) {
    std::string id = registerAndExecute("results_list_task");

    auto result = handler_->getTaskResults(id, 10);
    ASSERT_TRUE(result.contains("items"));
    EXPECT_TRUE(result["items"].is_array());
    EXPECT_GE(result["items"].size(), 1u);
    EXPECT_EQ(result.value("task_id", ""), id);
    EXPECT_GE(result.value("count", 0u), 1u);
}

TEST_F(TaskResultApiTest, GetTaskResults_EmptyForUnknownTask) {
    auto result = handler_->getTaskResults("no_such_task", 10);
    ASSERT_TRUE(result.contains("items"));
    EXPECT_EQ(result["items"].size(), 0u);
    EXPECT_EQ(result.value("count", -1), 0);
}

TEST_F(TaskResultApiTest, NullScheduler_ResultMethodsReturnError) {
    TaskSchedulerApiHandler h(nullptr);
    EXPECT_EQ(h.getTaskResults("t", 10).value("status", ""), "error");
    EXPECT_EQ(h.getLatestTaskResult("t").value("status", ""), "error");
}

// ============================================================================
// getExecutionHistory – with audit manager enabled (second fixture)
// ============================================================================

class TaskSchedulerApiHandlerAuditWithLoggingTest : public ::testing::Test {
protected:
    static std::string makeDbPath() {
        auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        return (std::filesystem::temp_directory_path() /
                std::filesystem::path("themis_audit_logging_test_" + std::to_string(now))).string();
    }

    void SetUp() override {
        db_path_ = makeDbPath();
        std::filesystem::create_directories(db_path_);

        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_ + "/db";
        cfg.enable_blobdb = false;
        storage_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(storage_->open());

        idx_    = std::make_unique<SecondaryIndexManager>(*storage_);
        engine_ = std::make_unique<QueryEngine>(*storage_, *idx_);

        TaskScheduler::Config sched_cfg;
        sched_cfg.max_concurrent_tasks    = 2;
        sched_cfg.check_interval          = 50ms;
        sched_cfg.persist_tasks           = false;
        sched_cfg.enable_audit_logging    = true;  // ENABLED for this fixture
        sched_cfg.enable_anomaly_detection = false;

        scheduler_ = std::make_unique<TaskScheduler>(engine_.get(), sched_cfg);
        handler_   = std::make_unique<TaskSchedulerApiHandler>(scheduler_.get());
    }

    void TearDown() override {
        handler_.reset();
        if (scheduler_) {
            scheduler_->stop();
            scheduler_.reset();
        }
        engine_.reset();
        idx_.reset();
        storage_->close();
        storage_.reset();
        std::filesystem::remove_all(db_path_);
    }

    // Directly inject audit events via the audit manager
    void injectAuditEvent(const std::string& task_id, bool success,
                          scheduler::TaskEventType ev_type = scheduler::TaskEventType::TASK_COMPLETED,
                          const std::string& trigger_type = "CRON",
                          const std::string& user_id = "test_user") {
        auto am = scheduler_->getAuditManager();
        if (!am) {
          return;
        }
        scheduler::TaskAuditEvent ev;
        ev.uuid        = scheduler::generateUUID();
        ev.timestamp   = std::chrono::system_clock::now();
        ev.task_id     = task_id;
        ev.task_name   = task_id + "_name";
        ev.event_type  = ev_type;
        ev.trigger_type = trigger_type;
        ev.user_id     = user_id;
        ev.ip_address  = "127.0.0.1";
        ev.success     = success;
        ev.duration_ms = 50.0;
        am->logAuditEvent(ev);
    }

    std::string db_path_;
    std::unique_ptr<RocksDBWrapper>          storage_;
    std::unique_ptr<SecondaryIndexManager>   idx_;
    std::unique_ptr<QueryEngine>             engine_;
    std::unique_ptr<TaskScheduler>           scheduler_;
    std::unique_ptr<TaskSchedulerApiHandler> handler_;
};

TEST_F(TaskSchedulerApiHandlerAuditWithLoggingTest, GetExecutionHistory_ReturnsItems) {
    injectAuditEvent("task-a", true);
    injectAuditEvent("task-a", false, scheduler::TaskEventType::TASK_FAILED);
    injectAuditEvent("task-b", true);

    auto result = handler_->getExecutionHistory("task-a");
    ASSERT_TRUE(result.contains("items"));
    EXPECT_EQ(result["items"].size(), 2u);
    EXPECT_EQ(result["total"].get<int64_t>(), 2);
    for (const auto& item : result["items"]) {
        EXPECT_EQ(item.value("task_id", ""), "task-a");
    }
}

TEST_F(TaskSchedulerApiHandlerAuditWithLoggingTest, GetExecutionHistory_FilterBySuccess) {
    injectAuditEvent("task-x", true);
    injectAuditEvent("task-x", true);
    injectAuditEvent("task-x", false, scheduler::TaskEventType::TASK_FAILED);

    nlohmann::json params{{"success", true}, {"limit", 100}};
    auto result = handler_->getExecutionHistory("task-x", params);
    ASSERT_EQ(result["items"].size(), 2u);
    for (const auto& item : result["items"]) {
        EXPECT_TRUE(item.value("success", false));
    }
}

TEST_F(TaskSchedulerApiHandlerAuditWithLoggingTest, GetExecutionHistory_FilterByFailure) {
    injectAuditEvent("task-y", true);
    injectAuditEvent("task-y", false, scheduler::TaskEventType::TASK_FAILED);

    nlohmann::json params{{"success", false}, {"limit", 100}};
    auto result = handler_->getExecutionHistory("task-y", params);
    ASSERT_EQ(result["items"].size(), 1u);
    EXPECT_FALSE(result["items"][0].value("success", true));
}

TEST_F(TaskSchedulerApiHandlerAuditWithLoggingTest, GetExecutionHistory_Pagination) {
    for (int i = 0; i < 5; ++i) {
        injectAuditEvent("pg-task", true);
    }

    nlohmann::json p1{{"limit", 2}, {"offset", 0}};
    auto page1 = handler_->getExecutionHistory("pg-task", p1);
    EXPECT_EQ(page1["items"].size(), 2u);

    nlohmann::json p2{{"limit", 2}, {"offset", 2}};
    auto page2 = handler_->getExecutionHistory("pg-task", p2);
    EXPECT_EQ(page2["items"].size(), 2u);

    // Both pages should report the total count of all matching records (5)
    EXPECT_EQ(page1["total"].get<int64_t>(), 5);
    EXPECT_EQ(page2["total"].get<int64_t>(), 5);

    // No overlap by UUID
    std::set<std::string> ids1, ids2;
    for (const auto& ev : page1["items"]) {
      ids1.insert(ev.value("uuid", ""));
    }
    for (const auto& ev : page2["items"]) {
      ids2.insert(ev.value("uuid", ""));
    }
    for (const auto& u : ids2) {
        EXPECT_EQ(0u, ids1.count(u)) << "Duplicate UUID across pages: " << u;
    }
}

TEST_F(TaskSchedulerApiHandlerAuditWithLoggingTest, GetExecutionHistory_EmptyTaskId_ReturnsAllTasks) {
    injectAuditEvent("alpha", true);
    injectAuditEvent("beta", true);
    injectAuditEvent("gamma", true);

    auto result = handler_->getExecutionHistory("");
    EXPECT_GE(result["items"].size(), 3u);
}

TEST_F(TaskSchedulerApiHandlerAuditWithLoggingTest, GetExecutionHistory_FilterByEventType) {
    injectAuditEvent("ev-task", true,  scheduler::TaskEventType::TASK_COMPLETED);
    injectAuditEvent("ev-task", false, scheduler::TaskEventType::TASK_FAILED);

    nlohmann::json params{{"event_type", "TASK_FAILED"}, {"limit", 100}};
    auto result = handler_->getExecutionHistory("ev-task", params);
    ASSERT_EQ(result["items"].size(), 1u);
    EXPECT_EQ(result["items"][0].value("event_type", ""), "TASK_FAILED");
}

TEST_F(TaskSchedulerApiHandlerAuditWithLoggingTest, GetExecutionHistory_FilterByTriggerType) {
    injectAuditEvent("trig-task", true,  scheduler::TaskEventType::TASK_COMPLETED, "CRON");
    injectAuditEvent("trig-task", true,  scheduler::TaskEventType::TASK_COMPLETED, "MANUAL");

    nlohmann::json params{{"trigger_type", "MANUAL"}, {"limit", 100}};
    auto result = handler_->getExecutionHistory("trig-task", params);
    ASSERT_EQ(result["items"].size(), 1u);
    EXPECT_EQ(result["items"][0].value("trigger_type", ""), "MANUAL");
}

TEST_F(TaskSchedulerApiHandlerAuditWithLoggingTest, GetExecutionHistory_FilterByUserId) {
    injectAuditEvent("user-task", true,  scheduler::TaskEventType::TASK_COMPLETED, "CRON", "alice");
    injectAuditEvent("user-task", true,  scheduler::TaskEventType::TASK_COMPLETED, "CRON", "bob");

    nlohmann::json params{{"user_id", "alice"}, {"limit", 100}};
    auto result = handler_->getExecutionHistory("user-task", params);
    ASSERT_EQ(result["items"].size(), 1u);
    EXPECT_EQ(result["items"][0].value("user_id", ""), "alice");
}

// ============================================================================
// executeDAG
// ============================================================================

TEST_F(TaskSchedulerApiHandlerTest, ExecuteDAG_MissingTaskIds_ReturnsError) {
    auto result = handler_->executeDAG(nlohmann::json::object());
    EXPECT_EQ(result.value("status", ""), "error");
    EXPECT_NE(result.value("error", "").find("task_ids"), std::string::npos);
}

TEST_F(TaskSchedulerApiHandlerTest, ExecuteDAG_InvalidTaskIdsType_ReturnsError) {
    auto result = handler_->executeDAG(nlohmann::json{{"task_ids", "not_an_array"}});
    EXPECT_EQ(result.value("status", ""), "error");
}

TEST_F(TaskSchedulerApiHandlerTest, ExecuteDAG_UnknownTaskId_ReturnsError) {
    auto result = handler_->executeDAG(nlohmann::json{{"task_ids", {"does_not_exist"}}});
    EXPECT_EQ(result.value("status", ""), "error");
}

TEST_F(TaskSchedulerApiHandlerTest, ExecuteDAG_EmptyList_ReturnsExecuted) {
    auto result = handler_->executeDAG(nlohmann::json{{"task_ids", nlohmann::json::array()}});
    EXPECT_EQ(result.value("status", ""), "executed");
    EXPECT_TRUE(result["succeeded"].empty());
    EXPECT_TRUE(result["failed"].empty());
}

TEST_F(TaskSchedulerApiHandlerTest, ExecuteDAG_SingleTask_Succeeds) {
    // Register a function task
    scheduler_->registerFunction("dag_api_fn",
        [](const nlohmann::json&) -> nlohmann::json { return {{"ok", true}}; });

    nlohmann::json req = makeTaskJson("dag_api_task");
    req["type"]          = "function";
    req["function_name"] = "dag_api_fn";
    req["trigger_type"]  = "manual";
    req.erase("aql_query");
    auto reg = handler_->registerTask(req);
    ASSERT_EQ(reg.value("status", ""), "created");
    std::string task_id = reg["id"];

    auto result = handler_->executeDAG(nlohmann::json{{"task_ids", {task_id}}});
    EXPECT_EQ(result.value("status", ""), "executed");
    ASSERT_TRUE(result["succeeded"].contains(task_id));
    EXPECT_TRUE(result["failed"].empty());
    EXPECT_TRUE(result["skipped"].empty());
    EXPECT_TRUE(result["condition_skipped"].empty());
}

TEST_F(TaskSchedulerApiHandlerTest, ExecuteDAG_LinearChain_RespectsDependencyOrder) {
    // Register three function tasks: a -> b -> c
    auto noop_fn = [](const nlohmann::json&) -> nlohmann::json { return {{"ok", true}}; };
    for (const auto& name : std::vector<std::string>{"dag_a", "dag_b", "dag_c"}) {
        scheduler_->registerFunction(name + "_fn", noop_fn);
    }

    auto register_task = [&](const std::string& id,
                              const std::vector<std::string>& deps) -> std::string {
        ScheduledTask t;
        t.id = id; t.name = id;
        t.type = ScheduledTask::TaskType::FUNCTION;
        t.function_name = id + "_fn";
        t.trigger_type = ScheduledTask::TriggerType::MANUAL;
        t.dependencies = deps;
        return scheduler_->registerTask(t);
    };

    register_task("dag_a", {});
    register_task("dag_b", {"dag_a"});
    register_task("dag_c", {"dag_b"});

    auto result = handler_->executeDAG(
        nlohmann::json{{"task_ids", {"dag_a", "dag_b", "dag_c"}}});
    EXPECT_EQ(result.value("status", ""), "executed");
    EXPECT_EQ(result["succeeded"].size(), 3u);
    EXPECT_TRUE(result["failed"].empty());
    EXPECT_TRUE(result["skipped"].empty());
}

TEST_F(TaskSchedulerApiHandlerTest, ExecuteDAG_CyclicDependency_ReturnsError) {
    scheduler_->registerFunction("cyc_api_fn",
        [](const nlohmann::json&) -> nlohmann::json { return {}; });

    ScheduledTask ta;
    ta.id = "cyc_api_a"; ta.name = ta.id;
    ta.type = ScheduledTask::TaskType::FUNCTION;
    ta.function_name = "cyc_api_fn";
    ta.trigger_type = ScheduledTask::TriggerType::MANUAL;
    ta.dependencies = {"cyc_api_b"};
    scheduler_->registerTask(ta);

    ScheduledTask tb;
    tb.id = "cyc_api_b"; tb.name = tb.id;
    tb.type = ScheduledTask::TaskType::FUNCTION;
    tb.function_name = "cyc_api_fn";
    tb.trigger_type = ScheduledTask::TriggerType::MANUAL;
    tb.dependencies = {"cyc_api_a"};
    scheduler_->registerTask(tb);

    auto result = handler_->executeDAG(
        nlohmann::json{{"task_ids", {"cyc_api_a", "cyc_api_b"}}});
    EXPECT_EQ(result.value("status", ""), "error");
}

TEST_F(TaskSchedulerApiHandlerTest, RegisterTask_WithDependencies_RoundTrips) {
    // Register two tasks and verify dependencies survive registerTask -> getTask
    scheduler_->registerFunction("dep_fn",
        [](const nlohmann::json&) -> nlohmann::json { return {}; });

    nlohmann::json req_a = makeTaskJson("dep_task_a");
    req_a["type"]          = "function";
    req_a["function_name"] = "dep_fn";
    req_a["trigger_type"]  = "manual";
    req_a.erase("aql_query");
    auto reg_a = handler_->registerTask(req_a);
    ASSERT_EQ(reg_a.value("status", ""), "created");

    nlohmann::json req_b = makeTaskJson("dep_task_b");
    req_b["type"]          = "function";
    req_b["function_name"] = "dep_fn";
    req_b["trigger_type"]  = "manual";
    req_b["dependencies"]  = nlohmann::json::array({reg_a["id"].get<std::string>()});
    req_b.erase("aql_query");
    auto reg_b = handler_->registerTask(req_b);
    ASSERT_EQ(reg_b.value("status", ""), "created");

    // getTask should return the dependencies
    auto detail = handler_->getTask(reg_b["id"].get<std::string>());
    ASSERT_TRUE(detail.contains("dependencies"));
    ASSERT_EQ(detail["dependencies"].size(), 1u);
    EXPECT_EQ(detail["dependencies"][0].get<std::string>(), reg_a["id"].get<std::string>());
}

// ============================================================================
// External scheduler – Kubernetes CronJob export (JSON)
// ============================================================================

TEST_F(TaskSchedulerApiHandlerTest, ExportToK8sJson_ReturnsManifest) {
    auto reg = handler_->registerTask(makeTaskJson("export_k8s_task"));
    ASSERT_EQ(reg.value("status", ""), "created");
    const std::string task_id = reg["id"].get<std::string>();

    nlohmann::json req{{"themisdb_base_url", "https://themisdb.example.com"}};
    auto result = handler_->exportToKubernetesCronJobJson(task_id, req);

    ASSERT_TRUE(result.contains("manifest")) << result.dump();
    const auto& manifest = result["manifest"];
    EXPECT_EQ(manifest.value("kind", ""), "CronJob");
    EXPECT_EQ(manifest.value("apiVersion", ""), "batch/v1");
}

TEST_F(TaskSchedulerApiHandlerTest, ExportToK8sJson_UnknownTaskReturnsError) {
    nlohmann::json req{{"themisdb_base_url", "https://themisdb.example.com"}};
    auto result = handler_->exportToKubernetesCronJobJson("no_such_task", req);
    EXPECT_EQ(result.value("status", ""), "error");
}

TEST_F(TaskSchedulerApiHandlerTest, ExportToK8sJson_MissingBaseUrlReturnsError) {
    auto reg = handler_->registerTask(makeTaskJson("k8s_nourl_task"));
    ASSERT_EQ(reg.value("status", ""), "created");
    const std::string task_id = reg["id"].get<std::string>();

    auto result = handler_->exportToKubernetesCronJobJson(task_id, nlohmann::json::object());
    EXPECT_EQ(result.value("status", ""), "error");
}

TEST_F(TaskSchedulerApiHandlerTest, ExportToK8sJson_NamespaceAndSuspendPropagated) {
    auto reg = handler_->registerTask(makeTaskJson("k8s_ns_task"));
    ASSERT_EQ(reg.value("status", ""), "created");
    const std::string task_id = reg["id"].get<std::string>();

    nlohmann::json req{
        {"themisdb_base_url", "https://themisdb.example.com"},
        {"k8s_namespace",     "production"},
        {"suspend",           true}
    };
    auto result = handler_->exportToKubernetesCronJobJson(task_id, req);
    ASSERT_TRUE(result.contains("manifest")) << result.dump();
    EXPECT_EQ(result["manifest"]["metadata"]["namespace"].get<std::string>(), "production");
    EXPECT_TRUE(result["manifest"]["spec"]["suspend"].get<bool>());
}

// ============================================================================
// External scheduler – Kubernetes CronJob export (YAML)
// ============================================================================

TEST_F(TaskSchedulerApiHandlerTest, ExportToK8sYaml_ReturnsYamlString) {
    auto reg = handler_->registerTask(makeTaskJson("export_yaml_task"));
    ASSERT_EQ(reg.value("status", ""), "created");
    const std::string task_id = reg["id"].get<std::string>();

    nlohmann::json req{{"themisdb_base_url", "https://themisdb.example.com"}};
    auto result = handler_->exportToKubernetesCronJobYaml(task_id, req);

    ASSERT_TRUE(result.contains("yaml")) << result.dump();
    const std::string yaml = result["yaml"].get<std::string>();
    EXPECT_NE(yaml.find("CronJob"), std::string::npos) << yaml;
    EXPECT_NE(yaml.find("batch/v1"), std::string::npos) << yaml;
}

TEST_F(TaskSchedulerApiHandlerTest, ExportToK8sYaml_UnknownTaskReturnsError) {
    nlohmann::json req{{"themisdb_base_url", "https://themisdb.example.com"}};
    auto result = handler_->exportToKubernetesCronJobYaml("no_such_task", req);
    EXPECT_EQ(result.value("status", ""), "error");
}

// ============================================================================
// External scheduler – Airflow DAG export
// ============================================================================

TEST_F(TaskSchedulerApiHandlerTest, ExportToAirflowDag_ReturnsDagPython) {
    auto reg = handler_->registerTask(makeTaskJson("airflow_task1"));
    ASSERT_EQ(reg.value("status", ""), "created");
    const std::string task_id = reg["id"].get<std::string>();

    nlohmann::json req{
        {"task_ids", nlohmann::json::array({task_id})},
        {"dag_id",   "test_dag"},
        {"start_date", "2026-01-01"}
    };
    auto result = handler_->exportToAirflowDag(req);

    ASSERT_TRUE(result.contains("dag_python")) << result.dump();
    const std::string py = result["dag_python"].get<std::string>();
    EXPECT_NE(py.find("from airflow import DAG"), std::string::npos) << py;
    EXPECT_NE(py.find("SimpleHttpOperator"),       std::string::npos) << py;
    EXPECT_NE(py.find("test_dag"),                 std::string::npos) << py;
}

TEST_F(TaskSchedulerApiHandlerTest, ExportToAirflowDag_MissingTaskIdsReturnsError) {
    auto result = handler_->exportToAirflowDag(nlohmann::json::object());
    EXPECT_EQ(result.value("status", ""), "error");
}

TEST_F(TaskSchedulerApiHandlerTest, ExportToAirflowDag_UnknownTaskIdReturnsError) {
    nlohmann::json req{{"task_ids", nlohmann::json::array({"no_such_task"})}};
    auto result = handler_->exportToAirflowDag(req);
    EXPECT_EQ(result.value("status", ""), "error");
}

TEST_F(TaskSchedulerApiHandlerTest, ExportToAirflowDag_MultipleTasksWired) {
    auto r1 = handler_->registerTask(makeTaskJson("extract_task"));
    auto r2 = handler_->registerTask(makeTaskJson("load_task"));
    ASSERT_EQ(r1.value("status", ""), "created");
    ASSERT_EQ(r2.value("status", ""), "created");

    nlohmann::json req{
        {"task_ids", nlohmann::json::array({
            r1["id"].get<std::string>(),
            r2["id"].get<std::string>()
        })},
        {"dag_id", "multi_task_dag"},
        {"start_date", "2026-01-01"}
    };
    auto result = handler_->exportToAirflowDag(req);
    ASSERT_TRUE(result.contains("dag_python")) << result.dump();
}

// ============================================================================
// External scheduler – Kubernetes CronJob import
// ============================================================================

TEST_F(TaskSchedulerApiHandlerTest, ImportFromK8sCronJob_CreatesTask) {
    nlohmann::json manifest{
        {"apiVersion", "batch/v1"},
        {"kind",       "CronJob"},
        {"metadata", {
            {"name",       "my-imported-job"},
            {"namespace",  "default"},
            {"annotations", {
                {"themisdb/task-name",        "Imported Task"},
                {"themisdb/task-description", "Imported from K8s"},
                {"themisdb/task-id",          "my-imported-job"}
            }}
        }},
        {"spec", {
            {"schedule", "0 * * * *"}
        }}
    };

    auto result = handler_->importFromKubernetesCronJob(manifest);
    EXPECT_EQ(result.value("status", ""), "created") << result.dump();
    ASSERT_TRUE(result.contains("id"));
    EXPECT_FALSE(result["id"].get<std::string>().empty());
}

TEST_F(TaskSchedulerApiHandlerTest, ImportFromK8sCronJob_MissingMetadataReturnsError) {
    nlohmann::json bad_manifest{
        {"apiVersion", "batch/v1"},
        {"kind",       "CronJob"},
        {"spec", {{"schedule", "* * * * *"}}}
    };
    auto result = handler_->importFromKubernetesCronJob(bad_manifest);
    EXPECT_EQ(result.value("status", ""), "error");
}

TEST_F(TaskSchedulerApiHandlerTest, ImportFromK8sCronJob_MissingScheduleReturnsError) {
    nlohmann::json bad_manifest{
        {"apiVersion", "batch/v1"},
        {"kind",       "CronJob"},
        {"metadata", {{"name", "no-schedule"}}},
        {"spec", nlohmann::json::object()}
    };
    auto result = handler_->importFromKubernetesCronJob(bad_manifest);
    EXPECT_EQ(result.value("status", ""), "error");
}

TEST_F(TaskSchedulerApiHandlerTest, ImportFromK8sCronJob_ImportedTaskAppearsInList) {
    nlohmann::json manifest{
        {"apiVersion", "batch/v1"},
        {"kind",       "CronJob"},
        {"metadata", {
            {"name", "listed-job"},
            {"annotations", {
                {"themisdb/task-id", "listed-job"}
            }}
        }},
        {"spec", {{"schedule", "*/5 * * * *"}}}
    };
    auto import_result = handler_->importFromKubernetesCronJob(manifest);
    ASSERT_EQ(import_result.value("status", ""), "created");

    auto list_result = handler_->listTasks();
    ASSERT_TRUE(list_result.contains("items"));
    bool found = false;
    for (const auto& item : list_result["items"]) {
        if (item.value("id", "") == import_result["id"].get<std::string>()) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Imported task not found in task list";
}

// ============================================================================
// GAP-018 — e.what() must NOT be forwarded into HTTP API responses (CWE-209)
// ============================================================================

// GAP-018-01: registerTask with a malformed request must return a generic error
// message (not the raw exception text) in the JSON response.
TEST_F(TaskSchedulerApiHandlerTest, GAP018_RegisterTask_ErrorResponseIsGeneric) {
    // Intentionally malformed: "type" and "trigger_type" are missing so
    // parseTaskFromJson must throw, exercising the catch block.
    nlohmann::json bad_req{{"name", "bad_task"}};
    auto result = handler_->registerTask(bad_req);

    ASSERT_EQ(result.value("status", ""), "error");
    const std::string err = result.value("error", "");
    // The response must NOT contain raw C++ exception text that would reveal
    // internal implementation details (CWE-209 / GAP-018).
    EXPECT_NE(err, "") << "Error field must be present";
    EXPECT_EQ(err, "Internal server error")
        << "Expected generic message; got: " << err;
}

// GAP-018-02: unregisterTask with an unknown task ID returns a generic error.
TEST_F(TaskSchedulerApiHandlerTest, GAP018_UnregisterTask_UnknownId_ErrorResponseIsGeneric) {
    auto result = handler_->unregisterTask("nonexistent_xyz");

    ASSERT_EQ(result.value("status", ""), "error");
    const std::string err = result.value("error", "");
    EXPECT_NE(err, "");
    EXPECT_TRUE(err.find("std::") == std::string::npos)
        << "Response must not expose C++ exception type; got: " << err;
    EXPECT_TRUE(err.find("exception") == std::string::npos)
        << "Response must not expose exception details; got: " << err;
}

// GAP-018-03: enableTask with an unknown task ID returns a generic error.
TEST_F(TaskSchedulerApiHandlerTest, GAP018_EnableTask_UnknownId_ErrorResponseIsGeneric) {
    auto result = handler_->enableTask("nonexistent_xyz");

    ASSERT_EQ(result.value("status", ""), "error");
    const std::string err = result.value("error", "");
    EXPECT_NE(err, "");
    EXPECT_TRUE(err.find("std::") == std::string::npos);
}

// GAP-018-04: disableTask with an unknown task ID returns a generic error.
TEST_F(TaskSchedulerApiHandlerTest, GAP018_DisableTask_UnknownId_ErrorResponseIsGeneric) {
    auto result = handler_->disableTask("nonexistent_xyz");

    ASSERT_EQ(result.value("status", ""), "error");
    const std::string err = result.value("error", "");
    EXPECT_NE(err, "");
    EXPECT_TRUE(err.find("std::") == std::string::npos);
}

// ============================================================================
// GAP-019 — mt19937 replaced with random_device for security-sensitive IDs
// ============================================================================

// GAP-019-01: Registered task IDs must be unique (verifies that the underlying
// UUID generator produces non-colliding values even on rapid successive calls).
TEST_F(TaskSchedulerApiHandlerTest, GAP019_RegisteredTaskIds_AreUnique) {
    auto t1 = handler_->registerTask(makeTaskJson("rng_task_1"));
    auto t2 = handler_->registerTask(makeTaskJson("rng_task_2"));
    ASSERT_EQ(t1.value("status", ""), "created");
    ASSERT_EQ(t2.value("status", ""), "created");
    EXPECT_NE(t1.value("id", ""), t2.value("id", ""))
        << "Task IDs must be unique across consecutive registrations";
}
