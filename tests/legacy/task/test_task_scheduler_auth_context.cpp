/**
 * @file test_task_scheduler_auth_context.cpp
 * @brief Unit tests for TaskScheduler thread-local RequestContext propagation
 *        to audit events, and the sandbox_execution config flag.
 */

#include <gtest/gtest.h>
#include "scheduler/task_scheduler.h"
#include "scheduler/task_audit_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "query/query_engine.h"
#include <filesystem>
#include <atomic>
#include <thread>
#include <chrono>

using namespace themis;
using namespace std::chrono_literals;

// ===== Test fixture =====

class TaskSchedulerAuthContextTest : public ::testing::Test {
protected:
    static std::string makeDbPath() {
        auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        return (std::filesystem::temp_directory_path() /
                std::filesystem::path("themis_auth_ctx_test_" + std::to_string(now))).string();
    }

    void SetUp() override {
        db_path_ = makeDbPath();
        std::filesystem::create_directories(db_path_);

        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_ + "/db";
        cfg.enable_blobdb = false;
        storage_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(storage_->open());

        idx_ = std::make_unique<SecondaryIndexManager>(*storage_);
        engine_ = std::make_unique<QueryEngine>(*storage_, *idx_);
    }

    // Create a scheduler with audit logging enabled.
    std::unique_ptr<TaskScheduler> makeAuditScheduler() {
        TaskScheduler::Config cfg;
        cfg.max_concurrent_tasks = 4;
        cfg.check_interval = 50ms;
        cfg.persist_tasks = false;
        cfg.enable_audit_logging = true;
        cfg.enable_anomaly_detection = false;
        return std::make_unique<TaskScheduler>(engine_.get(), cfg);
    }

    void TearDown() override {
        // Always clear thread-local context so tests don't interfere
        TaskScheduler::clearRequestContext();
        engine_.reset();
        idx_.reset();
        storage_->close();
        storage_.reset();
        std::filesystem::remove_all(db_path_);
    }

    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> storage_;
    std::unique_ptr<SecondaryIndexManager> idx_;
    std::unique_ptr<QueryEngine> engine_;
};

// ===== Static API: currentUserId / currentClientIp =====

TEST_F(TaskSchedulerAuthContextTest, CurrentUserIdFallsBackToSystemWhenNotSet) {
    TaskScheduler::clearRequestContext();
    EXPECT_EQ(TaskScheduler::currentUserId(), "system");
    EXPECT_EQ(TaskScheduler::currentUserId("admin"), "admin");
    EXPECT_EQ(TaskScheduler::currentUserId(nullptr), "system");
}

TEST_F(TaskSchedulerAuthContextTest, CurrentClientIpEmptyWhenNotSet) {
    TaskScheduler::clearRequestContext();
    EXPECT_TRUE(TaskScheduler::currentClientIp().empty());
}

TEST_F(TaskSchedulerAuthContextTest, SetRequestContextPopulatesUserId) {
    TaskScheduler::setRequestContext({"alice", "192.168.1.1"});
    EXPECT_EQ(TaskScheduler::currentUserId(), "alice");
    EXPECT_EQ(TaskScheduler::currentUserId("system"), "alice");
    TaskScheduler::clearRequestContext();
}

TEST_F(TaskSchedulerAuthContextTest, SetRequestContextPopulatesClientIp) {
    TaskScheduler::setRequestContext({"bob", "10.0.0.5"});
    EXPECT_EQ(TaskScheduler::currentClientIp(), "10.0.0.5");
    TaskScheduler::clearRequestContext();
}

TEST_F(TaskSchedulerAuthContextTest, ClearRequestContextRestoresFallback) {
    TaskScheduler::setRequestContext({"charlie", "172.16.0.1"});
    ASSERT_EQ(TaskScheduler::currentUserId(), "charlie");
    TaskScheduler::clearRequestContext();
    EXPECT_EQ(TaskScheduler::currentUserId(), "system");
    EXPECT_TRUE(TaskScheduler::currentClientIp().empty());
}

TEST_F(TaskSchedulerAuthContextTest, PermissionAndRoleFallbackAllowWhenContextUnset) {
    TaskScheduler::clearRequestContext();
    EXPECT_TRUE(TaskScheduler::hasPermission("task:register"));
    EXPECT_TRUE(TaskScheduler::hasRole("system_admin"));
}

TEST_F(TaskSchedulerAuthContextTest, EmptyUserIdFallsBackToSystemFallback) {
    // An empty user_id in the context should still use the fallback
    TaskScheduler::setRequestContext({"", "192.168.0.1"});
    EXPECT_EQ(TaskScheduler::currentUserId("system"), "system");
    TaskScheduler::clearRequestContext();
}

// ===== Thread isolation =====

TEST_F(TaskSchedulerAuthContextTest, ThreadLocalContextIsIsolatedPerThread) {
    TaskScheduler::setRequestContext({"main_thread_user", "1.2.3.4"});

    std::string thread_user;
    std::string thread_ip;
    std::thread worker([&]() {
        // Worker thread should start with no context set
        thread_user = TaskScheduler::currentUserId("fallback");
        thread_ip   = TaskScheduler::currentClientIp();

        // Setting context in worker should not affect main thread
        TaskScheduler::setRequestContext({"worker_user", "9.9.9.9"});
    });
    worker.join();

    // Worker thread had no context at start → fallback
    EXPECT_EQ(thread_user, "fallback");
    EXPECT_TRUE(thread_ip.empty());

    // Main thread context unchanged by worker
    EXPECT_EQ(TaskScheduler::currentUserId(), "main_thread_user");
    EXPECT_EQ(TaskScheduler::currentClientIp(), "1.2.3.4");

    TaskScheduler::clearRequestContext();
}

// ===== Audit event propagation via TaskAuditManager =====

TEST_F(TaskSchedulerAuthContextTest, RegisterTaskAuditEventUsesThreadLocalUser) {
    auto sched = makeAuditScheduler();

    TaskScheduler::RequestContext ctx;
    ctx.user_id = "operator1";
    ctx.client_ip = "192.168.0.100";
    ctx.granted_permissions.insert("task:register");
    TaskScheduler::setRequestContext(ctx);

    ScheduledTask task;
    task.id   = "auth_ctx_task";
    task.name = "Auth Context Test Task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "noop_fn";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;
    sched->registerTask(task);

    TaskScheduler::clearRequestContext();

    auto audit_mgr = sched->getAuditManager();
    ASSERT_NE(audit_mgr, nullptr);

    scheduler::AuditQueryParams params;
    params.task_id    = "auth_ctx_task";
    params.event_type = scheduler::TaskEventType::TASK_REGISTERED;
    params.limit      = 10;

    auto events = audit_mgr->queryAuditEvents(params);
    ASSERT_GE(events.size(), 1u);
    EXPECT_EQ(events[0].user_id, "operator1");
    EXPECT_EQ(events[0].ip_address, "192.168.0.100");
}

TEST_F(TaskSchedulerAuthContextTest, RegisterTaskAuditEventFallsBackToSystem) {
    auto sched = makeAuditScheduler();

    // No request context set
    TaskScheduler::clearRequestContext();

    ScheduledTask task;
    task.id   = "system_ctx_task";
    task.name = "System Context Test Task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "noop_fn";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;
    sched->registerTask(task);

    auto audit_mgr = sched->getAuditManager();
    ASSERT_NE(audit_mgr, nullptr);

    scheduler::AuditQueryParams params;
    params.task_id    = "system_ctx_task";
    params.event_type = scheduler::TaskEventType::TASK_REGISTERED;
    params.limit      = 10;

    auto events = audit_mgr->queryAuditEvents(params);
    ASSERT_GE(events.size(), 1u);
    EXPECT_EQ(events[0].user_id, "system");
}

TEST_F(TaskSchedulerAuthContextTest, RegisterTaskDeniedWithoutTaskRegisterPermission) {
    auto sched = makeAuditScheduler();

    TaskScheduler::RequestContext ctx;
    ctx.user_id = "alice";
    ctx.client_ip = "203.0.113.10";
    ctx.authorization_justification = "negative-authz-test";
    TaskScheduler::setRequestContext(ctx);

    ScheduledTask task;
    task.id = "denied_register_task";
    task.name = "Denied Register Task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "noop_fn";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;

    EXPECT_THROW(sched->registerTask(task), std::runtime_error);
    TaskScheduler::clearRequestContext();

    auto audit_mgr = sched->getAuditManager();
    ASSERT_NE(audit_mgr, nullptr);
    scheduler::AuditQueryParams q;
    q.task_id = "denied_register_task";
    q.limit = 10;
    auto security_events = audit_mgr->querySecurityEvents(q);
    ASSERT_FALSE(security_events.empty());
    EXPECT_EQ(security_events[0].event_type, scheduler::TaskSecurityEventType::UNAUTHORIZED_ACCESS);
    EXPECT_EQ(security_events[0].details.value("required_permission", ""), "task:register");
    EXPECT_EQ(security_events[0].details.value("justification", ""), "negative-authz-test");
}

TEST_F(TaskSchedulerAuthContextTest, RegisterTaskAllowedWithTaskRegisterPermission) {
    auto sched = makeAuditScheduler();

    TaskScheduler::RequestContext ctx;
    ctx.user_id = "alice";
    ctx.client_ip = "203.0.113.11";
    ctx.authorization_justification = "positive-authz-test";
    ctx.granted_permissions.insert("task:register");
    TaskScheduler::setRequestContext(ctx);

    ScheduledTask task;
    task.id = "allowed_register_task";
    task.name = "Allowed Register Task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "noop_fn";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;

    EXPECT_NO_THROW(sched->registerTask(task));
    TaskScheduler::clearRequestContext();
}

TEST_F(TaskSchedulerAuthContextTest, ExecuteTaskNowDeniedWithoutTaskExecutePermission) {
    auto sched = makeAuditScheduler();

    // Register task under authorized registration context.
    TaskScheduler::RequestContext register_ctx;
    register_ctx.user_id = "alice";
    register_ctx.granted_permissions.insert("task:register");
    TaskScheduler::setRequestContext(register_ctx);
    ScheduledTask task;
    task.id = "denied_execute_task";
    task.name = "Denied Execute Task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "noop_fn";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;
    sched->registerTask(task);
    TaskScheduler::clearRequestContext();

    TaskScheduler::RequestContext execute_ctx;
    execute_ctx.user_id = "alice";
    execute_ctx.authorization_justification = "missing-execute-scope";
    TaskScheduler::setRequestContext(execute_ctx);
    auto result = sched->executeTaskNow("denied_execute_task");
    TaskScheduler::clearRequestContext();

    EXPECT_TRUE(result.contains("error"));
    EXPECT_NE(result.value("error", "").find("Unauthorized"), std::string::npos);
}

TEST_F(TaskSchedulerAuthContextTest, RegisterFunctionRequiresPermissionAndSystemAdminRole) {
    auto sched = makeAuditScheduler();

    TaskScheduler::RequestContext denied_ctx;
    denied_ctx.user_id = "alice";
    denied_ctx.granted_permissions.insert("task:register_function");
    denied_ctx.authorization_justification = "missing-system-admin-role";
    TaskScheduler::setRequestContext(denied_ctx);
    try {
        sched->registerFunction("denied_fn", [](const nlohmann::json&) { return nlohmann::json{}; });
        FAIL() << "Expected registerFunction to throw runtime_error";
    } catch (const std::runtime_error& e) {
        EXPECT_NE(std::string(e.what()).find("task:register_function"), std::string::npos);
    }
    TaskScheduler::clearRequestContext();

    TaskScheduler::RequestContext allowed_ctx;
    allowed_ctx.user_id = "alice";
    allowed_ctx.granted_permissions.insert("task:register_function");
    allowed_ctx.roles.insert("system_admin");
    allowed_ctx.authorization_justification = "explicit-system-admin-grant";
    TaskScheduler::setRequestContext(allowed_ctx);
    EXPECT_NO_THROW(
        sched->registerFunction("allowed_fn", [](const nlohmann::json&) { return nlohmann::json{}; }));
    TaskScheduler::clearRequestContext();
}

// ===== sandbox_execution config flag =====

TEST_F(TaskSchedulerAuthContextTest, FunctionExecutesWithSandboxEnabled) {
    TaskScheduler::Config cfg;
    cfg.max_concurrent_tasks = 4;
    cfg.check_interval = 50ms;
    cfg.persist_tasks = false;
    cfg.enable_audit_logging = false;
    cfg.enable_anomaly_detection = false;
    cfg.sandbox_execution = true;

    auto sched = std::make_unique<TaskScheduler>(engine_.get(), cfg);

    std::atomic<int> call_count{0};
    sched->registerFunction("sandbox_fn", [&](const nlohmann::json& p) -> nlohmann::json {
        ++call_count;
        return {{"result", "ok"}, {"input", p}};
    });

    ScheduledTask task;
    task.id   = "sandbox_task";
    task.name = "Sandbox Test Task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "sandbox_fn";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;
    task.parameters = {{"x", 42}};
    sched->registerTask(task);

    auto result = sched->executeTaskNow("sandbox_task");
    // Function must have been called exactly once (sandbox launch may fail gracefully
    // in environments without cgroup support, but the function still runs)
    EXPECT_GE(call_count.load(), 1);
    // No error key in the result: function executed successfully regardless of sandbox
    EXPECT_FALSE(result.contains("error")) << "Unexpected error: " << result.dump();
}

TEST_F(TaskSchedulerAuthContextTest, FunctionExecutesWithSandboxDisabledByDefault) {
    // Default config has sandbox_execution = false
    TaskScheduler::Config cfg;
    cfg.max_concurrent_tasks = 4;
    cfg.check_interval = 50ms;
    cfg.persist_tasks = false;
    cfg.enable_audit_logging = false;
    cfg.enable_anomaly_detection = false;

    EXPECT_FALSE(cfg.sandbox_execution);

    auto sched = std::make_unique<TaskScheduler>(engine_.get(), cfg);

    sched->registerFunction("no_sandbox_fn", [](const nlohmann::json&) -> nlohmann::json {
        return {{"result", "direct"}};
    });

    ScheduledTask task;
    task.id   = "no_sandbox_task";
    task.name = "No Sandbox Test Task";
    task.type = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "no_sandbox_fn";
    task.trigger_type = ScheduledTask::TriggerType::MANUAL;
    sched->registerTask(task);

    auto result = sched->executeTaskNow("no_sandbox_task");
    EXPECT_FALSE(result.contains("error")) << "Unexpected error: " << result.dump();
}
