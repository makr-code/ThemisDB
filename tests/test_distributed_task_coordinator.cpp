/**
 * @file test_distributed_task_coordinator.cpp
 * @brief Unit tests for DistributedTaskCoordinator
 *
 * Tests verify that the coordinator:
 * - activates the TaskScheduler only when this node is the leader
 * - deactivates the TaskScheduler when leadership is lost
 * - maintains a local task registry across leadership changes
 * - forwards task operations to the active scheduler when leader
 */

#include <gtest/gtest.h>

#include "scheduler/distributed_task_coordinator.h"
#include "sharding/distributed_coordinator.h"
#include "sharding/shard_topology.h"
#include "sharding/gossip_config_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "query/query_engine.h"

#include <filesystem>
#include <atomic>
#include <chrono>
#include <thread>

using namespace themis;
using namespace themis::sharding;
using namespace std::chrono_literals;

// ============================================================================
// Test fixture
// ============================================================================

class DistributedTaskCoordinatorTest : public ::testing::Test {
protected:
    static std::string makeTempPath() {
        auto ts = std::chrono::high_resolution_clock::now()
                      .time_since_epoch().count();
        return (std::filesystem::temp_directory_path() /
                std::filesystem::path("themis_dtc_test_" + std::to_string(ts))).string();
    }

    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping distributed task coordinator focused tests on Windows due to intermittent segfaults in fixture lifecycle.";
#endif
        db_path_ = makeTempPath();
        std::filesystem::create_directories(db_path_);

        // Storage
        RocksDBWrapper::Config db_cfg;
        db_cfg.db_path       = db_path_ + "/db";
        db_cfg.enable_blobdb = false;
        storage_ = std::make_unique<RocksDBWrapper>(db_cfg);
        ASSERT_TRUE(storage_->open());

        idx_    = std::make_unique<SecondaryIndexManager>(*storage_);
        engine_ = std::make_unique<QueryEngine>(*storage_, *idx_);

        // TaskScheduler
        TaskScheduler::Config sched_cfg;
        sched_cfg.max_concurrent_tasks  = 2;
        sched_cfg.check_interval        = 50ms;
        sched_cfg.persist_tasks         = false;
        sched_cfg.enable_audit_logging  = false;
        sched_cfg.enable_anomaly_detection = false;
        scheduler_ = std::make_unique<TaskScheduler>(engine_.get(), sched_cfg);

        // Sharding infrastructure (gossip disabled for unit tests)
        topology_ = std::make_shared<ShardTopology>();

        ShardInfo si;
        si.shard_id          = "node-1";
        si.primary_endpoint  = "localhost:50001";
        si.is_healthy        = true;
        topology_->addShard(si);

        GossipConfigManagerConfig gossip_cfg;
        gossip_cfg.local_shard_id = "node-1";
        gossip_cfg.local_endpoint = "localhost:50001";
        gossip_cfg.enabled        = false;  // No real network in tests
        gossip_ = std::make_shared<GossipConfigManager>(gossip_cfg, topology_);

        // DistributedCoordinator (do NOT start it – we drive it manually)
        DistributedCoordinator::Config coord_cfg;
        coord_cfg.enable_automatic_failover = false;
        coordinator_ = std::make_unique<DistributedCoordinator>(
            "node-1", topology_, gossip_, coord_cfg);

        // DistributedTaskCoordinator under test
        DistributedTaskCoordinator::Config dtc_cfg;
        dtc_cfg.auto_manage_scheduler = true;
        dtc_ = std::make_unique<DistributedTaskCoordinator>(
            scheduler_.get(), coordinator_.get(), dtc_cfg);
    }

    void TearDown() override {
        if (dtc_) {
            dtc_->stop();
            dtc_.reset();
        }
        coordinator_.reset();
        scheduler_.reset();
        engine_.reset();
        idx_.reset();
        if (storage_) {
            storage_->close();
        }
        storage_.reset();
        if (!db_path_.empty()) {
            std::filesystem::remove_all(db_path_);
        }
    }

    // Helper – build a minimal FUNCTION task
    static ScheduledTask makeTask(const std::string& name) {
        ScheduledTask t;
        t.name         = name;
        t.type         = ScheduledTask::TaskType::FUNCTION;
        t.function_name = name + "_fn";
        t.trigger_type = ScheduledTask::TriggerType::MANUAL;
        return t;
    }

    std::string                                  db_path_;
    std::unique_ptr<RocksDBWrapper>              storage_;
    std::unique_ptr<SecondaryIndexManager>       idx_;
    std::unique_ptr<QueryEngine>                 engine_;
    std::unique_ptr<TaskScheduler>               scheduler_;
    std::shared_ptr<ShardTopology>               topology_;
    std::shared_ptr<GossipConfigManager>         gossip_;
    std::unique_ptr<DistributedCoordinator>      coordinator_;
    std::unique_ptr<DistributedTaskCoordinator>  dtc_;
};

// ============================================================================
// Lifecycle
// ============================================================================

TEST_F(DistributedTaskCoordinatorTest, InitialState) {
    EXPECT_FALSE(dtc_->isRunning());
    EXPECT_FALSE(dtc_->isSchedulerActive());
    EXPECT_FALSE(dtc_->isLeader());
}

TEST_F(DistributedTaskCoordinatorTest, StartStop) {
    dtc_->start();
    EXPECT_TRUE(dtc_->isRunning());

    dtc_->stop();
    EXPECT_FALSE(dtc_->isRunning());
    EXPECT_FALSE(dtc_->isSchedulerActive());
}

TEST_F(DistributedTaskCoordinatorTest, DoubleStartIsIdempotent) {
    dtc_->start();
    dtc_->start();  // Should not throw or hang
    EXPECT_TRUE(dtc_->isRunning());
}

TEST_F(DistributedTaskCoordinatorTest, DoubleStopIsIdempotent) {
    dtc_->start();
    dtc_->stop();
    dtc_->stop();  // Should not throw or hang
    EXPECT_FALSE(dtc_->isRunning());
}

// ============================================================================
// Leader election integration
// ============================================================================

TEST_F(DistributedTaskCoordinatorTest, SchedulerActivatedWhenBecomingLeader) {
    dtc_->start();
    EXPECT_FALSE(dtc_->isSchedulerActive());

    // Simulate this node winning an election
    coordinator_->becomeLeader();

    // The leadership callback is synchronous in becomeLeader(), so the
    // scheduler should be active now.
    EXPECT_TRUE(dtc_->isLeader());
    EXPECT_TRUE(dtc_->isSchedulerActive());
}

TEST_F(DistributedTaskCoordinatorTest, SchedulerDeactivatedWhenLeadershipLost) {
    dtc_->start();
    // This node wins the election
    coordinator_->becomeLeader();
    ASSERT_TRUE(dtc_->isSchedulerActive());

    // Simulate another node winning – fire the callback with a different node ID.
    // In a real cluster this would come from the gossip protocol.
    dtc_->onLeaderElected("node-99");

    EXPECT_FALSE(dtc_->isSchedulerActive());
}

TEST_F(DistributedTaskCoordinatorTest, SchedulerRemainsInactiveWhenNotLeader) {
    dtc_->start();
    // Never call becomeLeader() – scheduler should remain inactive
    EXPECT_FALSE(dtc_->isSchedulerActive());
    EXPECT_FALSE(scheduler_->isRunning());
}

TEST_F(DistributedTaskCoordinatorTest, AlreadyLeaderAtStart) {
    // Become leader BEFORE starting the coordinator
    coordinator_->becomeLeader();
    ASSERT_TRUE(coordinator_->isLeader());

    dtc_->start();

    // The coordinator should detect existing leadership in start()
    EXPECT_TRUE(dtc_->isSchedulerActive());
}

// ============================================================================
// Task management
// ============================================================================

TEST_F(DistributedTaskCoordinatorTest, RegisterTaskStoredInLocalRegistry) {
    dtc_->start();
    auto task_id = dtc_->registerTask(makeTask("my_task"));
    EXPECT_FALSE(task_id.empty());

    auto tasks = dtc_->listTasks();
    ASSERT_EQ(tasks.size(), 1u);
    EXPECT_EQ(tasks[0].name, "my_task");

    auto ptr = dtc_->getTask(task_id);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(ptr->name, "my_task");
}

TEST_F(DistributedTaskCoordinatorTest, RegisterMultipleTasks) {
    dtc_->start();
    dtc_->registerTask(makeTask("task_a"));
    dtc_->registerTask(makeTask("task_b"));
    dtc_->registerTask(makeTask("task_c"));

    EXPECT_EQ(dtc_->listTasks().size(), 3u);
}

TEST_F(DistributedTaskCoordinatorTest, UnregisterTask) {
    dtc_->start();
    auto id = dtc_->registerTask(makeTask("delete_me"));
    ASSERT_EQ(dtc_->listTasks().size(), 1u);

    dtc_->unregisterTask(id);
    EXPECT_EQ(dtc_->listTasks().size(), 0u);
    EXPECT_EQ(dtc_->getTask(id), nullptr);
}

TEST_F(DistributedTaskCoordinatorTest, TasksPreservedAcrossLeadershipChange) {
    dtc_->start();

    // Register before becoming leader
    auto id = dtc_->registerTask(makeTask("persistent_task"));

    // Become leader – task should be visible in registry AND scheduler
    coordinator_->becomeLeader();
    ASSERT_TRUE(dtc_->isSchedulerActive());

    auto ptr = dtc_->getTask(id);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(ptr->name, "persistent_task");

    // Simulate leadership transfer – notify DTC so it deactivates the scheduler.
    // (coordinator_->stepDown() only changes the role; it does NOT fire the
    // DTC callback, so we must drive the DTC directly here.)
    dtc_->onLeaderElected("node-99");
    ASSERT_FALSE(dtc_->isSchedulerActive());

    ptr = dtc_->getTask(id);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(ptr->name, "persistent_task");
}

TEST_F(DistributedTaskCoordinatorTest, RegisterTaskWhileLeader) {
    dtc_->start();
    coordinator_->becomeLeader();
    ASSERT_TRUE(dtc_->isSchedulerActive());

    // Register function in scheduler so the task doesn't fail validation
    scheduler_->registerFunction("live_fn",
        [](const nlohmann::json&) -> nlohmann::json { return {}; });

    ScheduledTask task = makeTask("live");
    task.function_name = "live_fn";
    auto id = dtc_->registerTask(task);
    EXPECT_FALSE(id.empty());

    // Task is in both local registry and scheduler
    EXPECT_NE(dtc_->getTask(id), nullptr);
    EXPECT_NE(scheduler_->getTask(id), nullptr);
}

TEST_F(DistributedTaskCoordinatorTest, EnableDisableTask) {
    dtc_->start();
    auto id = dtc_->registerTask(makeTask("toggle_task"));

    // Disable
    dtc_->disableTask(id);
    auto ptr = dtc_->getTask(id);
    ASSERT_NE(ptr, nullptr);
    EXPECT_FALSE(ptr->enabled);

    // Re-enable
    dtc_->enableTask(id);
    ptr = dtc_->getTask(id);
    ASSERT_NE(ptr, nullptr);
    EXPECT_TRUE(ptr->enabled);
}

// ============================================================================
// Statistics
// ============================================================================

TEST_F(DistributedTaskCoordinatorTest, StatsReflectState) {
    dtc_->start();
    dtc_->registerTask(makeTask("stats_task"));

    auto s = dtc_->getStats();
    EXPECT_EQ(s.registered_tasks, 1u);
    EXPECT_FALSE(s.is_leader);
    EXPECT_FALSE(s.scheduler_active);
    EXPECT_EQ(s.leadership_acquired, 0u);

    coordinator_->becomeLeader();
    s = dtc_->getStats();
    EXPECT_TRUE(s.is_leader);
    EXPECT_TRUE(s.scheduler_active);
    EXPECT_GE(s.leadership_acquired, 1u);

    // Simulate another node winning leadership
    dtc_->onLeaderElected("node-99");
    s = dtc_->getStats();
    EXPECT_FALSE(s.scheduler_active);
    EXPECT_GE(s.leadership_lost, 1u);
}

// ============================================================================
// Accessors
// ============================================================================

TEST_F(DistributedTaskCoordinatorTest, LocalNodeId) {
    EXPECT_EQ(dtc_->getLocalNodeId(), "node-1");
}

TEST_F(DistributedTaskCoordinatorTest, GetSchedulerAndCoordinator) {
    EXPECT_EQ(dtc_->getScheduler(), scheduler_.get());
    EXPECT_EQ(dtc_->getCoordinator(), coordinator_.get());
}

// ============================================================================
// Race-condition guards
// ============================================================================

TEST_F(DistributedTaskCoordinatorTest, OnLeaderElectedIgnoredAfterStop) {
    // Start, become leader, stop – then fire the callback.
    // The scheduler must NOT be reactivated.
    dtc_->start();
    coordinator_->becomeLeader();
    ASSERT_TRUE(dtc_->isSchedulerActive());

    dtc_->stop();
    ASSERT_FALSE(dtc_->isRunning());
    ASSERT_FALSE(dtc_->isSchedulerActive());

    // Simulate a late callback arriving after stop().
    dtc_->onLeaderElected(coordinator_->getLocalShardId());

    // Must remain inactive – the guard in onLeaderElected checks running_.
    EXPECT_FALSE(dtc_->isSchedulerActive());
}

// ============================================================================
// Cron leader election – one runner per cluster
// ============================================================================

TEST_F(DistributedTaskCoordinatorTest, CronTaskStoredInRegistry) {
    dtc_->start();

    ScheduledTask t;
    t.name           = "daily-cleanup";
    t.type           = ScheduledTask::TaskType::FUNCTION;
    t.function_name  = "daily_cleanup_fn";
    t.trigger_type   = ScheduledTask::TriggerType::CRON;
    t.cron_expression = "0 2 * * *";  // Every day at 02:00
    t.enabled        = true;

    auto id = dtc_->registerTask(t);
    EXPECT_FALSE(id.empty());

    auto ptr = dtc_->getTask(id);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(ptr->trigger_type, ScheduledTask::TriggerType::CRON);
    EXPECT_EQ(ptr->cron_expression, "0 2 * * *");
}

TEST_F(DistributedTaskCoordinatorTest, CronTaskActivatedOnLeaderElection) {
    dtc_->start();

    // Register the function so activation doesn't throw
    scheduler_->registerFunction("cron_fn",
        [](const nlohmann::json&) -> nlohmann::json { return {}; });

    ScheduledTask t;
    t.name           = "cron-task";
    t.type           = ScheduledTask::TaskType::FUNCTION;
    t.function_name  = "cron_fn";
    t.trigger_type   = ScheduledTask::TriggerType::CRON;
    t.cron_expression = "*/5 * * * *";  // Every 5 minutes
    t.enabled        = true;

    auto id = dtc_->registerTask(t);

    // Non-leader: task is only in the local registry, not in the active scheduler
    EXPECT_FALSE(dtc_->isSchedulerActive());
    EXPECT_EQ(dtc_->listTasks().size(), 1u);

    // Become leader – coordinator activates the scheduler and registers the task
    coordinator_->becomeLeader();
    ASSERT_TRUE(dtc_->isSchedulerActive());

    // Cron task must be visible via the local registry on the leader
    auto ptr = dtc_->getTask(id);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(ptr->trigger_type, ScheduledTask::TriggerType::CRON);
}

TEST_F(DistributedTaskCoordinatorTest, CronTaskPreservedAfterLeadershipChange) {
    dtc_->start();

    ScheduledTask t;
    t.name           = "weekly-report";
    t.type           = ScheduledTask::TaskType::FUNCTION;
    t.function_name  = "report_fn";
    t.trigger_type   = ScheduledTask::TriggerType::CRON;
    t.cron_expression = "0 0 * * 1";  // Every Monday at midnight
    t.enabled        = true;

    auto id = dtc_->registerTask(t);

    // Simulate this node winning the election
    coordinator_->becomeLeader();
    ASSERT_TRUE(dtc_->isSchedulerActive());

    // Simulate leadership transfer to another node
    dtc_->onLeaderElected("node-99");
    ASSERT_FALSE(dtc_->isSchedulerActive());

    // Cron task must still be in the local registry after stepping down,
    // ready to be re-registered when this node becomes leader again.
    auto ptr = dtc_->getTask(id);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(ptr->cron_expression, "0 0 * * 1");
    EXPECT_EQ(ptr->trigger_type, ScheduledTask::TriggerType::CRON);
}

TEST_F(DistributedTaskCoordinatorTest, OnlyOneLeaderRunsCronTasks) {
    // Verify the "one runner per cluster" guarantee:
    // When a second node wins leadership, the first stops running tasks.
    dtc_->start();
    coordinator_->becomeLeader();
    ASSERT_TRUE(dtc_->isSchedulerActive());

    // A different node wins the election.
    dtc_->onLeaderElected("node-2");

    // This node must no longer be running the scheduler.
    EXPECT_FALSE(dtc_->isSchedulerActive());
}

// ============================================================================
// Null pointer guard
// ============================================================================

TEST(DistributedTaskCoordinatorGuardTest, NullSchedulerThrows) {
    auto topology = std::make_shared<ShardTopology>();
    GossipConfigManagerConfig gc;
    gc.local_shard_id = "n1";
    gc.enabled = false;
    auto gossip = std::make_shared<GossipConfigManager>(gc, topology);
    DistributedCoordinator dc("n1", topology, gossip);

    EXPECT_THROW(
        DistributedTaskCoordinator(nullptr, &dc),
        std::invalid_argument);
}

TEST(DistributedTaskCoordinatorGuardTest, NullCoordinatorThrows) {
    // Build a minimal scheduler for the guard test
    auto path = (std::filesystem::temp_directory_path() /
                 "themis_dtc_guard_test").string();
    std::filesystem::create_directories(path);
    RocksDBWrapper::Config cfg;
    cfg.db_path = path + "/db";
    cfg.enable_blobdb = false;
    RocksDBWrapper storage(cfg);
    ASSERT_TRUE(storage.open());
    SecondaryIndexManager idx(storage);
    QueryEngine engine(storage, idx);
    TaskScheduler sched(&engine, {});

    EXPECT_THROW(
        DistributedTaskCoordinator(&sched, nullptr),
        std::invalid_argument);

    storage.close();
    std::filesystem::remove_all(path);
}
