/*
 * @file test_project_collaboration_concurrent.cpp
 * @brief Concurrent integration tests for CollaborationManager + InMemoryProjectAuditLog.
 *
 * Test IDs covered
 * ────────────────
 * Concurrent collaboration (CC):
 *   CC-01  Concurrent lockObject: only one thread wins the lock
 *   CC-02  Concurrent notifyChange: all change events are recorded under load
 *   CC-03  Concurrent subscribe + notifyChange: no subscriber callbacks lost
 *   CC-04  Concurrent shareProject: concurrent grants are idempotent
 *   CC-05  Lock contention: unlock by winner allows next waiter to acquire
 *   CC-06  Concurrent getChanges: read-heavy workload with concurrent writes
 *   CC-07  unsubscribeAll while notifyChange in flight does not crash
 *   CC-08  Mixed read/write: concurrent shareProject + getUserPermission
 *
 * Audit log (PAL):
 *   PAL-01  record + size: entry count reflects all records
 *   PAL-02  query by project_id: returns only matching entries
 *   PAL-03  query with action_filter: returns only matching action
 *   PAL-04  count is consistent with filtered query result size
 *   PAL-05  purge removes old entries for one project only
 *   PAL-06  setAuditLog wires notifyChange → audit entry recorded
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

#include "projects/collaboration_manager.h"
#include "projects/in_memory_project_audit_log.h"
#include "storage/rocksdb_wrapper.h"

namespace fs = std::filesystem;
using namespace themis;
using namespace themis::projects;
using sys_clock = std::chrono::system_clock;

// ─── Fixture ─────────────────────────────────────────────────────────────────

class ProjectCollaborationConcurrentTests : public ::testing::Test {
protected:
    static constexpr const char* kDbPath =
        "./data/test_projects_concurrent_db";

    void SetUp() override {
        std::error_code ec;
        fs::remove_all(kDbPath, ec);

        RocksDBWrapper::Config cfg;
        cfg.db_path = kDbPath;
        storage_    = std::make_shared<RocksDBWrapper>(cfg);
        ASSERT_TRUE(storage_->open()) << "Failed to open test RocksDB";

        cm_ = std::make_unique<CollaborationManager>(storage_);
    }

    void TearDown() override {
        cm_.reset();
        storage_.reset();
        std::error_code ec;
        fs::remove_all(kDbPath, ec);
    }

    Change makeChange(const std::string& project = "proj-A",
                      const std::string& obj     = "doc-1",
                      const std::string& actor   = "user-0") const
    {
        Change c;
        c.project_id  = project;
        c.object_name = obj;
        c.field_path  = "/title";
        c.old_value   = "old";
        c.new_value   = "new";
        c.timestamp   = sys_clock::to_time_t(sys_clock::now());
        c.actor       = actor;
        return c;
    }

    std::shared_ptr<RocksDBWrapper>        storage_;
    std::unique_ptr<CollaborationManager>  cm_;
};

// ══════════════════════════════════════════════════════════════════════════════
// CC — Concurrent collaboration tests
// ══════════════════════════════════════════════════════════════════════════════

// ── CC-01: Only one thread wins the lock ─────────────────────────────────────

TEST_F(ProjectCollaborationConcurrentTests, CC_01_ConcurrentLockOnlyOneWins)
{
    constexpr int kThreads = 8;
    std::atomic<int> winners{0};

    auto acquire = [&](int id) {
        const auto result = cm_->lockObject(
            "proj-lock", "doc-1", "locker-" + std::to_string(id));
        if (result.ok) ++winners;
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i)
        threads.emplace_back(acquire, i);
    for (auto& t : threads) t.join();

    EXPECT_EQ(winners.load(), 1)
        << "Exactly one thread must win the lock";
    EXPECT_TRUE(cm_->isLocked("proj-lock", "doc-1"));
}

// ── CC-02: All change events recorded under concurrent notifyChange ───────────

TEST_F(ProjectCollaborationConcurrentTests, CC_02_ConcurrentNotifyAllRecorded)
{
    constexpr int kThreads = 10;
    constexpr int kEach    = 20;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < kEach; ++i)
                cm_->notifyChange(
                    makeChange("proj-A", "doc-" + std::to_string(i),
                               "user-" + std::to_string(t)));
        });
    }
    for (auto& th : threads) th.join();

    const auto changes = cm_->getChanges("proj-A", 0);
    EXPECT_EQ(changes.size(), static_cast<size_t>(kThreads * kEach));
}

// ── CC-03: No subscriber callbacks lost under concurrent notify ───────────────

TEST_F(ProjectCollaborationConcurrentTests, CC_03_ConcurrentSubscribeNoCallbackLost)
{
    constexpr int kNotifiers  = 4;
    constexpr int kEventsEach = 25;
    constexpr int kTotal      = kNotifiers * kEventsEach;

    std::atomic<int> callback_count{0};

    cm_->subscribe([&](const Change&) { ++callback_count; });

    std::vector<std::thread> threads;
    threads.reserve(kNotifiers);
    for (int t = 0; t < kNotifiers; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < kEventsEach; ++i)
                cm_->notifyChange(
                    makeChange("proj-B", "doc-" + std::to_string(i),
                               "user-" + std::to_string(t)));
        });
    }
    for (auto& th : threads) th.join();

    EXPECT_EQ(callback_count.load(), kTotal);
}

// ── CC-04: Concurrent shareProject grants are idempotent ─────────────────────

TEST_F(ProjectCollaborationConcurrentTests, CC_04_ConcurrentShareIdempotent)
{
    constexpr int kThreads = 6;

    auto grant = [&](int id) {
        User u;
        u.id   = "user-shared";
        u.name = "Shared User " + std::to_string(id);
        cm_->shareProject("proj-C", {u}, Permission::WRITE);
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i)
        threads.emplace_back(grant, i);
    for (auto& t : threads) t.join();

    const auto perm = cm_->getUserPermission("proj-C", "user-shared");
    ASSERT_TRUE(perm.has_value());
    EXPECT_EQ(*perm, Permission::WRITE);
}

// ── CC-05: Unlock winner allows next waiter to acquire ───────────────────────

TEST_F(ProjectCollaborationConcurrentTests, CC_05_UnlockAllowsNextAcquire)
{
    // Thread 0 grabs the lock; thread 1 waits until lock is released,
    // then acquires it.
    std::atomic<bool> lock0_acquired{false};
    std::atomic<bool> lock0_released{false};
    std::atomic<bool> lock1_acquired{false};

    std::thread t0([&]() {
        auto r = cm_->lockObject("proj-D", "obj", "locker-0");
        EXPECT_TRUE(r.ok);
        lock0_acquired = true;
        // Spin until t1 has tried to acquire
        while (!lock1_acquired.load()) std::this_thread::yield();
        cm_->unlockObject("proj-D", "obj", "locker-0");
        lock0_released = true;
    });

    std::thread t1([&]() {
        // Wait for t0 to hold the lock
        while (!lock0_acquired.load()) std::this_thread::yield();
        // First attempt must fail (lock held by locker-0)
        auto r = cm_->lockObject("proj-D", "obj", "locker-1");
        EXPECT_FALSE(r.ok);
        lock1_acquired = true;
        // Wait for t0 to release
        while (!lock0_released.load()) std::this_thread::yield();
        // Second attempt must succeed
        auto r2 = cm_->lockObject("proj-D", "obj", "locker-1");
        EXPECT_TRUE(r2.ok);
    });

    t0.join();
    t1.join();
    EXPECT_TRUE(cm_->isLocked("proj-D", "obj"));
}

// ── CC-06: Concurrent read (getChanges) + write (notifyChange) ───────────────

TEST_F(ProjectCollaborationConcurrentTests, CC_06_ConcurrentReadWriteNoRace)
{
    constexpr int kWriters = 4;
    constexpr int kReaders = 4;
    constexpr int kEach    = 10;

    std::atomic<bool> stop{false};

    // Writers
    std::vector<std::thread> writers;
    writers.reserve(kWriters);
    for (int w = 0; w < kWriters; ++w) {
        writers.emplace_back([&, w]() {
            for (int i = 0; i < kEach; ++i)
                cm_->notifyChange(
                    makeChange("proj-E", "doc-" + std::to_string(i),
                               "writer-" + std::to_string(w)));
        });
    }

    // Readers — just verify no crash / exception during concurrent access
    std::vector<std::thread> readers;
    readers.reserve(kReaders);
    for (int r = 0; r < kReaders; ++r) {
        readers.emplace_back([&]() {
            for (int i = 0; i < kEach * 2; ++i) {
                (void)cm_->getChanges("proj-E", 0);
                std::this_thread::yield();
            }
        });
    }

    for (auto& t : writers) t.join();
    for (auto& t : readers) t.join();

    // All writes must be persisted
    const auto all = cm_->getChanges("proj-E", 0);
    EXPECT_EQ(all.size(), static_cast<size_t>(kWriters * kEach));
}

// ── CC-07: unsubscribeAll while notifyChange in flight does not crash ─────────

TEST_F(ProjectCollaborationConcurrentTests, CC_07_UnsubscribeAllWhileNotifyInFlight)
{
    std::atomic<int> invoked{0};
    cm_->subscribe([&](const Change&) {
        std::this_thread::sleep_for(std::chrono::microseconds(10));
        ++invoked;
    });

    std::atomic<bool> started{false};
    std::thread notifier([&]() {
        started = true;
        for (int i = 0; i < 20; ++i)
            cm_->notifyChange(makeChange("proj-F"));
    });

    std::thread unsub([&]() {
        while (!started.load()) std::this_thread::yield();
        cm_->unsubscribeAll();
    });

    notifier.join();
    unsub.join();

    // We just need no crash; invoked count is non-deterministic
    SUCCEED() << "No crash: invoked = " << invoked.load();
}

// ── CC-08: Mixed: concurrent shareProject + getUserPermission ─────────────────

TEST_F(ProjectCollaborationConcurrentTests, CC_08_ConcurrentShareAndRead)
{
    constexpr int kWriters = 4;
    constexpr int kReaders = 4;
    constexpr int kUsers   = 5;

    std::vector<std::thread> writers;
    writers.reserve(kWriters);
    for (int w = 0; w < kWriters; ++w) {
        writers.emplace_back([&, w]() {
            for (int u = 0; u < kUsers; ++u) {
                User user;
                user.id   = "user-" + std::to_string(u);
                user.name = "User " + std::to_string(u);
                cm_->shareProject("proj-G", {user}, Permission::READ);
            }
        });
    }

    std::vector<std::thread> readers;
    readers.reserve(kReaders);
    for (int r = 0; r < kReaders; ++r) {
        readers.emplace_back([&]() {
            for (int u = 0; u < kUsers * kWriters; ++u) {
                (void)cm_->getUserPermission(
                    "proj-G", "user-" + std::to_string(u % kUsers));
                std::this_thread::yield();
            }
        });
    }

    for (auto& t : writers) t.join();
    for (auto& t : readers) t.join();

    // After all writes: every user must have READ permission
    for (int u = 0; u < kUsers; ++u) {
        const auto perm = cm_->getUserPermission(
            "proj-G", "user-" + std::to_string(u));
        ASSERT_TRUE(perm.has_value()) << "user-" << u << " missing";
        EXPECT_EQ(*perm, Permission::READ);
    }
}

// ══════════════════════════════════════════════════════════════════════════════
// PAL — InMemoryProjectAuditLog tests
// ══════════════════════════════════════════════════════════════════════════════

static ProjectAuditEntry makeEntry(
    const std::string& project_id,
    ProjectAuditAction action,
    const std::string& actor_id = "alice",
    sys_clock::time_point ts    = sys_clock::now())
{
    ProjectAuditEntry e;
    e.entry_id    = "eid-" + project_id + "-" + actor_id;
    e.project_id  = project_id;
    e.action      = action;
    e.actor_id    = actor_id;
    e.actor_type  = "user";
    e.resource_id = "res-1";
    e.timestamp   = ts;
    return e;
}

// ── PAL-01: record + size ─────────────────────────────────────────────────────

TEST(InMemoryProjectAuditLogTests, PAL_01_RecordIncreasesSize)
{
    InMemoryProjectAuditLog log;
    EXPECT_EQ(log.size(), 0u);

    log.record(makeEntry("p1", ProjectAuditAction::PROJECT_CREATED));
    log.record(makeEntry("p1", ProjectAuditAction::DOCUMENT_INSERTED));
    log.record(makeEntry("p2", ProjectAuditAction::PERMISSION_GRANTED));

    EXPECT_EQ(log.size(), 3u);
}

// ── PAL-02: query by project_id returns only matching entries ─────────────────

TEST(InMemoryProjectAuditLogTests, PAL_02_QueryFiltersByProject)
{
    InMemoryProjectAuditLog log;
    log.record(makeEntry("proj-A", ProjectAuditAction::PROJECT_CREATED));
    log.record(makeEntry("proj-B", ProjectAuditAction::DOCUMENT_INSERTED));
    log.record(makeEntry("proj-A", ProjectAuditAction::DOCUMENT_UPDATED));

    AuditQueryOptions opts;
    opts.project_id = "proj-A";

    const auto result = log.query(opts);
    ASSERT_EQ(result.size(), 2u);
    for (const auto& e : result)
        EXPECT_EQ(e.project_id, "proj-A");
}

// ── PAL-03: query with action_filter ─────────────────────────────────────────

TEST(InMemoryProjectAuditLogTests, PAL_03_QueryFiltersByAction)
{
    InMemoryProjectAuditLog log;
    log.record(makeEntry("proj-X", ProjectAuditAction::DOCUMENT_INSERTED));
    log.record(makeEntry("proj-X", ProjectAuditAction::DOCUMENT_DELETED));
    log.record(makeEntry("proj-X", ProjectAuditAction::DOCUMENT_INSERTED));

    AuditQueryOptions opts;
    opts.project_id   = "proj-X";
    opts.action_filter = ProjectAuditAction::DOCUMENT_INSERTED;

    const auto result = log.query(opts);
    ASSERT_EQ(result.size(), 2u);
    for (const auto& e : result)
        EXPECT_EQ(e.action, ProjectAuditAction::DOCUMENT_INSERTED);
}

// ── PAL-04: count is consistent with query result ─────────────────────────────

TEST(InMemoryProjectAuditLogTests, PAL_04_CountConsistentWithQuery)
{
    InMemoryProjectAuditLog log;
    for (int i = 0; i < 10; ++i)
        log.record(makeEntry("proj-Y", ProjectAuditAction::DOCUMENT_UPDATED));
    for (int i = 0; i < 5; ++i)
        log.record(makeEntry("proj-Z", ProjectAuditAction::DOCUMENT_UPDATED));

    AuditQueryOptions opts;
    opts.project_id = "proj-Y";

    // count should equal query result size (ignoring limit/offset)
    opts.limit = 100;
    const auto result = log.query(opts);
    const auto cnt    = log.count(opts);
    EXPECT_EQ(cnt, result.size());
    EXPECT_EQ(cnt, 10u);
}

// ── PAL-05: purge removes entries before timestamp for one project ────────────

TEST(InMemoryProjectAuditLogTests, PAL_05_PurgeRemovesOldEntriesOneProject)
{
    InMemoryProjectAuditLog log;

    const auto epoch      = sys_clock::time_point{};
    const auto now        = sys_clock::now();
    const auto future     = now + std::chrono::hours(1);
    const auto long_ago   = epoch + std::chrono::seconds(1);

    // old entries for proj-1
    log.record(makeEntry("proj-1", ProjectAuditAction::PROJECT_CREATED, "alice", long_ago));
    log.record(makeEntry("proj-1", ProjectAuditAction::DOCUMENT_INSERTED, "bob", long_ago));
    // recent entry for proj-1
    log.record(makeEntry("proj-1", ProjectAuditAction::DOCUMENT_UPDATED, "carol", now));
    // old entry for proj-2 (must NOT be removed)
    log.record(makeEntry("proj-2", ProjectAuditAction::PROJECT_CREATED, "dave", long_ago));

    EXPECT_EQ(log.size(), 4u);

    const bool removed = log.purge("proj-1", now);
    EXPECT_TRUE(removed);
    EXPECT_EQ(log.size(), 2u);  // 1 recent proj-1 + 1 old proj-2

    AuditQueryOptions opts1;
    opts1.project_id = "proj-1";
    EXPECT_EQ(log.count(opts1), 1u);

    AuditQueryOptions opts2;
    opts2.project_id = "proj-2";
    EXPECT_EQ(log.count(opts2), 1u);
}

// ── PAL-06: setAuditLog wires notifyChange → audit entry recorded ─────────────

TEST_F(ProjectCollaborationConcurrentTests, PAL_06_AuditLogWiredViaSetAuditLog)
{
    auto audit = std::make_shared<InMemoryProjectAuditLog>();
    cm_->setAuditLog(audit);

    // Subscribe so notifyChange has a callback path too
    std::atomic<int> cb{0};
    cm_->subscribe([&](const Change&) { ++cb; });

    const Change c1 = makeChange("proj-H", "doc-2", "user-audit");
    const Change c2 = makeChange("proj-H", "doc-3", "user-audit");
    cm_->notifyChange(c1);
    cm_->notifyChange(c2);

    // Subscriber saw both
    EXPECT_EQ(cb.load(), 2);

    // Audit log recorded both
    AuditQueryOptions opts;
    opts.project_id   = "proj-H";
    opts.action_filter = ProjectAuditAction::DOCUMENT_UPDATED;
    EXPECT_EQ(audit->count(opts), 2u);

    // After clearAuditLog, further notifyChange does not affect old audit sink
    cm_->clearAuditLog();
    cm_->notifyChange(makeChange("proj-H", "doc-4", "user-audit"));
    EXPECT_EQ(audit->count(opts), 2u);  // still 2 — not incremented
}
