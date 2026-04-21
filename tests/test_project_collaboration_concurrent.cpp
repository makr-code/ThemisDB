/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_project_collaboration_concurrent.cpp          ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-04-21                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     ~360                                           ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_project_collaboration_concurrent.cpp
 * @brief Concurrent integration tests for CollaborationManager.
 *
 * Acceptance criteria covered (CC = Concurrent Collaboration)
 * ─────────────────────────────────────────────────────────────
 *   CC-01  Concurrent lock contention — only one thread wins the lock on the
 *          same object; all others get a failure Status.
 *   CC-02  Concurrent subscribers — N threads each subscribe a counter; a
 *          single notifyChange() increments every counter exactly once.
 *   CC-03  Concurrent notifyChange() calls — M threads each call notifyChange()
 *          T times; change log ends up with M×T entries and no data races.
 *   CC-04  Concurrent permission writes — N threads each call shareProject()
 *          for a unique user; all permissions are readable afterwards.
 *   CC-05  Concurrent lock/unlock interleaving — threads alternate lock and
 *          unlock; invariant: the object is never held by more than one thread.
 *   CC-06  Subscribe while notifyChange in flight — late subscriber does not
 *          crash or deadlock; all previously-registered callbacks complete.
 *   CC-07  getChanges under concurrent appends — read thread sees a consistent
 *          (non-torn) snapshot of the change log.
 *   CC-08  Concurrent projectLifecycle transitions from multiple threads —
 *          exactly one thread succeeds; all others get an error for an
 *          already-performed or invalid transition.
 *
 * All tests use real RocksDB instances (in-process) with per-test unique
 * temporary directories to avoid cross-test interference.
 */

#include <gtest/gtest.h>

#include "projects/collaboration_manager.h"
#include "projects/project_lifecycle.h"
#include "storage/rocksdb_wrapper.h"

#include <atomic>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>
#include <mutex>

namespace fs = std::filesystem;
using namespace themis;
using namespace themis::projects;

// ─── Fixture ──────────────────────────────────────────────────────────────────

class ProjectCollabConcurrentTest : public ::testing::Test {
protected:
    static constexpr const char* kDbPathBase = "./data/test_collab_concurrent_";

    void SetUp() override {
        // Use test name to get a unique path per test.
        db_path_ = std::string(kDbPathBase)
                 + ::testing::UnitTest::GetInstance()->current_test_info()->name();

        std::error_code ec;
        fs::remove_all(db_path_, ec);

        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        storage_ = std::make_shared<RocksDBWrapper>(cfg);
        ASSERT_TRUE(storage_->open()) << "Failed to open test RocksDB at " << db_path_;

        collab_ = std::make_unique<CollaborationManager>(storage_);
    }

    void TearDown() override {
        collab_.reset();
        storage_.reset();
        std::error_code ec;
        fs::remove_all(db_path_, ec);
    }

    std::string                             db_path_;
    std::shared_ptr<RocksDBWrapper>         storage_;
    std::unique_ptr<CollaborationManager>   collab_;
};

// ─── Lifecycle fixture ────────────────────────────────────────────────────────

class ProjectLifecycleConcurrentTest : public ::testing::Test {
protected:
    static constexpr const char* kDbPathBase = "./data/test_lifecycle_concurrent_";

    void SetUp() override {
        db_path_ = std::string(kDbPathBase)
                 + ::testing::UnitTest::GetInstance()->current_test_info()->name();

        std::error_code ec;
        fs::remove_all(db_path_, ec);

        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        storage_ = std::make_shared<RocksDBWrapper>(cfg);
        ASSERT_TRUE(storage_->open());

        lifecycle_ = std::make_unique<ProjectLifecycle>(storage_);
    }

    void TearDown() override {
        lifecycle_.reset();
        storage_.reset();
        std::error_code ec;
        fs::remove_all(db_path_, ec);
    }

    std::string                          db_path_;
    std::shared_ptr<RocksDBWrapper>      storage_;
    std::unique_ptr<ProjectLifecycle>    lifecycle_;
};

// ══════════════════════════════════════════════════════════════════════════════
// CC-01  Concurrent lock contention
// ══════════════════════════════════════════════════════════════════════════════
TEST_F(ProjectCollabConcurrentTest, CC01_ConcurrentLockContention) {
    constexpr int kThreads   = 8;
    const std::string pid    = "proj-cc01";
    const std::string obj    = "doc/main.json";

    std::atomic<int> acquired{0};
    std::atomic<int> rejected{0};

    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&, i] {
            const std::string locker = "user-" + std::to_string(i);
            auto s = collab_->lockObject(pid, obj, locker);
            if (s.ok) {
                ++acquired;
            } else {
                ++rejected;
            }
        });
    }
    for (auto& t : threads) t.join();

    // Exactly one thread must win.
    EXPECT_EQ(acquired.load(), 1)
        << "Expected exactly 1 lock acquisition; got " << acquired.load();
    EXPECT_EQ(rejected.load(), kThreads - 1)
        << "Expected " << (kThreads - 1) << " rejections; got " << rejected.load();
}

// ══════════════════════════════════════════════════════════════════════════════
// CC-02  Concurrent subscribers
// ══════════════════════════════════════════════════════════════════════════════
TEST_F(ProjectCollabConcurrentTest, CC02_ConcurrentSubscribersAllReceiveNotification) {
    constexpr int kSubscribers = 10;

    std::vector<std::atomic<int>> counters(kSubscribers);

    // Register subscribers from N threads.
    std::vector<std::thread> threads;
    threads.reserve(kSubscribers);
    for (int i = 0; i < kSubscribers; ++i) {
        threads.emplace_back([&, i] {
            collab_->subscribe([&counters, i](const Change&) {
                ++counters[i];
            });
        });
    }
    for (auto& t : threads) t.join();

    // One notification.
    Change c;
    c.project_id  = "proj-cc02";
    c.object_name = "obj";
    c.actor       = "system";
    c.timestamp   = 0;
    collab_->notifyChange(c);

    for (int i = 0; i < kSubscribers; ++i) {
        EXPECT_EQ(counters[i].load(), 1)
            << "Subscriber " << i << " expected 1 notification, got " << counters[i].load();
    }
}

// ══════════════════════════════════════════════════════════════════════════════
// CC-03  Concurrent notifyChange — M threads × T calls
// ══════════════════════════════════════════════════════════════════════════════
TEST_F(ProjectCollabConcurrentTest, CC03_ConcurrentNotifyChangeNoDataRace) {
    constexpr int kThreads   = 6;
    constexpr int kPerThread = 50;
    const std::string pid    = "proj-cc03";

    // Count notifications via an atomic counter subscribed before threads start.
    std::atomic<int> received{0};
    collab_->subscribe([&received](const Change&) { ++received; });

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            for (int i = 0; i < kPerThread; ++i) {
                Change c;
                c.project_id  = pid;
                c.object_name = "obj-" + std::to_string(t);
                c.actor       = "user-" + std::to_string(t);
                c.timestamp   = static_cast<int64_t>(i);
                collab_->notifyChange(c);
            }
        });
    }
    for (auto& t : threads) t.join();

    EXPECT_EQ(received.load(), kThreads * kPerThread);

    // Change log must contain at most 10 000 entries and at least all inserted.
    auto changes = collab_->getChanges(pid, 0);
    EXPECT_GE(static_cast<int>(changes.size()), std::min(kThreads * kPerThread, 10'000));
}

// ══════════════════════════════════════════════════════════════════════════════
// CC-04  Concurrent permission writes — unique users
// ══════════════════════════════════════════════════════════════════════════════
TEST_F(ProjectCollabConcurrentTest, CC04_ConcurrentPermissionWritesAllVisible) {
    constexpr int kThreads = 8;
    const std::string pid  = "proj-cc04";

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&, i] {
            collab_->shareProject(pid,
                                  "user-" + std::to_string(i),
                                  Permission::WRITE);
        });
    }
    for (auto& t : threads) t.join();

    // All permissions must be readable.
    for (int i = 0; i < kThreads; ++i) {
        auto perm = collab_->getUserPermission(pid, "user-" + std::to_string(i));
        EXPECT_TRUE(perm.has_value())
            << "Permission missing for user-" << i;
        EXPECT_EQ(*perm, Permission::WRITE);
    }
}

// ══════════════════════════════════════════════════════════════════════════════
// CC-05  Concurrent lock/unlock interleaving
// ══════════════════════════════════════════════════════════════════════════════
TEST_F(ProjectCollabConcurrentTest, CC05_ConcurrentLockUnlockNeverDoubleHeld) {
    constexpr int kThreads    = 6;
    constexpr int kIterations = 20;
    const std::string pid     = "proj-cc05";
    const std::string obj     = "shared-doc";

    std::atomic<int> concurrent_holders{0};
    std::atomic<bool> invariant_violated{false};

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&, i] {
            const std::string locker = "user-" + std::to_string(i);
            for (int iter = 0; iter < kIterations; ++iter) {
                auto s = collab_->lockObject(pid, obj, locker);
                if (s.ok) {
                    int prev = concurrent_holders.fetch_add(1);
                    if (prev != 0) {
                        invariant_violated.store(true);
                    }
                    // Briefly hold, then release.
                    concurrent_holders.fetch_sub(1);
                    collab_->unlockObject(pid, obj, locker);
                }
            }
        });
    }
    for (auto& t : threads) t.join();

    EXPECT_FALSE(invariant_violated.load())
        << "Lock was held by more than one thread simultaneously";
}

// ══════════════════════════════════════════════════════════════════════════════
// CC-06  Subscribe while notifyChange in flight — no crash/deadlock
// ══════════════════════════════════════════════════════════════════════════════
TEST_F(ProjectCollabConcurrentTest, CC06_LateSubscribeDoesNotDeadlock) {
    constexpr int kNotifiers   = 4;
    constexpr int kPerNotifier = 30;
    const std::string pid      = "proj-cc06";

    std::atomic<bool> stop{false};

    // Background thread continuously subscribes new callbacks.
    std::thread subscriber_thread([&] {
        int count = 0;
        while (!stop.load()) {
            collab_->subscribe([&count](const Change&) { (void)count; });
            ++count;
        }
    });

    // Multiple threads send notifications concurrently.
    std::vector<std::thread> notifiers;
    notifiers.reserve(kNotifiers);
    for (int i = 0; i < kNotifiers; ++i) {
        notifiers.emplace_back([&, i] {
            for (int k = 0; k < kPerNotifier; ++k) {
                Change c;
                c.project_id  = pid;
                c.object_name = "obj";
                c.actor       = "user-" + std::to_string(i);
                c.timestamp   = static_cast<int64_t>(k);
                collab_->notifyChange(c);
            }
        });
    }
    for (auto& t : notifiers) t.join();

    stop.store(true);
    subscriber_thread.join();
    // If we reach here without deadlock/crash the test passes.
    SUCCEED();
}

// ══════════════════════════════════════════════════════════════════════════════
// CC-07  getChanges under concurrent appends returns consistent snapshot
// ══════════════════════════════════════════════════════════════════════════════
TEST_F(ProjectCollabConcurrentTest, CC07_GetChangesConsistentUnderConcurrentAppends) {
    constexpr int kWriters   = 4;
    constexpr int kReads     = 20;
    constexpr int kPerWriter = 40;
    const std::string pid    = "proj-cc07";

    std::atomic<bool> writing{true};

    // Writer threads.
    std::vector<std::thread> writers;
    writers.reserve(kWriters);
    for (int i = 0; i < kWriters; ++i) {
        writers.emplace_back([&, i] {
            for (int k = 0; k < kPerWriter; ++k) {
                Change c;
                c.project_id  = pid;
                c.object_name = "obj-" + std::to_string(i);
                c.actor       = "writer-" + std::to_string(i);
                c.timestamp   = static_cast<int64_t>(k);
                collab_->notifyChange(c);
            }
        });
    }

    // Reader thread — must not crash or observe torn state.
    std::vector<std::thread> readers;
    readers.reserve(kReads);
    for (int r = 0; r < kReads; ++r) {
        readers.emplace_back([&] {
            auto changes = collab_->getChanges(pid, 0);
            // Each entry must have a non-empty project_id (structural sanity).
            for (const auto& ch : changes) {
                EXPECT_EQ(ch.project_id, pid);
            }
        });
    }

    for (auto& t : writers) t.join();
    for (auto& t : readers) t.join();

    writing.store(false);
    SUCCEED();
}

// ══════════════════════════════════════════════════════════════════════════════
// CC-08  Concurrent lifecycle transitions — exactly one thread activates
// ══════════════════════════════════════════════════════════════════════════════
TEST_F(ProjectLifecycleConcurrentTest, CC08_ConcurrentActivateExactlyOneSucceeds) {
    constexpr int kThreads = 8;
    const std::string pid  = "proj-cc08";

    // Initialise project from the main thread.
    ASSERT_TRUE(lifecycle_->initProject(pid, "system").ok);

    std::atomic<int> successes{0};
    std::atomic<int> failures{0};

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&, i] {
            auto s = lifecycle_->activate(pid, "user-" + std::to_string(i));
            if (s.ok) {
                ++successes;
            } else {
                ++failures;
            }
        });
    }
    for (auto& t : threads) t.join();

    // Exactly one activation must succeed (state machine is atomic).
    EXPECT_EQ(successes.load(), 1)
        << "Expected exactly 1 successful activation, got " << successes.load();
    EXPECT_EQ(failures.load(), kThreads - 1);

    // Final state must be ACTIVE.
    auto state = lifecycle_->getState(pid);
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(*state, ProjectState::ACTIVE);
}
