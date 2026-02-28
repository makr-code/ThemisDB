/**
 * @file test_task_scheduler_api_handler.cpp
 * @brief Unit tests for TaskSchedulerApiHandler
 */

#include <gtest/gtest.h>
#include "server/task_scheduler_api_handler.h"
#include "scheduler/task_scheduler.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "query/query_engine.h"
#include <filesystem>
#include <chrono>

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

    nlohmann::json req{{"name", "t"}, {"type", "aql_query"}, {"aql_query", "RETURN 1"}};
    EXPECT_EQ(h.registerTask(req).value("status", ""), "error");

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
        if (trigger == "cron") req["cron_expression"] = "*/5 * * * *";
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
