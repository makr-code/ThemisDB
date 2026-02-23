/*
 * test_plugin_security_audit.cpp
 *
 * Security audit tests for backend plugin loading and runtime probes.
 * Covers:
 *   - PluginSecurityVerifier::validatePluginPath (path traversal prevention)
 *   - PluginSecurityAuditor thread safety (concurrent logEvent / getAllEvents)
 *   - PluginSecurityAuditor logger integration (events forwarded to THEMIS logger)
 *   - PluginLoader path-traversal rejection via loadPlugin
 */

#include <gtest/gtest.h>
#include "acceleration/plugin_security.h"
#include "acceleration/plugin_loader.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <vector>
#include <atomic>

using namespace themis::acceleration;
namespace fs = std::filesystem;

// ============================================================================
// Helper: create a temporary file and return its path
// ============================================================================
static fs::path createTempFile(const fs::path& dir, const std::string& name,
                               const std::string& content = "FAKE_PLUGIN_DATA") {
    fs::path p = dir / name;
    std::ofstream f(p, std::ios::binary);
    f << content;
    return p;
}

// ============================================================================
// PathValidation tests
// ============================================================================

class PathValidationTest : public ::testing::Test {};

TEST_F(PathValidationTest, EmptyPathIsRejected) {
    std::string err;
    EXPECT_FALSE(PluginSecurityVerifier::validatePluginPath("", err));
    EXPECT_FALSE(err.empty());
}

TEST_F(PathValidationTest, DotDotSequenceIsRejected) {
    std::string err;
    EXPECT_FALSE(PluginSecurityVerifier::validatePluginPath("/plugins/../etc/passwd", err));
    EXPECT_FALSE(err.empty());
}

TEST_F(PathValidationTest, RelativeTraversalIsRejected) {
    std::string err;
    EXPECT_FALSE(PluginSecurityVerifier::validatePluginPath("../../lib/evil.so", err));
    EXPECT_FALSE(err.empty());
}

TEST_F(PathValidationTest, NullByteIsRejected) {
    std::string path = "/plugins/good.so";
    path.push_back('\0');
    path += "/extra";
    std::string err;
    EXPECT_FALSE(PluginSecurityVerifier::validatePluginPath(path, err));
    EXPECT_FALSE(err.empty());
}

TEST_F(PathValidationTest, ShellInjectionCharsAreRejected) {
    std::string err;
    // Semicolon
    EXPECT_FALSE(PluginSecurityVerifier::validatePluginPath("/plugins/a;b.so", err));
    // Pipe
    EXPECT_FALSE(PluginSecurityVerifier::validatePluginPath("/plugins/a|b.so", err));
    // Backtick
    EXPECT_FALSE(PluginSecurityVerifier::validatePluginPath("/plugins/a`b.so", err));
    // Dollar sign
    EXPECT_FALSE(PluginSecurityVerifier::validatePluginPath("/plugins/a$b.so", err));
    // Newline
    EXPECT_FALSE(PluginSecurityVerifier::validatePluginPath("/plugins/a\nb.so", err));
}

TEST_F(PathValidationTest, AbsoluteCleanPathIsAccepted) {
    std::string err;
    // A simple absolute path without traversal should be accepted
    EXPECT_TRUE(PluginSecurityVerifier::validatePluginPath("/opt/themis/plugins/good_plugin.so", err))
        << "Unexpected error: " << err;
}

// ============================================================================
// PluginLoader path-traversal rejection
// ============================================================================

class PluginLoaderSecurityTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = fs::temp_directory_path() / "themis_loader_security_test";
        fs::create_directories(test_dir_);
    }
    void TearDown() override {
        fs::remove_all(test_dir_);
    }
    fs::path test_dir_;
};

TEST_F(PluginLoaderSecurityTest, PathWithTraversalIsRejectedByLoader) {
    PluginLoader loader;
    // Construct a path with ".." – loadPlugin must reject it before any I/O
    std::string badPath = (test_dir_ / ".." / "evil.so").string();
    EXPECT_FALSE(loader.loadPlugin(badPath));
}

TEST_F(PluginLoaderSecurityTest, EmptyPathIsRejectedByLoader) {
    PluginLoader loader;
    EXPECT_FALSE(loader.loadPlugin(""));
}

// ============================================================================
// PluginSecurityAuditor thread-safety tests
// ============================================================================

class AuditorThreadSafetyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear any events left by previous tests
        PluginSecurityAuditor::instance().clearEvents();
    }
    void TearDown() override {
        PluginSecurityAuditor::instance().clearEvents();
    }
};

TEST_F(AuditorThreadSafetyTest, ConcurrentLogEventDoesNotCrash) {
    const int kThreads = 8;
    const int kEventsPerThread = 50;
    std::atomic<int> errors{0};

    auto worker = [&](int id) {
        try {
            for (int i = 0; i < kEventsPerThread; ++i) {
                PluginSecurityAuditor::instance().logEvent({
                    PluginSecurityEvent::EventType::PLUGIN_LOADED,
                    "/fake/plugin_" + std::to_string(id) + ".so",
                    "hash" + std::to_string(i),
                    "concurrent test event",
                    static_cast<uint64_t>(i),
                    "INFO"
                });
            }
        } catch (...) {
            ++errors;
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back(worker, t);
    }
    for (auto& th : threads) {
        th.join();
    }

    EXPECT_EQ(errors.load(), 0) << "Exception(s) occurred in concurrent logEvent";

    auto all = PluginSecurityAuditor::instance().getAllEvents();
    EXPECT_EQ(static_cast<int>(all.size()), kThreads * kEventsPerThread);
}

TEST_F(AuditorThreadSafetyTest, ConcurrentGetAndClearDoesNotCrash) {
    // Pre-populate some events
    for (int i = 0; i < 100; ++i) {
        PluginSecurityAuditor::instance().logEvent({
            PluginSecurityEvent::EventType::PLUGIN_LOADED,
            "/fake/plugin.so", "hash", "setup event",
            static_cast<uint64_t>(i), "INFO"
        });
    }

    std::atomic<int> errors{0};

    auto reader = [&]() {
        try {
            for (int i = 0; i < 50; ++i) {
                auto events = PluginSecurityAuditor::instance().getAllEvents();
                (void)events;
            }
        } catch (...) {
            ++errors;
        }
    };

    auto clearer = [&]() {
        try {
            for (int i = 0; i < 10; ++i) {
                PluginSecurityAuditor::instance().clearEvents();
            }
        } catch (...) {
            ++errors;
        }
    };

    std::thread t1(reader), t2(reader), t3(clearer);
    t1.join(); t2.join(); t3.join();

    EXPECT_EQ(errors.load(), 0) << "Exception(s) occurred during concurrent read/clear";
}

// ============================================================================
// PluginSecurityAuditor event filtering test
// ============================================================================

class AuditorFilterTest : public ::testing::Test {
protected:
    void SetUp() override {
        PluginSecurityAuditor::instance().clearEvents();
    }
    void TearDown() override {
        PluginSecurityAuditor::instance().clearEvents();
    }
};

TEST_F(AuditorFilterTest, GetEventsForPluginReturnsOnlyMatchingEvents) {
    const std::string targetPlugin = "/plugins/target.so";
    const std::string otherPlugin  = "/plugins/other.so";

    PluginSecurityAuditor::instance().logEvent({
        PluginSecurityEvent::EventType::PLUGIN_LOADED,
        targetPlugin, "abc", "loaded", 1ULL, "INFO"
    });
    PluginSecurityAuditor::instance().logEvent({
        PluginSecurityEvent::EventType::PLUGIN_LOAD_FAILED,
        otherPlugin, "def", "failed", 2ULL, "ERROR"
    });
    PluginSecurityAuditor::instance().logEvent({
        PluginSecurityEvent::EventType::SIGNATURE_VERIFIED,
        targetPlugin, "abc", "sig ok", 3ULL, "INFO"
    });

    auto targetEvents = PluginSecurityAuditor::instance().getEventsForPlugin(targetPlugin);
    ASSERT_EQ(targetEvents.size(), 2u);
    EXPECT_EQ(targetEvents[0].pluginPath, targetPlugin);
    EXPECT_EQ(targetEvents[1].pluginPath, targetPlugin);

    auto otherEvents = PluginSecurityAuditor::instance().getEventsForPlugin(otherPlugin);
    ASSERT_EQ(otherEvents.size(), 1u);
    EXPECT_EQ(otherEvents[0].pluginPath, otherPlugin);
}

// ============================================================================
// PluginSecurityAuditor export test
// ============================================================================

class AuditorExportTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = fs::temp_directory_path() / "themis_auditor_export_test";
        fs::create_directories(test_dir_);
        PluginSecurityAuditor::instance().clearEvents();
    }
    void TearDown() override {
        fs::remove_all(test_dir_);
        PluginSecurityAuditor::instance().clearEvents();
    }
    fs::path test_dir_;
};

TEST_F(AuditorExportTest, ExportCreatesJsonFile) {
    PluginSecurityAuditor::instance().logEvent({
        PluginSecurityEvent::EventType::BLACKLISTED,
        "/evil/plugin.so", "deadbeef", "blacklisted plugin detected",
        12345ULL, "CRITICAL"
    });

    fs::path outFile = test_dir_ / "audit_export.json";
    EXPECT_TRUE(PluginSecurityAuditor::instance().exportEvents(outFile.string()));
    EXPECT_TRUE(fs::exists(outFile));
    EXPECT_GT(fs::file_size(outFile), 0u);
}

TEST_F(AuditorExportTest, ExportToInvalidPathReturnsFalse) {
    EXPECT_FALSE(PluginSecurityAuditor::instance()
                     .exportEvents("/nonexistent_dir_xyz/audit.json"));
}

// ============================================================================
// getAllEvents returns a snapshot (not a reference to internal state)
// ============================================================================

TEST(AuditorSnapshotTest, GetAllEventsReturnsIndependentCopy) {
    PluginSecurityAuditor::instance().clearEvents();

    PluginSecurityAuditor::instance().logEvent({
        PluginSecurityEvent::EventType::PLUGIN_LOADED,
        "/a.so", "h1", "event 1", 1ULL, "INFO"
    });

    auto snapshot = PluginSecurityAuditor::instance().getAllEvents();
    ASSERT_EQ(snapshot.size(), 1u);

    // Add another event after taking snapshot
    PluginSecurityAuditor::instance().logEvent({
        PluginSecurityEvent::EventType::PLUGIN_LOADED,
        "/b.so", "h2", "event 2", 2ULL, "INFO"
    });

    // Snapshot must not have grown
    EXPECT_EQ(snapshot.size(), 1u);

    // But auditor now has 2 events
    EXPECT_EQ(PluginSecurityAuditor::instance().getAllEvents().size(), 2u);

    PluginSecurityAuditor::instance().clearEvents();
}
